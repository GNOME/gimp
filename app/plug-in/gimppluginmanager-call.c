/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapluginmanager-call.c
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

#include "libligmabase/ligmabase.h"
#include "libligmabase/ligmaprotocol.h"
#include "libligmabase/ligmawire.h"

#include "libligma/ligmagpparams.h"

#include "plug-in-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmadisplay.h"
#include "core/ligmaprogress.h"

#include "pdb/ligmapdbcontext.h"

#include "ligmaplugin.h"
#include "ligmaplugin-message.h"
#include "ligmaplugindef.h"
#include "ligmapluginerror.h"
#include "ligmapluginmanager.h"
#define __YES_I_NEED_LIGMA_PLUG_IN_MANAGER_CALL__
#include "ligmapluginmanager-call.h"
#include "ligmapluginshm.h"
#include "ligmatemporaryprocedure.h"

#include "ligma-intl.h"


static void
ligma_allow_set_foreground_window (LigmaPlugIn *plug_in)
{
#ifdef G_OS_WIN32
  AllowSetForegroundWindow (GetProcessId (plug_in->pid));
#endif
}


/*  public functions  */

void
ligma_plug_in_manager_call_query (LigmaPlugInManager *manager,
                                 LigmaContext       *context,
                                 LigmaPlugInDef     *plug_in_def)
{
  LigmaPlugIn *plug_in;

  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (LIGMA_IS_PDB_CONTEXT (context));
  g_return_if_fail (LIGMA_IS_PLUG_IN_DEF (plug_in_def));

  plug_in = ligma_plug_in_new (manager, context, NULL,
                              NULL, plug_in_def->file);

  if (plug_in)
    {
      plug_in->plug_in_def = plug_in_def;

      if (ligma_plug_in_open (plug_in, LIGMA_PLUG_IN_CALL_QUERY, TRUE))
        {
          while (plug_in->open)
            {
              LigmaWireMessage msg;

              if (! ligma_wire_read_msg (plug_in->my_read, &msg, plug_in))
                {
                  ligma_plug_in_close (plug_in, TRUE);
                }
              else
                {
                  ligma_plug_in_handle_message (plug_in, &msg);
                  ligma_wire_destroy (&msg);
                }
            }
        }

      g_object_unref (plug_in);
    }
}

void
ligma_plug_in_manager_call_init (LigmaPlugInManager *manager,
                                LigmaContext       *context,
                                LigmaPlugInDef     *plug_in_def)
{
  LigmaPlugIn *plug_in;

  g_return_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager));
  g_return_if_fail (LIGMA_IS_PDB_CONTEXT (context));
  g_return_if_fail (LIGMA_IS_PLUG_IN_DEF (plug_in_def));

  plug_in = ligma_plug_in_new (manager, context, NULL,
                              NULL, plug_in_def->file);

  if (plug_in)
    {
      plug_in->plug_in_def = plug_in_def;

      if (ligma_plug_in_open (plug_in, LIGMA_PLUG_IN_CALL_INIT, TRUE))
        {
          while (plug_in->open)
            {
              LigmaWireMessage msg;

              if (! ligma_wire_read_msg (plug_in->my_read, &msg, plug_in))
                {
                  ligma_plug_in_close (plug_in, TRUE);
                }
              else
                {
                  ligma_plug_in_handle_message (plug_in, &msg);
                  ligma_wire_destroy (&msg);
                }
            }
        }

      g_object_unref (plug_in);
    }
}

