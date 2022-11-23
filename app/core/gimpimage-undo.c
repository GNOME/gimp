/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "ligma.h"
#include "ligma-utils.h"
#include "ligmaimage.h"
#include "ligmaimage-private.h"
#include "ligmaimage-undo.h"
#include "ligmaitem.h"
#include "ligmalist.h"
#include "ligmaundostack.h"


/*  local function prototypes  */

static void          ligma_image_undo_pop_stack       (LigmaImage     *image,
                                                      LigmaUndoStack *undo_stack,
                                                      LigmaUndoStack *redo_stack,
                                                      LigmaUndoMode   undo_mode);
static void          ligma_image_undo_free_space      (LigmaImage     *image);
static void          ligma_image_undo_free_redo       (LigmaImage     *image);

static LigmaDirtyMask ligma_image_undo_dirty_from_type (LigmaUndoType   undo_type);


/*  public functions  */

gboolean
ligma_image_undo_is_enabled (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  return (LIGMA_IMAGE_GET_PRIVATE (image)->undo_freeze_count == 0);
}

gboolean
ligma_image_undo_enable (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  /*  Free all undo steps as they are now invalidated  */
  ligma_image_undo_free (image);

  return ligma_image_undo_thaw (image);
}

gboolean
ligma_image_undo_disable (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  return ligma_image_undo_freeze (image);
}

gboolean
ligma_image_undo_freeze (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->undo_freeze_count++;

  if (private->undo_freeze_count == 1)
    ligma_image_undo_event (image, LIGMA_UNDO_EVENT_UNDO_FREEZE, NULL);

  return TRUE;
}

gboolean
ligma_image_undo_thaw (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->undo_freeze_count > 0, FALSE);

  private->undo_freeze_count--;

  if (private->undo_freeze_count == 0)
    ligma_image_undo_event (image, LIGMA_UNDO_EVENT_UNDO_THAW, NULL);

  return TRUE;
}

gboolean
ligma_image_undo (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->pushing_undo_group == LIGMA_UNDO_GROUP_NONE,
                        FALSE);

  ligma_image_undo_pop_stack (image,
                             private->undo_stack,
                             private->redo_stack,
                             LIGMA_UNDO_MODE_UNDO);

  return TRUE;
}

gboolean
ligma_image_redo (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->pushing_undo_group == LIGMA_UNDO_GROUP_NONE,
                        FALSE);

  ligma_image_undo_pop_stack (image,
                             private->redo_stack,
                             private->undo_stack,
                             LIGMA_UNDO_MODE_REDO);

  return TRUE;
}

/*
 * this function continues to undo as long as it only sees certain
 * undo types, in particular visibility changes.
 */
gboolean
ligma_image_strong_undo (LigmaImage *image)
{
  LigmaImagePrivate *private;
  LigmaUndo         *undo;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->pushing_undo_group == LIGMA_UNDO_GROUP_NONE,
                        FALSE);

  undo = ligma_undo_stack_peek (private->undo_stack);

  ligma_image_undo (image);

  while (ligma_undo_is_weak (undo))
    {
      undo = ligma_undo_stack_peek (private->undo_stack);
      if (ligma_undo_is_weak (undo))
        ligma_image_undo (image);
    }

  return TRUE;
}

/*
 * this function continues to redo as long as it only sees certain
 * undo types, in particular visibility changes.  Note that the
 * order of events is set up to make it exactly reverse
 * ligma_image_strong_undo().
 */
gboolean
ligma_image_strong_redo (LigmaImage *image)
{
  LigmaImagePrivate *private;
  LigmaUndo         *undo;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->pushing_undo_group == LIGMA_UNDO_GROUP_NONE,
                        FALSE);

  undo = ligma_undo_stack_peek (private->redo_stack);

  ligma_image_redo (image);

  while (ligma_undo_is_weak (undo))
    {
      undo = ligma_undo_stack_peek (private->redo_stack);
      if (ligma_undo_is_weak (undo))
        ligma_image_redo (image);
    }

  return TRUE;
}

