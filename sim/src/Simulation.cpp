
#include <chrono>

#include "Simulation.h"

Simulation::Simulation() {

	if (_db.connect()) {
		//_db.createTables();
		_db.getEntities(_entities);
	}

	printf("Loading map ... ");

	_map = Map("../../../../map.xml");

	printf("Done\nCreating devices ... ");

	for (DeviceLoc devLoc : _map.devs) {
		_devices.push_back(new Device(devLoc));
	}

	printf("Done\nGenerating pathmaps ... ");

	for (EntityPtr entity : _entities) {
		entity->generatePathMaps(_map);
	}

	printf("Done\n");

	_display = Display::start();
	_display->setObservables(&_map, &_entities, &_devices);
}

void Simulation::run() {
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	printf("Running simulation...\n");

	for (EntityPtr entity : _entities) {
		entity->loadPathMap(1);
	}

	while (1) {
		for (EntityPtr entity : _entities) {
			entity->step(0.1);
		}
		for (Device* dev : _devices) {
			dev->run(_entities);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}