/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <glib-object.h>

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimp-utils.h"
#include "gimpdrawableundo.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpitem.h"
#include "gimplist.h"
#include "gimpundostack.h"


/*  local function prototypes  */

static void          gimp_image_undo_pop_stack       (GimpImage     *image,
                                                      GimpUndoStack *undo_stack,
                                                      GimpUndoStack *redo_stack,
                                                      GimpUndoMode   undo_mode);
static void          gimp_image_undo_free_space      (GimpImage     *image);
static void          gimp_image_undo_free_redo       (GimpImage     *image);

static GimpDirtyMask gimp_image_undo_dirty_from_type (GimpUndoType   undo_type);


/*  public functions  */

gboolean
gimp_image_undo (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (image->pushing_undo_group == GIMP_UNDO_GROUP_NONE,
                        FALSE);

  gimp_image_undo_pop_stack (image,
                             image->undo_stack,
                             image->redo_stack,
                             GIMP_UNDO_MODE_UNDO);

  return TRUE;
}

/*
 * this function continues to undo as long as it only sees certain
 * undo types, in particular visibility changes.
 */
gboolean
gimp_image_strong_undo (GimpImage *image)
{
  GimpUndo *undo;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (image->pushing_undo_group == GIMP_UNDO_GROUP_NONE,
                        FALSE);

  undo = gimp_undo_stack_peek (image->undo_stack);

  gimp_image_undo (image);

  while (gimp_undo_is_weak (undo))
    {
      undo = gimp_undo_stack_peek (image->undo_stack);
      if (gimp_undo_is_weak (undo))
        gimp_image_undo (image);
    }

  return TRUE;
}

gboolean
gimp_image_redo (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (image->pushing_undo_group == GIMP_UNDO_GROUP_NONE,
                        FALSE);

  gimp_image_undo_pop_stack (image,
                             image->redo_stack,
                             image->undo_stack,
                             GIMP_UNDO_MODE_REDO);

  return TRUE;
}

/*
 * this function continues to redo as long as it only sees certain
 * undo types, in particular visibility changes.  Note that the
 * order of events is set up to make it exactly reverse
 * gimp_image_strong_undo().
 */
gboolean
gimp_image_strong_redo (GimpImage *image)
{
  GimpUndo *undo;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (image->pushing_undo_group == GIMP_UNDO_GROUP_NONE,
                        FALSE);

  undo = gimp_undo_stack_peek (image->redo_stack);

  gimp_image_redo (image);

  while (gimp_undo_is_weak (undo))
    {
      undo = gimp_undo_stack_peek (image->redo_stack);
      if (gimp_undo_is_weak (undo))
        gimp_image_redo (image);
    }

  return TRUE;
}

void
gimp_image_undo_free (GimpImage *image)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  /*  Emit the UNDO_FREE event before actually freeing everything
   *  so the views can properly detach from the undo items
   */
  gimp_image_undo_event (image, GIMP_UNDO_EVENT_UNDO_FREE, NULL);

  gimp_undo_free (GIMP_UNDO (image->undo_stack), GIMP_UNDO_MODE_UNDO);
  gimp_undo_free (GIMP_UNDO (image->redo_stack), GIMP_UNDO_MODE_REDO);

  /* If the image was dirty, but could become clean by redo-ing
   * some actions, then it should now become 'infinitely' dirty.
   * This is because we've just nuked the actions that would allow
   * the image to become clean again.
   */
  if (image->dirty < 0)
    image->dirty = 100000;

  /* The same applies to the case where the image would become clean
   * due to undo actions, but since user can't undo without an undo
   * stack, that's not so much a problem.
   */
}

gboolean
gimp_image_undo_group_start (GimpImage    *image,
                             GimpUndoType  undo_type,
                             const gchar  *name)
{
  GimpUndoStack *undo_group;
  GimpDirtyMask  dirty_mask;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (undo_type >  GIMP_UNDO_GROUP_FIRST &&
                        undo_type <= GIMP_UNDO_GROUP_LAST, FALSE);

  if (! name)
    name = gimp_undo_type_to_name (undo_type);

  dirty_mask = gimp_image_undo_dirty_from_type (undo_type);

  /* Notify listeners that the image will be modified */
  if (image->group_count == 0 && dirty_mask != GIMP_DIRTY_NONE)
    gimp_image_dirty (image, dirty_mask);

  if (image->undo_freeze_count > 0)
    return FALSE;

  image->group_count++;

  /*  If we're already in a group...ignore  */
  if (image->group_count > 1)
    return TRUE;

  /*  nuke the redo stack  */
  gimp_image_undo_free_redo (image);

  undo_group = gimp_undo_stack_new (image);

  gimp_object_set_name (GIMP_OBJECT (undo_group), name);
  GIMP_UNDO (undo_group)->undo_type  = undo_type;
  GIMP_UNDO (undo_group)->dirty_mask = dirty_mask;

  gimp_undo_stack_push_undo (image->undo_stack, GIMP_UNDO (undo_group));

  image->pushing_undo_group = undo_type;

  return TRUE;
}

