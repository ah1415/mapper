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
 * m1.cpp file implemented by Austin Ho, Japtegh Singh, En-Mien Yang
 * This file implements high level functions to load a map and access properties of
 * different parts of the map such as street, intersection, and street segment properties.
 * This is done by implementing multiple data structures to efficiently access data
 * quickly and avoid redundant low level API calls that slows down the program.
 */

#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include <cmath>
#include <set>
#include <map>
#include "nodes.h"

//Create node structure
std::vector <Node> node_list;
double max_speed;
//Structure used for trie of street names
struct Name{
    
    //Container for street ids corresponding to node (prefix)
    std::vector<unsigned> street_ids;
	
    //Branches for next character
    std::map<char, Name*> names;
        
    //Destructor frees all memory
    ~Name(){
        //Delete any created branches
        for(std::map<char, Name*>::iterator i = names.begin(); i != names.end(); i++)
            delete i->second;
    }
};



//Intersection Struct Contains street segment IDs, street names, and connected intersections of specified intersection
struct Intersection{
    
    std::vector<unsigned> street_segment_ids;
    std::vector<std::string> street_names;
    std::vector<unsigned> connected_intersections;
};

//Structure containing street segment distance time values of specified street segment
struct StreetSegments{
    
    double distance;
    double time;
};

//Structure containing street segments, intersections, names, and lengths of specified street
struct Streets{

    std::vector <unsigned> street_segments;
    std::vector<unsigned> street_intersections;
    std::string street_name;
    double length = 0;
};

struct M1SuperClass{
    //Data structures for intersections, street segments, and street properties required for m1 functions
    std::vector<Intersection> intersection_properties;

    std::vector<StreetSegments> street_segments;

    std::vector<Streets> street_properties;

    //Pointer to beginning of street name trie
    Name *head;
};

M1SuperClass *g_m1_data;
void load_intersections_streets();
void load_street_segments();

//Loads a map streets.bin file. Returns true if successful and implements data structures required for functions,
//false if some error occurs and the map can't be loaded.
bool load_map(std::string map_path) {
    
    //Indicates whether the map has loaded successfully
    bool m_load_map_successful = false; 
    
    //Try to load the map
    m_load_map_successful =loadStreetsDatabaseBIN(map_path);
    
    
    //Return false if loading failed
    if(m_load_map_successful){
       
    
        g_m1_data=new M1SuperClass;
        //Resize data structures to appropriate size
        g_m1_data->intersection_properties.resize(getNumIntersections());
        g_m1_data->street_properties.resize(getNumStreets());
        g_m1_data->street_segments.resize(getNumStreetSegments());
        g_m1_data->head = new Name();

        //Build the street segments structure
        load_street_segments();

        //Build the intersections and streets structure
        load_intersections_streets();
    }
    
    bool m_load_osm_successful = false;
    std::string osm_path = map_path.substr(0, map_path.find(".")).append(".osm.bin");
    m_load_osm_successful=loadOSMDatabaseBIN(osm_path);
    //Return false if loading failed
    if(!m_load_osm_successful){
        if(m_load_map_successful){
            closeStreetDatabase();
            delete g_m1_data->head;
            delete g_m1_data;
            node_list.clear();
        }
        return m_load_osm_successful;
    }
    return m_load_map_successful;
    
}

//Close the map (if loaded)
void close_map() {
    
    //Delete empty the data structures implemented for m1
    delete g_m1_data->head;
    delete g_m1_data;
    node_list.clear();
    node_list.shrink_to_fit();
    closeOSMDatabase();
    
    //Close the database
    closeStreetDatabase();
}

//Returns the street segments for the given intersection 
std::vector<unsigned> find_intersection_street_segments(unsigned intersection_id) {
    
    return g_m1_data->intersection_properties[intersection_id].street_segment_ids;
    
}

