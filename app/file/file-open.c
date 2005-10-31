/* The GIMP -- an image manipulation program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * file-open.c
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#define R_OK 4
#endif

#include "core/core-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdocumentlist.h"
#include "core/gimpimage.h"
#include "core/gimpimage-merge.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimagefile.h"
#include "core/gimplayer.h"
#include "core/gimpprogress.h"

#include "pdb/procedural_db.h"

#include "plug-in/plug-in.h"
#include "plug-in/plug-in-proc-def.h"

#include "file-open.h"
#include "file-utils.h"
#include "gimprecentlist.h"

#include "gimp-intl.h"


static void  file_open_sanitize_image (GimpImage *gimage);


/*  public functions  */

GimpImage *
file_open_image (Gimp               *gimp,
                 GimpContext        *context,
                 GimpProgress       *progress,
		 const gchar        *uri,
		 const gchar        *entered_filename,
		 PlugInProcDef      *file_proc,
		 GimpRunMode         run_mode,
		 GimpPDBStatusType  *status,
                 const gchar       **mime_type,
                 GError            **error)
{
  const ProcRecord *proc;
  Argument         *args;
  Argument         *return_vals;
  gint              image_id;
  gint              i;
  gchar            *filename;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  *status = GIMP_PDB_EXECUTION_ERROR;

  if (! file_proc)
    file_proc = file_utils_find_proc (gimp->load_procs, uri);

  if (! file_proc)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unknown file type"));
      return NULL;
    }

  filename = file_utils_filename_from_uri (uri);

  if (filename)
    {
      /* check if we are opening a file */
      if (g_file_test (filename, G_FILE_TEST_EXISTS))
        {
          if (! g_file_test (filename, G_FILE_TEST_IS_REGULAR))
            {
              g_free (filename);
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Not a regular file"));
              return NULL;
            }

          if (g_access (filename, R_OK) != 0)
            {
              g_free (filename);
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_ACCES,
                           g_strerror (errno));
              return NULL;
            }
        }
    }

  proc = plug_in_proc_def_get_proc (file_proc);

  args = g_new0 (Argument, proc->num_args);

  for (i = 0; i < proc->num_args; i++)
    args[i].arg_type = proc->args[i].arg_type;

  args[0].value.pdb_int     = run_mode;
  args[1].value.pdb_pointer = filename ? filename : (gchar *) uri;
  args[2].value.pdb_pointer = (gchar *) entered_filename;

  return_vals = procedural_db_execute (gimp, context, progress,
                                       proc->name, args);

  if (filename)
    g_free (filename);

  *status  = return_vals[0].value.pdb_int;
  image_id = return_vals[1].value.pdb_int;

  procedural_db_destroy_args (return_vals, proc->num_values);
  g_free (args);

  if (*status == GIMP_PDB_SUCCESS)
    {
      if (image_id != -1)
        {
          GimpImage *gimage = gimp_image_get_by_ID (gimp, image_id);

          file_open_sanitize_image (gimage);

          if (mime_type)
            *mime_type = file_proc->mime_type;

          return gimage;
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Plug-In returned SUCCESS but did not "
                         "return an image"));
          *status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (*status != GIMP_PDB_CANCEL)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Plug-In could not open image"));
    }

  return NULL;
}

/*  Attempts to load a thumbnail by using a registered thumbnail loader.  */
GimpImage *
file_open_thumbnail (Gimp          *gimp,
                     GimpContext   *context,
                     GimpProgress  *progress,
                     const gchar   *uri,
                     gint           size,
                     const gchar  **mime_type,
                     gint          *image_width,
                     gint          *image_height)
{
  PlugInProcDef     *file_proc;
  const ProcRecord  *proc;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (mime_type != NULL, NULL);
  g_return_val_if_fail (image_width != NULL, NULL);
  g_return_val_if_fail (image_height != NULL, NULL);

  *image_width  = 0;
  *image_height = 0;

  file_proc = file_utils_find_proc (gimp->load_procs, uri);

  if (! file_proc || ! file_proc->thumb_loader)
    return NULL;

  proc = procedural_db_lookup (gimp, file_proc->thumb_loader);

  if (proc && proc->num_args >= 2 && proc->num_values >= 1)
    {
      GimpPDBStatusType  status;
      Argument          *args;
      Argument          *return_vals;
      gchar             *filename;
      gint               image_id;
      gint               i;

      filename = file_utils_filename_from_uri (uri);

      args = g_new0 (Argument, proc->num_args);

      for (i = 0; i < proc->num_args; i++)
        args[i].arg_type = proc->args[i].arg_type;

      args[0].value.pdb_pointer = filename ? filename : (gchar *) uri;
      args[1].value.pdb_int     = size;

      return_vals = procedural_db_execute (gimp, context, progress,
                                           proc->name, args);

      if (filename)
        g_free (filename);

      status   = return_vals[0].value.pdb_int;
      image_id = return_vals[1].value.pdb_int;

      if (proc->num_values >= 3)
        {
          *image_width  = MAX (0, return_vals[2].value.pdb_int);
          *image_height = MAX (0, return_vals[3].value.pdb_int);
        }

      procedural_db_destroy_args (return_vals, proc->num_values);
      g_free (args);

      if (status == GIMP_PDB_SUCCESS && image_id != -1)
        {
          GimpImage *image = gimp_image_get_by_ID (gimp, image_id);

          file_open_sanitize_image (image);

          *mime_type = file_proc->mime_type;

          g_printerr ("opened thumbnail at %d x %d\n",
                      image->width, image->height);

          return image;
        }
    }

  return NULL;
}