LigmaUndoStack *
ligma_image_get_undo_stack (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->undo_stack;
}

LigmaUndoStack *
ligma_image_get_redo_stack (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->redo_stack;
}

void
ligma_image_undo_free (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  /*  Emit the UNDO_FREE event before actually freeing everything
   *  so the views can properly detach from the undo items
   */
  ligma_image_undo_event (image, LIGMA_UNDO_EVENT_UNDO_FREE, NULL);

  ligma_undo_free (LIGMA_UNDO (private->undo_stack), LIGMA_UNDO_MODE_UNDO);
  ligma_undo_free (LIGMA_UNDO (private->redo_stack), LIGMA_UNDO_MODE_REDO);

  /* If the image was dirty, but could become clean by redo-ing
   * some actions, then it should now become 'infinitely' dirty.
   * This is because we've just nuked the actions that would allow
   * the image to become clean again.
   */
  if (private->dirty < 0)
    private->dirty = 100000;

  /* The same applies to the case where the image would become clean
   * due to undo actions, but since user can't undo without an undo
   * stack, that's not so much a problem.
   */
}

gint
ligma_image_get_undo_group_count (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), 0);

  return LIGMA_IMAGE_GET_PRIVATE (image)->group_count;
}

gboolean
ligma_image_undo_group_start (LigmaImage    *image,
                             LigmaUndoType  undo_type,
                             const gchar  *name)
{
  LigmaImagePrivate *private;
  LigmaUndoStack    *undo_group;
  LigmaDirtyMask     dirty_mask;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (undo_type >  LIGMA_UNDO_GROUP_FIRST &&
                        undo_type <= LIGMA_UNDO_GROUP_LAST, FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (! name)
    name = ligma_undo_type_to_name (undo_type);

  dirty_mask = ligma_image_undo_dirty_from_type (undo_type);

  /* Notify listeners that the image will be modified */
  if (private->group_count == 0 && dirty_mask != LIGMA_DIRTY_NONE)
    ligma_image_dirty (image, dirty_mask);

  if (private->undo_freeze_count > 0)
    return FALSE;

  private->group_count++;

  /*  If we're already in a group...ignore  */
  if (private->group_count > 1)
    return TRUE;

  /*  nuke the redo stack  */
  ligma_image_undo_free_redo (image);

  undo_group = ligma_undo_stack_new (image);

  ligma_object_set_name (LIGMA_OBJECT (undo_group), name);
  LIGMA_UNDO (undo_group)->undo_type  = undo_type;
  LIGMA_UNDO (undo_group)->dirty_mask = dirty_mask;

  ligma_undo_stack_push_undo (private->undo_stack, LIGMA_UNDO (undo_group));

  private->pushing_undo_group = undo_type;

  return TRUE;
}

gboolean
ligma_image_undo_group_end (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->undo_freeze_count > 0)
    return FALSE;

  g_return_val_if_fail (private->group_count > 0, FALSE);

  private->group_count--;

  if (private->group_count == 0)
    {
      private->pushing_undo_group = LIGMA_UNDO_GROUP_NONE;

      /* Do it here, since undo_push doesn't emit this event while in
       * the middle of a group
       */
      ligma_image_undo_event (image, LIGMA_UNDO_EVENT_UNDO_PUSHED,
                             ligma_undo_stack_peek (private->undo_stack));

      ligma_image_undo_free_space (image);
    }

  return TRUE;
}

