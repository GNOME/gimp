/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-call.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
/* Not include gtk/gtk.h and depend on gtk just for GDK_WINDOWING_QUARTZ macro.
 * __APPLE__ suffices, we only build for MacOS (vs iOS) and Quartz (vs X11)
 */
#import <Cocoa/Cocoa.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "libgimp/gimpgpparams.h"

#include "plug-in-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpdisplay.h"
#include "core/gimpprogress.h"

#include "pdb/gimppdbcontext.h"

#include "gimpplugin.h"
#include "gimpplugin-message.h"
#include "gimpplugindef.h"
#include "gimppluginerror.h"
#include "gimppluginmanager.h"
#define __YES_I_NEED_GIMP_PLUG_IN_MANAGER_CALL__
#include "gimppluginmanager-call.h"
#include "gimppluginshm.h"
#include "gimptemporaryprocedure.h"

#include "gimp-intl.h"


static void
gimp_allow_set_foreground_window (GimpPlugIn *plug_in)
{
#ifdef G_OS_WIN32
  AllowSetForegroundWindow (GetProcessId (plug_in->pid));
#endif
}


/*  public functions  */

void
gimp_plug_in_manager_call_query (GimpPlugInManager *manager,
                                 GimpContext       *context,
                                 GimpPlugInDef     *plug_in_def)
{
  GimpPlugIn *plug_in;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_PDB_CONTEXT (context));
  g_return_if_fail (GIMP_IS_PLUG_IN_DEF (plug_in_def));

  plug_in = gimp_plug_in_new (manager, context, NULL,
                              NULL, plug_in_def->file, NULL);

  if (plug_in)
    {
      plug_in->plug_in_def = plug_in_def;

      if (gimp_plug_in_open (plug_in, GIMP_PLUG_IN_CALL_QUERY, TRUE))
        {
          while (plug_in->open)
            {
              GimpWireMessage msg;

              if (! gimp_wire_read_msg (plug_in->my_read, &msg, plug_in))
                {
                  gimp_plug_in_close (plug_in, TRUE);
                }
              else
                {
                  gimp_plug_in_handle_message (plug_in, &msg);
                  gimp_wire_destroy (&msg);
                }
            }
        }

      g_object_unref (plug_in);
    }
}

void
gimp_plug_in_manager_call_init (GimpPlugInManager *manager,
                                GimpContext       *context,
                                GimpPlugInDef     *plug_in_def)
{
  GimpPlugIn *plug_in;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_PDB_CONTEXT (context));
  g_return_if_fail (GIMP_IS_PLUG_IN_DEF (plug_in_def));

  plug_in = gimp_plug_in_new (manager, context, NULL,
                              NULL, plug_in_def->file, NULL);

  if (plug_in)
    {
      plug_in->plug_in_def = plug_in_def;

      if (gimp_plug_in_open (plug_in, GIMP_PLUG_IN_CALL_INIT, TRUE))
        {
          while (plug_in->open)
            {
              GimpWireMessage msg;

              if (! gimp_wire_read_msg (plug_in->my_read, &msg, plug_in))
                {
                  gimp_plug_in_close (plug_in, TRUE);
                }
              else
                {
                  gimp_plug_in_handle_message (plug_in, &msg);
                  gimp_wire_destroy (&msg);
                }
            }
        }

      g_object_unref (plug_in);
    }
}

