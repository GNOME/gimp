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

#include <string.h>

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

#include "pdb/gimpargument.h"
#include "pdb/gimpprocedure.h"

#include "plug-in.h"
#include "plug-in-params.h"
#include "plug-in-proc-def.h"
#include "plug-in-run.h"
#include "plug-in-shm.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GimpArgument * plug_in_temp_run        (GimpProcedure   *procedure,
                                               GimpContext     *context,
                                               GimpProgress    *progress,
                                               GimpArgument    *args,
                                               gint             n_args);
static GimpArgument * plug_in_get_return_vals (PlugIn          *plug_in,
                                               PlugInProcFrame *proc_frame,
                                               gint            *n_return_vals);


/*  public functions  */

GimpArgument *
plug_in_run (Gimp          *gimp,
             GimpContext   *context,
             GimpProgress  *progress,
             GimpProcedure *procedure,
	     GimpArgument  *args,
	     gint           n_args,
	     gboolean       synchronous,
	     gboolean       destroy_return_vals,
	     gint           display_ID)
{
  GimpArgument *return_vals   = NULL;
  gint          n_return_vals = 0;
  PlugIn       *plug_in;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_args == 0 || args != NULL, NULL);

  if (procedure->proc_type == GIMP_TEMPORARY)
    {
      return_vals = plug_in_temp_run (procedure, context, progress,
                                      args, n_args);
      goto done;
    }

  plug_in = plug_in_new (gimp, context, progress, procedure,
                         procedure->exec_method.plug_in.filename);

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

      proc_run.name    = procedure->original_name;
      proc_run.nparams = n_args;
      proc_run.params  = plug_in_args_to_params (args, n_args, FALSE);

      if (! gp_config_write (plug_in->my_write, &config, plug_in)     ||
          ! gp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
          ! gimp_wire_flush (plug_in->my_write, plug_in))
        {
          return_vals = gimp_procedure_get_return_values (procedure, FALSE);

          g_free (config.display_name);

          goto done;
        }

      g_free (config.display_name);

      plug_in_params_destroy (proc_run.params, proc_run.nparams, FALSE);

      plug_in_ref (plug_in);

      /* If this is an extension,
       * wait for an installation-confirmation message
       */
      if (procedure->proc_type == GIMP_EXTENSION)
        {
          plug_in->ext_main_loop = g_main_loop_new (NULL, FALSE);

          gimp_threads_leave (gimp);
          g_main_loop_run (plug_in->ext_main_loop);
          gimp_threads_enter (gimp);

          g_main_loop_unref (plug_in->ext_main_loop);
          plug_in->ext_main_loop = NULL;
        }

      /* If this plug-in is requested to run synchronously,
       * wait for its return values
       */
      if (synchronous)
        {
          plug_in->main_proc_frame.main_loop = g_main_loop_new (NULL, FALSE);

          gimp_threads_leave (gimp);
          g_main_loop_run (plug_in->main_proc_frame.main_loop);
          gimp_threads_enter (gimp);

          g_main_loop_unref (plug_in->main_proc_frame.main_loop);
          plug_in->main_proc_frame.main_loop = NULL;

          return_vals = plug_in_get_return_vals (plug_in,
                                                 &plug_in->main_proc_frame,
                                                 &n_return_vals);
        }

      plug_in_unref (plug_in);
    }

 done:
  if (return_vals && destroy_return_vals)
    {
      gimp_arguments_destroy (return_vals, procedure->num_values, TRUE);
      return_vals = NULL;
    }

  return return_vals;
}

