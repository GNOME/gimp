/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */
#ifndef __SESSION_H__
#define __SESSION_H__

#include <glib.h>

/* Structures */
typedef struct _SessionGeometry SessionGeometry;

struct _SessionGeometry
{
  char name[16]; 
  int x;
  int y;
  int width;
  int height;
};

/*  global session variables  */
extern SessionGeometry toolbox_geometry;
extern SessionGeometry lc_dialog_geometry;
extern SessionGeometry info_dialog_geometry;
extern SessionGeometry tool_options_geometry;
extern SessionGeometry palette_geometry;
extern SessionGeometry brush_select_geometry;
extern SessionGeometry pattern_select_geometry;
extern SessionGeometry gradient_editor_geometry;

extern GList *session_geometry_updates;  /* This list holds all geometries
					    that should be written to the
					    sessionrc on exit.             */  

/*  function prototypes  */
void session_get_window_geometry (GtkWidget       *window, 
				  SessionGeometry *geometry);
void session_set_window_geometry (GtkWidget       *window,
				  SessionGeometry *geometry,
				  int              set_size);
void session_init (void);
void save_sessionrc (void);

#endif  /*  __SESSION_H__  */
