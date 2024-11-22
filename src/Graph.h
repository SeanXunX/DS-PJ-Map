#pragma once

#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <cmath>
#include <vector>
#include "Node.h"
#include "KDTree.h"
#include <array>

template <typename T>
class Graph {
    friend std::vector<Node> AStar(const Graph<Node> &graph, const Node &start, const Node &goal);
    friend std::vector<Node> BiAStar(const Graph<Node> &graph, const Node &start, const Node &goal);
public:
    void addNode(const T& node) {
        rev_adjList[node];
        adjList[node];  
    }

    void addNode2KDTree(const KDNode &node) {
        kdtree.insert(node);
    }

    void addDirectedEdge(const T& src, const T& dest, double dist) {
        rev_adjList[dest].insert(src);
        adjList[src].insert(dest);
        distances.insert({{src, dest}, dist});
    }

    const std::set<T>& getNeighbors(const T& node) const {
        auto it = adjList.find(node);
        if (it == adjList.end()) {
            throw std::runtime_error("Node not found in adjList.");
        }
        return it->second;
    }

    const std::set<T>& rev_getNeighbors(const T& node) const {
        auto it = rev_adjList.find(node);
        if (it == rev_adjList.end()) {
            throw std::runtime_error("Node not found in rev_adjList.");
        }
        return it->second;
    }

    bool containsNode(const T& node) const {
        return adjList.count(node);
    }

    void serialize(std::ofstream &out) const;    

    void deserialize(std::ifstream &in);

    inline std::pair<double, double> queryByName(const std::string &name) const {
        auto &[lng, lat] = location_map.at(name); 
        std::array<double, 2> data = {lng, lat};
        auto res_kd_node = kdtree.nearest_neighbor(KDNode(data));
        double llng = res_kd_node.data[0], llat = res_kd_node.data[1];
        return std::make_pair(llng, llat);
    }

    inline std::pair<double, double> queryByArbitrary(const std::pair<double, double> &coord) const {
        std::array<double, 2> data = {coord.first, coord.second};
        auto res_kd_node = kdtree.nearest_neighbor(KDNode(data));
        double llng = res_kd_node.data[0], llat = res_kd_node.data[1];
        return std::make_pair(llng, llat);
    }

    void addNamePoint(const std::string &name, const std::pair<double, double> &coord) {
        location_map[name] = coord;
    }
    
private:
    std::map<T, std::set<T>> adjList;  

    std::map<T, std::set<T>> rev_adjList;

    std::map<std::pair<T, T>, double> distances;

    // name to lng & lat
    std::map<std::string, std::pair<double, double>> location_map;

    KDTree kdtree;
};

std::vector<Node> AStar(const Graph<Node> &graph, const Node &start, const Node &goal);

template <typename T>
void Graph<T>::serialize(std::ofstream &out) const {
    // serialize rev_adjacent list
    size_t rev_adjCount = rev_adjList.size();
    out.write(reinterpret_cast<const char*>(&rev_adjCount), sizeof(rev_adjCount));
    for (const auto& [node, neighbors] : rev_adjList) {
        node.serialize(out);

        size_t neighborCount = neighbors.size();
        out.write(reinterpret_cast<const char *>(&neighborCount), sizeof(neighborCount));

        for (const auto& neighbor : neighbors) {
            neighbor.serialize(out);
        }
    }

    // serialize adjacent list
    size_t adjCount = adjList.size();
    out.write(reinterpret_cast<const char*>(&adjCount), sizeof(adjCount));
    for (const auto& [node, neighbors] : adjList) {
        node.serialize(out);

        size_t neighborCount = neighbors.size();
        out.write(reinterpret_cast<const char*>(&neighborCount), sizeof(neighborCount));

        for (const auto& neighbor : neighbors) {
            neighbor.serialize(out);
        }
    }

    // serialize distance map
    size_t distCount = distances.size();
    out.write(reinterpret_cast<const char*>(&distCount), sizeof(distCount));
    for (const auto& [nodePair, dist] : distances) {
        nodePair.first.serialize(out);
        nodePair.second.serialize(out);
        out.write(reinterpret_cast<const char*>(&dist), sizeof(dist));
    }

    // serialize location map
    size_t locationCount = location_map.size();
    out.write(reinterpret_cast<const char*>(&locationCount), sizeof(locationCount));
    for (const auto& [name, coord] : location_map) {
        size_t nameLength = name.size();
        out.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        out.write(name.c_str(), nameLength);
        
        out.write(reinterpret_cast<const char*>(&coord.first), sizeof(coord.first));
        out.write(reinterpret_cast<const char*>(&coord.second), sizeof(coord.second));
    }

    // serialize kdtree
    kdtree.serialize(out);
}

template <typename T>
void Graph<T>::deserialize(std::ifstream &in) {
    // clear current content
    rev_adjList.clear();
    adjList.clear();
    distances.clear();
    location_map.clear();

    // deserialize rev_adjList
    size_t rev_adjCount;
    in.read(reinterpret_cast<char*>(&rev_adjCount), sizeof(rev_adjCount));
    for (size_t i = 0; i < rev_adjCount; ++i) {
        T node;
        node.deserialize(in);
        size_t neighborCount;
        in.read(reinterpret_cast<char*>(&neighborCount), sizeof(neighborCount));

        for (size_t j = 0; j < neighborCount; ++j) {
            T neighbor;
            neighbor.deserialize(in);
            rev_adjList[node].insert(neighbor);
        }
    }

    // deserialize adjList
    size_t adjCount;
    in.read(reinterpret_cast<char*>(&adjCount), sizeof(adjCount));
    for (size_t i = 0; i < adjCount; ++i) {
        T node;
        node.deserialize(in);
        size_t neighborCount;
        in.read(reinterpret_cast<char*>(&neighborCount), sizeof(neighborCount));

        for (size_t j = 0; j < neighborCount; ++j) {
            T neighbor;
            neighbor.deserialize(in);
            adjList[node].insert(neighbor);
        }
    }

    // deserialize distance
    size_t distCount;
    in.read(reinterpret_cast<char*>(&distCount), sizeof(distCount));
    for (size_t i = 0; i < distCount; ++i) {
        T src, dest;
        double dist;
        src.deserialize(in);
        dest.deserialize(in);
        in.read(reinterpret_cast<char*>(&dist), sizeof(dist));
        distances[{src, dest}] = dist;
    }

    // deserialize location_map
    size_t locationCount;
    in.read(reinterpret_cast<char*>(&locationCount), sizeof(locationCount));
    for (size_t i = 0; i < locationCount; ++i) {
        size_t nameLength;
        in.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));

        std::string name(nameLength, '\0');
        in.read(&name[0], nameLength);

        double lng, lat;
        in.read(reinterpret_cast<char*>(&lng), sizeof(lng));
        in.read(reinterpret_cast<char*>(&lat), sizeof(lat));

        location_map[name] = {lng, lat};
    }

    kdtree.deserialize(in);
}