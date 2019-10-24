/* 
 * Copyright 2019 University of Toronto
 *
 * Permission is hereby granted, to use this software and associated 
 * documentation files (the "Software") in course work at the University 
 * of Toronto, or for personal use. Other uses are prohibited, in 
 * particular the distribution of the Software either publicly or to third 
 * parties.
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * m3.cpp file implemented by Austin Ho, Japtegh Singh, En-Mien Yang
 * This file implements high level functions to determine the optimal path between intersections.
 * Functions implemented include find turn type between street segments, 
 */

#include <cfloat>
#include <vector>
#include <list>
#include <string>
#include "m3.h"
#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include <cmath>
#include <algorithm>
#include <queue>
#include "nodes.h"



struct WaveElem{
    Node *node;
    
    double travel_time;
    double weight;
    WaveElem(Node *n, double time, double value){
    
        node=n;
        travel_time=time;
        weight=value;
        
    }
};

//Comparator structure for priority queue implementation of WaveElems with min heap
struct comp{
    bool operator()(WaveElem e1, WaveElem e2){
        return e1.weight>e2.weight;
    }
    
};


double m3_lon_to_x(double lon, double lat1, double lat2);
double m3_lat_to_y(double lat);
void latlon_to_point(LatLon loc1, LatLon loc2, double &x1, double &y1, double  &x2, double &y2);
bool bfsPath(unsigned sourceID, unsigned destID, double right_turn_penalty, double left_turn_penalty, std::vector<Node *> &modified_nodes);
void bfsTraceBack(unsigned destID, std::vector<unsigned> &path);



// Returns the turn type between two given segments.
// street_segment1 is the incoming segment and street_segment2 is the outgoing
// one.
// If the two street segments do not intersect, turn type is NONE.
// Otherwise if the two segments have the same street ID, turn type is 
// STRAIGHT.  
// If the two segments have different street ids, turn type is LEFT if 
// going from street_segment1 to street_segment2 involves a LEFT turn 
// and RIGHT otherwise.  Note that this means that even a 0-degree turn
// (same direction) is considered a RIGHT turn when the two street segments
// have different street IDs.
TurnType find_turn_type(unsigned street_segment1, unsigned street_segment2){
    
    double x1_start, y1_start, x2_start, y2_start, x1_end, y1_end, x2_end, y2_end;
    InfoStreetSegment s1=getInfoStreetSegment(street_segment1);
    InfoStreetSegment s2=getInfoStreetSegment(street_segment2);
    //Check if segments are from the same street
    if(s1.streetID==s2.streetID){
        return TurnType::STRAIGHT;
    
    //Checks if segments are connected to->from to use appropriate points
    }else if(s1.to==s2.from){
        
        //Checks if there are curve points in segment 1 and uses appropriate points
        if(s1.curvePointCount==0){
            latlon_to_point(node_list[s1.from].position, node_list[s1.to].position, x1_start, y1_start,x2_start,y2_start);  
        }else{
            latlon_to_point(getStreetSegmentCurvePoint(s1.curvePointCount-1, street_segment1),node_list[s1.to].position, x1_start, y1_start, x2_start, y2_start);
        }
        
        //Checks if there are curve points in segment 2 and uses appropriate points
        if(s2.curvePointCount==0){
            latlon_to_point(node_list[s2.from].position,node_list[s2.to].position, x1_end, y1_end,x2_end, y2_end);
        }else{
            latlon_to_point(node_list[s2.from].position,getStreetSegmentCurvePoint(0, street_segment2), x1_end, y1_end,x2_end, y2_end);
        }
    
    //Checks if segments are connected to->to to use appropriate points
    }else if(s1.to==s2.to){
        
        //Checks if there are curve points in segment 1 and uses appropriate points
        if(s1.curvePointCount==0){
            latlon_to_point(node_list[s1.from].position, node_list[s1.to].position, x1_start, y1_start,x2_start,y2_start);  
        }else{
            latlon_to_point(getStreetSegmentCurvePoint(s1.curvePointCount-1, street_segment1),node_list[s1.to].position, x1_start, y1_start, x2_start, y2_start);
        }
        
        //Checks if there are curve points in segment 2 and uses appropriate points
        if(s2.curvePointCount==0){
            latlon_to_point(node_list[s2.to].position,node_list[s2.from].position, x1_end, y1_end,x2_end, y2_end);
        }else{
            latlon_to_point(node_list[s2.to].position,getStreetSegmentCurvePoint(s2.curvePointCount-1, street_segment2), x1_end, y1_end,x2_end, y2_end);
        }
    
    //Checks if segments are connected from->from to use appropriate points
    }else if(s1.from==s2.from){
        
        //Checks if there are curve points in segment 1 and uses appropriate points
        if(s1.curvePointCount==0){
            latlon_to_point(node_list[s1.to].position, node_list[s1.from].position, x1_start, y1_start,x2_start,y2_start);  
        }else{
            latlon_to_point(getStreetSegmentCurvePoint(0, street_segment1),node_list[s1.from].position, x1_start, y1_start, x2_start, y2_start);
        }
        
        //Checks if there are curve points in segment 2 and uses appropriate points
        if(s2.curvePointCount==0){
            latlon_to_point(node_list[s2.from].position,node_list[s2.to].position, x1_end, y1_end,x2_end, y2_end);
        }else{
            latlon_to_point(node_list[s2.from].position,getStreetSegmentCurvePoint(0, street_segment2), x1_end, y1_end,x2_end, y2_end);
        }
        
    //Checks if segments are connected from->to to use appropriate points
    }else if(s1.from==s2.to){
        
        //Checks if there are curve points in segment 1 and uses appropriate points
        if(s1.curvePointCount==0){
            latlon_to_point(node_list[s1.to].position, node_list[s1.from].position, x1_start, y1_start,x2_start,y2_start);  
        }else{
            latlon_to_point(getStreetSegmentCurvePoint(0, street_segment1),node_list[s1.from].position, x1_start, y1_start, x2_start, y2_start);
        }
        
        //Checks if there are curve points in segment 2 and uses appropriate points
        if(s2.curvePointCount==0){
            latlon_to_point(node_list[s2.to].position,node_list[s2.from].position, x1_end, y1_end,x2_end, y2_end);
        }else{
            latlon_to_point(node_list[s2.to].position,getStreetSegmentCurvePoint(s2.curvePointCount-1, street_segment2), x1_end, y1_end,x2_end, y2_end);
        }
    
    //Segments are not connected
    }else{
        return TurnType::NONE;
    }
    
    //Use cross product to determine if turn type is left or right
    if(((x2_start-x1_start)*(y2_end-y1_end)-(y2_start-y1_start)*(x2_end-x1_end))<=0)
        return TurnType::RIGHT;
    else
        return TurnType::LEFT;
    
}


