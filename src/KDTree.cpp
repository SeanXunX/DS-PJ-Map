#include "KDTree.h"

double distance(const KDNode &node1, const KDNode &node2) {
    return calculate_distance(node1.data[0], node1.data[1], node2.data[0], node2.data[1]);
}

bool operator==(const KDNode &n1, const KDNode &n2) {
    return n1.data == n2.data;
}

KDTree::KDTree(std::vector<node_t> nodes, int depth) : root(nullptr) {
    
    build_recursive(root, nodes, depth);
}

inline void KDTree::build_recursive(node_ptr_t &cur, std::vector<node_t> &nodes, int depth)
{
    if (nodes.empty()) {
        cur = nullptr;
        return;
    }

    size_t axis = depth % 2;

    size_t median = nodes.size() / 2;

    auto cmp = [axis](const node_t &node1, const node_t &node2) {
        return node1.data[axis] < node2.data[axis];
    };

    std::nth_element(nodes.begin(), nodes.begin() + median, nodes.end(), cmp);

    cur = std::make_shared<KDNode>(nodes[median]);

    std::vector<node_t> leftNodes(nodes.begin(), nodes.begin() + median);
    std::vector<node_t> rightNodes(nodes.begin() + median + 1, nodes.end());

    build_recursive(cur->left, leftNodes, depth + 1);
    build_recursive(cur->left, rightNodes, depth + 1);
}

inline void KDTree::insert_recursive(node_ptr_t &cur, const node_t &node, int depth)
{
    if (cur == nullptr) {
        cur = std::make_shared<node_t>(node);
        return;
    }
    size_t axis = depth % 2;        
    auto cmp = [axis](const node_t &node1, const node_t &node2) {
        return node1.data[axis] < node2.data[axis];
    };
    if (cmp(node, *cur)) {
        // node < cur
        insert_recursive(cur->left, node, depth + 1);
    } else {
        insert_recursive(cur->right, node, depth + 1);
    }
}

KDTree::node_t KDTree::search_recursive(const node_ptr_t &cur, const node_t &node, int depth) const
{
    if (cur == nullptr) {
        return node_t();
    }
    
    if (*cur == node) {
        return *cur;
    }
    size_t axis = depth % 2;        
    auto cmp = [axis](const node_t &node1, const node_t &node2) {
        return node1.data[axis] < node2.data[axis];
    };
    if (cmp(node, *cur)) {
        // node < cur
        return search_recursive(cur->left, node, depth + 1);
    } else {
        return search_recursive(cur->right, node, depth + 1);
    }
}

inline void KDTree::nearest_neighbor_recursive(const node_ptr_t &cur, const node_t &node, int depth, node_t &nn, double &nn_dis) const
{
    if (cur == nullptr) {
        // Reaches the leaf node
        return;
    }

    size_t axis = depth % 2;        
    auto cmp = [axis](const node_t &node1, const node_t &node2) {
        return node1.data[axis] < node2.data[axis];
    };

    node_ptr_t next, other;    
    if (cmp(node, *cur)) {
        // node < *cur
        next = cur->left;
        other = cur->right;
    } else {
        next = cur->right;
        other = cur->left;
    }
    nearest_neighbor_recursive(next, node, depth + 1, nn, nn_dis);

    double t_dis = distance(node, *cur);
    if (t_dis < nn_dis) {
        nn = *cur;
        nn_dis = t_dis;
    }

    // Check if radius is large enough to reach the other region
    if (nn_dis > std::abs(cur->data[axis] - nn.data[axis])) {
        nearest_neighbor_recursive(other, node, depth + 1, nn, nn_dis);
    }

}

void KDTree::insert(const node_t &node) {
    insert_recursive(root, node, 0);    
}

KDTree::node_t KDTree::search(const node_t &node) const
{
    return search_recursive(root, node, 0);
}

KDTree::node_t KDTree::nearest_neighbor(const node_t &node) const
{
    double nn_dis = std::numeric_limits<double>::infinity();
    node_t nn;
    nearest_neighbor_recursive(root, node, 0, nn, nn_dis);
    return nn;
}

void KDTree::serialize(std::ofstream &out) const {
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open file for writing");
    }
    serialize_recursive(root, out);
}

void KDTree::serialize_recursive(const node_ptr_t &cur, std::ofstream &out) const {
    if (!cur) {
        // 用标志表示空节点
        bool is_null = true;
        out.write(reinterpret_cast<const char*>(&is_null), sizeof(is_null));
        return;
    }

    // 写入当前节点的存在标志
    bool is_null = false;
    out.write(reinterpret_cast<const char*>(&is_null), sizeof(is_null));

    // 写入节点数据
    out.write(reinterpret_cast<const char*>(&cur->data[0]), sizeof(cur->data[0]));
    out.write(reinterpret_cast<const char*>(&cur->data[1]), sizeof(cur->data[1]));

    // 递归写入左右子节点
    serialize_recursive(cur->left, out);
    serialize_recursive(cur->right, out);
}

void KDTree::deserialize(std::ifstream &in) {
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open file for reading");
    }
    deserialize_recursive(root, in);
}

void KDTree::deserialize_recursive(node_ptr_t &cur, std::ifstream &in) {
    // 读取当前节点的存在标志
    bool is_null;
    in.read(reinterpret_cast<char*>(&is_null), sizeof(is_null));

    if (is_null) {
        // 空节点
        cur = nullptr;
        return;
    }

    // 创建新节点
    cur = std::make_shared<KDNode>();

    // 读取节点数据
    in.read(reinterpret_cast<char*>(&cur->data[0]), sizeof(cur->data[0]));
    in.read(reinterpret_cast<char*>(&cur->data[1]), sizeof(cur->data[1]));

    // 递归读取左右子节点
    deserialize_recursive(cur->left, in);
    deserialize_recursive(cur->right, in);
}

// void KDNode::serialize(std::ofstream &out)
// {
//     out.write(reinterpret_cast<const char*>(&data[0]), sizeof(data[0]));
//     out.write(reinterpret_cast<const char*>(&data[1]), sizeof(data[1]));

//     out.write(reinterpret_cast<const char*>(&left), sizeof(left));
//     out.write(reinterpret_cast<const char*>(&right), sizeof(right));

// }

// void KDNode::deserialize(std::ifstream &in)
// {
//     in.read(reinterpret_cast<char *>(&data[0]), sizeof(data[0]));
//     in.read(reinterpret_cast<char *>(&data[1]), sizeof(data[1]));

//     in.read(reinterpret_cast<char *>(&left), sizeof(left));
//     in.read(reinterpret_cast<char *>(&right), sizeof(right));

// }
