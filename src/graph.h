#pragma once
#include <string>
#include <vector>
#include <map>
using namespace std;
struct Edge {
    string to;      
    int    weight;  
};
extern map<string, vector<Edge>> campusGraph;

extern map<string, pair<int,int>> nodePos;


void buildCampusGraph();


map<string,int> dijkstra(const string& src, map<string,string>& prev);


vector<string> getPath(const map<string,string>& prev,
                       const string& src, const string& dst);
