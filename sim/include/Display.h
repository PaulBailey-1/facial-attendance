#pragma once

#include <vector>
#include <thread>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Map.h"
#include "EntityState.h"
#include "Device.h"

class Display : public ci::app::App {
public:

	Display();

	static Display* start();

	bool isRunning() { return getNumWindows() > 0; }

	void setObservables(const Map* map, const std::vector<EntityPtr>* entities, const std::vector<Device*>* devs) {
		_map = map;
		_entities = entities;
		_devices = devs;
	}

	void setup() override;
	void draw() override;

private:

	const float DOOR_WIDTH = 6.0; // ft

	const Map* _map = nullptr;
	const std::vector<EntityPtr>* _entities = nullptr;
	const std::vector<Device*>* _devices = nullptr;

	ci::Font _font;

};

