/* GIMP - The GNU Image Manipulation Program
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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core/core-types.h"

#include "config/gimprc.h"

#include "base/base.h"

#include "core/gimp.h"
#include "core/gimp-user-install.h"

#include "file/file-open.h"

#ifndef GIMP_CONSOLE_COMPILATION
#include "dialogs/user-install-dialog.h"

#include "gui/gui.h"
#endif

#include "app.h"
#include "batch.h"
#include "errors.h"
#include "units.h"

#include "gimp-intl.h"

/*  local prototypes  */

static void       app_init_update_none    (const gchar *text1,
                                           const gchar *text2,
                                           gdouble      percentage);
static gboolean   app_exit_after_callback (Gimp        *gimp,
                                           gboolean     kill_it,
                                           GMainLoop   *loop);


/*  public functions  */

void
app_libs_init (GOptionContext *context,
               gboolean        no_interface)
{
  if (no_interface)
    {
      g_type_init ();
    }
#ifndef GIMP_CONSOLE_COMPILATION
  else
    {
      gui_libs_init (context);
    }
#endif
}

void
app_abort (gboolean     no_interface,
           const gchar *abort_message)
{
#ifndef GIMP_CONSOLE_COMPILATION
  if (no_interface)
#endif
    {
      g_print ("%s\n\n", abort_message);
    }
#ifndef GIMP_CONSOLE_COMPILATION
  else
    {
      gui_abort (abort_message);
    }
#endif

  app_exit (EXIT_FAILURE);
}

void
app_exit (gint status)
{
  exit (status);
}

void
app_run (const gchar         *full_prog_name,
         const gchar        **filenames,
         const gchar         *alternate_system_gimprc,
         const gchar         *alternate_gimprc,
         const gchar         *session_name,
         const gchar         *batch_interpreter,
         const gchar        **batch_commands,
         gboolean             as_new,
         gboolean             no_interface,
         gboolean             no_data,
         gboolean             no_fonts,
         gboolean             no_splash,
         gboolean             be_verbose,
         gboolean             use_shm,
         gboolean             use_cpu_accel,
         gboolean             console_messages,
         gboolean             use_debug_handler,
         GimpStackTraceMode   stack_trace_mode,
         GimpPDBCompatMode    pdb_compat_mode)
{
  GimpInitStatusFunc  update_status_func = NULL;
  Gimp               *gimp;
  GimpBaseConfig     *config;
  GMainLoop          *loop;
  gboolean            swap_is_ok;

  /*  Create an instance of the "Gimp" object which is the root of the
   *  core object system
   */
  gimp = gimp_new (full_prog_name,
                   session_name,
                   be_verbose,
                   no_data,
                   no_fonts,
                   no_interface,
                   use_shm,
                   console_messages,
                   stack_trace_mode,
                   pdb_compat_mode);

  errors_init (gimp, full_prog_name, use_debug_handler, stack_trace_mode);

  units_init (gimp);

  /*  Check if the user's gimp_directory exists
   */
  if (! g_file_test (gimp_directory (), G_FILE_TEST_IS_DIR))
    {
      GimpUserInstall *install = gimp_user_install_new (be_verbose);

#ifdef GIMP_CONSOLE_COMPILATION
      gimp_user_install_run (install);
#else
      if (! (no_interface ?
	     gimp_user_install_run (install) :
	     user_install_dialog_run (install)))
	exit (EXIT_FAILURE);
#endif

      gimp_user_install_free (install);
    }

  gimp_load_config (gimp, alternate_system_gimprc, alternate_gimprc);

  config = GIMP_BASE_CONFIG (gimp->config);

  /*  initialize lowlevel stuff  */
  swap_is_ok = base_init (config, be_verbose, use_cpu_accel);

#ifndef GIMP_CONSOLE_COMPILATION
  if (! no_interface)
    update_status_func = gui_init (gimp, no_splash);
#endif

  if (! update_status_func)
    update_status_func = app_init_update_none;

  /*  Create all members of the global Gimp instance which need an already
   *  parsed gimprc, e.g. the data factories
   */
  gimp_initialize (gimp, update_status_func);

  /*  Load all data files
   */
  gimp_restore (gimp, update_status_func);

  /* display a warning when no test swap file could be generated */
  if (! swap_is_ok)
    {
      gchar *path = gimp_config_path_expand (config->swap_path, FALSE, NULL);

      g_message (_("Unable to open a test swap file.\n\n"
		   "To avoid data loss, please check the location "
		   "and permissions of the swap directory defined in "
		   "your Preferences (currently \"%s\")."), path);

      g_free (path);
    }

  /*  enable autosave late so we don't autosave when the
   *  monitor resolution is set in gui_init()
   */
  gimp_rc_set_autosave (GIMP_RC (gimp->edit_config), TRUE);

  /*  Load the images given on the command-line.
   */
  if (filenames)
    {
      gint i;

      for (i = 0; filenames[i] != NULL; i++)
        file_open_from_command_line (gimp, filenames[i], as_new);
    }

#ifndef GIMP_CONSOLE_COMPILATION
  if (! no_interface)
    gui_post_init (gimp);
#endif

  batch_run (gimp, batch_interpreter, batch_commands);

  loop = g_main_loop_new (NULL, FALSE);

  g_signal_connect_after (gimp, "exit",
                          G_CALLBACK (app_exit_after_callback),
                          loop);

  gimp_threads_leave (gimp);
  g_main_loop_run (loop);
  gimp_threads_enter (gimp);

  g_main_loop_unref (loop);

  g_object_unref (gimp);
  errors_exit ();
  base_exit ();
}


/*  private functions  */

static void
app_init_update_none (const gchar *text1,
                      const gchar *text2,
                      gdouble      percentage)
{
}

static gboolean
app_exit_after_callback (Gimp      *gimp,
                         gboolean   kill_it,
                         GMainLoop *loop)
{
  if (gimp->be_verbose)
    g_print ("EXIT: app_exit_after_callback\n");

  /*
   *  In stable releases, we simply call exit() here. This speeds up
   *  the process of quitting GIMP and also works around the problem
   *  that plug-ins might still be running.
   *
   *  In unstable releases, we shut down GIMP properly in an attempt
   *  to catch possible problems in our finalizers.
   */

#ifdef GIMP_UNSTABLE
  g_main_loop_quit (loop);
#else
  /*  make sure that the swap file is removed before we quit */
  base_exit ();
  exit (EXIT_SUCCESS);
#endif

  return FALSE;
}
