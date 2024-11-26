#include "Graph.h"
#include "Node.h"
#include <jsoncpp/json/json.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <string>
#include <queue>
#include <algorithm>
#include <fstream>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using std::map;
using std::vector;
using std::string;
using std::ofstream;
using std::ifstream;
using std::cout;
using std::endl;


typedef websocketpp::server<websocketpp::config::asio> server;

const string working_path = "/home/sean/DS-PJ-Map";
const string highway_file = working_path + "/data/shanghai-highway.geojson";
const string point_file = working_path + "/data/shanghai.geojson";

void saveGraph(const Graph &graph, const string &filename) {
    std::ofstream out(filename, std::ios::binary);
    graph.serialize(out);
    out.close();
}

bool loadGraph(Graph &graph, const string &filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        return false;
    }
    graph.deserialize(in);
    in.close();
    return true;
}

priority getPriorityFromString(const string &str) {
    if (str == "motorway_junction") {
        return motorway;
    } else if (str == "trunk") {
        return trunk;
    } else if (str == "primary") {
        return primary;
    } else if (str == "secondary") {
        return secondary;
    } else if (str == "tertiary") {
        return tertiary;
    } else if (str == "unclassified") {
        return unclassified;
    } else if (str == "residential") {
        return residential;
    } else if (str == "service") {
        return service;
    } else if (str == "track") {
        return track;
    } else if (str == "path") {
        return path;
    } else if (str == "footway") {
        return footway;
    } else if (str == "cycleway") {
        return cycleway;
    } else {
        return unknown;
    }
}

// add highway to graph
void load_highway(const std::string &filename, Graph &graph)
{
    std::ifstream file(filename);
    Json::Value geojson;
    file >> geojson;

    for (const auto &feature : geojson["features"])
    {
        if (feature["geometry"]["type"].asString() == "LineString")
        {
            const auto &coordinates = feature["geometry"]["coordinates"];
            const string road_cat = feature["properties"]["highway"].asString();
            for (Json::ArrayIndex i = 0; i < coordinates.size() - 1; ++i)
            {
                double lng1 = coordinates[i][0].asDouble();
                double lat1 = coordinates[i][1].asDouble();
                double lng2 = coordinates[i + 1][0].asDouble();
                double lat2 = coordinates[i + 1][1].asDouble();

                Node n1(lng1, lat1, getPriorityFromString(road_cat)), n2(lng2, lat2, getPriorityFromString(road_cat));

                graph.addNode(n1);
                graph.addNode2KDTree(n1);
                graph.addNode(n2);
                graph.addNode2KDTree(n2);

                if (!graph.getNeighbors(n1).count(n2))
                {
                    double distance = calculate_weighted_distance(n1, n2);
                    graph.addDirectedEdge(n1, n2, distance);
                }
            }
        } 
    }
}

void load_point(const string &filename, Graph &graph) {
    std::ifstream file(filename);
    Json::Value geojson;
    file >> geojson;

    for (const auto &feature : geojson["features"]) {
        if (feature["geometry"]["type"].asString() == "Point" && !feature["properties"]["name"].asString().empty()) {
            double lng = feature["geometry"]["coordinates"][0].asDouble();
            double lat = feature["geometry"]["coordinates"][1].asDouble();
            string name = feature["properties"]["name"].asString();
            graph.addNamePoint(name, {lng, lat});
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

std::vector<Node> AStar(const Graph &graph, const Node &start, const Node &goal)
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

std::vector<Node> BiAStar(const Graph &graph, const Node &start, const Node &goal)
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
                // double t = gScoreStart[current_start] + calculate_weighted_distance(current_start, neighbor);
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
                // double t = gScoreStart[current_start] + calculate_weighted_distance(neighbor, current_goal);
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


void export_path_to_geojson_string(const std::vector<Node> &path, std::string &output_string) {
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

        // Json::Value point;
        // point["type"] = "Feature";
        // point["geometry"]["type"] = "Point";
        // point["geometry"]["coordinates"] = coord;
        // point["properties"]["popupContent"] = "coord";
        // geojson["features"].append(point);

        feature["geometry"]["coordinates"].append(coord);
    }

    geojson["features"].append(feature);

    std::ostringstream out;
    out << geojson;

    output_string = out.str();
}

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

        // Json::Value point;
        // point["type"] = "Feature";
        // point["geometry"]["type"] = "Point";
        // point["geometry"]["coordinates"] = coord;
        // point["properties"]["popupContent"] = "coord";
        // geojson["features"].append(point);

        feature["geometry"]["coordinates"].append(coord);
    }

    geojson["features"].append(feature);

    std::ofstream file(output_filename);
    file << geojson;
    file.close();
}

