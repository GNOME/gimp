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


/* Session-managment stuff   Copyright (C) 1998 Sven Neumann <sven@gimp.org>

   I include a short description here on what is done and what problems 
   are left:

   Since everything saved in sessionrc changes often (with each session?) 
   the whole file is rewritten each time the gimp exits. I don't see any
   use in implementing a more flexible scheme like it is used for gimprc.

   Right now session-managment is limited to window geometry. Restoring 
   openend images is planned, but I'm still not sure how to deal with dirty
   images.

   Dialogs are now reopened if the gimp is called with the command-line-option
   --restore-session or if the related entry is set in gimprc.
   Probably there should alternatively be a list of dialogs in the preferences 
   that should always be opened on start-up. 

   Please point me into the right direction to make this work with Gnome-SM.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include "app_procs.h"
#include "appenv.h"
#include "commands.h"
#include "gimprc.h"
#include "session.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpenv.h"

static void sessionrc_write_info (SessionInfo *, FILE *);
static void session_open_dialog (SessionInfo *);
static void session_reset_open_state (SessionInfo *);

GList *session_info_updates = NULL;

/* global session variables */
SessionInfo toolbox_session_info = 
  { "toolbox", NULL, 0, 0, 0, 0, FALSE };
SessionInfo lc_dialog_session_info = 
  { "lc-dialog", (GtkItemFactoryCallback)dialogs_lc_cmd_callback, 0, 400, 0, 0, FALSE };
SessionInfo info_dialog_session_info = 
  { "info-dialog", NULL, 165, 0, 0, 0, FALSE };
SessionInfo tool_options_session_info = 
  { "tool-options", (GtkItemFactoryCallback)dialogs_tools_options_cmd_callback, 0, 345, 0, 0, FALSE };
SessionInfo palette_session_info = 
  { "palette", (GtkItemFactoryCallback)dialogs_palette_cmd_callback, 140, 180, 0, 0, FALSE };
SessionInfo brush_select_session_info = 
  { "brush-select",  (GtkItemFactoryCallback)dialogs_brushes_cmd_callback, 150, 180, 0, 0, FALSE };
SessionInfo pattern_select_session_info = 
  { "pattern-select", (GtkItemFactoryCallback)dialogs_patterns_cmd_callback, 160, 180, 0, 0, FALSE };
SessionInfo gradient_select_session_info = 
  { "gradient-editor", (GtkItemFactoryCallback)dialogs_gradient_editor_cmd_callback, 170, 180, 0, 0, FALSE };
SessionInfo device_status_session_info = 
  { "device-status", (GtkItemFactoryCallback)dialogs_device_status_cmd_callback, 0, 600, 0, 0, FALSE };
SessionInfo error_console_session_info = 
  { "error-console", (GtkItemFactoryCallback)dialogs_error_console_cmd_callback, 400, 0, 250, 300, FALSE };

/* public functions */
void 
session_get_window_info (GtkWidget   *window, 
			 SessionInfo *info)
{
  if ( !save_session_info || info == NULL || window->window == NULL )
    return;

  gdk_window_get_root_origin (window->window, &info->x, &info->y);
  gdk_window_get_size (window->window, &info->width, &info->height);

  if ( we_are_exiting )
    info->open = GTK_WIDGET_VISIBLE (window);

  if ( g_list_find (session_info_updates, info) == NULL )
    session_info_updates = g_list_append (session_info_updates, info);
}

void 
session_set_window_geometry (GtkWidget   *window, 
			     SessionInfo *info,
			     int          set_size)
{
  if ( window == NULL || info == NULL)
    return;
  
#ifdef GDK_WINDOWING_WIN32
  /* We should not position windows so that no decoration is visible */
  if (info->y > 0)
    gtk_widget_set_uposition (window, info->x, info->y);
#else
  gtk_widget_set_uposition (window, info->x, info->y);
#endif
  
  if ( (set_size) && (info->width > 0) && (info->height > 0) )
    gtk_window_set_default_size (GTK_WINDOW(window), info->width, info->height);
}

void
save_sessionrc (void)
{
  char *filename;
  FILE *fp;

  filename = gimp_personal_rc_file ("sessionrc");

  fp = fopen (filename, "wt");
  g_free (filename);
  if (!fp)
    return;

  fprintf(fp, _("# GIMP sessionrc\n"
    "# This file takes session-specific info (that is info,\n"
    "# you want to keep between two gimp-sessions). You are\n"
    "# not supposed to edit it manually, but of course you\n"
    "# can do. This file will be entirely rewritten every time\n" 
    "# you quit the gimp. If this file isn't found, defaults\n"
    "# are used.\n\n"));
  
  /* save window geometries */
  g_list_foreach (session_info_updates, (GFunc)sessionrc_write_info, fp);
  
  /* save last tip shown */
  fprintf(fp, "(last-tip-shown %d)\n\n", last_tip + 1);
  
  fclose (fp);
}

void
session_init (void)
{
  gchar *filename;

  filename = gimp_personal_rc_file ("sessionrc");
  app_init_update_status (NULL, filename, -1);

  /*  always show L&C and Brushes on first invocation  */
  if (! parse_gimprc_file (filename) && save_session_info)
    {
      lc_dialog_session_info.open = TRUE;
      session_info_updates =
	g_list_append (session_info_updates, &lc_dialog_session_info);

      brush_select_session_info.open = TRUE;
      session_info_updates =
	g_list_append (session_info_updates, &brush_select_session_info);
    }

  g_free (filename);
}

void
session_restore (void)
{      
  /* open dialogs */
  if (restore_session)
    g_list_foreach (session_info_updates, (GFunc)session_open_dialog, NULL);

  /* reset the open state in the session_infos */
  g_list_foreach (session_info_updates, (GFunc)session_reset_open_state, NULL);
}

/* internal function */
static void
sessionrc_write_info (SessionInfo *info, FILE *fp)
{
  if (fp == NULL || info == NULL) 
    return;
  
  fprintf (fp,"(session-info \"%s\"\n", info->name);
  fprintf (fp,"   (position %d %d)\n", info->x, info->y);
  fprintf (fp,"   (size %d %d)", info->width, info->height);
  if ( info->open ) 
    fprintf (fp,"\n   (open-on-exit)");
  fprintf (fp,")\n\n"); 
}

static void
session_open_dialog (SessionInfo *info)
{
  if (info == NULL || info->open == FALSE || info->open_callback == NULL) 
    return;

  (info->open_callback) ();
}

static void
session_reset_open_state (SessionInfo *info)
{
  if (info == NULL ) 
    return;

  info->open = FALSE;
}
