#pragma once

#include <vector>
#include <thread>

#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>

#include <utils/Map.h>
#include <utils/EntityState.h>

#include "Device.h"

class Display : public ci::app::App {
public:

	Display();

	static Display* start();

	bool isRunning() { return getNumWindows() > 0; }

	void setObservables(const Map* map, const std::vector<EntityPtr>* entities) {
		_map = map;
		_entities = entities;
	}

	void setup() override;
	void draw() override;

private:

	const float DOOR_WIDTH = 6.0; // ft

	const Map* _map = nullptr;
	const std::vector<EntityPtr>* _entities = nullptr;

	ci::Font _font;

};

