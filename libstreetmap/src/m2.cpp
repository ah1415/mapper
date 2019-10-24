/*
 * m2.cpp file implemented by Austin Ho, Japtegh Singh, En-Mien Yang
 * This file implements functions to draw a map. It draws
 * different parts of the map such as street, intersection, and street segment properties.
 * This is done by implementing multiple data structures to efficiently access data
 * quickly and avoid redundant low level API calls that slows down the program.
 */

#include <ezgl/application.hpp>
#include <ezgl/graphics.hpp>
#include "ezgl/callback.hpp"
#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include <cmath>
#include <set>
#include <map>
#include <iostream>
#include <vector>
#include <thread>
#include <algorithm>

#define INTERSECTION_DOT_RADIUS 0.000001
#define TEXT_POSTITION_OFFSET 0.000002

//intersection struct that contains the name and the position to be drawn
struct IntersectionData {
    LatLon position;
    std::string name;
};

//street segment struct with x, y, osmid, highway type, rotation angle, name, and length
struct SegmentData{
    std::vector<double> x;
    std::vector<double> y;
    unsigned int osmid;
    std::string highway;
    std::vector<double> angle;
    bool one_way;
    std::string name;
    std::vector<double> length;
};

//OSM struct with OSMNodes, OSMWays, and OSMRelations
struct OSMData{
    std::map<OSMID, unsigned int> OSMNodes;
    std::map<OSMID, unsigned int> OSMWays;
    std::map<OSMID, unsigned int> OSMRelations;
};

//bool struct the point of interests to be drawn onto the map
struct POIBools{
    bool hospital;
    bool cafe;
    bool restaurant;
    bool bank;
    bool gas;
    bool station;
    bool POI;
};

//feature struct that contains the type, names, location, and area
struct FeatureData{
    FeatureType feature_type;
    std::string feature_name;
    std::vector<ezgl::point2d> points;
    double area;
    bool closed;
};

//struct that contain point of interests' name, type, and location
struct POIData{
    std::string name;
    std::string type;
    ezgl::point2d location = {0,0};
};

//superclass that contains globally used var
struct M2_SuperClass{

    //map related
    double latMin, latMax, lonMin, lonMax;
    int zoom_level;
    
    //modes
    bool night_mode_bool = false;
    bool navigation_mode_bool = false;

    //for clinking intersections
    bool clicked_on_an_Intersection_bool = false;
    bool clicked_on_second_Intersection_bool = false;
    ezgl::point2d clicked_intersection = {0.0,0.0};
    ezgl::point2d clicked_intersection_2 = {0.0,0.0};
    
    //for finding intersections
    bool found_first_Intersection_bool = false;
    bool found_second_Intersection_bool = false;
    ezgl::point2d found_intersection = {0.0,0.0};
    ezgl::point2d found_intersection_2 = {0.0,0.0};

    ezgl::point2d intersection_location = {0.0, 0.0};

    
    bool draw_found_intersection_bool = false;

    std::vector<unsigned> intersection_ids_1;
    std::vector<unsigned> intersection_ids_2;

    bool intersections_found_bool = false;

    int street1_id = -1;
    int street2_id = -1;
    int street3_id = -1;
    int street4_id = -1;
    
    
    
    std::string g_response;

    int count = -1;
    std::vector<unsigned> street_ids_found;
    std::string confirmation = "s";

    ezgl::rectangle initial_world = {{0, 0}, {0, 0}};
    OSMData OSM_data;
    std::vector<FeatureData> features;
    std::vector<POIData> POIs;

    POIBools POIChecks;
    std::vector<SegmentData> segments;
    std::vector<IntersectionData> intersections;

    std::vector<std::vector<ezgl::point2d> > subways;
    std::string directions;
    int img_count;
};

M2_SuperClass *g_m2_data;

//functions that will be used
void act_on_mouse_press(ezgl::application *application, GdkEventButton *event, double x, double y);
void act_on_mouse_move(ezgl::application *application, GdkEventButton *event, double x, double y);
void act_on_key_press(ezgl::application *application, GdkEventKey *event, char *key_name);
void initial_setup(ezgl::application *application);

void press_night(GtkWidget *, gpointer data);
void press_hospital(GtkWidget *, gpointer data);
void press_cafe(GtkWidget *, gpointer data);
void press_restaurant(GtkWidget *, gpointer data);
void press_bank(GtkWidget *, gpointer data);
void press_gas(GtkWidget *, gpointer data);
void press_POI(GtkWidget *, gpointer data);
void zoom_level(ezgl::renderer &g);
void press_no(GtkWidget *, gpointer data);
void press_yes(GtkWidget *, gpointer data);
void press_search_only(GtkWidget *, gpointer data);
void press_navigate(GtkWidget *, gpointer data);
void press_show(GtkWidget *, gpointer data);
void press_station(GtkWidget *, gpointer data);
void press_change(GtkWidget *, gpointer data);
void press_find(GtkWidget *, gpointer data);
void draw_found_intersection(ezgl::renderer &g);
void press_reset(GtkWidget *, gpointer data);
void press_info(GtkWidget *, gpointer data);
void press_directions(GtkWidget *, gpointer data);
void press_refresh(GtkWidget *, gpointer data);

void update_map();
void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data);
void on_help_response(GtkDialog *dialog, gint response_id, gpointer user_data);
void latlon_to_point(LatLon loc, double &x, double &y);
void draw_clicked_intersection(ezgl::renderer &g);
void find_intersections (ezgl::application * application);
double dynamic_width_large_roads();
double dynamic_width_small_roads();
double lon_to_x(double lon);
double lat_to_y(double lat);
double x_to_lon(double x);
double y_to_lat(double y);

void textbox1_changed (GtkEditable *editable, gpointer data);
void textbox2_changed (GtkEditable *editable, gpointer data);
void textbox3_changed (GtkEditable *editable, gpointer data);
void textbox4_changed (GtkEditable *editable, gpointer data);

void press_street_suggestions(GtkWidget *, gpointer data);

void initialize_world();
void load_POI_data();
void load_OSM_data();
void load_segments_data();
void load_features_data();

void press_clear(GtkWidget *, gpointer data);
void draw_path_between_intersections(ezgl::renderer &g, std::vector<unsigned> path);

void draw_feature(int check, ezgl::renderer &g);
void draw_street_segments(int check, ezgl::renderer &g);
void label_street_names(int check, ezgl::renderer &g);
void draw_highway_segments(ezgl::renderer &g);
void label_highway_names(ezgl::renderer &g);
void draw_subway_line(int check, ezgl::renderer &g);
void draw_POIs(int check, ezgl::renderer &g);


//Draw to the main canvas using the provided graphics object.
//The graphics object expects that x and y values will be in the main canvas' world coordinate system.
void draw_main_canvas(ezgl::renderer &g);

//This function initializes an ezgl application and runs it.
void draw_map(){
    //Build the mapping data structures
    update_map();

    ezgl::application::settings settings;

    // Path to the "main.ui" file that contains an XML description of the UI.
    settings.main_ui_resource = "libstreetmap/resources/main.ui";

    // Note: the "main.ui" file has a GtkWindow called "MainWindow".
    settings.window_identifier = "MainWindow";

    // Note: the "main.ui" file has a GtkDrawingArea called "MainCanvas".
    settings.canvas_identifier = "MainCanvas";

    // Create our EZGL application.
    ezgl::application application(settings);
    
    application.add_canvas("MainCanvas", draw_main_canvas, g_m2_data->initial_world);
    
    application.run(initial_setup, act_on_mouse_press, act_on_mouse_move, act_on_key_press);
    
    //Free memory
    delete g_m2_data;
}

void draw_main_canvas(ezgl::renderer &g){
    
    //get zoom_level whenever main canvas is drawn
    zoom_level(g);

    //Set two sets of color: one for regular mode, one for night mode
    ezgl::color BACKGROUND(0,0,0);
    ezgl::color WATER(0,0,0);
    ezgl::color BUILDING(0,0,0);
    ezgl::color SAND(0,0,0);
    ezgl::color GRASS(0,0,0);
    ezgl::color STREET(0,0,0);
    ezgl::color HIGHWAY(0,0,0);
    ezgl::color HIGHWAYBORDER(0,0,0);
    ezgl::color STREETBORDER(0, 0, 0);

    //night mode color palette
    if(g_m2_data->night_mode_bool){
        BACKGROUND=ezgl::N_BACKGROUND;
        WATER=ezgl::N_WATER;
        BUILDING=ezgl::N_BUILDING;
        SAND=ezgl::N_SAND;
        GRASS=ezgl::N_GRASS;
        STREET=ezgl::N_STREET;
        HIGHWAY=ezgl::N_HIGHWAY;
        HIGHWAYBORDER=ezgl::N_HIGHWAYBORDER;
        STREETBORDER=ezgl::N_STREETBORDER;
    }else{
        //regular mode color palette
        BACKGROUND=ezgl::D_BACKGROUND;
        WATER=ezgl::D_WATER;
        BUILDING=ezgl::D_BUILDING;
        SAND=ezgl::D_SAND;
        GRASS=ezgl::D_GRASS;
        STREET=ezgl::D_STREET;
        HIGHWAY=ezgl::D_HIGHWAY;
        HIGHWAYBORDER=ezgl::D_HIGHWAYBORDER;
        STREETBORDER=ezgl::D_STREETBORDER;
    }
    
    g.set_color(BACKGROUND);
    g.fill_rectangle(g.get_visible_world());
    
    //use zoom level to determine 3 checkpoints
    //checkpoints (check) are used to determine what features are to be drawn
    int check=0;
    if(g_m2_data->zoom_level >= 8){
        check=3;
    }else if(g_m2_data->zoom_level>=6){
        check=2;
    }
    else if(g_m2_data->zoom_level >=3){
        check=1;
    }
    
    //using smaller functions to draw the entire map
    draw_feature(check, g);
    draw_street_segments(check, g);
    label_street_names(check, g);
    draw_highway_segments(g);
    label_highway_names(g);
    draw_subway_line(check, g);
    draw_POIs(check, g);
    
    //Draw ONE intersection based on click location.
    //this happens ONLY IN SEARCH MODE
    if(g_m2_data->clicked_on_an_Intersection_bool == true){
        draw_clicked_intersection(g);
    }

    //Draw PATH between TWO clicked intersections. 
    //this happens ONLY IN NAVIGATION MODE
    if(g_m2_data->navigation_mode_bool == true && g_m2_data->clicked_on_second_Intersection_bool == true){
        
        LatLon clicked_intersection_position(y_to_lat(g_m2_data->clicked_intersection.y),x_to_lon(g_m2_data->clicked_intersection.x));
        LatLon clicked_intersection_position_2(y_to_lat(g_m2_data->clicked_intersection_2.y),x_to_lon(g_m2_data->clicked_intersection_2.x));
        int int_clicked_id_1 = find_closest_intersection(clicked_intersection_position);
        int int_clicked_id_2 = find_closest_intersection(clicked_intersection_position_2);
        
        std::vector<unsigned> path = find_path_between_intersections(int_clicked_id_1, int_clicked_id_2, 10, 20);
        draw_path_between_intersections(g, path);
        
        g_m2_data->found_intersection = {0.0,0.0};
        g_m2_data->found_intersection_2 = {0.0,0.0};    
        g_m2_data->found_first_Intersection_bool = false;
        g_m2_data->found_second_Intersection_bool = false;

        g_m2_data->found_first_Intersection_bool = false;
        g_m2_data->found_second_Intersection_bool = false;
        g_m2_data->found_intersection = {0.0,0.0};
        g_m2_data->found_intersection_2 = {0.0,0.0};
        
        g_m2_data->draw_found_intersection_bool = false;
    }
    
    
    //Draw ONE intersection based on find. 
    //this happens ONLY IN SEARCH MODE
    if(g_m2_data->draw_found_intersection_bool == true){
        draw_found_intersection(g);
    }
    
    //Draw PATH between TWO found intersections. 
    //this happens ONLY IN NAVIGATION MODE
    if(g_m2_data->navigation_mode_bool == true && g_m2_data->found_second_Intersection_bool == true){
        LatLon found_intersection_position(y_to_lat(g_m2_data->found_intersection.y),x_to_lon(g_m2_data->found_intersection.x));
        LatLon found_intersection_position_2(y_to_lat(g_m2_data->found_intersection_2.y),x_to_lon(g_m2_data->found_intersection_2.x));
        int int_found_id_1 = find_closest_intersection(found_intersection_position);
        int int_found_id_2 = find_closest_intersection(found_intersection_position_2);
        
        std::vector<unsigned> path = find_path_between_intersections(int_found_id_1, int_found_id_2, 10, 20);
        g.set_color(ezgl::YELLOW);
        g.fill_arc({g_m2_data->found_intersection_2.x,g_m2_data->found_intersection_2.y}, INTERSECTION_DOT_RADIUS, 0, 360);
        draw_path_between_intersections(g, path);
        
        if(g_m2_data->night_mode_bool){
                g.set_color(ezgl::WHITE);
            }else{
                g.set_color(ezgl::BLACK);
            }
        g.draw_text({g_m2_data->found_intersection_2.x,g_m2_data->found_intersection_2.y + TEXT_POSTITION_OFFSET}, g_m2_data->intersections[find_closest_intersection(found_intersection_position_2)].name, 100, 200);
        
    }
}