gboolean
gimp_image_undo_group_end (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  if (image->undo_freeze_count > 0)
    return FALSE;

  g_return_val_if_fail (image->group_count > 0, FALSE);

  image->group_count--;

  if (image->group_count == 0)
    {
      image->pushing_undo_group = GIMP_UNDO_GROUP_NONE;

      /* Do it here, since undo_push doesn't emit this event while in
       * the middle of a group
       */
      gimp_image_undo_event (image, GIMP_UNDO_EVENT_UNDO_PUSHED,
                             gimp_undo_stack_peek (image->undo_stack));

      gimp_image_undo_free_space (image);
    }

  return TRUE;
}

GimpUndo *
gimp_image_undo_push (GimpImage     *image,
                      GType          object_type,
                      GimpUndoType   undo_type,
                      const gchar   *name,
                      GimpDirtyMask  dirty_mask,
                      ...)
{
  GParameter *params   = NULL;
  gint        n_params = 0;
  va_list     args;
  GimpUndo   *undo;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (g_type_is_a (object_type, GIMP_TYPE_UNDO), NULL);
  g_return_val_if_fail (undo_type > GIMP_UNDO_GROUP_LAST, NULL);

  /* Does this undo dirty the image?  If so, we always want to mark
   * image dirty, even if we can't actually push the undo.
   */
  if (dirty_mask != GIMP_DIRTY_NONE)
    gimp_image_dirty (image, dirty_mask);

  if (image->undo_freeze_count > 0)
    return NULL;

  if (! name)
    name = gimp_undo_type_to_name (undo_type);

  params = gimp_parameters_append (object_type, params, &n_params,
                                   "name",       name,
                                   "image",      image,
                                   "undo-type",  undo_type,
                                   "dirty-mask", dirty_mask,
                                   NULL);

  va_start (args, dirty_mask);
  params = gimp_parameters_append_valist (object_type, params, &n_params, args);
  va_end (args);

  undo = g_object_newv (object_type, n_params, params);

  gimp_parameters_free (params, n_params);

  /*  nuke the redo stack  */
  gimp_image_undo_free_redo (image);

  if (image->pushing_undo_group == GIMP_UNDO_GROUP_NONE)
    {
      gimp_undo_stack_push_undo (image->undo_stack, undo);

      gimp_image_undo_event (image, GIMP_UNDO_EVENT_UNDO_PUSHED, undo);

      gimp_image_undo_free_space (image);

      /*  freeing undo space may have freed the newly pushed undo  */
      if (gimp_undo_stack_peek (image->undo_stack) == undo)
        return undo;
    }
  else
    {
      GimpUndoStack *undo_group;

      undo_group = GIMP_UNDO_STACK (gimp_undo_stack_peek (image->undo_stack));

      gimp_undo_stack_push_undo (undo_group, undo);

      return undo;
    }

  return NULL;
}

GimpUndo *
gimp_image_undo_can_compress (GimpImage    *image,
                              GType         object_type,
                              GimpUndoType  undo_type)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  if (image->dirty != 0 && ! gimp_undo_stack_peek (image->redo_stack))
    {
      GimpUndo *undo = gimp_undo_stack_peek (image->undo_stack);

      if (undo && undo->undo_type == undo_type &&
          g_type_is_a (G_TYPE_FROM_INSTANCE (undo), object_type))
        {
          return undo;
        }
    }

  return NULL;
}

GimpUndo *
gimp_image_undo_get_fadeable (GimpImage *image)
{
  GimpUndo *undo;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  undo = gimp_undo_stack_peek (image->undo_stack);

  if (GIMP_IS_UNDO_STACK (undo) && undo->undo_type == GIMP_UNDO_GROUP_PAINT)
    {
      GimpUndoStack *stack = GIMP_UNDO_STACK (undo);

      if (gimp_undo_stack_get_depth (stack) == 2)
        {
          undo = gimp_undo_stack_peek (stack);
        }
    }

  if (GIMP_IS_DRAWABLE_UNDO (undo))
    return undo;

  return NULL;
}


