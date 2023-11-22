#pragma once

#include <vector>
#include <set>

#include <glm/glm.hpp>
#include <cinder/Shape2d.h>

#include "EntityState.h"
#include "DBConnection.h"

#include "Map.h"

class Device {
    
public:

	Device(DeviceLoc loc);

	void run(const std::vector<EntityPtr> &entites);

    const ci::Shape2d& getView() const {return _view;}

private:

	const float CAM_ANGLE = 60 * (M_PI / 180); // degrees
	const float CAM_RANGE = 20; // ft

	DBConnection _db;

	int _id;
	glm::vec2 _pos;
	ci::Shape2d _view;

	std::set<int> _seenEntities;

};