#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

typedef struct GUI GUI;
typedef void(* GUI_Cb)(GUI*);

/* The user must not manipulate this struct directly */
struct GUI {
  void* data[12];
  GUI_Cb destroyer;
  char hex[65];
  char base64[45];
  int is_first;
};

void gui_init(GUI* gui);
void gui_destroy(GUI* gui);

GtkApplication* gui_get_gtk_app(GUI* gui);

#endif
