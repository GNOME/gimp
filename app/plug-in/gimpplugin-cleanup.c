/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugin-cleanup.c
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

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawable-shadow.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaundostack.h"

#include "ligmaplugin.h"
#include "ligmaplugin-cleanup.h"
#include "ligmapluginmanager.h"
#include "ligmapluginprocedure.h"

#include "ligma-log.h"


typedef struct _LigmaPlugInCleanupImage LigmaPlugInCleanupImage;

struct _LigmaPlugInCleanupImage
{
  LigmaImage *image;
  gint       image_id;

  gint       undo_group_count;
  gint       layers_freeze_count;
  gint       channels_freeze_count;
  gint       vectors_freeze_count;
};


typedef struct _LigmaPlugInCleanupItem LigmaPlugInCleanupItem;

struct _LigmaPlugInCleanupItem
{
  LigmaItem *item;
  gint      item_id;

  gboolean  shadow_buffer;
};


/*  local function prototypes  */

static LigmaPlugInCleanupImage *
              ligma_plug_in_cleanup_image_new      (LigmaPlugInProcFrame    *proc_frame,
                                                   LigmaImage              *image);
static void   ligma_plug_in_cleanup_image_free     (LigmaPlugInProcFrame    *proc_frame,
                                                   LigmaPlugInCleanupImage *cleanup);
static gboolean
              ligma_plug_in_cleanup_image_is_clean (LigmaPlugInCleanupImage *cleanup);
static LigmaPlugInCleanupImage *
              ligma_plug_in_cleanup_image_get      (LigmaPlugInProcFrame    *proc_frame,
                                                   LigmaImage              *image);
static void   ligma_plug_in_cleanup_image          (LigmaPlugInProcFrame    *proc_frame,
                                                   LigmaPlugInCleanupImage *cleanup);

static LigmaPlugInCleanupItem *
              ligma_plug_in_cleanup_item_new       (LigmaPlugInProcFrame    *proc_frame,
                                                   LigmaItem               *item);
static void   ligma_plug_in_cleanup_item_free      (LigmaPlugInProcFrame    *proc_frame,
                                                   LigmaPlugInCleanupItem  *cleanup);
static gboolean
              ligma_plug_in_cleanup_item_is_clean  (LigmaPlugInCleanupItem  *cleanup);
static LigmaPlugInCleanupItem *
              ligma_plug_in_cleanup_item_get       (LigmaPlugInProcFrame    *proc_frame,
                                                   LigmaItem               *item);
static void   ligma_plug_in_cleanup_item           (LigmaPlugInProcFrame    *proc_frame,
                                                   LigmaPlugInCleanupItem  *cleanup);


/*  public functions  */

gboolean
ligma_plug_in_cleanup_undo_group_start (LigmaPlugIn *plug_in,
                                       LigmaImage  *image)
{
  LigmaPlugInProcFrame    *proc_frame;
  LigmaPlugInCleanupImage *cleanup;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);
  cleanup    = ligma_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = ligma_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->undo_group_count++;

  return TRUE;
}

gboolean
ligma_plug_in_cleanup_undo_group_end (LigmaPlugIn *plug_in,
                                     LigmaImage  *image)
{
  LigmaPlugInProcFrame    *proc_frame;
  LigmaPlugInCleanupImage *cleanup;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);
  cleanup    = ligma_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->undo_group_count > 0)
    {
      cleanup->undo_group_count--;

      if (ligma_plug_in_cleanup_image_is_clean (cleanup))
        ligma_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
ligma_plug_in_cleanup_layers_freeze (LigmaPlugIn *plug_in,
                                    LigmaImage  *image)
{
  LigmaPlugInProcFrame    *proc_frame;
  LigmaPlugInCleanupImage *cleanup;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);
  cleanup    = ligma_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = ligma_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->layers_freeze_count++;

  return TRUE;
}

gboolean
ligma_plug_in_cleanup_layers_thaw (LigmaPlugIn *plug_in,
                                  LigmaImage  *image)
{
  LigmaPlugInProcFrame    *proc_frame;
  LigmaPlugInCleanupImage *cleanup;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);
  cleanup    = ligma_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->layers_freeze_count > 0)
    {
      cleanup->layers_freeze_count--;

      if (ligma_plug_in_cleanup_image_is_clean (cleanup))
        ligma_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
ligma_plug_in_cleanup_channels_freeze (LigmaPlugIn *plug_in,
                                      LigmaImage  *image)
{
  LigmaPlugInProcFrame    *proc_frame;
  LigmaPlugInCleanupImage *cleanup;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);
  cleanup    = ligma_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = ligma_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->channels_freeze_count++;

  return TRUE;
}

