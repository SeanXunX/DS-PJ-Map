#include "Graph.h"

#include <rapidfuzz/fuzz.hpp>
#include <map>
#include <queue>
#include <algorithm>
#include <limits>
#include <set>

using std::string;
using std::vector;
using std::map;

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

std::vector<string> Graph::fuzzySearch(const std::string &query, double threshold,  std::multimap<double, std::string>::size_type max_size) const
{
    std::multimap<double, string> res;
    for (const auto &entry : location_map) {
        double score = rapidfuzz::fuzz::ratio(query, entry.first);
        if (query.size() <= entry.first.size()) {
            double partial_score = rapidfuzz::fuzz::partial_ratio(query, entry.first);
            score = std::max(score, partial_score);
        }
        if (query == entry.first) {
            score = 200;
        }
        if (res.size() < max_size) {
            res.insert({score, entry.first});
        } else if (score >= threshold && score > res.begin()->first) {
            res.erase(res.begin());
            res.insert({score, entry.first});
        }
    }
    vector<string> result;
    for (auto itr = res.rbegin(); itr != res.rend(); ++itr) {
        result.push_back(itr->second);
    }
    return result;
}

std::vector<Node> constructPath(const map<Node, Node> &cameFromStart, Node &cur_start, const map<Node, Node> &cameFromGoal, Node &cur_goal) {
    vector<Node> path;
    while (cameFromStart.find(cur_start) != cameFromStart.end()) {
        path.push_back(cur_start);
        cur_start = cameFromStart.at(cur_start);
    }
    path.push_back(cur_start);
    std::reverse(path.begin(), path.end()); 
    while (cameFromGoal.find(cur_goal) != cameFromGoal.end()) {
        path.push_back(cur_goal);
        cur_goal = cameFromGoal.at(cur_goal);
    }
    path.push_back(cur_goal);
    return path;
}

double potential_function_forward(const Node &cur, const Node &dst) {
    return calculate_distance(cur, dst);
}

double potential_function_reverse(const Node &cur, const Node &start) {
    return calculate_distance(cur, start);
}

double predict_forward(const Node &cur, const Node &start, const Node &dst, double dis_s_t) {
    return 0.5 * (potential_function_forward(cur, dst) - potential_function_reverse(cur, start)) + 0.5 * dis_s_t;
}

double predict_reverse(const Node &cur, const Node &start, const Node &dst, double dis_s_t) {
    return 0.5 * ( - potential_function_forward(cur, dst) + potential_function_reverse(cur, start)) + 0.5 * dis_s_t;
}

std::vector<Node> Graph::BiAStar(const Node &start, const Node &dst) const
{
    if (!containsNode(start) || !containsNode(dst)) {
        throw std::runtime_error("Start or dst node note found in graph.");
    }

    double dis_s_t = calculate_distance(start, dst);

    double min_length = std::numeric_limits<double>::infinity();

    using QueueItem = std::pair<double, Node>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> forward;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> reverse;

    std::map<Node, Node> cameFromStart, cameFromDst;
    std::map<Node, double> disStart, disDst;
    std::map<Node, double> fStart, fDst;

    std::set<Node> visited_forward, visited_reverse;

    for (const auto &node : adjList) {
        disStart[node.first] = std::numeric_limits<double>::infinity();
        fStart[node.first] = std::numeric_limits<double>::infinity();
    }

    for (const auto &node : rev_adjList) {
        disDst[node.first] = std::numeric_limits<double>::infinity();
        fDst[node.first] = std::numeric_limits<double>::infinity();
    }

    disStart[start] = 0;
    fStart[start] = predict_forward(start, start, dst, dis_s_t);

    disDst[dst] = 0;
    fDst[dst] = predict_reverse(dst, start, dst, dis_s_t);

    forward.push({fStart[start], start});
    reverse.push({fDst[dst], dst});

    while (!forward.empty() && !reverse.empty()) {
        auto top_f = forward.top();
        forward.pop();

        auto top_r = reverse.top();
        reverse.pop();

        bool isIntersect = ((top_f.second == top_r.second)
         ||( visited_forward.count(top_r.second)
         && visited_reverse.count(top_f.second)));

        if (isIntersect && (top_f.first + top_r.first >= min_length + dis_s_t)) {
            return constructPath(cameFromStart, top_f.second, cameFromDst, top_r.second);
        }

        if (top_f.first <= fStart[top_f.second]) {
            visited_forward.insert(top_f.second);
            for (const Node &neighbor : getNeighbors(top_f.second)) {
                double t = disStart[top_f.second] + distances.at({top_f.second, neighbor});
                if (t < disStart[neighbor]) {
                    cameFromStart[neighbor] = top_f.second;
                    disStart[neighbor] = t;
                    fStart[neighbor] = disStart[neighbor] + predict_forward(neighbor, start, dst, dis_s_t);
                    forward.push({fStart[neighbor], neighbor});
                }
                if (visited_reverse.count(neighbor)) {
                    min_length = std::min(min_length, disStart[top_f.second] + disDst[neighbor] + distances.at({top_f.second, neighbor}));
                }
            }
        }

        if (top_r.first <= fDst[top_r.second]) {
            visited_reverse.insert(top_r.second);
            for (const Node &neighbor : rev_getNeighbors(top_r.second)) {
                double t = disDst[top_r.second] + distances.at({neighbor, top_r.second});
                if (t < disDst[neighbor]) {
                    cameFromDst[neighbor] = top_r.second;
                    disDst[neighbor] = t;
                    fDst[neighbor] = disDst[neighbor] + predict_reverse(neighbor, start, dst, dis_s_t);
                    reverse.push({fDst[neighbor], neighbor});
                }
                if (visited_forward.count(neighbor)) {
                    min_length = std::min(min_length, disDst[top_r.second] + disStart[neighbor] + distances.at({neighbor, top_r.second}));
                }
            }
        }
    }
    return {};
}


std::vector<Node> reconstructPath(const std::map<Node, Node> &cameFrom, Node current)
{
    std::vector<Node> path;
    while (cameFrom.find(current) != cameFrom.end()) {
        path.push_back(current);
        current = cameFrom.at(current);
    }
    path.push_back(current);
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<Node> Graph::AStar(const Node &start, const Node &goal) const
{
    if (!containsNode(start) || !containsNode(goal)) {
        throw std::runtime_error("Start or goal node not found in graph.");
    }

    // Priority queue for A* search
    using QueueItem = std::pair<double, Node>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> openSet;
    openSet.push({0, start});

    std::map<Node, Node> cameFrom;
    std::map<Node, double> gScore;
    std::map<Node, double> fScore;

    for (const auto &node : adjList) {
        gScore[node.first] = std::numeric_limits<double>::infinity();
        fScore[node.first] = std::numeric_limits<double>::infinity();
    }

    gScore[start] = 0;
    fScore[start] = calculate_distance(start, goal);

    while (!openSet.empty()) {
        Node current = openSet.top().second;
        openSet.pop();

        if (current == goal) {
            return reconstructPath(cameFrom, current);
        }

        for (const Node &neighbor : getNeighbors(current)) {
            double tentative_gScore = gScore[current] + distances.at({current, neighbor});
            if (tentative_gScore < gScore[neighbor]) {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentative_gScore;
                fScore[neighbor] = gScore[neighbor] + calculate_distance(neighbor, goal);
                openSet.push({fScore[neighbor], neighbor});
            }
        }
    }

    // Return an empty path if no path found
    return {};
}


