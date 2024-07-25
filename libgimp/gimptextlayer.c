/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimptextlayer.c
 * Copyright (C) 2022 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"


struct _GimpTextLayer
{
  GimpLayer parent_instance;
};


static GimpLayer * gimp_text_layer_copy (GimpLayer *layer);


G_DEFINE_TYPE (GimpTextLayer, gimp_text_layer, GIMP_TYPE_LAYER)

#define parent_class gimp_text_layer_parent_class


static void
gimp_text_layer_class_init (GimpTextLayerClass *klass)
{
  GimpLayerClass *layer_class = GIMP_LAYER_CLASS (klass);

  layer_class->copy = gimp_text_layer_copy;
}

static void
gimp_text_layer_init (GimpTextLayer *layer)
{
}


/* Public API. */

/**
 * gimp_text_layer_get_by_id:
 * @layer_id: The layer id.
 *
 * Returns a #GimpTextLayer representing @layer_id. This function calls
 * gimp_item_get_by_id() and returns the item if it is layer or %NULL
 * otherwise.
 *
 * Returns: (nullable) (transfer none): a #GimpTextLayer for @layer_id or
 *          %NULL if @layer_id does not represent a valid layer. The
 *          object belongs to libgimp and you must not modify or unref
 *          it.
 *
 * Since: 3.0
 **/
GimpTextLayer *
gimp_text_layer_get_by_id (gint32 layer_id)
{
  GimpItem *item = gimp_item_get_by_id (layer_id);

  if (GIMP_IS_TEXT_LAYER (item))
    return (GimpTextLayer *) item;

  return NULL;
}

/**
 * gimp_text_layer_new:
 * @image: The image to which to add the layer.
 * @text:  The text to generate (in UTF-8 encoding).
 * @font:  The name of the font.
 * @size:  The size of text in either pixels or points.
 * @unit:  The units of specified size.
 *
 * Create a new layer.
 *
 * This procedure creates a new text layer displaying the specified @text. By
 * default the width and height of the layer will be determined by the @text
 * contents, the @fontname, @size and @unit.
 *
 * The new layer still needs to be added to the image, as this is not automatic.
 * Add the new layer with the gimp_image_insert_layer() command. Other
 * attributes such as layer mask modes, and offsets should be set with explicit
 * procedure calls.
 *
 * Returns: (transfer none): The newly created text layer.
 *          The object belongs to libgimp and you should not free it.
 *
 * Since: 3.0
 */
GimpTextLayer *
gimp_text_layer_new (GimpImage   *image,
                     const gchar *text,
                     GimpFont    *font,
                     gdouble      size,
                     GimpUnit    *unit)
{
  return _gimp_text_layer_new (image, text, font, size, unit);
}


/*  private functions  */

static GimpLayer *
gimp_text_layer_copy (GimpLayer *layer)
{
  GimpTextLayer *new_layer;
  gchar         *text;
  GimpFont      *font;
  gdouble        size;
  GimpUnit      *unit;

  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (layer), NULL);

  text      = gimp_text_layer_get_text (GIMP_TEXT_LAYER (layer));
  font      = gimp_text_layer_get_font (GIMP_TEXT_LAYER (layer));
  size      = gimp_text_layer_get_font_size (GIMP_TEXT_LAYER (layer), &unit);
  new_layer = gimp_text_layer_new (gimp_item_get_image (GIMP_ITEM (layer)),
                                   text, font, size, unit);
  g_free (text);

  return GIMP_LAYER (new_layer);
}
