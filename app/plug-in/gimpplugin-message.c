/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin-message.c
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

#include "base/tile.h"
#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"

#include "pdb/gimppdb.h"
#include "pdb/gimp-pdb-compat.h"

#include "gimpplugin.h"
#include "gimpplugin-message.h"
#include "gimppluginmanager.h"
#include "gimpplugindef.h"
#include "gimppluginshm.h"
#include "gimptemporaryprocedure.h"
#include "plug-in-params.h"


/*  local function prototypes  */

static void gimp_plug_in_handle_quit             (GimpPlugIn      *plug_in);
static void gimp_plug_in_handle_tile_req         (GimpPlugIn      *plug_in,
                                                  GPTileReq       *tile_req);
static void gimp_plug_in_handle_proc_run         (GimpPlugIn      *plug_in,
                                                  GPProcRun       *proc_run);
static void gimp_plug_in_handle_proc_return      (GimpPlugIn      *plug_in,
                                                  GPProcReturn    *proc_return);
static void gimp_plug_in_handle_temp_proc_return (GimpPlugIn      *plug_in,
                                                  GPProcReturn    *proc_return);
static void gimp_plug_in_handle_proc_install     (GimpPlugIn      *plug_in,
                                                  GPProcInstall   *proc_install);
static void gimp_plug_in_handle_proc_uninstall   (GimpPlugIn      *plug_in,
                                                  GPProcUninstall *proc_uninstall);
static void gimp_plug_in_handle_extension_ack    (GimpPlugIn      *plug_in);
static void gimp_plug_in_handle_has_init         (GimpPlugIn      *plug_in);


/*  public functions  */

void
gimp_plug_in_handle_message (GimpPlugIn      *plug_in,
                             GimpWireMessage *msg)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (plug_in->open == TRUE);
  g_return_if_fail (msg != NULL);

  switch (msg->type)
    {
    case GP_QUIT:
      gimp_plug_in_handle_quit (plug_in);
      break;

    case GP_CONFIG:
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "sent a CONFIG message.  This should not happen.",
                    gimp_object_get_name (GIMP_OBJECT (plug_in)),
                    gimp_filename_to_utf8 (plug_in->prog));
      gimp_plug_in_close (plug_in, TRUE);
      break;

    case GP_TILE_REQ:
      gimp_plug_in_handle_tile_req (plug_in, msg->data);
      break;

    case GP_TILE_ACK:
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "sent a TILE_ACK message.  This should not happen.",
                    gimp_object_get_name (GIMP_OBJECT (plug_in)),
                    gimp_filename_to_utf8 (plug_in->prog));
      gimp_plug_in_close (plug_in, TRUE);
      break;

    case GP_TILE_DATA:
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "sent a TILE_DATA message.  This should not happen.",
                    gimp_object_get_name (GIMP_OBJECT (plug_in)),
                    gimp_filename_to_utf8 (plug_in->prog));
      gimp_plug_in_close (plug_in, TRUE);
      break;

    case GP_PROC_RUN:
      gimp_plug_in_handle_proc_run (plug_in, msg->data);
      break;

    case GP_PROC_RETURN:
      gimp_plug_in_handle_proc_return (plug_in, msg->data);
      break;

    case GP_TEMP_PROC_RUN:
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "sent a TEMP_PROC_RUN message.  This should not happen.",
                    gimp_object_get_name (GIMP_OBJECT (plug_in)),
                    gimp_filename_to_utf8 (plug_in->prog));
      gimp_plug_in_close (plug_in, TRUE);
      break;

    case GP_TEMP_PROC_RETURN:
      gimp_plug_in_handle_temp_proc_return (plug_in, msg->data);
      break;

    case GP_PROC_INSTALL:
      gimp_plug_in_handle_proc_install (plug_in, msg->data);
      break;

    case GP_PROC_UNINSTALL:
      gimp_plug_in_handle_proc_uninstall (plug_in, msg->data);
      break;

    case GP_EXTENSION_ACK:
      gimp_plug_in_handle_extension_ack (plug_in);
      break;

    case GP_HAS_INIT:
      gimp_plug_in_handle_has_init (plug_in);
      break;
    }
}


/*  private functions  */

static void
gimp_plug_in_handle_quit (GimpPlugIn *plug_in)
{
  gimp_plug_in_close (plug_in, FALSE);
}