LigmaUndo *
ligma_image_undo_push (LigmaImage     *image,
                      GType          object_type,
                      LigmaUndoType   undo_type,
                      const gchar   *name,
                      LigmaDirtyMask  dirty_mask,
                      ...)
{
  LigmaImagePrivate  *private;
  gint               n_properties = 0;
  gchar            **names        = NULL;
  GValue            *values       = NULL;
  va_list            args;
  LigmaUndo          *undo;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (g_type_is_a (object_type, LIGMA_TYPE_UNDO), NULL);
  g_return_val_if_fail (undo_type > LIGMA_UNDO_GROUP_LAST, NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  /* Does this undo dirty the image?  If so, we always want to mark
   * image dirty, even if we can't actually push the undo.
   */
  if (dirty_mask != LIGMA_DIRTY_NONE)
    ligma_image_dirty (image, dirty_mask);

  if (private->undo_freeze_count > 0)
    return NULL;

  if (! name)
    name = ligma_undo_type_to_name (undo_type);

  names = ligma_properties_append (object_type,
                                  &n_properties, names, &values,
                                  "name",       name,
                                  "image",      image,
                                  "undo-type",  undo_type,
                                  "dirty-mask", dirty_mask,
                                  NULL);

  va_start (args, dirty_mask);
  names = ligma_properties_append_valist (object_type,
                                         &n_properties, names, &values,
                                         args);
  va_end (args);

  undo = (LigmaUndo *) g_object_new_with_properties (object_type,
                                                    n_properties,
                                                    (const gchar **) names,
                                                    (const GValue *) values);

  ligma_properties_free (n_properties, names, values);

  /*  nuke the redo stack  */
  ligma_image_undo_free_redo (image);

  if (private->pushing_undo_group == LIGMA_UNDO_GROUP_NONE)
    {
      ligma_undo_stack_push_undo (private->undo_stack, undo);

      ligma_image_undo_event (image, LIGMA_UNDO_EVENT_UNDO_PUSHED, undo);

      ligma_image_undo_free_space (image);

      /*  freeing undo space may have freed the newly pushed undo  */
      if (ligma_undo_stack_peek (private->undo_stack) == undo)
        return undo;
    }
  else
    {
      LigmaUndoStack *undo_group;

      undo_group = LIGMA_UNDO_STACK (ligma_undo_stack_peek (private->undo_stack));

      ligma_undo_stack_push_undo (undo_group, undo);

      return undo;
    }

  return NULL;
}

LigmaUndo *
ligma_image_undo_can_compress (LigmaImage    *image,
                              GType         object_type,
                              LigmaUndoType  undo_type)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (ligma_image_is_dirty (image) &&
      ! ligma_undo_stack_peek (private->redo_stack))
    {
      LigmaUndo *undo = ligma_undo_stack_peek (private->undo_stack);

      if (undo && undo->undo_type == undo_type &&
          g_type_is_a (G_TYPE_FROM_INSTANCE (undo), object_type))
        {
          return undo;
        }
    }

  return NULL;
}


/*  private functions  */

static void
ligma_image_undo_pop_stack (LigmaImage     *image,
                           LigmaUndoStack *undo_stack,
                           LigmaUndoStack *redo_stack,
                           LigmaUndoMode   undo_mode)
{
  LigmaUndo            *undo;
  LigmaUndoAccumulator  accum = { 0, };

  g_object_freeze_notify (G_OBJECT (image));

  undo = ligma_undo_stack_pop_undo (undo_stack, undo_mode, &accum);

  if (undo)
    {
      if (LIGMA_IS_UNDO_STACK (undo))
        ligma_list_reverse (LIGMA_LIST (LIGMA_UNDO_STACK (undo)->undos));

      ligma_undo_stack_push_undo (redo_stack, undo);

      if (accum.mode_changed)
        ligma_image_mode_changed (image);

      if (accum.precision_changed)
        ligma_image_precision_changed (image);

      if (accum.size_changed)
        ligma_image_size_changed_detailed (image,
                                          accum.previous_origin_x,
                                          accum.previous_origin_y,
                                          accum.previous_width,
                                          accum.previous_height);

      if (accum.resolution_changed)
        ligma_image_resolution_changed (image);

      if (accum.unit_changed)
        ligma_image_unit_changed (image);

      /* let others know that we just popped an action */
      ligma_image_undo_event (image,
                             (undo_mode == LIGMA_UNDO_MODE_UNDO) ?
                             LIGMA_UNDO_EVENT_UNDO : LIGMA_UNDO_EVENT_REDO,
                             undo);
    }

  g_object_thaw_notify (G_OBJECT (image));
}

