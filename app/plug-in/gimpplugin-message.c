/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugin-message.c
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmabase/ligmaprotocol.h"
#include "libligmabase/ligmawire.h"

#include "libligma/ligmagpparams.h"

#include "plug-in-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-tile-compat.h"

#include "core/ligma.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawable-shadow.h"

#include "pdb/ligma-pdb-compat.h"
#include "pdb/ligmapdb.h"
#include "pdb/ligmapdberror.h"

#include "ligmaplugin.h"
#include "ligmaplugin-cleanup.h"
#include "ligmaplugin-message.h"
#include "ligmapluginmanager.h"
#include "ligmaplugindef.h"
#include "ligmapluginshm.h"
#include "ligmatemporaryprocedure.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void ligma_plug_in_handle_quit             (LigmaPlugIn      *plug_in);
static void ligma_plug_in_handle_tile_request     (LigmaPlugIn      *plug_in,
                                                  GPTileReq       *request);
static void ligma_plug_in_handle_tile_put         (LigmaPlugIn      *plug_in,
                                                  GPTileReq       *request);
static void ligma_plug_in_handle_tile_get         (LigmaPlugIn      *plug_in,
                                                  GPTileReq       *request);
static void ligma_plug_in_handle_proc_run         (LigmaPlugIn      *plug_in,
                                                  GPProcRun       *proc_run);
static void ligma_plug_in_handle_proc_return      (LigmaPlugIn      *plug_in,
                                                  GPProcReturn    *proc_return);
static void ligma_plug_in_handle_temp_proc_return (LigmaPlugIn      *plug_in,
                                                  GPProcReturn    *proc_return);
static void ligma_plug_in_handle_proc_install     (LigmaPlugIn      *plug_in,
                                                  GPProcInstall   *proc_install);
static void ligma_plug_in_handle_proc_uninstall   (LigmaPlugIn      *plug_in,
                                                  GPProcUninstall *proc_uninstall);
static void ligma_plug_in_handle_extension_ack    (LigmaPlugIn      *plug_in);
static void ligma_plug_in_handle_has_init         (LigmaPlugIn      *plug_in);


/*  public functions  */

void
ligma_plug_in_handle_message (LigmaPlugIn      *plug_in,
                             LigmaWireMessage *msg)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));
  g_return_if_fail (plug_in->open == TRUE);
  g_return_if_fail (msg != NULL);

  switch (msg->type)
    {
    case GP_QUIT:
      ligma_plug_in_handle_quit (plug_in);
      break;

    case GP_CONFIG:
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent a CONFIG message.  This should not happen.",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file));
      ligma_plug_in_close (plug_in, TRUE);
      break;

    case GP_TILE_REQ:
      ligma_plug_in_handle_tile_request (plug_in, msg->data);
      break;

    case GP_TILE_ACK:
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent a TILE_ACK message.  This should not happen.",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file));
      ligma_plug_in_close (plug_in, TRUE);
      break;

    case GP_TILE_DATA:
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent a TILE_DATA message.  This should not happen.",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file));
      ligma_plug_in_close (plug_in, TRUE);
      break;

    case GP_PROC_RUN:
      ligma_plug_in_handle_proc_run (plug_in, msg->data);
      break;

    case GP_PROC_RETURN:
      ligma_plug_in_handle_proc_return (plug_in, msg->data);
      break;

    case GP_TEMP_PROC_RUN:
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent a TEMP_PROC_RUN message.  This should not happen.",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file));
      ligma_plug_in_close (plug_in, TRUE);
      break;

    case GP_TEMP_PROC_RETURN:
      ligma_plug_in_handle_temp_proc_return (plug_in, msg->data);
      break;

    case GP_PROC_INSTALL:
      ligma_plug_in_handle_proc_install (plug_in, msg->data);
      break;

    case GP_PROC_UNINSTALL:
      ligma_plug_in_handle_proc_uninstall (plug_in, msg->data);
      break;

    case GP_EXTENSION_ACK:
      ligma_plug_in_handle_extension_ack (plug_in);
      break;

    case GP_HAS_INIT:
      ligma_plug_in_handle_has_init (plug_in);
      break;
    }
}


/*  private functions  */

static void
ligma_plug_in_handle_quit (LigmaPlugIn *plug_in)
{
  ligma_plug_in_close (plug_in, FALSE);
}

