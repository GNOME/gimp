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


static void  gimp_undo_stack_class_init      (GimpUndoStackClass *klass);
static void  gimp_undo_stack_init            (GimpUndoStack      *stack);

static void  gimp_undo_stack_finalize        (GObject            *object);

static void  gimp_undo_stack_add_callback    (GimpContainer      *container,
                                              GimpObject         *object,
                                              gpointer            data);
static void  gimp_undo_stack_remove_callback (GimpContainer      *container,
                                              GimpObject         *object,
                                              gpointer           data);


static GimpUndoClass *parent_class = NULL;


GType
gimp_undo_stack_get_type (void)
{
  static GType undo_stack_type = 0;

  if (! undo_stack_type)
    {
      static const GTypeInfo undo_stack_info =
      {
        sizeof (GimpUndoStackClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_undo_stack_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpUndoStack),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_undo_stack_init,
      };

      undo_stack_type = g_type_register_static (GIMP_TYPE_UNDO,
						"GimpUndoStack", 
						&undo_stack_info, 0);
    }

  return undo_stack_type;
}

static void
gimp_undo_stack_class_init (GimpUndoStackClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_undo_stack_finalize;
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

  g_signal_connect (G_OBJECT (undos), "add",
		    G_CALLBACK (gimp_undo_stack_add_callback), 
		    stack);
  g_signal_connect (G_OBJECT (undos), "remove",
		    G_CALLBACK (gimp_undo_stack_remove_callback), 
		    stack);

  stack->gimage = NULL;
}

static void
gimp_undo_stack_finalize (GObject *object)
{
  GimpUndoStack *stack;

  stack = GIMP_UNDO_STACK (object);

  if (stack->undos)
    {
      g_object_unref (G_OBJECT (stack->undos));
      stack->undos = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpUndoStack *
gimp_undo_stack_new (GimpImage *gimage)
{
  GimpUndoStack *stack;

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
  g_return_if_fail (GIMP_IS_UNDO_STACK (stack));
  g_return_if_fail (GIMP_IS_UNDO (undo));

  gimp_undo_push (undo, stack->gimage);
  gimp_container_add (GIMP_CONTAINER (stack->undos), GIMP_OBJECT (undo));
}

GimpUndo * 
gimp_undo_stack_pop (GimpUndoStack *stack)
{
  GimpObject *object;

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

  g_return_val_if_fail (GIMP_IS_UNDO_STACK (stack), NULL);

  object = gimp_container_get_child_by_index (GIMP_CONTAINER (stack->undos), 0);

  return (object ? GIMP_UNDO (object) : NULL);
}

static void
gimp_undo_stack_add_callback (GimpContainer *container,
                              GimpObject    *object,
                              gpointer       data)
{
  GimpUndo *undo;
  GimpUndo *stack;

  undo  = GIMP_UNDO (object);
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

  undo  = GIMP_UNDO (object);
  stack = GIMP_UNDO (data);
  
  stack->size -= undo->size;
}
