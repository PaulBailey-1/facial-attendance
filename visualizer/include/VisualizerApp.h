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
#include <utils/PathGraph.h>
#include <utils/DBConnection.h>

enum FaceType {
	DATASET,
	STS
};
struct Face {
	int entity;
	FaceType type;
	FFVec features;

	Face() {
		entity = -1;
		type = DATASET;
	}

	Face(int entity_, FaceType type_, FFVec features_) {
		entity = entity_;
		type = type_;
		features = features_;
	}
};

class VisualizerApp : public ci::app::App {
public:

	VisualizerApp();

	void setup() override;
	void update() override;
	void draw() override;
	void mouseDown(ci::app::MouseEvent event) override;
	void mouseDrag(ci::app::MouseEvent event) override;
	void keyDown(ci::app::KeyEvent event) override;
	void cleanup() override;

private:

	void loadData(std::string filename, int entities, int imgs);
	void loadEntities();
	void loadShortTermState();

	void loadLtsPath(int period, int ltsId);

	void projectionInit();
	void projectionGradientDescent();
	double computeCost();

	DBConnection _db;

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
	bool _descentDone{ false };

	std::queue<double> _pastCosts;
	double _avgCostSum{ 0.0 };

	PathGraphPtr _path;

	std::ofstream _log;

	ci::CameraPersp _cam;
	ci::CameraUi _camUi;

	ci::gl::BatchRef _nodeShape;

};

CINDER_APP( VisualizerApp, ci::app::RendererGl )