//Returns the street names at the given intersection (includes duplicate street 
//names in returned vector)
std::vector<std::string> find_intersection_street_names(unsigned intersection_id){
    
    return g_m1_data->intersection_properties[intersection_id].street_names;
}

//Returns true if you can get from intersection1 to intersection2 using a single 
//street segment, accounting for one way streets as well
//corner case: an intersection is considered to be connected to itself
bool are_directly_connected(unsigned intersection_id1, unsigned intersection_id2){
    
    //Checks if intersections are the same
    if(intersection_id1 == intersection_id2){
        return true;
    }
    
    //Checks if intersection2 is it is validly connected to intersection1
    for(int i = 0; i < int(g_m1_data->intersection_properties[intersection_id1].connected_intersections.size()); i++){
        if(intersection_id2 == g_m1_data->intersection_properties[intersection_id1].connected_intersections[i]){
            return true;
        }
    }
    return false;
}

//Returns all intersections reachable by traveling down one street segment 
//from given intersection, accounting for one way streets as well
//the returned vector should NOT contain duplicate intersections
std::vector<unsigned> find_adjacent_intersections(unsigned intersection_id){
    
    return g_m1_data->intersection_properties[intersection_id].connected_intersections;
}

//Returns all street segments for the given street
std::vector<unsigned> find_street_street_segments(unsigned street_id){

    return g_m1_data->street_properties[street_id].street_segments;

}

//Returns all intersections along the a given street
std::vector<unsigned> find_all_street_intersections(unsigned street_id){

    return g_m1_data->street_properties[street_id].street_intersections;

}

//Return all intersection ids for two intersecting streets
//This function will typically return one intersection id.
std::vector<unsigned> find_intersection_ids_from_street_ids(unsigned street_id1, 
                                                            unsigned street_id2){
    
    //Create connected intersections container and checks if any intersections
    //in street1 are a part of street2 and stores it if true
    std::vector<unsigned> m_temp_Con_Int;
    
    for(int i = 0; i < int(g_m1_data->street_properties[street_id1].street_intersections.size()); i++){
        
        for(int j = 0; j < int(g_m1_data->street_properties[street_id2].street_intersections.size()); j++){
            
            if(g_m1_data->street_properties[street_id1].street_intersections[i]==g_m1_data->street_properties[street_id2].street_intersections[j]){
                m_temp_Con_Int.push_back(g_m1_data->street_properties[street_id1].street_intersections[i]);
            }
        }
    }
    
    return m_temp_Con_Int;

}

//Returns the distance between two coordinates in meters
double find_distance_between_two_points(LatLon point1, LatLon point2){
    double m_lat_avg = ((point1.lat()+point2.lat())/2.0 ) * DEG_TO_RAD;
    
    //converting lon and lat to x,y coordinates
    double m_x1 = point1.lon()* DEG_TO_RAD * cos(m_lat_avg);
    double m_y1 = point1.lat()* DEG_TO_RAD;
    
    double m_x2 = point2.lon()* DEG_TO_RAD * cos(m_lat_avg);
    double m_y2 = point2.lat()* DEG_TO_RAD;
    
    //Calculate and return the distance between the two points
    double m_distance = EARTH_RADIUS_IN_METERS * sqrt(pow((m_y2-m_y1),2.0) + pow((m_x2-m_x1),2.0));    
    
    return m_distance;
    
}

//Returns the length of the given street segment in meters
double find_street_segment_length(unsigned street_segment_id){
    
    return g_m1_data->street_segments[street_segment_id].distance;
    
}

//Returns the length of the specified street in meters
double find_street_length(unsigned street_id){
    
    return g_m1_data->street_properties[street_id].length;
}

//Returns the travel time to drive a street segment in seconds 
double find_street_segment_travel_time(unsigned street_segment_id){
    
    return g_m1_data->street_segments[street_segment_id].time;
}

