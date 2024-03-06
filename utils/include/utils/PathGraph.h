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

    void update(int lastNode, int nextNode);
    void fuse(std::shared_ptr<PathGraph> other);
    int getFinalDev();

	const boost::span<UCHAR> getPathSpan() const { return boost::span<UCHAR>(reinterpret_cast<UCHAR*>(const_cast<float*>(_depths.data())), getPathByteSize()); }
    const Eigen::VectorXf& getDepths() const {return _depths;}
    
private:

    static std::vector<std::set<int>> _graph;

    Eigen::VectorXf _depths;

};

typedef std::shared_ptr<PathGraph> PathGraphPtr;