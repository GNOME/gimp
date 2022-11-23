/* Ligma - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma-memsize.h"
#include "ligmaitem.h"
#include "ligmaitemtree.h"
#include "ligmaitempropundo.h"
#include "ligmaparasitelist.h"


enum
{
  PROP_0,
  PROP_PARASITE_NAME
};


static void     ligma_item_prop_undo_constructed  (GObject             *object);
static void     ligma_item_prop_undo_set_property (GObject             *object,
                                                  guint                property_id,
                                                  const GValue        *value,
                                                  GParamSpec          *pspec);
static void     ligma_item_prop_undo_get_property (GObject             *object,
                                                  guint                property_id,
                                                  GValue              *value,
                                                  GParamSpec          *pspec);

static gint64   ligma_item_prop_undo_get_memsize  (LigmaObject          *object,
                                                  gint64              *gui_size);

static void     ligma_item_prop_undo_pop          (LigmaUndo            *undo,
                                                  LigmaUndoMode         undo_mode,
                                                  LigmaUndoAccumulator *accum);
static void     ligma_item_prop_undo_free         (LigmaUndo            *undo,
                                                  LigmaUndoMode         undo_mode);


G_DEFINE_TYPE (LigmaItemPropUndo, ligma_item_prop_undo, LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_item_prop_undo_parent_class


static void
ligma_item_prop_undo_class_init (LigmaItemPropUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaUndoClass   *undo_class        = LIGMA_UNDO_CLASS (klass);

  object_class->constructed      = ligma_item_prop_undo_constructed;
  object_class->set_property     = ligma_item_prop_undo_set_property;
  object_class->get_property     = ligma_item_prop_undo_get_property;

  ligma_object_class->get_memsize = ligma_item_prop_undo_get_memsize;

  undo_class->pop                = ligma_item_prop_undo_pop;
  undo_class->free               = ligma_item_prop_undo_free;

  g_object_class_install_property (object_class, PROP_PARASITE_NAME,
                                   g_param_spec_string ("parasite-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_item_prop_undo_init (LigmaItemPropUndo *undo)
{
}

static void
ligma_item_prop_undo_constructed (GObject *object)
{
  LigmaItemPropUndo *item_prop_undo = LIGMA_ITEM_PROP_UNDO (object);
  LigmaItem         *item;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  item = LIGMA_ITEM_UNDO (object)->item;

  switch (LIGMA_UNDO (object)->undo_type)
    {
    case LIGMA_UNDO_ITEM_REORDER:
      item_prop_undo->parent   = ligma_item_get_parent (item);
      item_prop_undo->position = ligma_item_get_index (item);
      break;

    case LIGMA_UNDO_ITEM_RENAME:
      item_prop_undo->name = g_strdup (ligma_object_get_name (item));
      break;

    case LIGMA_UNDO_ITEM_DISPLACE:
      ligma_item_get_offset (item,
                            &item_prop_undo->offset_x,
                            &item_prop_undo->offset_y);
      break;

    case LIGMA_UNDO_ITEM_VISIBILITY:
      item_prop_undo->visible = ligma_item_get_visible (item);
      break;

    case LIGMA_UNDO_ITEM_COLOR_TAG:
      item_prop_undo->color_tag = ligma_item_get_color_tag (item);
      break;

    case LIGMA_UNDO_ITEM_LOCK_CONTENT:
      item_prop_undo->lock_content = ligma_item_get_lock_content (item);
      break;

    case LIGMA_UNDO_ITEM_LOCK_POSITION:
      item_prop_undo->lock_position = ligma_item_get_lock_position (item);
      break;

    case LIGMA_UNDO_ITEM_LOCK_VISIBILITY:
      item_prop_undo->lock_visibility = ligma_item_get_lock_visibility (item);
      break;

    case LIGMA_UNDO_PARASITE_ATTACH:
    case LIGMA_UNDO_PARASITE_REMOVE:
      ligma_assert (item_prop_undo->parasite_name != NULL);

      item_prop_undo->parasite = ligma_parasite_copy
        (ligma_item_parasite_find (item, item_prop_undo->parasite_name));
      break;

    default:
      g_return_if_reached ();
    }
}

static void
ligma_item_prop_undo_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LigmaItemPropUndo *item_prop_undo = LIGMA_ITEM_PROP_UNDO (object);

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
ligma_item_prop_undo_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  LigmaItemPropUndo *item_prop_undo = LIGMA_ITEM_PROP_UNDO (object);

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
ligma_item_prop_undo_get_memsize (LigmaObject *object,
                                 gint64     *gui_size)
{
  LigmaItemPropUndo *item_prop_undo = LIGMA_ITEM_PROP_UNDO (object);
  gint64            memsize        = 0;

  memsize += ligma_string_get_memsize (item_prop_undo->name);
  memsize += ligma_string_get_memsize (item_prop_undo->parasite_name);
  memsize += ligma_parasite_get_memsize (item_prop_undo->parasite, NULL);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_item_prop_undo_pop (LigmaUndo            *undo,
                         LigmaUndoMode         undo_mode,
                         LigmaUndoAccumulator *accum)
{
  LigmaItemPropUndo *item_prop_undo = LIGMA_ITEM_PROP_UNDO (undo);
  LigmaItem         *item           = LIGMA_ITEM_UNDO (undo)->item;

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case LIGMA_UNDO_ITEM_REORDER:
      {
        LigmaItem *parent;
        gint      position;

        parent   = ligma_item_get_parent (item);
        position = ligma_item_get_index (item);

        ligma_item_tree_reorder_item (ligma_item_get_tree (item), item,
                                     item_prop_undo->parent,
                                     item_prop_undo->position,
                                     FALSE, NULL);

        item_prop_undo->parent   = parent;
        item_prop_undo->position = position;
      }
      break;

    case LIGMA_UNDO_ITEM_RENAME:
      {
        gchar *name;

        name = g_strdup (ligma_object_get_name (item));

        ligma_item_tree_rename_item (ligma_item_get_tree (item), item,
                                    item_prop_undo->name,
                                    FALSE, NULL);

        g_free (item_prop_undo->name);
        item_prop_undo->name = name;
      }
      break;

    case LIGMA_UNDO_ITEM_DISPLACE:
      {
        gint offset_x;
        gint offset_y;

        ligma_item_get_offset (item, &offset_x, &offset_y);

        ligma_item_translate (item,
                             item_prop_undo->offset_x - offset_x,
                             item_prop_undo->offset_y - offset_y,
                             FALSE);

        item_prop_undo->offset_x = offset_x;
        item_prop_undo->offset_y = offset_y;
      }
      break;

    case LIGMA_UNDO_ITEM_VISIBILITY:
      {
        gboolean visible;

        visible = ligma_item_get_visible (item);
        ligma_item_set_visible (item, item_prop_undo->visible, FALSE);
        item_prop_undo->visible = visible;
      }
      break;

    case LIGMA_UNDO_ITEM_COLOR_TAG:
      {
        LigmaColorTag color_tag;

        color_tag = ligma_item_get_color_tag (item);
        ligma_item_set_color_tag (item, item_prop_undo->color_tag, FALSE);
        item_prop_undo->color_tag = color_tag;
      }
      break;

    case LIGMA_UNDO_ITEM_LOCK_CONTENT:
      {
        gboolean lock_content;

        lock_content = ligma_item_get_lock_content (item);
        ligma_item_set_lock_content (item, item_prop_undo->lock_content, FALSE);
        item_prop_undo->lock_content = lock_content;
      }
      break;

    case LIGMA_UNDO_ITEM_LOCK_POSITION:
      {
        gboolean lock_position;

        lock_position = ligma_item_get_lock_position (item);
        ligma_item_set_lock_position (item, item_prop_undo->lock_position, FALSE);
        item_prop_undo->lock_position = lock_position;
      }
      break;

    case LIGMA_UNDO_ITEM_LOCK_VISIBILITY:
      {
        gboolean lock_visibility;

        lock_visibility = ligma_item_get_lock_visibility (item);
        ligma_item_set_lock_visibility (item, item_prop_undo->lock_visibility, FALSE);
        item_prop_undo->lock_visibility = lock_visibility;
      }
      break;

    case LIGMA_UNDO_PARASITE_ATTACH:
    case LIGMA_UNDO_PARASITE_REMOVE:
      {
        LigmaParasite *parasite;

        parasite = item_prop_undo->parasite;

        item_prop_undo->parasite = ligma_parasite_copy
          (ligma_item_parasite_find (item, item_prop_undo->parasite_name));

        if (parasite)
          ligma_item_parasite_attach (item, parasite, FALSE);
        else
          ligma_item_parasite_detach (item, item_prop_undo->parasite_name, FALSE);

        if (parasite)
          ligma_parasite_free (parasite);
      }
      break;

    default:
      g_return_if_reached ();
    }
}

static void
ligma_item_prop_undo_free (LigmaUndo     *undo,
                          LigmaUndoMode  undo_mode)
{
  LigmaItemPropUndo *item_prop_undo = LIGMA_ITEM_PROP_UNDO (undo);

  g_clear_pointer (&item_prop_undo->name,          g_free);
  g_clear_pointer (&item_prop_undo->parasite_name, g_free);
  g_clear_pointer (&item_prop_undo->parasite,      ligma_parasite_free);

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
