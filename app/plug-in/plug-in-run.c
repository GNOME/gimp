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

#include "pdb/procedural_db.h"

#include "plug-in.h"
#include "plug-in-params.h"
#include "plug-in-proc-def.h"
#include "plug-in-run.h"
#include "plug-in-shm.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static Argument * plug_in_temp_run        (ProcRecord      *proc_rec,
                                           GimpContext     *context,
                                           GimpProgress    *progress,
                                           Argument        *args,
                                           gint             argc);
static Argument * plug_in_get_return_vals (PlugIn          *plug_in,
                                           PlugInProcFrame *proc_frame);


/*  public functions  */

Argument *
plug_in_run (Gimp         *gimp,
             GimpContext  *context,
             GimpProgress *progress,
             ProcRecord   *proc_rec,
	     Argument     *args,
	     gint          argc,
	     gboolean      synchronous,
	     gboolean      destroy_return_vals,
	     gint          gdisp_ID)
{
  Argument *return_vals = NULL;
  PlugIn   *plug_in;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (proc_rec != NULL, NULL);
  g_return_val_if_fail (argc == 0 || args != NULL, NULL);

  if (proc_rec->proc_type == GIMP_TEMPORARY)
    {
      return_vals = plug_in_temp_run (proc_rec, context, progress, args, argc);
      goto done;
    }

  plug_in = plug_in_new (gimp, context, progress, proc_rec,
                         proc_rec->exec_method.plug_in.filename);

  if (plug_in)
    {
      GimpDisplayConfig *display_config = GIMP_DISPLAY_CONFIG (gimp->config);
      GimpGuiConfig     *gui_config = GIMP_GUI_CONFIG (gimp->config);
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
      config.show_tool_tips   = gui_config->show_tool_tips;
      config.min_colors       = CLAMP (gimp->config->min_colors, 27, 256);
      config.gdisp_ID         = gdisp_ID;
      config.app_name         = (gchar *) g_get_application_name ();
      config.wm_class         = (gchar *) gimp_get_program_class (gimp);
      config.display_name     = gimp_get_display_name (gimp, gdisp_ID, &monitor);
      config.monitor_number   = monitor;

      proc_run.name    = proc_rec->original_name;
      proc_run.nparams = argc;
      proc_run.params  = plug_in_args_to_params (args, argc, FALSE);

      if (! gp_config_write (plug_in->my_write, &config, plug_in)     ||
          ! gp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
          ! gimp_wire_flush (plug_in->my_write, plug_in))
        {
          return_vals = procedural_db_return_args (proc_rec, FALSE);

          g_free (config.display_name);

          goto done;
        }

      g_free (config.display_name);

      plug_in_params_destroy (proc_run.params, proc_run.nparams, FALSE);

      plug_in_ref (plug_in);

      /* If this is an extension,
       * wait for an installation-confirmation message
       */
      if (proc_rec->proc_type == GIMP_EXTENSION)
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
                                                 &plug_in->main_proc_frame);
        }

      plug_in_unref (plug_in);
    }

 done:
  if (return_vals && destroy_return_vals)
    {
      procedural_db_destroy_args (return_vals, proc_rec->num_values);
      return_vals = NULL;
    }

  return return_vals;
}

void
plug_in_repeat (Gimp         *gimp,
                GimpContext  *context,
                GimpProgress *progress,
                gint          display_ID,
                gint          image_ID,
                gint          drawable_ID,
                gboolean      with_interface)
{
  Argument *args;
  gint      i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (gimp->last_plug_in)
    {
      /* construct the procedures arguments */
      args = g_new (Argument, 3);

      /* initialize the first three argument types */
      for (i = 0; i < 3; i++)
	args[i].arg_type = gimp->last_plug_in->db_info.args[i].arg_type;

      /* initialize the first three plug-in arguments  */
      args[0].value.pdb_int = (with_interface ?
                               GIMP_RUN_INTERACTIVE : GIMP_RUN_WITH_LAST_VALS);
      args[1].value.pdb_int = image_ID;
      args[2].value.pdb_int = drawable_ID;

      /* run the plug-in procedure */
      plug_in_run (gimp, context, progress, &gimp->last_plug_in->db_info,
                   args, 3, FALSE, TRUE, display_ID);

      g_free (args);
    }
}


/*  private functions  */

static Argument *
plug_in_temp_run (ProcRecord   *proc_rec,
                  GimpContext  *context,
                  GimpProgress *progress,
		  Argument     *args,
		  gint          argc)
{
  Argument *return_vals = NULL;
  PlugIn   *plug_in;

  plug_in = (PlugIn *) proc_rec->exec_method.temporary.plug_in;

  if (plug_in)
    {
      PlugInProcFrame *proc_frame;
      GPProcRun        proc_run;

      proc_frame = plug_in_proc_frame_push (plug_in, context, progress,
                                            proc_rec);

      proc_run.name    = proc_rec->original_name;
      proc_run.nparams = argc;
      proc_run.params  = plug_in_args_to_params (args, argc, FALSE);

      if (! gp_temp_proc_run_write (plug_in->my_write, &proc_run, plug_in) ||
	  ! gimp_wire_flush (plug_in->my_write, plug_in))
	{
	  return_vals = procedural_db_return_args (proc_rec, FALSE);

          plug_in_proc_frame_pop (plug_in);

	  goto done;
	}

      plug_in_params_destroy (proc_run.params, proc_run.nparams, FALSE);

      plug_in_ref (plug_in);
      plug_in_proc_frame_ref (proc_frame);

      plug_in_main_loop (plug_in);

      return_vals = plug_in_get_return_vals (plug_in, proc_frame);

      /*  main_loop is quit and proc_frame is popped in
       *  plug_in_handle_temp_proc_return()
       */

      plug_in_proc_frame_unref (proc_frame, plug_in);
      plug_in_unref (plug_in);
    }

 done:
  return return_vals;
}

static Argument *
plug_in_get_return_vals (PlugIn          *plug_in,
                         PlugInProcFrame *proc_frame)
{
  Argument *return_vals;
  gint      nargs;

  g_return_val_if_fail (plug_in != NULL, NULL);
  g_return_val_if_fail (proc_frame != NULL, NULL);

  /* Return the status code plus the current return values. */
  nargs = proc_frame->proc_rec->num_values + 1;

  if (proc_frame->return_vals && proc_frame->n_return_vals == nargs)
    {
      return_vals = proc_frame->return_vals;
    }
  else if (proc_frame->return_vals)
    {
      /* Allocate new return values of the correct size. */
      return_vals = procedural_db_return_args (proc_frame->proc_rec, FALSE);

      /* Copy all of the arguments we can. */
      memcpy (return_vals, proc_frame->return_vals,
	      sizeof (Argument) * MIN (proc_frame->n_return_vals, nargs));

      /* Free the old argument pointer.  This will cause a memory leak
       * only if there were more values returned than we need (which
       * shouldn't ever happen).
       */
      g_free (proc_frame->return_vals);
    }
  else
    {
      /* Just return a dummy set of values. */
      return_vals = procedural_db_return_args (proc_frame->proc_rec, FALSE);
    }

  /* We have consumed any saved values, so clear them. */
  proc_frame->return_vals   = NULL;
  proc_frame->n_return_vals = 0;

  return return_vals;
}
