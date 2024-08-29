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

#include "config/gimprc.h"

#include "gegl/gimp-gegl.h"

#include "core/gimp.h"
#include "core/gimp-batch.h"
#include "core/gimp-user-install.h"
#include "core/gimpimage.h"

#include "file/file-open.h"

#ifndef GIMP_CONSOLE_COMPILATION
#include <gtk/gtk.h>

#include "dialogs/user-install-dialog.h"

#include "gui/gimpapp.h"
#include "gui/gui.h"
#endif

#include "app.h"
#include "errors.h"
#include "gimpconsoleapp.h"
#include "gimpcoreapp.h"
#include "language.h"
#include "sanity.h"
#include "gimp-debug.h"

#include "gimp-intl.h"
#include "gimp-update.h"


/*  local prototypes  */

static void       app_init_update_noop       (const gchar        *text1,
                                              const gchar        *text2,
                                              gdouble             percentage);
static void       app_activate_callback      (GimpCoreApp        *app,
                                              gpointer            user_data);
static void       app_restore_after_callback (Gimp               *gimp,
                                              GimpInitStatusFunc  status_callback);
static gboolean   app_exit_after_callback    (Gimp               *gimp,
                                              gboolean            kill_it,
                                              GApplication       *app);

#if 0
/*  left here as documentation how to do compat enums  */
GType gimp_convert_dither_type_compat_get_type (void); /* compat cruft */
#endif


/*  local variables  */

static GObject *initial_monitor = NULL;


/*  public functions  */

