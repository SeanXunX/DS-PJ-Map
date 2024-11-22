#pragma once

#include <algorithm>
#include <vector>
#include <array>
#include "Node.h"
#include <memory>
#include <limits>
#include <fstream>

class KDNode {
    friend bool operator==(const KDNode &n1, const KDNode &n2);

    using node_t = KDNode;
    using node_ptr_t = std::shared_ptr<node_t>;
public:
    std::array<double, 2> data;
    node_ptr_t left;
    node_ptr_t right;

    KDNode() : left(nullptr), right(nullptr) {}
    KDNode(const std::array<double, 2> &_data) : data(_data), left(nullptr), right(nullptr) {} 
    KDNode(const Node &node) : left(nullptr), right(nullptr) {
        data[0] = node.getLng();
        data[1] = node.getLat();
    }
    void swap(KDNode &other) {
        std::swap(data, other.data);
        std::swap(left, other.left);
        std::swap(right, other.right);
    }

    // void serialize(std::ofstream &out);

    // void deserialize(std::ifstream &in);
};

// namespace std {
//     template <>
//     void swap(KDNode& a, KDNode& b) noexcept {
//         a.swap(b);
//     }
// }

class KDTree {
    using node_t = KDNode;
    using node_ptr_t = std::shared_ptr<node_t>;
public:
    KDTree() : root(nullptr) {}
    KDTree(std::vector<node_t> nodes, int depth = 0);

    void insert(const node_t &node);

    node_t search(const node_t &node) const;

    node_t nearest_neighbor(const node_t &node) const;

    void serialize(std::ofstream &out) const;

    void deserialize(std::ifstream &in);

private:
    node_ptr_t root;

    void build_recursive(node_ptr_t &cur, std::vector<node_t> &nodes, int depth);

    void insert_recursive(node_ptr_t &cur, const node_t &node, int depth);

    node_t search_recursive(const node_ptr_t &cur, const node_t &node, int depth) const;

    void nearest_neighbor_recursive(const node_ptr_t &cur, const node_t &node, int depth, node_t &nn, double &nn_dis) const;

    void serialize_recursive(const node_ptr_t &cur, std::ofstream &out) const;

    void deserialize_recursive(node_ptr_t &cur, std::ifstream &out);


};
