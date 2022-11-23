/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmatextlayer.c
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

#include "ligma.h"


struct _LigmaTextLayer
{
  LigmaLayer parent_instance;
};


static LigmaLayer * ligma_text_layer_copy (LigmaLayer *layer);


G_DEFINE_TYPE (LigmaTextLayer, ligma_text_layer, LIGMA_TYPE_LAYER)

#define parent_class ligma_text_layer_parent_class


static void
ligma_text_layer_class_init (LigmaTextLayerClass *klass)
{
  LigmaLayerClass *layer_class = LIGMA_LAYER_CLASS (klass);

  layer_class->copy = ligma_text_layer_copy;
}

static void
ligma_text_layer_init (LigmaTextLayer *layer)
{
}


/* Public API. */

/**
 * ligma_text_layer_get_by_id:
 * @layer_id: The layer id.
 *
 * Returns a #LigmaTextLayer representing @layer_id. This function calls
 * ligma_item_get_by_id() and returns the item if it is layer or %NULL
 * otherwise.
 *
 * Returns: (nullable) (transfer none): a #LigmaTextLayer for @layer_id or
 *          %NULL if @layer_id does not represent a valid layer. The
 *          object belongs to libligma and you must not modify or unref
 *          it.
 *
 * Since: 3.0
 **/
LigmaTextLayer *
ligma_text_layer_get_by_id (gint32 layer_id)
{
  LigmaItem *item = ligma_item_get_by_id (layer_id);

  if (LIGMA_IS_TEXT_LAYER (item))
    return (LigmaTextLayer *) item;

  return NULL;
}

/**
 * ligma_text_layer_new:
 * @image:    The image to which to add the layer.
 * @text:     The text to generate (in UTF-8 encoding).
 * @fontname: The name of the font.
 * @size:     The size of text in either pixels or points.
 * @unit:     The units of specified size.
 *
 * Create a new layer.
 *
 * This procedure creates a new text layer displaying the specified @text. By
 * default the width and height of the layer will be determined by the @text
 * contents, the @fontname, @size and @unit.
 *
 * The new layer still needs to be added to the image, as this is not automatic.
 * Add the new layer with the ligma_image_insert_layer() command. Other
 * attributes such as layer mask modes, and offsets should be set with explicit
 * procedure calls.
 *
 * Returns: (transfer none): The newly created text layer.
 *          The object belongs to libligma and you should not free it.
 *
 * Since: 3.0
 */
LigmaTextLayer *
ligma_text_layer_new (LigmaImage   *image,
                     const gchar *text,
                     const gchar *fontname,
                     gdouble      size,
                     LigmaUnit     unit)
{
  return _ligma_text_layer_new (image, text, fontname, size, unit);
}


/*  private functions  */

static LigmaLayer *
ligma_text_layer_copy (LigmaLayer *layer)
{
  LigmaTextLayer *new_layer;
  gchar         *text;
  gchar         *fontname;
  gdouble        size;
  LigmaUnit       unit;

  g_return_val_if_fail (LIGMA_IS_TEXT_LAYER (layer), NULL);

  text      = ligma_text_layer_get_text (LIGMA_TEXT_LAYER (layer));
  fontname  = ligma_text_layer_get_font (LIGMA_TEXT_LAYER (layer));
  size      = ligma_text_layer_get_font_size (LIGMA_TEXT_LAYER (layer), &unit);
  new_layer = ligma_text_layer_new (ligma_item_get_image (LIGMA_ITEM (layer)),
                                   text, fontname, size, unit);
  g_free (text);
  g_free (fontname);

  return LIGMA_LAYER (new_layer);
}