GimpImage *
file_open_with_display (Gimp               *gimp,
                        GimpContext        *context,
                        GimpProgress       *progress,
                        const gchar        *uri,
                        GimpPDBStatusType  *status,
                        GError            **error)
{
  return file_open_with_proc_and_display (gimp, context, progress,
                                          uri, uri, NULL, status, error);
}

GimpImage *
file_open_with_proc_and_display (Gimp               *gimp,
                                 GimpContext        *context,
                                 GimpProgress       *progress,
                                 const gchar        *uri,
                                 const gchar        *entered_filename,
                                 PlugInProcDef      *file_proc,
                                 GimpPDBStatusType  *status,
                                 GError            **error)
{
  GimpImage   *gimage;
  const gchar *mime_type = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (status != NULL, NULL);

  gimage = file_open_image (gimp, context, progress,
                            uri,
                            entered_filename,
                            file_proc,
                            GIMP_RUN_INTERACTIVE,
                            status,
                            &mime_type,
                            error);

  if (gimage)
    {
      GimpDocumentList *documents = GIMP_DOCUMENT_LIST (gimp->documents);
      GimpImagefile    *imagefile;

      gimp_create_display (gimage->gimp, gimage, GIMP_UNIT_PIXEL, 1.0);

      imagefile = gimp_document_list_add_uri (documents, uri, mime_type);

      /*  can only create a thumbnail if the passed uri and the
       *  resulting image's uri match.
       */
      if (strcmp (uri, gimp_image_get_uri (gimage)) == 0)
        {
          /*  no need to save a thumbnail if there's a good one already  */
          if (! gimp_imagefile_check_thumbnail (imagefile))
            {
              gimp_imagefile_save_thumbnail (imagefile, mime_type, gimage);
            }
        }

      if (gimp->config->save_document_history)
        gimp_recent_list_add_uri (uri, mime_type);

      /*  the display owns the image now  */
      g_object_unref (gimage);
    }

  return gimage;
}

GimpLayer *
file_open_layer (Gimp               *gimp,
                 GimpContext        *context,
                 GimpProgress       *progress,
                 GimpImage          *dest_image,
                 const gchar        *uri,
                 GimpRunMode         run_mode,
                 GimpPDBStatusType  *status,
                 GError            **error)
{
  GimpLayer   *new_layer = NULL;
  GimpImage   *new_image;
  const gchar *mime_type = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (uri != NULL, NULL);
  g_return_val_if_fail (status != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  new_image = file_open_image (gimp, context, progress,
                               uri, uri,
                               NULL, run_mode,
                               status, &mime_type, error);

  if (new_image)
    {
      GList     *list;
      GimpLayer *layer     = NULL;
      gint       n_visible = 0;

      gimp_image_undo_disable (new_image);

      for (list = GIMP_LIST (new_image->layers)->list;
           list;
           list = g_list_next (list))
        {
          if (gimp_item_get_visible (list->data))
            {
              n_visible++;

              if (! layer)
                layer = list->data;
            }
        }

      if (n_visible > 1)
        layer = gimp_image_merge_visible_layers (new_image, context,
                                                 GIMP_CLIP_TO_IMAGE);

      if (layer)
        {
          GimpItem *item = gimp_item_convert (GIMP_ITEM (layer), dest_image,
                                              G_TYPE_FROM_INSTANCE (layer),
                                              TRUE);

          if (item)
            {
              gchar *basename = file_utils_uri_display_basename (uri);

              new_layer = GIMP_LAYER (item);

              gimp_object_set_name (GIMP_OBJECT (new_layer), basename);
              g_free (basename);

              gimp_document_list_add_uri (GIMP_DOCUMENT_LIST (gimp->documents),
                                          uri, mime_type);

              if (gimp->config->save_document_history)
                gimp_recent_list_add_uri (uri, mime_type);
            }
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Image doesn't contain any visible layers"));
          *status = GIMP_PDB_EXECUTION_ERROR;
       }

      g_object_unref (new_image);
    }

  return new_layer;
}


/*  private functions  */

static void
file_open_sanitize_image (GimpImage *gimage)
{
  /* clear all undo steps */
  gimp_image_undo_free (gimage);

  /* make sure that undo is enabled */
  while (gimage->undo_freeze_count)
    gimp_image_undo_thaw (gimage);

  /* set the image to clean  */
  gimp_image_clean_all (gimage);

  gimp_image_invalidate_layer_previews (gimage);
  gimp_image_invalidate_channel_previews (gimage);
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
}
