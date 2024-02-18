#include<algorithm>

#include <fmt/core.h>

#include "utils/Map.h"

DeviceView::DeviceView(int id_, glm::vec2 pos_, float angle_) {

    id = id_;
    pos = pos_;
    angle = angle_ * (M_PI / 180);

    view.moveTo({ 0, 0 });
    view.lineTo({ CAM_RANGE, 0 });
    view.arc({ 0, 0 }, CAM_RANGE, 0.0, CAM_ANGLE);
    view.close();
    glm::mat3 trans(1.0);
    trans = glm::translate(trans, pos);
    trans = glm::rotate(trans, float(-CAM_ANGLE / 2 - angle));
    view.transform(trans);

}

Map::Map(std::string filename) {

    fmt::print("Loading map from {} ... ", filename);

	ci::XmlTree doc(ci::loadFile(filename));
	ci::XmlTree map = doc.getChild("map");
	size.x = map.getAttributeValue<float>("width");
	size.y = map.getAttributeValue<float>("height");
	loadBounds(map, "inBound", inBounds);
	loadBounds(map, "outBound", outBounds);
	for (ci::XmlTree::Iter child = map.begin("device"); child != map.end(); ++child) {
		int id = child->getAttributeValue<int>("id");
		float x = child->getAttributeValue<float>("x");
		float y = child->getAttributeValue<float>("y");
		float angle = child->getAttributeValue<float>("direction");
		devs.push_back(DeviceView( id, {x, y}, angle));
	}
    std::sort(devs.begin(), devs.end(), [](const DeviceView& a, const DeviceView& b) {return a.id < b.id;});
	for (ci::XmlTree::Iter child = map.begin("door"); child != map.end(); ++child) {
		int id = child->getAttributeValue<int>("id");
		float x = child->getAttributeValue<float>("x");
		float y = child->getAttributeValue<float>("y");
		float angle = child->getAttributeValue<float>("angle");
		doors.push_back(Door{ id, {x, y}, angle });
	}

    printf("Done\n");
}

ci::Shape2d Map::makeRect(glm::vec2 topLeft, float width, float height) {
	ci::Shape2d shape;
	shape.moveTo(topLeft);
	shape.lineTo(glm::vec2(topLeft.x + width, topLeft.y));
	shape.lineTo(glm::vec2(topLeft.x + width, topLeft.y + height));
	shape.lineTo(glm::vec2(topLeft.x, topLeft.y + height));
	shape.close();
	return shape;
}

void Map::loadBounds(ci::XmlTree map, std::string name, std::vector<ci::Shape2d>& bounds) {
	for (ci::XmlTree::Iter child = map.begin(name); child != map.end(); ++child) {
		float x = child->getAttributeValue<float>("x");
		float y = child->getAttributeValue<float>("y");
		float width = child->getAttributeValue<float>("width");
		float height = child->getAttributeValue<float>("height");
		bounds.push_back(makeRect({ x, y }, width, height));
	}
}

void Map::generatePathMaps(std::vector<std::set<int>>& matches) {
    bool getMatches = matches.size() == 0;
	for (Door door : doors) {
        _pathMaps.push_back(iGrid());
        if (getMatches) {
            std::set<int> devs;
            createPathMap(_pathMaps.back(), door.pos, devs);
            matches.push_back(devs);
        } else {
            createPathMap(_pathMaps.back(), door.pos);
        }
	}
}

void Map::getDeviceConnections(std::vector<std::set<int>>& conns) {
	for (DeviceView dev : devs) {
        std::set<int> connectedDevs;
        iGrid pathMap = iGrid();
        createPathMap(pathMap, dev.pos, connectedDevs, dev.id);
        conns.push_back(connectedDevs);
	}
}

void Map::createPathMap(iGrid& pathMap, glm::ivec2 end, std::set<int>& boundingDevs, int excludeDev) {

    bool bounding = boundingDevs.size() == 0;

    std::vector<int> col(size.y + 1, -2);
    pathMap = iGrid(size.x + 1, col);

    for (int x = 0; x < pathMap.size(); x++) {
        for (int y = 0; y < pathMap[x].size(); y++) {
            glm::vec2 here = { x, y };
            for (const ci::Shape2d& inBound : inBounds) {
                if (inBound.contains(here)) {
                    pathMap[x][y] = -1;
                    break;
                }
            }
            if (bounding) {
                for (DeviceView& devView : devs) {
                    if (devView.id != excludeDev && devView.view.contains(here)) {
                        pathMap[x][y] = -devView.id - 3;
                        break;
                    }
                }
            }
            for (const ci::Shape2d& outBound : outBounds) {
                if (outBound.contains(here)) {
                    pathMap[x][y] = -2;
                    break;
                }
            }
        }
    }

    pathMap[end.x][end.y] = 0;

    int set = 0;
    while (1) {
        auto pathMapCopy = pathMap;
        int setNow = 0;
        for (int x = 0; x < pathMap.size(); x++) {
            for (int y = 0; y < pathMap[x].size(); y++) {
                if (pathMap[x][y] >= 0) {
                    setNow++;
                    for (int i = -1; i < 2; i++) {
                        for (int j = -1; j < 2; j++) {
                            if (!(i == 0 && j == 0) &&
                                x + i >= 0 && x + i < pathMap.size() &&
                                y + j >= 0 && y + j < pathMap[x].size()) {
                                
                                int adjVal = pathMap[x + i][y + j];
                                if (adjVal == -1) {
                                    pathMapCopy[x + i][y + j] = pathMap[x][y] + 1;
                                }
                                if (bounding && adjVal < -2) {
                                    boundingDevs.insert(-adjVal - 3);
                                }
                            }
                        }
                    }
                }
            }
        }
        pathMap = pathMapCopy;
        if (setNow == set) break;
        set = setNow;
    }
}