static void
ligma_plug_in_handle_tile_request (LigmaPlugIn *plug_in,
                                  GPTileReq  *request)
{
  g_return_if_fail (request != NULL);

  if (request->drawable_id == -1)
    ligma_plug_in_handle_tile_put (plug_in, request);
  else
    ligma_plug_in_handle_tile_get (plug_in, request);
}

static void
ligma_plug_in_handle_tile_put (LigmaPlugIn *plug_in,
                              GPTileReq  *request)
{
  GPTileData       tile_data;
  GPTileData      *tile_info;
  LigmaWireMessage  msg;
  LigmaDrawable    *drawable;
  GeglBuffer      *buffer;
  const Babl      *format;
  GeglRectangle    tile_rect;

  tile_data.drawable_id = -1;
  tile_data.tile_num    = 0;
  tile_data.shadow      = 0;
  tile_data.bpp         = 0;
  tile_data.width       = 0;
  tile_data.height      = 0;
  tile_data.use_shm     = (plug_in->manager->shm != NULL);
  tile_data.data        = NULL;

  if (! gp_tile_data_write (plug_in->my_write, &tile_data, plug_in))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }

  if (! ligma_wire_read_msg (plug_in->my_read, &msg, plug_in))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }

  if (msg.type != GP_TILE_DATA)
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "expected tile data and received: %d", msg.type);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }

  tile_info = msg.data;

  drawable = (LigmaDrawable *) ligma_item_get_by_id (plug_in->manager->ligma,
                                                   tile_info->drawable_id);

  if (! LIGMA_IS_DRAWABLE (drawable))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "tried writing to invalid drawable %d (killing)",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file),
                    tile_info->drawable_id);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }
  else if (ligma_item_is_removed (LIGMA_ITEM (drawable)))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "tried writing to drawable %d which was removed "
                    "from the image (killing)",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file),
                    tile_info->drawable_id);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }

  if (tile_info->shadow)
    {

      /*  don't check whether the drawable is a group or locked here,
       *  the plugin will get a proper error message when it tries to
       *  merge the shadow tiles, which is much better than just
       *  killing it.
       */
      buffer = ligma_drawable_get_shadow_buffer (drawable);

      ligma_plug_in_cleanup_add_shadow (plug_in, drawable);
    }
  else
    {
      if (ligma_item_is_content_locked (LIGMA_ITEM (drawable), NULL))
        {
          ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                        "Plug-in \"%s\"\n(%s)\n\n"
                        "tried writing to a locked drawable %d (killing)",
                        ligma_object_get_name (plug_in),
                        ligma_file_get_utf8_name (plug_in->file),
                        tile_info->drawable_id);
          ligma_plug_in_close (plug_in, TRUE);
          return;
        }
      else if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawable)))
        {
          ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                        "Plug-in \"%s\"\n(%s)\n\n"
                        "tried writing to a group layer %d (killing)",
                        ligma_object_get_name (plug_in),
                        ligma_file_get_utf8_name (plug_in->file),
                        tile_info->drawable_id);
          ligma_plug_in_close (plug_in, TRUE);
          return;
        }

      buffer = ligma_drawable_get_buffer (drawable);
    }

  if (! ligma_gegl_buffer_get_tile_rect (buffer,
                                        LIGMA_PLUG_IN_TILE_WIDTH,
                                        LIGMA_PLUG_IN_TILE_HEIGHT,
                                        tile_info->tile_num,
                                        &tile_rect))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "requested invalid tile #%d for writing (killing)",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file),
                    tile_info->tile_num);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }

  format = gegl_buffer_get_format (buffer);

  if (tile_data.use_shm)
    {
      gegl_buffer_set (buffer, &tile_rect, 0, format,
                       ligma_plug_in_shm_get_addr (plug_in->manager->shm),
                       GEGL_AUTO_ROWSTRIDE);
    }
  else
    {
      gegl_buffer_set (buffer, &tile_rect, 0, format,
                       tile_info->data,
                       GEGL_AUTO_ROWSTRIDE);
    }

  ligma_wire_destroy (&msg);

  if (! gp_tile_ack_write (plug_in->my_write, plug_in))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }
}