LigmaValueArray *
ligma_plug_in_manager_call_run (LigmaPlugInManager   *manager,
                               LigmaContext         *context,
                               LigmaProgress        *progress,
                               LigmaPlugInProcedure *procedure,
                               LigmaValueArray      *args,
                               gboolean             synchronous,
                               LigmaDisplay         *display)
{
  LigmaValueArray *return_vals = NULL;
  LigmaPlugIn     *plug_in;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (LIGMA_IS_PDB_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (LIGMA_IS_PLUG_IN_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);
  g_return_val_if_fail (display == NULL || LIGMA_IS_DISPLAY (display), NULL);

  plug_in = ligma_plug_in_new (manager, context, progress, procedure, NULL);

  if (plug_in)
    {
      LigmaCoreConfig    *core_config    = manager->ligma->config;
      LigmaGeglConfig    *gegl_config    = LIGMA_GEGL_CONFIG (core_config);
      LigmaDisplayConfig *display_config = LIGMA_DISPLAY_CONFIG (core_config);
      LigmaGuiConfig     *gui_config     = LIGMA_GUI_CONFIG (core_config);
      GPConfig           config;
      GPProcRun          proc_run;
      gint               display_id;
      GObject           *monitor;
      GFile             *icon_theme_dir;

      if (! ligma_plug_in_open (plug_in, LIGMA_PLUG_IN_CALL_RUN, FALSE))
        {
          const gchar *name  = ligma_object_get_name (plug_in);
          GError      *error = g_error_new (LIGMA_PLUG_IN_ERROR,
                                            LIGMA_PLUG_IN_EXECUTION_FAILED,
                                            _("Failed to run plug-in \"%s\""),
                                            name);

          g_object_unref (plug_in);

          return_vals = ligma_procedure_get_return_values (LIGMA_PROCEDURE (procedure),
                                                          FALSE, error);
          g_error_free (error);

          return return_vals;
        }

      if (! display)
        display = ligma_context_get_display (context);

      display_id = display ? ligma_display_get_id (display) : -1;

      icon_theme_dir = ligma_get_icon_theme_dir (manager->ligma);

      config.tile_width           = LIGMA_PLUG_IN_TILE_WIDTH;
      config.tile_height          = LIGMA_PLUG_IN_TILE_HEIGHT;
      config.shm_id               = (manager->shm ?
                                     ligma_plug_in_shm_get_id (manager->shm) :
                                     -1);
      config.check_size           = display_config->transparency_size;
      config.check_type           = display_config->transparency_type;
      config.check_custom_color1  = display_config->transparency_custom_color1;
      config.check_custom_color2  = display_config->transparency_custom_color2;
      config.show_help_button     = (gui_config->use_help &&
                                     gui_config->show_help_button);
      config.use_cpu_accel        = manager->ligma->use_cpu_accel;
      config.use_opencl           = gegl_config->use_opencl;
      config.export_color_profile = core_config->export_color_profile;
      config.export_comment       = core_config->export_comment;
      config.export_exif          = core_config->export_metadata_exif;
      config.export_xmp           = core_config->export_metadata_xmp;
      config.export_iptc          = core_config->export_metadata_iptc;
      config.default_display_id   = display_id;
      config.app_name             = (gchar *) g_get_application_name ();
      config.wm_class             = (gchar *) ligma_get_program_class (manager->ligma);
      config.display_name         = ligma_get_display_name (manager->ligma,
                                                           display_id,
                                                           &monitor,
                                                           &config.monitor_number);
      config.timestamp            = ligma_get_user_time (manager->ligma);
      config.icon_theme_dir       = (icon_theme_dir ?
                                     g_file_get_path (icon_theme_dir) :
                                     NULL);
      config.tile_cache_size      = gegl_config->tile_cache_size;
      config.swap_path            = gegl_config->swap_path;
      config.swap_compression     = gegl_config->swap_compression;
      config.num_processors       = gegl_config->num_processors;

      proc_run.name     = (gchar *) ligma_object_get_name (procedure);
      proc_run.n_params = ligma_value_array_length (args);
      proc_run.params   = _ligma_value_array_to_gp_params (args, FALSE);

      if (! gp_config_write (plug_in->my_write, &config, plug_in)     ||
          ! gp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
          ! ligma_wire_flush (plug_in->my_write, plug_in))
        {
          const gchar *name  = ligma_object_get_name (plug_in);
          GError      *error = g_error_new (LIGMA_PLUG_IN_ERROR,
                                            LIGMA_PLUG_IN_EXECUTION_FAILED,
                                            _("Failed to run plug-in \"%s\""),
                                            name);

          g_free (config.display_name);
          g_free (config.icon_theme_dir);

          _ligma_gp_params_free (proc_run.params, proc_run.n_params, FALSE);

          g_object_unref (plug_in);

          return_vals = ligma_procedure_get_return_values (LIGMA_PROCEDURE (procedure),
                                                          FALSE, error);
          g_error_free (error);

          return return_vals;
        }

      g_free (config.display_name);
      g_free (config.icon_theme_dir);

      _ligma_gp_params_free (proc_run.params, proc_run.n_params, FALSE);

      /* If this is an extension,
       * wait for an installation-confirmation message
       */
      if (LIGMA_PROCEDURE (procedure)->proc_type == LIGMA_PDB_PROC_TYPE_EXTENSION)
        {
          plug_in->ext_main_loop = g_main_loop_new (NULL, FALSE);

          g_main_loop_run (plug_in->ext_main_loop);

          /*  main_loop is quit in ligma_plug_in_handle_extension_ack()  */

          g_clear_pointer (&plug_in->ext_main_loop, g_main_loop_unref);
        }

      /* If this plug-in is requested to run synchronously,
       * wait for its return values
       */
      if (synchronous)
        {
          LigmaPlugInProcFrame *proc_frame = &plug_in->main_proc_frame;

          proc_frame->main_loop = g_main_loop_new (NULL, FALSE);

          g_main_loop_run (proc_frame->main_loop);

          /*  main_loop is quit in ligma_plug_in_handle_proc_return()  */

          g_clear_pointer (&proc_frame->main_loop, g_main_loop_unref);

          return_vals = ligma_plug_in_proc_frame_get_return_values (proc_frame);
        }

      g_object_unref (plug_in);
    }

  return return_vals;
}

LigmaValueArray *
ligma_plug_in_manager_call_run_temp (LigmaPlugInManager      *manager,
                                    LigmaContext            *context,
                                    LigmaProgress           *progress,
                                    LigmaTemporaryProcedure *procedure,
                                    LigmaValueArray         *args)
{
  LigmaValueArray *return_vals = NULL;
  LigmaPlugIn     *plug_in;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN_MANAGER (manager), NULL);
  g_return_val_if_fail (LIGMA_IS_PDB_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (LIGMA_IS_TEMPORARY_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);

  plug_in = procedure->plug_in;

  if (plug_in)
    {
      LigmaPlugInProcFrame *proc_frame;
      GPProcRun            proc_run;

      proc_frame = ligma_plug_in_proc_frame_push (plug_in, context, progress,
                                                 procedure);

      proc_run.name     = (gchar *) ligma_object_get_name (procedure);
      proc_run.n_params = ligma_value_array_length (args);
      proc_run.params   = _ligma_value_array_to_gp_params (args, FALSE);

      if (! gp_temp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
          ! ligma_wire_flush (plug_in->my_write, plug_in))
        {
          const gchar *name  = ligma_object_get_name (plug_in);
          GError      *error = g_error_new (LIGMA_PLUG_IN_ERROR,
                                            LIGMA_PLUG_IN_EXECUTION_FAILED,
                                            _("Failed to run plug-in \"%s\""),
                                            name);


          _ligma_gp_params_free (proc_run.params, proc_run.n_params, FALSE);
          ligma_plug_in_proc_frame_pop (plug_in);

          return_vals = ligma_procedure_get_return_values (LIGMA_PROCEDURE (procedure),
                                                          FALSE, error);
          g_error_free (error);

          return return_vals;
        }

      ligma_allow_set_foreground_window (plug_in);

      _ligma_gp_params_free (proc_run.params, proc_run.n_params, FALSE);

      g_object_ref (plug_in);
      ligma_plug_in_proc_frame_ref (proc_frame);

      ligma_plug_in_main_loop (plug_in);

      /*  main_loop is quit and proc_frame is popped in
       *  ligma_plug_in_handle_temp_proc_return()
       */

      return_vals = ligma_plug_in_proc_frame_get_return_values (proc_frame);

      ligma_plug_in_proc_frame_unref (proc_frame, plug_in);
      g_object_unref (plug_in);
    }

  return return_vals;
}
