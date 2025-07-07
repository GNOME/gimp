/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin-cleanup.c
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

#include "plug-in-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-shadow.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpundostack.h"

#include "gimpplugin.h"
#include "gimpplugin-cleanup.h"
#include "gimppluginmanager.h"
#include "gimppluginprocedure.h"

#include "gimp-log.h"


typedef struct _GimpPlugInCleanupImage GimpPlugInCleanupImage;

struct _GimpPlugInCleanupImage
{
  GimpImage *image;
  gint       image_id;

  gint       undo_group_count;
  gint       layers_freeze_count;
  gint       channels_freeze_count;
  gint       paths_freeze_count;
};


typedef struct _GimpPlugInCleanupItem GimpPlugInCleanupItem;

struct _GimpPlugInCleanupItem
{
  GimpItem *item;
  gint      item_id;

  gboolean  shadow_buffer;
};


/*  local function prototypes  */

static GimpPlugInCleanupImage *
              gimp_plug_in_cleanup_image_new      (GimpPlugInProcFrame    *proc_frame,
                                                   GimpImage              *image);
static void   gimp_plug_in_cleanup_image_free     (GimpPlugInProcFrame    *proc_frame,
                                                   GimpPlugInCleanupImage *cleanup);
static gboolean
              gimp_plug_in_cleanup_image_is_clean (GimpPlugInCleanupImage *cleanup);
static GimpPlugInCleanupImage *
              gimp_plug_in_cleanup_image_get      (GimpPlugInProcFrame    *proc_frame,
                                                   GimpImage              *image);
static void   gimp_plug_in_cleanup_image          (GimpPlugInProcFrame    *proc_frame,
                                                   GimpPlugInCleanupImage *cleanup);

static GimpPlugInCleanupItem *
              gimp_plug_in_cleanup_item_new       (GimpPlugInProcFrame    *proc_frame,
                                                   GimpItem               *item);
static void   gimp_plug_in_cleanup_item_free      (GimpPlugInProcFrame    *proc_frame,
                                                   GimpPlugInCleanupItem  *cleanup);
static gboolean
              gimp_plug_in_cleanup_item_is_clean  (GimpPlugInCleanupItem  *cleanup);
static GimpPlugInCleanupItem *
              gimp_plug_in_cleanup_item_get       (GimpPlugInProcFrame    *proc_frame,
                                                   GimpItem               *item);
static void   gimp_plug_in_cleanup_item           (GimpPlugInProcFrame    *proc_frame,
                                                   GimpPlugInCleanupItem  *cleanup);


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
  cleanup    = gimp_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = gimp_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->undo_group_count++;

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
  cleanup    = gimp_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->undo_group_count > 0)
    {
      cleanup->undo_group_count--;

      if (gimp_plug_in_cleanup_image_is_clean (cleanup))
        gimp_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_plug_in_cleanup_layers_freeze (GimpPlugIn *plug_in,
                                    GimpImage  *image)
{
  GimpPlugInProcFrame    *proc_frame;
  GimpPlugInCleanupImage *cleanup;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);
  cleanup    = gimp_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = gimp_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->layers_freeze_count++;

  return TRUE;
}

gboolean
gimp_plug_in_cleanup_layers_thaw (GimpPlugIn *plug_in,
                                  GimpImage  *image)
{
  GimpPlugInProcFrame    *proc_frame;
  GimpPlugInCleanupImage *cleanup;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);
  cleanup    = gimp_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->layers_freeze_count > 0)
    {
      cleanup->layers_freeze_count--;

      if (gimp_plug_in_cleanup_image_is_clean (cleanup))
        gimp_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_plug_in_cleanup_channels_freeze (GimpPlugIn *plug_in,
                                      GimpImage  *image)
{
  GimpPlugInProcFrame    *proc_frame;
  GimpPlugInCleanupImage *cleanup;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);
  cleanup    = gimp_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = gimp_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->channels_freeze_count++;

  return TRUE;
}

gboolean
gimp_plug_in_cleanup_channels_thaw (GimpPlugIn *plug_in,
                                    GimpImage  *image)
{
  GimpPlugInProcFrame    *proc_frame;
  GimpPlugInCleanupImage *cleanup;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);
  cleanup    = gimp_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->channels_freeze_count > 0)
    {
      cleanup->channels_freeze_count--;

      if (gimp_plug_in_cleanup_image_is_clean (cleanup))
        gimp_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_plug_in_cleanup_paths_freeze (GimpPlugIn *plug_in,
                                   GimpImage  *image)
{
  GimpPlugInProcFrame    *proc_frame;
  GimpPlugInCleanupImage *cleanup;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);
  cleanup    = gimp_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = gimp_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->paths_freeze_count++;

  return TRUE;
}

