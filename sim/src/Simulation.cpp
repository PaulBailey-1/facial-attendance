
#include <chrono>

#include "Simulation.h"

ci::Shape2d makeRect(glm::vec2 topLeft, float width, float height) {
	ci::Shape2d shape;
	shape.moveTo(topLeft);
	shape.lineTo(glm::vec2(topLeft.x + width, topLeft.y));
	shape.lineTo(glm::vec2(topLeft.x + width, topLeft.y + height));
	shape.lineTo(glm::vec2(topLeft.x, topLeft.y + height));
	shape.close();
	return shape;
}

Simulation::Simulation() {

	if (_db.connect()) {
		//_db.createTables();
		_db.getEntities(_entities);
	}

	printf("Loading map ... ");

	_map.size = {100, 60};

	_map.inBounds.push_back(makeRect({0, 0}, 100, 60));
	_map.outBounds.push_back(makeRect({10, 10}, 80, 40));

	_map.devs.push_back(DeviceLoc{ 0, { 50, 5 }, 180 });
	_map.devs.push_back(DeviceLoc{ 1, { 50, 5 }, 0 });
	_map.devs.push_back(DeviceLoc{2, { 95, 30 }, 90});
	_map.devs.push_back(DeviceLoc{3, { 95, 30 }, -90});
	_map.devs.push_back(DeviceLoc{4, { 50, 55 }, 0});
	_map.devs.push_back(DeviceLoc{5, { 50, 55 }, 180});
	_map.devs.push_back(DeviceLoc{6, { 5, 30 }, -90});
	_map.devs.push_back(DeviceLoc{7, { 5, 30 }, 90});

	_map.doors.push_back(Door(0, { 20, 0 }, 0));
	_map.doors.push_back(Door(1, { 40, 0 }, 0));
	_map.doors.push_back(Door(2, { 60, 0 }, 0));
	_map.doors.push_back(Door(3, { 80, 0 }, 0));

	_map.doors.push_back(Door(4, { 100, 20 }, 90));
	_map.doors.push_back(Door(5, { 100, 40 }, 90));

	_map.doors.push_back(Door(6, { 80, 60 }, 0));
	_map.doors.push_back(Door(7, { 60, 60 }, 0));
	_map.doors.push_back(Door(8, { 40, 60 }, 0));
	_map.doors.push_back(Door(9, { 20, 60 }, 0));

	_map.doors.push_back(Door(10, { 0, 40 }, 90));
	_map.doors.push_back(Door(11, { 0, 20 }, 90));

	_map.doors.push_back(Door(12, { 10, 30 }, 90));
	_map.doors.push_back(Door(13, { 90, 30 }, 90));

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