void
plug_in_repeat (Gimp         *gimp,
                gint          index,
                GimpContext  *context,
                GimpProgress *progress,
                gint          display_ID,
                gint          image_ID,
                gint          drawable_ID,
                gboolean      with_interface)
{
  PlugInProcDef *proc_def;
  GimpArgument  *args;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (index >= 0);
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  proc_def = g_slist_nth_data (gimp->last_plug_ins, index);

  if (proc_def)
    {
      args = gimp_procedure_get_arguments (proc_def->procedure);

      g_value_set_int (&args[0].value,
                       with_interface ?
                       GIMP_RUN_INTERACTIVE : GIMP_RUN_WITH_LAST_VALS);
      g_value_set_int (&args[1].value, image_ID);
      g_value_set_int (&args[2].value, drawable_ID);

      /* run the plug-in procedure */
      plug_in_run (gimp, context, progress, proc_def->procedure,
                   args, 3 /* not proc_def->procedure->num_args */,
                   FALSE, TRUE, display_ID);

      gimp_arguments_destroy (args, proc_def->procedure->num_args, TRUE);
    }
}


/*  private functions  */

static GimpArgument *
plug_in_temp_run (GimpProcedure *procedure,
                  GimpContext   *context,
                  GimpProgress  *progress,
		  GimpArgument  *args,
		  gint           n_args)
{
  GimpArgument *return_vals   = NULL;
  gint          n_return_vals = 0;
  PlugIn       *plug_in;

  plug_in = (PlugIn *) procedure->exec_method.temporary.plug_in;

  if (plug_in)
    {
      PlugInProcFrame *proc_frame;
      GPProcRun        proc_run;

      proc_frame = plug_in_proc_frame_push (plug_in, context, progress,
                                            procedure);

      proc_run.name    = procedure->original_name;
      proc_run.nparams = n_args;
      proc_run.params  = plug_in_args_to_params (args, n_args, FALSE);

      if (! gp_temp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
	  ! gimp_wire_flush (plug_in->my_write, plug_in))
	{
	  return_vals = gimp_procedure_get_return_values (procedure, FALSE);

          plug_in_proc_frame_pop (plug_in);

	  goto done;
	}

      plug_in_params_destroy (proc_run.params, proc_run.nparams, FALSE);

      plug_in_ref (plug_in);
      plug_in_proc_frame_ref (proc_frame);

      plug_in_main_loop (plug_in);

      return_vals = plug_in_get_return_vals (plug_in, proc_frame,
                                             &n_return_vals);

      /*  main_loop is quit and proc_frame is popped in
       *  plug_in_handle_temp_proc_return()
       */

      plug_in_proc_frame_unref (proc_frame, plug_in);
      plug_in_unref (plug_in);
    }

 done:
  return return_vals;
}

static GimpArgument *
plug_in_get_return_vals (PlugIn          *plug_in,
                         PlugInProcFrame *proc_frame,
                         gint            *n_return_vals)
{
  GimpArgument *return_vals;

  g_return_val_if_fail (plug_in != NULL, NULL);
  g_return_val_if_fail (proc_frame != NULL, NULL);
  g_return_val_if_fail (n_return_vals != NULL, NULL);

  /* Return the status code plus the current return values. */
  *n_return_vals = proc_frame->procedure->num_values + 1;

  if (proc_frame->return_vals &&
      proc_frame->n_return_vals == *n_return_vals)
    {
      return_vals = proc_frame->return_vals;
    }
  else if (proc_frame->return_vals)
    {
      /* Allocate new return values of the correct size. */
      return_vals = gimp_procedure_get_return_values (proc_frame->procedure,
                                                      FALSE);

      /* Copy all of the arguments we can. */
      memcpy (return_vals, proc_frame->return_vals,
	      sizeof (GimpArgument) * MIN (proc_frame->n_return_vals,
                                           *n_return_vals));

      /* Free the old argument pointer.  This will cause a memory leak
       * only if there were more values returned than we need (which
       * shouldn't ever happen).
       */
      g_free (proc_frame->return_vals);
    }
  else
    {
      /* Just return a dummy set of values. */
      return_vals = gimp_procedure_get_return_values (proc_frame->procedure,
                                                      FALSE);
    }

  /* We have consumed any saved values, so clear them. */
  proc_frame->return_vals   = NULL;
  proc_frame->n_return_vals = 0;

  return return_vals;
}
