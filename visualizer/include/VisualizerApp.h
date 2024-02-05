#pragma once

#include <vector>
#include <queue>

#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>
#include "cinder/Camera.h"
#include "cinder/GeomIo.h"
#include "cinder/CameraUi.h"

#include <utils/EntityState.h>

struct Face {
	FFVec features;
	int entity;
};

class VisualizerApp : public ci::app::App {
public:

	VisualizerApp();

	void setup() override;
	void update() override;
	void draw() override;
	void mouseDown(ci::app::MouseEvent event) override;
	void mouseDrag(ci::app::MouseEvent event) override;
	void cleanup() override;

private:

	void loadData(std::string filename, int entities, int imgs);
	double computeCost();

	ci::Font _font;

	std::vector<Face> _faces;
	Eigen::MatrixXd _distances;
	Eigen::MatrixXd _state;
	Eigen::MatrixXd _stateGradient;
	double _stepSize {0.01};
	int _nDim {0};
	double _maxDistance{ 0.0 };
	double _zoom{ 4.0 };
	int _time { 0 };
	bool _done{ false };

	std::queue<double> _pastCosts;
	double _avgCostSum{ 0.0 };

	std::ofstream _log;

	ci::CameraPersp _cam;
	ci::CameraUi _camUi;

	ci::gl::BatchRef _nodeShape;

};

CINDER_APP( VisualizerApp, ci::app::RendererGl )