#pragma once

#include <vector>

#include <utils/Map.h>
#include <utils/DBConnection.h>
#include <utils/EntityState.h>

#include "Device.h"
#include "Display.h"

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

	void uploadDataSet(std::string filename, int max = -1, bool startingData = false);
	void getSchedules();

};