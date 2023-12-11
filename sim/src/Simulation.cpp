
#include <chrono>

#include <fmt/core.h>

#include "Simulation.h"

Simulation::Simulation() {

	_db.connect();

	_db.createTables();

	printf("Loading map ... ");

	_map = Map("../../../../map.xml");
	_map.generatePathMaps();

	printf("Done\n");

	uploadDataSet("../../../../dataset.csv", 9, 12);
	_db.getEntities(_entities);
	getSchedules();

	printf("Creating devices ... ");

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

void Simulation::uploadDataSet(std::string filename, int entities, int imgs) {

	if (_db.getEntityFeatures(EntityPtr(new Entity(1)), 0)) {
		return;
	}

	fmt::print("Uploading dataset to db from {} ... ", filename);
	try {
		std::ifstream file(filename);
		int updateNum = 0;
		int entity = _db.addStudent();
		if (file.is_open()) {
			std::string line;
			while (1) {
				std::getline(file, line);
				if (file.eof()) break;
				std::stringstream s(line);
				std::string num;
				UpdatePtr data = UpdatePtr(new Update(0, updateNum));
				int feature = 0;
				while (std::getline(s, num, ',')) {
					data->facialFeatures[feature] = stof(num);
					feature++;
				}
				if (feature != FACE_VEC_SIZE) {
					throw std::runtime_error("improper feature vector dimensions\n");
				}
				_db.pushStudentData(data, entity);
				updateNum++;
				if (updateNum == imgs) {
					updateNum = 0;
					entity = _db.addStudent();
				}
			}
			if (updateNum != 0) {
				throw std::runtime_error("inproper alignment\n");
			}
			if (entity == entities) {
				throw std::runtime_error("unexpected number of entities\n");
			}
		}
		file.close();
		printf("Done\n");
	}
	catch (const std::exception& err) {
		std::cerr << "Failed to read dataset: " << err.what() << std::endl;
	}
}

void Simulation::getSchedules() {

	std::vector<Schedule> check;
	_db.getSchedules(check);
	if (check.size() != 0) {
		for (int i = 0; i < check.size(); i++) {
			_entities[i]->schedule = check[i].rooms;
		}
		return;
	}

	printf("Creating schedules ... ");

	for (EntityPtr entity : _entities) {
		for (int i = 1; i <= 7; i++) {
			int classroom;
			bool good = false;
			while(!good) {
				classroom = rand() % _map.doors.size();
				good = true;
				for (int cr : entity->schedule) {
					if (cr == classroom) good = false;
				}
			}
			entity->schedule.push_back(classroom);
			_db.addToSchedule(entity->id, i, classroom);
		}
	}

	printf("Done\n");
}