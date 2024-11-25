#include "Graph.h"

void Graph::serialize(std::ofstream &out) const {
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

void Graph::deserialize(std::ifstream &in) {
    // clear current content
    rev_adjList.clear();
    adjList.clear();
    distances.clear();
    location_map.clear();

    // deserialize rev_adjList
    size_t rev_adjCount;
    in.read(reinterpret_cast<char*>(&rev_adjCount), sizeof(rev_adjCount));
    for (size_t i = 0; i < rev_adjCount; ++i) {
        Node node;
        node.deserialize(in);
        size_t neighborCount;
        in.read(reinterpret_cast<char*>(&neighborCount), sizeof(neighborCount));

        for (size_t j = 0; j < neighborCount; ++j) {
            Node neighbor;
            neighbor.deserialize(in);
            rev_adjList[node].insert(neighbor);
        }
    }

    // deserialize adjList
    size_t adjCount;
    in.read(reinterpret_cast<char*>(&adjCount), sizeof(adjCount));
    for (size_t i = 0; i < adjCount; ++i) {
        Node node;
        node.deserialize(in);
        size_t neighborCount;
        in.read(reinterpret_cast<char*>(&neighborCount), sizeof(neighborCount));

        for (size_t j = 0; j < neighborCount; ++j) {
            Node neighbor;
            neighbor.deserialize(in);
            adjList[node].insert(neighbor);
        }
    }

    // deserialize distance
    size_t distCount;
    in.read(reinterpret_cast<char*>(&distCount), sizeof(distCount));
    for (size_t i = 0; i < distCount; ++i) {
        Node src, dest;
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