
#include <string>

#include "Display.h"

bool displayRunning = false;

Display::Display() {
	displayRunning = true;
}

Display* Display::start() {

	printf("Starting display ... ");

	std::thread thread([] {
		cinder::app::RendererRef renderer(new ci::app::RendererGl);
		cinder::app::AppMsw::main<Display>(renderer, "Simulation");
	});
	thread.detach();
	while (cinder::app::AppMsw::get() == nullptr || !displayRunning) {}

	printf("Done\n");

	return (Display*) cinder::app::AppMsw::get();
}

void Display::setup() {
	_font = ci::Font("Times New Roman", 18);

	_db.connect();

}

void Display::update() {

	static int limiter = 0;

	if (limiter == 10) {
		limiter = 0;
		_shortTermStates.clear();
		_db.getShortTermStates(_shortTermStates, true);
		_particles.clear();
		_db.getParticles(_particles);
		if (_shortTermStates.size() > 0 && _shortTermStates[0]->longTermStateKey != -1) {
			_pathGraph = _db.getLtsPath(_shortTermStates[0]->longTermStateKey, _db.getPeriod());
		}
	}
	limiter++;
}

void Display::draw() {
	ci::ColorA grey = ci::Color::gray(0.4);
	static ci::TextBox tboxBase = ci::TextBox().alignment(ci::TextBox::CENTER).font(_font).size(glm::ivec2(50, 15));

	ci::gl::clear(grey);

	if (_map == nullptr || _entities == nullptr) {
		return;
	}

	ci::gl::setMatricesWindow(getWindowSize());
	//ci::gl::enableAlphaBlending();

	float scale = getWindowSize().x / _map->size.x * 0.9;
	ci::gl::translate(glm::vec2(getWindowSize()) * 0.05f);
	ci::gl::scale(scale, scale);

	ci::gl::color(ci::Color::white());
	for (const ci::Shape2d& shape : _map->inBounds) {
		ci::gl::drawSolid(shape);
	}

	ci::gl::color(grey);
	for (const ci::Shape2d& shape : _map->outBounds) {
		ci::gl::drawSolid(shape);
	}

	// Draw device views
	ci::gl::color(ci::ColorA(170/255.0, 70/255.0, 190/255.0, 0.5));
	for (const DeviceView devView : _map->devs) {
		ci::gl::drawSolid(devView.view);
	}

	// Draw doors
	ci::gl::color(ci::Color(90/255.0, 65/255.0, 55/255.0));
	for (const Door &door : _map->doors) {
		ci::gl::ScopedModelMatrix model;
		ci::gl::translate(door.pos);
		ci::gl::rotate(door.angle * (M_PI / 180));
		ci::gl::drawSolidRect(ci::Rectf({ -DOOR_WIDTH / 2, -0.5 }, { DOOR_WIDTH / 2, 0.5 }));

		//ci::gl::scale(1 / scale, 1 / scale);
		//ci::gl::color(ci::Color::black());
		//ci::gl::Texture2dRef textTexture = ci::gl::Texture2d::create(tboxBase.text(std::to_string(door.id)).render());
		//glm::vec2 txtPos = (door.pos * scale) - glm::vec2(tboxBase.getSize()) * 0.5f;
		//ci::gl::draw(textTexture, txtPos);
		//ci::gl::scale(scale, scale);

	}

	//ci::gl::scale(1/scale, 1/scale);
	// ci::gl::color(ci::Color::black());

	// Draw flood fill
	// ci::gl::color(ci::Color::black());
	// ci::gl::scale(1 / scale, 1 / scale);
	// if (_entities->size() > 0) {
	// 	// const iGrid& pathMap = *(_map->getPathMap(19));
	// 	iGrid pathMap = _map->getDevicePathMap(19);
	// 	for (int x = 0; x < pathMap.size(); x+=4) {
	// 		for (int y = 0; y < pathMap[x].size(); y+=4) {
	// 			ci::gl::Texture2dRef textTexture = ci::gl::Texture2d::create(tboxBase.text(std::to_string(pathMap[x][y])).render());
	// 			ci::gl::draw(textTexture, (glm::vec2(x, y) * scale) - glm::vec2(tboxBase.getSize()) * 0.5f);
	// 		}
	// 	}
	// }
	// ci::gl::scale(scale, scale);
	
	// Draw entities
	for (EntityPtr entity : *_entities) {
		ci::gl::ScopedModelMatrix model;
		ci::gl::color(ci::Color(ci::CM_HSV, entity->id / (double) _entities->size(), 1.0, 1.0));
		ci::gl::translate(entity->getPos());
		ci::gl::rotate(entity->getHeading());
		static glm::vec2 points[3] = { {-1.0, 1.0}, {-1.0, -1.0}, {1.0, 0.0} };
		ci::gl::drawSolidTriangle(points);

		// Draw entity ids
		//ci::gl::scale(1 / scale, 1 / scale);
		//ci::gl::color(ci::Color::black());
		//ci::gl::Texture2dRef textTexture = ci::gl::Texture2d::create(tboxBase.text(std::to_string(entity->id)).render());
		//glm::vec2 txtPos = (entity->getPos() * scale) - glm::vec2(tboxBase.getSize()) * 0.5f - glm::vec2(0, 10.0);
		//ci::gl::draw(textTexture, txtPos);
		//ci::gl::scale(scale, scale);

		//ci::gl::drawSolidCircle(entity->getPos(), 1.0);
	}

	// Draw sts as crosses
	// for (ShortTermStatePtr sts : _shortTermStates) {
	// 	ci::gl::ScopedModelMatrix model;
	// 	if (sts->longTermStateKey == -1) {
	// 		ci::gl::color(ci::Color::black());
	// 	} else {
	// 		ci::gl::color(ci::Color(ci::CM_HSV, sts->longTermStateKey / 15.0, 1.0, 1.0));
	// 	}
	// 	ci::gl::translate(_map->devs[sts->lastUpdateDeviceId].pos);
	// 	ci::gl::drawLine({-1.0, 0.0}, {1.0, 0.0});
	// 	ci::gl::drawLine({0.0, 1.0}, {0.0, -1.0});
	// }

	// Draw Particles as circles
	std::vector<float> offsets(_map->devs.size());
	for (Particle par : _particles) {
		ci::gl::color(ci::Color(ci::CM_HSV, par.shortTermStateId / 20.0, par.weight, 1.0));
		offsets[par.originDeviceId] += 1.5;
		const DeviceView& dev = _map->devs[par.originDeviceId]; 
		ci::gl::drawSolidCircle(dev.pos + glm::rotate(glm::vec2(offsets[par.originDeviceId], 0.0), -dev.angle), 1.0);
	}

	if (_pathGraph) {
		// Draw path graph of first sts
		ci::gl::color(ci::Color::black());
		for (int i = 0; i < _map->devs.size(); i++) {
			std::set<int> edges = PathGraph::getGraphEdges(i);
			glm::vec2 nodePos = _map->devs[i].pos;
			for (auto e = edges.begin(); e != edges.end(); e++) {
				ci::gl::drawLine(nodePos, _map->devs[*e].pos);
			}

			ci::gl::scale(1 / scale, 1 / scale);
			// ci::gl::color(ci::Color(255, 0, 0));
			ci::gl::Texture2dRef textTexture = ci::gl::Texture2d::create(tboxBase.text(std::to_string(_pathGraph->getDepth(i))).render());
			glm::vec2 txtPos = (nodePos * scale) - glm::vec2(tboxBase.getSize()) * 0.5f + glm::rotate(glm::vec2(10.0, 0.0), -_map->devs[i].angle);
			ci::gl::draw(textTexture, txtPos);
			ci::gl::scale(scale, scale);
		}
	}

}