static void
gimp_plug_in_handle_tile_req (GimpPlugIn *plug_in,
                              GPTileReq  *tile_req)
{
  GPTileData       tile_data;
  GPTileData      *tile_info;
  GimpWireMessage  msg;
  GimpDrawable    *drawable;
  TileManager     *tm;
  Tile            *tile;

  if (tile_req->drawable_ID == -1)
    {
      /*  this branch communicates with libgimp/gimptile.c:gimp_tile_put()  */

      tile_data.drawable_ID = -1;
      tile_data.tile_num    = 0;
      tile_data.shadow      = 0;
      tile_data.bpp         = 0;
      tile_data.width       = 0;
      tile_data.height      = 0;
      tile_data.use_shm     = (plug_in->manager->shm != NULL);
      tile_data.data        = NULL;

      if (! gp_tile_data_write (plug_in->my_write, &tile_data, plug_in))
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "plug_in_handle_tile_req: ERROR");
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }

      if (! gimp_wire_read_msg (plug_in->my_read, &msg, plug_in))
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "plug_in_handle_tile_req: ERROR");
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }

      if (msg.type != GP_TILE_DATA)
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "expected tile data and received: %d", msg.type);
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }

      tile_info = msg.data;

      drawable = (GimpDrawable *) gimp_item_get_by_ID (plug_in->manager->gimp,
                                                       tile_info->drawable_ID);

      if (! GIMP_IS_DRAWABLE (drawable))
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "Plug-In \"%s\"\n(%s)\n\n"
                        "requested invalid drawable (killing)",
                        gimp_object_get_name (GIMP_OBJECT (plug_in)),
                        gimp_filename_to_utf8 (plug_in->prog));
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }

      if (tile_info->shadow)
        tm = gimp_drawable_get_shadow_tiles (drawable);
      else
        tm = gimp_drawable_get_tiles (drawable);

      tile = tile_manager_get (tm, tile_info->tile_num, TRUE, TRUE);

      if (! tile)
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "Plug-In \"%s\"\n(%s)\n\n"
                        "requested invalid tile (killing)",
                        gimp_object_get_name (GIMP_OBJECT (plug_in)),
                        gimp_filename_to_utf8 (plug_in->prog));
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }

      if (tile_data.use_shm)
        memcpy (tile_data_pointer (tile, 0, 0),
                gimp_plug_in_shm_get_addr (plug_in->manager->shm),
                tile_size (tile));
      else
        memcpy (tile_data_pointer (tile, 0, 0),
                tile_info->data,
                tile_size (tile));

      tile_release (tile, TRUE);
      gimp_wire_destroy (&msg);

      if (! gp_tile_ack_write (plug_in->my_write, plug_in))
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "plug_in_handle_tile_req: ERROR");
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }
    }
  else
    {
      /*  this branch communicates with libgimp/gimptile.c:gimp_tile_get()  */

      drawable = (GimpDrawable *) gimp_item_get_by_ID (plug_in->manager->gimp,
                                                       tile_req->drawable_ID);

      if (! GIMP_IS_DRAWABLE (drawable))
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "Plug-In \"%s\"\n(%s)\n\n"
                        "requested invalid drawable (killing)",
                        gimp_object_get_name (GIMP_OBJECT (plug_in)),
                        gimp_filename_to_utf8 (plug_in->prog));
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }

      if (tile_req->shadow)
        tm = gimp_drawable_get_shadow_tiles (drawable);
      else
        tm = gimp_drawable_get_tiles (drawable);

      tile = tile_manager_get (tm, tile_req->tile_num, TRUE, FALSE);

      if (! tile)
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "Plug-In \"%s\"\n(%s)\n\n"
                        "requested invalid tile (killing)",
                        gimp_object_get_name (GIMP_OBJECT (plug_in)),
                        gimp_filename_to_utf8 (plug_in->prog));
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }

      tile_data.drawable_ID = tile_req->drawable_ID;
      tile_data.tile_num    = tile_req->tile_num;
      tile_data.shadow      = tile_req->shadow;
      tile_data.bpp         = tile_bpp (tile);
      tile_data.width       = tile_ewidth (tile);
      tile_data.height      = tile_eheight (tile);
      tile_data.use_shm     = (plug_in->manager->shm != NULL);

      if (tile_data.use_shm)
        memcpy (gimp_plug_in_shm_get_addr (plug_in->manager->shm),
                tile_data_pointer (tile, 0, 0),
                tile_size (tile));
      else
        tile_data.data = tile_data_pointer (tile, 0, 0);

      if (! gp_tile_data_write (plug_in->my_write, &tile_data, plug_in))
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "plug_in_handle_tile_req: ERROR");
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }

      tile_release (tile, FALSE);

      if (! gimp_wire_read_msg (plug_in->my_read, &msg, plug_in))
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "plug_in_handle_tile_req: ERROR");
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }

      if (msg.type != GP_TILE_ACK)
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "expected tile ack and received: %d", msg.type);
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }

      gimp_wire_destroy (&msg);
    }
}

