/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef _IMAP_MAIN_H
#define _IMAP_MAIN_H

#include "gtk/gtk.h"

#include "imap_mru.h"
#include "imap_object.h"
#include "imap_preferences.h"
#include "imap_preview.h"

typedef enum {NCSA, CERN, CSIM} MapFormat_t;

typedef struct {
   MapFormat_t map_format;
   gchar *image_name;
   gchar *title;
   gchar *author;
   gchar *default_url;
   gchar *description;
   gint   old_image_width;
   gint   old_image_height;
   gboolean color;		/* Color (TRUE) or Gray (FALSE) */
   gboolean show_gray;
} MapInfo_t;

void main_set_dimension(gint width, gint height);
void main_clear_dimension(void);
void load(const gchar *filename);
void save_as(const gchar *filename);
void dump_output(gpointer param, OutputFunc_t output);
GdkColor *get_yellow(void);
MRU_t *get_mru(void);
MapInfo_t *get_map_info(void);
PreferencesData_t *get_preferences(void);
GtkWidget *get_top_widget(void);
gint get_image_width(void);
gint get_image_height(void);

void set_busy_cursor(void);
void remove_busy_cursor(void);

void main_toolbar_set_grid(gboolean active);

void set_zoom(gint zoom_factor);
gint get_real_coord(gint coord);
void draw_line(GdkWindow *window, GdkGC *gc, gint x1, gint y1, gint x2, 
	       gint y2);
void draw_rectangle(GdkWindow *window, GdkGC *gc, gint filled, gint x, gint y,
		    gint width, gint height);
void draw_arc(GdkWindow *window, GdkGC *gc, gint filled, gint x, gint y,
	      gint width, gint height, gint angle1, gint angle2);
void draw_circle(GdkWindow *window, GdkGC *gc, gint filled, gint x, gint y, 
		 gint r);
void draw_polygon(GdkWindow *window, GdkGC *gc, GList *list);

const char *get_filename(void);

void set_arrow_func(void);
void set_rectangle_func(void);
void set_circle_func(void);
void set_polygon_func(void);

ObjectList_t *get_shapes(void);
void add_shape(Object_t *obj);
void update_shape(Object_t *obj);
void select_shape(GtkWidget *widget, GdkEventButton *event);
void edit_shape(gint x, gint y);

void do_popup_menu(GdkEventButton *event);
void draw_shapes(GtkWidget *preview);
void redraw_preview(void);

void show_url(void);
void hide_url(void);

#endif /* _IMAP_MAIN_H */
