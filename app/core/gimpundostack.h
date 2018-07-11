/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_UNDO_STACK_H__
#define __GIMP_UNDO_STACK_H__


#include "gimpundo.h"


#define GIMP_TYPE_UNDO_STACK            (gimp_undo_stack_get_type ())
#define GIMP_UNDO_STACK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UNDO_STACK, GimpUndoStack))
#define GIMP_UNDO_STACK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNDO_STACK, GimpUndoStackClass))
#define GIMP_IS_UNDO_STACK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_UNDO_STACK))
#define GIMP_IS_UNDO_STACK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNDO_STACK))
#define GIMP_UNDO_STACK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_UNDO_STACK, GimpUndoStackClass))


typedef struct _GimpUndoStackClass GimpUndoStackClass;

struct _GimpUndoStack
{
  GimpUndo       parent_instance;

  GimpContainer *undos;
};

struct _GimpUndoStackClass
{
  GimpUndoClass  parent_class;
};


GType           gimp_undo_stack_get_type    (void) G_GNUC_CONST;

GimpUndoStack * gimp_undo_stack_new         (GimpImage           *image);

void            gimp_undo_stack_push_undo   (GimpUndoStack       *stack,
                                             GimpUndo            *undo);
GimpUndo      * gimp_undo_stack_pop_undo    (GimpUndoStack       *stack,
                                             GimpUndoMode         undo_mode,
                                             GimpUndoAccumulator *accum);

GimpUndo      * gimp_undo_stack_free_bottom (GimpUndoStack       *stack,
                                             GimpUndoMode         undo_mode);
GimpUndo      * gimp_undo_stack_peek        (GimpUndoStack       *stack);
gint            gimp_undo_stack_get_depth   (GimpUndoStack       *stack);


#endif /* __GIMP_UNDO_STACK_H__ */