//Returns the nearest point of interest to the given position
unsigned find_closest_point_of_interest(LatLon my_position){
    
    int m_index_of_closest_POI = 0;
    double m_min_distance = find_distance_between_two_points(getPointOfInterestPosition(0), my_position);

    //loop around all set of distances between every POI with my_position
    //find the index of the POI with the closest distance from my_position
    for(int i = 1; i < getNumPointsOfInterest(); i++){
        
        double m_distance = find_distance_between_two_points(getPointOfInterestPosition(i), my_position);
        if(m_min_distance > m_distance){
            m_index_of_closest_POI = i;
            m_min_distance = m_distance;
        }
    }
    
    return m_index_of_closest_POI;
}

//Returns the nearest intersection to the given position
unsigned find_closest_intersection(LatLon my_position){
    
    int m_index_of_closest_intersection = 0;
    double m_min_distance = find_distance_between_two_points(getIntersectionPosition(0), my_position);

    //loop around all set of distances between every intersection with my_position
    //find the index of the intersection with the closest distance from my_position
    for(int i = 1; i < getNumIntersections(); i++){
        
        double m_distance = find_distance_between_two_points(getIntersectionPosition(i), my_position);
        if(m_min_distance > m_distance){
            m_index_of_closest_intersection = i;
            m_min_distance = m_distance;
        }
    }
    
    return m_index_of_closest_intersection;
    
}

//Returns all street ids corresponding to street names that start with the given prefix
//The function is case-insensitive to the street prefix. 
//If no street names match the given prefix or the prefix is empty, this routine
//returns an empty (length 0) vector.
std::vector<unsigned> find_street_ids_from_partial_street_name(std::string street_prefix){
    
    //Returns empty vector if prefix is empty
    std::vector<unsigned> m_empty;
    if(street_prefix.empty()){
        return m_empty;
    }
    
    //Move through the names trie structure according to the street_prefix's characters
    //Returns an empty vector if no names contain the prefix
    Name* current = g_m1_data->head;
    for(int i=0; i<int(street_prefix.length()); i++){
        char m_character = tolower(street_prefix[i]);
        if(current->names.count(m_character)){
            current = current->names[m_character];
        }else{
            return m_empty;
        }
    }
    return current->street_ids;
    
}

