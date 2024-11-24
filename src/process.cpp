#include "Graph.h"
#include "Node.h"
#include <jsoncpp/json/json.h>
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <string>
#include <queue>
#include <algorithm>
#include <fstream>

using std::map;
using std::vector;
using std::string;
using std::ofstream;
using std::ifstream;
using std::cout;
using std::endl;

const string working_path = "/home/sean/DS-PJ-Map";

void saveGraph(const Graph<Node> &graph, const string &filename) {
    std::ofstream out(filename, std::ios::binary);
    graph.serialize(out);
    out.close();
}

bool loadGraph(Graph<Node> &graph, const string &filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        return false;
    }
    graph.deserialize(in);
    in.close();
    return true;
}

// 解析 GeoJSON 并构建图
void load_geojson(const std::string &filename, Graph<Node> &graph)
{
    std::ifstream file(filename);
    Json::Value geojson;
    file >> geojson;

    for (const auto &feature : geojson["features"])
    {
        if (feature["geometry"]["type"].asString() == "LineString")
        {
            const auto &coordinates = feature["geometry"]["coordinates"];
            for (Json::ArrayIndex i = 0; i < coordinates.size() - 1; ++i)
            {
                double lng1 = coordinates[i][0].asDouble();
                double lat1 = coordinates[i][1].asDouble();
                double lng2 = coordinates[i + 1][0].asDouble();
                double lat2 = coordinates[i + 1][1].asDouble();

                Node n1(lng1, lat1), n2(lng2, lat2);

                graph.addNode(n1);
                graph.addNode2KDTree(n1);
                graph.addNode(n2);
                graph.addNode2KDTree(n2);

                if (!graph.getNeighbors(n1).count(n2))
                {
                    double distance = calculate_distance(lng1, lat1, lng2, lat2);
                    graph.addDirectedEdge(n1, n2, distance);
                }
            }
        } else if (feature["geometry"]["type"].asString() == "Point" && !feature["properties"]["name"].asString().empty()) {
            const auto &coordinate = feature["geometry"]["coordinates"];
            double lng = coordinate[0].asDouble();
            double lat = coordinate[1].asDouble();
            graph.addNamePoint(feature["properties"]["name"].asString(), {lng, lat});
        }
    }
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

std::vector<Node> AStar(const Graph<Node> &graph, const Node &start, const Node &goal)
{
    if (!graph.containsNode(start) || !graph.containsNode(goal)) {
        throw std::runtime_error("Start or goal node not found in graph.");
    }

    // Priority queue for A* search
    using QueueItem = std::pair<double, Node>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> openSet;
    openSet.push({0, start});

    std::map<Node, Node> cameFrom;
    std::map<Node, double> gScore;
    std::map<Node, double> fScore;

    for (const auto &node : graph.adjList) {
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

        for (const Node &neighbor : graph.getNeighbors(current)) {
            double tentative_gScore = gScore[current] + graph.distances.at({current, neighbor});
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

std::vector<Node> BiAStar(const Graph<Node> &graph, const Node &start, const Node &goal)
{
    if (!graph.containsNode(start) || !graph.containsNode(goal)) {
        throw std::runtime_error("Start or goal node not found in graph.");
    }

    // Priority queue for A* search
    using QueueItem = std::pair<double, Node>;

    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> openSetStart;
    openSetStart.push({0, start});

    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> openSetGoal;
    openSetGoal.push({0, goal});

    std::map<Node, Node> cameFromStart, cameFromGoal;
    std::map<Node, double> gScoreStart, gScoreGoal;
    std::map<Node, double> fScoreStart, fScoreGoal;

    for (const auto &node : graph.adjList) {
        gScoreStart[node.first] = std::numeric_limits<double>::infinity();
        fScoreStart[node.first] = std::numeric_limits<double>::infinity();
    }

    for (const auto &node : graph.rev_adjList) {
        gScoreGoal[node.first] = std::numeric_limits<double>::infinity();
        fScoreGoal[node.first] = std::numeric_limits<double>::infinity();
    }

    gScoreStart[start] = 0;
    gScoreGoal[goal] = 0;

    fScoreStart[start] = calculate_distance(start, goal);
    fScoreGoal[goal] = calculate_distance(goal, start);

    Node current_start = start, current_goal = goal;

    while (!openSetGoal.empty() && !openSetStart.empty()) {
        if (!openSetStart.empty()) {
            current_start = openSetStart.top().second;
            openSetStart.pop();
            if (current_start == current_goal) {
                return constructPath(cameFromStart, current_start, cameFromGoal, current_goal);
            }
            for (const Node &neighbor : graph.getNeighbors(current_start)) {
                double t = gScoreStart[current_start] + graph.distances.at({current_start, neighbor});
                if (t < gScoreStart[neighbor]) {
                    cameFromStart[neighbor] = current_start;
                    gScoreStart[neighbor] = t;
                    fScoreStart[neighbor] = gScoreStart[neighbor] + calculate_distance(neighbor, goal);
                    openSetStart.push({fScoreStart[neighbor], neighbor});
                }
            }
        }

        if (!openSetGoal.empty()) {
            current_goal = openSetGoal.top().second;
            openSetGoal.pop();
            if (current_goal == current_start) {
                return constructPath(cameFromStart, current_start, cameFromGoal, current_goal);
            }
            for (const Node &neighbor : graph.rev_getNeighbors(current_goal)) {
                double t = gScoreGoal[current_goal] + graph.distances.at({neighbor, current_goal});
                if (t < gScoreGoal[neighbor]) {
                    cameFromGoal[neighbor] = current_goal;
                    gScoreGoal[neighbor] = t;
                    fScoreGoal[neighbor] = gScoreGoal[neighbor] + calculate_distance(neighbor, start);
                    openSetGoal.push({fScoreGoal[neighbor], neighbor});
                }
            }
        }
    }

    // Return an empty path if no path found
    return {};
}


		// {"type":"Feature","geometry":{"type":"Point","coordinates":[121.141005,31.2719763]}},

void export_path_to_geojson(const std::vector<Node> &path, const std::string &output_filename)
{
    Json::Value geojson;
    geojson["type"] = "FeatureCollection";
    Json::Value feature;
    feature["type"] = "Feature";
    feature["geometry"]["type"] = "LineString";

    for (const auto &node : path)
    {
        Json::Value coord;
        coord.append(node.getLng());
        coord.append(node.getLat());

        Json::Value point;
        point["type"] = "Feature";
        point["geometry"]["type"] = "Point";
        point["geometry"]["coordinates"] = coord;
        point["properties"]["popupContent"] = "coord";
        geojson["features"].append(point);

        feature["geometry"]["coordinates"].append(coord);
    }

    geojson["features"].append(feature);

    std::ofstream file(output_filename);
    file << geojson;
    file.close();
}

void calculate_shortest_path_by_name(const Graph<Node> &graph, const string &start_name, const string &goal_name) {
    auto start_coord = graph.queryByName(start_name);
    auto goal_coord = graph.queryByName(goal_name);

    Node start(start_coord);
    Node goal(goal_coord);

    cout << start_name << ":" << start.getLat() << "," << start.getLng() << endl;
    cout << goal_name << ":" << goal.getLat() << "," << goal.getLng() << endl;

    auto path = BiAStar(graph, start, goal);

    string export_path = working_path + "/public/shortest_path.geojson";
    export_path_to_geojson(path, export_path);
    std::cout << "Shortest path saved to shortest_path.geojson" << std::endl;
}

int main(int argc, char *argv[])
{
    // Load graph
    Graph<Node> graph;
    string binaryFilename = working_path + "/src/graph_cache.bin";
    if (!loadGraph(graph, binaryFilename)) {
        string data_path = working_path + "/data/shanghai-primary.geojson";
        cout << "Loading from geojson and building graph" << endl;
        load_geojson(data_path, graph);
        saveGraph(graph, binaryFilename);
    } else {
        cout << "Graph loaded from binary cache." << endl;
    }

    if (argc == 3) {
        cout << "Calculating shortest path~" << endl;
        calculate_shortest_path_by_name(graph, argv[1], argv[2]);
    }

    return 0;
}
