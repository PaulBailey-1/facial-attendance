#pragma once

#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>

#include "Map.h"

class PathGraph {
public:

    PathGraph();

    static void initGraph(std::string mapPath, std::string cachePath);

private:

    static std::vector<std::set<int>> _graph;

    Eigen::Vectorxd _depths;

};