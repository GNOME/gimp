/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin-cleanup.c
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

#include "glib-object.h"

#include "plug-in-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpdrawable.h"
#include "core/gimpundostack.h"

#include "gimpplugin.h"
#include "gimpplugin-cleanup.h"
#include "gimppluginmanager.h"
#include "gimppluginprocedure.h"


typedef struct _GimpPlugInCleanupImage GimpPlugInCleanupImage;

struct _GimpPlugInCleanupImage
{
  GimpImage *image;
  gint       undo_group_count;
};


/*  local function prototypes  */

static GimpPlugInCleanupImage *
         gimp_plug_in_cleanup_get_image (GimpPlugInProcFrame *proc_frame,
                                         GimpImage           *image);


/*  public functions  */

gboolean
gimp_plug_in_cleanup_undo_group_start (GimpPlugIn *plug_in,
                                       GimpImage  *image)
{
  GimpPlugInProcFrame    *proc_frame;
  GimpPlugInCleanupImage *cleanup;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);
  cleanup    = gimp_plug_in_cleanup_get_image (proc_frame, image);

  if (! cleanup)
    {
      cleanup = g_new0 (GimpPlugInCleanupImage, 1);

      cleanup->image            = image;
      cleanup->undo_group_count = image->group_count;

      proc_frame->cleanups = g_list_prepend (proc_frame->cleanups, cleanup);
    }

  return TRUE;
}

gboolean
gimp_plug_in_cleanup_undo_group_end (GimpPlugIn *plug_in,
                                     GimpImage  *image)
{
  GimpPlugInProcFrame    *proc_frame;
  GimpPlugInCleanupImage *cleanup;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);
  cleanup    = gimp_plug_in_cleanup_get_image (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->undo_group_count == image->group_count - 1)
    {
      proc_frame->cleanups = g_list_remove (proc_frame->cleanups, cleanup);
      g_free (cleanup);
    }

  return TRUE;
}

void
gimp_plug_in_cleanup (GimpPlugIn          *plug_in,
                      GimpPlugInProcFrame *proc_frame)
{
  GList *list;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (proc_frame != NULL);

  for (list = proc_frame->cleanups; list; list = g_list_next (list))
    {
      GimpPlugInCleanupImage *cleanup = list->data;
      GimpImage              *image   = cleanup->image;

      if (! gimp_container_have (plug_in->manager->gimp->images,
                                 (GimpObject *) image))
        continue;

      if (image->pushing_undo_group == GIMP_UNDO_GROUP_NONE)
        continue;

      if (cleanup->undo_group_count != image->group_count)
        {
          GimpProcedure *proc = proc_frame->procedure;

          g_message ("Plug-In '%s' left image undo in inconsistent state, "
                     "closing open undo groups.",
                     gimp_plug_in_procedure_get_label (GIMP_PLUG_IN_PROCEDURE (proc)));

          while (image->pushing_undo_group != GIMP_UNDO_GROUP_NONE &&
                 cleanup->undo_group_count < image->group_count)
            {
              if (! gimp_image_undo_group_end (image))
                break;
            }
        }

      g_free (cleanup);
    }

  g_list_free (proc_frame->cleanups);
  proc_frame->cleanups = NULL;
}


/*  private functions  */

static GimpPlugInCleanupImage *
gimp_plug_in_cleanup_get_image (GimpPlugInProcFrame *proc_frame,
                                GimpImage           *image)
{
  GList *list;

  for (list = proc_frame->cleanups; list; list = g_list_next (list))
    {
      GimpPlugInCleanupImage *cleanup = list->data;

      if (cleanup->image == image)
        return cleanup;
    }

  return NULL;
}