GimpValueArray *
gimp_plug_in_manager_call_run (GimpPlugInManager   *manager,
                               GimpContext         *context,
                               GimpProgress        *progress,
                               GimpPlugInProcedure *procedure,
                               GimpValueArray      *args,
                               gboolean             synchronous,
                               GimpDisplay         *display)
{
  GimpValueArray *return_vals = NULL;
  GimpPlugIn     *plug_in;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (GIMP_IS_PDB_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);
  g_return_val_if_fail (display == NULL || GIMP_IS_DISPLAY (display), NULL);

  if (! display)
    display = gimp_context_get_display (context);

  plug_in = gimp_plug_in_new (manager, context, progress, procedure, NULL, display);

  if (plug_in)
    {
      GimpCoreConfig    *core_config    = manager->gimp->config;
      GimpGeglConfig    *gegl_config    = GIMP_GEGL_CONFIG (core_config);
      GimpDisplayConfig *display_config = GIMP_DISPLAY_CONFIG (core_config);
      GimpGuiConfig     *gui_config     = GIMP_GUI_CONFIG (core_config);
      GPConfig           config;
      GPProcRun          proc_run;
      gint               display_id;
      GObject           *monitor;
      GFile             *icon_theme_dir;
      const Babl        *format;
      const guint8      *icc;
      gint               icc_length;

      if (! gimp_plug_in_open (plug_in, GIMP_PLUG_IN_CALL_RUN, FALSE))
        {
          const gchar *name  = gimp_object_get_name (plug_in);
          GError      *error = g_error_new (GIMP_PLUG_IN_ERROR,
                                            GIMP_PLUG_IN_EXECUTION_FAILED,
                                            _("Failed to run plug-in \"%s\""),
                                            name);

          g_object_unref (plug_in);

          return_vals = gimp_procedure_get_return_values (GIMP_PROCEDURE (procedure),
                                                          FALSE, error);
          g_error_free (error);

          return return_vals;
        }

      display_id = display ? gimp_display_get_id (display) : -1;

      icon_theme_dir = gimp_get_icon_theme_dir (manager->gimp);

      config.tile_width           = GIMP_PLUG_IN_TILE_WIDTH;
      config.tile_height          = GIMP_PLUG_IN_TILE_HEIGHT;
      config.shm_id               = (manager->shm ?
                                     gimp_plug_in_shm_get_id (manager->shm) :
                                     -1);
      config.check_size           = display_config->transparency_size;
      config.check_type           = display_config->transparency_type;

      format = gegl_color_get_format (display_config->transparency_custom_color1);
      config.check_custom_encoding1 = (gchar *) babl_format_get_encoding (format);
      config.check_custom_color1  = gegl_color_get_bytes (display_config->transparency_custom_color1, format);
      icc = (const guint8 *) babl_space_get_icc (babl_format_get_space (format), &icc_length);
      config.check_custom_icc1    = g_bytes_new (icc, (gsize) icc_length);

      format = gegl_color_get_format (display_config->transparency_custom_color2);
      config.check_custom_encoding2 = (gchar *) babl_format_get_encoding (format);
      config.check_custom_color2  = gegl_color_get_bytes (display_config->transparency_custom_color2, format);
      icc = (const guint8 *) babl_space_get_icc (babl_format_get_space (format), &icc_length);
      config.check_custom_icc2    = g_bytes_new (icc, (gsize) icc_length);

      config.show_help_button     = (gui_config->use_help &&
                                     gui_config->show_help_button);
      config.use_cpu_accel        = manager->gimp->use_cpu_accel;
      config.use_opencl           = gegl_config->use_opencl;
      config.export_color_profile = core_config->export_color_profile;
      config.export_comment       = core_config->export_comment;
      config.export_exif          = core_config->export_metadata_exif;
      config.export_xmp           = core_config->export_metadata_xmp;
      config.export_iptc          = core_config->export_metadata_iptc;
      config.update_metadata      = core_config->export_update_metadata;
      config.default_display_id   = display_id;
      config.app_name             = (gchar *) g_get_application_name ();
      config.wm_class             = (gchar *) gimp_get_program_class (manager->gimp);
      config.display_name         = gimp_get_display_name (manager->gimp,
                                                           display_id,
                                                           &monitor,
                                                           &config.monitor_number);
      config.timestamp            = gimp_get_user_time (manager->gimp);
      config.icon_theme_dir       = (icon_theme_dir ?
                                     g_file_get_path (icon_theme_dir) :
                                     NULL);
      config.tile_cache_size      = gegl_config->tile_cache_size;
      config.swap_path            = gegl_config->swap_path;
      config.swap_compression     = gegl_config->swap_compression;
      config.num_processors       = gegl_config->num_processors;

      proc_run.name     = (gchar *) gimp_object_get_name (procedure);
      proc_run.n_params = gimp_value_array_length (args);
      proc_run.params   = _gimp_value_array_to_gp_params (args, FALSE);

      if (! gp_config_write (plug_in->my_write, &config, plug_in)     ||
          ! gp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
          ! gimp_wire_flush (plug_in->my_write, plug_in))
        {
          const gchar *name  = gimp_object_get_name (plug_in);
          GError      *error = g_error_new (GIMP_PLUG_IN_ERROR,
                                            GIMP_PLUG_IN_EXECUTION_FAILED,
                                            _("Failed to run plug-in \"%s\""),
                                            name);

          g_free (config.display_name);
          g_free (config.icon_theme_dir);

          _gimp_gp_params_free (proc_run.params, proc_run.n_params, FALSE);

          g_object_unref (plug_in);

          return_vals = gimp_procedure_get_return_values (GIMP_PROCEDURE (procedure),
                                                          FALSE, error);
          g_error_free (error);

          return return_vals;
        }

      g_free (config.display_name);
      g_free (config.icon_theme_dir);
      g_bytes_unref (config.check_custom_color1);
      g_bytes_unref (config.check_custom_icc1);
      g_bytes_unref (config.check_custom_color2);
      g_bytes_unref (config.check_custom_icc2);

      _gimp_gp_params_free (proc_run.params, proc_run.n_params, FALSE);

      /* If this is an extension,
       * wait for an installation-confirmation message
       */
      if (GIMP_PROCEDURE (procedure)->proc_type == GIMP_PDB_PROC_TYPE_PERSISTENT)
        {
          plug_in->ext_main_loop = g_main_loop_new (NULL, FALSE);

          g_main_loop_run (plug_in->ext_main_loop);

          /*  main_loop is quit in gimp_plug_in_handle_extension_ack()  */

          g_clear_pointer (&plug_in->ext_main_loop, g_main_loop_unref);
        }

      /* If this plug-in is requested to run synchronously,
       * wait for its return values
       */
      if (synchronous)
        {
          GimpPlugInProcFrame *proc_frame = &plug_in->main_proc_frame;

          proc_frame->main_loop = g_main_loop_new (NULL, FALSE);

          g_main_loop_run (proc_frame->main_loop);

          /*  main_loop is quit in gimp_plug_in_handle_proc_return()  */

          g_clear_pointer (&proc_frame->main_loop, g_main_loop_unref);

          return_vals = gimp_plug_in_proc_frame_get_return_values (proc_frame);
        }

      g_object_unref (plug_in);
    }

  return return_vals;
}

