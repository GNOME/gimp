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

#include "core/gimpdatafactory.h"

#include "pdb/internal_procs.h"

#include "tools/tools.h"

#include "gui/color-notebook.h"
#include "gui/file-open-dialog.h"
#include "gui/gui.h"
#include "gui/splash.h"

#include "appenv.h"
#include "app_procs.h"
#include "batch.h"
#include "colormaps.h"
#include "context_manager.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "gimprc.h"
#include "gimpparasite.h"
#include "plug_in.h"
#include "module_db.h"

#include "undo.h"
#include "unitrc.h"
#include "xcf.h"
#include "errors.h"
#include "docindex.h"

#ifdef DISPLAY_FILTERS
#include "gdisplay_color.h"
#endif /* DISPLAY_FILTERS */

#include "gimpparasite.h"

#include "libgimp/gimpintl.h"


static void   app_init (void);


/* FIXME: gimp_busy HACK */
gboolean gimp_busy = FALSE;

static gboolean is_app_exit_finish_done = FALSE;


void
gimp_init (gint    gimp_argc,
	   gchar **gimp_argv)
{
  /* Initialize the application */
  app_init ();

  /* Parse the rest of the command line arguments as images to load */
  if (gimp_argc > 0)
    while (gimp_argc--)
      {
	if (*gimp_argv)
	  file_open_with_display (*gimp_argv, *gimp_argv);
	gimp_argv++;
      }

  batch_init ();

  if (! no_interface)
    {
      gui_post_init ();
    }
}

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

static void
app_init (void)
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

  /*  Initialize the context system before loading any data  */
  context_manager_init ();

  /*  Initialize the procedural database
   *    We need to do this first because any of the init
   *    procedures might install or query it as needed.
   */
  procedural_db_init ();
  RESET_BAR();
  internal_procs_init ();

#ifdef DISPLAY_FILTERS
  color_display_init ();
#endif /* DISPLAY_FILTERS */

  RESET_BAR();
  if (gimprc.always_restore_session)
    restore_session = TRUE;

  /* make sure the monitor resolution is valid */
  if (gimprc.monitor_xres < GIMP_MIN_RESOLUTION ||
      gimprc.monitor_yres < GIMP_MIN_RESOLUTION)
    {
      gdisplay_xserver_resolution (&gimprc.monitor_xres, &gimprc.monitor_yres);
      gimprc.using_xserver_resolution = TRUE;
    }

  /* Now we are ready to draw the splash-screen-image to the start-up window */
  if (! no_interface && ! no_splash_image)
    {
      splash_logo_load ();
    }

  RESET_BAR();
  xcf_init ();             /*  initialize the xcf file format routines */

  /*  initialize  the global parasite table  */
  app_init_update_status (_("Looking for data files"), _("Parasites"), 0.00);
  gimp_init_parasites ();

  /*  initialize the list of gimp brushes    */
  app_init_update_status (NULL, _("Brushes"), 0.20);
  gimp_data_factory_data_init (global_brush_factory, no_data); 

  /*  initialize the list of gimp patterns   */
  app_init_update_status (NULL, _("Patterns"), 0.40);
  gimp_data_factory_data_init (global_pattern_factory, no_data); 

  /*  initialize the list of gimp palettes   */
  app_init_update_status (NULL, _("Palettes"), 0.60);
  gimp_data_factory_data_init (global_palette_factory, no_data); 

  /*  initialize the list of gimp gradients  */
  app_init_update_status (NULL, _("Gradients"), 0.80);
  gimp_data_factory_data_init (global_gradient_factory, no_data); 

  app_init_update_status (NULL, NULL, 1.00);

  plug_in_init ();           /*  initialize the plug in structures  */
  module_db_init ();         /*  load any modules we need           */

  RESET_BAR();

  /*  initialize lowlevel stuff  */
  base_init ();

  if (! no_interface)
    {
      if (! no_splash)
	splash_destroy ();

      gui_init ();

      /*  FIXME: This needs to go in preferences  */
      message_handler = MESSAGE_BOX;
    }

  if (! no_interface)
    {
      gui_restore ();
    }
}

void
app_exit (gboolean kill_it)
{
  /*  If it's the user's perogative, and there are dirty images  */
  if (!kill_it && gdisplays_dirty () && !no_interface)
    really_quit_dialog ();
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
      gui_shutdown ();
    }

  module_db_free ();
  gdisplays_delete ();
  context_manager_free ();
  plug_in_kill ();
  procedural_db_free ();
  save_unitrc ();
  gimp_parasiterc_save ();

  tools_exit ();

  if (! no_interface)
    {
      gui_exit ();
    }

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

  gui_set_busy ();
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
  gui_unset_busy ();

  /* FIXME: gimp_busy HACK */
  gimp_busy = FALSE;
}