static void
ligma_image_undo_free_space (LigmaImage *image)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);
  LigmaContainer    *container;
  gint              min_undo_levels;
  gint              max_undo_levels;
  gint64            undo_size;

  container = private->undo_stack->undos;

  min_undo_levels = image->ligma->config->levels_of_undo;
  max_undo_levels = 1024; /* FIXME */
  undo_size       = image->ligma->config->undo_size;

#ifdef DEBUG_IMAGE_UNDO
  g_printerr ("undo_steps: %d    undo_bytes: %ld\n",
              ligma_container_get_n_children (container),
              (glong) ligma_object_get_memsize (LIGMA_OBJECT (container), NULL));
#endif

  /*  keep at least min_undo_levels undo steps  */
  if (ligma_container_get_n_children (container) <= min_undo_levels)
    return;

  while ((ligma_object_get_memsize (LIGMA_OBJECT (container), NULL) > undo_size) ||
         (ligma_container_get_n_children (container) > max_undo_levels))
    {
      LigmaUndo *freed = ligma_undo_stack_free_bottom (private->undo_stack,
                                                     LIGMA_UNDO_MODE_UNDO);

#ifdef DEBUG_IMAGE_UNDO
      g_printerr ("freed one step: undo_steps: %d    undo_bytes: %ld\n",
                  ligma_container_get_n_children (container),
                  (glong) ligma_object_get_memsize (LIGMA_OBJECT (container),
                                                   NULL));
#endif

      ligma_image_undo_event (image, LIGMA_UNDO_EVENT_UNDO_EXPIRED, freed);

      g_object_unref (freed);

      if (ligma_container_get_n_children (container) <= min_undo_levels)
        return;
    }
}

static void
ligma_image_undo_free_redo (LigmaImage *image)
{
  LigmaImagePrivate *private   = LIGMA_IMAGE_GET_PRIVATE (image);
  LigmaContainer    *container = private->redo_stack->undos;

#ifdef DEBUG_IMAGE_UNDO
  g_printerr ("redo_steps: %d    redo_bytes: %ld\n",
              ligma_container_get_n_children (container),
              (glong) ligma_object_get_memsize (LIGMA_OBJECT (container), NULL));
#endif

  if (ligma_container_is_empty (container))
    return;

  while (ligma_container_get_n_children (container) > 0)
    {
      LigmaUndo *freed = ligma_undo_stack_free_bottom (private->redo_stack,
                                                     LIGMA_UNDO_MODE_REDO);

#ifdef DEBUG_IMAGE_UNDO
      g_printerr ("freed one step: redo_steps: %d    redo_bytes: %ld\n",
                  ligma_container_get_n_children (container),
                  (glong )ligma_object_get_memsize (LIGMA_OBJECT (container),
                                                   NULL));
#endif

      ligma_image_undo_event (image, LIGMA_UNDO_EVENT_REDO_EXPIRED, freed);

      g_object_unref (freed);
    }

  /* We need to use <= here because the undo counter has already been
   * incremented at this point.
   */
  if (private->dirty <= 0)
    {
      /* If the image was dirty, but could become clean by redo-ing
       * some actions, then it should now become 'infinitely' dirty.
       * This is because we've just nuked the actions that would allow
       * the image to become clean again.
       */
      private->dirty = 100000;
    }
}