//Function called before the activation of the application
//Can be used to create additional buttons, initialize the status message,
//or connect added widgets to their callback functions
void initial_setup(ezgl::application *application)
{
  // Update the status bar message
  application->update_message("Map To Go Application");
  
  //Import CSS styling
  GtkCssProvider *cssProvider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(cssProvider, "libstreetmap/resources/styling.css", NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cssProvider),GTK_STYLE_PROVIDER_PRIORITY_USER);
  
  //Connect functions with gtk object signals
  GObject *night_button = application->get_object("NightButton");
  g_signal_connect(night_button, "clicked", G_CALLBACK(press_night), application);
  
  GObject *hospital_check = application->get_object("HospitalCheck");
  g_signal_connect(hospital_check, "toggled", G_CALLBACK(press_hospital), application);
  
  GObject *cafe_check = application->get_object("CafeCheck");
  g_signal_connect(cafe_check, "toggled", G_CALLBACK(press_cafe), application);
  
  GObject *restaurant_check = application->get_object("RestaurantCheck");
  g_signal_connect(restaurant_check, "toggled", G_CALLBACK(press_restaurant), application);
  
  GObject *bank_check = application->get_object("BankCheck");
  g_signal_connect(bank_check, "toggled", G_CALLBACK(press_bank), application);
  
  GObject *gas_check = application->get_object("GasCheck");
  g_signal_connect(gas_check, "toggled", G_CALLBACK(press_gas), application);
  
  GObject *station_check = application->get_object("StationCheck");
  g_signal_connect(station_check, "toggled", G_CALLBACK(press_station), application);
  
  GObject *POI_check = application->get_object("POICheck");
  g_signal_connect(POI_check, "toggled", G_CALLBACK(press_POI), application);
  
  GObject *find_button = application->get_object("FindButton");
  g_signal_connect(find_button, "clicked", G_CALLBACK(press_find), application);
  
  GObject *yes_button = application->get_object("YesButton");
  g_signal_connect(yes_button, "clicked", G_CALLBACK(press_yes), application);
  
  GObject *no_button = application->get_object("NoButton");
  g_signal_connect(no_button, "clicked", G_CALLBACK(press_no), application);

  GObject *change_map = application->get_object("MapButton");
  g_signal_connect(change_map,"clicked",G_CALLBACK(press_change), application);

  GObject *reset_button = application->get_object("ResetButton");
  g_signal_connect(reset_button, "clicked", G_CALLBACK(press_reset), application);
  
  GObject *search_button = application->get_object("SearchButton");
  g_signal_connect(search_button, "toggled", G_CALLBACK(press_search_only), application);
  
  GObject *navigate_button = application->get_object("NavigateButton");
  g_signal_connect(navigate_button, "toggled", G_CALLBACK(press_navigate), application);
  
  GObject *clear_clicked_intersections = application->get_object("ClearButton");
  g_signal_connect(clear_clicked_intersections, "clicked", G_CALLBACK(press_clear), application);
  
  GObject *menu_rev = application->get_object("SearchButton");
  GtkToggleButton *menu_rev_p=(GtkToggleButton *)menu_rev;
  
  GObject *object_find_text = application->get_object("StreetSearch_1");
  g_signal_connect (object_find_text, "changed", G_CALLBACK (textbox1_changed), application);
  
  GObject *object_find_text_2 = application->get_object("StreetSearch_2");
  g_signal_connect (object_find_text_2, "changed", G_CALLBACK (textbox2_changed), application);
  
  GObject *object_find_text_3 = application->get_object("StreetSearch_3");
  g_signal_connect (object_find_text_3, "changed", G_CALLBACK (textbox3_changed), application);
  
  GObject *object_find_text_4 = application->get_object("StreetSearch_4");
  g_signal_connect (object_find_text_4, "changed", G_CALLBACK (textbox4_changed), application);
  
  GObject *street1_button = application->get_object("Street_search_1");
  g_signal_connect(street1_button, "clicked", G_CALLBACK(press_street_suggestions), application);
  
  GObject *info_button = application->get_object("HelpButton");
  g_signal_connect(info_button, "clicked", G_CALLBACK(press_info), application);
  GObject *dir_button = application->get_object("DirectionsButton");
  g_signal_connect(dir_button, "clicked", G_CALLBACK(press_directions), application);
  GObject *ref_button = application->get_object("RefreshButton");
  g_signal_connect(ref_button, "clicked", G_CALLBACK(press_refresh), application);
  
  gtk_toggle_button_set_active(menu_rev_p, true);
}


//Function to handle mouse press event to find closest intersection
void act_on_mouse_press(ezgl::application *application, GdkEventButton *, double x, double y)
{
    //get the LatLon set from the x,y pressed by mouse
    LatLon mouse_LatLon(y_to_lat(y), x_to_lon(x));

    //When it is not in navigation mode,
    //the program should only allow one click
    if(g_m2_data->navigation_mode_bool == false){
        
            g_m2_data->clicked_intersection.x = lon_to_x(getIntersectionPosition(find_closest_intersection(mouse_LatLon)).lon());
            g_m2_data->clicked_intersection.y = lat_to_y(getIntersectionPosition(find_closest_intersection(mouse_LatLon)).lat());
            g_m2_data->clicked_on_an_Intersection_bool = true;
    
    //When it is in navigation mode,
    //the program will allow users to click two intersections
    }else if(g_m2_data->navigation_mode_bool == true){
        
        if(g_m2_data->clicked_on_an_Intersection_bool == false){
            g_m2_data->clicked_intersection.x = lon_to_x(getIntersectionPosition(find_closest_intersection(mouse_LatLon)).lon());
            g_m2_data->clicked_intersection.y = lat_to_y(getIntersectionPosition(find_closest_intersection(mouse_LatLon)).lat());
           
            g_m2_data->clicked_on_an_Intersection_bool = true;
        }else if(g_m2_data->clicked_on_an_Intersection_bool == true){
            g_m2_data->clicked_intersection_2.x = lon_to_x(getIntersectionPosition(find_closest_intersection(mouse_LatLon)).lon());
            g_m2_data->clicked_intersection_2.y = lat_to_y(getIntersectionPosition(find_closest_intersection(mouse_LatLon)).lat());
            g_m2_data->clicked_on_second_Intersection_bool = true;
        }
    }
    
    application ->refresh_drawing();
}



// Function to handle mouse move event for searched intersection
void act_on_mouse_move(ezgl::application *, GdkEventButton *, double , double )
{
    if (g_m2_data->intersections_found_bool == true)  g_m2_data->draw_found_intersection_bool = true;
    if (g_m2_data->intersections_found_bool == false)  g_m2_data->draw_found_intersection_bool = false;
    
}


//Function to handle keyboard press event
void act_on_key_press(ezgl::application *, GdkEventKey *, char *)
{

}

//convert lon to x
double lon_to_x(double lon){
    return lon*DEG_TO_RAD*cos((g_m2_data->latMin+g_m2_data->latMax)/2*DEG_TO_RAD);
}

//convert lat to y
double lat_to_y(double lat){
    return lat*DEG_TO_RAD;
}

//convert x to lon
double x_to_lon(double x){
    return x / (DEG_TO_RAD*cos((g_m2_data->latMin+g_m2_data->latMax)/2*DEG_TO_RAD));
}

//convert y to lat
double y_to_lat(double y){
    return y / DEG_TO_RAD;
}

//convert latlon to x,y
void latlon_to_point(LatLon loc, double &x, double &y){
    x=lon_to_x(loc.lon());
    y=lat_to_y(loc.lat());
}

//function for night mode. Called when GUI switch is pressed
//redraw the map based on night mode colors
void press_night(GtkWidget *, gpointer data)
{
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *night_sw = application->get_object("NightSwitch");
    GtkSwitch * night_sw_ptr=(GtkSwitch *)night_sw;
    gtk_switch_set_active(night_sw_ptr, !gtk_switch_get_active(night_sw_ptr));
    
    g_m2_data->night_mode_bool = gtk_switch_get_active(night_sw_ptr);
  
    application->refresh_drawing();
}

//function for displaying hospitals. Called when hospital toggle is clicked
//draw the hospital icon on the map according to OSM data 
void press_hospital(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *menu_rev = application->get_object("HospitalCheck");
    GtkToggleButton * menu_rev_p=(GtkToggleButton *)menu_rev;
    g_m2_data->POIChecks.hospital=gtk_toggle_button_get_active(menu_rev_p);
    application->refresh_drawing();
}

//function for displaying cafe. Called when cafe toggle is clicked
//draw the cafe icon on the map according to OSM data 
void press_cafe(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *menu_rev = application->get_object("CafeCheck");
    GtkToggleButton * menu_rev_p=(GtkToggleButton *)menu_rev;
    g_m2_data->POIChecks.cafe=gtk_toggle_button_get_active(menu_rev_p);
    application->refresh_drawing();
}

//function for displaying restaurant. Called when restaurant toggle is clicked
//draw the restaurant icon on the map according to OSM data 
void press_restaurant(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);

    GObject *menu_rev = application->get_object("RestaurantCheck");
    GtkToggleButton * menu_rev_p=(GtkToggleButton *)menu_rev;
    g_m2_data->POIChecks.restaurant=gtk_toggle_button_get_active(menu_rev_p);
    application->refresh_drawing();
}

//function for displaying banks. Called when banks toggle is clicked
//draw the banks icon on the map according to OSM data 
void press_bank(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *menu_rev = application->get_object("BankCheck");
    GtkToggleButton * menu_rev_p=(GtkToggleButton *)menu_rev;
    g_m2_data->POIChecks.bank=gtk_toggle_button_get_active(menu_rev_p);
    application->refresh_drawing();
}

//function for displaying gas station. Called when station toggle is clicked
//draw the gas icon on the map according to OSM data 
void press_gas(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *menu_rev = application->get_object("GasCheck");
    GtkToggleButton * menu_rev_p=(GtkToggleButton *)menu_rev;
    g_m2_data->POIChecks.gas=gtk_toggle_button_get_active(menu_rev_p);
    application->refresh_drawing();
}

//function for displaying station. Called when station toggle is clicked
//draw the station icon on the map according to OSM data 
void press_station(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *menu_rev = application->get_object("StationCheck");
    GtkToggleButton * menu_rev_p=(GtkToggleButton *)menu_rev;
    g_m2_data->POIChecks.station=gtk_toggle_button_get_active(menu_rev_p);
    application->refresh_drawing();
}

//function for displaying all the point of interests. Called when POI toggle is clicked
//draw the POIs with a red dot icon on the map according to OSM data 
void press_POI(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *menu_rev = application->get_object("POICheck");
    GtkToggleButton * menu_rev_p=(GtkToggleButton *)menu_rev;
    g_m2_data->POIChecks.POI=gtk_toggle_button_get_active(menu_rev_p);
    application->refresh_drawing();
}