/*  private functions  */

static void
gimp_image_undo_pop_stack (GimpImage     *image,
                           GimpUndoStack *undo_stack,
                           GimpUndoStack *redo_stack,
                           GimpUndoMode   undo_mode)
{
  GimpUndo            *undo;
  GimpUndoAccumulator  accum = { 0, };

  g_object_freeze_notify (G_OBJECT (image));

  undo = gimp_undo_stack_pop_undo (undo_stack, undo_mode, &accum);

  if (undo)
    {
      if (GIMP_IS_UNDO_STACK (undo))
        gimp_list_reverse (GIMP_LIST (GIMP_UNDO_STACK (undo)->undos));

      gimp_undo_stack_push_undo (redo_stack, undo);

      if (accum.mode_changed)
        gimp_image_mode_changed (image);

      if (accum.size_changed)
        gimp_viewable_size_changed (GIMP_VIEWABLE (image));

      if (accum.resolution_changed)
        gimp_image_resolution_changed (image);

      if (accum.unit_changed)
        gimp_image_unit_changed (image);

      if (accum.quick_mask_changed)
        gimp_image_quick_mask_changed (image);

      if (accum.alpha_changed)
        gimp_image_alpha_changed (image);

      /* let others know that we just popped an action */
      gimp_image_undo_event (image,
                             (undo_mode == GIMP_UNDO_MODE_UNDO) ?
                             GIMP_UNDO_EVENT_UNDO : GIMP_UNDO_EVENT_REDO,
                             undo);
    }

  g_object_thaw_notify (G_OBJECT (image));
}

static void
gimp_image_undo_free_space (GimpImage *image)
{
  GimpContainer *container;
  gint           min_undo_levels;
  gint           max_undo_levels;
  gint64         undo_size;

  container = image->undo_stack->undos;

  min_undo_levels = image->gimp->config->levels_of_undo;
  max_undo_levels = 1024; /* FIXME */
  undo_size       = image->gimp->config->undo_size;

#ifdef DEBUG_IMAGE_UNDO
  g_printerr ("undo_steps: %d    undo_bytes: %ld\n",
              gimp_container_num_children (container),
              (glong) gimp_object_get_memsize (GIMP_OBJECT (container), NULL));
#endif

  /*  keep at least min_undo_levels undo steps  */
  if (gimp_container_num_children (container) <= min_undo_levels)
    return;

  while ((gimp_object_get_memsize (GIMP_OBJECT (container), NULL) > undo_size) ||
         (gimp_container_num_children (container) > max_undo_levels))
    {
      GimpUndo *freed = gimp_undo_stack_free_bottom (image->undo_stack,
                                                     GIMP_UNDO_MODE_UNDO);

#ifdef DEBUG_IMAGE_UNDO
      g_printerr ("freed one step: undo_steps: %d    undo_bytes: %ld\n",
                  gimp_container_num_children (container),
                  (glong) gimp_object_get_memsize (GIMP_OBJECT (container),
                                                   NULL));
#endif

      gimp_image_undo_event (image, GIMP_UNDO_EVENT_UNDO_EXPIRED, freed);

      g_object_unref (freed);

      if (gimp_container_num_children (container) <= min_undo_levels)
        return;
    }
}

static void
gimp_image_undo_free_redo (GimpImage *image)
{
  GimpContainer *container = image->redo_stack->undos;

#ifdef DEBUG_IMAGE_UNDO
  g_printerr ("redo_steps: %d    redo_bytes: %ld\n",
              gimp_container_num_children (container),
              (glong) gimp_object_get_memsize (GIMP_OBJECT (container), NULL));
#endif

  if (gimp_container_is_empty (container))
    return;

  while (gimp_container_num_children (container) > 0)
    {
      GimpUndo *freed = gimp_undo_stack_free_bottom (image->redo_stack,
                                                     GIMP_UNDO_MODE_REDO);

#ifdef DEBUG_IMAGE_UNDO
      g_printerr ("freed one step: redo_steps: %d    redo_bytes: %ld\n",
                  gimp_container_num_children (container),
                  (glong )gimp_object_get_memsize (GIMP_OBJECT (container),
                                                   NULL));
#endif

      gimp_image_undo_event (image, GIMP_UNDO_EVENT_REDO_EXPIRED, freed);

      g_object_unref (freed);
    }

  /* We need to use <= here because the undo counter has already been
   * incremented at this point.
   */
  if (image->dirty <= 0)
    {
      /* If the image was dirty, but could become clean by redo-ing
       * some actions, then it should now become 'infinitely' dirty.
       * This is because we've just nuked the actions that would allow
       * the image to become clean again.
       */
      image->dirty = 100000;
    }
}