gboolean
ligma_plug_in_cleanup_channels_thaw (LigmaPlugIn *plug_in,
                                    LigmaImage  *image)
{
  LigmaPlugInProcFrame    *proc_frame;
  LigmaPlugInCleanupImage *cleanup;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);
  cleanup    = ligma_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->channels_freeze_count > 0)
    {
      cleanup->channels_freeze_count--;

      if (ligma_plug_in_cleanup_image_is_clean (cleanup))
        ligma_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
ligma_plug_in_cleanup_vectors_freeze (LigmaPlugIn *plug_in,
                                     LigmaImage  *image)
{
  LigmaPlugInProcFrame    *proc_frame;
  LigmaPlugInCleanupImage *cleanup;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);
  cleanup    = ligma_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    cleanup = ligma_plug_in_cleanup_image_new (proc_frame, image);

  cleanup->vectors_freeze_count++;

  return TRUE;
}

gboolean
ligma_plug_in_cleanup_vectors_thaw (LigmaPlugIn *plug_in,
                                   LigmaImage  *image)
{
  LigmaPlugInProcFrame    *proc_frame;
  LigmaPlugInCleanupImage *cleanup;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);
  cleanup    = ligma_plug_in_cleanup_image_get (proc_frame, image);

  if (! cleanup)
    return FALSE;

  if (cleanup->vectors_freeze_count > 0)
    {
      cleanup->vectors_freeze_count--;

      if (ligma_plug_in_cleanup_image_is_clean (cleanup))
        ligma_plug_in_cleanup_image_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

gboolean
ligma_plug_in_cleanup_add_shadow (LigmaPlugIn   *plug_in,
                                 LigmaDrawable *drawable)
{
  LigmaPlugInProcFrame   *proc_frame;
  LigmaPlugInCleanupItem *cleanup;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);
  cleanup    = ligma_plug_in_cleanup_item_get (proc_frame, LIGMA_ITEM (drawable));

  if (! cleanup)
    cleanup = ligma_plug_in_cleanup_item_new (proc_frame, LIGMA_ITEM (drawable));

  cleanup->shadow_buffer = TRUE;

  return TRUE;
}

gboolean
ligma_plug_in_cleanup_remove_shadow (LigmaPlugIn   *plug_in,
                                    LigmaDrawable *drawable)
{
  LigmaPlugInProcFrame   *proc_frame;
  LigmaPlugInCleanupItem *cleanup;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);
  cleanup    = ligma_plug_in_cleanup_item_get (proc_frame, LIGMA_ITEM (drawable));

  if (! cleanup)
    return FALSE;

  if (cleanup->shadow_buffer)
    {
      cleanup->shadow_buffer = FALSE;

      if (ligma_plug_in_cleanup_item_is_clean (cleanup))
        ligma_plug_in_cleanup_item_free (proc_frame, cleanup);

      return TRUE;
    }

  return FALSE;
}

void
ligma_plug_in_cleanup (LigmaPlugIn          *plug_in,
                      LigmaPlugInProcFrame *proc_frame)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));
  g_return_if_fail (proc_frame != NULL);

  while (proc_frame->image_cleanups)
    {
      LigmaPlugInCleanupImage *cleanup = proc_frame->image_cleanups->data;

      if (ligma_image_get_by_id (plug_in->manager->ligma,
                                cleanup->image_id) == cleanup->image)
        {
          ligma_plug_in_cleanup_image (proc_frame, cleanup);
        }

      ligma_plug_in_cleanup_image_free (proc_frame, cleanup);
    }

  while (proc_frame->item_cleanups)
    {
      LigmaPlugInCleanupItem *cleanup = proc_frame->item_cleanups->data;

      if (ligma_item_get_by_id (plug_in->manager->ligma,
                               cleanup->item_id) == cleanup->item)
        {
          ligma_plug_in_cleanup_item (proc_frame, cleanup);
        }

      ligma_plug_in_cleanup_item_free (proc_frame, cleanup);
    }
}


/*  private functions  */

static LigmaPlugInCleanupImage *
ligma_plug_in_cleanup_image_new (LigmaPlugInProcFrame *proc_frame,
                                LigmaImage           *image)
{
  LigmaPlugInCleanupImage *cleanup = g_slice_new0 (LigmaPlugInCleanupImage);

  cleanup->image    = image;
  cleanup->image_id = ligma_image_get_id (image);

  proc_frame->image_cleanups = g_list_prepend (proc_frame->image_cleanups,
                                               cleanup);

  return cleanup;
}

static void
ligma_plug_in_cleanup_image_free (LigmaPlugInProcFrame    *proc_frame,
                                 LigmaPlugInCleanupImage *cleanup)
{
  proc_frame->image_cleanups = g_list_remove (proc_frame->image_cleanups,
                                              cleanup);

  g_slice_free (LigmaPlugInCleanupImage, cleanup);
}