//function for taking in inputs from the find text box and bar
//this calls helper functions to perform street search and plot intersections on the map
void press_find(GtkWidget *widget, gpointer data){
    
        press_clear(widget, data);
        
        auto application = static_cast<ezgl::application *>(data);
        
        
        GObject *object_find_text = application->get_object("StreetSearch_1");
        GtkEditable *editable_find_1 = (GtkEditable*) object_find_text; 
        
        object_find_text = application->get_object("StreetSearch_2");
        GtkEditable *editable_find_2 = (GtkEditable*) object_find_text; 
        
        object_find_text = application->get_object("StreetSearch_3");
        GtkEditable *editable_find_3 = (GtkEditable*) object_find_text; 
        
        object_find_text = application->get_object("StreetSearch_4");
        GtkEditable *editable_find_4 = (GtkEditable*) object_find_text; 
        
        GObject *menu_rev_search = application->get_object("SearchButton");
        GtkToggleButton *menu_rev_p_search=(GtkToggleButton *)menu_rev_search;

        std::string response_1 = gtk_editable_get_chars(editable_find_1, 0, -1);
        std::string response_2 = gtk_editable_get_chars(editable_find_2, 0, -1);
        std::string response_3 = "";
        std::string response_4 = "";

        // will take in input for response 3 and 4 only when in navigate mode
        if (gtk_toggle_button_get_active(menu_rev_p_search) == false){
            response_3 = gtk_editable_get_chars(editable_find_3, 0, -1);
            response_4 = gtk_editable_get_chars(editable_find_4, 0, -1);
        }
        
        
        gint start = 0;
        
        // if there is some reponse in textbox 1
        if (response_1 != ""){
            g_m2_data->street_ids_found = find_street_ids_from_partial_street_name(response_1);
            
            // if streets are found with that name
            if (g_m2_data->street_ids_found.size() != 0){

                if (true){
                    application->update_message("Search found! View terminal for results.");
                    application->refresh_drawing();
                }

                application->refresh_drawing();

                g_m2_data->confirmation = "n";
                g_m2_data->count = 0;
                
                std::string street_name = getStreetName(g_m2_data->street_ids_found[g_m2_data->count]);
                
                g_m2_data->street1_id = g_m2_data->street_ids_found[0];
                
                gtk_editable_delete_text(editable_find_1, 0, -1);
                gtk_editable_insert_text (editable_find_1, street_name.c_str(), -1, &start);
                

               
            } else {    // if streets are not found
                application->update_message("Street 1 not found! Try again: ");
                gtk_editable_delete_text(editable_find_1, 0, -1);
            }
        } else {
            application->update_message("Enter street names to start searching!");
        }
        
        // the process repeats for other texts as well
        if (response_2 != ""){
            g_m2_data->street_ids_found = find_street_ids_from_partial_street_name(response_2);
        
            if (g_m2_data->street_ids_found.size() != 0){

                if (true){
                    application->update_message("Search found! View terminal for results.");
                    application->refresh_drawing();
                }

                application->refresh_drawing();

                g_m2_data->confirmation = "n";
                g_m2_data->count = 0;
                
                std::string street_name = getStreetName(g_m2_data->street_ids_found[g_m2_data->count]);
                
                g_m2_data->street2_id = g_m2_data->street_ids_found[0];
                
                gtk_editable_delete_text(editable_find_2, 0, -1);
                gtk_editable_insert_text (editable_find_2, street_name.c_str(), -1, &start);
                
            } else {
                application->update_message("Street 2 not found! Try again: ");
                gtk_editable_delete_text(editable_find_2, 0, -1);
            }
        } else {
            application->update_message("Enter street names to start searching!");
        }
        
        if (response_3 != ""){
            g_m2_data->street_ids_found = find_street_ids_from_partial_street_name(response_3);
        
            if (g_m2_data->street_ids_found.size() != 0){

                if (true){
                    application->update_message("Search found! View terminal for results.");
                    application->refresh_drawing();
                }

                application->refresh_drawing();

                g_m2_data->confirmation = "n";
                g_m2_data->count = 0;
                
                std::string street_name = getStreetName(g_m2_data->street_ids_found[g_m2_data->count]);
                
                g_m2_data->street3_id = g_m2_data->street_ids_found[0];
                
                gtk_editable_delete_text(editable_find_3, 0, -1);
                gtk_editable_insert_text (editable_find_3, street_name.c_str(), -1, &start);
                

               
            } else {
                application->update_message("Street 3 not found! Try again: ");
                gtk_editable_delete_text(editable_find_3, 0, -1);
            }
        } else {
            application->update_message("Enter street names to start searching!");
        }
        
        if (response_4 != ""){
            g_m2_data->street_ids_found = find_street_ids_from_partial_street_name(response_4);
        
            if (g_m2_data->street_ids_found.size() != 0){

                if (true){
                    application->update_message("Search found! View terminal for results.");
                    application->refresh_drawing();
                }

                application->refresh_drawing();

                g_m2_data->confirmation = "n";
                g_m2_data->count = 0;
                
                std::string street_name = getStreetName(g_m2_data->street_ids_found[g_m2_data->count]);
                
                g_m2_data->street4_id = g_m2_data->street_ids_found[0];
                
                gtk_editable_delete_text(editable_find_4, 0, -1);
                gtk_editable_insert_text (editable_find_4, street_name.c_str(), -1, &start);
                

               
            } else {
                application->update_message("Street 4 not found! Try again: ");
                gtk_editable_delete_text(editable_find_1, 0, -1);
            }
        } else {
            application->update_message("Enter street names to start searching!");
        }
        
        // when there are inputs for every street textbox
        if ((g_m2_data->street1_id != -1) && (g_m2_data->street2_id != -1) && (g_m2_data->street3_id != -1) && (g_m2_data->street4_id != -1)){
            find_intersections(application);
            
            // if no intersections are found
            
            if(g_m2_data->intersection_ids_1.size() == 0){
                application->update_message("No intersections were found with streets 1 & 2. Please reset and try again!");
            } else if(g_m2_data->intersection_ids_2.size() == 0){
                application->update_message("No intersections were found with streets 3 & 4. Please reset and try again!");
            } else {    // if intersections are found
                application->update_message("Intersection found! Take a look!");
            }
            
            std::string main_canvas_id = application->get_main_canvas_id();
            
            
            g_m2_data->draw_found_intersection_bool = true;
            

        } else if ((g_m2_data->street1_id != -1) && (g_m2_data->street2_id != -1)){
            
            find_intersections(application);

            if(g_m2_data->intersection_ids_1.size() == 0){
                application->update_message("No intersections were found with streets 1 & 2. Please reset and try again!");
            }else {
                application->update_message("Intersection found! Take a look!");
            }
        }
            
    application->refresh_drawing();
 }
  
//this function finds the intersections based on street names entered
void find_intersections (ezgl::application * application){
    // now both streets are selected
    std::string main_canvas_id=application->get_main_canvas_id();
    auto canvas = application->get_canvas(main_canvas_id);
    ezgl::point2d point1(0,0);
    ezgl::point2d point2(0,0);
    g_m2_data->found_intersection_2 = {0.0,0.0};  
    ezgl::point2d point(0,0);
    
    g_m2_data->intersection_ids_1 = find_intersection_ids_from_street_ids(g_m2_data->street1_id, g_m2_data->street2_id);

    if(g_m2_data->intersection_ids_1.size() == 0){
        g_m2_data->intersections_found_bool = false;
    }else{
        std::cout << "Printing found intersections: \n";

        for(unsigned j = 0; j < g_m2_data->intersection_ids_1.size(); j++){
            std::cout << getIntersectionName(g_m2_data->intersection_ids_1[j]) << "\n";
        }
        
        g_m2_data->intersections_found_bool = true;
        std::cout << "End of results\nPress 'Reset' to start searching again.\n";
        
        
        
        latlon_to_point(getIntersectionPosition(g_m2_data->intersection_ids_1[0]), point.x,point.y);
        g_m2_data->found_intersection = point;

        g_m2_data->found_first_Intersection_bool = true;
        
        //Autozoom to intersection
        latlon_to_point(getIntersectionPosition(g_m2_data->intersection_ids_1[0]), point1.x,point1.y);
        ezgl::zoom_in(canvas,sqrt(canvas->get_camera().get_world().area()/(pow((3.0/5.0),20)*canvas->get_camera().get_initial_world().area())));
        ezgl::translate(canvas,point1.x-canvas->get_camera().get_world().center_x(), point1.y-canvas->get_camera().get_world().center_y());
    }
    if ((g_m2_data->street3_id != -1) && (g_m2_data->street4_id != -1)){
        g_m2_data->intersection_ids_2 = find_intersection_ids_from_street_ids(g_m2_data->street3_id, g_m2_data->street4_id);

        if(g_m2_data->intersection_ids_2.size() == 0){
            g_m2_data->intersections_found_bool = false;
        }else{
            std::cout << "Printing found intersections: \n";

            for(unsigned j = 0; j < g_m2_data->intersection_ids_2.size(); j++){
                std::cout << getIntersectionName(g_m2_data->intersection_ids_2[j]) << "\n";
            }

            g_m2_data->intersections_found_bool = true;
            std::cout << "End of results\nPress 'Reset' to start searching again.\n";
            
            latlon_to_point(getIntersectionPosition(g_m2_data->intersection_ids_2[0]), point.x,point.y);
            g_m2_data->found_intersection_2 = point;

            g_m2_data->found_second_Intersection_bool = true;
            
            //Autozoom to path
            latlon_to_point(getIntersectionPosition(g_m2_data->intersection_ids_2[0]), point2.x,point2.y);
            double scale =std::min(canvas->get_camera().get_world().height()/fabs(point2.y-point1.y),canvas->get_camera().get_world().width()/fabs(point2.x-point1.x));
            if(!(canvas->get_camera().get_world().area()/(scale*scale)>pow((3.0/5.0),20)*canvas->get_camera().get_initial_world().area()))
                scale = sqrt(canvas->get_camera().get_world().area()/(pow((3.0/5.0),20)*canvas->get_camera().get_initial_world().area()));
            
            ezgl::zoom_in(canvas,scale);
            ezgl::translate(canvas,(point1.x+point2.x)/2.0-canvas->get_camera().get_world().center_x(), (point1.y+point2.y)/2.0-canvas->get_camera().get_world().center_y());
        }
    }
}

// this function handles code for enabaling street name suggestions in the UI
void press_street_suggestions(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *object_find_text = application->get_object("StreetSearch_1");
    GtkEditable *editable_find = (GtkEditable*) object_find_text;
    
    std::string response = gtk_editable_get_chars(editable_find, 0, -1);
    g_m2_data->count = 0;
    
    if (response != ""){
        g_m2_data->g_response = response;
        g_m2_data->street_ids_found = find_street_ids_from_partial_street_name(response);
    
        if (g_m2_data->street_ids_found.size() != 0){
            std::string street_name = getStreetName(g_m2_data->street_ids_found[g_m2_data->count]);

            GObject *object_suggestions = application->get_object("Street_Suggestions");
            GtkEditable *editable_suggestions = (GtkEditable*) object_suggestions;
            gtk_editable_delete_text(editable_suggestions, 0, -1);

            gint start = 0;

            application->update_message("Did you mean this? Press 'Y'/'N': ");
            gtk_editable_insert_text (editable_suggestions, street_name.c_str(), -1, &start);
        } else {
            application->update_message("Get started by entering a street name: ");
            
            gtk_editable_delete_text(editable_find, 0, -1);

            object_find_text = application->get_object("StreetSearch_2");
            editable_find = (GtkEditable*) object_find_text;
            gtk_editable_delete_text(editable_find, 0, -1);

            object_find_text = application->get_object("StreetSearch_3");
            editable_find = (GtkEditable*) object_find_text;
            gtk_editable_delete_text(editable_find, 0, -1);

            object_find_text = application->get_object("StreetSearch_4");
            editable_find = (GtkEditable*) object_find_text;
            gtk_editable_delete_text(editable_find, 0, -1);
            
            GObject *object_suggestions = application->get_object("Street_Suggestions");
            GtkEditable *editable_suggestions = (GtkEditable*) object_suggestions;
            gtk_editable_delete_text(editable_suggestions, 0, -1);
        }
        
    } else {
        application->update_message("Street Suggestions turned on!");
    }
}

// this function handles event if there are changes made to the first street text box
void textbox1_changed (GtkEditable *editable, gpointer data){
    
    auto application = static_cast<ezgl::application *>(data);
    
    std::string response = gtk_editable_get_chars(editable, 0, -1);
    g_m2_data->g_response = response;
    
    if (response != ""){
        g_m2_data->street_ids_found = find_street_ids_from_partial_street_name(response);
        
        if (g_m2_data->street_ids_found.size() != 0){
            std::string street_name = getStreetName(g_m2_data->street_ids_found[g_m2_data->count]);
            
            if (street_name != "<unknown>"){
                GObject *object_suggestions = application->get_object("Street_Suggestions");
                GtkEditable *editable_suggestions = (GtkEditable*) object_suggestions;
                gtk_editable_delete_text(editable_suggestions, 0, -1);

                gint start = 0;

                application->update_message("Did you mean this? Press 'Y'/'N': ");
                gtk_editable_insert_text (editable_suggestions, street_name.c_str(), -1, &start);
            }
            
        } else {
            application->update_message("Street 1 not found! Please check your input and try again!");
        }
        
    }
    
}

// this function handles event if there are changes made to the second street text box
void textbox2_changed (GtkEditable *editable, gpointer data){
    
    auto application = static_cast<ezgl::application *>(data);
    
    
    g_m2_data->g_response = gtk_editable_get_chars(editable, 0, -1);
    
    if (g_m2_data->g_response != ""){
        g_m2_data->street_ids_found = find_street_ids_from_partial_street_name(g_m2_data->g_response);
        
        if (g_m2_data->street_ids_found.size() != 0){
            std::string street_name = getStreetName(g_m2_data->street_ids_found[g_m2_data->count]);

            GObject *object_suggestions = application->get_object("Street_Suggestions");
            GtkEditable *editable_suggestions = (GtkEditable*) object_suggestions;
            gtk_editable_delete_text(editable_suggestions, 0, -1);

            gint start = 0;

            application->update_message("Did you mean this? Press 'Y'/'N': ");
            gtk_editable_insert_text (editable_suggestions, street_name.c_str(), -1, &start);
        }else {
            application->update_message("Street 2 not found! Please check your input and try again!");
        }
    }
    
}

// this function handles event if there are changes made to the third street text box
void textbox3_changed (GtkEditable *editable, gpointer data){
    
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *menu_rev_search = application->get_object("SearchButton");
    GtkToggleButton *menu_rev_p_search=(GtkToggleButton *)menu_rev_search;
    
    
    if (gtk_toggle_button_get_active(menu_rev_p_search) == false){
        g_m2_data->g_response = gtk_editable_get_chars(editable, 0, -1);
    
        if (g_m2_data->g_response != ""){
            g_m2_data->street_ids_found = find_street_ids_from_partial_street_name(g_m2_data->g_response);

            if (g_m2_data->street_ids_found.size() != 0){
                std::string street_name = getStreetName(g_m2_data->street_ids_found[g_m2_data->count]);

                GObject *object_suggestions = application->get_object("Street_Suggestions");
                GtkEditable *editable_suggestions = (GtkEditable*) object_suggestions;
                gtk_editable_delete_text(editable_suggestions, 0, -1);

                gint start = 0;

                application->update_message("Did you mean this? Press 'Y'/'N': ");
                gtk_editable_insert_text (editable_suggestions, street_name.c_str(), -1, &start);
            }else {
                application->update_message("Street 3 not found! Please check your input and try again!");
            }
        }
    } else {
        application->update_message("Tip: Make use of streets 1 and 2 only for search mode!");
    }
    
    
}

// this function handles event if there are changes made to the fourth street text box
void textbox4_changed (GtkEditable *editable, gpointer data){
    
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *menu_rev_search = application->get_object("SearchButton");
    GtkToggleButton *menu_rev_p_search=(GtkToggleButton *)menu_rev_search;
    
    
    if (gtk_toggle_button_get_active(menu_rev_p_search) == false){
        g_m2_data->g_response = gtk_editable_get_chars(editable, 0, -1);
    
        if (g_m2_data->g_response != ""){
            g_m2_data->street_ids_found = find_street_ids_from_partial_street_name(g_m2_data->g_response);

            if (g_m2_data->street_ids_found.size() != 0){
                std::string street_name = getStreetName(g_m2_data->street_ids_found[g_m2_data->count]);

                GObject *object_suggestions = application->get_object("Street_Suggestions");
                GtkEditable *editable_suggestions = (GtkEditable*) object_suggestions;
                gtk_editable_delete_text(editable_suggestions, 0, -1);

                gint start = 0;

                application->update_message("Did you mean this? Press 'Y'/'N': ");
                gtk_editable_insert_text (editable_suggestions, street_name.c_str(), -1, &start);
            }else {
                application->update_message("Street 4 not found! Please check your input and try again!");
            }


        }
    }else {
        application->update_message("Tip: Make use of streets 1 and 2 only for search mode!");
    }
    
}


