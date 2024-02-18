
#include "utils/PathGraph.h"

std::vector<std::set<int>> PathGraph::_graph;

PathGraph::PathGraph(int stsId_, int ltsId_, int period_, boost::span<const unsigned char> path) :
    shortTermStateId(stsId_),
    longTermStateId(ltsId_),
    period(period_)
{
    if (_graph.size() == 0) {
        printf("PathGraph::PathGraph - Error: PathGraph uninitlized");
    }

    if (path.size() > 0) {
	    memcpy(_depths.data(), path.data(), path.size_bytes());
    } else {
        _depths = Eigen::VectorXf::Zero(_graph.size());
    }
}

void PathGraph::initGraph(std::string mapPath, std::string cachePath) {
    
    printf("Initilizing path graph ... \n");

    std::ifstream cacheIn;
    cacheIn.open(cachePath.c_str());
    if (!cacheIn.good()) {
        cacheIn.close();

        printf("Getting connections\n");

        Map map(mapPath);
	    map.getDeviceConnections(_graph);

        printf("Writing to cache\n");
        std::ofstream cacheOut;
        cacheOut.open(cachePath);
        if (cacheOut.good()) {
            for (std::set<int>& conns : _graph) {
                for (auto i = conns.begin(); i != conns.end(); i++) {
                    cacheOut << std::to_string(*i);
                    if (i != --conns.end()) {
                        cacheOut << ", ";
                    }
                }
                cacheOut << "\n";
            }
        }
        cacheOut.close();
    } else {
        printf("Reading cache\n");
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
    printf("Done\n");
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
