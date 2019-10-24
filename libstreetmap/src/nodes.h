/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   nodes.h
 * Author: hoaustin
 *
 * Created on March 14, 2019, 9:20 PM
 */

#ifndef NODES_H
#define NODES_H
#include <vector>
#include <unordered_map>
#include<cfloat>
#include "StreetsDatabaseAPI.h"
#define NO_EDGE -1
class Node{
public:
    std::vector<unsigned> out_edge;
    std::vector<unsigned>inter;
    std::vector<double>time;
    unsigned id;
    LatLon position;
    int reaching_edge=NO_EDGE;
    double best_time = DBL_MAX;
    
};

extern std::vector <Node> node_list;
extern double max_speed;
#endif /* NODES_H */

