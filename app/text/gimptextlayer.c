/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextLayer
 * Copyright (C) 2002-2004  Sven Neumann <sven@gimp.org>
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
#include <pango/pangoft2.h>

#include "text-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-params.h"
#include "config/gimpconfig-utils.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpparasitelist.h"

#include "gimptext.h"
#include "gimptext-bitmap.h"
#include "gimptext-private.h"
#include "gimptextlayer.h"
#include "gimptextlayer-transform.h"
#include "gimptextlayout.h"
#include "gimptextlayout-render.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TEXT,
  PROP_AUTO_RENAME,
  PROP_MODIFIED
};

static void       gimp_text_layer_class_init     (GimpTextLayerClass *klass);
static void       gimp_text_layer_init           (GimpTextLayer  *layer);
static void       gimp_text_layer_finalize       (GObject        *object);
static void       gimp_text_layer_get_property   (GObject        *object,
                                                  guint           property_id,
                                                  GValue         *value,
                                                  GParamSpec     *pspec);
static void       gimp_text_layer_set_property   (GObject        *object,
                                                  guint           property_id,
                                                  const GValue   *value,
                                                  GParamSpec     *pspec);

static gint64     gimp_text_layer_get_memsize    (GimpObject     *object,
                                                  gint64         *gui_size);

static GimpItem * gimp_text_layer_duplicate      (GimpItem       *item,
                                                  GType           new_type,
                                                  gboolean        add_alpha);
static gboolean   gimp_text_layer_rename         (GimpItem       *item,
                                                  const gchar    *new_name,
                                                  const gchar    *undo_desc);

static void       gimp_text_layer_apply_region   (GimpDrawable   *drawable,
                                                  PixelRegion    *src2PR,
                                                  gboolean        push_undo,
                                                  const gchar    *undo_desc,
                                                  gdouble         opacity,
                                                  GimpLayerModeEffects  mode,
                                                  TileManager    *src1_tiles,
                                                  gint            x,
                                                  gint            y);
static void       gimp_text_layer_replace_region (GimpDrawable   *drawable,
                                                  PixelRegion    *src2PR,
                                                  gboolean        push_undo,
                                                  const gchar    *undo_desc,
                                                  gdouble         opacity,
                                                  PixelRegion    *maskPR,
                                                  gint            x,
                                                  gint            y);
static void       gimp_text_layer_set_tiles      (GimpDrawable   *drawable,
                                                  gboolean        push_undo,
                                                  const gchar    *undo_desc,
                                                  TileManager    *tiles,
                                                  GimpImageType   type,
                                                  gint            offset_x,
                                                  gint            offset_y);
static void       gimp_text_layer_swap_pixels    (GimpDrawable   *drawable,
                                                  TileManager    *tiles,
                                                  gboolean        sparse,
                                                  gint            x,
                                                  gint            y,
                                                  gint            width,
                                                  gint            height);

static void       gimp_text_layer_text_notify    (GimpTextLayer  *layer);
static gboolean   gimp_text_layer_render         (GimpTextLayer  *layer);
static void       gimp_text_layer_render_layout  (GimpTextLayer  *layer,
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
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize           = gimp_text_layer_finalize;
  object_class->get_property       = gimp_text_layer_get_property;
  object_class->set_property       = gimp_text_layer_set_property;

  gimp_object_class->get_memsize   = gimp_text_layer_get_memsize;

  viewable_class->default_stock_id = "gimp-text-layer";

  item_class->default_name         = _("Text Layer");
  item_class->duplicate            = gimp_text_layer_duplicate;
  item_class->rename               = gimp_text_layer_rename;

#if 0
  item_class->scale                = gimp_text_layer_scale;
  item_class->flip                 = gimp_text_layer_flip;
  item_class->rotate               = gimp_text_layer_rotate;
  item_class->transform            = gimp_text_layer_transform;
#endif

  drawable_class->apply_region     = gimp_text_layer_apply_region;
  drawable_class->replace_region   = gimp_text_layer_replace_region;
  drawable_class->set_tiles        = gimp_text_layer_set_tiles;
  drawable_class->swap_pixels      = gimp_text_layer_swap_pixels;

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_TEXT,
                                   "text", NULL,
                                   GIMP_TYPE_TEXT,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_AUTO_RENAME,
                                    "auto-rename", NULL,
                                    TRUE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_MODIFIED,
                                    "modified", NULL,
                                    FALSE,
                                    0);
}

static void
gimp_text_layer_init (GimpTextLayer *layer)
{
  layer->text          = NULL;
  layer->text_parasite = NULL;
}

