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

#include "apptypes.h"

#include "gimpimage.h"
#include "gimprc.h"
#include "gimpundo.h"
#include "temp_buf.h"


enum
{
  PUSH,
  POP,
  LAST_SIGNAL
};


static void      gimp_undo_class_init  (GimpUndoClass *klass);
static void      gimp_undo_init        (GimpUndo      *undo);
static void      gimp_undo_destroy     (GtkObject     *object);
static void      gimp_undo_real_push   (GimpUndo      *undo,
                                        GimpImage     *gimage);
static void      gimp_undo_real_pop    (GimpUndo      *undo,
                                        GimpImage     *gimage);
static TempBuf * gimp_undo_get_preview (GimpViewable  *viewable,
                                        gint           width,
                                        gint           height);


static guint undo_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GtkType
gimp_undo_get_type (void)
{
  static GtkType undo_type = 0;

  if (! undo_type)
    {
      static const GtkTypeInfo undo_info =
      {
        "GimpUndo",
        sizeof (GimpUndo),
        sizeof (GimpUndoClass),
        (GtkClassInitFunc) gimp_undo_class_init,
        (GtkObjectInitFunc) gimp_undo_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      undo_type = gtk_type_unique (GIMP_TYPE_VIEWABLE, &undo_info);
  }

  return undo_type;
}

static void
gimp_undo_class_init (GimpUndoClass *klass)
{
  GtkObjectClass     *object_class;
  GimpViewableClass  *viewable_class;

  object_class = (GtkObjectClass *) klass;
  viewable_class = (GimpViewableClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_VIEWABLE);

  undo_signals[PUSH] = 
    gtk_signal_new ("push",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpUndoClass, push),
                    gtk_marshal_NONE__POINTER,
                    GTK_TYPE_NONE, 
                    1, GTK_TYPE_POINTER);

  undo_signals[POP] = 
    gtk_signal_new ("pop",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpUndoClass, pop),
                    gtk_marshal_NONE__POINTER,
                    GTK_TYPE_NONE, 
                    1, GTK_TYPE_POINTER);

  gtk_object_class_add_signals (object_class, undo_signals, LAST_SIGNAL);

  object_class->destroy = gimp_undo_destroy;
  
  viewable_class->get_preview = gimp_undo_get_preview;

  klass->push = gimp_undo_real_push;
  klass->pop  = gimp_undo_real_pop;
}

static void
gimp_undo_init (GimpUndo *undo)
{
  undo->data          = NULL;
  undo->size          = 0;
  undo->dirties_image = FALSE;
  undo->pop_func      = NULL;
  undo->free_func     = NULL;
  undo->preview       = NULL;
}

static void
gimp_undo_destroy (GtkObject *object)
{
  GimpUndo *undo;

  undo = GIMP_UNDO (object);

  if (undo->free_func)
    undo->free_func (undo);

  if (undo->preview)
    temp_buf_free (undo->preview);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GimpUndo *
gimp_undo_new (const gchar      *name,
               gpointer          data,
               glong             size,
               gboolean          dirties_image,
               GimpUndoPopFunc   pop_func,
               GimpUndoFreeFunc  free_func)
{
  GimpUndo *undo;

  undo = GIMP_UNDO (gtk_object_new (GIMP_TYPE_UNDO, NULL));
  
  gimp_object_set_name (GIMP_OBJECT (undo), name);
  
  undo->data = data;
  undo->size = sizeof (GimpUndo) + size;
  
  undo->pop_func  = pop_func;
  undo->free_func = free_func;

  return undo;
}

void
gimp_undo_push (GimpUndo  *undo,
                GimpImage *gimage)
{
  g_return_if_fail (undo != NULL);
  g_return_if_fail (GIMP_IS_UNDO (undo));
  
  g_return_if_fail (gimage != NULL);
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gtk_signal_emit (GTK_OBJECT (undo), undo_signals[PUSH], gimage);
}

void
gimp_undo_pop (GimpUndo  *undo,
               GimpImage *gimage)
{
  g_return_if_fail (undo != NULL);
  g_return_if_fail (GIMP_IS_UNDO (undo));
  
  g_return_if_fail (gimage != NULL);
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gtk_signal_emit (GTK_OBJECT (undo), undo_signals[POP], gimage);
}

static void
gimp_undo_real_push (GimpUndo  *undo,
                     GimpImage *gimage)
{
  undo->preview = gimp_viewable_get_preview (GIMP_VIEWABLE (gimage),
                                             preview_size, preview_size);
}

static void
gimp_undo_real_pop (GimpUndo  *undo,
                    GimpImage *gimage)
{
  if (undo->pop_func)
    undo->pop_func (undo, gimage);
}

static TempBuf *
gimp_undo_get_preview (GimpViewable *viewable,
                       gint          width,
                       gint          height)
{
  return (GIMP_UNDO (viewable)->preview);
}
