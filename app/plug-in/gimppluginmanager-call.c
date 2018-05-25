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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "plug-in-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
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
#include "plug-in-params.h"

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
                              NULL, plug_in_def->file);

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
                              NULL, plug_in_def->file);

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
                               GimpObject          *display)
{
  GimpValueArray *return_vals = NULL;
  GimpPlugIn     *plug_in;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (GIMP_IS_PDB_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);
  g_return_val_if_fail (display == NULL || GIMP_IS_OBJECT (display), NULL);

  plug_in = gimp_plug_in_new (manager, context, progress, procedure, NULL);

  if (plug_in)
    {
      GimpCoreConfig    *core_config    = manager->gimp->config;
      GimpGeglConfig    *gegl_config    = GIMP_GEGL_CONFIG (core_config);
      GimpDisplayConfig *display_config = GIMP_DISPLAY_CONFIG (core_config);
      GimpGuiConfig     *gui_config     = GIMP_GUI_CONFIG (core_config);
      GPConfig           config;
      GPProcRun          proc_run;
      gint               display_ID;
      GObject           *monitor;

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

      display_ID = display ? gimp_get_display_ID (manager->gimp, display) : -1;

      config.tile_width       = GIMP_PLUG_IN_TILE_WIDTH;
      config.tile_height      = GIMP_PLUG_IN_TILE_HEIGHT;
      config.shm_ID           = (manager->shm ?
                                 gimp_plug_in_shm_get_ID (manager->shm) : -1);
      config.check_size       = display_config->transparency_size;
      config.check_type       = display_config->transparency_type;
      config.show_help_button = (gui_config->use_help &&
                                 gui_config->show_help_button);
      config.use_cpu_accel    = manager->gimp->use_cpu_accel;
      config.use_opencl       = gegl_config->use_opencl;
      config.export_exif      = core_config->export_metadata_exif;
      config.export_xmp       = core_config->export_metadata_xmp;
      config.export_iptc      = core_config->export_metadata_iptc;
      config.show_tooltips    = gui_config->show_tooltips;
      config.gdisp_ID         = display_ID;
      config.app_name         = (gchar *) g_get_application_name ();
      config.wm_class         = (gchar *) gimp_get_program_class (manager->gimp);
      config.display_name     = gimp_get_display_name (manager->gimp,
                                                       display_ID,
                                                       &monitor,
                                                       &config.monitor_number);
      config.timestamp        = gimp_get_user_time (manager->gimp);

      proc_run.name    = GIMP_PROCEDURE (procedure)->original_name;
      proc_run.nparams = gimp_value_array_length (args);
      proc_run.params  = plug_in_args_to_params (args, FALSE);

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
          g_free (proc_run.params);

          g_object_unref (plug_in);

          return_vals = gimp_procedure_get_return_values (GIMP_PROCEDURE (procedure),
                                                          FALSE, error);
          g_error_free (error);

          return return_vals;
        }

      g_free (config.display_name);
      g_free (proc_run.params);

      /* If this is an extension,
       * wait for an installation-confirmation message
       */
      if (GIMP_PROCEDURE (procedure)->proc_type == GIMP_EXTENSION)
        {
          plug_in->ext_main_loop = g_main_loop_new (NULL, FALSE);

          gimp_threads_leave (manager->gimp);
          g_main_loop_run (plug_in->ext_main_loop);
          gimp_threads_enter (manager->gimp);

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

          gimp_threads_leave (manager->gimp);
          g_main_loop_run (proc_frame->main_loop);
          gimp_threads_enter (manager->gimp);

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

      proc_run.name    = GIMP_PROCEDURE (procedure)->original_name;
      proc_run.nparams = gimp_value_array_length (args);
      proc_run.params  = plug_in_args_to_params (args, FALSE);

      if (! gp_temp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
          ! gimp_wire_flush (plug_in->my_write, plug_in))
        {
          const gchar *name  = gimp_object_get_name (plug_in);
          GError      *error = g_error_new (GIMP_PLUG_IN_ERROR,
                                            GIMP_PLUG_IN_EXECUTION_FAILED,
                                            _("Failed to run plug-in \"%s\""),
                                            name);

          g_free (proc_run.params);
          gimp_plug_in_proc_frame_pop (plug_in);

          return_vals = gimp_procedure_get_return_values (GIMP_PROCEDURE (procedure),
                                                          FALSE, error);
          g_error_free (error);

          return return_vals;
        }
      gimp_allow_set_foreground_window (plug_in);

      g_free (proc_run.params);

      g_object_ref (plug_in);
      gimp_plug_in_proc_frame_ref (proc_frame);

      gimp_plug_in_main_loop (plug_in);

      /*  main_loop is quit and proc_frame is popped in
       *  gimp_plug_in_handle_temp_proc_return()
       */

      return_vals = gimp_plug_in_proc_frame_get_return_values (proc_frame);

      gimp_plug_in_proc_frame_unref (proc_frame, plug_in);
      g_object_unref (plug_in);
    }

  return return_vals;
}
