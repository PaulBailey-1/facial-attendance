#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <cinder/Shape2d.h>

struct Door {
	int id;
	glm::vec2 pos;
	float angle;
	Door(int id_, glm::vec2 pos_, float angle_) : id(id_), pos(pos_), angle(angle_) {}
};
struct DeviceLoc {
	int id;
	glm::vec2 pos;
	float angle;
};
struct Map {
	glm::vec2 size;
	std::vector<ci::Shape2d> inBounds;
	std::vector<ci::Shape2d> outBounds;
	std::vector<DeviceLoc> devs;
	std::vector<Door> doors;
};