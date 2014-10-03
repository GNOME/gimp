/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin-message.c
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "plug-in-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-babl-compat.h"
#include "gegl/gimp-gegl-tile-compat.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-shadow.h"

#include "pdb/gimp-pdb-compat.h"
#include "pdb/gimppdb.h"
#include "pdb/gimppdberror.h"

#include "gimpplugin.h"
#include "gimpplugin-cleanup.h"
#include "gimpplugin-message.h"
#include "gimppluginmanager.h"
#include "gimpplugindef.h"
#include "gimppluginshm.h"
#include "gimptemporaryprocedure.h"
#include "plug-in-params.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void gimp_plug_in_handle_quit             (GimpPlugIn      *plug_in);
static void gimp_plug_in_handle_tile_request     (GimpPlugIn      *plug_in,
                                                  GPTileReq       *request);
static void gimp_plug_in_handle_tile_put         (GimpPlugIn      *plug_in,
                                                  GPTileReq       *request);
static void gimp_plug_in_handle_tile_get         (GimpPlugIn      *plug_in,
                                                  GPTileReq       *request);
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
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
      gimp_plug_in_close (plug_in, TRUE);
      break;

    case GP_TILE_REQ:
      gimp_plug_in_handle_tile_request (plug_in, msg->data);
      break;

    case GP_TILE_ACK:
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "sent a TILE_ACK message.  This should not happen.",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
      gimp_plug_in_close (plug_in, TRUE);
      break;

    case GP_TILE_DATA:
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "sent a TILE_DATA message.  This should not happen.",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
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
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
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
gimp_plug_in_handle_tile_request (GimpPlugIn *plug_in,
                                  GPTileReq  *request)
{
  g_return_if_fail (request != NULL);

  if (request->drawable_ID == -1)
    gimp_plug_in_handle_tile_put (plug_in, request);
  else
    gimp_plug_in_handle_tile_get (plug_in, request);
}

static void
gimp_plug_in_handle_tile_put (GimpPlugIn *plug_in,
                              GPTileReq  *request)
{
  GPTileData       tile_data;
  GPTileData      *tile_info;
  GimpWireMessage  msg;
  GimpDrawable    *drawable;
  GeglBuffer      *buffer;
  const Babl      *format;
  GeglRectangle    tile_rect;

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
                    "%s: ERROR", G_STRFUNC);
      gimp_plug_in_close (plug_in, TRUE);
      return;
    }

  if (! gimp_wire_read_msg (plug_in->my_read, &msg, plug_in))
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
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
                    "tried writing to invalid drawable %d (killing)",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file),
                    tile_info->drawable_ID);
      gimp_plug_in_close (plug_in, TRUE);
      return;
    }
  else if (gimp_item_is_removed (GIMP_ITEM (drawable)))
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "tried writing to drawable %d which was removed "
                    "from the image (killing)",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file),
                    tile_info->drawable_ID);
      gimp_plug_in_close (plug_in, TRUE);
      return;
    }

  if (tile_info->shadow)
    {

      /*  don't check whether the drawable is a group or locked here,
       *  the plugin will get a proper error message when it tries to
       *  merge the shadow tiles, which is much better than just
       *  killing it.
       */
      buffer = gimp_drawable_get_shadow_buffer (drawable);

      gimp_plug_in_cleanup_add_shadow (plug_in, drawable);
    }
  else
    {
      if (gimp_item_is_content_locked (GIMP_ITEM (drawable)))
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "Plug-In \"%s\"\n(%s)\n\n"
                        "tried writing to a locked drawable %d (killing)",
                        gimp_object_get_name (plug_in),
                        gimp_file_get_utf8_name (plug_in->file),
                        tile_info->drawable_ID);
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }
      else if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "Plug-In \"%s\"\n(%s)\n\n"
                        "tried writing to a group layer %d (killing)",
                        gimp_object_get_name (plug_in),
                        gimp_file_get_utf8_name (plug_in->file),
                        tile_info->drawable_ID);
          gimp_plug_in_close (plug_in, TRUE);
          return;
        }

      buffer = gimp_drawable_get_buffer (drawable);
    }

  if (! gimp_gegl_buffer_get_tile_rect (buffer,
                                        GIMP_PLUG_IN_TILE_WIDTH,
                                        GIMP_PLUG_IN_TILE_HEIGHT,
                                        tile_info->tile_num,
                                        &tile_rect))
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "requested invalid tile (killing)",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
      gimp_plug_in_close (plug_in, TRUE);
      return;
    }

  format = gegl_buffer_get_format (buffer);

  if (! gimp_plug_in_precision_enabled (plug_in))
    {
      format = gimp_babl_compat_u8_format (format);
    }

  if (tile_data.use_shm)
    {
      gegl_buffer_set (buffer, &tile_rect, 0, format,
                       gimp_plug_in_shm_get_addr (plug_in->manager->shm),
                       GEGL_AUTO_ROWSTRIDE);
    }
  else
    {
      gegl_buffer_set (buffer, &tile_rect, 0, format,
                       tile_info->data,
                       GEGL_AUTO_ROWSTRIDE);
    }

  gimp_wire_destroy (&msg);

  if (! gp_tile_ack_write (plug_in->my_write, plug_in))
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      gimp_plug_in_close (plug_in, TRUE);
      return;
    }
}