static void
gimp_text_layer_finalize (GObject *object)
{
  GimpTextLayer *layer = GIMP_TEXT_LAYER (object);

  if (layer->text)
    {
      g_object_unref (layer->text);
      layer->text = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_text_layer_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  GimpTextLayer *text_layer = GIMP_TEXT_LAYER (object);

  switch (property_id)
    {
    case PROP_TEXT:
      g_value_set_object (value, text_layer->text);
      break;
    case PROP_AUTO_RENAME:
      g_value_set_boolean (value, text_layer->auto_rename);
      break;
    case PROP_MODIFIED:
      g_value_set_boolean (value, text_layer->modified);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_text_layer_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpTextLayer *text_layer = GIMP_TEXT_LAYER (object);

  switch (property_id)
    {
    case PROP_TEXT:
      gimp_text_layer_set_text (text_layer, g_value_get_object (value));
      break;
    case PROP_AUTO_RENAME:
      text_layer->auto_rename = g_value_get_boolean (value);
      break;
    case PROP_MODIFIED:
      text_layer->modified = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_text_layer_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpTextLayer *text_layer;
  gint64         memsize = 0;

  text_layer = GIMP_TEXT_LAYER (object);

  if (text_layer->text)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (text_layer->text),
                                        gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static GimpItem *
gimp_text_layer_duplicate (GimpItem *item,
                           GType     new_type,
                           gboolean  add_alpha)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type,
                                                        add_alpha);

  if (! GIMP_IS_TEXT_LAYER (new_item))
    return new_item;

  gimp_config_sync (GIMP_CONFIG (item), GIMP_CONFIG (new_item), 0);

  return new_item;
}

static gboolean
gimp_text_layer_rename (GimpItem    *item,
                        const gchar *new_name,
                        const gchar *undo_desc)
{
  if (GIMP_ITEM_CLASS (parent_class)->rename (item, new_name, undo_desc))
    {
      g_object_set (item, "auto-rename", FALSE, NULL);

      return TRUE;
    }

  return FALSE;
}

static void
gimp_text_layer_apply_region (GimpDrawable         *drawable,
                              PixelRegion          *src2PR,
                              gboolean              push_undo,
                              const gchar          *undo_desc,
                              gdouble               opacity,
                              GimpLayerModeEffects  mode,
                              TileManager          *src1_tiles,
                              gint                  x,
                              gint                  y)
{
  GIMP_DRAWABLE_CLASS (parent_class)->apply_region (drawable, src2PR,
                                                    push_undo, undo_desc,
                                                    opacity, mode,
                                                    src1_tiles, x, y);
  g_object_set (drawable, "modified", TRUE, NULL);
}

static void
gimp_text_layer_replace_region (GimpDrawable *drawable,
                                PixelRegion  *src2PR,
                                gboolean      push_undo,
                                const gchar  *undo_desc,
                                gdouble       opacity,
                                PixelRegion  *maskPR,
                                gint          x,
                                gint          y)
{
  GIMP_DRAWABLE_CLASS (parent_class)->replace_region (drawable, src2PR,
                                                      push_undo, undo_desc,
                                                      opacity,
                                                      maskPR, x, y);
  g_object_set (drawable, "modified", TRUE, NULL);
}

static void
gimp_text_layer_set_tiles (GimpDrawable  *drawable,
                           gboolean       push_undo,
                           const gchar   *undo_desc,
                           TileManager   *tiles,
                           GimpImageType  type,
                           gint           offset_x,
                           gint           offset_y)
{
  GIMP_DRAWABLE_CLASS (parent_class)->set_tiles (drawable,
                                                 push_undo, undo_desc,
                                                 tiles, type,
                                                 offset_x, offset_y);
  g_object_set (drawable, "modified", TRUE, NULL);
}

static void
gimp_text_layer_swap_pixels (GimpDrawable *drawable,
                             TileManager  *tiles,
                             gboolean      sparse,
                             gint          x,
                             gint          y,
                             gint          width,
                             gint          height)
{
  GIMP_DRAWABLE_CLASS (parent_class)->swap_pixels (drawable, tiles, sparse,
                                                   x, y, width, height);
  g_object_set (drawable, "modified", TRUE, NULL);
}


/**
 * gimp_text_layer_new:
 * @image: the #GimpImage the layer should belong to
 * @text: a #GimpText object
 *
 * Creates a new text layer.
 *
 * Return value: a new #GimpTextLayer or %NULL in case of a problem
 **/
GimpLayer *
gimp_text_layer_new (GimpImage *image,
                     GimpText  *text)
{
  GimpTextLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);

  if (! text->text)
    return NULL;

  layer = g_object_new (GIMP_TYPE_TEXT_LAYER, NULL);

  gimp_drawable_configure (GIMP_DRAWABLE (layer),
                           image,
                           0, 0, 0, 0,
                           gimp_image_base_type_with_alpha (image),
                           NULL);

  gimp_text_layer_set_text (layer, text);

  if (! gimp_text_layer_render (layer))
    {
      g_object_unref (layer);
      return NULL;
    }

  return GIMP_LAYER (layer);
}

void
gimp_text_layer_set_text (GimpTextLayer *layer,
                          GimpText      *text)
{
  g_return_if_fail (GIMP_IS_TEXT_LAYER (layer));
  g_return_if_fail (text == NULL || GIMP_IS_TEXT (text));

  if (layer->text == text)
    return;

  if (layer->text)
    {
      g_signal_handlers_disconnect_by_func (layer->text,
                                            G_CALLBACK (gimp_text_layer_text_notify),
                                            layer);

      g_object_unref (layer->text);
      layer->text = NULL;
    }

  if (text)
    {
      layer->text = g_object_ref (text);

      g_signal_connect_object (text, "notify",
                               G_CALLBACK (gimp_text_layer_text_notify),
                               layer, G_CONNECT_SWAPPED);
    }

  g_object_notify (G_OBJECT (layer), "text");

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));
}

