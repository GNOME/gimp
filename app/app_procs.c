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

#include "core/core-types.h"

#include "config/gimprc.h"

#include "core/gimp.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "tools/gimp-tools.h"

#include "gui/gui.h"
#include "gui/user-install-dialog.h"

#include "app_procs.h"
#include "batch.h"
#include "errors.h"
#include "units.h"

#include "gimp-intl.h"


/*  local prototypes  */

static void       app_init_update_none    (const gchar *text1,
                                           const gchar *text2,
                                           gdouble      percentage);
static gboolean   app_exit_after_callback (Gimp        *gimp,
                                           gboolean     kill_it);


/*  private variables  */

static Gimp *the_gimp = NULL;


/*  public functions  */

gboolean
app_gui_libs_init (gint    *argc,
                   gchar ***argv)
{
  return gui_libs_init (argc, argv);
}

void
app_init (const gchar         *full_prog_name,
          gint                 gimp_argc,
          gchar              **gimp_argv,
          const gchar         *alternate_system_gimprc,
          const gchar         *alternate_gimprc,
          const gchar         *session_name,
          const gchar        **batch_cmds,
          gboolean             no_interface,
          gboolean             no_data,
          gboolean             no_fonts,
          gboolean             no_splash,
          gboolean             no_splash_image,
          gboolean             be_verbose,
          gboolean             use_shm,
          gboolean             use_mmx,
          gboolean             console_messages,
          GimpStackTraceMode   stack_trace_mode)
{
  GimpInitStatusFunc update_status_func = NULL;

  /*  Create an instance of the "Gimp" object which is the root of the
   *  core object system
   */
  the_gimp = gimp_new (full_prog_name,
                       session_name,
                       be_verbose,
                       no_data,
                       no_fonts,
                       no_interface,
                       use_shm,
                       console_messages,
                       stack_trace_mode);

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
  g_log_set_handler ("Gimp-Config",
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
  g_log_set_handler ("Gimp-Text",
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

  units_init (the_gimp);

  /*  Check if the user's gimp_directory exists
   */
  if (! g_file_test (gimp_directory (), G_FILE_TEST_IS_DIR))
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
          user_install_dialog_run (alternate_system_gimprc,
                                   alternate_gimprc,
                                   be_verbose);
	}
    }

  gimp_load_config (the_gimp,
                    alternate_system_gimprc,
                    alternate_gimprc,
                    use_mmx);

  if (! no_interface)
    update_status_func = gui_init (the_gimp, no_splash, no_splash_image);

  if (! update_status_func)
    update_status_func = app_init_update_none;

  /*  connect our "exit" callbacks after gui_init() so they are
   *  invoked after the GUI's "exit" callbacks
   */
  g_signal_connect_after (the_gimp, "exit",
                          G_CALLBACK (app_exit_after_callback),
                          NULL);

  /*  Create all members of the global Gimp instance which need an already
   *  parsed gimprc, e.g. the data factories
   */
  gimp_initialize (the_gimp, update_status_func);

  /*  Load all data files
   */
  gimp_restore (the_gimp, update_status_func);

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

                      g_message (_("Opening '%s' failed: %s"),
                                 filename, error->message);
                      g_clear_error (&error);

                      g_free (filename);
                   }

                  g_free (uri);
                }
            }
        }
    }

  batch_init (the_gimp, batch_cmds);

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
      gui_post_init (the_gimp);

      gtk_main ();
    }
}


/*  private functions  */

static void
app_init_update_none (const gchar *text1,
                      const gchar *text2,
                      gdouble      percentage)
{
}

static gboolean
app_exit_after_callback (Gimp     *gimp,
                         gboolean  kill_it)
{
  if (gimp->be_verbose)
    g_print ("EXIT: app_exit_after_callback\n");

  g_object_unref (gimp);
  the_gimp = NULL;

  /*  There used to be foo_main_quit() here, but there's a chance
   *  that foo_main() was never called before we reach this point. --Sven
   */
  exit (0);

  return FALSE;
}