static void
gimp_plug_in_handle_tile_get (GimpPlugIn *plug_in,
                              GPTileReq  *request)
{
  GPTileData       tile_data;
  GimpWireMessage  msg;
  GimpDrawable    *drawable;
  GeglBuffer      *buffer;
  const Babl      *format;
  GeglRectangle    tile_rect;
  gint             tile_size;

  drawable = (GimpDrawable *) gimp_item_get_by_ID (plug_in->manager->gimp,
                                                   request->drawable_ID);

  if (! GIMP_IS_DRAWABLE (drawable))
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "tried reading from invalid drawable %d (killing)",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file),
                    request->drawable_ID);
      gimp_plug_in_close (plug_in, TRUE);
      return;
    }
  else if (gimp_item_is_removed (GIMP_ITEM (drawable)))
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "tried reading from drawable %d which was removed "
                    "from the image (killing)",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file),
                    request->drawable_ID);
      gimp_plug_in_close (plug_in, TRUE);
      return;
    }

  if (request->shadow)
    {
      buffer = gimp_drawable_get_shadow_buffer (drawable);

      gimp_plug_in_cleanup_add_shadow (plug_in, drawable);
    }
  else
    {
      buffer = gimp_drawable_get_buffer (drawable);
    }

  if (! gimp_gegl_buffer_get_tile_rect (buffer,
                                        GIMP_PLUG_IN_TILE_WIDTH,
                                        GIMP_PLUG_IN_TILE_HEIGHT,
                                        request->tile_num,
                                        &tile_rect))
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "requested invalid tile (killing)",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
      gimp_plug_in_close (plug_in, TRUE);
      return;
    }

  format = gegl_buffer_get_format (buffer);

  if (! gimp_plug_in_precision_enabled (plug_in))
    {
      format = gimp_babl_compat_u8_format (format);
    }

  tile_size = (babl_format_get_bytes_per_pixel (format) *
               tile_rect.width * tile_rect.height);

  tile_data.drawable_ID = request->drawable_ID;
  tile_data.tile_num    = request->tile_num;
  tile_data.shadow      = request->shadow;
  tile_data.bpp         = babl_format_get_bytes_per_pixel (format);
  tile_data.width       = tile_rect.width;
  tile_data.height      = tile_rect.height;
  tile_data.use_shm     = (plug_in->manager->shm != NULL);

  if (tile_data.use_shm)
    {
      gegl_buffer_get (buffer, &tile_rect, 1.0, format,
                       gimp_plug_in_shm_get_addr (plug_in->manager->shm),
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
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
      gimp_plug_in_close (plug_in, TRUE);
      return;
    }

  if (! gimp_wire_read_msg (plug_in->my_read, &msg, plug_in))
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "%s: ERROR", G_STRFUNC);
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

