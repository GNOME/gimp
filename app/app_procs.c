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

#include "config.h"

#include <stdio.h>
#include <string.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"
#include "tools/tools-types.h"

#include "base/base.h"

#include "core/gimp.h"
#include "core/gimpdatafactory.h"

#include "pdb/internal_procs.h"

#include "xcf/xcf.h"

#include "tools/tool_manager.h"

#include "gui/color-notebook.h"
#include "gui/file-open-dialog.h"
#include "gui/gui.h"
#include "gui/splash.h"

#include "appenv.h"
#include "app_procs.h"
#include "batch.h"
#include "colormaps.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "gimprc.h"
#include "plug_in.h"
#include "module_db.h"

#include "undo.h"
#include "unitrc.h"
#include "errors.h"
#include "docindex.h"

#ifdef DISPLAY_FILTERS
#include "gdisplay_color.h"
#endif /* DISPLAY_FILTERS */

#include "libgimp/gimpintl.h"


Gimp *the_gimp = NULL;


/* FIXME: gimp_busy HACK */
gboolean gimp_busy = FALSE;

static gboolean is_app_exit_finish_done = FALSE;


void
app_init_update_status (const gchar *text1,
		        const gchar *text2,
		        gdouble      percentage)
{
  if (! no_interface && ! no_splash)
    {
      splash_update (text1, text2, percentage);
    }
}

/* #define RESET_BAR() app_init_update_status("", "", 0) */
#define RESET_BAR()

void
app_init (gint    gimp_argc,
	  gchar **gimp_argv)
{
  const gchar *gtkrc;
  gchar       *filename;

  /*  parse the systemwide gtkrc  */
  gtkrc = gimp_gtkrc ();

  if (be_verbose)
    g_print (_("parsing \"%s\"\n"), gtkrc);

  gtk_rc_parse (gtkrc);

  /*  parse the user gtkrc  */
  filename = gimp_personal_rc_file ("gtkrc");

  if (be_verbose)
    g_print (_("parsing \"%s\"\n"), filename);

  gtk_rc_parse (filename);

  g_free (filename);

  if (gimprc_init ())
    {
      parse_unitrc ();   /*  this needs to be done before gimprc loading  */
      parse_gimprc ();   /*  parse the local GIMP configuration file      */
    }

  if (! no_interface)
    {
      get_standard_colormaps ();

      if (! no_splash)
	splash_create ();
    }

  /*  initialize lowlevel stuff  */
  base_init ();

  /*  Create an instance of the "Gimp" object which is the root of the
   *  core object system
   */
  the_gimp = gimp_new ();

  gtk_object_ref (GTK_OBJECT (the_gimp));
  gtk_object_sink (GTK_OBJECT (the_gimp));

  tool_manager_init (the_gimp);

  /*  Initialize the procedural database
   *    We need to do this first because any of the init
   *    procedures might install or query it as needed.
   */
  app_init_update_status (_("Procedural Database"), NULL, -1);
  internal_procs_init (the_gimp);

#ifdef DISPLAY_FILTERS
  color_display_init ();
#endif /* DISPLAY_FILTERS */

  RESET_BAR();
  if (gimprc.always_restore_session)
    restore_session = TRUE;

  /* Now we are ready to draw the splash-screen-image to the start-up window */
  if (! no_interface && ! no_splash_image)
    {
      splash_logo_load ();
    }

  RESET_BAR();
  xcf_init (the_gimp);       /*  initialize the xcf file format routines */

  /*  load all data files  */
  gimp_restore (the_gimp);

  plug_in_init ();           /*  initialize the plug in structures  */
  module_db_init ();         /*  load any modules we need           */

  RESET_BAR();

  if (! no_interface)
    {
      if (! no_splash)
	splash_destroy ();

      gui_init (the_gimp);

      /*  FIXME: This needs to go in preferences  */
      message_handler = MESSAGE_BOX;
    }

  if (! no_interface)
    {
      gui_restore (the_gimp);
    }

  /* Parse the rest of the command line arguments as images to load */
  if (gimp_argc > 0)
    while (gimp_argc--)
      {
	if (*gimp_argv)
	  file_open_with_display (*gimp_argv, *gimp_argv);
	gimp_argv++;
      }

  batch_init (the_gimp);

  if (! no_interface)
    {
      gui_post_init (the_gimp);
    }
}

void
app_exit (gboolean kill_it)
{
  /*  If it's the user's perogative, and there are dirty images  */
  if (! kill_it && gdisplays_dirty () && ! no_interface)
    gui_really_quit_dialog ();
  else
    app_exit_finish ();
}

void
app_exit_finish (void)
{
  if (app_exit_finish_done ())
    return;

  is_app_exit_finish_done = TRUE;

  message_handler = CONSOLE;

  if (! no_interface)
    {
      gui_shutdown (the_gimp);
    }

  module_db_free ();
  gdisplays_delete ();
  plug_in_kill ();
  save_unitrc ();

  tool_manager_exit (the_gimp);

  if (! no_interface)
    {
      gui_exit (the_gimp);
    }

  xcf_exit ();

  gimp_shutdown (the_gimp);

  gtk_object_unref (GTK_OBJECT (the_gimp));
  the_gimp = NULL;

  base_exit ();

  /*  There used to be gtk_main_quit() here, but there's a chance 
   *  that gtk_main() was never called before we reach this point. --Sven  
   */
  gtk_exit (0);
}

gboolean
app_exit_finish_done (void)
{
  return is_app_exit_finish_done;
}

void
gimp_set_busy (void)
{
  /* FIXME: gimp_busy HACK */
  gimp_busy = TRUE;

  gui_set_busy (the_gimp);
}

static gboolean
gimp_idle_unset_busy (gpointer data)
{
  gimp_unset_busy ();

  *((guint *) data) = 0;

  return FALSE;
}

void
gimp_set_busy_until_idle (void)
{
  static guint busy_idle_id = 0;

  if (! busy_idle_id)
    {
      gimp_set_busy ();

      busy_idle_id = g_idle_add_full (G_PRIORITY_HIGH,
				      gimp_idle_unset_busy, &busy_idle_id,
				      NULL);
    }
}

void
gimp_unset_busy (void)
{
  gui_unset_busy (the_gimp);

  /* FIXME: gimp_busy HACK */
  gimp_busy = FALSE;
}
