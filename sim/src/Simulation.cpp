
#include <chrono>

#include <fmt/core.h>

#include "Simulation.h"

Simulation::Simulation() {

    PathGraph::initGraph("../../../map.xml", "pathGraph.csv");

	_db.connect();

	_db.clearTables();
	_db.createTables();
    _db.initGlobals();

	_map = Map("../../../map.xml");
	_map.generatePathMaps();

	uploadDataSet("../../../dataset.csv", 1);
	_db.getEntities(_entities);
	srand(56789765);
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

	int period = 0;

	while (1) {
		if (_db.getPeriod() != period) {
			period = _db.getPeriod();
			for (EntityPtr entity : _entities) {
				entity->setPathMap(_map.getPathMap(entity->getNextDoor(period)));
				if (period == 1) {
					entity->setStartPos(_map.doors[0].pos);
				}
			}
		}
		for (EntityPtr entity : _entities) {
			entity->step(0.2);
		}
		for (Device* dev : _devices) {
			dev->run(_entities);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void Simulation::uploadDataSet(std::string filename, int max) {

	if (_db.getEntityFeatures(EntityPtr(new Entity(1)), 0)) {
		return;
	}

	std::vector<std::vector<std::vector<float>>> dataSet;
	std::vector<std::vector<float>> currentEntity;
	fmt::print("Uploading dataset to db from {} ... ", filename);

	try {
		std::ifstream file(filename);
		int updateNum = 0;
		int entity = 0;
		if (file.is_open()) {
			std::string line;

			std::getline(file, line);
			std::stringstream s(line);
			std::string num;
			std::getline(s, num, ',');
			int entities = stoi(num);
			std::getline(s, num, ',');
			int imgs = stoi(num);

			while (1) {
				std::getline(file, line);
				if (file.eof()) break;
				s = std::stringstream(line);
				UpdatePtr data = UpdatePtr(new Update(0, updateNum));
				int feature = 0;
				std::vector<float> vec;
				while (std::getline(s, num, ',')) {
					data->facialFeatures[feature] = stof(num);
					feature++;
					vec.push_back(stof(num));
				}
				currentEntity.push_back(vec);
				if (feature != FACE_VEC_SIZE) {
					throw std::runtime_error("improper feature vector dimensions\n");
				}
				if (updateNum == 0) {
					entity = _db.addStudent();
				}
				_db.pushStudentData(data, entity);
				updateNum++;
				if (updateNum == imgs) {
					updateNum = 0;
					dataSet.push_back(currentEntity);
					currentEntity.clear();
				}
				if (dataSet.size() == max) break;
			}
			if (updateNum != 0) {
				throw std::runtime_error("inproper alignment\n");
			}
			if (entity != entities && max == -1) {
				throw std::runtime_error("unexpected number of entities\n");
			}
		}
		file.close();
		printf("Done\n");

		std::ofstream distancesFile("distances.csv");
		for (int entity = 0; entity < dataSet.size() - 1; entity++) {
			for (int otherEntity = entity + 1; otherEntity < dataSet.size(); otherEntity++) {
				for (int i = 0; i < dataSet[entity].size(); i++) {
					for (int j = 0; j < dataSet[otherEntity].size(); j++) {
						float distance = 0.0;
						for (int k = 0; k < FACE_VEC_SIZE; k++) {
							distance += pow(dataSet[entity][i][k] - dataSet[otherEntity][j][k], 2);
						}
						if (distance != 0.0) {
							distancesFile << std::to_string(distance) << ", ";
						}
					}
				}
			}
		}
		distancesFile << "\n";
		for (int entity = 0; entity < dataSet.size(); entity++) {
			for (int i = 0; i < dataSet[entity].size() - 1; i++) {
				for (int j = i + 1; j < dataSet[entity].size(); j++) {
					float distance = 0.0;
					for (int k = 0; k < FACE_VEC_SIZE; k++) {
						distance += pow(dataSet[entity][i][k] - dataSet[entity][j][k], 2);
					}
					if (distance != 0.0) {
						distancesFile << std::to_string(distance) << ", ";
					}
				}
			}
		}

		distancesFile << "\n";
		distancesFile.close();

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