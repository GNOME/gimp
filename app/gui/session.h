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
#include <gtk/gtk.h>

/* Structures */
typedef struct _SessionInfo SessionInfo;

struct _SessionInfo
{
  gchar                  *name;
  GtkItemFactoryCallback  open_callback;
  gint                    x;
  gint                    y;
  gint                    width;
  gint                    height;
  gint                    count;
  gboolean                open;
};

/*  global session variables  */
extern SessionInfo toolbox_session_info;
extern SessionInfo lc_dialog_session_info;
extern SessionInfo info_dialog_session_info;
extern SessionInfo tool_options_session_info;
extern SessionInfo palette_session_info;
extern SessionInfo brush_select_session_info;
extern SessionInfo pattern_select_session_info;
extern SessionInfo gradient_select_session_info;
extern SessionInfo device_status_session_info;
extern SessionInfo error_console_session_info;

extern GList *session_info_updates;  /* This list holds all session_infos
					that should be written to the
					sessionrc on exit.             */  

/*  function prototypes  */
void session_get_window_info     (GtkWidget   *window, 
				  SessionInfo *info);
void session_set_window_geometry (GtkWidget   *window,
				  SessionInfo *info,
				  gboolean     set_size);
void session_init                (void);
void session_restore             (void);
void save_sessionrc              (void);

#endif  /*  __SESSION_H__  */
