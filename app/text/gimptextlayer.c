/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include "text-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "config/gimpconfig.h"

#include "core/gimpimage.h"

#include "gimptext.h"
#include "gimptextlayer.h"
#include "gimptextlayout.h"

#include "gimp-intl.h"


static void       gimp_text_layer_class_init    (GimpTextLayerClass *klass);
static void       gimp_text_layer_init          (GimpTextLayer  *layer);
static void       gimp_text_layer_dispose       (GObject        *object);

static gsize      gimp_text_layer_get_memsize   (GimpObject     *object);
static TempBuf  * gimp_text_layer_get_preview   (GimpViewable   *viewable,
                                                 gint            width,
                                                 gint            height);
static GimpItem * gimp_text_layer_duplicate     (GimpItem       *item,
                                                 GType           new_type,
                                                 gboolean        add_alpha);
static void       gimp_text_layer_rename        (GimpItem       *item,
                                                 const gchar    *new_name,
                                                 const gchar    *undo_desc);

static void       gimp_text_layer_notify_text   (GimpTextLayer  *layer);
static gboolean   gimp_text_layer_idle_render   (GimpTextLayer  *layer);
static gboolean   gimp_text_layer_render        (GimpTextLayer  *layer);
static void       gimp_text_layer_render_layout (GimpTextLayer  *layer,
                                                 GimpTextLayout *layout);


static GimpLayerClass *parent_class = NULL;


GType
gimp_text_layer_get_type (void)
{
  static GType layer_type = 0;

  if (! layer_type)
    {
      static const GTypeInfo layer_info =
      {
        sizeof (GimpTextLayerClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_text_layer_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpTextLayer),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_text_layer_init,
      };

      layer_type = g_type_register_static (GIMP_TYPE_LAYER,
					   "GimpTextLayer",
					   &layer_info, 0);
    }

  return layer_type;
}

static void
gimp_text_layer_class_init (GimpTextLayerClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;
  GimpItemClass     *item_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  item_class        = GIMP_ITEM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->dispose            = gimp_text_layer_dispose;

  gimp_object_class->get_memsize   = gimp_text_layer_get_memsize;

  viewable_class->default_stock_id = "gimp-text-layer";
  viewable_class->get_preview      = gimp_text_layer_get_preview;

  item_class->duplicate            = gimp_text_layer_duplicate;
  item_class->rename               = gimp_text_layer_rename;
  item_class->default_name         = _("Text Layer");
}

static void
gimp_text_layer_init (GimpTextLayer *layer)
{
  layer->text           = NULL;
  layer->idle_render_id = 0;
  layer->auto_rename    = TRUE;
}

static void
gimp_text_layer_dispose (GObject *object)
{
  GimpTextLayer *layer;

  layer = GIMP_TEXT_LAYER (object);

  if (layer->idle_render_id)
    {
      g_source_remove (layer->idle_render_id);
      layer->idle_render_id = 0;
    }
  if (layer->text)
    {
      g_object_unref (layer->text);
      layer->text = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gsize
gimp_text_layer_get_memsize (GimpObject *object)
{
  GimpTextLayer *text_layer;
  gsize          memsize = 0;

  text_layer = GIMP_TEXT_LAYER (object);

  if (text_layer->text)
    {
      memsize += sizeof (GimpText);

      if (text_layer->text->text)
	memsize += strlen (text_layer->text->text);
    }

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
}

static TempBuf *
gimp_text_layer_get_preview (GimpViewable *viewable,
			     gint          width,
			     gint          height)
{
  return GIMP_VIEWABLE_CLASS (parent_class)->get_preview (viewable,
							  width, height);
}

static GimpItem *
gimp_text_layer_duplicate (GimpItem *item,
                           GType     new_type,
                           gboolean  add_alpha)
{
  GimpTextLayer *text_layer;
  GimpItem      *new_item;
  GimpTextLayer *new_text_layer;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type,
                                                        add_alpha);

  if (! GIMP_IS_TEXT_LAYER (new_item))
    return new_item;

  text_layer     = GIMP_TEXT_LAYER (item);
  new_text_layer = GIMP_TEXT_LAYER (new_item);

  new_text_layer->text =
    GIMP_TEXT (gimp_config_duplicate (G_OBJECT (text_layer->text)));

  g_signal_connect_object (new_text_layer->text, "notify",
                           G_CALLBACK (gimp_text_layer_notify_text),
                           new_text_layer, G_CONNECT_SWAPPED);

  return new_item;
}

static void
gimp_text_layer_rename (GimpItem    *item,
                        const gchar *new_name,
                        const gchar *undo_desc)
{
  GIMP_TEXT_LAYER (item)->auto_rename = FALSE;
  
  GIMP_ITEM_CLASS (parent_class)->rename (item, new_name, undo_desc);
}

GimpLayer *
gimp_text_layer_new (GimpImage *image,
		     GimpText  *text)
{
  GimpTextLayer  *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);

  if (!text->text)
    return NULL;

  layer = g_object_new (GIMP_TYPE_TEXT_LAYER, NULL);

  gimp_drawable_configure (GIMP_DRAWABLE (layer),
                           image,
                           0, 0, 0, 0,
                           gimp_image_base_type_with_alpha (image),
                           NULL);

  layer->text = g_object_ref (text);

  if (! gimp_text_layer_render (layer))
    {
      g_object_unref (layer);
      return NULL;
    }

  g_signal_connect_object (text, "notify",
                           G_CALLBACK (gimp_text_layer_notify_text),
                           layer, G_CONNECT_SWAPPED);

  return GIMP_LAYER (layer);
}