GimpText *
gimp_text_layer_get_text (GimpTextLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (layer), NULL);

  return layer->text;
}

gboolean
gimp_drawable_is_text_layer (GimpDrawable *drawable)
{
  return (GIMP_IS_TEXT_LAYER (drawable)    &&
          GIMP_TEXT_LAYER (drawable)->text &&
          GIMP_TEXT_LAYER (drawable)->modified == FALSE);
}


/**
 * gimp_text_layer_discard:
 * @layer: a #GimpTextLayer
 *
 * Discards the text information. This makes @layer behave like a
 * normal layer.
 */
void
gimp_text_layer_discard (GimpTextLayer *layer)
{
  g_return_if_fail (GIMP_IS_TEXT_LAYER (layer));

  if (! layer->text)
    return;

  gimp_image_undo_push_text_layer (gimp_item_get_image (GIMP_ITEM (layer)),
                                   _("Discard Text Information"),
                                   layer);

  gimp_text_layer_set_text (layer, NULL);
}

static void
gimp_text_layer_text_notify (GimpTextLayer *layer)
{
  /*   If the text layer was created from a parasite, it's time to
   *   remove that parasite now.
   */
  if (layer->text_parasite)
    {
      gimp_parasite_list_remove (GIMP_ITEM (layer)->parasites,
                                 layer->text_parasite);
      layer->text_parasite = NULL;
    }

  gimp_text_layer_render (layer);
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

  if (! layer->text)
    return FALSE;

  drawable = GIMP_DRAWABLE (layer);
  item     = GIMP_ITEM (layer);
  image    = gimp_item_get_image (item);

  if (gimp_container_num_children (image->gimp->fonts) == 0)
    {
      g_message (_("Due to lack of any fonts, "
                   "text functionality is not available."));
      return FALSE;
    }

  layout = gimp_text_layout_new (layer->text, image);

  if (gimp_text_layout_get_size (layout, &width, &height) &&
      (width  != gimp_item_width (item) ||
       height != gimp_item_height (item)))
    {
      TileManager *new_tiles = tile_manager_new (width, height,
                                                 drawable->bytes);

      gimp_drawable_set_tiles (drawable, FALSE, NULL, new_tiles);
      tile_manager_unref (new_tiles);
    }

  if (layer->auto_rename)
    gimp_object_set_name_safe (GIMP_OBJECT (layer),
                               layer->text->text ?
                               layer->text->text : _("Empty Text Layer"));

  gimp_text_layer_render_layout (layer, layout);
  g_object_unref (layout);

  g_object_set (drawable, "modified", FALSE, NULL);

  return (width > 0 && height > 0);
}

static void
gimp_text_layer_render_layout (GimpTextLayer  *layer,
			       GimpTextLayout *layout)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (layer);
  GimpItem     *item     = GIMP_ITEM (layer);
  TileManager  *mask;
  FT_Bitmap     bitmap;
  PixelRegion   textPR;
  PixelRegion   maskPR;
  gint          i;

  gimp_drawable_fill (drawable, &layer->text->color, NULL);

  bitmap.width = gimp_item_width  (item);
  bitmap.rows  = gimp_item_height (item);
  bitmap.pitch = bitmap.width;
  if (bitmap.pitch & 3)
    bitmap.pitch += 4 - (bitmap.pitch & 3);

  bitmap.buffer = g_malloc0 (bitmap.rows * bitmap.pitch);

  gimp_text_layout_render (layout,
			   (GimpTextRenderFunc) gimp_text_render_bitmap,
			   &bitmap);

  mask = tile_manager_new (bitmap.width, bitmap.rows, 1);
  pixel_region_init (&maskPR, mask, 0, 0, bitmap.width, bitmap.rows, TRUE);

  for (i = 0; i < bitmap.rows; i++)
    pixel_region_set_row (&maskPR,
			  0, i, bitmap.width,
			  bitmap.buffer + i * bitmap.pitch);

  g_free (bitmap.buffer);

  pixel_region_init (&textPR, drawable->tiles,
		     0, 0, bitmap.width, bitmap.rows, TRUE);
  pixel_region_init (&maskPR, mask,
		     0, 0, bitmap.width, bitmap.rows, FALSE);

  apply_mask_to_region (&textPR, &maskPR, OPAQUE_OPACITY);

  tile_manager_unref (mask);

  /*  no need to gimp_drawable_update() since gimp_drawable_fill()
   *  did that for us.
   */
}
