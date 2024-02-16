
#include "PathGraph.h"

std::vector<std::set<int>> _graph;

PathGraph::PathGraph(int stsId_, int ltsId_, int period_, boost::span<const unsigned char> path) :
    shortTermStateId(stsId_),
    longTermStateId(ltsId_),
    period(period_)
{
    if (_graph.size() == 0) {
        printf("PathGraph::PathGraph - Error: PathGraph uninitlized");
    }

    if (path != nullptr) {
	    memcpy(_depths.data(), path.data(), path.size_bytes());
    } else {
        _depths = Eigen::Vectorxf::Zero(_graph.size());
    }
}

void PathGraph::initGraph(std::string mapPath, std::string cachePath) {
    
    std::ifstream cacheIn;
    cacheIn.open(cachePath.c_str());
    if (!cacheIn.good()) {
        cacheIn.close();

        Map map(mapPath);
	    map.getDeviceConnections(_graph);

        // Write cache
        std::ofstream cacheOut;
        cacheOut.open(cachePath);
        if (cacheOut.good()) {
            for (std::set<int>& conns : _graph) {
                for (auto i = conns.begin(); i != conns.end(); i++) {
                    cacheOut << std::to_string(*i) << ", ";
                }
                cacheOut << "\n";
            }
        }
        cacheOut.close();
    } else {
        // Read cache
        std::string line;
        while (1) {
            std::getline(cacheIn, line);
            if (cacheIn.eof()) break;
            std::stringstream s(line);
            std::string num;
            std::set<int> conns;
            while (std::getline(s, num, ',')) {
                conns.emplace(stoi(num));
            }
            _graph.push_back(conns);
        }
        cacheIn.close();
    }
}

size_t PathGraph::getPathByteSize() {
    if (_graph.size() == 0) {
        printf("PathGraph:getPathByteSize - Error: PathGraph uninitalized\n");
    }
    return _graph.size() * sizeof(float);
}

void PathGraph::update(int lastNode, int nextNode) {
    _depths[nextNode] = _depths[lastNode] - 1;
}

void PathGraph::fuse(PathGraphPtr other) {
    _depths += other->getDepths();
}
