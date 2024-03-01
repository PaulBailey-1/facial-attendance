
#include <string>
#include <fmt/core.h>
#include <cinder/Log.h>

#include "VisualizerApp.h"

VisualizerApp::VisualizerApp() {
}

void VisualizerApp::setup() {
	_font = ci::Font("Times New Roman", 18);

	_camUi = ci::CameraUi(&_cam);

	auto lambert = ci::gl::ShaderDef().lambert().color();
	ci::gl::GlslProgRef shader = ci::gl::getStockShader(lambert);

	auto nodeShape = ci::geom::Sphere().radius(0.5f).subdivisions(3);
	_nodeShape = ci::gl::Batch::create(nodeShape, shader);

	_db.connect();
	loadEntities();
	loadShortTermState();

	// loadData("../../../dataset.csv", 9, 12);
	//loadData("../../../../datasetSmall.csv", 4, 12);
	//loadData("../../../../datasetTiny.csv", 2, 1);

	// Compute distances
	_distances = Eigen::MatrixXd::Zero(_nDim, _nDim);
	for (int i = 0; i < _nDim; i++) {
		for (int j = i + 1; j < _nDim; j++) {
			_distances(i, j) = l2Distance(_faces[i].features, _faces[j].features);
		}
	}
	// _distances = _distances.selfadjointView<Upper>();
	_maxDistance = _distances.maxCoeff();

	// Randomize state
	srand(56711176);
	_state = Eigen::MatrixXd::Random(_nDim, 3);
	_state = _state * _maxDistance / 2.0;
	_stateGradient = Eigen::MatrixXd(_nDim, 3);

	_log.open("log.csv");
	_log << "timestep, cost, step, gradient\n";

	CI_LOG_D("Beginning optimization ...");

}

void VisualizerApp::update() {

	if (!_done) {
		// compute gradient
		for (int i = 0; i < _nDim; i++) {
			Eigen::VectorXd sum = Eigen::VectorXd::Zero(_state.cols());
			for (int j = i + 1; j < _nDim; j++) {
				double a = 1 - _distances(i, j) / (_state.row(i) - _state.row(j)).norm();
				sum += (_state.row(i) - _state.row(j)) * a;
			}
			_stateGradient.row(i) = 2.0 * sum;
		}
		double gradientAvg = _stateGradient.sum() / double(_stateGradient.rows() * _stateGradient.cols());

		// gradient decent update
		_state = _state - _stepSize * _stateGradient;

		// step size freezing
		_stepSize = 0.999 * _stepSize;

		//fmt::println("Cost: {}", computeCost());
		_time++;
		double cost = computeCost();

		CI_LOG_D("Cost: " + std::to_string(cost));
		_log << time << ", " << cost << ", " << _stepSize << ", " << gradientAvg << "\n";

		glm::vec3 center = { _state.col(0).sum(), _state.col(1).sum(), _state.col(2).sum() };
		center = center / (float)_nDim;
		_cam.setFarClip((_zoom + 0.5) * _maxDistance);
		_cam.lookAt(center + glm::vec3{ _zoom * _maxDistance, 0, 0 }, center);

		_pastCosts.push(cost);
		_avgCostSum += cost;
		if (_pastCosts.size() > 100) {
			_avgCostSum -= _pastCosts.front();
			_pastCosts.pop();
		}
		double avgCost = _avgCostSum / _pastCosts.size();

		if (cost > avgCost) {
			_done = true;
			CI_LOG_D("Optimization complete");
		}
	}
}

void VisualizerApp::draw() {
	static ci::TextBox tboxBase = ci::TextBox().alignment(ci::TextBox::CENTER).font(_font).size(glm::ivec2(50, 15));

	ci::gl::clear(ci::Color::white());
	ci::gl::enableDepthRead();
	ci::gl::enableDepthWrite();

	ci::gl::setMatrices(_cam);

	//ci::gl::setMatricesWindow(getWindowSize());
	////ci::gl::enableAlphaBlending();

	//ci::gl::translate({getWindowSize().x / 2.0, getWindowSize().y / 2.0});
	//double xRange = getWindowSize().x /  (_state.col(0).maxCoeff() - _state.col(0).minCoeff());
	//double yRange = getWindowSize().y / (_state.col(1).maxCoeff() - _state.col(1).minCoeff());
	//double xRange = _state.col(0).maxCoeff() - _state.col(0).minCoeff();
	//double yRange = _state.col(1).maxCoeff() - _state.col(1).minCoeff();
	//double scale;
	//if (xRange > yRange) {
	//	scale = xRange;
	//} else {
	//	scale = yRange;
	//}
	//ci::gl::scale(scale, scale);

	//if (_done) {
	//	for (int i = 0; i < _nDim; i++) {
	//		glm::vec3 pos = { _state(i, 0), _state(i, 1), _state(i, 2) };
	//		for (int j = i + 1; j < _nDim; j++) {
	//			ci::gl::color(ci::ColorA(0.0, 0.0, 0.0, exp(-5 * _distances(i, j) / _maxDistance)));
	//			ci::gl::drawLine(pos, { _state(j, 0), _state(j, 1), _state(j, 2) });

	//			//ci::gl::pushModelMatrix();
	//			//ci::gl::scale(1 / scale, 1 / scale);
	//			//double error = (_state.row(i) - _state.row(j)).norm() - _distances(i, j);
	//			//ci::gl::Texture2dRef textTexture = ci::gl::Texture2d::create(tboxBase.text(std::to_string(error)).render());
	//			//glm::vec2 mid = glm::vec2{ _state(j, 0), _state(j, 1) } - pos;
	//			//mid /= 2.0;
	//			//mid += pos;
	//			//ci::gl::draw(textTexture, mid * (float)scale);
	//			//ci::gl::popModelMatrix();
	//		}
	//	}
	//}

	double scale = _maxDistance / (_zoom * 5.0);
	for (int i = 0; i < _nDim; i++) {
		ci::gl::ScopedModelMatrix model;
		if (_face[i].type == DATASET) {
			ci::gl::color(ci::Color(ci::CM_HSV, _faces[i].entity / 15.0, 1, 1));
		} else if (_face[i].type == STS) {
			ci::gl::color(ci::Color(1.0, 1.0, 1.0));
		}
		glm::vec3 pos = { _state(i, 0), _state(i, 1),  _state(i, 2) };
		ci::gl::translate(pos);
		ci::gl::scale(scale, scale, scale);
		_nodeShape->draw();

		//ci::gl::drawSolidCircle(pos, 10.0 / scale);
	}

}

void VisualizerApp::mouseDown(ci::app::MouseEvent event) {
	_camUi.mouseDown(event);
}

void VisualizerApp::mouseDrag(ci::app::MouseEvent event) {
	_camUi.mouseDrag(event);
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
				face.type = DATASET;
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

void VisualizerApp::loadEntities() {
	std::vector<EntityPtr> entities;
	_db.getEntitiesFeatures(entities);
	for (EntityPtr entity& : entities) {
		_faces.push_back(Face(entity->id, DATASET, entity->facialFeatures));
	}
}

void VisualizerApp::loadShortTermState() {
	std::vector<ShortTermStatePtr> stateTermStates;
	_db.getShortTermStates(stateTermStates);
	for (ShortTermStatePtr& sts : ShortTermStates) {
		_faces.push_back(Face(sts->id, STS, sts->facialFeatures));
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