//function for taking a confirmation for the street name entered
//this enables implementation of partial street name search and helps user select the street name they wanted among the other relevant similar street names
void press_yes(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);  
    
    gint start = 0;
    
    GObject *object_suggestions = application->get_object("Street_Suggestions");
    GtkEditable *editable_suggestions = (GtkEditable*) object_suggestions;
    
    std::string response = gtk_editable_get_chars(editable_suggestions, 0, -1);
    gtk_editable_delete_text(editable_suggestions, 0, -1);
    
    if (g_m2_data->street1_id == -1){
        object_suggestions = application->get_object("StreetSearch_1");
        editable_suggestions = (GtkEditable*) object_suggestions;
        gtk_editable_delete_text(editable_suggestions, 0, -1);
        
        gtk_editable_insert_text (editable_suggestions, response.c_str(), -1, &start);
        
        g_m2_data->street1_id = -2;
    } else if (g_m2_data->street2_id == -1){
        object_suggestions = application->get_object("StreetSearch_2");
        editable_suggestions = (GtkEditable*) object_suggestions;
        gtk_editable_delete_text(editable_suggestions, 0, -1);

        gtk_editable_insert_text (editable_suggestions, response.c_str(), -1, &start);
        g_m2_data->street2_id = -2;
    } else if (g_m2_data->street3_id == -1){
        object_suggestions = application->get_object("StreetSearch_3");
        editable_suggestions = (GtkEditable*) object_suggestions;
        gtk_editable_delete_text(editable_suggestions, 0, -1);

        gtk_editable_insert_text (editable_suggestions, response.c_str(), -1, &start);
        g_m2_data->street3_id = -2;
    } else if (g_m2_data->street4_id == -1){
        object_suggestions = application->get_object("StreetSearch_4");
        editable_suggestions = (GtkEditable*) object_suggestions;
        gtk_editable_delete_text(editable_suggestions, 0, -1);

        gtk_editable_insert_text (editable_suggestions, response.c_str(), -1, &start);
        g_m2_data->street4_id = -2;
    }
    
    application->refresh_drawing();
    
}

//function for getting a confirmation to see more results when partial street names have a number of street names similar to them
void press_no(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);  
    
    gint start = 0;
    if (g_m2_data->g_response != ""){
        g_m2_data->count++;
        try{
            if(g_m2_data->street_ids_found.size()>0){
                if(g_m2_data->count>=int(g_m2_data->street_ids_found.size()))
                    g_m2_data->count=0;
            std::string street_name = getStreetName(g_m2_data->street_ids_found[g_m2_data->count]);

            GObject *object_suggestions = application->get_object("Street_Suggestions");
            GtkEditable *editable_suggestions = (GtkEditable*) object_suggestions;
            gtk_editable_delete_text(editable_suggestions, 0, -1);

            gtk_editable_insert_text (editable_suggestions, street_name.c_str(), -1, &start);

            application->update_message("Did you mean this? Press 'Y'/'N': ");
            }
        } catch (...){
            g_m2_data->count = 0;

            if (g_m2_data->g_response != ""){
                
                g_m2_data->street_ids_found = find_street_ids_from_partial_street_name(g_m2_data->g_response);
                std::string street_name = getStreetName(g_m2_data->street_ids_found[g_m2_data->count]);


                GObject *object_suggestions = application->get_object("Street_Suggestions");
                GtkEditable *editable_suggestions = (GtkEditable*) object_suggestions;
                gtk_editable_delete_text(editable_suggestions, 0, -1);

                gtk_editable_insert_text (editable_suggestions, street_name.c_str(), -1, &start);

                application->update_message("Did you mean this? Press 'Y'/'N': ");
            } else {
                application->update_message("Tip: Start by entering a street name below, use Y/N to confirm/view more suggestions:");
            }
        
        }
    } else {
        application->update_message("Tip: Start by entering a street name below, use Y/N to confirm/view more suggestions:");
    }
    
    
    
    application->refresh_drawing();
}


// function for resetting past street names entered
//this allows for performing more searches by removing plotted points in the map
void press_reset(GtkWidget *, gpointer data)
{
    auto application = static_cast<ezgl::application *>(data);
    GObject *object_find_text = application->get_object("StreetSearch_1");
    GtkEditable *editable_find = (GtkEditable*) object_find_text;
    gtk_editable_delete_text(editable_find, 0, -1);
    
    object_find_text = application->get_object("StreetSearch_2");
    editable_find = (GtkEditable*) object_find_text;
    gtk_editable_delete_text(editable_find, 0, -1);
    
    object_find_text = application->get_object("StreetSearch_3");
    editable_find = (GtkEditable*) object_find_text;
    gtk_editable_delete_text(editable_find, 0, -1);
    
    object_find_text = application->get_object("StreetSearch_4");
    editable_find = (GtkEditable*) object_find_text;
    gtk_editable_delete_text(editable_find, 0, -1);
    
    object_find_text = application->get_object("Street_Suggestions");
    editable_find = (GtkEditable*) object_find_text;
    gtk_editable_delete_text(editable_find, 0, -1);

    g_m2_data->found_intersection = {0.0,0.0};
    g_m2_data->found_intersection_2 = {0.0,0.0};    
    g_m2_data->found_first_Intersection_bool = false;
    g_m2_data->found_second_Intersection_bool = false;
    
    g_m2_data->found_first_Intersection_bool = false;
    g_m2_data->found_second_Intersection_bool = false;
    g_m2_data->found_intersection = {0.0,0.0};
    g_m2_data->found_intersection_2 = {0.0,0.0};
    
    g_m2_data->intersections_found_bool = false;
    g_m2_data->street1_id = -1;
    g_m2_data->street2_id = -1;
    g_m2_data->street3_id = -1;
    g_m2_data->street4_id = -1;
    application->update_message("Reset successful. Type a street name to start searching: ");
    g_m2_data->draw_found_intersection_bool = false;
    application->refresh_drawing();

}

// this will be executed when user checks search option in the UI
void press_search_only(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *menu_rev_search = application->get_object("SearchButton");
    GtkToggleButton *menu_rev_p_search=(GtkToggleButton *)menu_rev_search;
    
    GObject *menu_rev = application->get_object("NavigateButton");
    GtkToggleButton *menu_rev_p=(GtkToggleButton *)menu_rev;
    
    
    
    if (gtk_toggle_button_get_active(menu_rev_p_search) == true){

        application->update_message("With search mode, use streets 1&2 to find and view intersections on map: ");

        gtk_toggle_button_set_active(menu_rev_p, FALSE);    // un-toggle the navigate mode
        GObject* object_find_text = application->get_object("StreetSearch_3");
        GtkEditable *editable_find = (GtkEditable*) object_find_text;
        GtkEntry *entry_find = (GtkEntry*) object_find_text;
        gtk_editable_delete_text(editable_find, 0, -1);
        gtk_entry_set_visibility (entry_find, false);
        gtk_entry_set_invisible_char (entry_find, 'x');

        object_find_text = application->get_object("StreetSearch_4");
        editable_find = (GtkEditable*) object_find_text;
        entry_find = (GtkEntry*) object_find_text;
        gtk_editable_delete_text(editable_find, 0, -1);
        gtk_entry_set_visibility (entry_find, false);
        gtk_entry_set_invisible_char (entry_find, 'x');
        application->refresh_drawing();
    } else {
        GObject* object_find_text = application->get_object("StreetSearch_3");
        GtkEditable *editable_find = (GtkEditable*) object_find_text;
        GtkEntry *entry_find = (GtkEntry*) object_find_text;
        gtk_editable_delete_text(editable_find, 0, -1);
        gtk_entry_set_visibility (entry_find, true);

        object_find_text = application->get_object("StreetSearch_4");
        editable_find = (GtkEditable*) object_find_text;
        entry_find = (GtkEntry*) object_find_text;
        gtk_editable_delete_text(editable_find, 0, -1);
        gtk_entry_set_visibility (entry_find, true);
            
        if (gtk_toggle_button_get_active(menu_rev_p) == false){ // if navigate is not on
           
            application->update_message("Tip: Select 'Search' OR 'Navigate' to simplify your search: ");
            
        }
    }
    application->refresh_drawing();
}

// this will be executed when user checks navigation option in the UI
void press_navigate(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);    
    
    GObject *menu_rev_navigate = application->get_object("NavigateButton");
    GtkToggleButton *menu_rev_p_navigate=(GtkToggleButton *)menu_rev_navigate;
    GObject *menu_rev = application->get_object("SearchButton");
    GtkToggleButton *menu_rev_p=(GtkToggleButton *)menu_rev;
    
    //the transition from search to navigate mode or vice versa should always clear the clicked intersections
    g_m2_data->navigation_mode_bool = gtk_toggle_button_get_active(menu_rev_p_navigate);
    g_m2_data->clicked_intersection = {0.0,0.0};
    g_m2_data->clicked_intersection_2 = {0.0,0.0};    
    g_m2_data->clicked_on_an_Intersection_bool = false;
    g_m2_data->clicked_on_second_Intersection_bool = false;
    
    
    if (gtk_toggle_button_get_active(menu_rev_p_navigate) == true){
       
    
        application->update_message("With navigate mode, you can enter 4 streets to find view fastest path on map: ");

        gtk_toggle_button_set_active(menu_rev_p, FALSE);    // un-toggle the find mode
        application->refresh_drawing();
    } else {
        if (gtk_toggle_button_get_active(menu_rev_p) == false){ // if search is not on
           
    
            application->update_message("Tip: Select 'Search' OR 'Navigate' to simplify your search: ");
        }
    }
    
    
    application->refresh_drawing();
}

//this function set an addition width to be added to the large road width
//width size will be set based on the zoom level
double dynamic_width_large_roads(){
    
    if(g_m2_data->zoom_level == 4){
        return 3;
    }else if(g_m2_data->zoom_level == 5){
        return 4;
    }else if(g_m2_data->zoom_level == 6){
        return 4;
    }else if(g_m2_data->zoom_level == 7){
        return 6;
    }else if(g_m2_data->zoom_level == 8){
        return 8;
    }else if(g_m2_data->zoom_level == 9){
        return 10; 
    }else if(g_m2_data->zoom_level == 10){
        return 14;
    }else if(g_m2_data->zoom_level == 11){
        return 18;
    }
    return 0;
}

//this function set an addition width to be added to the small road width
//width size will be set based on the zoom level
double dynamic_width_small_roads(){
    
    if(g_m2_data->zoom_level == 7){
        return 3;
    }else if(g_m2_data->zoom_level == 8){
        return 5;
    }else if(g_m2_data->zoom_level == 9){
        return 7; 
    }else if(g_m2_data->zoom_level == 10){
        return 8;
    }else if(g_m2_data->zoom_level == 11){
        return 12;
    }

    return 0;
}


//this function divides the map into 10 zooms called zoom_level
//zoom_level is stored in global var g_zoom_level
void zoom_level(ezgl::renderer &g){
    
    //std::cout << "current area: " << g.get_visible_world().area() << std::endl;
    
    if(g.get_visible_world().area() >=  g_m2_data->initial_world.area()){
        g_m2_data->zoom_level = 1;
    }else if(g.get_visible_world().area() < g_m2_data->initial_world.area() && g.get_visible_world().area() >= g_m2_data->initial_world.area()*pow((3.0/5.0),2)){
        g_m2_data->zoom_level = 2;
    }else if(g.get_visible_world().area() < g_m2_data->initial_world.area()*pow((3.0/5.0),2) && g.get_visible_world().area() >= g_m2_data->initial_world.area()*pow((3.0/5.0),4)){
        g_m2_data->zoom_level = 3;
    }else if(g.get_visible_world().area() < g_m2_data->initial_world.area()*pow((3.0/5.0),4) && g.get_visible_world().area() >= g_m2_data->initial_world.area()*pow((3.0/5.0),6)){
        g_m2_data->zoom_level = 4;
    }else if(g.get_visible_world().area() < g_m2_data->initial_world.area()*pow((3.0/5.0),6) && g.get_visible_world().area() >= g_m2_data->initial_world.area()*pow((3.0/5.0),8)){
        g_m2_data->zoom_level = 5;
    }else if(g.get_visible_world().area() < g_m2_data->initial_world.area()*pow((3.0/5.0),8) && g.get_visible_world().area() >= g_m2_data->initial_world.area()*pow((3.0/5.0),10)){
        g_m2_data->zoom_level = 6;
    }else if(g.get_visible_world().area() < g_m2_data->initial_world.area()*pow((3.0/5.0),10) && g.get_visible_world().area() >= g_m2_data->initial_world.area()*pow((3.0/5.0),12)){
        g_m2_data->zoom_level = 7;
    }else if(g.get_visible_world().area() < g_m2_data->initial_world.area()*pow((3.0/5.0),12) && g.get_visible_world().area() >= g_m2_data->initial_world.area()*pow((3.0/5.0),14)){
        g_m2_data->zoom_level = 8;
    }else if(g.get_visible_world().area() < g_m2_data->initial_world.area()*pow((3.0/5.0),14) && g.get_visible_world().area() >= g_m2_data->initial_world.area()*pow((3.0/5.0),16)){
        g_m2_data->zoom_level = 9;
    }else if(g.get_visible_world().area() < g_m2_data->initial_world.area()*pow((3.0/5.0),16) && g.get_visible_world().area() >= g_m2_data->initial_world.area()*pow((3.0/5.0),18)){
        g_m2_data->zoom_level = 10;
    }else if(g.get_visible_world().area() < g_m2_data->initial_world.area()*pow((3.0/5.0),18) && g.get_visible_world().area() >= g_m2_data->initial_world.area()*pow((3.0/5.0),20)){
        g_m2_data->zoom_level = 11;
    }
}

