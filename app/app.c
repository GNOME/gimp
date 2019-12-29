/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gegl.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include <winnls.h>
#endif

#undef GIMP_DISABLE_DEPRECATED /* for compat enums */
#include "libgimpbase/gimpbase.h"
#define GIMP_DISABLE_DEPRECATED
#include "libgimpconfig/gimpconfig.h"

#include "core/core-types.h"

#include "config/gimplangrc.h"
#include "config/gimprc.h"

#include "gegl/gimp-gegl.h"

#include "core/gimp.h"
#include "core/gimp-batch.h"
#include "core/gimp-user-install.h"
#include "core/gimpimage.h"

#include "file/file-open.h"

#ifndef GIMP_CONSOLE_COMPILATION
#include "dialogs/user-install-dialog.h"

#include "gui/gui.h"
#endif

#include "app.h"
#include "errors.h"
#include "language.h"
#include "sanity.h"
#include "gimp-debug.h"

#include "gimp-intl.h"
#include "gimp-update.h"


/*  local prototypes  */

static void       app_init_update_noop       (const gchar        *text1,
                                              const gchar        *text2,
                                              gdouble             percentage);
static void       app_restore_after_callback (Gimp               *gimp,
                                              GimpInitStatusFunc  status_callback);
static gboolean   app_exit_after_callback    (Gimp               *gimp,
                                              gboolean            kill_it,
                                              GMainLoop         **loop);

GType gimp_convert_dither_type_compat_get_type (void); /* compat cruft */
GType gimp_layer_mode_effects_get_type         (void); /* compat cruft */


/*  local variables  */

static GObject *initial_screen  = NULL;
static gint     initial_monitor = 0;


/*  public functions  */