void calculate_shortest_path_by_name(const Graph &graph, const string &start_name, const string &goal_name) {
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

void calculate_shortest_path_by_name_to_string(const Graph &graph, const string &start_name, const string &goal_name, string &output_string) {
    auto start_coord = graph.queryByName(start_name);
    auto goal_coord = graph.queryByName(goal_name);

    Node start(start_coord);
    Node goal(goal_coord);

    cout << start_name << ":" << start.getLat() << "," << start.getLng() << endl;
    cout << goal_name << ":" << goal.getLat() << "," << goal.getLng() << endl;

    auto path = BiAStar(graph, start, goal);

    export_path_to_geojson_string(path, output_string);
}



void calculateAndRespond(const std::string& startLocation, const std::string& endLocation, websocketpp::connection_hdl hdl, server& wsServer, const Graph &graph) {
    string result;
    calculate_shortest_path_by_name_to_string(graph, startLocation, endLocation, result);
    // std::cout << result << std::endl;
    if (result.empty()) {
        wsServer.send(hdl, "Cannot find path!", websocketpp::frame::opcode::text);
        return;
    }

    // 发送结果
    wsServer.send(hdl, result, websocketpp::frame::opcode::text);
}

void loadData(Graph &graph) {
    string binaryFilename = working_path + "/src/graph_cache.bin";
    if (!loadGraph(graph, binaryFilename)) {
        cout << "Loading from geojson and building graph" << endl;
        load_highway(highway_file, graph);
        load_point(point_file, graph);
        saveGraph(graph, binaryFilename);
    } else {
        cout << "Graph loaded from binary cache." << endl;
    }
}

void performFuzzyQuery(const std::string& locationName, websocketpp::connection_hdl hdl, server& wsServer, const Graph &graph) {
    auto locations = graph.fuzzySearch(locationName, 80.0, 10);
    Json::Value result(Json::arrayValue);

    for (const auto &location : locations) {
        result.append(location);
    }

    Json::FastWriter writer;
    std::string response = writer.write(result);

    wsServer.send(hdl, response, websocketpp::frame::opcode::text);
}

int main()
{
    // Load graph
    Graph graph;
    loadData(graph);


    server wsServer;
    wsServer.init_asio();
    
    wsServer.set_message_handler([&](websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg) {
        try {
            std::string payload = msg->get_payload();
            Json::Reader reader;
            Json::Value jsonData;

            if (reader.parse(payload, jsonData)) {
                std::string queryType = jsonData["queryType"].asString();
                
                if (queryType == "path") {
                    std::string startLocation = jsonData["startLocation"].asString();
                    std::string endLocation = jsonData["endLocation"].asString();

                    std::cout << "Path query: " << startLocation << " -> " << endLocation << std::endl;
                    calculateAndRespond(startLocation, endLocation, hdl, wsServer, graph);

                } else if (queryType == "fuzzy") {
                    std::string locationName = jsonData["locationName"].asString();

                    std::cout << "Fuzzy query: " << locationName << std::endl;
                    performFuzzyQuery(locationName, hdl, wsServer, graph); // Implement this function to return fuzzy matches
                }
            } else {
                std::cerr << "Failed to parse JSON: " << reader.getFormattedErrorMessages() << std::endl;
            }
        } catch (const std::exception& e) {
            wsServer.send(hdl, std::string("Error: ") + e.what(), websocketpp::frame::opcode::text);
        }
    });

    wsServer.listen(3002);
    wsServer.start_accept();

    std::cout << "C++ WebSocket server listening on port 3002..." << std::endl;

    wsServer.run();



    return 0;
}
