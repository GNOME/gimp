/* Gimp - The GNU Image Manipulation Program
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

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpitem.h"
#include "gimpitempropundo.h"


static GObject * gimp_item_prop_undo_constructor (GType                  type,
                                                  guint                  n_params,
                                                  GObjectConstructParam *params);

static void      gimp_item_prop_undo_pop         (GimpUndo              *undo,
                                                  GimpUndoMode           undo_mode,
                                                  GimpUndoAccumulator   *accum);
static void      gimp_item_prop_undo_free        (GimpUndo              *undo,
                                                  GimpUndoMode           undo_mode);


G_DEFINE_TYPE (GimpItemPropUndo, gimp_item_prop_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_item_prop_undo_parent_class


static void
gimp_item_prop_undo_class_init (GimpItemPropUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructor = gimp_item_prop_undo_constructor;

  undo_class->pop           = gimp_item_prop_undo_pop;
  undo_class->free          = gimp_item_prop_undo_free;
}

static void
gimp_item_prop_undo_init (GimpItemPropUndo *undo)
{
}

static GObject *
gimp_item_prop_undo_constructor (GType                  type,
                                 guint                  n_params,
                                 GObjectConstructParam *params)
{
  GObject          *object;
  GimpItemPropUndo *item_prop_undo;
  GimpItem         *item;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  item_prop_undo = GIMP_ITEM_PROP_UNDO (object);

  item = GIMP_ITEM_UNDO (object)->item;

  if (GIMP_UNDO (object)->undo_type == GIMP_UNDO_ITEM_RENAME)
    {
      item_prop_undo->name = g_strdup (gimp_object_get_name (GIMP_OBJECT (item)));
      GIMP_UNDO (object)->size += strlen (item_prop_undo->name) + 1;
    }

  gimp_item_offsets (item,
                     &item_prop_undo->offset_x,
                     &item_prop_undo->offset_y);

  item_prop_undo->visible = gimp_item_get_visible (item);
  item_prop_undo->linked  = gimp_item_get_linked (item);

  return object;
}

static void
gimp_item_prop_undo_pop (GimpUndo            *undo,
                         GimpUndoMode         undo_mode,
                         GimpUndoAccumulator *accum)
{
  GimpItemPropUndo *item_prop_undo = GIMP_ITEM_PROP_UNDO (undo);
  GimpItem         *item           = GIMP_ITEM_UNDO (undo)->item;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if (undo->undo_type == GIMP_UNDO_ITEM_RENAME)
    {
      gchar *name;

      undo->size -= strlen (item_prop_undo->name) + 1;

      name = g_strdup (gimp_object_get_name (GIMP_OBJECT (item)));
      gimp_object_take_name (GIMP_OBJECT (item), item_prop_undo->name);
      item_prop_undo->name = name;

      undo->size += strlen (item_prop_undo->name) + 1;
    }
  else if (undo->undo_type == GIMP_UNDO_ITEM_DISPLACE)
    {
      gint offset_x;
      gint offset_y;

      gimp_item_offsets (item, &offset_x, &offset_y);

      gimp_item_translate (item,
                           item_prop_undo->offset_x - offset_x,
                           item_prop_undo->offset_y - offset_y,
                           FALSE);

      item_prop_undo->offset_x = offset_x;
      item_prop_undo->offset_y = offset_y;
    }
  else if (undo->undo_type == GIMP_UNDO_ITEM_VISIBILITY)
    {
      gboolean visible;

      visible = gimp_item_get_visible (item);
      gimp_item_set_visible (item, item_prop_undo->visible, FALSE);
      item_prop_undo->visible = visible;
    }
  else if (undo->undo_type == GIMP_UNDO_ITEM_LINKED)
    {
      gboolean linked;

      linked = gimp_item_get_linked (item);
      gimp_item_set_linked (item, item_prop_undo->linked, FALSE);
      item_prop_undo->linked = linked;
    }
  else
    {
      g_assert_not_reached ();
    }
}

static void
gimp_item_prop_undo_free (GimpUndo     *undo,
                        GimpUndoMode  undo_mode)
{
  GimpItemPropUndo *item_prop_undo = GIMP_ITEM_PROP_UNDO (undo);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);

  if (item_prop_undo->name)
    {
      g_free (item_prop_undo->name);
      item_prop_undo->name = NULL;
    }
}
