/* The GIMP -- an image manipulation program
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
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpitem.h"
#include "gimpitemundo.h"
#include "gimplist.h"
#include "gimpundostack.h"


/*  local function prototypes  */

static void    gimp_image_undo_pop_stack    (GimpImage     *gimage,
                                             GimpUndoStack *undo_stack,
                                             GimpUndoStack *redo_stack,
                                             GimpUndoMode   undo_mode);
static void    gimp_image_undo_free_space   (GimpImage     *gimage);
static void    gimp_image_undo_free_redo    (GimpImage     *gimage);


/*  public functions  */

gboolean
gimp_image_undo (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (gimage->pushing_undo_group == GIMP_UNDO_GROUP_NONE,
                        FALSE);

  gimp_image_undo_pop_stack (gimage,
                             gimage->undo_stack,
                             gimage->redo_stack,
                             GIMP_UNDO_MODE_UNDO);

  return TRUE;
}

gboolean
gimp_image_redo (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (gimage->pushing_undo_group == GIMP_UNDO_GROUP_NONE,
                        FALSE);

  gimp_image_undo_pop_stack (gimage,
                             gimage->redo_stack,
                             gimage->undo_stack,
                             GIMP_UNDO_MODE_REDO);

  return TRUE;
}

void
gimp_image_undo_free (GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  Emit the UNDO_FREE event before actually freeing everything
   *  so the views can properly detach from the undo items
   */
  gimp_image_undo_event (gimage, GIMP_UNDO_EVENT_UNDO_FREE, NULL);

  gimp_undo_free (GIMP_UNDO (gimage->undo_stack), GIMP_UNDO_MODE_UNDO);
  gimp_undo_free (GIMP_UNDO (gimage->redo_stack), GIMP_UNDO_MODE_REDO);

  /* If the image was dirty, but could become clean by redo-ing
   * some actions, then it should now become 'infinitely' dirty.
   * This is because we've just nuked the actions that would allow
   * the image to become clean again.  The only hope for salvation
   * is to save the image now!  -- austin
   */
  if (gimage->dirty < 0)
    gimage->dirty = 10000;

  /* The same applies to the case where the image would become clean
   * due to undo actions, but since user can't undo without an undo
   * stack, that's not so much a problem.
   */
}

gboolean
gimp_image_undo_group_start (GimpImage    *gimage,
                             GimpUndoType  type,
                             const gchar  *name)
{
  GimpUndoStack *undo_group;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (type >  GIMP_UNDO_GROUP_FIRST &&
                        type <= GIMP_UNDO_GROUP_LAST, FALSE);

  if (! name)
    name = gimp_undo_type_to_name (type);

  /* Notify listeners that the image will be modified */
  gimp_image_undo_start (gimage);

  if (gimage->undo_freeze_count > 0)
    return FALSE;

  gimage->group_count++;

  /*  If we're already in a group...ignore  */
  if (gimage->group_count > 1)
    return TRUE;

  /*  nuke the redo stack  */
  gimp_image_undo_free_redo (gimage);

  /* If the image was dirty, but could become clean by redo-ing
   * some actions, then it should now become 'infinitely' dirty.
   * This is because we've just nuked the actions that would allow
   * the image to become clean again.  The only hope for salvation
   * is to save the image now!  -- austin
   */
  if (gimage->dirty < 0)
    gimage->dirty = 10000;

  undo_group = gimp_undo_stack_new (gimage);

  gimp_object_set_name (GIMP_OBJECT (undo_group), name);
  GIMP_UNDO (undo_group)->undo_type = type;

  gimp_undo_stack_push_undo (gimage->undo_stack,
                             GIMP_UNDO (undo_group));

  gimage->pushing_undo_group = type;

  return TRUE;
}

gboolean
gimp_image_undo_group_end (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);

  if (gimage->undo_freeze_count > 0)
    return FALSE;

  g_return_val_if_fail (gimage->group_count > 0, FALSE);

  gimage->group_count--;

  if (gimage->group_count == 0)
    {
      gimage->pushing_undo_group = GIMP_UNDO_GROUP_NONE;

      gimp_image_undo_free_space (gimage);

      /* Do it here, since undo_push doesn't emit this event while in
       * the middle of a group
       */
      gimp_image_undo_event (gimage, GIMP_UNDO_EVENT_UNDO_PUSHED,
                             gimp_undo_stack_peek (gimage->undo_stack));
    }

  return TRUE;
}

GimpUndo *
gimp_image_undo_push (GimpImage        *gimage,
                      gint64            size,
                      gsize             struct_size,
                      GimpUndoType      type,
                      const gchar      *name,
                      gboolean          dirties_image,
                      GimpUndoPopFunc   pop_func,
                      GimpUndoFreeFunc  free_func)
{
  return gimp_image_undo_push_item (gimage, NULL,
                                    size, struct_size,
                                    type, name, dirties_image,
                                    pop_func, free_func);
}

