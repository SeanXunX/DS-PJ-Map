#pragma once

#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <cmath>
#include <vector>
#include "Node.h"
#include "KDTree.h"
#include <array>

class Graph {
    friend std::vector<Node> AStar(const Graph &graph, const Node &start, const Node &goal);
    friend std::vector<Node> BiAStar(const Graph &graph, const Node &start, const Node &goal);
public:
    void addNode(const Node& node) {
        rev_adjList[node];
        adjList[node];  
    }

    void addNode2KDTree(const KDNode &node) {
        kdtree.insert(node);
    }

    void addDirectedEdge(const Node& src, const Node& dest, double dist) {
        rev_adjList[dest].insert(src);
        adjList[src].insert(dest);
        distances.insert({{src, dest}, dist});
    }

    const std::set<Node>& getNeighbors(const Node& node) const {
        auto it = adjList.find(node);
        if (it == adjList.end()) {
            throw std::runtime_error("Node not found in adjList.");
        }
        return it->second;
    }

    const std::set<Node>& rev_getNeighbors(const Node& node) const {
        auto it = rev_adjList.find(node);
        if (it == rev_adjList.end()) {
            throw std::runtime_error("Node not found in rev_adjList.");
        }
        return it->second;
    }

    bool containsNode(const Node& node) const {
        return adjList.count(node);
    }

    void serialize(std::ofstream &out) const;    

    void deserialize(std::ifstream &in);

    std::pair<double, double> queryByName(const std::string &name) const {
        auto &[lng, lat] = location_map.at(name); 
        std::array<double, 2> data = {lng, lat};
        auto res_kd_node = kdtree.nearest_neighbor(KDNode(data));
        double llng = res_kd_node.data[0], llat = res_kd_node.data[1];
        return std::make_pair(llng, llat);
    }

    std::pair<double, double> queryByArbitrary(const std::pair<double, double> &coord) const {
        std::array<double, 2> data = {coord.first, coord.second};
        auto res_kd_node = kdtree.nearest_neighbor(KDNode(data));
        double llng = res_kd_node.data[0], llat = res_kd_node.data[1];
        return std::make_pair(llng, llat);
    }

    void addNamePoint(const std::string &name, const std::pair<double, double> &coord) {
        location_map[name] = coord;
    }

    std::vector<std::string> fuzzySearch(const std::string &query, double threshold,  std::multimap<double, std::string>::size_type max_size) const;

    inline bool location_mapContains(const std::string &name) {
        return location_map.count(name);
    }

    std::vector<Node> AStar(const Node &start, const Node &goal) const;

    std::vector<Node> BiAStar(const Node &start, const Node &dst) const;

private:
    std::map<Node, std::set<Node>> adjList;  

    std::map<Node, std::set<Node>> rev_adjList;

    // weighted distance
    std::map<std::pair<Node, Node>, double> distances;

    // name to lng & lat
    std::map<std::string, std::pair<double, double>> location_map;

    KDTree kdtree;
};

std::vector<Node> AStar(const Graph &graph, const Node &start, const Node &goal);
std::vector<Node> BiAStar(const Graph &graph, const Node &start, const Node &goal);

