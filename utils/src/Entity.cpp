
#include <fmt/core.h>

#include "utils/EntityState.h"

void EntityState::kalmanUpdate(std::shared_ptr<EntityState> update) {

   // z measurement vector is update->facialFeatures
   // H is identity

   static FFMat I = FFMat::Identity();
   static FFMat* K = new FFMat();
   *K = facialFeaturesCov * (facialFeaturesCov + *update->getFacialFeaturesCov()).inverse();

   facialFeatures += *K * (update->facialFeatures - facialFeatures);
   facialFeaturesCov = (I - *K) * facialFeaturesCov;

}

void Entity::step(float dt) {

    const iGrid& pathMap = *_pathMap;

    glm::ivec2 ipos = { round(_pos.x), round(_pos.y) };

    int current = pathMap[ipos.x][ipos.y];
    if (current == 0) return;
    if (current == -2) {
        current = 1e3;
    }

    int li = 0, lj = 0;
    float step = 0.0;
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            if (
                !(i == 0 && j == 0) &&
                ipos.x + i >= 0 && ipos.x + i < pathMap.size() &&
                ipos.y + j >= 0 && ipos.y + j < pathMap[ipos.x].size() &&
                pathMap[ipos.x + i][ipos.y + j] != -2 &&
                current - pathMap[ipos.x + i][ipos.y + j] > step) {
                li = i;
                lj = j;
                int a = 0;
                step = current - pathMap[ipos.x + i][ipos.y + j];
                if (i * j == 0) {
                    step += 0.1;
                }
            }
        }
    }
    if (li == 0 && lj == 0) return;

    glm::vec2 dir = glm::normalize(glm::vec2(glm::ivec2(ipos.x + li, ipos.y + lj) - ipos));
    _pos += dir * dt;
    _heading = atan2f(dir.y, dir.x);

}