static void
ligma_plug_in_handle_tile_get (LigmaPlugIn *plug_in,
                              GPTileReq  *request)
{
  GPTileData       tile_data;
  LigmaWireMessage  msg;
  LigmaDrawable    *drawable;
  GeglBuffer      *buffer;
  const Babl      *format;
  GeglRectangle    tile_rect;
  gint             tile_size;

  drawable = (LigmaDrawable *) ligma_item_get_by_id (plug_in->manager->ligma,
                                                   request->drawable_id);

  if (! LIGMA_IS_DRAWABLE (drawable))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "tried reading from invalid drawable %d (killing)",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file),
                    request->drawable_id);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }
  else if (ligma_item_is_removed (LIGMA_ITEM (drawable)))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "tried reading from drawable %d which was removed "
                    "from the image (killing)",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file),
                    request->drawable_id);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }

  if (request->shadow)
    {
      buffer = ligma_drawable_get_shadow_buffer (drawable);

      ligma_plug_in_cleanup_add_shadow (plug_in, drawable);
    }
  else
    {
      buffer = ligma_drawable_get_buffer (drawable);
    }

  if (! ligma_gegl_buffer_get_tile_rect (buffer,
                                        LIGMA_PLUG_IN_TILE_WIDTH,
                                        LIGMA_PLUG_IN_TILE_HEIGHT,
                                        request->tile_num,
                                        &tile_rect))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "requested invalid tile #%d for reading (killing)",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file),
                    request->tile_num);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }

  format = gegl_buffer_get_format (buffer);

  tile_size = (babl_format_get_bytes_per_pixel (format) *
               tile_rect.width * tile_rect.height);

  tile_data.drawable_id = request->drawable_id;
  tile_data.tile_num    = request->tile_num;
  tile_data.shadow      = request->shadow;
  tile_data.bpp         = babl_format_get_bytes_per_pixel (format);
  tile_data.width       = tile_rect.width;
  tile_data.height      = tile_rect.height;
  tile_data.use_shm     = (plug_in->manager->shm != NULL);

  if (tile_data.use_shm)
    {
      gegl_buffer_get (buffer, &tile_rect, 1.0, format,
                       ligma_plug_in_shm_get_addr (plug_in->manager->shm),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }
  else
    {
      tile_data.data = g_malloc (tile_size);

      gegl_buffer_get (buffer, &tile_rect, 1.0, format,
                       tile_data.data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }

  if (! gp_tile_data_write (plug_in->my_write, &tile_data, plug_in))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }

  if (! ligma_wire_read_msg (plug_in->my_read, &msg, plug_in))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }

  if (msg.type != GP_TILE_ACK)
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "expected tile ack and received: %d", msg.type);
      ligma_plug_in_close (plug_in, TRUE);
      return;
    }

  ligma_wire_destroy (&msg);
}

static void
ligma_plug_in_handle_proc_error (LigmaPlugIn          *plug_in,
                                LigmaPlugInProcFrame *proc_frame,
                                const gchar         *name,
                                const GError        *error)
{
  switch (proc_frame->error_handler)
    {
    case LIGMA_PDB_ERROR_HANDLER_INTERNAL:
      if (error->domain == LIGMA_PDB_ERROR)
        {
          ligma_message (plug_in->manager->ligma,
                        G_OBJECT (proc_frame->progress),
                        LIGMA_MESSAGE_ERROR,
                        _("Calling error for procedure '%s':\n"
                          "%s"),
                        name, error->message);
        }
      else
        {
          ligma_message (plug_in->manager->ligma,
                        G_OBJECT (proc_frame->progress),
                        LIGMA_MESSAGE_ERROR,
                        _("Execution error for procedure '%s':\n"
                          "%s"),
                        name, error->message);
        }
      break;

    case LIGMA_PDB_ERROR_HANDLER_PLUGIN:
      /*  the plug-in is responsible for handling this error  */
      break;
    }
}