//this function draws the intersection that is clicked by the mouse
//the red circle will be drawn and the name of the intersection will be displayed
void draw_clicked_intersection(ezgl::renderer &g){

    //draw one intersection when not in navigation mode
    if(g_m2_data->navigation_mode_bool == false){
        LatLon intersection_position(y_to_lat(g_m2_data->clicked_intersection.y),x_to_lon(g_m2_data->clicked_intersection.x));
        
        g.set_text_rotation(0);
        if(g_m2_data->zoom_level >= 7){
            g.set_color(ezgl::RED);
            g.fill_arc({g_m2_data->clicked_intersection.x, g_m2_data->clicked_intersection.y}, INTERSECTION_DOT_RADIUS, 0, 360);

            if(g_m2_data->night_mode_bool){
                g.set_color(ezgl::WHITE);
            }else{
                g.set_color(ezgl::BLACK);
            }

            g.set_font_size(15);
            g.draw_text({g_m2_data->clicked_intersection.x,g_m2_data->clicked_intersection.y + TEXT_POSTITION_OFFSET}, g_m2_data->intersections[find_closest_intersection(intersection_position)].name, 100, 200);

            //std::cout << "First intersection clicked:  " << g_m2_data->intersections[find_closest_intersection(intersection_position)].name << std::endl;
        }
    //draw the two clicked intersections when in navigation mode    
    }else if(g_m2_data->navigation_mode_bool == true){
        LatLon intersection_position(y_to_lat(g_m2_data->clicked_intersection.y),x_to_lon(g_m2_data->clicked_intersection.x));
        //std::cout << "(" << g_m2_data->clicked_intersection.x << ", " << g_m2_data->clicked_intersection.y << ")" << std::endl;
        LatLon intersection_position_2(y_to_lat(g_m2_data->clicked_intersection_2.y),x_to_lon(g_m2_data->clicked_intersection_2.x));
        
        g.set_text_rotation(0);
        if(g_m2_data->zoom_level >= 7){
            g.set_color(ezgl::RED);
            g.fill_arc({g_m2_data->clicked_intersection.x, g_m2_data->clicked_intersection.y}, INTERSECTION_DOT_RADIUS, 0, 360);

            if(g_m2_data->clicked_on_second_Intersection_bool == true){
                g.fill_arc({g_m2_data->clicked_intersection_2.x, g_m2_data->clicked_intersection_2.y}, INTERSECTION_DOT_RADIUS, 0, 360);
            }   

            if(g_m2_data->night_mode_bool){
                g.set_color(ezgl::WHITE);
            }else{
                g.set_color(ezgl::BLACK);
            }

            g.set_font_size(15);
            g.draw_text({g_m2_data->clicked_intersection.x,g_m2_data->clicked_intersection.y + TEXT_POSTITION_OFFSET}, g_m2_data->intersections[find_closest_intersection(intersection_position)].name, 100, 200);

            //std::cout << "First intersection clicked:  " << g_m2_data->intersections[find_closest_intersection(intersection_position)].name << std::endl;


            if(g_m2_data->clicked_on_second_Intersection_bool == true){
                g.draw_text({g_m2_data->clicked_intersection_2.x,g_m2_data->clicked_intersection_2.y + TEXT_POSTITION_OFFSET}, g_m2_data->intersections[find_closest_intersection(intersection_position_2)].name, 100, 200);
                            
                //std::cout << "x:  " << g_m2_data->clicked_intersection_2.x << ". y: " << g_m2_data->clicked_intersection_2.y << std::endl; 
                //std::cout << "Second intersection clicked:  " << g_m2_data->intersections[find_closest_intersection(intersection_position_2)].name << std::endl << std::endl;

            }
        }
    }
}

// this function below draws the name and puts an intersection icon on the found intersection based on inputs from the find bar
void draw_found_intersection(ezgl::renderer &g){
    
    LatLon intersection_location_latlon;
//    ezgl::point2d intersection_location(0.0, 0.0);

    //ezgl::surface *intersection_png = ezgl::renderer::load_png("libstreetmap/resources/intersection.png");

    
    g.set_text_rotation(0);
    for (unsigned j = 0; j < g_m2_data->intersection_ids_1.size(); j++){
        intersection_location_latlon = getIntersectionPosition(g_m2_data->intersection_ids_1[j]);
//      intersection_location.x = lon_to_x(intersection_location_latlon.lon());
//      intersection_location.y = lat_to_y(intersection_location_latlon.lat());

        latlon_to_point(intersection_location_latlon, g_m2_data->intersection_location.x,g_m2_data->intersection_location.y);

        //g.draw_surface(intersection_png, intersection_location);
        g.set_color(ezgl::YELLOW);
        g.fill_arc({g_m2_data->intersection_location.x,g_m2_data->intersection_location.y}, INTERSECTION_DOT_RADIUS, 0, 360);

        if(g_m2_data->night_mode_bool){
            g.set_color(ezgl::WHITE);
        }else{
            g.set_color(ezgl::BLACK);
        }
        g.set_font_size(15);
        g.draw_text({g_m2_data->intersection_location.x,g_m2_data->intersection_location.y + TEXT_POSTITION_OFFSET}, getIntersectionName(g_m2_data->intersection_ids_1[j]), 100, 200);
    }
        
    //ezgl::renderer::free_surface(intersection_png);
    //application->refresh_drawing();
}

//this function is called when the D button is clicked
//it shows the detailed directions for a path
void press_directions(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  GObject *menu_rev = application->get_object("DirReveal");
  GtkRevealer * menu_rev_p=(GtkRevealer *)menu_rev;
  
  gtk_revealer_set_reveal_child(menu_rev_p, !gtk_revealer_get_reveal_child(menu_rev_p));
  GObject * dir=application->get_object("DirLabel");
  GtkLabel * dir_p=(GtkLabel *)dir;
  
  gtk_label_set_text(dir_p,g_m2_data->directions.c_str());
}

//this function is called when the refresh button is clicked
//it refreshes the path direction when a new path is clicked/searched
void press_refresh(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  GObject * dir=application->get_object("DirLabel");
  GtkLabel * dir_p=(GtkLabel *)dir;
  
  gtk_label_set_text(dir_p,g_m2_data->directions.c_str());
}

//this function is called after user press "Info" 
// a window pops up that asks the user to select the map they want 
void press_info(GtkWidget *, gpointer data)
{
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *window; // the parent window over which to add the dialog
    GtkWidget *content_area; // the content area of the dialog
    GtkWidget *image; // the image we will create to display a message in the content area
    GtkWidget *dialog; // the dialog box we will create
    // get a pointer to the main application window
    window = application->get_object(application->get_main_window_id().c_str());
    
    dialog = gtk_dialog_new_with_buttons(
    "Help",
    (GtkWindow*) window,
    GTK_DIALOG_MODAL,
    ("Previous"),
    1,
    ("Next"),
    2,
    ("Close"),
    GTK_RESPONSE_REJECT,
    NULL
    );
    // Create a label and attach it to the content area of the dialog
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    image = gtk_image_new_from_file("libstreetmap/resources/1.png");
    gtk_container_add(GTK_CONTAINER(content_area), image);
    // The main purpose of this is to show dialog’s child widget, image
    gtk_widget_show_all(dialog);
    g_m2_data->img_count=1;
    // Connecting the "response" signal from the user to the associated callback function
    g_signal_connect(
    GTK_DIALOG(dialog),
    "response",
    G_CALLBACK(on_help_response),
    application);
    application->refresh_drawing();
}


//This function is called when the user makes a selection in the pop up window
//It updates the help step image
void on_help_response(GtkDialog *dialog, gint response_id, gpointer )
{
    // For demonstration purposes, this will show the enum name and int value of the button that was pressed
    
    
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* image=GTK_WIDGET(gtk_container_get_children (GTK_CONTAINER(content_area))->data);
    std::string image_path;
    
    //Destroy dialog if exit command
    if(response_id==GTK_RESPONSE_DELETE_EVENT||response_id==GTK_RESPONSE_REJECT){
        gtk_widget_destroy(GTK_WIDGET (dialog));
    }
    
    //Loads previous step image
    if(response_id==1){
            if(g_m2_data->img_count ==1){

            }else{
                g_m2_data->img_count --;
                image_path="libstreetmap/resources/"+std::to_string(g_m2_data->img_count)+".png";
                gtk_container_remove(GTK_CONTAINER(content_area), image);
                image=gtk_image_new_from_file(image_path.c_str());
                gtk_container_add(GTK_CONTAINER(content_area), image);
                gtk_widget_show_all(GTK_WIDGET(dialog));

            }
    }
    //Loads next step image
    else if(response_id==2){
            if(g_m2_data->img_count==8){

            }else{
                g_m2_data->img_count ++;
                image_path="libstreetmap/resources/"+std::to_string(g_m2_data->img_count)+".png";
                gtk_container_remove(GTK_CONTAINER(content_area), image);
                image=gtk_image_new_from_file(image_path.c_str());
                gtk_container_add(GTK_CONTAINER(content_area), image);
                gtk_widget_show_all(GTK_WIDGET(dialog));
            }

    }
    
}


//this function is called after user press "Change Map" 
// a window pops up that asks the user to select the map they want 
void press_change(GtkWidget *, gpointer data)
{
    auto application = static_cast<ezgl::application *>(data);
    
    GObject *window; // the parent window over which to add the dialog
    GtkWidget *content_area; // the content area of the dialog
    GtkWidget *label; // the label we will create to display a message in the content area
    GtkWidget *dialog; // the dialog box we will create
    // get a pointer to the main application window
    window = application->get_object(application->get_main_window_id().c_str());
    
    dialog = gtk_dialog_new_with_buttons(
    "Change Map",
    (GtkWindow*) window,
    GTK_DIALOG_MODAL,
    ("Toronto"),
    1,
    ("Cairo"),
    2,
    ("Beijing"),
    3,
    ("Tehran"),
    4,
    ("Iceland"),
    5,
    ("Rio De Janeiro"),
    6,
    ("Golden Horseshoe"),
    7,
    ("New Delhi"),
    8,
    ("New York"),
    9,
    ("London"),
    10,
    ("Saint Helena"),
    11,
    ("CANCEL"),
    GTK_RESPONSE_REJECT,
    NULL
    );
    // Create a label and attach it to the content area of the dialog
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    label = gtk_label_new("Select a Map to Open:");
    gtk_container_add(GTK_CONTAINER(content_area), label);
    // The main purpose of this is to show dialog’s child widget, label
    gtk_widget_show_all(dialog);
    
    // Connecting the "response" signal from the user to the associated callback function
    g_signal_connect(
    GTK_DIALOG(dialog),
    "response",
    G_CALLBACK(on_dialog_response),
    application);
    application->refresh_drawing();
}

//This function is called when the user makes a selection in the pop up window
//It updates the map based on what user selects
void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    // For demonstration purposes, this will show the enum name and int value of the button that was pressed
    auto application = static_cast<ezgl::application *>(user_data);
    std::string new_map = "";
    switch(response_id) {

        case GTK_RESPONSE_DELETE_EVENT:
            break;
        case GTK_RESPONSE_REJECT:
            break;
        case 1:
            new_map="/cad2/ece297s/public/maps/toronto_canada.streets.bin";
            
            break;
        case 2:
            new_map="/cad2/ece297s/public/maps/cairo_egypt.streets.bin";
            
            break;
        case 3:
            new_map="/cad2/ece297s/public/maps/beijing_china.streets.bin";
            
            break;
        case 4:
            new_map="/cad2/ece297s/public/maps/tehran_iran.streets.bin";
            
            break;
        case 5:
            new_map="/cad2/ece297s/public/maps/iceland.streets.bin";
            
            break;
        case 6:
            new_map="/cad2/ece297s/public/maps/rio-de-janeiro_brazil.streets.bin";
            
            break;
        case 7:
            new_map="/cad2/ece297s/public/maps/golden-horseshoe_canada.streets.bin";
            
            break;
        case 8:
            new_map="/cad2/ece297s/public/maps/new-delhi_india.streets.bin";
            
            break;
        case 9:
            new_map="/cad2/ece297s/public/maps/new-york_usa.streets.bin";
            
            break;
        case 10:
            new_map="/cad2/ece297s/public/maps/london_england.streets.bin";
            break;
        case 11:
            new_map="/cad2/ece297s/public/maps/saint-helena.streets.bin";
            
            break;
        default:
        break;
    }
    // This will cause the dialog to be destroyed and close
    // without this line the dialog remains open unless the
    // response_id is GTK_RESPONSE_DELETE_EVENT which
    // automatically closes the dialog without the following line.
    if(new_map!=""){
        delete g_m2_data;
        close_map();
        load_map(new_map);
        update_map();
        application->get_canvas(application->get_main_canvas_id())->get_camera().set_initial_world(g_m2_data->initial_world);
        application->get_canvas(application->get_main_canvas_id())->get_camera().set_world(g_m2_data->initial_world);
        GObject *night_sw = application->get_object("NightSwitch");
        GtkSwitch * night_sw_ptr=(GtkSwitch *)night_sw;
        gtk_switch_set_active(night_sw_ptr, false);
        GObject *check_box = application->get_object("HospitalCheck");
        GtkToggleButton * check_box_ptr=(GtkToggleButton *)check_box;
        gtk_toggle_button_set_active(check_box_ptr,false);
        check_box = application->get_object("BankCheck");
        check_box_ptr=(GtkToggleButton *)check_box;
        gtk_toggle_button_set_active(check_box_ptr,false);
        check_box = application->get_object("CafeCheck");
        check_box_ptr=(GtkToggleButton *)check_box;
        gtk_toggle_button_set_active(check_box_ptr,false);
        check_box = application->get_object("RestaurantCheck");
        check_box_ptr=(GtkToggleButton *)check_box;
        gtk_toggle_button_set_active(check_box_ptr,false);
        check_box = application->get_object("GasCheck");
        check_box_ptr=(GtkToggleButton *)check_box;
        gtk_toggle_button_set_active(check_box_ptr,false);
        check_box = application->get_object("StationCheck");
        check_box_ptr=(GtkToggleButton *)check_box;
        gtk_toggle_button_set_active(check_box_ptr,false);
        check_box = application->get_object("POICheck");
        check_box_ptr=(GtkToggleButton *)check_box;
        gtk_toggle_button_set_active(check_box_ptr,false);
        application->refresh_drawing();
    }
    gtk_widget_destroy(GTK_WIDGET (dialog));
}