static void
gimp_plug_in_handle_proc_run (GimpPlugIn *plug_in,
                              GPProcRun  *proc_run)
{
  GimpPlugInProcFrame *proc_frame;
  gchar               *canonical;
  const gchar         *proc_name     = NULL;
  GimpProcedure       *procedure;
  GValueArray         *args          = NULL;
  GValueArray         *return_vals   = NULL;

  canonical = gimp_canonicalize_identifier (proc_run->name);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  procedure = gimp_pdb_lookup_procedure (plug_in->manager->gimp->pdb,
                                         canonical);

  if (! procedure)
    {
      proc_name = gimp_pdb_lookup_compat_proc_name (plug_in->manager->gimp->pdb,
                                                    canonical);

      if (proc_name)
        {
          procedure = gimp_pdb_lookup_procedure (plug_in->manager->gimp->pdb,
                                                 proc_name);

          if (plug_in->manager->gimp->pdb_compat_mode == GIMP_PDB_COMPAT_WARN)
            {
              gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_WARNING,
                            "Plug-In \"%s\"\n(%s)\n"
                            "called deprecated procedure '%s'.\n"
                            "It should call '%s' instead!",
                            gimp_object_get_name (GIMP_OBJECT (plug_in)),
                            gimp_filename_to_utf8 (plug_in->prog),
                            canonical, proc_name);
            }
        }
    }
  else if (procedure->deprecated)
    {
      if (plug_in->manager->gimp->pdb_compat_mode == GIMP_PDB_COMPAT_WARN)
        {
          if (! strcmp (procedure->deprecated, "NONE"))
            {
              gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_WARNING,
                            "Plug-In \"%s\"\n(%s)\n"
                            "called deprecated procedure '%s'.",
                            gimp_object_get_name (GIMP_OBJECT (plug_in)),
                            gimp_filename_to_utf8 (plug_in->prog),
                            canonical);
            }
          else
            {
              gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_WARNING,
                            "WARNING: Plug-In \"%s\"\n(%s)\n"
                            "called deprecated procedure '%s'.\n"
                            "It should call '%s' instead!",
                            gimp_object_get_name (GIMP_OBJECT (plug_in)),
                            gimp_filename_to_utf8 (plug_in->prog),
                            canonical, procedure->deprecated);
            }
        }
    }

  if (! proc_name)
    proc_name = canonical;

  args = plug_in_params_to_args (procedure ? procedure->args     : NULL,
                                 procedure ? procedure->num_args : 0,
                                 proc_run->params, proc_run->nparams,
                                 FALSE, FALSE);

  /*  Execute the procedure even if gimp_pdb_lookup_procedure()
   *  returned NULL, gimp_pdb_execute_procedure_by_name_args() will
   *  return appropriate error return_vals.
   */
  gimp_plug_in_manager_plug_in_push (plug_in->manager, plug_in);
  return_vals = gimp_pdb_execute_procedure_by_name_args (plug_in->manager->gimp->pdb,
                                                         proc_frame->context_stack ?
                                                         proc_frame->context_stack->data :
                                                         proc_frame->main_context,
                                                         proc_frame->progress,
                                                         proc_name,
                                                         args);
  gimp_plug_in_manager_plug_in_pop (plug_in->manager);

  g_value_array_free (args);
  g_free (canonical);

  /*  Don't bother to send the return value if executing the procedure
   *  closed the plug-in (e.g. if the procedure is gimp-quit)
   */
  if (plug_in->open)
    {
      GPProcReturn proc_return;

      /*  Return the name we got called with, *not* proc_name or canonical,
       *  since proc_name may have been remapped by gimp->procedural_compat_ht
       *  and canonical may be different too.
       */
      proc_return.name    = proc_run->name;
      proc_return.nparams = return_vals->n_values;
      proc_return.params  = plug_in_args_to_params (return_vals, FALSE);

      if (! gp_proc_return_write (plug_in->my_write, &proc_return, plug_in))
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "plug_in_handle_proc_run: ERROR");
          gimp_plug_in_close (plug_in, TRUE);
        }

      g_free (proc_return.params);
    }

  g_value_array_free (return_vals);
}

