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
#include "gimplist.h"
#include "gimpundostack.h"


/*  local function prototypes  */

static void          gimp_image_undo_pop_stack    (GimpImage     *gimage,
                                                   GimpUndoStack *undo_stack,
                                                   GimpUndoStack *redo_stack,
                                                   GimpUndoMode   undo_mode);
static void          gimp_image_undo_free_space   (GimpImage     *gimage);
static const gchar * gimp_image_undo_type_to_name (GimpUndoType   type);


/*  public functions  */

gboolean
gimp_image_undo (GimpImage *gimage)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (gimage->pushing_undo_group == NO_UNDO_GROUP, FALSE);

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
  g_return_val_if_fail (gimage->pushing_undo_group == NO_UNDO_GROUP, FALSE);

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

  gimp_undo_free (GIMP_UNDO (gimage->undo_stack), gimage, GIMP_UNDO_MODE_UNDO);
  gimp_undo_free (GIMP_UNDO (gimage->redo_stack), gimage, GIMP_UNDO_MODE_REDO);

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
  gimp_image_undo_event (gimage, UNDO_FREE);
}

gboolean
gimp_image_undo_group_start (GimpImage    *gimage,
                             GimpUndoType  type,
                             const gchar  *name)
{
  GimpUndoStack *undo_group;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), FALSE);
  g_return_val_if_fail (type >  FIRST_UNDO_GROUP &&
                        type <= LAST_UNDO_GROUP, FALSE);

  if (! name)
    name = gimp_image_undo_type_to_name (type);

  /* Notify listeners that the image will be modified */
  gimp_image_undo_start (gimage);

  if (! gimage->undo_on)
    return FALSE;

  gimage->group_count++;

  /*  If we're already in a group...ignore  */
  if (gimage->group_count > 1)
    return TRUE;

  /*  nuke the redo stack  */
  gimp_undo_free (GIMP_UNDO (gimage->redo_stack), gimage,
                  GIMP_UNDO_MODE_REDO);

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
  g_return_val_if_fail (gimage->group_count > 0, FALSE);

  if (! gimage->undo_on)
    return FALSE;

  gimage->group_count--;

  if (gimage->group_count == 0)
    {
      gimage->pushing_undo_group = NO_UNDO_GROUP;

      gimp_image_undo_free_space (gimage);

      /* Do it here, since undo_push doesn't emit this event while in
       * the middle of a group
       */
      gimp_image_undo_event (gimage, UNDO_PUSHED);
    }

  return TRUE;
}

GimpUndo *
gimp_image_undo_push (GimpImage        *gimage,
                      gsize             size,
                      gsize             struct_size,
                      GimpUndoType      type,
                      const gchar      *name,
                      gboolean          dirties_image,
                      GimpUndoPopFunc   pop_func,
                      GimpUndoFreeFunc  free_func)
{
  GimpUndo *new;
  gpointer  undo_struct = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (type > LAST_UNDO_GROUP, NULL);

  if (! name)
    name = gimp_image_undo_type_to_name (type);

  /* Does this undo dirty the image?  If so, we always want to mark
   * image dirty, even if we can't actually push the undo.
   */
  if (dirties_image)
    gimp_image_dirty (gimage);

  if (! gimage->undo_on)
    return NULL;

  /*  nuke the redo stack  */
  gimp_undo_free (GIMP_UNDO (gimage->redo_stack), gimage,
                  GIMP_UNDO_MODE_REDO);

  /* If the image was dirty, but could become clean by redo-ing
   * some actions, then it should now become 'infinitely' dirty.
   * This is because we've just nuked the actions that would allow
   * the image to become clean again.  The only hope for salvation
   * is to save the image now!  -- austin
   */
  if (gimage->dirty < 0)
    gimage->dirty = 10000;

  if (struct_size > 0)
    undo_struct = g_malloc0 (struct_size);

  new = gimp_undo_new (type,
                       name,
                       undo_struct, size,
                       dirties_image,
                       pop_func, free_func);

  if (gimage->pushing_undo_group == NO_UNDO_GROUP)
    {
      gimp_undo_stack_push_undo (gimage->undo_stack, new);

      gimp_image_undo_free_space (gimage);

      gimp_image_undo_event (gimage, UNDO_PUSHED);
    }
  else
    {
      GimpUndoStack *undo_group;

      undo_group = GIMP_UNDO_STACK (gimp_undo_stack_peek (gimage->undo_stack));

      gimp_undo_stack_push_undo (undo_group, new);
    }

  return new;
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

  if (accum.mask_changed)
    gimp_image_mask_changed (gimage);

  if (accum.qmask_changed)
    gimp_image_qmask_changed (gimage);

  if (accum.alpha_changed)
    gimp_image_alpha_changed (gimage);

  /* let others know that we just popped an action */
  gimp_image_undo_event (gimage,
                         (undo_mode == GIMP_UNDO_MODE_UNDO) ?
                         UNDO_POPPED : UNDO_REDO);
}

static void
gimp_image_undo_free_space (GimpImage *gimage)
{
  GimpContainer *container;
  gint           min_undo_levels;
  gint           max_undo_levels;
  gulong         undo_size;

  container = gimage->undo_stack->undos;

  min_undo_levels = gimage->gimp->config->levels_of_undo;
  max_undo_levels = 1024; /* FIXME */
  undo_size       = gimage->gimp->config->undo_size;

  /*  keep at least undo_levels undo steps  */
  if (gimp_container_num_children (container) <= min_undo_levels)
    return;

  while ((gimp_object_get_memsize (GIMP_OBJECT (container)) > undo_size) ||
         (gimp_container_num_children (container) > max_undo_levels))
    {
      gimp_undo_stack_free_bottom (gimage->undo_stack, GIMP_UNDO_MODE_UNDO);

      gimp_image_undo_event (gimage, UNDO_EXPIRED);

      if (gimp_container_num_children (container) <= min_undo_levels)
        return;
    }
}

static const gchar *
gimp_image_undo_type_to_name (GimpUndoType type)
{
  static GEnumClass *enum_class = NULL;
  GEnumValue        *value;

  if (! enum_class)
    enum_class = g_type_class_ref (GIMP_TYPE_UNDO_TYPE);

  value = g_enum_get_value (enum_class, type);

  if (value)
    return value->value_name;

  return "";
}
