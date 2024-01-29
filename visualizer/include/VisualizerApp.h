#pragma once

#include <vector>

#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>

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

private:

	void loadData(std::string filename, int entities, int imgs);
	double computeCost();

	ci::Font _font;

	std::vector<Face> _faces;
	FFMat _distances;
	Eigen::Matrixxd _state;
	Eigen::Matrixxd _stateGradient;
	double _stepSize {0.1};
	int _nDim;

};

CINDER_APP( VisualizerApp, ci::app::RendererGl )