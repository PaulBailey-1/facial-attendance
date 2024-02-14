#pragma once

#include <vector>
#include <set>

#include <glm/glm.hpp>
#include <cinder/Shape2d.h>
#include <cinder/Xml.h>

typedef std::vector<std::vector<int>> iGrid;

struct Door {
	int id;
	glm::vec2 pos;
	float angle;
	Door(int id_, glm::vec2 pos_, float angle_) : id(id_), pos(pos_), angle(angle_) {}
};
struct DeviceView {
	int id;
	glm::vec2 pos;
	float angle;

	ci::Shape2d view;

	const float CAM_ANGLE = 60 * (M_PI / 180); // degrees
	const float CAM_RANGE = 20; // ft

	DeviceView(int id_, glm::vec2 pos_, float angle_);

	bool operator=(const DeviceView& other) {
		return id == other.id;
	}

};
class Map {
public:

	glm::vec2 size;
	std::vector<ci::Shape2d> inBounds;
	std::vector<ci::Shape2d> outBounds;
	std::vector<DeviceView> devs;
	std::vector<Door> doors;

	Map() {}
	Map(std::string filename);

	void generatePathMaps(std::vector<std::set<int>>& matches = std::vector<std::set<int>>(1));
	void getDeviceConnections(std::vector<std::set<int>>& conns);

	const iGrid* getPathMap(int room) const { return &_pathMaps[room]; }

private:

	ci::Shape2d makeRect(glm::vec2 topLeft, float width, float height);
	void loadBounds(ci::XmlTree map, std::string name, std::vector<ci::Shape2d>& bounds);
	void createPathMap(iGrid& pathMap, glm::ivec2 end, std::set<int>& boundingDevs = std::set<int>({0}));

	std::vector<iGrid> _pathMaps;

};