static gboolean
ligma_plug_in_cleanup_image_is_clean (LigmaPlugInCleanupImage *cleanup)
{
  if (cleanup->undo_group_count > 0)
    return FALSE;

  if (cleanup->layers_freeze_count > 0)
    return FALSE;

  if (cleanup->channels_freeze_count > 0)
    return FALSE;

  if (cleanup->vectors_freeze_count > 0)
    return FALSE;

  return TRUE;
}

static LigmaPlugInCleanupImage *
ligma_plug_in_cleanup_image_get (LigmaPlugInProcFrame *proc_frame,
                                LigmaImage           *image)
{
  GList *list;

  for (list = proc_frame->image_cleanups; list; list = g_list_next (list))
    {
      LigmaPlugInCleanupImage *cleanup = list->data;

      if (cleanup->image == image)
        return cleanup;
    }

  return NULL;
}

static void
ligma_plug_in_cleanup_image (LigmaPlugInProcFrame    *proc_frame,
                            LigmaPlugInCleanupImage *cleanup)
{
  LigmaImage     *image = cleanup->image;
  LigmaContainer *container;

  if (cleanup->undo_group_count > 0)
    {
      g_message ("Plug-in '%s' left image undo in inconsistent state, "
                 "closing open undo groups.",
                 ligma_procedure_get_label (proc_frame->procedure));

      while (cleanup->undo_group_count--)
        if (! ligma_image_undo_group_end (image))
          break;
    }

  container = ligma_image_get_layers (image);

  if (cleanup->layers_freeze_count > 0)
    {
      g_message ("Plug-in '%s' left image's layers frozen, "
                 "thawing layers.",
                 ligma_procedure_get_label (proc_frame->procedure));

      while (cleanup->layers_freeze_count-- > 0 &&
             ligma_container_frozen (container))
        {
          ligma_container_thaw (container);
        }
    }

  container = ligma_image_get_channels (image);

  if (cleanup->channels_freeze_count > 0)
    {
      g_message ("Plug-in '%s' left image's channels frozen, "
                 "thawing channels.",
                 ligma_procedure_get_label (proc_frame->procedure));

      while (cleanup->channels_freeze_count-- > 0 &&
             ligma_container_frozen (container))
        {
          ligma_container_thaw (container);
        }
    }

  container = ligma_image_get_vectors (image);

  if (cleanup->vectors_freeze_count > 0)
    {
      g_message ("Plug-in '%s' left image's vectors frozen, "
                 "thawing vectors.",
                 ligma_procedure_get_label (proc_frame->procedure));

      while (cleanup->vectors_freeze_count > 0 &&
             ligma_container_frozen (container))
        {
          ligma_container_thaw (container);
        }
    }
}

static LigmaPlugInCleanupItem *
ligma_plug_in_cleanup_item_new (LigmaPlugInProcFrame *proc_frame,
                               LigmaItem            *item)
{
  LigmaPlugInCleanupItem *cleanup = g_slice_new0 (LigmaPlugInCleanupItem);

  cleanup->item    = item;
  cleanup->item_id = ligma_item_get_id (item);

  proc_frame->item_cleanups = g_list_prepend (proc_frame->item_cleanups,
                                              cleanup);

  return cleanup;
}

static void
ligma_plug_in_cleanup_item_free (LigmaPlugInProcFrame   *proc_frame,
                                LigmaPlugInCleanupItem *cleanup)
{
  proc_frame->item_cleanups = g_list_remove (proc_frame->item_cleanups,
                                             cleanup);

  g_slice_free (LigmaPlugInCleanupItem, cleanup);
}

static gboolean
ligma_plug_in_cleanup_item_is_clean (LigmaPlugInCleanupItem *cleanup)
{
  if (cleanup->shadow_buffer)
    return FALSE;

  return TRUE;
}

static LigmaPlugInCleanupItem *
ligma_plug_in_cleanup_item_get (LigmaPlugInProcFrame *proc_frame,
                               LigmaItem            *item)
{
  GList *list;

  for (list = proc_frame->item_cleanups; list; list = g_list_next (list))
    {
      LigmaPlugInCleanupItem *cleanup = list->data;

      if (cleanup->item == item)
        return cleanup;
    }

  return NULL;
}

static void
ligma_plug_in_cleanup_item (LigmaPlugInProcFrame   *proc_frame,
                           LigmaPlugInCleanupItem *cleanup)
{
  LigmaItem *item = cleanup->item;

  if (cleanup->shadow_buffer)
    {
      LIGMA_LOG (SHADOW_TILES,
                "Freeing shadow buffer of drawable '%s' on behalf of '%s'.",
                ligma_object_get_name (item),
                ligma_procedure_get_label (proc_frame->procedure));

      ligma_drawable_free_shadow_buffer (LIGMA_DRAWABLE (item));

      cleanup->shadow_buffer = FALSE;
    }
}