void
app_libs_init (GOptionContext *context,
               gboolean        no_interface)
{
  GQuark quark;

  /* disable OpenCL before GEGL is even initialized; this way we only
   * enable if wanted in gimprc, instead of always enabling, and then
   * disabling again if wanted in gimprc
   */
  g_object_set (gegl_config (),
                "use-opencl", FALSE,
                "application-license", "GPL3",
                NULL);

  g_option_context_add_group (context, gegl_get_option_group ());

#ifndef GIMP_CONSOLE_COMPILATION
  if (! no_interface)
    {
      gui_libs_init (context);
    }
#endif

  /*  keep compat enum code in sync with pdb/enumcode.pl  */
  quark = g_quark_from_static_string ("gimp-compat-enum");

  g_type_set_qdata (GIMP_TYPE_CONVERT_DITHER_TYPE, quark,
                    (gpointer) gimp_convert_dither_type_compat_get_type ());
  g_type_set_qdata (GIMP_TYPE_LAYER_MODE, quark,
                    (gpointer) gimp_layer_mode_effects_get_type ());
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
         GFile               *alternate_system_gimprc,
         GFile               *alternate_gimprc,
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
         gboolean             show_playground,
         gboolean             show_debug_menu,
         GimpStackTraceMode   stack_trace_mode,
         GimpPDBCompatMode    pdb_compat_mode,
         const gchar         *backtrace_file)
{
  GimpInitStatusFunc  update_status_func = NULL;
  Gimp               *gimp;
  GMainLoop          *loop;
  GMainLoop          *run_loop;
  GFile              *default_folder = NULL;
  GFile              *gimpdir;
  const gchar        *abort_message;
  GimpLangRc         *temprc;
  gchar              *language   = NULL;
  GError             *font_error = NULL;
  gboolean            save_gimprc_at_exit = FALSE;

  if (filenames && filenames[0] && ! filenames[1] &&
      g_file_test (filenames[0], G_FILE_TEST_IS_DIR))
    {
      if (g_path_is_absolute (filenames[0]))
        {
          default_folder = g_file_new_for_path (filenames[0]);
        }
      else
        {
          gchar *absolute = g_build_path (G_DIR_SEPARATOR_S,
                                          g_get_current_dir (),
                                          filenames[0],
                                          NULL);
          default_folder = g_file_new_for_path (absolute);
          g_free (absolute);
        }

      filenames = NULL;
    }

  /* Language needs to be determined first, before any GimpContext is
   * instantiated (which happens when the Gimp object is created)
   * because its properties need to be properly localized in the
   * settings language (if different from system language). Otherwise we
   * end up with pieces of GUI always using the system language (cf. bug
   * 787457). Therefore we do a first pass on "gimprc" file for the sole
   * purpose of getting the settings language, so that we can initialize
   * it before anything else.
   */
  temprc = gimp_lang_rc_new (alternate_system_gimprc,
                             alternate_gimprc,
                             be_verbose);
  language = gimp_lang_rc_get_language (temprc);
  g_object_unref (temprc);

  /*  change the locale if a language if specified  */
  language_init (language);
  if (language)
    g_free (language);

  /*  Create an instance of the "Gimp" object which is the root of the
   *  core object system
   */
  gimp = gimp_new (full_prog_name,
                   session_name,
                   default_folder,
                   be_verbose,
                   no_data,
                   no_fonts,
                   no_interface,
                   use_shm,
                   use_cpu_accel,
                   console_messages,
                   show_playground,
                   show_debug_menu,
                   stack_trace_mode,
                   pdb_compat_mode);

  if (default_folder)
    g_object_unref (default_folder);

  gimp_cpu_accel_set_use (use_cpu_accel);

  /*  Check if the user's gimp_directory exists
   */
  gimpdir = gimp_directory_file (NULL);

  if (g_file_query_file_type (gimpdir, G_FILE_QUERY_INFO_NONE, NULL) !=
      G_FILE_TYPE_DIRECTORY)
    {
      GimpUserInstall *install = gimp_user_install_new (G_OBJECT (gimp),
                                                        be_verbose);

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

  g_object_unref (gimpdir);

  gimp_load_config (gimp, alternate_system_gimprc, alternate_gimprc);

  /* We usually only save gimprc when Preferences are edited.
   * Thus we have to add a special flag when we make an update
   * check so that the timestamp is saved.
   */
  save_gimprc_at_exit = gimp_update_auto_check (gimp->config);

  /* Initialize the error handling after creating/migrating the config
   * directory because it will create some folders for backup and crash
   * logs in advance. Therefore running this before
   * gimp_user_install_new() would break migration as well as initial
   * folder creations.
   *
   * It also needs to be run after gimp_load_config() because error
   * handling is based on Preferences. It means that early loading code
   * is not handled by our debug code, but that's not a big deal.
   */
  errors_init (gimp, full_prog_name, use_debug_handler,
               stack_trace_mode, backtrace_file);

  /*  run the late-stage sanity check.  it's important that this check is run
   *  after the call to language_init() (see comment in sanity_check_late().)
   */
  abort_message = sanity_check_late ();
  if (abort_message)
    app_abort (no_interface, abort_message);

  /*  initialize lowlevel stuff  */
  gimp_gegl_init (gimp);

  /*  Connect our restore_after callback before gui_init() connects
   *  theirs, so ours runs first and can grab the initial monitor
   *  before the GUI's restore_after callback resets it.
   */
  g_signal_connect_after (gimp, "restore",
                          G_CALLBACK (app_restore_after_callback),
                          NULL);

#ifndef GIMP_CONSOLE_COMPILATION
  if (! no_interface)
    update_status_func = gui_init (gimp, no_splash);
#endif

  if (! update_status_func)
    update_status_func = app_init_update_noop;

  /*  Create all members of the global Gimp instance which need an already
   *  parsed gimprc, e.g. the data factories
   */
  gimp_initialize (gimp, update_status_func);

  /*  Load all data files
   */
  gimp_restore (gimp, update_status_func, &font_error);

  /*  enable autosave late so we don't autosave when the
   *  monitor resolution is set in gui_init()
   */
  gimp_rc_set_autosave (GIMP_RC (gimp->edit_config), TRUE);

  loop = run_loop = g_main_loop_new (NULL, FALSE);

  g_signal_connect_after (gimp, "exit",
                          G_CALLBACK (app_exit_after_callback),
                          &run_loop);

#ifndef GIMP_CONSOLE_COMPILATION
  if (run_loop && ! no_interface)
    {
      /* Before opening images from command line, check for salvaged images
       * and query interactively to know if we should recover or discard
       * them.
       */
      GList *recovered_files;
      GList *iter;

      recovered_files = errors_recovered ();
      if (recovered_files &&
          gui_recover (g_list_length (recovered_files)))
        {
          for (iter = recovered_files; iter; iter = iter->next)
            {
              GFile             *file;
              GimpImage         *image;
              GError            *error = NULL;
              GimpPDBStatusType  status;

              file = g_file_new_for_path (iter->data);
              image = file_open_with_display (gimp,
                                              gimp_get_user_context (gimp),
                                              NULL,
                                              file, as_new,
                                              initial_screen,
                                              initial_monitor,
                                              &status, &error);
              if (image)
                {
                  /* Break ties with the backup directory. */
                  gimp_image_set_file (image, NULL);
                  /* One of the rare exceptions where we should call
                   * gimp_image_dirty() directly instead of creating
                   * an undo. We want the image to be dirty from
                   * scratch, without anything to undo.
                   */
                  gimp_image_dirty (image, GIMP_DIRTY_IMAGE);
                }
              else
                {
                  g_error_free (error);
                }

              g_object_unref (file);
            }
        }
      /* Delete backup XCF images. */
      for (iter = recovered_files; iter; iter = iter->next)
        {
          g_unlink (iter->data);
        }
      g_list_free_full (recovered_files, g_free);
    }
#endif

  /*  Load the images given on the command-line. */
  if (filenames)
    {
      gint i;

      for (i = 0; filenames[i] != NULL; i++)
        {
          if (run_loop)
            {
              GFile *file = g_file_new_for_commandline_arg (filenames[i]);

              file_open_from_command_line (gimp, file, as_new,
                                           initial_screen,
                                           initial_monitor);

              g_object_unref (file);
            }
        }
    }
  if (font_error)
    {
      gimp_message_literal (gimp, NULL,
                            GIMP_MESSAGE_INFO,
                            font_error->message);
      g_error_free (font_error);
    }

  if (run_loop)
    gimp_batch_run (gimp, batch_interpreter, batch_commands);

  if (run_loop)
    {
      gimp_threads_leave (gimp);
      g_main_loop_run (loop);
      gimp_threads_enter (gimp);
    }

  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  if (save_gimprc_at_exit)
    gimp_rc_save (GIMP_RC (gimp->config));

  g_main_loop_unref (loop);

  gimp_gegl_exit (gimp);

  errors_exit ();

  g_object_unref (gimp);

  gimp_debug_instances ();

  gegl_exit ();
}


/*  private functions  */

static void
app_init_update_noop (const gchar *text1,
                      const gchar *text2,
                      gdouble      percentage)
{
  /*  deliberately do nothing  */
}

static void
app_restore_after_callback (Gimp               *gimp,
                            GimpInitStatusFunc  status_callback)
{
  /*  Getting the display name for a -1 display returns the initial
   *  monitor during startup. Need to call this from a restore_after
   *  callback, because before restore(), the GUI can't return anything,
   *  after after restore() the initial monitor gets reset.
   */
  g_free (gimp_get_display_name (gimp, -1, &initial_screen, &initial_monitor));
}

static gboolean
app_exit_after_callback (Gimp       *gimp,
                         gboolean    kill_it,
                         GMainLoop **loop)
{
  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  /*
   *  In stable releases, we simply call exit() here. This speeds up
   *  the process of quitting GIMP and also works around the problem
   *  that plug-ins might still be running.
   *
   *  In unstable releases, we shut down GIMP properly in an attempt
   *  to catch possible problems in our finalizers.
   */

#ifdef GIMP_UNSTABLE

  if (g_main_loop_is_running (*loop))
    g_main_loop_quit (*loop);

  *loop = NULL;

#else

  gimp_gegl_exit (gimp);

  gegl_exit ();

  exit (EXIT_SUCCESS);

#endif

  return FALSE;
}
