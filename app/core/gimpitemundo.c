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

#include "gimpimage.h"
#include "gimpitem.h"
#include "gimpitemundo.h"


static void      gimp_item_undo_class_init (GimpItemUndoClass *klass);
static void      gimp_item_undo_init       (GimpItemUndo      *undo);

static void      gimp_item_undo_free       (GimpUndo          *undo,
                                            GimpUndoMode       undo_mode);


static GimpUndoClass *parent_class = NULL;


GType
gimp_item_undo_get_type (void)
{
  static GType undo_type = 0;

  if (! undo_type)
    {
      static const GTypeInfo undo_info =
      {
        sizeof (GimpItemUndoClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_item_undo_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpItemUndo),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_item_undo_init,
      };

      undo_type = g_type_register_static (GIMP_TYPE_UNDO,
					  "GimpItemUndo",
					  &undo_info, 0);
  }

  return undo_type;
}

static void
gimp_item_undo_class_init (GimpItemUndoClass *klass)
{
  GObjectClass  *object_class;
  GimpUndoClass *undo_class;

  object_class = G_OBJECT_CLASS (klass);
  undo_class   = GIMP_UNDO_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  undo_class->free = gimp_item_undo_free;
}

static void
gimp_item_undo_init (GimpItemUndo *undo)
{
  undo->item = NULL;
}

static void
gimp_item_undo_free (GimpUndo     *undo,
                     GimpUndoMode  undo_mode)
{
  GimpItemUndo *item_undo;

  item_undo = GIMP_ITEM_UNDO (undo);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);

  if (item_undo->item)
    {
      g_object_unref (item_undo->item);
      item_undo->item = NULL;
    }
}

GimpUndo *
gimp_item_undo_new (GimpImage        *gimage,
                    GimpItem         *item,
                    GimpUndoType      undo_type,
                    const gchar      *name,
                    gpointer          data,
                    gint64            size,
                    gboolean          dirties_image,
                    GimpUndoPopFunc   pop_func,
                    GimpUndoFreeFunc  free_func)
{
  GimpUndo *undo;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (size == 0 || data != NULL, NULL);

  if (! name)
    name = gimp_undo_type_to_name (undo_type);

  undo = g_object_new (GIMP_TYPE_ITEM_UNDO,
                       "name", name,
                       NULL);

  undo->gimage        = gimage;
  undo->undo_type     = undo_type;
  undo->data          = data;
  undo->size          = size;
  undo->dirties_image = dirties_image ? TRUE : FALSE;
  undo->pop_func      = pop_func;
  undo->free_func     = free_func;

  GIMP_ITEM_UNDO (undo)->item = g_object_ref (item);

  return undo;
}