static GimpDirtyMask
gimp_image_undo_dirty_from_type (GimpUndoType undo_type)
{
  switch (undo_type)
    {
    case GIMP_UNDO_GROUP_IMAGE_SCALE:
    case GIMP_UNDO_GROUP_IMAGE_RESIZE:
    case GIMP_UNDO_GROUP_IMAGE_FLIP:
    case GIMP_UNDO_GROUP_IMAGE_ROTATE:
    case GIMP_UNDO_GROUP_IMAGE_CROP:
      return GIMP_DIRTY_IMAGE | GIMP_DIRTY_IMAGE_SIZE;

    case GIMP_UNDO_GROUP_IMAGE_CONVERT:
      return GIMP_DIRTY_IMAGE | GIMP_DIRTY_DRAWABLE;

    case GIMP_UNDO_GROUP_IMAGE_LAYERS_MERGE:
      return GIMP_DIRTY_IMAGE_STRUCTURE | GIMP_DIRTY_DRAWABLE;

    case GIMP_UNDO_GROUP_IMAGE_VECTORS_MERGE:
      return GIMP_DIRTY_IMAGE_STRUCTURE | GIMP_DIRTY_VECTORS;

    case GIMP_UNDO_GROUP_IMAGE_QUICK_MASK: /* FIXME */
      return GIMP_DIRTY_IMAGE_STRUCTURE | GIMP_DIRTY_SELECTION;

    case GIMP_UNDO_GROUP_IMAGE_GRID:
    case GIMP_UNDO_GROUP_GUIDE:
      return GIMP_DIRTY_IMAGE_META;

    case GIMP_UNDO_GROUP_DRAWABLE:
    case GIMP_UNDO_GROUP_DRAWABLE_MOD:
      return GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE;

    case GIMP_UNDO_GROUP_MASK: /* FIXME */
      return GIMP_DIRTY_SELECTION;

    case GIMP_UNDO_GROUP_ITEM_VISIBILITY:
    case GIMP_UNDO_GROUP_ITEM_LINKED:
    case GIMP_UNDO_GROUP_ITEM_PROPERTIES:
      return GIMP_DIRTY_ITEM_META;

    case GIMP_UNDO_GROUP_ITEM_DISPLACE: /* FIXME */
      return GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE | GIMP_DIRTY_VECTORS;

    case GIMP_UNDO_GROUP_ITEM_SCALE: /* FIXME */
    case GIMP_UNDO_GROUP_ITEM_RESIZE: /* FIXME */
      return GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE | GIMP_DIRTY_VECTORS;

    case GIMP_UNDO_GROUP_LAYER_ADD_MASK:
    case GIMP_UNDO_GROUP_LAYER_APPLY_MASK:
      return GIMP_DIRTY_IMAGE_STRUCTURE;

    case GIMP_UNDO_GROUP_FS_TO_LAYER:
    case GIMP_UNDO_GROUP_FS_FLOAT:
    case GIMP_UNDO_GROUP_FS_ANCHOR:
    case GIMP_UNDO_GROUP_FS_REMOVE:
      return GIMP_DIRTY_IMAGE_STRUCTURE;

    case GIMP_UNDO_GROUP_EDIT_PASTE:
      return GIMP_DIRTY_IMAGE_STRUCTURE;

    case GIMP_UNDO_GROUP_EDIT_CUT:
      return GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE;

    case GIMP_UNDO_GROUP_TEXT:
      return GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE;

    case GIMP_UNDO_GROUP_TRANSFORM: /* FIXME */
      return GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE | GIMP_DIRTY_VECTORS;

    case GIMP_UNDO_GROUP_PAINT:
      return GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE;

    case GIMP_UNDO_GROUP_PARASITE_ATTACH:
    case GIMP_UNDO_GROUP_PARASITE_REMOVE:
      return GIMP_DIRTY_IMAGE_META | GIMP_DIRTY_ITEM_META;

    case GIMP_UNDO_GROUP_VECTORS_IMPORT:
      return GIMP_DIRTY_IMAGE_STRUCTURE | GIMP_DIRTY_VECTORS;

    case GIMP_UNDO_GROUP_MISC:
      return GIMP_DIRTY_ALL;

    default:
      break;
    }

  return GIMP_DIRTY_ALL;
}
