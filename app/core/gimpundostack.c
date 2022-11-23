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

#include "ligmaimage.h"
#include "ligmalist.h"
#include "ligmaundo.h"
#include "ligmaundostack.h"


static void    ligma_undo_stack_finalize    (GObject             *object);

static gint64  ligma_undo_stack_get_memsize (LigmaObject          *object,
                                            gint64              *gui_size);

static void    ligma_undo_stack_pop         (LigmaUndo            *undo,
                                            LigmaUndoMode         undo_mode,
                                            LigmaUndoAccumulator *accum);
static void    ligma_undo_stack_free        (LigmaUndo            *undo,
                                            LigmaUndoMode         undo_mode);


G_DEFINE_TYPE (LigmaUndoStack, ligma_undo_stack, LIGMA_TYPE_UNDO)

#define parent_class ligma_undo_stack_parent_class


static void
ligma_undo_stack_class_init (LigmaUndoStackClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaUndoClass   *undo_class        = LIGMA_UNDO_CLASS (klass);

  object_class->finalize         = ligma_undo_stack_finalize;

  ligma_object_class->get_memsize = ligma_undo_stack_get_memsize;

  undo_class->pop                = ligma_undo_stack_pop;
  undo_class->free               = ligma_undo_stack_free;
}

static void
ligma_undo_stack_init (LigmaUndoStack *stack)
{
  stack->undos = ligma_list_new (LIGMA_TYPE_UNDO, FALSE);
}

static void
ligma_undo_stack_finalize (GObject *object)
{
  LigmaUndoStack *stack = LIGMA_UNDO_STACK (object);

  g_clear_object (&stack->undos);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_undo_stack_get_memsize (LigmaObject *object,
                             gint64     *gui_size)
{
  LigmaUndoStack *stack   = LIGMA_UNDO_STACK (object);
  gint64         memsize = 0;

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (stack->undos), gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_undo_stack_pop (LigmaUndo            *undo,
                     LigmaUndoMode         undo_mode,
                     LigmaUndoAccumulator *accum)
{
  LigmaUndoStack *stack = LIGMA_UNDO_STACK (undo);
  GList         *list;

  for (list = LIGMA_LIST (stack->undos)->queue->head;
       list;
       list = g_list_next (list))
    {
      LigmaUndo *child = list->data;

      ligma_undo_pop (child, undo_mode, accum);
    }
}

static void
ligma_undo_stack_free (LigmaUndo     *undo,
                      LigmaUndoMode  undo_mode)
{
  LigmaUndoStack *stack = LIGMA_UNDO_STACK (undo);
  GList         *list;

  for (list = LIGMA_LIST (stack->undos)->queue->head;
       list;
       list = g_list_next (list))
    {
      LigmaUndo *child = list->data;

      ligma_undo_free (child, undo_mode);
      g_object_unref (child);
    }

  ligma_container_clear (stack->undos);
}

LigmaUndoStack *
ligma_undo_stack_new (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return g_object_new (LIGMA_TYPE_UNDO_STACK,
                       "image", image,
                       NULL);
}

void
ligma_undo_stack_push_undo (LigmaUndoStack *stack,
                           LigmaUndo      *undo)
{
  g_return_if_fail (LIGMA_IS_UNDO_STACK (stack));
  g_return_if_fail (LIGMA_IS_UNDO (undo));

  ligma_container_add (stack->undos, LIGMA_OBJECT (undo));
}

LigmaUndo *
ligma_undo_stack_pop_undo (LigmaUndoStack       *stack,
                          LigmaUndoMode         undo_mode,
                          LigmaUndoAccumulator *accum)
{
  LigmaUndo *undo;

  g_return_val_if_fail (LIGMA_IS_UNDO_STACK (stack), NULL);
  g_return_val_if_fail (accum != NULL, NULL);

  undo = LIGMA_UNDO (ligma_container_get_first_child (stack->undos));

  if (undo)
    {
      ligma_container_remove (stack->undos, LIGMA_OBJECT (undo));
      ligma_undo_pop (undo, undo_mode, accum);

      return undo;
    }

  return NULL;
}

LigmaUndo *
ligma_undo_stack_free_bottom (LigmaUndoStack *stack,
                             LigmaUndoMode   undo_mode)
{
  LigmaUndo *undo;

  g_return_val_if_fail (LIGMA_IS_UNDO_STACK (stack), NULL);

  undo = LIGMA_UNDO (ligma_container_get_last_child (stack->undos));

  if (undo)
    {
      ligma_container_remove (stack->undos, LIGMA_OBJECT (undo));
      ligma_undo_free (undo, undo_mode);

      return undo;
    }

  return NULL;
}

LigmaUndo *
ligma_undo_stack_peek (LigmaUndoStack *stack)
{
  g_return_val_if_fail (LIGMA_IS_UNDO_STACK (stack), NULL);

  return LIGMA_UNDO (ligma_container_get_first_child (stack->undos));
}

gint
ligma_undo_stack_get_depth (LigmaUndoStack *stack)
{
  g_return_val_if_fail (LIGMA_IS_UNDO_STACK (stack), 0);

  return ligma_container_get_n_children (stack->undos);
}
