#pragma once

#include <vector>
#include <set>

#include <glm/glm.hpp>

#include <utils/EntityState.h>
#include <utils/DBConnection.h>
#include <utils/Map.h>

class Device : public DeviceView {
    
public:

	Device(DeviceView view);

	void run(const std::vector<EntityPtr> &entites);

private:

	DBConnection _db;

	std::set<int> _seenEntities;

};