static void
ligma_plug_in_handle_proc_run (LigmaPlugIn *plug_in,
                              GPProcRun  *proc_run)
{
  LigmaPlugInProcFrame *proc_frame;
  gchar               *canonical;
  const gchar         *proc_name   = NULL;
  LigmaProcedure       *procedure;
  LigmaValueArray      *args        = NULL;
  LigmaValueArray      *return_vals = NULL;
  GError              *error       = NULL;

  g_return_if_fail (proc_run != NULL);
  g_return_if_fail (proc_run->name != NULL);

  canonical = ligma_canonicalize_identifier (proc_run->name);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);

  procedure = ligma_pdb_lookup_procedure (plug_in->manager->ligma->pdb,
                                         canonical);

  if (! procedure)
    {
      proc_name = ligma_pdb_lookup_compat_proc_name (plug_in->manager->ligma->pdb,
                                                    canonical);

      if (proc_name)
        {
          procedure = ligma_pdb_lookup_procedure (plug_in->manager->ligma->pdb,
                                                 proc_name);

          if (plug_in->manager->ligma->pdb_compat_mode == LIGMA_PDB_COMPAT_WARN)
            {
              ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_WARNING,
                            "Plug-in \"%s\"\n(%s)\n"
                            "called deprecated procedure '%s'.\n"
                            "It should call '%s' instead!",
                            ligma_object_get_name (plug_in),
                            ligma_file_get_utf8_name (plug_in->file),
                            canonical, proc_name);
            }
        }
    }
  else if (procedure->deprecated)
    {
      if (plug_in->manager->ligma->pdb_compat_mode == LIGMA_PDB_COMPAT_WARN)
        {
          if (! strcmp (procedure->deprecated, "NONE"))
            {
              ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_WARNING,
                            "Plug-in \"%s\"\n(%s)\n"
                            "called deprecated procedure '%s'.",
                            ligma_object_get_name (plug_in),
                            ligma_file_get_utf8_name (plug_in->file),
                            canonical);
            }
          else
            {
              ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_WARNING,
                            "WARNING: Plug-in \"%s\"\n(%s)\n"
                            "called deprecated procedure '%s'.\n"
                            "It should call '%s' instead!",
                            ligma_object_get_name (plug_in),
                            ligma_file_get_utf8_name (plug_in->file),
                            canonical, procedure->deprecated);
            }
        }
    }

  if (! proc_name)
    proc_name = canonical;

  args = _ligma_gp_params_to_value_array (plug_in->manager->ligma,
                                         procedure ? procedure->args     : NULL,
                                         procedure ? procedure->num_args : 0,
                                         proc_run->params,
                                         proc_run->n_params,
                                         FALSE);

  /*  Execute the procedure even if ligma_pdb_lookup_procedure()
   *  returned NULL, ligma_pdb_execute_procedure_by_name_args() will
   *  return appropriate error return_vals.
   */
  ligma_plug_in_manager_plug_in_push (plug_in->manager, plug_in);
  return_vals = ligma_pdb_execute_procedure_by_name_args (plug_in->manager->ligma->pdb,
                                                         proc_frame->context_stack ?
                                                         proc_frame->context_stack->data :
                                                         proc_frame->main_context,
                                                         proc_frame->progress,
                                                         &error,
                                                         proc_name,
                                                         args);
  ligma_plug_in_manager_plug_in_pop (plug_in->manager);

  ligma_value_array_unref (args);

  if (error)
    {
      ligma_plug_in_handle_proc_error (plug_in, proc_frame,
                                      canonical, error);
      g_error_free (error);
    }

  g_free (canonical);

  /*  Don't bother to send the return value if executing the procedure
   *  closed the plug-in (e.g. if the procedure is ligma-quit)
   */
  if (plug_in->open)
    {
      GPProcReturn proc_return;

      /*  Return the name we got called with, *not* proc_name or canonical,
       *  since proc_name may have been remapped by ligma->procedural_compat_ht
       *  and canonical may be different too.
       */
      proc_return.name     = proc_run->name;
      proc_return.n_params = ligma_value_array_length (return_vals);
      proc_return.params   = _ligma_value_array_to_gp_params (return_vals, FALSE);

      if (! gp_proc_return_write (plug_in->my_write, &proc_return, plug_in))
        {
          ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                        "%s: ERROR", G_STRFUNC);
          ligma_plug_in_close (plug_in, TRUE);
        }

      _ligma_gp_params_free (proc_return.params, proc_return.n_params, FALSE);
    }

  ligma_value_array_unref (return_vals);
}