gboolean
gimp_plug_in_cleanup_paths_thaw (GimpPlugIn *plug_in,
                                 GimpImage  *image)
{
  GimpPlugInProcFrame    *proc_frame;
  GimpPlugInCleanupImage *cleanup;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);
  cleanup    = gimp_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->paths_freeze_count > 0)
    {
      cleanup->paths_freeze_count--;

      if (gimp_plug_in_cleanup_image_is_clean (cleanup))
        gimp_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_plug_in_cleanup_add_shadow (GimpPlugIn   *plug_in,
                                 GimpDrawable *drawable)
{
  GimpPlugInProcFrame   *proc_frame;
  GimpPlugInCleanupItem *cleanup;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);
  cleanup    = gimp_plug_in_cleanup_item_get (proc_frame, GIMP_ITEM (drawable));

  if (! cleanup)
    cleanup = gimp_plug_in_cleanup_item_new (proc_frame, GIMP_ITEM (drawable));

  cleanup->shadow_buffer = TRUE;

  return TRUE;
}

gboolean
gimp_plug_in_cleanup_remove_shadow (GimpPlugIn   *plug_in,
                                    GimpDrawable *drawable)
{
  GimpPlugInProcFrame   *proc_frame;
  GimpPlugInCleanupItem *cleanup;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);
  cleanup    = gimp_plug_in_cleanup_item_get (proc_frame, GIMP_ITEM (drawable));

  if (! cleanup)
    return FALSE;

  if (cleanup->shadow_buffer)
    {
      cleanup->shadow_buffer = FALSE;

      if (gimp_plug_in_cleanup_item_is_clean (cleanup))
        gimp_plug_in_cleanup_item_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

void
gimp_plug_in_cleanup (GimpPlugIn          *plug_in,
                      GimpPlugInProcFrame *proc_frame)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (proc_frame != NULL);

  while (proc_frame->image_cleanups)
    {
      GimpPlugInCleanupImage *cleanup = proc_frame->image_cleanups->data;

      if (gimp_image_get_by_id (plug_in->manager->gimp,
                                cleanup->image_id) == cleanup->image)
        {
          gimp_plug_in_cleanup_image (proc_frame, cleanup);
        }

      gimp_plug_in_cleanup_image_free (proc_frame, cleanup);
    }

  while (proc_frame->item_cleanups)
    {
      GimpPlugInCleanupItem *cleanup = proc_frame->item_cleanups->data;

      if (gimp_item_get_by_id (plug_in->manager->gimp,
                               cleanup->item_id) == cleanup->item)
        {
          gimp_plug_in_cleanup_item (proc_frame, cleanup);
        }

      gimp_plug_in_cleanup_item_free (proc_frame, cleanup);
    }
}


/*  private functions  */

static GimpPlugInCleanupImage *
gimp_plug_in_cleanup_image_new (GimpPlugInProcFrame *proc_frame,
                                GimpImage           *image)
{
  GimpPlugInCleanupImage *cleanup = g_slice_new0 (GimpPlugInCleanupImage);

  cleanup->image    = image;
  cleanup->image_id = gimp_image_get_id (image);

  proc_frame->image_cleanups = g_list_prepend (proc_frame->image_cleanups,
                                               cleanup);

  return cleanup;
}

static void
gimp_plug_in_cleanup_image_free (GimpPlugInProcFrame    *proc_frame,
                                 GimpPlugInCleanupImage *cleanup)
{
  proc_frame->image_cleanups = g_list_remove (proc_frame->image_cleanups,
                                              cleanup);

  g_slice_free (GimpPlugInCleanupImage, cleanup);
}

static gboolean
gimp_plug_in_cleanup_image_is_clean (GimpPlugInCleanupImage *cleanup)
{
  if (cleanup->undo_group_count > 0)
    return FALSE;

  if (cleanup->layers_freeze_count > 0)
    return FALSE;

  if (cleanup->channels_freeze_count > 0)
    return FALSE;

  if (cleanup->paths_freeze_count > 0)
    return FALSE;

  return TRUE;
}

