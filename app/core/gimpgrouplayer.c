/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGroupLayer
 * Copyright (C) 2009  Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/tile-manager.h"

#include "gimpgrouplayer.h"
#include "gimpimage.h"
#include "gimpdrawablestack.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_LOCK_CONTENT
};


static void            gimp_group_layer_set_property (GObject        *object,
                                                      guint           property_id,
                                                      const GValue   *value,
                                                      GParamSpec     *pspec);
static void            gimp_group_layer_get_property (GObject        *object,
                                                      guint           property_id,
                                                      GValue         *value,
                                                      GParamSpec     *pspec);
static void            gimp_group_layer_finalize     (GObject        *object);

static gint64          gimp_group_layer_get_memsize  (GimpObject     *object,
                                                      gint64         *gui_size);

static GimpContainer * gimp_group_layer_get_children (GimpViewable   *viewable);

static GimpItem      * gimp_group_layer_duplicate    (GimpItem       *item,
                                                      GType           new_type);

static void            gimp_group_layer_child_add    (GimpContainer  *container,
                                                      GimpLayer      *child,
                                                      GimpGroupLayer *group);
static void            gimp_group_layer_child_remove (GimpContainer  *container,
                                                      GimpLayer      *child,
                                                      GimpGroupLayer *group);
static void            gimp_group_layer_child_move   (GimpLayer      *child,
                                                      GParamSpec     *pspec,
                                                      GimpGroupLayer *group);
static void            gimp_group_layer_child_resize (GimpLayer      *child,
                                                      GimpGroupLayer *group);

static void            gimp_group_layer_update_size  (GimpGroupLayer *layer);


G_DEFINE_TYPE (GimpGroupLayer, gimp_group_layer, GIMP_TYPE_LAYER)

#define parent_class gimp_group_layer_parent_class


static void
gimp_group_layer_class_init (GimpGroupLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);

  object_class->set_property       = gimp_group_layer_set_property;
  object_class->get_property       = gimp_group_layer_get_property;
  object_class->finalize           = gimp_group_layer_finalize;

  gimp_object_class->get_memsize   = gimp_group_layer_get_memsize;

  viewable_class->default_stock_id = "gtk-directory";
  viewable_class->get_children     = gimp_group_layer_get_children;

  item_class->duplicate            = gimp_group_layer_duplicate;

  item_class->default_name         = _("Group Layer");
  item_class->rename_desc          = _("Rename Group Layer");
  item_class->translate_desc       = _("Move Group Layer");
  item_class->scale_desc           = _("Scale Group Layer");
  item_class->resize_desc          = _("Resize Group Layer");
  item_class->flip_desc            = _("Flip Group Layer");
  item_class->rotate_desc          = _("Rotate Group Layer");
  item_class->transform_desc       = _("Transform Group Layer");

  g_object_class_install_property (object_class, PROP_LOCK_CONTENT,
                                   g_param_spec_boolean ("lock-content",
                                                         NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READABLE));
}

static void
gimp_group_layer_init (GimpGroupLayer *layer)
{
  GIMP_ITEM (layer)->lock_content = TRUE;

  layer->children = gimp_drawable_stack_new (GIMP_TYPE_LAYER);

  g_signal_connect (layer->children, "add",
                    G_CALLBACK (gimp_group_layer_child_add),
                    layer);
  g_signal_connect (layer->children, "remove",
                    G_CALLBACK (gimp_group_layer_child_remove),
                    layer);

  gimp_container_add_handler (layer->children, "notify::offset-x",
                              G_CALLBACK (gimp_group_layer_child_move),
                              layer);
  gimp_container_add_handler (layer->children, "notify::offset-y",
                              G_CALLBACK (gimp_group_layer_child_move),
                              layer);
  gimp_container_add_handler (layer->children, "size-changed",
                              G_CALLBACK (gimp_group_layer_child_resize),
                              layer);
}