static void
ligma_plug_in_handle_proc_return (LigmaPlugIn   *plug_in,
                                 GPProcReturn *proc_return)
{
  LigmaPlugInProcFrame *proc_frame = &plug_in->main_proc_frame;

  g_return_if_fail (proc_return != NULL);

  proc_frame->return_vals =
    _ligma_gp_params_to_value_array (plug_in->manager->ligma,
                                    proc_frame->procedure->values,
                                    proc_frame->procedure->num_values,
                                    proc_return->params,
                                    proc_return->n_params,
                                    TRUE);

  if (proc_frame->main_loop)
    {
      g_main_loop_quit (proc_frame->main_loop);
    }
  else
    {
      /*  the plug-in is run asynchronously, so display its error
       *  messages here because nobody else will do it
       */
      ligma_plug_in_procedure_handle_return_values (LIGMA_PLUG_IN_PROCEDURE (proc_frame->procedure),
                                                   plug_in->manager->ligma,
                                                   proc_frame->progress,
                                                   proc_frame->return_vals);
    }

  ligma_plug_in_close (plug_in, FALSE);
}

static void
ligma_plug_in_handle_temp_proc_return (LigmaPlugIn   *plug_in,
                                      GPProcReturn *proc_return)
{
  g_return_if_fail (proc_return != NULL);

  if (plug_in->temp_proc_frames)
    {
      LigmaPlugInProcFrame *proc_frame = plug_in->temp_proc_frames->data;

      proc_frame->return_vals =
        _ligma_gp_params_to_value_array (plug_in->manager->ligma,
                                        proc_frame->procedure->values,
                                        proc_frame->procedure->num_values,
                                        proc_return->params,
                                        proc_return->n_params,
                                        TRUE);

      ligma_plug_in_main_loop_quit (plug_in);
      ligma_plug_in_proc_frame_pop (plug_in);
    }
  else
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent a TEMP_PROC_RETURN message while not running "
                    "a temporary procedure.  This should not happen.",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file));
      ligma_plug_in_close (plug_in, TRUE);
    }
}

static void
ligma_plug_in_handle_proc_install (LigmaPlugIn    *plug_in,
                                  GPProcInstall *proc_install)
{
  LigmaPlugInProcedure *proc       = NULL;
  LigmaProcedure       *procedure  = NULL;
  gboolean             null_name  = FALSE;
  gboolean             valid_utf8 = TRUE;
  gint                 i;

  g_return_if_fail (proc_install != NULL);
  g_return_if_fail (proc_install->name != NULL);

  if (! ligma_is_canonical_identifier (proc_install->name))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "attempted to install procedure \"%s\" with a "
                    "non-canonical name.",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file),
                    proc_install->name);
      return;
    }

  /*  Sanity check for array arguments  */

  for (i = 1; i < proc_install->n_params; i++)
    {
      GPParamDef *param_def      = &proc_install->params[i];
      GPParamDef *prev_param_def = &proc_install->params[i - 1];

      if ((! strcmp (param_def->type_name, "LigmaParamInt32Array")     ||
           ! strcmp (param_def->type_name, "LigmaParamUInt8Array")     ||
           ! strcmp (param_def->type_name, "LigmaParamIntFloatArray")  ||
           ! strcmp (param_def->type_name, "LigmaParamIntStringArray") ||
           ! strcmp (param_def->type_name, "LigmaParamIntColorArray"))
          &&
          strcmp (prev_param_def->type_name, "GParamInt"))
        {
          ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                        "Plug-in \"%s\"\n(%s)\n\n"
                        "attempted to install procedure \"%s\" "
                        "which fails to comply with the array parameter "
                        "passing standard.  Argument %d is noncompliant.",
                        ligma_object_get_name (plug_in),
                        ligma_file_get_utf8_name (plug_in->file),
                        proc_install->name, i);
          return;
        }
    }

  /*  Sanity check strings for UTF-8 validity  */

#define VALIDATE(str) ((str) == NULL || g_utf8_validate ((str), -1, NULL))

  for (i = 0; i < proc_install->n_params && valid_utf8 && ! null_name; i++)
    {
      if (! proc_install->params[i].name)
        {
          null_name = TRUE;
        }
      else if (! (VALIDATE (proc_install->params[i].name) &&
                  VALIDATE (proc_install->params[i].nick) &&
                  VALIDATE (proc_install->params[i].blurb)))
        {
          valid_utf8 = FALSE;
        }
    }

  for (i = 0; i < proc_install->n_return_vals && valid_utf8 && !null_name; i++)
    {
      if (! proc_install->return_vals[i].name)
        {
          null_name = TRUE;
        }
      else if (! (VALIDATE (proc_install->return_vals[i].name) &&
                  VALIDATE (proc_install->return_vals[i].nick) &&
                  VALIDATE (proc_install->return_vals[i].blurb)))
        {
          valid_utf8 = FALSE;
        }
    }

