
#include <string>
#include <fmt/core.h>
#include <cinder/Log.h>

#include "VisualizerApp.h"

VisualizerApp::VisualizerApp() {
}

void VisualizerApp::setup() {
	_font = ci::Font("Times New Roman", 18);

	loadData("../../../../datasetSmall.csv", 4, 12);
	//Face face1;
	//face1.entity = 0;
	//face1.features << 500.0, 100.0, Eigen::VectorXf::Zero(126);
	//_faces.push_back(face1);

	//Face face2;
	//face2.entity = 0;
	//face2.features << 150.0, 200.0, Eigen::VectorXf::Zero(126);
	//_faces.push_back(face2);

	//Face face3;
	//face3.entity = 1;
	//face3.features << 350.0, 600.0, Eigen::VectorXf::Zero(126);
	//_faces.push_back(face3);

	//Face face4;
	//face4.entity = 1;
	//face4.features << 100.0, 50.0, Eigen::VectorXf::Zero(126);
	//_faces.push_back(face4);

	//_nDim = _faces.size();

	// Compute distances
	_distances(_nDim, _nDim);
	for (int i = 0; i < _nDim; i++) {
		_distances(i, i) = 0.0;
		for (int j = i + 1; j < _nDim; j++) {
			_distances(i, j) = l2Distance(_faces[i].features, _faces[j].features);
		}
	}
	// _distances = _distances.selfadjointView<Upper>();

	// Randomize state
	srand(56711176);
	_state = Eigen::MatrixXd::Random(_nDim, 2);
	_state = _state * _distances.maxCoeff() / 2.0;
	_stateGradient = Eigen::MatrixXd(_nDim, 2);

	_log.open("log.csv");
	_log << "timestep, cost, step, gradient\n";

	CI_LOG_D("Beginning optimization ...");
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
	double gradientAvg = _stateGradient.sum() / double(_nDim * 2);

	// gradient decent update
	_state = _state - _stepSize * _stateGradient;

	// step size freezing
	_stepSize = 0.999 * _stepSize;

	//fmt::println("Cost: {}", computeCost());
	double cost = computeCost();
	CI_LOG_D("Cost: " + std::to_string(cost));
	_log << time << ", " << cost << ", " << _stepSize << ", " << gradientAvg << "\n";
	_time++;
}

void VisualizerApp::draw() {
	static ci::TextBox tboxBase = ci::TextBox().alignment(ci::TextBox::CENTER).font(_font).size(glm::ivec2(50, 15));

	ci::gl::clear(ci::Color::white());
	ci::gl::setMatricesWindow(getWindowSize());
	//ci::gl::enableAlphaBlending();

	ci::gl::translate({getWindowSize().x / 2.0, getWindowSize().y / 2.0});
	double xRange = getWindowSize().x /  (_state.col(0).maxCoeff() - _state.col(0).minCoeff());
	double yRange = getWindowSize().y / (_state.col(1).maxCoeff() - _state.col(1).minCoeff());
	double scale;
	if (xRange > yRange) {
		scale = xRange;
	} else {
		scale = yRange;
	}
	scale *= 0.3;
	ci::gl::scale(scale, scale);

	for (int i = 0; i < _nDim; i++) {
		glm::vec2 pos = { _state(i, 0), _state(i, 1) };
		for (int j = i + 1; j < _nDim; j++) {
			ci::gl::color(ci::Color::black());
			ci::gl::drawLine(pos, { _state(j, 0), _state(j, 1) });

			//ci::gl::pushModelMatrix();
			//ci::gl::scale(1 / scale, 1 / scale);
			//double error = (_state.row(i) - _state.row(j)).norm() - _distances(i, j);
			//ci::gl::Texture2dRef textTexture = ci::gl::Texture2d::create(tboxBase.text(std::to_string(error)).render());
			//glm::vec2 mid = glm::vec2{ _state(j, 0), _state(j, 1) } - pos;
			//mid /= 2.0;
			//mid += pos;
			//ci::gl::draw(textTexture, mid * (float)scale);
			//ci::gl::popModelMatrix();
		}
	}

	for (int i = 0; i < _nDim; i++) {
		ci::gl::color(ci::Color(ci::CM_HSV, _faces[i].entity / 15.0, 1, 1));
		glm::vec2 pos = { _state(i, 0), _state(i, 1) };
		ci::gl::drawSolidCircle(pos, 10.0 / scale);
	}
}

void VisualizerApp::cleanup() {
	_log.close();
}

void VisualizerApp::loadData(std::string filename, int entities, int imgs) {

	//fmt::print("Loading data from {} ... ", filename);
	CI_LOG_D("Loading data from ... " + filename);

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
		//printf("Done\n");
		CI_LOG_D("Done");

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
	return cost;
}