void
app_libs_init (GOptionContext *context,
               gboolean        no_interface)
{
#if 0
  GQuark quark;
#endif

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

#if 0
  /*  left here as documentation how to do compat enums  */

  /*  keep compat enum code in sync with pdb/enumcode.pl  */
  quark = g_quark_from_static_string ("gimp-compat-enum");

  g_type_set_qdata (GIMP_TYPE_CONVERT_DITHER_TYPE, quark,
                    (gpointer) gimp_convert_dither_type_compat_get_type ());
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

gint
app_run (const gchar         *full_prog_name,
         const gchar        **filenames,
         GFile               *alternate_system_gimprc,
         GFile               *alternate_gimprc,
         const gchar         *session_name,
         const gchar         *batch_interpreter,
         const gchar        **batch_commands,
         gboolean             quit,
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
  Gimp               *gimp           = NULL;
  GApplication       *app            = NULL;
  GFile              *default_folder = NULL;
  GFile              *gimpdir        = NULL;
  const gchar        *abort_message  = NULL;
  gint                retval         = EXIT_SUCCESS;

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

  g_clear_object (&default_folder);

#ifndef GIMP_CONSOLE_COMPILATION
  app = gimp_app_new (gimp, no_splash, quit, as_new, filenames, batch_interpreter, batch_commands);
#else
  app = gimp_console_app_new (gimp, quit, as_new, filenames, batch_interpreter, batch_commands);
#endif

  gimp->app = app;

  gimp_cpu_accel_set_use (use_cpu_accel);

  /*  Check if the user's gimp_directory exists */
  gimpdir = gimp_directory_file (NULL);

  if (g_file_query_file_type (gimpdir, G_FILE_QUERY_INFO_NONE, NULL) !=
      G_FILE_TYPE_DIRECTORY)
    {
      GimpUserInstall *install = gimp_user_install_new (G_OBJECT (gimp),
                                                        be_verbose);

#ifdef GIMP_CONSOLE_COMPILATION
      gimp_user_install_run (install, 1);
#else
      if (! (no_interface ?
             gimp_user_install_run (install, 1) :
             user_install_dialog_run (install)))
        exit (EXIT_FAILURE);
#endif

      gimp_user_install_free (install);
    }

  g_object_unref (gimpdir);

  gimp_load_config (gimp, alternate_system_gimprc, alternate_gimprc);

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

  g_signal_connect_after (gimp, "exit",
                          G_CALLBACK (app_exit_after_callback),
                          app);

  g_signal_connect (app, "activate",
                    G_CALLBACK (app_activate_callback),
                    NULL);
  retval = g_application_run (app, 0, NULL);

  if (! retval)
    retval = gimp_core_app_get_exit_status (GIMP_CORE_APP (app));

  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  g_clear_object (&app);

  gimp_gegl_exit (gimp);

  errors_exit ();

  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, TRUE);

  g_object_unref (gimp);

  gimp_debug_instances ();

  gegl_exit ();

  return retval;
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
app_activate_callback (GimpCoreApp *app,
                       gpointer     user_data)
{
  Gimp               *gimp               = NULL;
  GimpInitStatusFunc  update_status_func = NULL;
  const gchar       **filenames;
  const gchar        *current_language;
  const gchar        *system_lang_l10n   = NULL;
  gchar              *prev_language      = NULL;
  GError             *font_error         = NULL;
  gint                batch_retval;

  g_return_if_fail (GIMP_IS_CORE_APP (app));

  gimp = gimp_core_app_get_gimp (app);

  gimp_core_app_set_exit_status (app, EXIT_SUCCESS);

  /* Language was already initialized. I call this again only to get the
   * actual language information and the "System Language" string
   * localized in the actual system language.
   */
  current_language = language_init (NULL, &system_lang_l10n);
#ifndef GIMP_CONSOLE_COMPILATION
  if (! gimp->no_interface)
    update_status_func = gui_init (gimp, gimp_app_get_no_splash (GIMP_APP (app)),
                                   GIMP_APP (app), NULL, system_lang_l10n);
#endif

  if (! update_status_func)
    update_status_func = app_init_update_noop;

  /*  Create all members of the global Gimp instance which need an already
   *  parsed gimprc, e.g. the data factories
   */
  gimp_initialize (gimp, update_status_func);

  g_object_get (gimp->edit_config,
                "prev-language", &prev_language,
                NULL);

  gimp->query_all = (prev_language == NULL ||
                     g_strcmp0 (prev_language, current_language) != 0);
  g_free (prev_language);

  /*  Load all data files */
  gimp_restore (gimp, update_status_func, &font_error);

  /*  enable autosave late so we don't autosave when the
   *  monitor resolution is set in gui_init()
   */
  gimp_rc_set_autosave (GIMP_RC (gimp->edit_config), TRUE);

#ifndef GIMP_CONSOLE_COMPILATION
  if (! gimp->no_interface)
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
                                              file,
                                              gimp_core_app_get_as_new (app),
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

  /*  check for updates *after* enabling config autosave, so that the timestamp
   *  is saved
   */
  gimp_update_auto_check (gimp->edit_config, gimp);

  /* Setting properties to be used for the next run.  */
  g_object_set (gimp->edit_config,
                /* Set this after gimp_update_auto_check(). */
                "config-version", GIMP_VERSION,
                /* Set this after gimp_restore(). */
                "prev-language",  current_language,
                NULL);

  /*  Load the images given on the command-line. */
  filenames = gimp_core_app_get_filenames (app);
  if (filenames != NULL)
    {
      gint i;

      for (i = 0; filenames[i] != NULL; i++)
        {
          GFile *file = g_file_new_for_commandline_arg (filenames[i]);

          file_open_from_command_line (gimp, file,
                                       gimp_core_app_get_as_new (app),
                                       initial_monitor);

          g_object_unref (file);
        }
    }

  /* The software is now fully loaded and ready to be used and get
   * external input.
   */
  gimp->initialized = TRUE;

  if (font_error)
    {
      gimp_message_literal (gimp, NULL,
                            GIMP_MESSAGE_INFO,
                            font_error->message);
      g_error_free (font_error);
    }

  batch_retval = gimp_batch_run (gimp,
                                 gimp_core_app_get_batch_interpreter (app),
                                 gimp_core_app_get_batch_commands (app));

  if (gimp_core_app_get_quit (app))
    {
      /*  Only if we are in batch mode, we want to exit with the
       *  return value of the batch command.
       */
      gimp_core_app_set_exit_status (app, batch_retval);

      /* Emit the "exit" signal, but also properly free all images still
      * opened.
      */
      gimp_exit (gimp, TRUE);
    }
}

static void
app_restore_after_callback (Gimp               *gimp,
                            GimpInitStatusFunc  status_callback)
{
  gint dummy;

  /*  Getting the display name for a -1 display returns the initial
   *  monitor during startup. Need to call this from a restore_after
   *  callback, because before restore(), the GUI can't return anything,
   *  after after restore() the initial monitor gets reset.
   */
  g_free (gimp_get_display_name (gimp, -1, &initial_monitor, &dummy));
}

static gboolean
app_exit_after_callback (Gimp         *gimp,
                         gboolean      kill_it,
                         GApplication *app)
{
  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  /*
   *  In releases, we simply call exit() here. This speeds up the
   *  process of quitting GIMP and also works around the problem that
   *  plug-ins might still be running.
   *
   *  In unstable releases, we shut down GIMP properly in an attempt
   *  to catch possible problems in our finalizers.
   */

#ifdef GIMP_RELEASE

  gimp_gegl_exit (gimp);

  gegl_exit ();

  exit (gimp_core_app_get_exit_status (GIMP_CORE_APP (app)));

#else

  g_application_quit (G_APPLICATION (app));

#endif

  return FALSE;
}
