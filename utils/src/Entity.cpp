
#include <fmt/core.h>

#include "utils/EntityState.h"

void loadUpdateCov(std::string filename, FFMat& R) {
    fmt::print("Loading update covariance matrix from {} ... ", filename);
    try {
        std::ifstream file(filename);
        int updateNum = 0;
        int entity = 0;
        if (file.is_open()) {
            std::string line;
            int row = 0;
            while (1) {
                std::getline(file, line);
                if (file.eof()) break;
                std::stringstream s(line);
                std::string num;
                int col = 0;
                while (std::getline(s, num, ',')) {
                    if (row == col)
                       R(row, col) = stof(num);
                    col++;
                }
                if (col != FACE_VEC_SIZE) {
                    throw std::runtime_error("invalid column dimension\n");
                }
                row++;
            }
            if (row != FACE_VEC_SIZE) {
                throw std::runtime_error("invalid row dimension\n");
            }
        }
        printf("Done\n");
    }
    catch (const std::exception& err) {
        std::cerr << "Failed to read update covariance: " << err.what() << std::endl;
    }
}

double l2Distance(const FFVec& first, const FFVec& second) {
    float distance = 0.0;
    for (int j = 0; j < FACE_VEC_SIZE; j++) {
        double diff = first[j] - second[j];
        distance += diff * diff;
    }
    return distance;
}

void EntityState::kalmanUpdate(std::shared_ptr<EntityState> update) {

   // z measurement vector is update->facialFeatures
   // H is identity

   static FFMat I = FFMat::Identity();
   static FFMat* K = new FFMat();
   *K = facialFeaturesCov * (facialFeaturesCov + update->facialFeaturesCov).inverse();

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
