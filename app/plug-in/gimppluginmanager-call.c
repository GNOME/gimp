/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginmanager-call.c
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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "plug-in-types.h"

#include "config/gimpguiconfig.h"

#include "base/tile.h"

#include "composite/gimp-composite.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpprogress.h"

#include "gimpplugin.h"
#include "gimpplugin-message.h"
#include "gimpplugindef.h"
#include "gimppluginmanager.h"
#define __YES_I_NEED_GIMP_PLUG_IN_MANAGER_CALL__
#include "gimppluginmanager-call.h"
#include "gimppluginshm.h"
#include "gimptemporaryprocedure.h"
#include "plug-in-params.h"

#include "gimp-intl.h"


/*  public functions  */

void
gimp_plug_in_manager_call_query (GimpPlugInManager *manager,
                                 GimpContext       *context,
                                 GimpPlugInDef     *plug_in_def)
{
  GimpPlugIn *plug_in;

  g_return_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (GIMP_IS_PLUG_IN_DEF (plug_in_def));

  plug_in = gimp_plug_in_new (manager, context, NULL,
                              NULL, plug_in_def->prog);

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
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (GIMP_IS_PLUG_IN_DEF (plug_in_def));

  plug_in = gimp_plug_in_new (manager, context, NULL,
                              NULL, plug_in_def->prog);

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

GValueArray *
gimp_plug_in_manager_call_run (GimpPlugInManager   *manager,
                               GimpContext         *context,
                               GimpProgress        *progress,
                               GimpPlugInProcedure *procedure,
                               GValueArray         *args,
                               gboolean             synchronous,
                               gboolean             destroy_return_vals,
                               GimpObject          *display)
{
  GValueArray *return_vals = NULL;
  GimpPlugIn  *plug_in;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);
  g_return_val_if_fail (display == NULL || GIMP_IS_OBJECT (display), NULL);

  plug_in = gimp_plug_in_new (manager, context, progress,
                              procedure, NULL);

  if (plug_in)
    {
      GimpCoreConfig    *core_config    = manager->gimp->config;
      GimpDisplayConfig *display_config = GIMP_DISPLAY_CONFIG (core_config);
      GimpGuiConfig     *gui_config     = GIMP_GUI_CONFIG (core_config);
      GPConfig           config;
      GPProcRun          proc_run;
      gint               display_ID;
      gint               monitor;

      if (! gimp_plug_in_open (plug_in, GIMP_PLUG_IN_CALL_RUN, FALSE))
        {
          g_object_unref (plug_in);
          goto done;
        }

      display_ID = display ? gimp_get_display_ID (manager->gimp, display) : -1;

      config.version          = GIMP_PROTOCOL_VERSION;
      config.tile_width       = TILE_WIDTH;
      config.tile_height      = TILE_HEIGHT;
      config.shm_ID           = (manager->shm ?
                                 gimp_plug_in_shm_get_ID (manager->shm) : -1);
      config.check_size       = display_config->transparency_size;
      config.check_type       = display_config->transparency_type;
      config.show_help_button = (gui_config->use_help &&
                                 gui_config->show_help_button);
      config.use_cpu_accel    = gimp_composite_use_cpu_accel ();
      config.gimp_reserved_5  = 0;
      config.gimp_reserved_6  = 0;
      config.gimp_reserved_7  = 0;
      config.gimp_reserved_8  = 0;
      config.install_cmap     = core_config->install_cmap;
      config.show_tooltips    = gui_config->show_tooltips;
      config.min_colors       = CLAMP (core_config->min_colors, 27, 256);
      config.gdisp_ID         = display_ID;
      config.app_name         = (gchar *) g_get_application_name ();
      config.wm_class         = (gchar *) gimp_get_program_class (manager->gimp);
      config.display_name     = gimp_get_display_name (manager->gimp,
                                                       display_ID, &monitor);
      config.monitor_number   = monitor;

      proc_run.name    = GIMP_PROCEDURE (procedure)->original_name;
      proc_run.nparams = args->n_values;
      proc_run.params  = plug_in_args_to_params (args, FALSE);

      if (! gp_config_write (plug_in->my_write, &config, plug_in)     ||
          ! gp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
          ! gimp_wire_flush (plug_in->my_write, plug_in))
        {
          g_free (config.display_name);
          g_free (proc_run.params);

          return_vals =
            gimp_procedure_get_return_values (GIMP_PROCEDURE (procedure), FALSE);

          g_object_unref (plug_in);
          goto done;
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

          g_main_loop_unref (plug_in->ext_main_loop);
          plug_in->ext_main_loop = NULL;
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

          g_main_loop_unref (proc_frame->main_loop);
          proc_frame->main_loop = NULL;

          return_vals = gimp_plug_in_proc_frame_get_return_vals (proc_frame);
        }

      g_object_unref (plug_in);
    }

 done:
  if (return_vals && destroy_return_vals)
    {
      g_value_array_free (return_vals);
      return_vals = NULL;
    }

  return return_vals;
}

GValueArray *
gimp_plug_in_manager_call_run_temp (GimpPlugInManager      *manager,
                                    GimpContext            *context,
                                    GimpProgress           *progress,
                                    GimpTemporaryProcedure *procedure,
                                    GValueArray            *args)
{
  GValueArray *return_vals = NULL;
  GimpPlugIn  *plug_in;

  g_return_val_if_fail (GIMP_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
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
      proc_run.nparams = args->n_values;
      proc_run.params  = plug_in_args_to_params (args, FALSE);

      if (! gp_temp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
          ! gimp_wire_flush (plug_in->my_write, plug_in))
        {
          g_free (proc_run.params);
          gimp_plug_in_proc_frame_pop (plug_in);

          return_vals =
            gimp_procedure_get_return_values (GIMP_PROCEDURE (procedure), FALSE);

          goto done;
        }

      g_free (proc_run.params);

      g_object_ref (plug_in);
      gimp_plug_in_proc_frame_ref (proc_frame);

      gimp_plug_in_main_loop (plug_in);

      /*  main_loop is quit and proc_frame is popped in
       *  gimp_plug_in_handle_temp_proc_return()
       */

      return_vals = gimp_plug_in_proc_frame_get_return_vals (proc_frame);

      gimp_plug_in_proc_frame_unref (proc_frame, plug_in);
      g_object_unref (plug_in);
    }

 done:
  return return_vals;
}
