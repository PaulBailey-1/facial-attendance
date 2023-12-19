
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

	ci::gl::color(ci::ColorA(170/255.0, 70/255.0, 190/255.0, 0.5));
	for (const DeviceView devView : _map->devs) {
		ci::gl::drawSolid(devView.view);
	}

	ci::gl::color(ci::Color(90/255.0, 65/255.0, 55/255.0));
	for (const Door &door : _map->doors) {
		//ci::gl::pushModelMatrix();
		//ci::gl::translate(door.pos);
		//ci::gl::rotate(door.angle * (M_PI / 180));
		//ci::gl::drawSolidRect(ci::Rectf({ -DOOR_WIDTH / 2, -0.5 }, { DOOR_WIDTH / 2, 0.5 }));
		//ci::gl::popModelMatrix();

		ci::gl::scale(1 / scale, 1 / scale);
		ci::gl::color(ci::Color::black());
		ci::gl::Texture2dRef textTexture = ci::gl::Texture2d::create(tboxBase.text(std::to_string(door.id)).render());
		glm::vec2 txtPos = (door.pos * scale) - glm::vec2(tboxBase.getSize()) * 0.5f;
		ci::gl::draw(textTexture, txtPos);
		ci::gl::scale(scale, scale);

	}

	//ci::gl::scale(1/scale, 1/scale);
	//ci::gl::color(ci::Color::black());

	//if (_entities->size() > 0) {
	//	const iGrid& pathMap = *(_map->getPathMap((*_entities)[0]->getNextDoor(1)));
	//	for (int x = 0; x < pathMap.size(); x+=4) {
	//		for (int y = 0; y < pathMap[x].size(); y+=4) {
	//			ci::gl::Texture2dRef textTexture = ci::gl::Texture2d::create(tboxBase.text(std::to_string(pathMap[x][y])).render());
	//			ci::gl::draw(textTexture, (glm::vec2(x, y) * scale) - glm::vec2(tboxBase.getSize()) * 0.5f);
	//		}
	//	}
	//}
	
	ci::gl::color(ci::Color(1, 0, 0));
	for (EntityPtr entity : *_entities) {
		ci::gl::pushModelMatrix();
		ci::gl::translate(entity->getPos());
		ci::gl::rotate(entity->getHeading());
		static glm::vec2 points[3] = { {-1.0, 1.0}, {-1.0, -1.0}, {1.0, 0.0} };
		ci::gl::drawSolidTriangle(points);
		ci::gl::popModelMatrix();

		//ci::gl::scale(1 / scale, 1 / scale);
		//ci::gl::color(ci::Color::black());
		//ci::gl::Texture2dRef textTexture = ci::gl::Texture2d::create(tboxBase.text(std::to_string(entity->id)).render());
		//glm::vec2 txtPos = (entity->getPos() * scale) - glm::vec2(tboxBase.getSize()) * 0.5f - glm::vec2(0, 10.0);
		//ci::gl::draw(textTexture, txtPos);
		//ci::gl::scale(scale, scale);

		//ci::gl::drawSolidCircle(entity->getPos(), 1.0);
	}

}