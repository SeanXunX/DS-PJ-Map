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

inline bool isOneWay(const Json::Value &feature) {
    if (!feature["properties"]["oneway"].isNull() && feature["properties"]["oneway"].asString() == "yes") {
        return true;
    }
    return false;
}

inline bool isSideWalk(const Json::Value &feature) {
    if (!feature["properties"]["sidewalk"].isNull() && feature["properties"]["sidewalk"].asString() == "no") {
        return false;
    }
    return true;
}

// add highway to graph
void load_highway(const std::string &filename, Graph &graph)
{
    std::ifstream file(filename);
    Json::Value geojson;
    file >> geojson;

    for (const auto &feature : geojson["features"])
    {
        if (feature["geometry"]["type"].asString() == "LineString" && !feature["properties"]["highway"].isNull())
        {
            const auto &coordinates = feature["geometry"]["coordinates"];
            const string road_cat = feature["properties"]["highway"].asString();
            bool is_one_way = isOneWay(feature);
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
                if (!is_one_way && !graph.getNeighbors(n2).count(n1)) {
                    double distance = calculate_weighted_distance(n2, n1);
                    graph.addDirectedEdge(n2, n1, distance);
                }
            }
        } 
    }
}

void ped_load_highway(const std::string &filename, Graph &graph)
{
    std::ifstream file(filename);
    Json::Value geojson;
    file >> geojson;

    for (const auto &feature : geojson["features"])
    {
        if (feature["geometry"]["type"].asString() == "LineString" && !feature["properties"]["highway"].isNull())
        {
            const auto &coordinates = feature["geometry"]["coordinates"];
            const string road_cat = feature["properties"]["highway"].asString();
            bool is_one_way = false;
            bool is_sidewalk = isSideWalk(feature);
            if (!is_sidewalk) {
                continue;
            }
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
                    double distance = calculate_distance(n1, n2);
                    graph.addDirectedEdge(n1, n2, distance);
                }
                if (!is_one_way && !graph.getNeighbors(n2).count(n1)) {
                    double distance = calculate_distance(n2, n1);
                    graph.addDirectedEdge(n2, n1, distance);
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
        } else if (feature["geometry"]["type"].asString() == "MultiPolygon"
         && !feature["properties"]["name"].asString().empty()
         && !graph.location_mapContains(feature["properties"]["name"].asString())) {
            double lng = feature["geometry"]["coordinates"][0][0][0][0].asDouble();
            double lat = feature["geometry"]["coordinates"][0][0][0][1].asDouble();
            string name = feature["properties"]["name"].asString();
            graph.addNamePoint(name, {lng, lat});
        }
    }
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

    auto path = graph.AStar(start, goal);

    string export_path = working_path + "/public/shortest_path.geojson";
    export_path_to_geojson(path, export_path);
    std::cout << "Shortest path saved to shortest_path.geojson" << std::endl;
}

/**
 * Return the geojson result to output_string for websocket transmission.
 */
void calculate_shortest_path_by_name_to_string(const Graph &graph, const string &start_name, const string &goal_name, string &output_string) {
    auto start_coord = graph.queryByName(start_name);
    auto goal_coord = graph.queryByName(goal_name);

    Node start(start_coord);
    Node goal(goal_coord);

    cout << start_name << ":" << start.getLat() << "," << start.getLng() << endl;
    cout << goal_name << ":" << goal.getLat() << "," << goal.getLng() << endl;

    auto path = graph.AStar(start, goal);

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



void performFuzzyQuery(const std::string& locationName, websocketpp::connection_hdl hdl, server& wsServer, const Graph &graph) {
    auto locations = graph.fuzzySearch(locationName, 75.0, 20);
    Json::Value result(Json::arrayValue);

    for (const auto &location : locations) {
        result.append(location);
    }

    Json::FastWriter writer;
    std::string response = writer.write(result);

    wsServer.send(hdl, response, websocketpp::frame::opcode::text);
}

void performArbitrary(double startLat, double startLng, double endLat, double endLng, websocketpp::connection_hdl hdl, server& wsServer, const Graph &graph) {
    auto start_coord = graph.queryByArbitrary({startLng, startLat});
    auto end_coord = graph.queryByArbitrary({endLng, endLat});

    Node start(start_coord);
    Node end(end_coord);

    cout << start.getLat() << "," << start.getLng() << endl;
    cout << end.getLat() << "," << end.getLng() << endl;

    auto path = graph.AStar(start, end);

    string result;
    export_path_to_geojson_string(path, result);
    wsServer.send(hdl, result, websocketpp::frame::opcode::text);
}

void loadData(Graph &graph) {
    string binaryFilename = working_path + "/bin/graph_cache.bin";
    if (!loadGraph(graph, binaryFilename)) {
        cout << "Loading from geojson and building graph" << endl;
        load_highway(highway_file, graph);
        load_point(point_file, graph);
        saveGraph(graph, binaryFilename);
    } else {
        cout << "Graph loaded from binary cache." << endl;
    }
}

void ped_loadData(Graph &graph) {
    string binaryFilename = working_path + "/bin/ped_graph_cache.bin";
    if (!loadGraph(graph, binaryFilename)) {
        cout << "Loading from geojson and building graph" << endl;
        ped_load_highway(highway_file, graph);
        load_point(point_file, graph);
        saveGraph(graph, binaryFilename);
    } else {
        cout << "Graph loaded from binary cache." << endl;
    }
}

int main()
{
    // Load graph
    Graph graph, ped_graph;
    loadData(graph);
    ped_loadData(ped_graph);


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
                } else if (queryType == "arbitrary") {
                    double startLat = jsonData["startLocation"]["lat"].asDouble();
                    double startLng = jsonData["startLocation"]["lng"].asDouble();
                    double endLat = jsonData["endLocation"]["lat"].asDouble();
                    double endLng = jsonData["endLocation"]["lng"].asDouble();
                    std::cout << "Arbitrary two points:" << startLat << "," << startLng << "->" << endLat << "," << endLng << std::endl;
                    performArbitrary(startLat, startLng, endLat, endLng, hdl, wsServer, graph);
                } else if (queryType == "ped_path") {
                    std::string startLocation = jsonData["startLocation"].asString();
                    std::string endLocation = jsonData["endLocation"].asString();

                    std::cout << "Path query: " << startLocation << " -> " << endLocation << std::endl;
                    calculateAndRespond(startLocation, endLocation, hdl, wsServer, ped_graph);
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
