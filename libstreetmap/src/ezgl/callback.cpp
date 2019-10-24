#include "ezgl/callback.hpp"
#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include <string>
#include <graphics.hpp>

namespace ezgl {

// File wide static variables to track whether the middle mouse
// button is currently pressed AND the old x and y positions of the mouse pointer
bool left_mouse_button_pressed = false;
int last_panning_event_time = 0;
double prev_x = 0, prev_y = 0;
double prev_xx = 0, prev_yy =0;

int g_street1_id = -1;
int g_street2_id = -1;

bool g_intersections_found = false;

std::vector<unsigned> g_intersection_ids;


gboolean press_key(GtkWidget *, GdkEventKey *event, gpointer data)
{
  auto application = static_cast<ezgl::application *>(data);

  // Call the user-defined key press callback if defined
  if(application->key_press_callback != nullptr) {
    // see: https://developer.gnome.org/gdk3/stable/gdk3-Keyboard-Handling.html
    application->key_press_callback(application, event, gdk_keyval_name(event->keyval));
  }

  return FALSE; // propagate the event
}

gboolean press_mouse(GtkWidget *, GdkEventButton *event, gpointer )
{
  

  if(event->type == GDK_BUTTON_PRESS) {

    // Check for Middle mouse press to support dragging
    if(event->button == 1) {
      left_mouse_button_pressed = true;
      prev_x = event->x;
      prev_y = event->y;
      prev_xx = event->x;
      prev_yy = event->y;
    }

    
  }

  return TRUE; // consume the event
}

gboolean release_mouse(GtkWidget *, GdkEventButton *event, gpointer data)
{
    auto application = static_cast<ezgl::application *>(data);
  if(event->type == GDK_BUTTON_RELEASE) {
    // Check for Middle mouse release to support dragging
    if(event->button == 1) {
      left_mouse_button_pressed = false;
    }
    
    // Call the user-defined mouse press callback if defined
    if(application->mouse_press_callback != nullptr) {
      ezgl::point2d const widget_coordinates(event->x, event->y);
      if(widget_coordinates.x==prev_xx&&widget_coordinates.y==prev_yy){
      std::string main_canvas_id = application->get_main_canvas_id();
      ezgl::canvas *canvas = application->get_canvas(main_canvas_id);

      ezgl::point2d const world = canvas->get_camera().widget_to_world(widget_coordinates);
      application->mouse_press_callback(application, event, world.x, world.y);}
    }
  }

  return TRUE; // consume the event
}

gboolean move_mouse(GtkWidget *, GdkEventButton *event, gpointer data)
{
  auto application = static_cast<ezgl::application *>(data);

  if(event->type == GDK_MOTION_NOTIFY) {

    // Check if the middle mouse is pressed to support dragging
    if(left_mouse_button_pressed) {
      // drop this panning event if we have just served another one
      if(gtk_get_current_event_time() - last_panning_event_time < 100)
        return true;

      last_panning_event_time = gtk_get_current_event_time();

      GdkEventMotion *motion_event = (GdkEventMotion *)event;

      std::string main_canvas_id = application->get_main_canvas_id();
      auto canvas = application->get_canvas(main_canvas_id);

      point2d curr_trans = canvas->get_camera().widget_to_world({motion_event->x, motion_event->y});
      point2d prev_trans = canvas->get_camera().widget_to_world({prev_x, prev_y});

      double dx = curr_trans.x - prev_trans.x;
      double dy = curr_trans.y - prev_trans.y;

      prev_x = motion_event->x;
      prev_y = motion_event->y;
      if(canvas->get_camera().get_world().m_first.x-dx<canvas->get_camera().get_initial_world().m_first.x){
          dx = canvas->get_camera().get_world().m_first.x-canvas->get_camera().get_initial_world().m_first.x;
      }
      if(canvas->get_camera().get_world().m_first.y-dy<canvas->get_camera().get_initial_world().m_first.y){
          dy = canvas->get_camera().get_world().m_first.y-canvas->get_camera().get_initial_world().m_first.y;
      }
      if(canvas->get_camera().get_world().m_second.x-dx>canvas->get_camera().get_initial_world().m_second.x){
          dx = canvas->get_camera().get_world().m_second.x-canvas->get_camera().get_initial_world().m_second.x;
      }
      if(canvas->get_camera().get_world().m_second.y-dy>canvas->get_camera().get_initial_world().m_second.y){
          dy = canvas->get_camera().get_world().m_second.y-canvas->get_camera().get_initial_world().m_second.y;
      }
      // Flip the delta x to avoid inverted dragging
      translate(canvas, -dx, -dy);
    }
    // Else call the user-defined mouse move callback if defined
    else if(application->mouse_move_callback != nullptr) {
      ezgl::point2d const widget_coordinates(event->x, event->y);

      std::string main_canvas_id = application->get_main_canvas_id();
      ezgl::canvas *canvas = application->get_canvas(main_canvas_id);

      ezgl::point2d const world = canvas->get_camera().widget_to_world(widget_coordinates);
      application->mouse_move_callback(application, event, world.x, world.y);
    }
  }

  return TRUE; // consume the event
}

gboolean scroll_mouse(GtkWidget *, GdkEvent *event, gpointer data)
{

  if(event->type == GDK_SCROLL) {
    auto application = static_cast<ezgl::application *>(data);

    std::string main_canvas_id = application->get_main_canvas_id();
    auto canvas = application->get_canvas(main_canvas_id);

    GdkEventScroll *scroll_event = (GdkEventScroll *)event;

    ezgl::point2d scroll_point(scroll_event->x, scroll_event->y);

    if(scroll_event->direction == GDK_SCROLL_UP) {
      // Zoom in at the scroll point
        double scale = 5.0/3.0;
        if(canvas->get_camera().get_world().area()/scale/scale>pow((3.0/5.0),20)*canvas->get_camera().get_initial_world().area()){
        ezgl::zoom_in(canvas, scroll_point, scale);
        }else{
            ezgl::zoom_in(canvas, scroll_point, sqrt(canvas->get_camera().get_world().area()/(pow((3.0/5.0),20)*canvas->get_camera().get_initial_world().area())));
        }
    } else if(scroll_event->direction == GDK_SCROLL_DOWN) {
      // Zoom out at the scroll point
        double scale = 5.0/3.0;
        if(canvas->get_camera().get_world().area()*scale*scale<canvas->get_camera().get_initial_world().area()){
        ezgl::zoom_out(canvas, scroll_point, scale);
        }else{
            ezgl::zoom_out(canvas, scroll_point, sqrt(canvas->get_camera().get_initial_world().area()/canvas->get_camera().get_world().area()));
        }
    } else if(scroll_event->direction == GDK_SCROLL_SMOOTH) {
      // Doesn't seem to be happening
    } // NOTE: We ignore scroll GDK_SCROLL_LEFT and GDK_SCROLL_RIGHT
  }
  return TRUE;
}

gboolean press_zoom_fit(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::zoom_fit(canvas, canvas->get_camera().get_initial_world());

  return TRUE;
}

gboolean press_zoom_in(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);
  double scale = 5.0/3.0;
  if(canvas->get_camera().get_world().area()/(scale*scale)>pow((3.0/5.0),20)*canvas->get_camera().get_initial_world().area()){
    ezgl::zoom_in(canvas, 5.0 / 3.0);
  }else{
      ezgl::zoom_in(canvas, sqrt(canvas->get_camera().get_world().area()/(pow((3.0/5.0),20)*canvas->get_camera().get_initial_world().area())));
  }
  return TRUE;
}

