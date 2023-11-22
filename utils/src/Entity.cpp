
#include "EntityState.h"


void Entity::generatePathMaps(const Map& map) {

    _startPositions.clear();
    for (int i = 0; i < _schedule.size(); i++) {
        glm::ivec2 startPos = { map.doors[_schedule[i]].pos.x, map.doors[_schedule[i]].pos.y };
        _startPositions.push_back(startPos);
        if (i == 0) continue;
        iGrid pathMap;
        createPathMap(map, pathMap, startPos);
        _pathMaps.push_back(pathMap);
    }

}

void Entity::createPathMap(const Map& map, iGrid& pathMap, glm::ivec2 end) {

    std::vector<int> col(map.size.y + 1, -2);
    pathMap = iGrid(map.size.x + 1, col);

    for (int x = 0; x < pathMap.size(); x++) {
        for (int y = 0; y < pathMap[x].size(); y++) {
            for (const ci::Shape2d& inBound : map.inBounds) {
                if (inBound.contains(glm::vec2(x, y))) {
                    pathMap[x][y] = -1;
                    break;
                }
            }
            for (const ci::Shape2d& outBound : map.outBounds) {
                if (outBound.contains(glm::vec2(x, y))) {
                    pathMap[x][y] = -2;
                    break;
                }
            }
        }
    }

    pathMap[end.x][end.y] = 0;

    while (1) {
        auto pathMapCopy = pathMap;
        int unset = 0;
        for (int x = 0; x < pathMap.size(); x++) {
            for (int y = 0; y < pathMap[x].size(); y++) {
                if (pathMap[x][y] == -1) {
                    unset++;
                    for (int i = -1; i < 2; i++) {
                        for (int j = -1; j < 2; j++) {
                            if (!(i == 0 && j == 0) &&
                                x + i >= 0 && x + i < pathMap.size() &&
                                y + j >= 0 && y + j < pathMap[x].size() &&
                                pathMap[x + i][y + j] >= 0) {
                                pathMapCopy[x][y] = pathMap[x + i][y + j] + 1;
                            }
                        }
                    }
                }
            }
        }
        pathMap = pathMapCopy;
        if (unset == 0) break;
    }
}

void Entity::loadPathMap(int period) {
    _period = period;
    _pos = _startPositions[period - 1];
}

void Entity::step(float dt) {

    iGrid& pathMap = _pathMaps[_period - 1];

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

}