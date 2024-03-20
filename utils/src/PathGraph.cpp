
#include "utils/PathGraph.h"

std::vector<std::set<int>> PathGraph::_graph;

PathGraph::PathGraph(int stsId_, int ltsId_, int period_, boost::span<const unsigned char> path) :
    shortTermStateId(stsId_),
    longTermStateId(ltsId_),
    period(period_)
{
    if (_graph.size() == 0) {
        printf("PathGraph::PathGraph - Error: PathGraph uninitlized\n");
    }

    _depths = Eigen::VectorXi::Zero(_graph.size());
    if (path.size() > 0) {
	    memcpy(_depths.data(), path.data(), path.size_bytes());
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

std::set<int> PathGraph::getGraphEdges(int node) {
    if (_graph.size() == 0) {
        printf("PathGraph:getPathByteSize - Error: PathGraph uninitalized\n");
    }
    if (_graph.size() > node) {
        return _graph[node];
    }
    return std::set<int>();
}

void PathGraph::start(int node) {
    _depths[node] = -1;
}

void PathGraph::update(int lastNode, int nextNode) {
    _depths[nextNode] = _depths[lastNode] - 1;
}

void PathGraph::fuse(PathGraphPtr other) {
    _depths += other->getDepths();
}

int PathGraph::getFinalDev() {
    // this might not be the right solution
    int lowest = 0;
    for (int i = 1; i < _depths.size(); i++) {
        if (_depths[i] < _depths[lowest]) {
            lowest = i;
        }
    }
    return lowest;
}

const boost::span<UCHAR> PathGraph::getPathSpan() const { return boost::span<UCHAR>(reinterpret_cast<UCHAR*>(const_cast<int*>(_depths.data())), getPathByteSize()); }
const Eigen::VectorXi& PathGraph::getDepths() const {return _depths;}
int PathGraph::getDepth(int node) {return _depths(node);}