//This function updates the map to be drawn
void update_map(){
    
    g_m2_data = new M2_SuperClass();
    
    //get the bound of the map.
    initialize_world();
    
    //load all the required data into g_m2_data
    load_POI_data();
    load_OSM_data();
    
    load_features_data();
    
}

//set initial world bound based on min/max of LatLon
void initialize_world(){
    g_m2_data->latMin = getIntersectionPosition(0).lat();
    g_m2_data->latMax = getIntersectionPosition(0).lat();
    g_m2_data->lonMin = getIntersectionPosition(0).lon();
    g_m2_data->lonMax = getIntersectionPosition(0).lon();

    g_m2_data->intersections.resize(getNumIntersections());

    for(int i = 0; i < getNumIntersections(); ++i){
        g_m2_data->intersections[i].position = getIntersectionPosition(i);
        g_m2_data->intersections[i].name = getIntersectionName(i);

        g_m2_data->latMin=std::min(g_m2_data->latMin,g_m2_data->intersections[i].position.lat());
        g_m2_data->latMax=std::max(g_m2_data->latMax,g_m2_data->intersections[i].position.lat());
        g_m2_data->lonMin=std::min(g_m2_data->lonMin,g_m2_data->intersections[i].position.lon());
        g_m2_data->lonMax=std::max(g_m2_data->lonMax,g_m2_data->intersections[i].position.lon());
    }
    for(int i=0;i<getNumFeatures();i++){
        for(int j=0; j<getFeaturePointCount(i);j++){
            g_m2_data->latMin=std::min(g_m2_data->latMin,getFeaturePoint(j,i).lat());
            g_m2_data->latMax=std::max(g_m2_data->latMax,getFeaturePoint(j,i).lat());
            g_m2_data->lonMin=std::min(g_m2_data->lonMin,getFeaturePoint(j,i).lon());
            g_m2_data->lonMax=std::max(g_m2_data->lonMax,getFeaturePoint(j,i).lon());
        }
    }

    g_m2_data->initial_world.m_first={lon_to_x(g_m2_data->lonMin), lat_to_y(g_m2_data->latMin)};
    g_m2_data->initial_world.m_second={lon_to_x(g_m2_data->lonMax),lat_to_y(g_m2_data->latMax)};
}

//Determines values to insert into POI  data structure and inserts if applicable
void load_POI_data(){
    g_m2_data->POIs.resize(getNumPointsOfInterest());
    for(int i = 0; i < getNumPointsOfInterest(); i++){
        g_m2_data->POIs[i].name=getPointOfInterestName(i);
        g_m2_data->POIs[i].type=getPointOfInterestType(i);
        latlon_to_point(getPointOfInterestPosition(i),g_m2_data->POIs[i].location.x,g_m2_data->POIs[i].location.y);
    }
}

//Determines values to insert into OSM data structure and inserts if applicable
void load_OSM_data(){
    for(unsigned int i=0; i<getNumberOfNodes();i++){
        g_m2_data->OSM_data.OSMNodes.insert({getNodeByIndex(i)->id(),i});
        const OSMNode *current = getNodeByIndex(i);
        for(unsigned int j=0; j<getTagCount(current);j++){
            std:: string key, value;
            std::tie(key,value) = getTagPair(current,j);
            if(key=="railway"&&value=="station"){
                POIData temp;
                for(unsigned int k=0; k<getTagCount(current);k++){
                    std:: string key_temp, value_temp;
                    std::tie(key_temp,value_temp) = getTagPair(current,k);
                    if(key_temp=="name"){
                        temp.name = value_temp;
                    }
                }
                
                temp.type = "station";
                latlon_to_point(getNodeByIndex(i)->coords(), temp.location.x,temp.location.y);
                g_m2_data->POIs.push_back(temp);
            }
        }
    }

    for(unsigned int i=0; i<getNumberOfWays();i++){
        g_m2_data->OSM_data.OSMWays.insert({getWayByIndex(i)->id(),i});
    }
    for(unsigned int i=0; i<getNumberOfRelations();i++){
        g_m2_data->OSM_data.OSMRelations.insert({getRelationByIndex(i)->id(),i});
        const OSMRelation *current = getRelationByIndex(i);
        for(unsigned int j=0; j<getTagCount(current);j++){
            std:: string key, value;
            std::tie(key,value) = getTagPair(current,j);
            if(key=="route"&&value=="subway"){
                std::vector<OSMRelation::Member> temp =current->members();
                for(unsigned int k=0; k<temp.size();k++){
                    
                    if(temp[k].tid.type()==TypedOSMID::Way){
                        std::vector<OSMID> a=getWayByIndex(g_m2_data->OSM_data.OSMWays[temp[k].tid])->ndrefs();
                        std::vector<ezgl::point2d>wayy;
                        for(unsigned int l=0; l<a.size();l++){
                            wayy.push_back({0,0});
                            latlon_to_point(getNodeByIndex(g_m2_data->OSM_data.OSMNodes[a[l]])->coords(),wayy[l].x,wayy[l].y);
                      
                        }
                        g_m2_data->subways.push_back(wayy);
                    }
                }
            }
        }
    }
    load_segments_data();
}

//Determines values to insert into segments data structure and inserts if applicable
void load_segments_data(){
    g_m2_data->segments.resize(getNumStreetSegments());
    for(int i=0; i<getNumStreetSegments();i++){
        g_m2_data->segments[i].x.resize(getInfoStreetSegment(i).curvePointCount+2);  // + 2 for the beginning and the end
        g_m2_data->segments[i].y.resize(getInfoStreetSegment(i).curvePointCount+2);
        g_m2_data->segments[i].name = getStreetName(getInfoStreetSegment(i).streetID);
        g_m2_data->segments[i].one_way=getInfoStreetSegment(i).oneWay;
        latlon_to_point(getIntersectionPosition(getInfoStreetSegment(i).from),g_m2_data->segments[i].x[0], g_m2_data->segments[i].y[0]);
        latlon_to_point(getIntersectionPosition(getInfoStreetSegment(i).to),g_m2_data->segments[i].x[getInfoStreetSegment(i).curvePointCount+1], g_m2_data->segments[i].y[getInfoStreetSegment(i).curvePointCount+1]);
        
        for(int j=0; j<getInfoStreetSegment(i).curvePointCount; j++){
            latlon_to_point(getStreetSegmentCurvePoint(j,i),g_m2_data->segments[i].x[j+1], g_m2_data->segments[i].y[j+1]);
        }

        for(unsigned int j=0; j<g_m2_data->segments[i].x.size()-1; j++){
            g_m2_data->segments[i].length.push_back(sqrt(pow(g_m2_data->segments[i].y[j+1]-g_m2_data->segments[i].y[j],2)+pow(g_m2_data->segments[i].x[j+1]-g_m2_data->segments[i].x[j],2)));
            g_m2_data->segments[i].angle.push_back(atan2(g_m2_data->segments[i].y[j+1]-g_m2_data->segments[i].y[j], g_m2_data->segments[i].x[j+1]-g_m2_data->segments[i].x[j]));

        }
        
        g_m2_data->segments[i].osmid=g_m2_data->OSM_data.OSMWays[getInfoStreetSegment(i).wayOSMID];

        const OSMWay *temp = getWayByIndex(g_m2_data->segments[i].osmid);
        for(unsigned j=0;j<getTagCount(temp);j++){
           std:: string key, value;
           std::tie(key,value) = getTagPair(temp,j); 

           if(key=="highway"){
               g_m2_data->segments[i].highway=value;
               break;
           }
        }
    }
}
struct comp{
    bool operator()(FeatureData f1, FeatureData f2){
        return f1.area>f2.area;
    }
    
};
//Determines values to insert into features data structure and inserts if applicable
void load_features_data(){
    g_m2_data->features.resize(getNumFeatures());
    for(int i=0; i<getNumFeatures();i++){
         FeatureData temp;
        temp.feature_name=getFeatureName(i);
        temp.feature_type=getFeatureType(i);
        temp.closed=getFeaturePoint(0,i).lat()==getFeaturePoint(getFeaturePointCount(i)-1,i).lat()&&getFeaturePoint(0,i).lon()==getFeaturePoint(getFeaturePointCount(i)-1,i).lon();
        for(int j=0; j<getFeaturePointCount(i);j++){
            double x,y;
            latlon_to_point(getFeaturePoint(j,i), x, y);
            temp.points.push_back(ezgl::point2d(x,y));
        }
        
        
        if(temp.closed){
            temp.area=0;
            int k=temp.points.size()-2;
            for(unsigned int j=0; j<temp.points.size()-1;j++){
                temp.area=temp.area+(temp.points[k].x+temp.points[j].x)*(temp.points[k].y-temp.points[j].y);
                k=j;
            }
            temp.area=temp.area/2.0;
            if(temp.area<0){
                temp.area=-temp.area;
            }
        }else{
            temp.area=-1;
        }
        g_m2_data->features.push_back(temp);
        
    }
    std::sort(g_m2_data->features.begin(), g_m2_data->features.end(), comp());
}

//this function clears the values for the clicked intersections
void press_clear(GtkWidget *, gpointer data){
    auto application = static_cast<ezgl::application *>(data);
    
    g_m2_data->clicked_intersection = {0.0,0.0};
    g_m2_data->clicked_intersection_2 = {0.0,0.0};    
    g_m2_data->clicked_on_an_Intersection_bool = false;
    g_m2_data->clicked_on_second_Intersection_bool = false;
    g_m2_data->directions="";
    

    GObject * dir=application->get_object("DirLabel");
    GtkLabel * dir_p=(GtkLabel *)dir;
  
    gtk_label_set_text(dir_p,g_m2_data->directions.c_str());
    //std::cout << "Successfully Clicked" << std::endl;
    
    
    application->refresh_drawing();

}

