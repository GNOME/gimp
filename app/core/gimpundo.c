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

#include "base/temp-buf.h"

#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimpundo.h"


enum
{
  PUSH,
  POP,
  LAST_SIGNAL
};


static void      gimp_undo_class_init  (GimpUndoClass *klass);
static void      gimp_undo_init        (GimpUndo      *undo);

static void      gimp_undo_finalize    (GObject       *object);

static void      gimp_undo_real_push   (GimpUndo      *undo,
                                        GimpImage     *gimage);
static void      gimp_undo_real_pop    (GimpUndo      *undo,
                                        GimpImage     *gimage);
static TempBuf * gimp_undo_get_preview (GimpViewable  *viewable,
                                        gint           width,
                                        gint           height);


static guint undo_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GType
gimp_undo_get_type (void)
{
  static GType undo_type = 0;

  if (! undo_type)
    {
      static const GTypeInfo undo_info =
      {
        sizeof (GimpUndoClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_undo_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpUndo),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_undo_init,
      };

      undo_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
					  "GimpUndo",
					  &undo_info, 0);
  }

  return undo_type;
}

static void
gimp_undo_class_init (GimpUndoClass *klass)
{
  GObjectClass      *object_class;
  GimpViewableClass *viewable_class;

  object_class   = G_OBJECT_CLASS (klass);
  viewable_class = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  undo_signals[PUSH] = 
    g_signal_new ("push",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpUndoClass, push),
		  NULL, NULL,
		  gimp_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  undo_signals[POP] = 
    g_signal_new ("pop",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpUndoClass, pop),
		  NULL, NULL,
		  gimp_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);

  object_class->finalize      = gimp_undo_finalize;

  viewable_class->get_preview = gimp_undo_get_preview;

  klass->push                 = gimp_undo_real_push;
  klass->pop                  = gimp_undo_real_pop;
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
gimp_undo_finalize (GObject *object)
{
  GimpUndo *undo;

  undo = GIMP_UNDO (object);

  if (undo->free_func)
    {
      undo->free_func (undo);
      undo->free_func = NULL;
    }

  if (undo->preview)
    {
      temp_buf_free (undo->preview);
      undo->preview = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
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

  undo = GIMP_UNDO (g_object_new (GIMP_TYPE_UNDO, NULL));
  
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
  g_return_if_fail (GIMP_IS_UNDO (undo));
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (undo), undo_signals[PUSH], 0,
		 gimage);
}

void
gimp_undo_pop (GimpUndo  *undo,
               GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  g_signal_emit (G_OBJECT (undo), undo_signals[POP], 0,
		 gimage);
}

static void
gimp_undo_real_push (GimpUndo  *undo,
                     GimpImage *gimage)
{
  /* FIXME: need core_config->undo_preview_size */

  undo->preview = gimp_viewable_get_preview (GIMP_VIEWABLE (gimage),
                                             24,
					     24);
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