GimpValueArray *
gimp_plug_in_manager_call_run_temp (GimpPlugInManager      *manager,
                                    GimpContext            *context,
                                    GimpProgress           *progress,
                                    GimpTemporaryProcedure *procedure,
                                    GimpValueArray         *args)
{
  GimpValueArray *return_vals = NULL;
  GimpPlugIn     *plug_in;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (GIMP_IS_PDB_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_TEMPORARY_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);

  plug_in = procedure->plug_in;

  if (plug_in)
    {
      GimpPlugInProcFrame *proc_frame;
      GPProcRun            proc_run;

      proc_frame = gimp_plug_in_proc_frame_push (plug_in, context, progress,
                                                 procedure);

      proc_run.name     = (gchar *) gimp_object_get_name (procedure);
      proc_run.n_params = gimp_value_array_length (args);
      proc_run.params   = _gimp_value_array_to_gp_params (args, FALSE);

      if (! gp_temp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
          ! gimp_wire_flush (plug_in->my_write, plug_in))
        {
          const gchar *name  = gimp_object_get_name (plug_in);
          GError      *error = g_error_new (GIMP_PLUG_IN_ERROR,
                                            GIMP_PLUG_IN_EXECUTION_FAILED,
                                            _("Failed to run plug-in \"%s\""),
                                            name);


          _gimp_gp_params_free (proc_run.params, proc_run.n_params, FALSE);
          gimp_plug_in_proc_frame_pop (plug_in);

          return_vals = gimp_procedure_get_return_values (GIMP_PROCEDURE (procedure),
                                                          FALSE, error);
          g_error_free (error);

          return return_vals;
        }

      gimp_allow_set_foreground_window (plug_in);

      _gimp_gp_params_free (proc_run.params, proc_run.n_params, FALSE);

      g_object_ref (plug_in);
      gimp_plug_in_proc_frame_ref (proc_frame);

      gimp_plug_in_main_loop (plug_in);

#ifdef __APPLE__
      /* The plugin temporary procedure returned, from separate, active process.
      * Ensure gimp app active.
      * Usually extension-script-fu was active, except when the temporary procedure
      * was say a callback from a Resource chooser dialog.
      * In that case, when the chooser is closing, it would be better
      * to activate the plugin, avoiding an extra click by the user.
      * We can't do that here, unless we use the more cooperative API of MacOS.
      */
      [NSApp activateIgnoringOtherApps:YES];
#endif

      /*  main_loop is quit and proc_frame is popped in
       *  gimp_plug_in_handle_temp_proc_return()
       */

      return_vals = gimp_plug_in_proc_frame_get_return_values (proc_frame);

      gimp_plug_in_proc_frame_unref (proc_frame, plug_in);
      g_object_unref (plug_in);
    }

  return return_vals;
}