static void
gimp_group_layer_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_group_layer_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpItem *item = GIMP_ITEM (object);

  switch (property_id)
    {
    case PROP_LOCK_CONTENT:
      g_value_set_boolean (value, gimp_item_get_lock_content (item));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_group_layer_finalize (GObject *object)
{
  GimpGroupLayer *layer = GIMP_GROUP_LAYER (object);

  if (layer->children)
    {
      g_signal_handlers_disconnect_by_func (layer->children,
                                            gimp_group_layer_child_add,
                                            layer);
      g_signal_handlers_disconnect_by_func (layer->children,
                                            gimp_group_layer_child_remove,
                                            layer);

      g_object_unref (layer->children);
      layer->children = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_group_layer_get_memsize (GimpObject *object,
                              gint64     *gui_size)
{
  GimpGroupLayer *layer   = GIMP_GROUP_LAYER (object);
  gint64          memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (layer->children), gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static GimpContainer *
gimp_group_layer_get_children (GimpViewable *viewable)
{
  GimpGroupLayer *layer = GIMP_GROUP_LAYER (viewable);

  return layer->children;
}

static GimpItem *
gimp_group_layer_duplicate (GimpItem *item,
                            GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_GROUP_LAYER), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (GIMP_IS_GROUP_LAYER (new_item))
    {
      GimpGroupLayer *layer     = GIMP_GROUP_LAYER (item);
      GimpGroupLayer *new_layer = GIMP_GROUP_LAYER (new_item);
      GList          *list;
      gint            position;

      for (list = GIMP_LIST (layer->children)->list, position = 0;
           list;
           list = g_list_next (list))
        {
          GimpItem *child = list->data;
          GimpItem *new_child;

          new_child = gimp_item_duplicate (child, G_TYPE_FROM_INSTANCE (child));

          gimp_viewable_set_parent (GIMP_VIEWABLE (new_child),
                                    GIMP_VIEWABLE (new_layer));

          gimp_container_insert (new_layer->children,
                                 GIMP_OBJECT (new_child),
                                 position);
        }
    }

  return new_item;
}

GimpLayer *
gimp_group_layer_new (GimpImage *image)
{
  GimpGroupLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  layer = g_object_new (GIMP_TYPE_GROUP_LAYER, NULL);

  gimp_drawable_configure (GIMP_DRAWABLE (layer),
                           image,
                           0, 0, 1, 1,
                           gimp_image_base_type_with_alpha (image),
                           _("Group Layer"));

  return GIMP_LAYER (layer);
}


/*  private functions  */

static void
gimp_group_layer_child_add (GimpContainer  *container,
                            GimpLayer      *child,
                            GimpGroupLayer *group)
{
  gimp_group_layer_update_size (group);
}

static void
gimp_group_layer_child_remove (GimpContainer  *container,
                               GimpLayer      *child,
                               GimpGroupLayer *group)
{
  gimp_group_layer_update_size (group);
}

static void
gimp_group_layer_child_move (GimpLayer      *child,
                             GParamSpec     *pspec,
                             GimpGroupLayer *group)
{
  gimp_group_layer_update_size (group);
}

static void
gimp_group_layer_child_resize (GimpLayer      *child,
                               GimpGroupLayer *group)
{
  gimp_group_layer_update_size (group);
}

static void
gimp_group_layer_update_size (GimpGroupLayer *layer)
{
  GList   *list;
  gint     x      = 0;
  gint     y      = 0;
  gint     width  = 1;
  gint     height = 1;
  gboolean first  = TRUE;

  for (list = GIMP_LIST (layer->children)->list;
       list;
       list = g_list_next (list))
    {
      GimpItem *child = list->data;

      if (first)
        {
          x      = gimp_item_get_offset_x (child);
          y      = gimp_item_get_offset_y (child);
          width  = gimp_item_get_width    (child);
          height = gimp_item_get_height   (child);

          first = FALSE;
        }
      else
        {
          gimp_rectangle_union (x, y, width, height,
                                gimp_item_get_offset_x (child),
                                gimp_item_get_offset_y (child),
                                gimp_item_get_width    (child),
                                gimp_item_get_height   (child),
                                &x, &y, &width, &height);
        }
    }

  if (x      != gimp_item_get_offset_x (GIMP_ITEM (layer)) ||
      y      != gimp_item_get_offset_y (GIMP_ITEM (layer)) ||
      width  != gimp_item_get_width    (GIMP_ITEM (layer)) ||
      height != gimp_item_get_height   (GIMP_ITEM (layer)))
    {
      if (width  != gimp_item_get_width  (GIMP_ITEM (layer)) ||
          height != gimp_item_get_height (GIMP_ITEM (layer)))
        {
          TileManager *tiles;

          tiles = tile_manager_new (width, height,
                                    gimp_drawable_bytes (GIMP_DRAWABLE (layer)));

          gimp_drawable_set_tiles_full (GIMP_DRAWABLE (layer),
                                        FALSE, NULL,
                                        tiles,
                                        gimp_drawable_type (GIMP_DRAWABLE (layer)),
                                        x, y);
          tile_manager_unref (tiles);
        }
      else
        {
          gimp_item_translate (GIMP_ITEM (layer),
                               x - gimp_item_get_offset_x (GIMP_ITEM (layer)),
                               y - gimp_item_get_offset_y (GIMP_ITEM (layer)),
                               FALSE);
        }
    }
}
