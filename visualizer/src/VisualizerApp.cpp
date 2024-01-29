
#include <string>
#include <fmt/core.h>
#include <cinder/Rand.h>

#include "VisualizerApp.h"

VisualizerApp::VisualizerApp() {
}

void VisualizerApp::setup() {
	_font = ci::Font("Times New Roman", 18);

	// loadData("../../../../dataset.csv", 9, 12);

	// Compute distances
	_distances(_nDim, _nDim)
	for (int i = 0; i < _nDim; i++) {
		_distance(i, i) = 0.0;
		for (int j = i + 1; j < _nDim; j++) {
			_distances(i, j) = l2Distance(_faces[i].features, _faces[j].features);
		}
	}
	// _distances = _distances.selfadjointView<Upper>();

	// Randomize state
	_state::Random(_nDim, 2);
	_state = _state * _distances.maxCoeff() / 2.0;
	_stateGradient(_nDim, 2);
	printf("Beginning optimization ...\n")
}

void VisualizerApp::update() {

	// compute gradient
	for (int i = 0; i < _nDim; i++) {
		double xSum = 0.0, ySum = 0.0;
		for (int j = i + 1; j < _nDim; j++) {
			double a = 1 - _distances(i, j) / (_state.row(i) - _state.row(j)).norm();
			xSum += (_state(i, 0) - _state(j, 0)) * a;
			ySum += (_state(i, 1) - _state(j, 1)) * a;
		}
		_stateGradient(i, 0) = 2.0 * xSum;
		_stateGradient(i, 1) = 2.0 * ySum;
	}

	// gradient decent update
	_state = _state - _stepSize * _stateGradient;

	fmt::println("Cost: {}", computeCost());

}

void VisualizerApp::draw() {
	static ci::TextBox tboxBase = ci::TextBox().alignment(ci::TextBox::CENTER).font(_font).size(glm::ivec2(50, 15));

	ci::gl::clear(ci::Color::white());
	ci::gl::setMatricesWindow(getWindowSize());
	//ci::gl::enableAlphaBlending();

	double largeDim = std::max(_state.col(0).maxCoeff() - _state.col(0).minCoeff(), _state.col(1).maxCoeff() - _state.col(1).minCoeff()) / 0.9;
	ci::gl::scale(getWindowSize().x / largeDim, getWindowSize().y / largeDim);

	for (int i = 0; i < _nDim; i++) {
		ci::gl::color(ci::Color(CM_HSV, _faces[i].entity / 15.0, 1, 1));
		glm::vec2 pos = {_state(i, 0), _state(i, 1)};
		ci::gl::drawSolidCircle(pos, largeDim * 0.1);
		for (int j = i + 1; j < _nDim; j++) {
			ci::gl::clear(ci::Color::black());
			ci::gl::drawLine(pos, {_state(j, 0), _state(j, 1)})
		}
	}
}

void VisualizerApp::loadData(std::string filename, int entities, int imgs) {

	fmt::print("Loading data from {} ... ", filename);
	try {
		int faceNum = 0;
		int entity = 0;
		std::ifstream file(filename);
		if (file.is_open()) {
			std::string line;
			while (1) {
				std::getline(file, line);
				if (file.eof()) break;
				std::stringstream s(line);
				std::string num;
				int feature = 0;
				Face face;
				face.entity = entity;
				while (std::getline(s, num, ',')) {
					face.features[feature] = stof(num);
					feature++;
				}
				if (feature != FACE_VEC_SIZE) {
					throw std::runtime_error("improper feature vector dimensions\n");
				}
				_faces.push_back(face);
				faceNum++;
				if (faceNum == imgs) {
					faceNum = 0;
					entity++;
				}
			}
			if (faceNum != 0) {
				throw std::runtime_error("inproper alignment\n");
			}
			if (entity != entities) {
				throw std::runtime_error("unexpected number of entities\n");
			}
		}
		_nDim = _faces.size();
		file.close();
		printf("Done\n");
	}
	catch (const std::exception& err) {
		std::cerr << "Failed to read dataset: " << err.what() << std::endl;
	}
}

double VisualizerApp::computeCost() {
	double cost = 0.0;
	for (int i = 0; i < _nDim; i++) {
		for (int j = i; j < _nDim; j++) {
			cost += pow((_state.row(i) - _state.row(j)).norm() - _distances(i, j), 2);
		}
	}
}