
#include <chrono>

#include "Simulation.h"

Simulation::Simulation() {

	if (_db.connect()) {
		//_db.createTables();
		_db.getEntities(_entities);
	}

	printf("Loading map ... ");

	_map = Map("../../../../map.xml");
	_map.generatePathMaps();

	printf("Done\nCreating devices ... ");

	for (DeviceView devView : _map.devs) {
		_devices.push_back(new Device(devView));
	}

	printf("Done\n");

	_display = Display::start();
	_display->setObservables(&_map, &_entities);
}

void Simulation::run() {
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	printf("Running simulation...\n");

	int period = 1;

	for (EntityPtr entity : _entities) {
		entity->setPathMap(_map.getPathMap(entity->getNextDoor(period)));
		entity->setStartPos(_map.doors[0].pos);
	}

	while (1) {
		for (EntityPtr entity : _entities) {
			entity->step(0.1);
		}
		for (Device* dev : _devices) {
			dev->run(_entities);
		}
		if (_db.getPeriod() == period) {
			if (period == 3) {
				period = 1;
			} else {
				period++;
			}
			for (EntityPtr entity : _entities) {
				entity->setPathMap(_map.getPathMap(entity->getNextDoor(period)));
				if (period == 1) {
					entity->setStartPos(_map.doors[0].pos);
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}