static GimpPlugInCleanupImage *
gimp_plug_in_cleanup_image_get (GimpPlugInProcFrame *proc_frame,
                                GimpImage           *image)
{
  GList *list;

  for (list = proc_frame->image_cleanups; list; list = g_list_next (list))
    {
      GimpPlugInCleanupImage *cleanup = list->data;

      if (cleanup->image == image)
        return cleanup;
    }

  return NULL;
}

static void
gimp_plug_in_cleanup_image (GimpPlugInProcFrame    *proc_frame,
                            GimpPlugInCleanupImage *cleanup)
{
  GimpImage     *image = cleanup->image;
  GimpContainer *container;

  if (cleanup->undo_group_count > 0)
    {
      g_message ("Plug-in '%s' left image undo in inconsistent state, "
                 "closing open undo groups.",
                 gimp_procedure_get_label (proc_frame->procedure));

      while (cleanup->undo_group_count--)
        if (! gimp_image_undo_group_end (image))
          break;
    }

  container = gimp_image_get_layers (image);

  if (cleanup->layers_freeze_count > 0)
    {
      g_message ("Plug-in '%s' left image's layers frozen, "
                 "thawing layers.",
                 gimp_procedure_get_label (proc_frame->procedure));

      while (cleanup->layers_freeze_count-- > 0 &&
             gimp_container_frozen (container))
        {
          gimp_container_thaw (container);
        }
    }

  container = gimp_image_get_channels (image);

  if (cleanup->channels_freeze_count > 0)
    {
      g_message ("Plug-in '%s' left image's channels frozen, "
                 "thawing channels.",
                 gimp_procedure_get_label (proc_frame->procedure));

      while (cleanup->channels_freeze_count-- > 0 &&
             gimp_container_frozen (container))
        {
          gimp_container_thaw (container);
        }
    }

  container = gimp_image_get_paths (image);

  if (cleanup->paths_freeze_count > 0)
    {
      g_message ("Plug-in '%s' left image's paths frozen, "
                 "thawing paths.",
                 gimp_procedure_get_label (proc_frame->procedure));

      while (cleanup->paths_freeze_count > 0 &&
             gimp_container_frozen (container))
        {
          gimp_container_thaw (container);
        }
    }
}

static GimpPlugInCleanupItem *
gimp_plug_in_cleanup_item_new (GimpPlugInProcFrame *proc_frame,
                               GimpItem            *item)
{
  GimpPlugInCleanupItem *cleanup = g_slice_new0 (GimpPlugInCleanupItem);

  cleanup->item    = item;
  cleanup->item_id = gimp_item_get_id (item);

  proc_frame->item_cleanups = g_list_prepend (proc_frame->item_cleanups,
                                              cleanup);

  return cleanup;
}

static void
gimp_plug_in_cleanup_item_free (GimpPlugInProcFrame   *proc_frame,
                                GimpPlugInCleanupItem *cleanup)
{
  proc_frame->item_cleanups = g_list_remove (proc_frame->item_cleanups,
                                             cleanup);

  g_slice_free (GimpPlugInCleanupItem, cleanup);
}

static gboolean
gimp_plug_in_cleanup_item_is_clean (GimpPlugInCleanupItem *cleanup)
{
  if (cleanup->shadow_buffer)
    return FALSE;

  return TRUE;
}

static GimpPlugInCleanupItem *
gimp_plug_in_cleanup_item_get (GimpPlugInProcFrame *proc_frame,
                               GimpItem            *item)
{
  GList *list;

  for (list = proc_frame->item_cleanups; list; list = g_list_next (list))
    {
      GimpPlugInCleanupItem *cleanup = list->data;

      if (cleanup->item == item)
        return cleanup;
    }

  return NULL;
}

static void
gimp_plug_in_cleanup_item (GimpPlugInProcFrame   *proc_frame,
                           GimpPlugInCleanupItem *cleanup)
{
  GimpItem *item = cleanup->item;

  if (cleanup->shadow_buffer)
    {
      GIMP_LOG (SHADOW_TILES,
                "Freeing shadow buffer of drawable '%s' on behalf of '%s'.",
                gimp_object_get_name (item),
                gimp_procedure_get_label (proc_frame->procedure));

      gimp_drawable_free_shadow_buffer (GIMP_DRAWABLE (item));

      cleanup->shadow_buffer = FALSE;
    }
}
