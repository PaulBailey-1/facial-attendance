#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <cinder/Shape2d.h>
#include <cinder/Xml.h>

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

	ci::Shape2d makeRect(glm::vec2 topLeft, float width, float height) {
		ci::Shape2d shape;
		shape.moveTo(topLeft);
		shape.lineTo(glm::vec2(topLeft.x + width, topLeft.y));
		shape.lineTo(glm::vec2(topLeft.x + width, topLeft.y + height));
		shape.lineTo(glm::vec2(topLeft.x, topLeft.y + height));
		shape.close();
		return shape;
	}

	void loadBounds(ci::XmlTree map,  std::string name, std::vector<ci::Shape2d>& bounds) {
		for (ci::XmlTree::Iter child = map.begin(name); child != map.end(); ++child) {
			float x = child->getAttributeValue<float>("x");
			float y = child->getAttributeValue<float>("y");
			float width = child->getAttributeValue<float>("width");
			float height = child->getAttributeValue<float>("height");
			bounds.push_back(makeRect({ x, y }, width, height));
		}
	}

	Map() {}

	Map(std::string filename) {
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
			devs.push_back(DeviceLoc{id, {x, y},  angle});
		}
		for (ci::XmlTree::Iter child = map.begin("door"); child != map.end(); ++child) {
			int id = child->getAttributeValue<int>("id");
			float x = child->getAttributeValue<float>("x");
			float y = child->getAttributeValue<float>("y");
			float angle = child->getAttributeValue<float>("angle");
			doors.push_back(Door{ id, {x, y}, angle });
		}
	}
};