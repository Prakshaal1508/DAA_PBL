#include "dfs.h"
#include "graph.h"
#include <set>
#include <algorithm>

// Helper function for backtracking
void findAllPathsRecursive(const string& u, const string& dst, set<string>& visited, 
                            vector<string>& currentPath, vector<vector<string>>& allPaths) {
    
    visited.insert(u);
    currentPath.push_back(u);

  
    if (u == dst) {
        allPaths.push_back(currentPath);
    } else {
 
        for (auto& edge : campusGraph[u]) {
            if (visited.find(edge.to) == visited.end()) {
                findAllPathsRecursive(edge.to, dst, visited, currentPath, allPaths);
            }
        }
    }

    currentPath.pop_back();
    visited.erase(u);
}

vector<vector<string>> getAllPaths(const string& src, const string& dst) {
    vector<vector<string>> allPaths;
    vector<string> currentPath;
    set<string> visited;

    if (campusGraph.count(src) && campusGraph.count(dst)) {
        findAllPathsRecursive(src, dst, visited, currentPath, allPaths);
    }
    return allPaths;
}
void dfsRecursive(const string& u, set<string>& visited, vector<string>& order) {
    visited.insert(u);
    order.push_back(u);

    vector<Edge> neighbours = campusGraph[u];
    sort(neighbours.begin(), neighbours.end(), [](const Edge& a, const Edge& b) {
        return a.to < b.to; 
    });

    for (auto& e : neighbours) {
        if (visited.find(e.to) == visited.end()) {
            dfsRecursive(e.to, visited, order);
        }
    }
}
vector<string> dfsExplore(const string& src) {
    vector<string> order;
    set<string> visited;

    if (campusGraph.count(src)) {
        dfsRecursive(src, visited, order);
    }
    
    return order; 
}