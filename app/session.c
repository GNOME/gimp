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


/* Session-managment stuff

   I include a short description here on what is done and what problems 
   are left.

   Right now session-managment is limited to window geometry. I plan to add 
   at least the saving of Last-Used-Images (using nuke's patch).

   There is a problem with the offset introduced by the window-manager adding
   decorations to the windows. This is annoying and should be fixed somehow.
   
   Still not sure how to implement stuff for opening windows on start-up.
   Probably the best thing to do, would be to have a list of dialogs in
   the preferences-dialog that should always be opened.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "appenv.h"
#include "gimprc.h"
#include "session.h"

static void sessionrc_write_geometry (SessionGeometry *, FILE *);

GList *session_geometry_updates = NULL;

/* global session variables */
SessionGeometry toolbox_geometry = { "toolbox", 0, 0, 0, 0 };
SessionGeometry lc_dialog_geometry = { "lc-dialog", 0, 400, 0, 0 };
SessionGeometry info_dialog_geometry = { "info-dialog", 165, 0, 0, 0 };
SessionGeometry tool_options_geometry = { "tool-options", 0, 345, 0, 0 };
SessionGeometry palette_geometry = { "palette", 140, 180, 0, 0 };
SessionGeometry brush_select_geometry = { "brush-select", 150, 180, 0, 0 };
SessionGeometry pattern_select_geometry = { "pattern-select", 160, 180, 0, 0 };
SessionGeometry gradient_editor_geometry = { "gradient-editor", 170, 180, 0, 0 };

/* public functions */
void 
session_get_window_geometry (GtkWidget       *window, 
			     SessionGeometry *geometry)
{
  if ( !save_window_positions_on_exit || geometry == NULL || window->window == NULL )
    return;

  gdk_window_get_origin (window->window, &geometry->x, &geometry->y);
  gdk_window_get_size (window->window, &geometry->width, &geometry->height);

  if (g_list_find (session_geometry_updates, geometry) == NULL)
    session_geometry_updates = g_list_append (session_geometry_updates, geometry);
}

void 
session_set_window_geometry (GtkWidget       *window, 
			     SessionGeometry *geometry,
			     int              set_size)
{
  if ( window == NULL || geometry == NULL)
    return;
  
  gtk_widget_set_uposition (window, geometry->x, geometry->y);
  
  if ( (set_size) && (geometry->width > 0) && (geometry->height > 0) )
    gtk_widget_set_usize (window, geometry->width, geometry->height);
}

void
save_sessionrc (void)
{
  char *gimp_dir;
  char filename[512];
  FILE *fp;

  gimp_dir = gimp_directory ();
  if ('\000' != gimp_dir[0])
    {
      sprintf (filename, "%s/sessionrc", gimp_dir);

      fp = fopen (filename, "w");
      if (!fp)
	return;

      fprintf(fp, "# GIMP sessionrc\n");
      fprintf(fp, "# This file takes session-specific info (that is info,\n");
      fprintf(fp, "# you want to keep between two gimp-sessions). You are\n");
      fprintf(fp, "# not supposed to edit it manually, but of course you\n");
      fprintf(fp, "# can do. This file will be entirely rewritten every time\n"); 
      fprintf(fp, "# you quit the gimp. If this file isn't found, defaults\n");
      fprintf(fp, "# are used.\n\n");

      g_list_foreach (session_geometry_updates, (GFunc)sessionrc_write_geometry, fp);

      fclose (fp);
    }
}

void
session_init (void)
{
  char *gimp_dir;
  char filename[512];

  gimp_dir = gimp_directory ();

  if (gimp_dir)
    {
      sprintf (filename, "%s/sessionrc", gimp_dir);
      parse_gimprc_file (filename);
    }
}


/* internal function */
static void
sessionrc_write_geometry (SessionGeometry *geometry, FILE *fp)
{
  if (fp == NULL || geometry == NULL) 
    return;
  
  fprintf (fp,"(session-geometry \"%s\"\n", geometry->name);
  fprintf (fp,"   (position %d %d)\n", geometry->x, geometry->y);
  fprintf (fp,"   (size %d %d))\n\n", geometry->width, geometry->height);
}










