#pragma once

#include <vector>

#include "Map.h"
#include "Device.h"
#include "Display.h"
#include "DBConnection.h"
#include "EntityState.h"

class Simulation {
public:

	Simulation();

	void run();

private:

	DBConnection _db;

	Map _map;
	Display* _display;
	std::vector<Device*> _devices;
	std::vector<EntityPtr> _entities;

};