static void
gimp_plug_in_handle_proc_error (GimpPlugIn          *plug_in,
                                GimpPlugInProcFrame *proc_frame,
                                const gchar         *name,
                                const GError        *error)
{
  switch (proc_frame->error_handler)
    {
    case GIMP_PDB_ERROR_HANDLER_INTERNAL:
      if (error->domain == GIMP_PDB_ERROR)
        {
          gimp_message (plug_in->manager->gimp,
                        G_OBJECT (proc_frame->progress),
                        GIMP_MESSAGE_ERROR,
                        _("Calling error for procedure '%s':\n"
                          "%s"),
                        name, error->message);
        }
      else
        {
          gimp_message (plug_in->manager->gimp,
                        G_OBJECT (proc_frame->progress),
                        GIMP_MESSAGE_ERROR,
                        _("Execution error for procedure '%s':\n"
                          "%s"),
                        name, error->message);
        }
      break;

    case GIMP_PDB_ERROR_HANDLER_PLUGIN:
      /*  the plug-in is responsible for handling this error  */
      break;
    }
}

static void
gimp_plug_in_handle_proc_run (GimpPlugIn *plug_in,
                              GPProcRun  *proc_run)
{
  GimpPlugInProcFrame *proc_frame;
  gchar               *canonical;
  const gchar         *proc_name   = NULL;
  GimpProcedure       *procedure;
  GimpValueArray      *args        = NULL;
  GimpValueArray      *return_vals = NULL;
  GError              *error       = NULL;

  g_return_if_fail (proc_run != NULL);
  g_return_if_fail (proc_run->name != NULL);

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
                            gimp_object_get_name (plug_in),
                            gimp_file_get_utf8_name (plug_in->file),
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
                            gimp_object_get_name (plug_in),
                            gimp_file_get_utf8_name (plug_in->file),
                            canonical);
            }
          else
            {
              gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_WARNING,
                            "WARNING: Plug-In \"%s\"\n(%s)\n"
                            "called deprecated procedure '%s'.\n"
                            "It should call '%s' instead!",
                            gimp_object_get_name (plug_in),
                            gimp_file_get_utf8_name (plug_in->file),
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
                                                         &error,
                                                         proc_name,
                                                         args);
  gimp_plug_in_manager_plug_in_pop (plug_in->manager);

  gimp_value_array_unref (args);

  if (error)
    {
      gimp_plug_in_handle_proc_error (plug_in, proc_frame,
                                      canonical, error);
      g_error_free (error);
    }

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
      proc_return.nparams = gimp_value_array_length (return_vals);
      proc_return.params  = plug_in_args_to_params (return_vals, FALSE);

      if (! gp_proc_return_write (plug_in->my_write, &proc_return, plug_in))
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "%s: ERROR", G_STRFUNC);
          gimp_plug_in_close (plug_in, TRUE);
        }

      g_free (proc_return.params);
    }

  gimp_value_array_unref (return_vals);
}

static void
gimp_plug_in_handle_proc_return (GimpPlugIn   *plug_in,
                                 GPProcReturn *proc_return)
{
  GimpPlugInProcFrame *proc_frame = &plug_in->main_proc_frame;

  g_return_if_fail (proc_return != NULL);

  proc_frame->return_vals =
    plug_in_params_to_args (proc_frame->procedure->values,
                            proc_frame->procedure->num_values,
                            proc_return->params,
                            proc_return->nparams,
                            TRUE, TRUE);

  if (proc_frame->main_loop)
    {
      g_main_loop_quit (proc_frame->main_loop);
    }
  else
    {
      /*  the plug-in is run asynchronously, so display its error
       *  messages here because nobody else will do it
       */
      gimp_plug_in_procedure_handle_return_values (GIMP_PLUG_IN_PROCEDURE (proc_frame->procedure),
                                                   plug_in->manager->gimp,
                                                   proc_frame->progress,
                                                   proc_frame->return_vals);
    }

  gimp_plug_in_close (plug_in, FALSE);
}

static void
gimp_plug_in_handle_temp_proc_return (GimpPlugIn   *plug_in,
                                      GPProcReturn *proc_return)
{
  g_return_if_fail (proc_return != NULL);

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
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
      gimp_plug_in_close (plug_in, TRUE);
    }
}