GimpText *
gimp_text_layer_get_text (GimpTextLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (layer), NULL);
  
  return layer->text;
}

static void
gimp_text_layer_notify_text (GimpTextLayer *layer)
{
  if (layer->idle_render_id)
    g_source_remove (layer->idle_render_id);

  layer->idle_render_id =
    g_idle_add_full (G_PRIORITY_LOW,
                     (GSourceFunc) gimp_text_layer_idle_render, layer,
                     NULL);
}

static gboolean
gimp_text_layer_idle_render (GimpTextLayer *layer)
{
  layer->idle_render_id = 0;

  gimp_text_layer_render (layer);

  return FALSE;
}

static gboolean
gimp_text_layer_render (GimpTextLayer *layer)
{
  GimpDrawable   *drawable;
  GimpItem       *item;
  GimpImage      *image;
  GimpTextLayout *layout;
  gint            width;
  gint            height;

  drawable = GIMP_DRAWABLE (layer);
  item     = GIMP_ITEM (layer);
  image    = gimp_item_get_image (item);

  layout = gimp_text_layout_new (layer->text, image);

  if (gimp_text_layout_get_size (layout, &width, &height))
    {
      if (width  != gimp_item_width (item) ||
          height != gimp_item_height (item))
        {
          gimp_drawable_update (drawable,
                                0, 0,
                                gimp_item_width (item),
                                gimp_item_height (item));
          
          GIMP_ITEM (drawable)->width  = width;
          GIMP_ITEM (drawable)->height = height;
          
          if (drawable->tiles)
            tile_manager_destroy (drawable->tiles);
          
          drawable->tiles = tile_manager_new (width, height, drawable->bytes);

          gimp_viewable_size_changed (GIMP_VIEWABLE (layer));
        }
    }

  if (layer->auto_rename)
    gimp_object_set_name_safe (GIMP_OBJECT (layer),
                               layer->text->text ?
                               layer->text->text : _("Empty Text Layer"));

  gimp_text_layer_render_layout (layer, layout);
  g_object_unref (layout);

  gimp_image_flush (image);

  return (width > 0 && height > 0);
}

static void
gimp_text_layer_render_layout (GimpTextLayer  *layer,
			       GimpTextLayout *layout)
{
  GimpDrawable *drawable;
  GimpItem     *item;
  TileManager  *mask;
  PixelRegion   textPR;
  PixelRegion   maskPR;
  gint          width;
  gint          height;

  drawable = GIMP_DRAWABLE (layer);
  item     = GIMP_ITEM (layer);

  gimp_drawable_fill (drawable, &layer->text->color);

  width  = gimp_item_width  (item);
  height = gimp_item_height (item);

  mask = gimp_text_layout_render (layout, width, height);

  pixel_region_init (&textPR, drawable->tiles, 0, 0, width, height, TRUE);
  pixel_region_init (&maskPR, mask, 0, 0, width, height, FALSE);

  apply_mask_to_region (&textPR, &maskPR, OPAQUE_OPACITY);
  
  tile_manager_destroy (mask);

  gimp_drawable_update (drawable, 0, 0, width, height);
}
