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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdktypes.h>

#ifdef G_OS_WIN32
#include <process.h>		/* For _getpid() */
#endif
 
#include "libgimp/gimplimits.h"
#include "libgimp/gimpfeatures.h"
#include "libgimp/gimpenv.h"

#include "apptypes.h"

#include "gui/gui.h"
#include "gui/splash.h"

#include "paint-funcs/paint-funcs.h"

#include "pdb/internal_procs.h"

#include "tools/curves.h"
#include "tools/hue_saturation.h"
#include "tools/levels.h"

#include "appenv.h"
#include "app_procs.h"
#include "batch.h"
#include "brush_select.h"
#include "color_transfer.h"
#include "colormaps.h"
#include "context_manager.h"
#include "errorconsole.h"
#include "file-open.h"
#include "file-save.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "gimpcontext.h"
#include "gimpdatafactory.h"
#include "gimpimage.h"
#include "gimprc.h"
#include "gimpparasite.h"
#include "global_edit.h"
#include "gradient_select.h"
#include "lc_dialog.h"
#include "palette.h"
#include "pattern_select.h"
#include "plug_in.h"
#include "module_db.h"

#include "temp_buf.h"
#include "tile_swap.h"
#include "tips_dialog.h"
#include "undo.h"
#include "unitrc.h"
#include "xcf.h"
#include "errors.h"
#include "docindex.h"
#include "colormap_dialog.h"

#ifdef DISPLAY_FILTERS
#include "gdisplay_color.h"
#endif /* DISPLAY_FILTERS */

#include "color_notebook.h"
#include "color_select.h"
#include "gimpparasite.h"

#include "libgimp/gimpintl.h"


static void   toast_old_temp_files (void);


static gboolean is_app_exit_finish_done = FALSE;
gboolean        we_are_exiting          = FALSE;


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
	  file_open (*gimp_argv, *gimp_argv);
	gimp_argv++;
      }

  batch_init ();

  /* Handle showing dialogs with gdk_quit_adds here  */
  if (!no_interface && show_tips)
    tips_dialog_create ();
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

void
app_init (void)
{
  const gchar *gtkrc;
  gchar       *filename;
  gchar       *path;

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

  if (parse_buffers_init ())
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
  if (always_restore_session)
    restore_session = TRUE;

  /* make sure the monitor resolution is valid */
  if (monitor_xres < GIMP_MIN_RESOLUTION ||
      monitor_yres < GIMP_MIN_RESOLUTION)
    {
      gdisplay_xserver_resolution (&monitor_xres, &monitor_yres);
      using_xserver_resolution = TRUE;
    }

  /* Now we are ready to draw the splash-screen-image to the start-up window */
  if (! no_interface && ! no_splash_image)
    {
      splash_logo_load ();
    }

  RESET_BAR();
  file_open_pre_init ();   /*  pre-initialize the file types     */
  file_save_pre_init ();

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
  file_open_post_init ();    /*  post-initialize the file types     */
  file_save_post_init ();

  /* Add the swap file  */
  if (swap_path == NULL)
    swap_path = g_get_tmp_dir ();

  toast_old_temp_files ();
  path = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "gimpswap.%lu",
			  swap_path, (unsigned long) getpid ());
  tile_swap_add (path, NULL, NULL);
  g_free (path);

  if (! no_interface)
    {
      if (! no_splash)
	splash_destroy ();

      gui_init ();

      /*  FIXME: This needs to go in preferences  */
      message_handler = MESSAGE_BOX;
    }

  color_transfer_init ();
  paint_funcs_setup ();

  /* register internal color selectors */
  color_select_init ();

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
  we_are_exiting  = TRUE;

  if (! no_interface)
    {
      gui_shutdown ();
    }

  module_db_free ();
  lc_dialog_free ();
  gdisplays_delete ();
  global_edit_free ();
  named_buffers_free ();
  swapping_free ();
  brush_dialog_free ();
  pattern_dialog_free ();
  palette_dialog_free ();
  gradient_dialog_free ();
  context_manager_free ();
  hue_saturation_free ();
  curves_free ();
  levels_free ();
  paint_funcs_free ();
  plug_in_kill ();
  procedural_db_free ();
  error_console_free ();
  tile_swap_exit ();
  save_unitrc ();
  gimp_parasiterc_save ();

  if (! no_interface)
    {
      gui_exit ();
    }

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

static void
toast_old_temp_files (void)
{
  DIR           *dir;
  struct dirent *entry;
  GString       *filename = g_string_new ("");

  dir = opendir (swap_path);

  if (!dir)
    return;

  while ((entry = readdir (dir)) != NULL)
    if (!strncmp (entry->d_name, "gimpswap.", 9))
      {
        /* don't try to kill swap files of running processes
         * yes, I know they might not all be gimp processes, and when you
         * unlink, it's refcounted, but lets not confuse the user by
         * "where did my disk space go?" cause the filename is gone
         * if the kill succeeds, and there running process isn't gimp
         * we'll probably get it the next time around
         */

	gint pid = atoi (entry->d_name + 9);
#ifndef G_OS_WIN32
	if (kill (pid, 0))
#endif
	  {
	    /*  On Windows, you can't remove open files anyhow,
	     *  so no harm trying.
	     */
	    g_string_sprintf (filename, "%s" G_DIR_SEPARATOR_S "%s",
			      swap_path, entry->d_name);
	    unlink (filename->str);
	  }
      }

  closedir (dir);
  
  g_string_free (filename, TRUE);
}