static void
gimp_plug_in_handle_proc_install (GimpPlugIn    *plug_in,
                                  GPProcInstall *proc_install)
{
  GimpPlugInProcedure *proc       = NULL;
  GimpProcedure       *procedure  = NULL;
  gchar               *canonical;
  gboolean             null_name  = FALSE;
  gboolean             valid_utf8 = FALSE;
  gint                 i;

  g_return_if_fail (proc_install != NULL);
  g_return_if_fail (proc_install->name != NULL);

  canonical = gimp_canonicalize_identifier (proc_install->name);

  /*  Sanity check for array arguments  */

  for (i = 1; i < proc_install->nparams; i++)
    {
      if ((proc_install->params[i].type == GIMP_PDB_INT32ARRAY ||
           proc_install->params[i].type == GIMP_PDB_INT8ARRAY  ||
           proc_install->params[i].type == GIMP_PDB_FLOATARRAY ||
           proc_install->params[i].type == GIMP_PDB_STRINGARRAY ||
           proc_install->params[i].type == GIMP_PDB_COLORARRAY)
          &&
          proc_install->params[i - 1].type != GIMP_PDB_INT32)
        {
          gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                        "Plug-In \"%s\"\n(%s)\n\n"
                        "attempted to install procedure \"%s\" "
                        "which fails to comply with the array parameter "
                        "passing standard.  Argument %d is noncompliant.",
                        gimp_object_get_name (plug_in),
                        gimp_file_get_utf8_name (plug_in->file),
                        canonical, i);
          g_free (canonical);
          return;
        }
    }

  /*  Sanity check strings for UTF-8 validity  */

#define VALIDATE(str)         (g_utf8_validate ((str), -1, NULL))
#define VALIDATE_OR_NULL(str) ((str) == NULL || g_utf8_validate ((str), -1, NULL))

  if (VALIDATE_OR_NULL (proc_install->menu_path) &&
      VALIDATE         (canonical)               &&
      VALIDATE_OR_NULL (proc_install->blurb)     &&
      VALIDATE_OR_NULL (proc_install->help)      &&
      VALIDATE_OR_NULL (proc_install->author)    &&
      VALIDATE_OR_NULL (proc_install->copyright) &&
      VALIDATE_OR_NULL (proc_install->date))
    {
      null_name  = FALSE;
      valid_utf8 = TRUE;

      for (i = 0; i < proc_install->nparams && valid_utf8 && !null_name; i++)
        {
          if (! proc_install->params[i].name)
            {
              null_name = TRUE;
            }
          else if (! (VALIDATE         (proc_install->params[i].name) &&
                      VALIDATE_OR_NULL (proc_install->params[i].description)))
            {
              valid_utf8 = FALSE;
            }
        }

      for (i = 0; i < proc_install->nreturn_vals && valid_utf8 && !null_name; i++)
        {
          if (! proc_install->return_vals[i].name)
            {
              null_name = TRUE;
            }
          else if (! (VALIDATE         (proc_install->return_vals[i].name) &&
                      VALIDATE_OR_NULL (proc_install->return_vals[i].description)))
            {
              valid_utf8 = FALSE;
            }
        }
    }

#undef VALIDATE
#undef VALIDATE_OR_NULL

  if (null_name)
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "attempted to install a procedure NULL parameter name.",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
      g_free (canonical);
      return;
    }

  if (! valid_utf8)
    {
      gimp_message (plug_in->manager->gimp, NULL, GIMP_MESSAGE_ERROR,
                    "Plug-In \"%s\"\n(%s)\n\n"
                    "attempted to install a procedure with invalid UTF-8 strings.",
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
      g_free (canonical);
      return;
    }

  /*  Create the procedure object  */

  switch (proc_install->type)
    {
    case GIMP_PLUGIN:
    case GIMP_EXTENSION:
      procedure = gimp_plug_in_procedure_new (proc_install->type,
                                              plug_in->file);
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

  if (proc_install->menu_path && strlen (proc_install->menu_path))
    {
      if (proc_install->menu_path[0] == '<')
        {
          GError *error = NULL;

          if (! gimp_plug_in_procedure_add_menu_path (proc,
                                                      proc_install->menu_path,
                                                      &error))
            {
              gimp_message_literal (plug_in->manager->gimp,
                                    NULL, GIMP_MESSAGE_WARNING,
                                    error->message);
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

  g_return_if_fail (proc_uninstall != NULL);
  g_return_if_fail (proc_uninstall->name != NULL);

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
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
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
                    gimp_object_get_name (plug_in),
                    gimp_file_get_utf8_name (plug_in->file));
      gimp_plug_in_close (plug_in, TRUE);
    }
}
