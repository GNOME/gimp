/* Gimp - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp-memsize.h"
#include "gimpitem.h"
#include "gimpitemtree.h"
#include "gimpitempropundo.h"
#include "gimpparasitelist.h"


enum
{
  PROP_0,
  PROP_PARASITE_NAME
};


static void     gimp_item_prop_undo_constructed  (GObject             *object);
static void     gimp_item_prop_undo_set_property (GObject             *object,
                                                  guint                property_id,
                                                  const GValue        *value,
                                                  GParamSpec          *pspec);
static void     gimp_item_prop_undo_get_property (GObject             *object,
                                                  guint                property_id,
                                                  GValue              *value,
                                                  GParamSpec          *pspec);

static gint64   gimp_item_prop_undo_get_memsize  (GimpObject          *object,
                                                  gint64              *gui_size);

static void     gimp_item_prop_undo_pop          (GimpUndo            *undo,
                                                  GimpUndoMode         undo_mode,
                                                  GimpUndoAccumulator *accum);
static void     gimp_item_prop_undo_free         (GimpUndo            *undo,
                                                  GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpItemPropUndo, gimp_item_prop_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_item_prop_undo_parent_class


static void
gimp_item_prop_undo_class_init (GimpItemPropUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed      = gimp_item_prop_undo_constructed;
  object_class->set_property     = gimp_item_prop_undo_set_property;
  object_class->get_property     = gimp_item_prop_undo_get_property;

  gimp_object_class->get_memsize = gimp_item_prop_undo_get_memsize;

  undo_class->pop                = gimp_item_prop_undo_pop;
  undo_class->free               = gimp_item_prop_undo_free;

  g_object_class_install_property (object_class, PROP_PARASITE_NAME,
                                   g_param_spec_string ("parasite-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_item_prop_undo_init (GimpItemPropUndo *undo)
{
}

static void
gimp_item_prop_undo_constructed (GObject *object)
{
  GimpItemPropUndo *item_prop_undo = GIMP_ITEM_PROP_UNDO (object);
  GimpItem         *item;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  item = GIMP_ITEM_UNDO (object)->item;

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_ITEM_REORDER:
      item_prop_undo->parent   = gimp_item_get_parent (item);
      item_prop_undo->position = gimp_item_get_index (item);
      break;

    case GIMP_UNDO_ITEM_RENAME:
      item_prop_undo->name = g_strdup (gimp_object_get_name (item));
      break;

    case GIMP_UNDO_ITEM_DISPLACE:
      gimp_item_get_offset (item,
                            &item_prop_undo->offset_x,
                            &item_prop_undo->offset_y);
      break;

    case GIMP_UNDO_ITEM_VISIBILITY:
      item_prop_undo->visible = gimp_item_get_visible (item);
      break;

    case GIMP_UNDO_ITEM_COLOR_TAG:
      item_prop_undo->color_tag = gimp_item_get_color_tag (item);
      break;

    case GIMP_UNDO_ITEM_LOCK_CONTENT:
      item_prop_undo->lock_content = gimp_item_get_lock_content (item);
      break;

    case GIMP_UNDO_ITEM_LOCK_POSITION:
      item_prop_undo->lock_position = gimp_item_get_lock_position (item);
      break;

    case GIMP_UNDO_ITEM_LOCK_VISIBILITY:
      item_prop_undo->lock_visibility = gimp_item_get_lock_visibility (item);
      break;

    case GIMP_UNDO_PARASITE_ATTACH:
    case GIMP_UNDO_PARASITE_REMOVE:
      gimp_assert (item_prop_undo->parasite_name != NULL);

      item_prop_undo->parasite = gimp_parasite_copy
        (gimp_item_parasite_find (item, item_prop_undo->parasite_name));
      break;

    default:
      g_return_if_reached ();
    }
}

static void
gimp_item_prop_undo_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpItemPropUndo *item_prop_undo = GIMP_ITEM_PROP_UNDO (object);

  switch (property_id)
    {
    case PROP_PARASITE_NAME:
      item_prop_undo->parasite_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_item_prop_undo_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpItemPropUndo *item_prop_undo = GIMP_ITEM_PROP_UNDO (object);

  switch (property_id)
    {
    case PROP_PARASITE_NAME:
      g_value_set_string (value, item_prop_undo->parasite_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_item_prop_undo_get_memsize (GimpObject *object,
                                 gint64     *gui_size)
{
  GimpItemPropUndo *item_prop_undo = GIMP_ITEM_PROP_UNDO (object);
  gint64            memsize        = 0;

  memsize += gimp_string_get_memsize (item_prop_undo->name);
  memsize += gimp_string_get_memsize (item_prop_undo->parasite_name);
  memsize += gimp_parasite_get_memsize (item_prop_undo->parasite, NULL);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_item_prop_undo_pop (GimpUndo            *undo,
                         GimpUndoMode         undo_mode,
                         GimpUndoAccumulator *accum)
{
  GimpItemPropUndo *item_prop_undo = GIMP_ITEM_PROP_UNDO (undo);
  GimpItem         *item           = GIMP_ITEM_UNDO (undo)->item;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_ITEM_REORDER:
      {
        GimpItem *parent;
        gint      position;

        parent   = gimp_item_get_parent (item);
        position = gimp_item_get_index (item);

        gimp_item_tree_reorder_item (gimp_item_get_tree (item), item,
                                     item_prop_undo->parent,
                                     item_prop_undo->position,
                                     FALSE, NULL);

        item_prop_undo->parent   = parent;
        item_prop_undo->position = position;
      }
      break;

    case GIMP_UNDO_ITEM_RENAME:
      {
        gchar *name;

        name = g_strdup (gimp_object_get_name (item));

        gimp_item_tree_rename_item (gimp_item_get_tree (item), item,
                                    item_prop_undo->name,
                                    FALSE, NULL);

        g_free (item_prop_undo->name);
        item_prop_undo->name = name;
      }
      break;

    case GIMP_UNDO_ITEM_DISPLACE:
      {
        gint offset_x;
        gint offset_y;

        gimp_item_get_offset (item, &offset_x, &offset_y);

        gimp_item_translate (item,
                             item_prop_undo->offset_x - offset_x,
                             item_prop_undo->offset_y - offset_y,
                             FALSE);

        item_prop_undo->offset_x = offset_x;
        item_prop_undo->offset_y = offset_y;
      }
      break;

    case GIMP_UNDO_ITEM_VISIBILITY:
      {
        gboolean visible;

        visible = gimp_item_get_visible (item);
        gimp_item_set_visible (item, item_prop_undo->visible, FALSE);
        item_prop_undo->visible = visible;
      }
      break;

    case GIMP_UNDO_ITEM_COLOR_TAG:
      {
        GimpColorTag color_tag;

        color_tag = gimp_item_get_color_tag (item);
        gimp_item_set_color_tag (item, item_prop_undo->color_tag, FALSE);
        item_prop_undo->color_tag = color_tag;
      }
      break;

    case GIMP_UNDO_ITEM_LOCK_CONTENT:
      {
        gboolean lock_content;

        lock_content = gimp_item_get_lock_content (item);
        gimp_item_set_lock_content (item, item_prop_undo->lock_content, FALSE);
        item_prop_undo->lock_content = lock_content;
      }
      break;

    case GIMP_UNDO_ITEM_LOCK_POSITION:
      {
        gboolean lock_position;

        lock_position = gimp_item_get_lock_position (item);
        gimp_item_set_lock_position (item, item_prop_undo->lock_position, FALSE);
        item_prop_undo->lock_position = lock_position;
      }
      break;

    case GIMP_UNDO_ITEM_LOCK_VISIBILITY:
      {
        gboolean lock_visibility;

        lock_visibility = gimp_item_get_lock_visibility (item);
        gimp_item_set_lock_visibility (item, item_prop_undo->lock_visibility, FALSE);
        item_prop_undo->lock_visibility = lock_visibility;
      }
      break;

    case GIMP_UNDO_PARASITE_ATTACH:
    case GIMP_UNDO_PARASITE_REMOVE:
      {
        GimpParasite *parasite;

        parasite = item_prop_undo->parasite;

        item_prop_undo->parasite = gimp_parasite_copy
          (gimp_item_parasite_find (item, item_prop_undo->parasite_name));

        if (parasite)
          gimp_item_parasite_attach (item, parasite, FALSE);
        else
          gimp_item_parasite_detach (item, item_prop_undo->parasite_name, FALSE);

        if (parasite)
          gimp_parasite_free (parasite);
      }
      break;

    default:
      g_return_if_reached ();
    }
}

static void
gimp_item_prop_undo_free (GimpUndo     *undo,
                          GimpUndoMode  undo_mode)
{
  GimpItemPropUndo *item_prop_undo = GIMP_ITEM_PROP_UNDO (undo);

  g_clear_pointer (&item_prop_undo->name,          g_free);
  g_clear_pointer (&item_prop_undo->parasite_name, g_free);
  g_clear_pointer (&item_prop_undo->parasite,      gimp_parasite_free);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
