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
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "tools/tools-types.h"

#include "base/base.h"

#include "core/gimp.h"
#include "core/gimpdatafactory.h"
#include "core/gimpunit.h"

#include "file/file-open.h"

#include "display/gimpdisplay-foreach.h"

#include "tools/tool_manager.h"

#include "gui/gui.h"
#include "gui/splash.h"
#include "gui/user-install-dialog.h"

#include "appenv.h"
#include "app_procs.h"
#include "batch.h"
#include "colormaps.h"
#include "errors.h"
#include "gimprc.h"
#include "plug_in.h"
#include "undo.h"

#ifdef DISPLAY_FILTERS
#include "gdisplay_color.h"
#endif /* DISPLAY_FILTERS */

#include "libgimp/gimpintl.h"


/*  local prototypes  */

static void   app_init_update_status (const gchar *text1,
                                      const gchar *text2,
                                      gdouble      percentage);
static void   app_exit_finish        (void);


/*  global variables  */

Gimp *the_gimp = NULL;


/*  public functions  */

void
app_init (gint    gimp_argc,
	  gchar **gimp_argv)
{
  const gchar *gimp_dir;
  struct stat  stat_buf;

  /*  Create an instance of the "Gimp" object which is the root of the
   *  core object system
   */
  the_gimp = gimp_new (be_verbose, no_data, no_interface);

  /*  Check if the user's gimp_directory exists
   */
  gimp_dir = gimp_directory ();

  if (stat (gimp_dir, &stat_buf) != 0)
    {
      /*  not properly installed  */

      if (no_interface)
	{
	  g_print (_("The GIMP is not properly installed for the current user\n"));
	  g_print (_("User installation was skipped because the '--nointerface' flag was encountered\n"));
	  g_print (_("To perform user installation, run the GIMP without the '--nointerface' flag\n"));
	}
      else
	{
	  user_install_dialog_create (the_gimp);

	  gtk_main ();
	}
    }

  /*  The user_install dialog may have parsed unitrc and gimprc, so
   *  check gimprc_init()'s return value
   */
  if (gimprc_init (the_gimp))
    {
      /*  this needs to be done before gimprc loading  */
      gimp_unitrc_load (the_gimp);

      /*  parse the local GIMP configuration file  */
      gimprc_parse (the_gimp);
    }

  /*  initialize lowlevel stuff  */
  base_init ();

  if (! no_interface)
    {
      gui_themes_init (the_gimp);

      get_standard_colormaps ();

      if (! no_splash)
	splash_create (! no_splash_image);
    }

  /*  Create all members of the global Gimp instance which need an already
   *  parsed gimprc, e.g. the data factories
   */
  gimp_initialize (the_gimp, app_init_update_status);

  if (! no_interface)
    {
      tool_manager_init (the_gimp);
    }

  /*  Load all data files
   */
  gimp_restore (the_gimp, app_init_update_status, no_data);

  if (! no_interface)
    {
      gui_init (the_gimp);
    }

  /*  Initialize the plug-in structures
   */
  plug_in_init (the_gimp, app_init_update_status);

  if (! no_interface)
    {
      if (! no_splash)
	splash_destroy ();

      /*  FIXME: This needs to go in preferences  */
      message_handler = MESSAGE_BOX;

      gui_restore (the_gimp, restore_session);
    }

  /*  Parse the rest of the command line arguments as images to load
   */
  if (gimp_argc > 0)
    {
      gint i;

      for (i = 0; i < gimp_argc; i++)
        {
          if (gimp_argv[i])
            {
              file_open_with_display (the_gimp, gimp_argv[i]);
            }
        }
    }

  batch_init (the_gimp);

  if (! no_interface)
    {
      gui_post_init (the_gimp);
    }

  gimp_main_loop (the_gimp);
}

void
app_exit (gboolean kill_it)
{
  /*  If it's the user's perogative, and there are dirty images  */
  if (! kill_it && gdisplays_dirty () && ! no_interface)
    gui_really_quit_dialog (G_CALLBACK (app_exit_finish));
  else
    app_exit_finish ();
}


/*  private functions  */

static void
app_init_update_status (const gchar *text1,
		        const gchar *text2,
		        gdouble      percentage)
{
  if (! no_interface && ! no_splash)
    {
      splash_update (text1, text2, percentage);
    }
}

static void
app_exit_finish (void)
{
  message_handler = CONSOLE;

  if (! no_interface)
    {
      gui_shutdown (the_gimp);
    }

  plug_in_kill ();

  if (! no_interface)
    {
      tool_manager_exit (the_gimp);

      gui_exit (the_gimp);
    }

  gimp_shutdown (the_gimp);

  g_object_unref (G_OBJECT (the_gimp));
  the_gimp = NULL;

  base_exit ();

  /*  There used to be foo_main_quit() here, but there's a chance 
   *  that foo_main() was never called before we reach this point. --Sven  
   */
  exit (0);
}