//This function draws a path on the map
//Path will be the return value from the function find_path_between_intersections
void draw_path_between_intersections(ezgl::renderer &g, std::vector<unsigned> path){
    //for clicked intersections
    LatLon intersection_position(y_to_lat(g_m2_data->clicked_intersection.y),x_to_lon(g_m2_data->clicked_intersection.x));
    LatLon intersection_position_2(y_to_lat(g_m2_data->clicked_intersection_2.y),x_to_lon(g_m2_data->clicked_intersection_2.x));
//    std::string main_canvas_id=g_m2_data->application->get_main_canvas_id();
//    auto canvas = g_m2_data->application->get_canvas(main_canvas_id);
//        ezgl::point2d point(0,0);
//        point.x=(g_m2_data->clicked_intersection.x+g_m2_data->clicked_intersection_2.x)/2.0;
//        point.y=(g_m2_data->clicked_intersection.y+g_m2_data->clicked_intersection_2.y)/2.0;
//        
//        ezgl::zoom_in(canvas,sqrt(canvas->get_camera().get_world().area()/(pow((3.0/5.0),20)*canvas->get_camera().get_initial_world().area())));
//        ezgl::translate(canvas,point.x-canvas->get_camera().get_world().center_x(), point.y-canvas->get_camera().get_world().center_y());
//        g_m2_data->application->refresh_drawing();
    //corner case
    if(path.size() == 0){
        g_m2_data->directions="";
        return;
    }
    if(path.size()>0){
        g_m2_data->directions="";
        int count = 1;
        //Goes through all path segments
        double distance =0;
        bool cont = false;
        for(unsigned int i=1; i<path.size();i++){
            if(cont == false){
                distance =0;
            }
            //Add segment travel time
            distance += find_street_segment_length(path[i-1]);
            
            //Add appropriate turn penalty
            if(find_turn_type(path[i-1],path[i])==TurnType::RIGHT){
                g_m2_data->directions += std::to_string(count)+". Head down "+getStreetName(getInfoStreetSegment(path[i-1]).streetID) + " for " +std::to_string(int(round(distance)))+ "m.\n";
                count ++;
                g_m2_data->directions += std::to_string(count) + ". Turn right onto " + getStreetName(getInfoStreetSegment(path[i]).streetID) + ".\n";
                count ++;
                cont = false;
            }else if(find_turn_type(path[i-1],path[i])==TurnType::LEFT){
                g_m2_data->directions += std::to_string(count)+". Head down "+getStreetName(getInfoStreetSegment(path[i-1]).streetID) + " for " +std::to_string(int(round(distance)))+ "m.\n";
                count ++;
                g_m2_data->directions += std::to_string(count) + ". " + "Turn left onto " + getStreetName(getInfoStreetSegment(path[i]).streetID) + ".\n";
                count ++;
                cont = false;
                
            }else{
                cont = true;
            }
        }
        distance += find_street_segment_length(path[path.size()-1]);
        g_m2_data->directions += std::to_string(count) + ". Head down "+getStreetName(getInfoStreetSegment(path[path.size()-1]).streetID) + " for " +std::to_string(int(round(distance)))+ "m.";
        
    }
    if(g_m2_data->zoom_level >= 7){
        for(unsigned int i = 0 ; i < path.size() ; i++){
            
            float id1, id2, x1, x2, y1, y2, x1_curve, x2_curve, y1_curve, y2_curve;

            id1 = getInfoStreetSegment(path[i]).from;
            id2 = getInfoStreetSegment(path[i]).to;


            x1 = lon_to_x(g_m2_data->intersections[id1].position.lon());
            x2 = lon_to_x(g_m2_data->intersections[id2].position.lon());

            y1 = lat_to_y(g_m2_data->intersections[id1].position.lat());
            y2 = lat_to_y(g_m2_data->intersections[id2].position.lat());

            //for drawing curved streets
            if(getInfoStreetSegment(path[i]).curvePointCount >= 1){

                double first_point_x = lon_to_x(getStreetSegmentCurvePoint(0,path[i]).lon());
                double second_point_x = lon_to_x(getStreetSegmentCurvePoint(getInfoStreetSegment(path[i]).curvePointCount-1,path[i]).lon());
                double first_point_y =  lat_to_y(getStreetSegmentCurvePoint(0,path[i]).lat());
                double second_point_y = lat_to_y(getStreetSegmentCurvePoint(getInfoStreetSegment(path[i]).curvePointCount-1,path[i]).lat());

                //draw path
                g.set_color(ezgl::color(0x7F, 0xD9, 0xF9));
                g.set_line_width(8);
                g.draw_line({x1, y1}, {first_point_x,first_point_y});
                g.draw_line({x2, y2}, {second_point_x,second_point_y});

                for(int j = 0 ; j < getInfoStreetSegment(path[i]).curvePointCount-1 ; j++){
                    x1_curve = lon_to_x(getStreetSegmentCurvePoint(j,path[i]).lon());
                    x2_curve = lon_to_x(getStreetSegmentCurvePoint(j+1,path[i]).lon());
                    y1_curve = lat_to_y(getStreetSegmentCurvePoint(j,path[i]).lat());
                    y2_curve = lat_to_y(getStreetSegmentCurvePoint(j+1,path[i]).lat());

                    //Draw path
                    g.set_color(ezgl::color(0x7F, 0xD9, 0xF9));
                    g.set_line_width(8);
                    g.draw_line({x1_curve, y1_curve}, {x2_curve, y2_curve});

                }

            }else{
                //Draw path
                g.set_color(ezgl::color(0x7F, 0xD9, 0xF9));
                g.set_line_width(8);
                g.draw_line({x1, y1}, {x2, y2});
            }
        }
        
        //redraw the text so that names are at the top
        if(g_m2_data->clicked_on_second_Intersection_bool == true){
            if(g_m2_data->night_mode_bool == true){
                g.set_color(ezgl::WHITE);
            }else{            
                g.set_color(ezgl::BLACK);
            }
            g.set_font_size(15);
            g.draw_text({g_m2_data->clicked_intersection.x,g_m2_data->clicked_intersection.y + TEXT_POSTITION_OFFSET}, g_m2_data->intersections[find_closest_intersection(intersection_position)].name, 100, 200);
            g.draw_text({g_m2_data->clicked_intersection_2.x,g_m2_data->clicked_intersection_2.y + TEXT_POSTITION_OFFSET}, g_m2_data->intersections[find_closest_intersection(intersection_position_2)].name, 100, 200);
            
        }
    }
}

//draws the features onto the map
//this is a function called inside draw_main_canvas
void draw_feature(int check, ezgl::renderer &g){
    //Set two sets of color: one for regular mode, one for night mode
    ezgl::color BACKGROUND(0,0,0);
    ezgl::color WATER(0,0,0);
    ezgl::color BUILDING(0,0,0);
    ezgl::color SAND(0,0,0);
    ezgl::color GRASS(0,0,0);
    ezgl::color STREET(0,0,0);
    ezgl::color HIGHWAY(0,0,0);
    ezgl::color HIGHWAYBORDER(0,0,0);
    ezgl::color STREETBORDER(0, 0, 0);

    //night mode color palette
    if(g_m2_data->night_mode_bool){
        BACKGROUND=ezgl::N_BACKGROUND;
        WATER=ezgl::N_WATER;
        BUILDING=ezgl::N_BUILDING;
        SAND=ezgl::N_SAND;
        GRASS=ezgl::N_GRASS;
        STREET=ezgl::N_STREET;
        HIGHWAY=ezgl::N_HIGHWAY;
        HIGHWAYBORDER=ezgl::N_HIGHWAYBORDER;
        STREETBORDER=ezgl::N_STREETBORDER;
    }else{
        //regular mode color palette
        BACKGROUND=ezgl::D_BACKGROUND;
        WATER=ezgl::D_WATER;
        BUILDING=ezgl::D_BUILDING;
        SAND=ezgl::D_SAND;
        GRASS=ezgl::D_GRASS;
        STREET=ezgl::D_STREET;
        HIGHWAY=ezgl::D_HIGHWAY;
        HIGHWAYBORDER=ezgl::D_HIGHWAYBORDER;
        STREETBORDER=ezgl::D_STREETBORDER;
    }
    
    //setting colors and draw features
    for(unsigned int i=0; i<g_m2_data->features.size(); i++){
        if(g_m2_data->features[i].feature_type==Lake){
            g.set_color(WATER); 
        }
        if(g_m2_data->features[i].feature_type==Island){
            g.set_color(BACKGROUND);
        }
        if(g_m2_data->features[i].feature_type==Park){
            g.set_color(GRASS);
        }
        if(g_m2_data->features[i].feature_type==Greenspace){
            g.set_color(GRASS);
        }
        if(g_m2_data->features[i].feature_type==Golfcourse){
            g.set_color(GRASS);
        }
        if(g_m2_data->features[i].feature_type==Beach){
            g.set_color(SAND);
        }if(g_m2_data->features[i].feature_type==Building){
            g.set_color(BUILDING);
        }
        if(g_m2_data->features[i].feature_type==River){
            g.set_color(WATER);
            g.set_line_width(4);
        }
        if(g_m2_data->features[i].feature_type==Stream){
            g.set_color(WATER);
            g.set_line_width(2);
        }
        if(!(g_m2_data->features[i].feature_type==Building&&check<2)){
            
            //Draw depending on if data is polygon, line, or point
            if(g_m2_data->features[i].points.size()>1){
                if(g_m2_data->features[i].closed){
                    g.fill_poly(g_m2_data->features[i].points);
                }else if(check>=2){
                    for(unsigned int j=0; j<g_m2_data->features[i].points.size()-1;j++)
                        g.draw_line(g_m2_data->features[i].points[j], g_m2_data->features[i].points[j+1]);
                }
            }
        }
    }
}

//draw the street segments onto the map
//this is a function called inside draw_main_canvas
void draw_street_segments(int check, ezgl::renderer &g){
     //Set two sets of color: one for regular mode, one for night mode
    ezgl::color BACKGROUND(0,0,0);
    ezgl::color WATER(0,0,0);
    ezgl::color BUILDING(0,0,0);
    ezgl::color SAND(0,0,0);
    ezgl::color GRASS(0,0,0);
    ezgl::color STREET(0,0,0);
    ezgl::color HIGHWAY(0,0,0);
    ezgl::color HIGHWAYBORDER(0,0,0);
    ezgl::color STREETBORDER(0, 0, 0);

    //night mode color palette
    if(g_m2_data->night_mode_bool){
        BACKGROUND=ezgl::N_BACKGROUND;
        WATER=ezgl::N_WATER;
        BUILDING=ezgl::N_BUILDING;
        SAND=ezgl::N_SAND;
        GRASS=ezgl::N_GRASS;
        STREET=ezgl::N_STREET;
        HIGHWAY=ezgl::N_HIGHWAY;
        HIGHWAYBORDER=ezgl::N_HIGHWAYBORDER;
        STREETBORDER=ezgl::N_STREETBORDER;
    }else{
        //regular mode color palette
        BACKGROUND=ezgl::D_BACKGROUND;
        WATER=ezgl::D_WATER;
        BUILDING=ezgl::D_BUILDING;
        SAND=ezgl::D_SAND;
        GRASS=ezgl::D_GRASS;
        STREET=ezgl::D_STREET;
        HIGHWAY=ezgl::D_HIGHWAY;
        HIGHWAYBORDER=ezgl::D_HIGHWAYBORDER;
        STREETBORDER=ezgl::D_STREETBORDER;
    }
    //draw street segments borders and set color and width
    //width of the segments are based on dynamic_width sizing function
    g.set_line_cap(ezgl::line_cap::round);
    for(unsigned int i=0; i < g_m2_data->segments.size(); i++){
        int border_size;
        bool draw=false;

        //major roads are drawn and scaled differently from minor roads
        if((g_m2_data->segments[i].highway=="primary"||g_m2_data->segments[i].highway=="primary_link")){

            border_size=7 + dynamic_width_large_roads();
            draw=check>=1;
        }else if((g_m2_data->segments[i].highway=="secondary"||g_m2_data->segments[i].highway=="secondary_link")){
            border_size=6 + dynamic_width_large_roads();
            draw=check>=1;
        }else if((g_m2_data->segments[i].highway=="tertiary"||g_m2_data->segments[i].highway=="tertiary_link")){
            border_size=5 + dynamic_width_small_roads();
            draw=check>=2;
        }else if (!(g_m2_data->segments[i].highway=="motorway"||g_m2_data->segments[i].highway=="motorway_link"
                 ||g_m2_data->segments[i].highway=="trunk"||g_m2_data->segments[i].highway=="trunk_link")){
            border_size=4 + dynamic_width_small_roads();
            draw=check>=2;
        }
        if(draw)
        for(unsigned int j=0; j<g_m2_data->segments[i].x.size()-1;j++){
            g.set_color(STREETBORDER);
            g.set_line_width(border_size);
            g.draw_line({g_m2_data->segments[i].x[j],g_m2_data->segments[i].y[j]}, {g_m2_data->segments[i].x[j+1], g_m2_data->segments[i].y[j+1]});
        }
    }

    //draw highway borders and set color and width
    //width of the segments are based on dynamic_width sizing function
    for(unsigned int i=0; i<g_m2_data->segments.size();i++){
        if(g_m2_data->segments[i].highway=="motorway"||g_m2_data->segments[i].highway=="motorway_link"
           ||g_m2_data->segments[i].highway=="trunk"||g_m2_data->segments[i].highway=="trunk_link"){  
            g.set_color(HIGHWAYBORDER);
            g.set_line_width(8 + dynamic_width_large_roads());
            for(unsigned int j=0; j<g_m2_data->segments[i].x.size()-1;j++){
            g.draw_line({g_m2_data->segments[i].x[j],g_m2_data->segments[i].y[j]}, {g_m2_data->segments[i].x[j+1], g_m2_data->segments[i].y[j+1]}); 
            }
        }
    }
    
    //draw street segments and set color and width
    //width of the segments are based on dynamic_width sizing function
    for(unsigned int i=0; i < g_m2_data->segments.size(); i++){
        int  street_size;
        bool draw=false;
        if((g_m2_data->segments[i].highway=="primary"||g_m2_data->segments[i].highway=="primary_link")){
            street_size=5 + dynamic_width_large_roads();
            draw=check>=1;
        }else if((g_m2_data->segments[i].highway=="secondary"||g_m2_data->segments[i].highway=="secondary_link")){
            street_size=4 + dynamic_width_large_roads();
            draw=check>=1;
        }else if((g_m2_data->segments[i].highway=="tertiary"||g_m2_data->segments[i].highway=="tertiary_link")){
            street_size=3 + dynamic_width_small_roads();
            draw=check>=2;
        }else if (!(g_m2_data->segments[i].highway=="motorway"||g_m2_data->segments[i].highway=="motorway_link"
                 ||g_m2_data->segments[i].highway=="trunk"||g_m2_data->segments[i].highway=="trunk_link")){
            street_size=2 + dynamic_width_small_roads();
            draw=check>=2;
        }
        if(draw){
            for(unsigned int j=0; j<g_m2_data->segments[i].x.size()-1;j++){
                g.set_color(STREET);
                g.set_line_width(street_size);
                g.draw_line({g_m2_data->segments[i].x[j],g_m2_data->segments[i].y[j]}, {g_m2_data->segments[i].x[j+1], g_m2_data->segments[i].y[j+1]});
            }
        }
    }
}