GimpUndo *
gimp_image_undo_push_item (GimpImage        *gimage,
                           GimpItem         *item,
                           gint64            size,
                           gsize             struct_size,
                           GimpUndoType      type,
                           const gchar      *name,
                           gboolean          dirties_image,
                           GimpUndoPopFunc   pop_func,
                           GimpUndoFreeFunc  free_func)
{
  GimpUndo *undo;
  gpointer  undo_struct = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (item == NULL || GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (type > GIMP_UNDO_GROUP_LAST, NULL);

  /* Does this undo dirty the image?  If so, we always want to mark
   * image dirty, even if we can't actually push the undo.
   */
  if (dirties_image)
    gimp_image_dirty (gimage);

  if (gimage->undo_freeze_count > 0)
    return NULL;

  if (struct_size > 0)
    undo_struct = g_malloc0 (struct_size);

  if (item)
    {
      undo = gimp_item_undo_new (gimage, item,
                                 type, name,
                                 undo_struct, size,
                                 dirties_image,
                                 pop_func, free_func);
    }
  else
    {
      undo = gimp_undo_new (gimage,
                            type, name,
                            undo_struct, size,
                            dirties_image,
                            pop_func, free_func);
    }

  gimp_image_undo_push_undo (gimage, undo);

  return undo;
}

void
gimp_image_undo_push_undo (GimpImage *gimage,
                           GimpUndo  *undo)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_UNDO (undo));
  g_return_if_fail (undo->gimage == gimage);
  g_return_if_fail (gimage->undo_freeze_count == 0);

  /*  nuke the redo stack  */
  gimp_image_undo_free_redo (gimage);

  /* If the image was dirty, but could become clean by redo-ing
   * some actions, then it should now become 'infinitely' dirty.
   * This is because we've just nuked the actions that would allow
   * the image to become clean again.  The only hope for salvation
   * is to save the image now!  -- austin
   */
  if (gimage->dirty < 0)
    gimage->dirty = 10000;

  if (gimage->pushing_undo_group == GIMP_UNDO_GROUP_NONE)
    {
      gimp_undo_stack_push_undo (gimage->undo_stack, undo);

      gimp_image_undo_free_space (gimage);

      gimp_image_undo_event (gimage, GIMP_UNDO_EVENT_UNDO_PUSHED, undo);
    }
  else
    {
      GimpUndoStack *undo_group;

      undo_group = GIMP_UNDO_STACK (gimp_undo_stack_peek (gimage->undo_stack));

      gimp_undo_stack_push_undo (undo_group, undo);
    }
}


/*  private functions  */

static void
gimp_image_undo_pop_stack (GimpImage     *gimage,
                           GimpUndoStack *undo_stack,
                           GimpUndoStack *redo_stack,
                           GimpUndoMode   undo_mode)
{
  GimpUndo            *undo;
  GimpUndoAccumulator  accum = { 0, };

  undo = gimp_undo_stack_pop_undo (undo_stack, undo_mode, &accum);

  if (! undo)
    return;

  if (GIMP_IS_UNDO_STACK (undo))
    gimp_list_reverse (GIMP_LIST (GIMP_UNDO_STACK (undo)->undos));

  gimp_undo_stack_push_undo (redo_stack, undo);

  if (accum.mode_changed)
    gimp_image_mode_changed (gimage);

  if (accum.size_changed)
    gimp_viewable_size_changed (GIMP_VIEWABLE (gimage));

  if (accum.resolution_changed)
    gimp_image_resolution_changed (gimage);

  if (accum.unit_changed)
    gimp_image_unit_changed (gimage);

  if (accum.qmask_changed)
    gimp_image_qmask_changed (gimage);

  if (accum.alpha_changed)
    gimp_image_alpha_changed (gimage);

  /* let others know that we just popped an action */
  gimp_image_undo_event (gimage,
                         (undo_mode == GIMP_UNDO_MODE_UNDO) ?
                         GIMP_UNDO_EVENT_UNDO : GIMP_UNDO_EVENT_REDO,
                         undo);
}

static void
gimp_image_undo_free_space (GimpImage *gimage)
{
  GimpContainer *container;
  gint           min_undo_levels;
  gint           max_undo_levels;
  gint64         undo_size;

  container = gimage->undo_stack->undos;

  min_undo_levels = gimage->gimp->config->levels_of_undo;
  max_undo_levels = 1024; /* FIXME */
  undo_size       = gimage->gimp->config->undo_size;

#if 0
  g_print ("undo_steps: %d    undo_bytes: %d\n",
           gimp_container_num_children (container),
           gimp_object_get_memsize (GIMP_OBJECT (container), NULL));
#endif

  /*  keep at least min_undo_levels undo steps  */
  if (gimp_container_num_children (container) <= min_undo_levels)
    return;

  while ((gimp_object_get_memsize (GIMP_OBJECT (container), NULL) > undo_size) ||
         (gimp_container_num_children (container) > max_undo_levels))
    {
      GimpUndo *freed;

      freed = gimp_undo_stack_free_bottom (gimage->undo_stack,
                                           GIMP_UNDO_MODE_UNDO);

#if 0
      g_print ("freed one step: undo_steps: %d    undo_bytes: %d\n",
               gimp_container_num_children (container),
               gimp_object_get_memsize (GIMP_OBJECT (container), NULL));
#endif

      gimp_image_undo_event (gimage, GIMP_UNDO_EVENT_UNDO_EXPIRED, freed);

      g_object_unref (freed);

      if (gimp_container_num_children (container) <= min_undo_levels)
        return;
    }
}

static void
gimp_image_undo_free_redo (GimpImage *gimage)
{
  GimpContainer *container;

  container = gimage->redo_stack->undos;

#if 0
  g_print ("redo_steps: %d    redo_bytes: %d\n",
           gimp_container_num_children (container),
           gimp_object_get_memsize (GIMP_OBJECT (container)), NULL);
#endif

  while (gimp_container_num_children (container) > 0)
    {
      GimpUndo *freed;

      freed = gimp_undo_stack_free_bottom (gimage->redo_stack,
                                           GIMP_UNDO_MODE_REDO);

#if 0
      g_print ("freed one step: redo_steps: %d    redo_bytes: %d\n",
               gimp_container_num_children (container),
               gimp_object_get_memsize (GIMP_OBJECT (container)), NULL);
#endif

      gimp_image_undo_event (gimage, GIMP_UNDO_EVENT_REDO_EXPIRED, freed);

      g_object_unref (freed);
    }
}
