#pragma once

#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <memory>

#include <boost/core/span.hpp>
#include <Eigen/dense>

#include "Map.h"

typedef unsigned char UCHAR;
class PathGraph {
public:

    int shortTermStateId;
    int longTermStateId;
    int period;

    PathGraph(int stsId_, int ltsId_, int period_, boost::span<const UCHAR> path = boost::span<const UCHAR>());

    static void initGraph(std::string mapPath, std::string cachePath);
    static size_t getPathByteSize();
    static std::set<int> getGraphEdges(int node);
    static double getGraphEdgeLength(int from, int to);

    void start(int node);
    void update(int lastNode, int nextNode);
    void fuse(std::shared_ptr<PathGraph> other);
    int getFinalDev();
    int getNext(int node);

	const boost::span<UCHAR> getPathSpan() const;
    const Eigen::VectorXi& getDepths() const;
    int getDepth(int node);
    
private:

    static std::vector<std::set<int>> _graph;
    static Eigen::MatrixXd _distances;

    Eigen::VectorXi _depths;

};

typedef std::shared_ptr<PathGraph> PathGraphPtr;