gboolean press_zoom_out(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);
  double scale = 5.0/3.0;
  if(canvas->get_camera().get_world().area()*scale*scale<canvas->get_camera().get_initial_world().area()){
    ezgl::zoom_out(canvas, scale);
  }else{
      ezgl::zoom_out(canvas, sqrt(canvas->get_camera().get_initial_world().area()/canvas->get_camera().get_world().area()));
  }
  return TRUE;
}
/*
gboolean press_up(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_up(canvas, 5.0);

  return TRUE;
}

gboolean press_down(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_down(canvas, 5.0);

  return TRUE;
}

gboolean press_left(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_left(canvas, 5.0);

  return TRUE;
}

gboolean press_right(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  std::string main_canvas_id = application->get_main_canvas_id();
  auto canvas = application->get_canvas(main_canvas_id);

  ezgl::translate_right(canvas, 5.0);

  return TRUE;
}
*/
gboolean press_proceed(GtkWidget *, gpointer data)
{
  auto ezgl_app = static_cast<ezgl::application *>(data);
  ezgl_app->quit();

  return TRUE;
}


gboolean press_menu(GtkWidget *, gpointer data)
{

  auto application = static_cast<ezgl::application *>(data);

  GObject *menu_rev = application->get_object("MenuReveal");
  GtkRevealer * menu_rev_p=(GtkRevealer *)menu_rev;
  
  gtk_revealer_set_reveal_child(menu_rev_p, !gtk_revealer_get_reveal_child(menu_rev_p));
  GObject *img;
  if(gtk_revealer_get_reveal_child(menu_rev_p)){
      img = application->get_object("Hide");
  }else{
      img = application->get_object("Reveal");
  }
  
  GtkImage * img_p=(GtkImage *)img;
  GObject *menu_but = application->get_object("MenuButton");
  GtkButton * menu_but_p=(GtkButton *)menu_but;
  gtk_button_set_image(menu_but_p,GTK_WIDGET(img_p));
  return TRUE;
}


}
