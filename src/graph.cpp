#include "graph.h"
#include <queue>
#include <climits>
#include <algorithm>
map<string, vector<Edge>> campusGraph;
map<string, pair<int,int>> nodePos;
void buildCampusGraph() {
    auto addEdge = [](const string& a, const string& b, int w) {
        campusGraph[a].push_back({b, w});
        campusGraph[b].push_back({a, w});
    };
    addEdge("MainGate",   "BTech",       150);
    addEdge("MainGate",   "Library",     100);
    addEdge("MainGate",   "Canteen",      80);
    addEdge("BTech",      "CSIT",        120);
    addEdge("BTech",      "Mechanical",  100);
    addEdge("BTech",      "Canteen",      90);
    addEdge("CSIT",       "ParamLab",     80);
    addEdge("CSIT",       "Library",     110);
    addEdge("CSIT",       "Aryabhatta",  130);
    addEdge("Mechanical", "Civil",        90);
    addEdge("Mechanical", "Canteen",     110);
    addEdge("Civil",      "Library",     140);
    addEdge("Civil",      "Hostel",      160);
    addEdge("Library",    "Aryabhatta",  100);
    addEdge("Library",    "Canteen",     120);
    addEdge("ParamLab",   "Aryabhatta",   70);
    addEdge("Aryabhatta", "Hostel",      130);
    addEdge("Canteen",    "Hostel",      200);


    nodePos["MainGate"]   = {50, 92};
    nodePos["Canteen"]    = {38, 78};
    nodePos["BTech"]      = {28, 62};
    nodePos["Mechanical"] = {18, 45};
    nodePos["Civil"]      = {22, 28};
    nodePos["Library"]    = {50, 55};
    nodePos["CSIT"]       = {62, 62};
    nodePos["ParamLab"]   = {75, 48};
    nodePos["Aryabhatta"] = {65, 32};
    nodePos["Hostel"]     = {40, 15};
}
map<string,int> dijkstra(const string& src, map<string,string>& prev) {
    map<string,int> dist;
    for (auto& p : campusGraph)
        dist[p.first] = INT_MAX;
    dist[src] = 0;

    priority_queue<pair<int,string>,
                   vector<pair<int,string>>,
                   greater<pair<int,string>>> pq;
    pq.push({0, src});

    while (!pq.empty()) {
        pair<int,string> top=pq.top();
        int d=top.first;
        string u=top.second;
        pq.pop();

     
        if (d > dist[u]) continue;

        for (auto& edge : campusGraph[u]) {
            int newDist = dist[u] + edge.weight;
            if (newDist < dist[edge.to]) 
            {
                dist[edge.to]  = newDist;
                prev[edge.to]  = u;        
                pq.push({newDist, edge.to});
            }
        }
    }

    return dist;
}
vector<string> getPath(const map<string,string>& prev,
                       const string& src, const string& dst) {
    vector<string> path;
    string cur = dst;

    while (cur != src) {
        if (prev.find(cur) == prev.end())
            return {};          // no path exists
        path.push_back(cur);
        cur = prev.at(cur);
    }
    path.push_back(src);
    reverse(path.begin(), path.end());
    return path;
}