// Returns the time required to travel along the path specified, in seconds.
// The path is given as a vector of street segment ids, and this function can
// assume the vector either forms a legal path or has size == 0.  The travel
// time is the sum of the length/speed-limit of each street segment, plus the
// given right_turn_penalty and left_turn_penalty (in seconds) per turn implied
// by the path.  If the turn type is STRAIGHT, then there is no penalty
double compute_path_travel_time(const std::vector<unsigned>& path, 
                                const double right_turn_penalty, 
                                const double left_turn_penalty){
    
    double time = 0;
    
    //Ensures path is nonempty
    if(path.size()>0){
        
        //Goes through all path segments
        for(unsigned int i=1; i<path.size();i++){
            
            //Add segment travel time
            time += find_street_segment_travel_time(path[i-1]);
            
            //Add appropriate turn penalty
            if(find_turn_type(path[i-1],path[i])==TurnType::RIGHT){
                time +=right_turn_penalty;
            }else if(find_turn_type(path[i-1],path[i])==TurnType::LEFT){
                time += left_turn_penalty;
                
            }
        }
        
        //Adds last segment travel time (no turn penalty)
        time+=find_street_segment_travel_time(path[path.size()-1]);
    }
    
    //Return path travel time
    return time;
}

// Returns a path (route) between the start intersection and the end
// intersection, if one exists. This routine should return the shortest path
// between the given intersections, where the time penalties to turn right and
// left are given by right_turn_penalty and left_turn_penalty, respectively (in
// seconds).  If no path exists, this routine returns an empty (size == 0)
// vector.  If more than one path exists, the path with the shortest travel
// time is returned. The path is returned as a vector of street segment ids;
// traversing these street segments, in the returned order, would take one from
// the start to the end intersection.
std::vector<unsigned> find_path_between_intersections(
		  const unsigned intersect_id_start, 
                  const unsigned intersect_id_end,
                  const double right_turn_penalty, 
                  const double left_turn_penalty){
    
    std::vector<unsigned> path;
    //Structure to keep track of modified nodes to restore default values
    std::vector<Node *> modified_nodes;
    
    if(bfsPath(intersect_id_start, intersect_id_end,right_turn_penalty,left_turn_penalty, modified_nodes)){
        bfsTraceBack(intersect_id_end, path);
    }
    for(std::vector<Node *>::iterator i = modified_nodes.begin(); i != modified_nodes.end(); i++){
        (*i)->best_time=DBL_MAX;
        (*i)->reaching_edge=NO_EDGE;
       
    }
    return path;
}

