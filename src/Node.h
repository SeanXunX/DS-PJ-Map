#pragma once

#include <fstream>
#include <string>
#include <tuple>

class Node
{
    friend double calculate_distance(const Node& node1, const Node &node2); 
    friend bool operator==(const Node &n1, const Node &n2);
public:
    Node() = default;
    Node(double _lng, double _lat) : lng(_lng), lat(_lat) {}
    Node(const std::pair<double, double> &coord) : lng(coord.first), lat(coord.second) {}
    
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
};

bool operator==(const Node &n1, const Node &n2);
double calculate_distance(const Node& node1, const Node &node2); 
double calculate_distance(double lng1, double lat1, double lng2, double lat2);