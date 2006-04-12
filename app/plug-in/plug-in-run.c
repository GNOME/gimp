/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-run.c
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

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpprogress.h"

#include "pdb/gimptemporaryprocedure.h"

#include "plug-in.h"
#include "plug-in-params.h"
#define __YES_I_NEED_PLUG_IN_RUN__
#include "plug-in-run.h"
#include "plug-in-shm.h"

#include "gimp-intl.h"


/*  public functions  */

GValueArray *
plug_in_run (Gimp                *gimp,
             GimpContext         *context,
             GimpProgress        *progress,
             GimpPlugInProcedure *procedure,
             GValueArray         *args,
             gboolean             synchronous,
             gboolean             destroy_return_vals,
             gint                 display_ID)
{
  GValueArray *return_vals = NULL;
  PlugIn      *plug_in;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);

  plug_in = plug_in_new (gimp, context, progress, GIMP_PROCEDURE (procedure),
                         procedure->prog);

  if (plug_in)
    {
      GimpDisplayConfig *display_config = GIMP_DISPLAY_CONFIG (gimp->config);
      GimpGuiConfig     *gui_config     = GIMP_GUI_CONFIG (gimp->config);
      GPConfig           config;
      GPProcRun          proc_run;
      gint               monitor;

      if (! plug_in_open (plug_in))
        {
          plug_in_unref (plug_in);
          goto done;
        }

      config.version          = GIMP_PROTOCOL_VERSION;
      config.tile_width       = TILE_WIDTH;
      config.tile_height      = TILE_HEIGHT;
      config.shm_ID           = plug_in_shm_get_ID (gimp);
      config.check_size       = display_config->transparency_size;
      config.check_type       = display_config->transparency_type;
      config.show_help_button = (gui_config->use_help &&
                                 gui_config->show_help_button);
      config.gimp_reserved_4  = 0;
      config.gimp_reserved_5  = 0;
      config.gimp_reserved_6  = 0;
      config.gimp_reserved_7  = 0;
      config.gimp_reserved_8  = 0;
      config.install_cmap     = gimp->config->install_cmap;
      config.show_tooltips    = gui_config->show_tooltips;
      config.min_colors       = CLAMP (gimp->config->min_colors, 27, 256);
      config.gdisp_ID         = display_ID;
      config.app_name         = (gchar *) g_get_application_name ();
      config.wm_class         = (gchar *) gimp_get_program_class (gimp);
      config.display_name     = gimp_get_display_name (gimp,
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

          goto done;
        }

      g_free (config.display_name);
      g_free (proc_run.params);

      plug_in_ref (plug_in);

      /* If this is an extension,
       * wait for an installation-confirmation message
       */
      if (GIMP_PROCEDURE (procedure)->proc_type == GIMP_EXTENSION)
        {
          plug_in->ext_main_loop = g_main_loop_new (NULL, FALSE);

          gimp_threads_leave (gimp);
          g_main_loop_run (plug_in->ext_main_loop);
          gimp_threads_enter (gimp);

          /*  main_loop is quit in plug_in_handle_extension_ack()  */

          g_main_loop_unref (plug_in->ext_main_loop);
          plug_in->ext_main_loop = NULL;
        }

      /* If this plug-in is requested to run synchronously,
       * wait for its return values
       */
      if (synchronous)
        {
          PlugInProcFrame *proc_frame = &plug_in->main_proc_frame;

          proc_frame->main_loop = g_main_loop_new (NULL, FALSE);

          gimp_threads_leave (gimp);
          g_main_loop_run (proc_frame->main_loop);
          gimp_threads_enter (gimp);

          /*  main_loop is quit in plug_in_handle_proc_return()  */

          g_main_loop_unref (proc_frame->main_loop);
          proc_frame->main_loop = NULL;

          return_vals = plug_in_proc_frame_get_return_vals (proc_frame);
        }

      plug_in_unref (plug_in);
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
plug_in_run_temp (Gimp                   *gimp,
                  GimpContext            *context,
                  GimpProgress           *progress,
                  GimpTemporaryProcedure *procedure,
                  GValueArray            *args)
{
  GValueArray *return_vals = NULL;
  PlugIn      *plug_in;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_TEMPORARY_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);

  plug_in = procedure->plug_in;

  if (plug_in)
    {
      PlugInProcFrame *proc_frame;
      GPProcRun        proc_run;

      proc_frame = plug_in_proc_frame_push (plug_in, context, progress,
                                            GIMP_PROCEDURE (procedure));

      proc_run.name    = GIMP_PROCEDURE (procedure)->original_name;
      proc_run.nparams = args->n_values;
      proc_run.params  = plug_in_args_to_params (args, FALSE);

      if (! gp_temp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
          ! gimp_wire_flush (plug_in->my_write, plug_in))
        {
          g_free (proc_run.params);
          plug_in_proc_frame_pop (plug_in);

          return_vals =
            gimp_procedure_get_return_values (GIMP_PROCEDURE (procedure), FALSE);

          goto done;
        }

      g_free (proc_run.params);

      plug_in_ref (plug_in);
      plug_in_proc_frame_ref (proc_frame);

      plug_in_main_loop (plug_in);

      /*  main_loop is quit and proc_frame is popped in
       *  plug_in_handle_temp_proc_return()
       */

      return_vals = plug_in_proc_frame_get_return_vals (proc_frame);

      plug_in_proc_frame_unref (proc_frame, plug_in);
      plug_in_unref (plug_in);
    }

 done:
  return return_vals;
}