#undef VALIDATE

  if (null_name)
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "attempted to install procedure \"%s\" with a "
                    "NULL parameter name.",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file),
                    proc_install->name);
      return;
    }

  if (! valid_utf8)
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "attempted to install procedure \"%s\" with "
                    "invalid UTF-8 strings.",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file),
                    proc_install->name);
      return;
    }

  /*  Create the procedure object  */

  switch (proc_install->type)
    {
    case LIGMA_PDB_PROC_TYPE_PLUGIN:
    case LIGMA_PDB_PROC_TYPE_EXTENSION:
      procedure = ligma_plug_in_procedure_new (proc_install->type,
                                              plug_in->file);
      break;

    case LIGMA_PDB_PROC_TYPE_TEMPORARY:
      procedure = ligma_temporary_procedure_new (plug_in);
      break;
    }

  proc = LIGMA_PLUG_IN_PROCEDURE (procedure);

  proc->mtime                 = time (NULL);
  proc->installed_during_init = (plug_in->call_mode == LIGMA_PLUG_IN_CALL_INIT);

  ligma_object_set_name (LIGMA_OBJECT (procedure), proc_install->name);

  for (i = 0; i < proc_install->n_params; i++)
    {
      GParamSpec *pspec =
        _ligma_gp_param_def_to_param_spec (&proc_install->params[i]);

      if (pspec)
        ligma_procedure_add_argument (procedure, pspec);
    }

  for (i = 0; i < proc_install->n_return_vals; i++)
    {
      GParamSpec *pspec =
        _ligma_gp_param_def_to_param_spec (&proc_install->return_vals[i]);

      if (pspec)
        ligma_procedure_add_return_value (procedure, pspec);
    }

  /*  Install the procedure  */

  switch (proc_install->type)
    {
    case LIGMA_PDB_PROC_TYPE_PLUGIN:
    case LIGMA_PDB_PROC_TYPE_EXTENSION:
      ligma_plug_in_def_add_procedure (plug_in->plug_in_def, proc);
      break;

    case LIGMA_PDB_PROC_TYPE_TEMPORARY:
      ligma_plug_in_add_temp_proc (plug_in, LIGMA_TEMPORARY_PROCEDURE (proc));
      break;
    }

  g_object_unref (proc);
}

static void
ligma_plug_in_handle_proc_uninstall (LigmaPlugIn      *plug_in,
                                    GPProcUninstall *proc_uninstall)
{
  LigmaPlugInProcedure *proc;

  g_return_if_fail (proc_uninstall != NULL);
  g_return_if_fail (proc_uninstall->name != NULL);

  if (! ligma_is_canonical_identifier (proc_uninstall->name))
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "attempted to uninstall procedure \"%s\" with a "
                    "non-canonical name.",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file),
                    proc_uninstall->name);
      return;
    }

  proc = ligma_plug_in_procedure_find (plug_in->temp_procedures,
                                      proc_uninstall->name);

  if (proc)
    ligma_plug_in_remove_temp_proc (plug_in, LIGMA_TEMPORARY_PROCEDURE (proc));
}

static void
ligma_plug_in_handle_extension_ack (LigmaPlugIn *plug_in)
{
  if (plug_in->ext_main_loop)
    {
      g_main_loop_quit (plug_in->ext_main_loop);
    }
  else
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent an EXTENSION_ACK message while not being started "
                    "as an extension.  This should not happen.",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file));
      ligma_plug_in_close (plug_in, TRUE);
    }
}

static void
ligma_plug_in_handle_has_init (LigmaPlugIn *plug_in)
{
  if (plug_in->call_mode == LIGMA_PLUG_IN_CALL_QUERY)
    {
      ligma_plug_in_def_set_has_init (plug_in->plug_in_def, TRUE);
    }
  else
    {
      ligma_message (plug_in->manager->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    "Plug-in \"%s\"\n(%s)\n\n"
                    "sent an HAS_INIT message while not in query().  "
                    "This should not happen.",
                    ligma_object_get_name (plug_in),
                    ligma_file_get_utf8_name (plug_in->file));
      ligma_plug_in_close (plug_in, TRUE);
    }
}