static void
gimp_plug_in_handle_proc_return (GimpPlugIn   *plug_in,
                                 GPProcReturn *proc_return)
{
  GimpPlugInProcFrame *proc_frame = &plug_in->main_proc_frame;

  if (proc_frame->main_loop)
    {
      proc_frame->return_vals =
        plug_in_params_to_args (proc_frame->procedure->values,
                                proc_frame->procedure->num_values,
                                proc_return->params,
                                proc_return->nparams,
                                TRUE, TRUE);

      g_main_loop_quit (proc_frame->main_loop);
    }

  gimp_plug_in_close (plug_in, FALSE);
}

static void
gimp_plug_in_handle_temp_proc_return (GimpPlugIn   *plug_in,
                                      GPProcReturn *proc_return)
{
  if (plug_in->temp_proc_frames)
    {
      GimpPlugInProcFrame *proc_frame = plug_in->temp_proc_frames->data;

      proc_frame->return_vals =
        plug_in_params_to_args (proc_frame->procedure->values,
                                proc_frame->procedure->num_values,
                                proc_return->params,
                                proc_return->nparams,
                                TRUE, TRUE);

      gimp_plug_in_main_loop_quit (plug_in);
      gimp_plug_in_proc_frame_pop (plug_in);
    }
  else
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "sent a TEMP_PROC_RETURN message while not running "
                    "a temporary procedure.  This should not happen.",
                    gimp_object_get_name (GIMP_OBJECT (plug_in)),
                    gimp_filename_to_utf8 (plug_in->prog));
      gimp_plug_in_close (plug_in, TRUE);
    }
}

static void
gimp_plug_in_handle_proc_install (GimpPlugIn    *plug_in,
                                  GPProcInstall *proc_install)
{
  GimpPlugInProcedure *proc        = NULL;
  GimpProcedure       *procedure   = NULL;
  gchar               *canonical;
  gboolean             valid_utf8  = FALSE;
  gint                 i;

  canonical = gimp_canonicalize_identifier (proc_install->name);

  /*  Sanity check for array arguments  */

  for (i = 1; i < proc_install->nparams; i++)
    {
      if ((proc_install->params[i].type == GIMP_PDB_INT32ARRAY ||
           proc_install->params[i].type == GIMP_PDB_INT8ARRAY  ||
           proc_install->params[i].type == GIMP_PDB_FLOATARRAY ||
           proc_install->params[i].type == GIMP_PDB_STRINGARRAY)
          &&
          proc_install->params[i - 1].type != GIMP_PDB_INT32)
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "Plug-In \"%s\"\n(%s)\n\n"
                        "attempted to install procedure \"%s\" "
                        "which fails to comply with the array parameter "
                        "passing standard.  Argument %d is noncompliant.",
                        gimp_object_get_name (GIMP_OBJECT (plug_in)),
                        gimp_filename_to_utf8 (plug_in->prog),
                        canonical, i);
          g_free (canonical);
          return;
        }
    }

  /*  Sanity check strings for UTF-8 validity  */

  if ((proc_install->menu_path == NULL ||
       g_utf8_validate (proc_install->menu_path, -1, NULL)) &&
      (g_utf8_validate (canonical, -1, NULL))               &&
      (proc_install->blurb == NULL ||
       g_utf8_validate (proc_install->blurb, -1, NULL))     &&
      (proc_install->help == NULL ||
       g_utf8_validate (proc_install->help, -1, NULL))      &&
      (proc_install->author == NULL ||
       g_utf8_validate (proc_install->author, -1, NULL))    &&
      (proc_install->copyright == NULL ||
       g_utf8_validate (proc_install->copyright, -1, NULL)) &&
      (proc_install->date == NULL ||
       g_utf8_validate (proc_install->date, -1, NULL)))
    {
      valid_utf8 = TRUE;

      for (i = 0; i < proc_install->nparams && valid_utf8; i++)
        {
          if (! (g_utf8_validate (proc_install->params[i].name, -1, NULL) &&
                 (proc_install->params[i].description == NULL ||
                  g_utf8_validate (proc_install->params[i].description, -1, NULL))))
            valid_utf8 = FALSE;
        }

      for (i = 0; i < proc_install->nreturn_vals && valid_utf8; i++)
        {
          if (! (g_utf8_validate (proc_install->return_vals[i].name, -1, NULL) &&
                 (proc_install->return_vals[i].description == NULL ||
                  g_utf8_validate (proc_install->return_vals[i].description, -1, NULL))))
            valid_utf8 = FALSE;
        }
    }

  if (! valid_utf8)
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "attempted to install a procedure with invalid UTF-8 strings.",
                    gimp_object_get_name (GIMP_OBJECT (plug_in)),
                    gimp_filename_to_utf8 (plug_in->prog));
      g_free (canonical);
      return;
    }

  /*  Create the procedure object  */

  switch (proc_install->type)
    {
    case GIMP_PLUGIN:
    case GIMP_EXTENSION:
      procedure = gimp_plug_in_procedure_new (proc_install->type,
                                              plug_in->prog);
      break;

    case GIMP_TEMPORARY:
      procedure = gimp_temporary_procedure_new (plug_in);
      break;
    }

  proc = GIMP_PLUG_IN_PROCEDURE (procedure);

  proc->mtime                 = time (NULL);
  proc->installed_during_init = (plug_in->call_mode == GIMP_PLUG_IN_CALL_INIT);

  gimp_object_take_name (GIMP_OBJECT (procedure), canonical);
  gimp_procedure_set_strings (procedure,
                              proc_install->name,
                              proc_install->blurb,
                              proc_install->help,
                              proc_install->author,
                              proc_install->copyright,
                              proc_install->date,
                              NULL);

  gimp_plug_in_procedure_set_image_types (proc, proc_install->image_types);

  for (i = 0; i < proc_install->nparams; i++)
    {
      GParamSpec *pspec =
        gimp_pdb_compat_param_spec (plug_in->manager->gimp,
                                    proc_install->params[i].type,
                                    proc_install->params[i].name,
                                    proc_install->params[i].description);

      gimp_procedure_add_argument (procedure, pspec);
    }

  for (i = 0; i < proc_install->nreturn_vals; i++)
    {
      GParamSpec *pspec =
        gimp_pdb_compat_param_spec (plug_in->manager->gimp,
                                    proc_install->return_vals[i].type,
                                    proc_install->return_vals[i].name,
                                    proc_install->return_vals[i].description);

      gimp_procedure_add_return_value (procedure, pspec);
    }

  /*  Sanity check menu path  */

  if (proc_install->menu_path)
    {
      if (proc_install->menu_path[0] == '<')
        {
          GError *error = NULL;

          if (! gimp_plug_in_procedure_add_menu_path (proc,
                                                      proc_install->menu_path,
                                                      &error))
            {
              gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_WARNING,
                            "%s", error->message);
              g_clear_error (&error);
            }
        }
      else
        {
          proc->menu_label = g_strdup (proc_install->menu_path);
        }
    }

  /*  Install the procedure  */

  switch (proc_install->type)
    {
    case GIMP_PLUGIN:
    case GIMP_EXTENSION:
      gimp_plug_in_def_add_procedure (plug_in->plug_in_def, proc);
      break;

    case GIMP_TEMPORARY:
      gimp_plug_in_add_temp_proc (plug_in, GIMP_TEMPORARY_PROCEDURE (proc));
      break;
    }

  g_object_unref (proc);
}