static LigmaDirtyMask
ligma_image_undo_dirty_from_type (LigmaUndoType undo_type)
{
  switch (undo_type)
    {
    case LIGMA_UNDO_GROUP_IMAGE_SCALE:
    case LIGMA_UNDO_GROUP_IMAGE_RESIZE:
    case LIGMA_UNDO_GROUP_IMAGE_FLIP:
    case LIGMA_UNDO_GROUP_IMAGE_ROTATE:
    case LIGMA_UNDO_GROUP_IMAGE_TRANSFORM:
    case LIGMA_UNDO_GROUP_IMAGE_CROP:
      return LIGMA_DIRTY_IMAGE | LIGMA_DIRTY_IMAGE_SIZE;

    case LIGMA_UNDO_GROUP_IMAGE_CONVERT:
      return LIGMA_DIRTY_IMAGE | LIGMA_DIRTY_DRAWABLE;

    case LIGMA_UNDO_GROUP_IMAGE_LAYERS_MERGE:
      return LIGMA_DIRTY_IMAGE_STRUCTURE | LIGMA_DIRTY_DRAWABLE;

    case LIGMA_UNDO_GROUP_IMAGE_VECTORS_MERGE:
      return LIGMA_DIRTY_IMAGE_STRUCTURE | LIGMA_DIRTY_VECTORS;

    case LIGMA_UNDO_GROUP_IMAGE_QUICK_MASK: /* FIXME */
      return LIGMA_DIRTY_IMAGE_STRUCTURE | LIGMA_DIRTY_SELECTION;

    case LIGMA_UNDO_GROUP_IMAGE_GRID:
    case LIGMA_UNDO_GROUP_GUIDE:
      return LIGMA_DIRTY_IMAGE_META;

    case LIGMA_UNDO_GROUP_DRAWABLE:
    case LIGMA_UNDO_GROUP_DRAWABLE_MOD:
      return LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE;

    case LIGMA_UNDO_GROUP_MASK: /* FIXME */
      return LIGMA_DIRTY_SELECTION;

    case LIGMA_UNDO_GROUP_ITEM_VISIBILITY:
    case LIGMA_UNDO_GROUP_ITEM_PROPERTIES:
      return LIGMA_DIRTY_ITEM_META;

    case LIGMA_UNDO_GROUP_ITEM_DISPLACE: /* FIXME */
      return LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE | LIGMA_DIRTY_VECTORS;

    case LIGMA_UNDO_GROUP_ITEM_SCALE: /* FIXME */
    case LIGMA_UNDO_GROUP_ITEM_RESIZE: /* FIXME */
      return LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE | LIGMA_DIRTY_VECTORS;

    case LIGMA_UNDO_GROUP_LAYER_ADD_MASK:
    case LIGMA_UNDO_GROUP_LAYER_APPLY_MASK:
      return LIGMA_DIRTY_IMAGE_STRUCTURE;

    case LIGMA_UNDO_GROUP_FS_TO_LAYER:
    case LIGMA_UNDO_GROUP_FS_FLOAT:
    case LIGMA_UNDO_GROUP_FS_ANCHOR:
      return LIGMA_DIRTY_IMAGE_STRUCTURE;

    case LIGMA_UNDO_GROUP_EDIT_PASTE:
      return LIGMA_DIRTY_IMAGE_STRUCTURE;

    case LIGMA_UNDO_GROUP_EDIT_CUT:
      return LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE;

    case LIGMA_UNDO_GROUP_TEXT:
      return LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE;

    case LIGMA_UNDO_GROUP_TRANSFORM: /* FIXME */
      return LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE | LIGMA_DIRTY_VECTORS;

    case LIGMA_UNDO_GROUP_PAINT:
      return LIGMA_DIRTY_ITEM | LIGMA_DIRTY_DRAWABLE;

    case LIGMA_UNDO_GROUP_PARASITE_ATTACH:
    case LIGMA_UNDO_GROUP_PARASITE_REMOVE:
      return LIGMA_DIRTY_IMAGE_META | LIGMA_DIRTY_ITEM_META;

    case LIGMA_UNDO_GROUP_VECTORS_IMPORT:
      return LIGMA_DIRTY_IMAGE_STRUCTURE | LIGMA_DIRTY_VECTORS;

    case LIGMA_UNDO_GROUP_MISC:
      return LIGMA_DIRTY_ALL;

    default:
      break;
    }

  return LIGMA_DIRTY_ALL;
}
