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

#include <gtk/gtk.h>

#include "core-types.h"

#include "gimpimage.h"
#include "gimplist.h"
#include "gimpundo.h"
#include "gimpundostack.h"


static void  gimp_undo_stack_class_init  (GimpUndoStackClass *klass);
static void  gimp_undo_stack_init        (GimpUndoStack      *stack);
static void  gimp_undo_stack_destroy     (GtkObject          *object);

static void  gimp_undo_stack_add_callback    (GimpContainer      *container,
                                              GimpObject         *object,
                                              gpointer            data);
static void  gimp_undo_stack_remove_callback (GimpContainer      *container,
                                              GimpObject         *object,
                                              gpointer           data);

static GimpUndoClass *parent_class = NULL;


GtkType
gimp_undo_stack_get_type (void)
{
  static GtkType undo_stack_type = 0;

  if (! undo_stack_type)
    {
      static const GtkTypeInfo undo_stack_info =
      {
        "GimpUndoStack",
        sizeof (GimpUndoStack),
        sizeof (GimpUndoStackClass),
        (GtkClassInitFunc) gimp_undo_stack_class_init,
        (GtkObjectInitFunc) gimp_undo_stack_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      undo_stack_type = gtk_type_unique (GIMP_TYPE_UNDO, 
                                         &undo_stack_info);
    }

  return undo_stack_type;
}

static void
gimp_undo_stack_class_init (GimpUndoStackClass *klass)
{
  GtkObjectClass  *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_UNDO);

  object_class->destroy = gimp_undo_stack_destroy;
}

static void
gimp_undo_stack_init (GimpUndoStack *stack)
{
  GimpContainer *undos;

  undos = gimp_list_new (GIMP_TYPE_UNDO,
                         GIMP_CONTAINER_POLICY_STRONG);

  stack->undos  = undos;
  gtk_object_ref (GTK_OBJECT (undos));
  gtk_object_sink (GTK_OBJECT (undos));  

  gtk_signal_connect (GTK_OBJECT (undos), "add",
                      GTK_SIGNAL_FUNC (gimp_undo_stack_add_callback), 
                      stack);
  gtk_signal_connect (GTK_OBJECT (undos), "remove",
                      GTK_SIGNAL_FUNC (gimp_undo_stack_remove_callback), 
                      stack);

  stack->gimage = NULL;
}

static void
gimp_undo_stack_destroy (GtkObject *object)
{
  GimpUndoStack *stack;

  stack = GIMP_UNDO_STACK (object);

  gtk_object_unref (GTK_OBJECT (stack->undos));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GimpUndoStack *
gimp_undo_stack_new (GimpImage *gimage)
{
  GimpUndoStack *stack;

  g_return_val_if_fail (gimage != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  stack = GIMP_UNDO_STACK (gtk_object_new (GIMP_TYPE_UNDO_STACK, 
                                           NULL));

  stack->gimage = gimage;

  return stack;
}

void            
gimp_undo_stack_push (GimpUndoStack *stack, 
                      GimpUndo      *undo)
{
  g_return_if_fail (stack != NULL);
  g_return_if_fail (GIMP_IS_UNDO_STACK (stack));
  
  g_return_if_fail (undo != NULL);
  g_return_if_fail (GIMP_IS_UNDO (undo));

  gimp_undo_push (undo, stack->gimage);
  gimp_container_add (GIMP_CONTAINER (stack->undos), GIMP_OBJECT (undo));
}

GimpUndo * 
gimp_undo_stack_pop (GimpUndoStack *stack)
{
  GimpObject *object;

  g_return_val_if_fail (stack != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_UNDO_STACK (stack), NULL);
  
  object = gimp_container_get_child_by_index (GIMP_CONTAINER (stack->undos),0);

  if (object)
    {
      gimp_container_remove (GIMP_CONTAINER (stack->undos), object);
      gimp_undo_pop (GIMP_UNDO (object), stack->gimage);

      return GIMP_UNDO (object);
    }
 
  return NULL;
}

GimpUndo * 
gimp_undo_stack_peek (GimpUndoStack *stack)
{
  GimpObject *object;

  g_return_val_if_fail (stack != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_UNDO_STACK (stack), NULL);

  object = gimp_container_get_child_by_index (GIMP_CONTAINER (stack->undos),0);

  return (object ? GIMP_UNDO (object) : NULL);
}

static void
gimp_undo_stack_add_callback (GimpContainer *container,
                              GimpObject    *object,
                              gpointer       data)
{
  GimpUndo *undo;
  GimpUndo *stack;

  undo = GIMP_UNDO (object);
  stack = GIMP_UNDO (data);
  
  stack->size += undo->size;
}

static void
gimp_undo_stack_remove_callback (GimpContainer *container,
                                 GimpObject    *object,
                                 gpointer       data)
{
  GimpUndo *undo;
  GimpUndo *stack;

  undo = GIMP_UNDO (object);
  stack = GIMP_UNDO (data);
  
  stack->size -= undo->size;
}
