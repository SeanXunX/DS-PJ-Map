#pragma once

#include <fstream>
#include <string>
#include <tuple>

enum priority {
    unknown = 0,
    motorway = 80,
    trunk = 100,
    primary = 125,
    secondary = 150,
    tertiary = 200,
    unclassified = 300,
    residential = 400,
    service = 500,
    track = 1000,
    path = 5000,
    cycleway = 5500,
    footway = 6000,
};

// enum priority {
//     unknown = 100,
//     motorway = 100,
//     trunk = 100,
//     primary = 100,
//     secondary = 100,
//     tertiary = 100,
//     unclassified = 100,
//     residential = 100,
//     service = 100,
//     track = 100,
//     path = 100,
//     cycleway = 100,
//     footway = 100,
// };

class Node
{
    friend double calculate_distance(const Node& node1, const Node &node2); 
    friend double calculate_weighted_distance(const Node& node1, const Node &node2);
    friend bool operator==(const Node &n1, const Node &n2);
public:
    Node() = default;
    Node(double _lng, double _lat) : lng(_lng), lat(_lat), weight(unknown) {}
    Node(const std::pair<double, double> &coord) : Node(coord.first, coord.second) {}
    Node(double _lng, double _lat, priority w) : lng(_lng), lat(_lat), weight(w) {}
    Node(const std::pair<double, double> &coord, priority w) : lng(coord.first), lat(coord.second), weight(w) {}
    
    inline double getLng() const {
        return lng;
    }
    
    inline double getLat() const {
        return lat;
    }

    // TODO: change to hashmap or a better comparison method
    bool operator<(const Node &other) const{
        // return ((this->lat * 114) + (this->lng)) < (other.lat * 114 + other.lng);
        return std::tie(lng, lat) < std::tie(other.lng, other.lat);
    }

    void serialize(std::ofstream &out) const;

    void deserialize(std::ifstream &in);

private:
    double lng, lat;
    priority weight;
};

bool operator==(const Node &n1, const Node &n2);
double calculate_distance(const Node& node1, const Node &node2); 
double calculate_distance(double lng1, double lat1, double lng2, double lat2);
double calculate_weighted_distance(const Node& node1, const Node &node2);