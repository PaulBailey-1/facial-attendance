
#include <fmt/core.h>

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
	    map.getDeviceConnections(_graph, _distances);

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

            cacheOut << "distances\n";
            for (int row = 0; row < _distances.rows(); row++) {
                for (int col = 0; col < _distances.cols(); col++) {
                    cacheOut << std::to_string(_distances(row, col));
                    if (col != _distances.cols() - 1) {
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
        bool readingDistances = false;
        int distancesRow = 0;
        while (1) {
            std::getline(cacheIn, line);
            if (line == "distances") {
                readingDistances = true;
                _distances = Eigen::MatrixXd::Zeros(_graph.size(), _graph.size());
                continue;
            }
            if (cacheIn.eof()) break;
            std::stringstream s(line);
            std::string num;
            if (!readingDistances) {
                std::set<int> conns;
                while (std::getline(s, num, ',')) {
                    conns.emplace(stoi(num));
                }
                _graph.push_back(conns);
            } else {
                int col = 0;
                while (std::getline(s, num, ',')) {
                    _distances(distancesRow, col) = stoif(num);
                    col++;
                }
                distancesRow++;
            }
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

double PathGraph::getGraphEdgeLength(int from, int to) {
    double distance = _distances(from, to);
    if (distance == 0.0) {
        fmt::println("PathGraph:getGraphEdgeLength - Error: Can't get edge length from {} to {}", from, to);
    }
    return distance;
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