static void
gimp_plug_in_handle_proc_uninstall (GimpPlugIn      *plug_in,
                                    GPProcUninstall *proc_uninstall)
{
  GimpPlugInProcedure *proc;
  gchar               *canonical;

  canonical = gimp_canonicalize_identifier (proc_uninstall->name);

  proc = gimp_plug_in_procedure_find (plug_in->temp_procedures, canonical);

  if (proc)
    gimp_plug_in_remove_temp_proc (plug_in, GIMP_TEMPORARY_PROCEDURE (proc));

  g_free (canonical);
}

static void
gimp_plug_in_handle_extension_ack (GimpPlugIn *plug_in)
{
  if (plug_in->ext_main_loop)
    {
      g_main_loop_quit (plug_in->ext_main_loop);
    }
  else
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "sent an EXTENSION_ACK message while not being started "
                    "as an extension.  This should not happen.",
                    gimp_object_get_name (GIMP_OBJECT (plug_in)),
                    gimp_filename_to_utf8 (plug_in->prog));
      gimp_plug_in_close (plug_in, TRUE);
    }
}

static void
gimp_plug_in_handle_has_init (GimpPlugIn *plug_in)
{
  if (plug_in->call_mode == GIMP_PLUG_IN_CALL_QUERY)
    {
      gimp_plug_in_def_set_has_init (plug_in->plug_in_def, TRUE);
    }
  else
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "sent an HAS_INIT message while not in query().  "
                    "This should not happen.",
                    gimp_object_get_name (GIMP_OBJECT (plug_in)),
                    gimp_filename_to_utf8 (plug_in->prog));
      gimp_plug_in_close (plug_in, TRUE);
    }
}
