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
#include <stdlib.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "tools/tools-types.h"

#include "base/base.h"

#include "config/gimprc.h"

#include "core/gimp.h"
#include "core/gimpunits.h"

#include "plug-in/plug-ins.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "tools/tool_manager.h"

#include "gui/gui.h"
#include "gui/splash.h"
#include "gui/user-install-dialog.h"

#include "appenv.h"
#include "app_procs.h"
#include "batch.h"
#include "errors.h"

#include "gimp-intl.h"


/*  local prototypes  */

static void       app_init_update_status   (const gchar *text1,
                                            const gchar *text2,
                                            gdouble      percentage);
static gboolean   app_exit_callback        (Gimp        *gimp,
                                            gboolean     kill_it);
static gboolean   app_exit_finish_callback (Gimp        *gimp,
                                            gboolean     kill_it);


/*  global variables  */

Gimp *the_gimp = NULL;


/*  public functions  */

gboolean
app_gui_libs_init (gint    *argc,
                   gchar ***argv)
{
  return gui_libs_init (argc, argv);
}

void
app_init (gint    gimp_argc,
	  gchar **gimp_argv)
{
  const gchar *gimp_dir;
  GimpRc      *gimprc;

  /*  Create an instance of the "Gimp" object which is the root of the
   *  core object system
   */
  the_gimp = gimp_new (be_verbose,
                       no_data,
                       no_interface,
                       use_shm,
                       stack_trace_mode);

  gimp_object_set_name (GIMP_OBJECT (the_gimp), prog_name);

  g_log_set_handler ("Gimp",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);
  g_log_set_handler ("Gimp-Base",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);
  g_log_set_handler ("Gimp-Paint-Funcs",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);
  g_log_set_handler ("Gimp-Core",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);
  g_log_set_handler ("Gimp-PDB",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);
  g_log_set_handler ("Gimp-Plug-In",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);
  g_log_set_handler ("Gimp-File",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);
  g_log_set_handler ("Gimp-XCF",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);
  g_log_set_handler ("Gimp-Widgets",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);
  g_log_set_handler ("Gimp-Display",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);
  g_log_set_handler ("Gimp-Tools",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);
  g_log_set_handler ("Gimp-GUI",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_log_func,
		     &the_gimp);

  g_log_set_handler (NULL,
		     G_LOG_LEVEL_ERROR | G_LOG_FLAG_FATAL,
		     gimp_error_log_func,
		     &the_gimp);

  /*  Check if the user's gimp_directory exists
   */
  gimp_dir = gimp_directory ();

  if (! g_file_test (gimp_dir, G_FILE_TEST_IS_DIR))
    {
      /*  not properly installed  */

      if (no_interface)
	{
          const gchar *msg;

          msg = _("GIMP is not properly installed for the current user.\n"
                  "User installation was skipped because the '--no-interface' flag was used.\n"
                  "To perform user installation, run the GIMP without the '--no-interface' flag.");

          g_print ("%s\n\n", msg);
	}
      else
	{
          user_install_dialog_create (alternate_system_gimprc,
                                      alternate_gimprc,
                                      be_verbose);

	  gtk_main ();
	}
    }

  /*  this needs to be done before gimprc loading because gimprc can
   *  use user defined units
   */
  gimp_unitrc_load (the_gimp);

  gimprc = gimp_rc_new (alternate_system_gimprc,
                        alternate_gimprc,
                        be_verbose);

#if 0
  /* solely for debugging */
  g_signal_connect (gimprc, "notify",
                    G_CALLBACK (gimprc_notify_callback),
                    NULL);
#endif

  /*  initialize lowlevel stuff  */
  base_init (GIMP_BASE_CONFIG (gimprc), use_mmx);

  gimp_set_config (the_gimp, GIMP_CORE_CONFIG (gimprc));

  g_object_unref (gimprc);
  gimprc = NULL;

  if (! no_interface)
    {
      gui_themes_init (the_gimp);

      if (! no_splash)
	splash_create (! no_splash_image);
    }

  /*  Create all members of the global Gimp instance which need an already
   *  parsed gimprc, e.g. the data factories
   */
  gimp_initialize (the_gimp, app_init_update_status);

  if (! no_interface)
    {
      gui_environ_init (the_gimp);
      tool_manager_init (the_gimp);
    }

  /*  Load all data files
   */
  gimp_restore (the_gimp, app_init_update_status, no_data);

  if (! no_interface)
    {
      gui_init (the_gimp);
      tool_manager_restore (the_gimp);
    }

  /*  Initialize the plug-in structures
   */
  plug_ins_init (the_gimp, app_init_update_status);

  if (! no_interface)
    {
      if (! no_splash)
	splash_destroy ();

      gui_restore (the_gimp, restore_session);
    }

  /*  connect our "exit" callbacks after gui_restore() so they are
   *  invoked after the GUI's "exit" callbacks
   */
  g_signal_connect (the_gimp, "exit",
                    G_CALLBACK (app_exit_callback),
                    NULL);
  g_signal_connect_after (the_gimp, "exit",
                          G_CALLBACK (app_exit_finish_callback),
                          NULL);

  /*  enable autosave late so we don't autosave when the
   *  monitor resolution is set in gui_init()
   */
  gimp_rc_set_autosave (GIMP_RC (the_gimp->edit_config), TRUE);

  /*  Parse the rest of the command line arguments as images to load
   */
  if (gimp_argc > 0)
    {
      gint i;

      for (i = 0; i < gimp_argc; i++)
        {
          if (gimp_argv[i])
            {
              GError *error = NULL;
              gchar  *uri;

              /*  first try if we got a file uri  */
              uri = g_filename_from_uri (gimp_argv[i], NULL, NULL);

              if (uri)
                {
                  g_free (uri);
                  uri = g_strdup (gimp_argv[i]);
                }
              else
                {
                  uri = file_utils_filename_to_uri (the_gimp->load_procs,
                                                    gimp_argv[i], &error);
                }

              if (! uri)
                {
                  g_printerr ("conversion filename -> uri failed: %s\n",
                              error->message);
                  g_clear_error (&error);
                }
              else
                {
                  GimpImage         *gimage;
                  GimpPDBStatusType  status;

                  gimage = file_open_with_display (the_gimp, uri,
                                                   &status, &error);

                  if (! gimage && status != GIMP_PDB_CANCEL)
                    {
                      gchar *filename;

                      filename = file_utils_uri_to_utf8_filename (uri);

                      g_message (_("Opening '%s' failed:\n%s"),
                                 filename, error->message);
                      g_clear_error (&error);

                      g_free (filename);
                   }

                  g_free (uri);
                }
            }
        }
    }

  batch_init (the_gimp, (const gchar **) batch_cmds);

  if (! no_interface)
    {
      gui_post_init (the_gimp);
    }

  if (no_interface)
    {
      GMainLoop *loop;

      loop = g_main_loop_new (NULL, FALSE);

      gimp_threads_leave (the_gimp);
      g_main_loop_run (loop);
      gimp_threads_enter (the_gimp);

      g_main_loop_unref (loop);
    }
  else
    {
      gtk_main ();
    }
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

static gboolean
app_exit_callback (Gimp     *gimp,
                   gboolean  kill_it)
{
  plug_ins_exit (gimp);

  if (! gimp->no_interface)
    {
      tool_manager_save (gimp);
      tool_manager_exit (gimp);
    }

  return FALSE; /* continue exiting */
}

static gboolean
app_exit_finish_callback (Gimp     *gimp,
                          gboolean  kill_it)
{
  g_object_unref (gimp);
  the_gimp = NULL;

  base_exit ();

  /*  There used to be foo_main_quit() here, but there's a chance 
   *  that foo_main() was never called before we reach this point. --Sven  
   */
  exit (0);

  return FALSE;
}


#if 0

/****************************************
 * gimprc debugging code, to be removed *
 ****************************************/

#include "config/gimpconfig-serialize.h"

static void
gimprc_notify_callback (GObject    *object,
			GParamSpec *pspec)
{
  GString *str;
  GValue   value = { 0, };

  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  g_value_init (&value, pspec->value_type);
  g_object_get_property (object, pspec->name, &value);

  str = g_string_new (NULL);

  if (gimp_config_serialize_value (&value, str, TRUE))
    {
      g_print ("  %s -> %s\n", pspec->name, str->str);
    }
  else
    {
      g_print ("  %s changed but we failed to serialize its value!\n", 
               pspec->name);
    }

  g_string_free (str, TRUE);
  g_value_unset (&value);
}

#endif
