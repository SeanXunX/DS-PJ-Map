#include <cmath>
#include "Node.h"
#include <algorithm>

const double pi = M_PI, R = 6371000;


double rad(double d)
{
    return d * pi / 180.0;
}

double calculate_distance(double lng1, double lat1, double lng2, double lat2)
{
    double radLat1 = rad(lat1);
    double radLat2 = rad(lat2);
    double a = radLat1 - radLat2;
    double b = rad(lng1) - rad(lng2);
    double s = 2 * std::asin(std::sqrt(std::pow(std::sin(a / 2), 2) + std::cos(radLat1) * std::cos(radLat2) * std::pow(std::sin(b / 2), 2)));
    s = s * R;
    s = std::round(s * 10000) / 10000;
    return s;
}

double calculate_distance(const Node& node1, const Node &node2) {
    return calculate_distance(node1.lng, node1.lat, node2.lng, node2.lat);
}

double calculate_weighted_distance(const Node& node1, const Node &node2) {
    double w;
    if (node1.weight == unknown && node2.weight == unknown) {
        w = 1;
    } else {
        w = static_cast<double>(std::max(node1.weight, node2.weight)) / 100.0;
    }
    return calculate_distance(node1.lng, node1.lat, node2.lng, node2.lat) * w;
}

bool operator==(const Node &n1, const Node &n2) {
    return n1.lat == n2.lat && n1.lng == n2.lng && n1.weight == n2.weight;
}

void Node::serialize(std::ofstream &out) const
{
    out.write(reinterpret_cast<const char*>(&lng), sizeof(lng));
    out.write(reinterpret_cast<const char*>(&lat), sizeof(lat));
    out.write(reinterpret_cast<const char*>(&weight), sizeof(weight));
}

void Node::deserialize(std::ifstream &in)
{
    in.read(reinterpret_cast<char *>(&lng), sizeof(lng));
    in.read(reinterpret_cast<char *>(&lat), sizeof(lat));
    in.read(reinterpret_cast<char *>(&weight), sizeof(weight));
}