//labels the street names
//texts are labeled according to segment size
//this is a function called inside draw_main_canvas
void label_street_names(int check, ezgl::renderer &g){
     //Set two sets of color: one for regular mode, one for night mode
    ezgl::color BACKGROUND(0,0,0);
    ezgl::color WATER(0,0,0);
    ezgl::color BUILDING(0,0,0);
    ezgl::color SAND(0,0,0);
    ezgl::color GRASS(0,0,0);
    ezgl::color STREET(0,0,0);
    ezgl::color HIGHWAY(0,0,0);
    ezgl::color HIGHWAYBORDER(0,0,0);
    ezgl::color STREETBORDER(0, 0, 0);

    //night mode color palette
    if(g_m2_data->night_mode_bool){
        BACKGROUND=ezgl::N_BACKGROUND;
        WATER=ezgl::N_WATER;
        BUILDING=ezgl::N_BUILDING;
        SAND=ezgl::N_SAND;
        GRASS=ezgl::N_GRASS;
        STREET=ezgl::N_STREET;
        HIGHWAY=ezgl::N_HIGHWAY;
        HIGHWAYBORDER=ezgl::N_HIGHWAYBORDER;
        STREETBORDER=ezgl::N_STREETBORDER;
    }else{
        //regular mode color palette
        BACKGROUND=ezgl::D_BACKGROUND;
        WATER=ezgl::D_WATER;
        BUILDING=ezgl::D_BUILDING;
        SAND=ezgl::D_SAND;
        GRASS=ezgl::D_GRASS;
        STREET=ezgl::D_STREET;
        HIGHWAY=ezgl::D_HIGHWAY;
        HIGHWAYBORDER=ezgl::D_HIGHWAYBORDER;
        STREETBORDER=ezgl::D_STREETBORDER;
    }
    
    //labeling street names
    //prints out the text with size that are inside each street segment
    //arrows "-> "points in the direction of the segments
    for(unsigned int i=0; i < g_m2_data->segments.size(); i++){
        int  street_size;
        bool draw=false;
        if((g_m2_data->segments[i].highway=="primary"||g_m2_data->segments[i].highway=="primary_link")){
            street_size=5 + dynamic_width_large_roads();
            draw=check>=1;
        }else if((g_m2_data->segments[i].highway=="secondary"||g_m2_data->segments[i].highway=="secondary_link")){
            street_size=4 + dynamic_width_large_roads();
            draw=check>=1;
        }else if((g_m2_data->segments[i].highway=="tertiary"||g_m2_data->segments[i].highway=="tertiary_link")){
            street_size=3 + dynamic_width_small_roads();
            draw=check>=2;
        }else if (!(g_m2_data->segments[i].highway=="motorway"||g_m2_data->segments[i].highway=="motorway_link" ||g_m2_data->segments[i].highway=="trunk"||g_m2_data->segments[i].highway=="trunk_link")){
            street_size=2 + dynamic_width_small_roads();
            draw=check>=2;
        }
        if(draw){
            g.set_font_size(street_size);
            for(unsigned int j=0; j<g_m2_data->segments[i].x.size()-1;j++){

                if(g_m2_data->segments[i].one_way){
                    g.set_color(BUILDING);
                    g.set_text_rotation(g_m2_data->segments[i].angle[j]*180/M_PI);
                    g.draw_text({(g_m2_data->segments[i].x[j]+g_m2_data->segments[i].x[j+1])/2.0,(g_m2_data->segments[i].y[j]+g_m2_data->segments[i].y[j+1])/2},"->", g_m2_data->segments[i].length[j], street_size);
                }

                if(g_m2_data->night_mode_bool){
                    g.set_color(ezgl::WHITE);
                }else{
                    g.set_color(ezgl::BLACK);
                }

                if(g_m2_data->segments[i].angle[j]>M_PI/2.0){
                    g.set_text_rotation((g_m2_data->segments[i].angle[j]-M_PI)*180/M_PI);
                }else if(g_m2_data->segments[i].angle[j]<-M_PI/2.0){
                    g.set_text_rotation((g_m2_data->segments[i].angle[j]+M_PI)*180/M_PI);
                }else
                    g.set_text_rotation(g_m2_data->segments[i].angle[j]*180/M_PI);
                g.draw_text({(g_m2_data->segments[i].x[j]+g_m2_data->segments[i].x[j+1])/2.0,(g_m2_data->segments[i].y[j]+g_m2_data->segments[i].y[j+1])/2.0},g_m2_data->segments[i].name,g_m2_data->segments[i].length[j],street_size);
            }
        }
    }
}

//draws the highways
//this is a function called inside draw_main_canvas
void draw_highway_segments(ezgl::renderer &g){
     //Set two sets of color: one for regular mode, one for night mode
    ezgl::color BACKGROUND(0,0,0);
    ezgl::color WATER(0,0,0);
    ezgl::color BUILDING(0,0,0);
    ezgl::color SAND(0,0,0);
    ezgl::color GRASS(0,0,0);
    ezgl::color STREET(0,0,0);
    ezgl::color HIGHWAY(0,0,0);
    ezgl::color HIGHWAYBORDER(0,0,0);
    ezgl::color STREETBORDER(0, 0, 0);

    //night mode color palette
    if(g_m2_data->night_mode_bool){
        BACKGROUND=ezgl::N_BACKGROUND;
        WATER=ezgl::N_WATER;
        BUILDING=ezgl::N_BUILDING;
        SAND=ezgl::N_SAND;
        GRASS=ezgl::N_GRASS;
        STREET=ezgl::N_STREET;
        HIGHWAY=ezgl::N_HIGHWAY;
        HIGHWAYBORDER=ezgl::N_HIGHWAYBORDER;
        STREETBORDER=ezgl::N_STREETBORDER;
    }else{
        //regular mode color palette
        BACKGROUND=ezgl::D_BACKGROUND;
        WATER=ezgl::D_WATER;
        BUILDING=ezgl::D_BUILDING;
        SAND=ezgl::D_SAND;
        GRASS=ezgl::D_GRASS;
        STREET=ezgl::D_STREET;
        HIGHWAY=ezgl::D_HIGHWAY;
        HIGHWAYBORDER=ezgl::D_HIGHWAYBORDER;
        STREETBORDER=ezgl::D_STREETBORDER;
    }
    //draw highway segments and set color and width
    //width of the segments are based on dynamic_width sizing function
    for(unsigned int i=0; i<g_m2_data->segments.size();i++){
        g.set_color(HIGHWAY);

        if(g_m2_data->segments[i].highway=="motorway"||g_m2_data->segments[i].highway=="motorway_link"||g_m2_data->segments[i].highway=="trunk"||g_m2_data->segments[i].highway=="trunk_link"){   
            double width=6+ dynamic_width_large_roads();
            g.set_line_width(width);
            for(unsigned int j=0; j<g_m2_data->segments[i].x.size()-1;j++){
                g.draw_line({g_m2_data->segments[i].x[j],g_m2_data->segments[i].y[j]}, {g_m2_data->segments[i].x[j+1], g_m2_data->segments[i].y[j+1]});
            }
        }
    }
}

//labels the names of the highways drawn
//texts are drawn base according to the size of the segment
//this is a function called inside draw_main_canvas
void label_highway_names(ezgl::renderer &g){
     //Set two sets of color: one for regular mode, one for night mode
    ezgl::color BACKGROUND(0,0,0);
    ezgl::color WATER(0,0,0);
    ezgl::color BUILDING(0,0,0);
    ezgl::color SAND(0,0,0);
    ezgl::color GRASS(0,0,0);
    ezgl::color STREET(0,0,0);
    ezgl::color HIGHWAY(0,0,0);
    ezgl::color HIGHWAYBORDER(0,0,0);
    ezgl::color STREETBORDER(0, 0, 0);

    //night mode color palette
    if(g_m2_data->night_mode_bool){
        BACKGROUND=ezgl::N_BACKGROUND;
        WATER=ezgl::N_WATER;
        BUILDING=ezgl::N_BUILDING;
        SAND=ezgl::N_SAND;
        GRASS=ezgl::N_GRASS;
        STREET=ezgl::N_STREET;
        HIGHWAY=ezgl::N_HIGHWAY;
        HIGHWAYBORDER=ezgl::N_HIGHWAYBORDER;
        STREETBORDER=ezgl::N_STREETBORDER;
    }else{
        //regular mode color palette
        BACKGROUND=ezgl::D_BACKGROUND;
        WATER=ezgl::D_WATER;
        BUILDING=ezgl::D_BUILDING;
        SAND=ezgl::D_SAND;
        GRASS=ezgl::D_GRASS;
        STREET=ezgl::D_STREET;
        HIGHWAY=ezgl::D_HIGHWAY;
        HIGHWAYBORDER=ezgl::D_HIGHWAYBORDER;
        STREETBORDER=ezgl::D_STREETBORDER;
    }
    //labeling highway names
    //prints out the text with size that are inside each street segment
    //arrows "-> "points in the direction of the segments
    for(unsigned int i=0; i<g_m2_data->segments.size();i++){
        if(g_m2_data->segments[i].highway=="motorway"||g_m2_data->segments[i].highway=="motorway_link"||g_m2_data->segments[i].highway=="trunk"||g_m2_data->segments[i].highway=="trunk_link"){  
            double width=6+ dynamic_width_large_roads();
            g.set_font_size(width);
            for(unsigned int j=0; j<g_m2_data->segments[i].x.size()-1;j++){
            if(g_m2_data->segments[i].one_way){
                g.set_color(BUILDING);
                g.set_text_rotation(g_m2_data->segments[i].angle[j]*180/M_PI);
                g.draw_text({(g_m2_data->segments[i].x[j]+g_m2_data->segments[i].x[j+1])/2.0,(g_m2_data->segments[i].y[j]+g_m2_data->segments[i].y[j+1])/2},"->", g_m2_data->segments[i].length[j], width);
            }
            
            if(g_m2_data->night_mode_bool){
                g.set_color(ezgl::WHITE);
            }else{
                g.set_color(ezgl::BLACK);
            }
                
            if(g_m2_data->segments[i].angle[j]>M_PI/2.0){
                g.set_text_rotation((g_m2_data->segments[i].angle[j]-M_PI)*180/M_PI);
            }else if(g_m2_data->segments[i].angle[j]<-M_PI/2.0){
                g.set_text_rotation((g_m2_data->segments[i].angle[j]+M_PI)*180/M_PI);
            }else
                g.set_text_rotation(g_m2_data->segments[i].angle[j]*180/M_PI);
            g.draw_text({(g_m2_data->segments[i].x[j]+g_m2_data->segments[i].x[j+1])/2.0,(g_m2_data->segments[i].y[j]+g_m2_data->segments[i].y[j+1])/2.0},g_m2_data->segments[i].name,g_m2_data->segments[i].length[j],width);
            }
        }
    }
}

//draws the subway line
//this is a function called inside draw_main_canvas
void draw_subway_line(int check, ezgl::renderer &g){
    //Draw subway lines
    if(g_m2_data->POIChecks.station&&check>=1){
        if(g_m2_data->night_mode_bool){
            g.set_color(ezgl::DARK_SLATE_BLUE);
        }else{
            g.set_color(ezgl::BLUE);
        }
        g.set_line_width(4);
        g.set_line_cap(ezgl::line_cap::round);
        for(unsigned int i = 0; i<g_m2_data->subways.size(); i++){
            for(unsigned int j =0; j<g_m2_data->subways[i].size()-1;j++){
                g.draw_line(g_m2_data->subways[i][j], g_m2_data->subways[i][j+1]);
            }
        }
    }
}

//draw the POIs onto the map
//this is a function called inside draw_main_canvas
void draw_POIs(int check, ezgl::renderer &g){

    //drawing POI icons
    //icons will be drawn base on zoom area
    ezgl::surface *hospital_png = ezgl::renderer::load_png("libstreetmap/resources/hospital.png");
    ezgl::surface *cafe_png = ezgl::renderer::load_png("libstreetmap/resources/cafe.png");
    ezgl::surface *restaurant_png = ezgl::renderer::load_png("libstreetmap/resources/restaurant.png");
    ezgl::surface *bank_png = ezgl::renderer::load_png("libstreetmap/resources/bank.png");
    ezgl::surface *gas_png = ezgl::renderer::load_png("libstreetmap/resources/gas.png");
    ezgl::surface *subway_png = ezgl::renderer::load_png("libstreetmap/resources/subway.png");
    
    if(check>=1)
    for(unsigned int i = 0; i < g_m2_data->POIs.size(); i++){

        bool draw = false;
        
        if(g_m2_data->POIChecks.POI){
            g.set_color(ezgl::RED);
            g.fill_arc(g_m2_data->POIs[i].location, INTERSECTION_DOT_RADIUS,0,360);
            draw = true;
        }
        
        if(g_m2_data->POIs[i].type=="hospital"&&g_m2_data->POIChecks.hospital){
            g.draw_surface(hospital_png, g_m2_data->POIs[i].location);
            draw = true;
        }
        
        if(g_m2_data->POIs[i].type=="cafe"&&g_m2_data->POIChecks.cafe){
            g.draw_surface(cafe_png, g_m2_data->POIs[i].location);
            draw = true;
        }
        
        if(g_m2_data->POIs[i].type=="restaurant"&&g_m2_data->POIChecks.restaurant){
            g.draw_surface(restaurant_png, g_m2_data->POIs[i].location);
            draw = true;
        }
        
        if(g_m2_data->POIs[i].type=="bank"&&g_m2_data->POIChecks.bank){
            g.draw_surface(bank_png, g_m2_data->POIs[i].location);
            draw = true;
        }
        
        if(g_m2_data->POIs[i].type=="fuel"&&g_m2_data->POIChecks.gas){
            g.draw_surface(gas_png, g_m2_data->POIs[i].location);
            draw = true;
        }
        
        if(g_m2_data->POIs[i].type=="station"&&g_m2_data->POIChecks.station){
            g.draw_surface(subway_png, g_m2_data->POIs[i].location);
            draw = true;
        }
        
        if(check>=3&&draw){
            if(g_m2_data->night_mode_bool){
                g.set_color(ezgl::WHITE);
            }else{
                g.set_color(ezgl::BLACK);
            }
            g.set_text_rotation(0);
            g.set_font_size(15);
            g.draw_text(g_m2_data->POIs[i].location,g_m2_data->POIs[i].name);
        }
    }

    ezgl::renderer::free_surface(hospital_png);
    ezgl::renderer::free_surface(cafe_png);
    ezgl::renderer::free_surface(restaurant_png);
    ezgl::renderer::free_surface(bank_png);
    ezgl::renderer::free_surface(gas_png);
    ezgl::renderer::free_surface(subway_png);
    
}