//convert lon to x
double m3_lon_to_x(double lon, double lat1, double lat2){
    return lon*DEG_TO_RAD*cos((lat1+lat2)/2*DEG_TO_RAD);
}

//convert lat to y
double m3_lat_to_y(double lat){
    return lat*DEG_TO_RAD;
}


//convert latlon to x,y
void latlon_to_point(LatLon loc1, LatLon loc2, double &x1, double &y1, double  &x2, double &y2){
    x1=m3_lon_to_x(loc1.lon(),loc1.lat(),loc2.lat());
    y1=m3_lat_to_y(loc1.lat());
    x2=m3_lon_to_x(loc2.lon(),loc1.lat(),loc2.lat());
    y2=m3_lat_to_y(loc2.lat());
}

void bfsTraceBack(unsigned destID, std::vector<unsigned> &path){
    
    Node*curr_node = &node_list[destID];
    
    //Move through reaching edges from destination to source and store in path vector
    while(curr_node->reaching_edge!=NO_EDGE){
        
        path.push_back(curr_node->reaching_edge);
        
        //Determine which node is connected to current node in traceback
        if(unsigned(getInfoStreetSegment(path.back()).from)==curr_node->id){
            curr_node=&node_list[getInfoStreetSegment(path.back()).to];
        }else{
            curr_node=&node_list[getInfoStreetSegment(path.back()).from];
        }
        
    }
    
    //Reverse path to get from start to end
    std::reverse(path.begin(), path.end());
}

bool bfsPath(unsigned sourceID, unsigned destID, double right_turn_penalty, double left_turn_penalty, std::vector <Node *> &modified_nodes){
    
    //Create min heap priority queue of WaveElem and insert source node
    std::priority_queue<WaveElem, std::vector<WaveElem>, comp> wavefront;
    Node *source_node = &node_list[sourceID];
    source_node->reaching_edge=NO_EDGE;
    source_node->best_time=0;
    modified_nodes.push_back(source_node);
    Node * dest_node = &node_list[destID];
    wavefront.push(WaveElem(source_node,0,find_distance_between_two_points(source_node->position,dest_node->position)/max_speed));
    /*
    //Insert connected nodes from current node into wavefront
    for(int i=0; i<int(source_node->out_edge.size());i++){

        //Check to not go backwards over reaching edge (wasted insertion)
        if(source_node->reaching_edge!=int(source_node->out_edge[i])){

            //Get connected node and update distance if not computed
            Node *to_node=&node_list[source_node->inter[i]];
            //Determine total time spent to get to next node
            double time = source_node->best_time+source_node->time[i];

            if(time<to_node->best_time){
                to_node->best_time=time;
                to_node->reaching_edge=source_node->out_edge[i];
                modified_nodes.push_back(to_node);
                if(to_node->distance==DBL_MAX){
                    to_node->distance=find_distance_between_two_points(to_node->position,dest_node->position);
                }
                double weight = time+to_node->distance/25.0;
                wavefront.push(WaveElem(to_node, time,weight));
            }
        }
    }*/

    //Continue searching until destination is reached or wavefront is empty(not found)
    while(wavefront.size()>0){
        
        //Remove minimum element
        WaveElem wave = wavefront.top();
        wavefront.pop();
        Node *curr_node = wave.node;
        
        //Check if path has improved travel time and only re-expand if it is
        if(wave.travel_time== curr_node->best_time){

            
            //If destination found exit loop
            if(curr_node->id==destID)
                return true;
            
            //Insert connected nodes from current node into wavefront
            for(int i=0; i<int(curr_node->out_edge.size());i++){
                
                //Check to not go backwards over reaching edge (wasted insertion)
                if(curr_node->reaching_edge!=int(curr_node->out_edge[i])){
                    
                    //Get connected node and update distance if not computed
                    Node *to_node=&node_list[curr_node->inter[i]];
                    //Determine total time spent to get to next node
                    double time = curr_node->best_time+curr_node->time[i];
                    if(curr_node->reaching_edge!=NO_EDGE){
                        TurnType turn = find_turn_type(curr_node->reaching_edge,curr_node->out_edge[i]);
                        if(turn ==TurnType::RIGHT){
                            time +=right_turn_penalty;

                        }else if(turn ==TurnType::LEFT){
                            time +=left_turn_penalty;
                        }
                    }
                    if(time<to_node->best_time){
                        to_node->best_time=time;
                        to_node->reaching_edge=curr_node->out_edge[i];
                        modified_nodes.push_back(to_node);
                        
                        double weight = time+find_distance_between_two_points(to_node->position,dest_node->position)/max_speed;
                        wavefront.push(WaveElem(to_node, time,weight));
                    }
                }
            }
        }
    }
    
    //No path found
    return false;
}