void load_intersections_streets(){
    //Temporary data structures using sets to store only unique values
    std::vector<std::set<unsigned> > m_temp_Int_ID(getNumStreets());
    std::vector<std::set<unsigned> > m_temp_Str_Seg_ID(getNumStreets());
    node_list.resize(getNumIntersections());
    //Determines values to insert into intersection properties data structure and inserts if applicable
    for(int i = 0; i < getNumIntersections(); i++){
        
        //Resize temporary data structures to store street segment ids and street names
        g_m1_data->intersection_properties[i].street_segment_ids.resize(getIntersectionStreetSegmentCount(i));
        g_m1_data->intersection_properties[i].street_names.resize(getIntersectionStreetSegmentCount(i));
        
        //Create temporary set container to store unique connected intersections
        std::set<unsigned> m_temp_Con_Int;
        
        //Create node structure and update id
        
        node_list[i].id=i;
        node_list[i].position=getIntersectionPosition(i);
        //Loop through all street segments at intersection
        for(int s = 0; s < getIntersectionStreetSegmentCount(i); s++){
            
            //Store segment id and street name to avoid repetitive function calls
            unsigned m_str_Seg_ID = getIntersectionStreetSegment(s, i);
            unsigned m_str_ID = getInfoStreetSegment(m_str_Seg_ID).streetID;
            
            //Insert street segment and street name for intersection properties
            g_m1_data->intersection_properties[i].street_segment_ids[s] = m_str_Seg_ID;
            g_m1_data->intersection_properties[i].street_names[s] = getStreetName(m_str_ID);
            
            //Temporarily store intersection id and street segment id for street properties
            m_temp_Int_ID[m_str_ID].insert(i);
            m_temp_Str_Seg_ID[m_str_ID].insert(m_str_Seg_ID);
            
            //Store street name for street properties
            g_m1_data->street_properties[m_str_ID].street_name = g_m1_data->intersection_properties[i].street_names[s];
            
           
            //Stores unique intersection id if you come from the current intersection or to the current intersection on a two way street and update node with segment, intersection, and time
            if(getInfoStreetSegment(m_str_Seg_ID).from == i){
                m_temp_Con_Int.insert(getInfoStreetSegment(m_str_Seg_ID).to);
                node_list[i].inter.push_back(getInfoStreetSegment(m_str_Seg_ID).to);
                node_list[i].out_edge.push_back(m_str_Seg_ID);
                node_list[i].time.push_back(find_street_segment_travel_time(m_str_Seg_ID));
            }
            else if(!getInfoStreetSegment(m_str_Seg_ID).oneWay&&getInfoStreetSegment(m_str_Seg_ID).to == i){
                m_temp_Con_Int.insert(getInfoStreetSegment(m_str_Seg_ID).from);
                node_list[i].inter.push_back(getInfoStreetSegment(m_str_Seg_ID).from);
                node_list[i].out_edge.push_back(m_str_Seg_ID);
                node_list[i].time.push_back(find_street_segment_travel_time(m_str_Seg_ID));
            }
            
        }
        
        //Insert connected intersections into intersection properties
        g_m1_data->intersection_properties[i].connected_intersections.assign(m_temp_Con_Int.begin(),m_temp_Con_Int.end());
    }
    
    //Move through all streets and inserts them into names trie structure and
    //inserts temporary data structure contents into street properties structure
    for(int i=0; i< int(g_m1_data->street_properties.size()); i++){
        
        //Insert temporary structure contents into street properties structure
        g_m1_data->street_properties[i].street_segments.assign(m_temp_Str_Seg_ID[i].begin(),m_temp_Str_Seg_ID[i].end());
        g_m1_data->street_properties[i].street_intersections.assign(m_temp_Int_ID[i].begin(),m_temp_Int_ID[i].end());
        
        //Move through characters of street name to insert into trie
        Name* m_current= g_m1_data->head;
	for(int j = 0; j < int(g_m1_data->street_properties[i].street_name.length()); j++){
            char c = tolower(g_m1_data->street_properties[i].street_name[j]);
            
            //Create new branch if branch doesn't exist
            if(!m_current->names.count(c)){
		Name *tempConInt = new Name();
		m_current->names.insert({c,tempConInt});
            }
            
            //Move to corresponding branch and store street id
            m_current = m_current->names[c];
            m_current->street_ids.push_back(i);
	}
    }
}

void load_street_segments(){
    
    max_speed=0;
    //Move through all street segments to calculate distance time properties
    for(int s=0; s< getNumStreetSegments(); s++){
        max_speed=std::max(max_speed,getInfoStreetSegment(s).speedLimit/3.6);
        
        //Sum street segment distances of curve point segments
        double m_length = 0;
    
        LatLon m_start = getIntersectionPosition(getInfoStreetSegment(s).from);
        LatLon m_end;

        for(int i = 0; i < getInfoStreetSegment(s).curvePointCount; i++){
            m_end = getStreetSegmentCurvePoint(i, s);
            m_length += find_distance_between_two_points(m_start,m_end);
            m_start = m_end;
        }

        m_end = getIntersectionPosition(getInfoStreetSegment(s).to);
        m_length += find_distance_between_two_points(m_start,m_end);
        
        //Store length and time into street segments structure and add length to street length
        g_m1_data->street_segments[s].distance = m_length;
        g_m1_data->street_segments[s].time = g_m1_data->street_segments[s].distance/getInfoStreetSegment(s).speedLimit*3.6;
        g_m1_data->street_properties[getInfoStreetSegment(s).streetID].length += m_length;
    }

}
