
#include "PathGraph.h"

std::vector<std::set<int>> _graph;

PathGraph::PathGraph() {

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
