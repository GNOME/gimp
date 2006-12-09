/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#include "text-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpparasitelist.h"

#include "gimptext.h"
#include "gimptext-parasite.h"
#include "gimptextlayer.h"
#include "gimptextlayer-xcf.h"

#include "gimp-intl.h"


enum
{
  TEXT_LAYER_XCF_NONE              = 0,
  TEXT_LAYER_XCF_DONT_AUTO_RENAME  = 1 << 0,
  TEXT_LAYER_XCF_MODIFIED          = 1 << 1
};


static GimpLayer * gimp_text_layer_from_layer (GimpLayer *layer,
                                               GimpText  *text);


gboolean
gimp_text_layer_xcf_load_hack (GimpLayer **layer)
{
  const gchar        *name;
  GimpText           *text = NULL;
  const GimpParasite *parasite;

  g_return_val_if_fail (layer != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (*layer), FALSE);

  name = gimp_text_parasite_name ();
  parasite = gimp_item_parasite_find (GIMP_ITEM (*layer), name);

  if (parasite)
    {
      GError *error = NULL;

      text = gimp_text_from_parasite (parasite, &error);

      if (error)
        {
          gimp_message (gimp_item_get_image (GIMP_ITEM (*layer))->gimp, NULL,
                        GIMP_MESSAGE_ERROR,
                        _("Problems parsing the text parasite for layer '%s':\n"
                          "%s\n\n"
                          "Some text properties may be wrong. "
                          "Unless you want to edit the text layer, "
                          "you don't need to worry about this."),
                        gimp_object_get_name (GIMP_OBJECT (*layer)),
                        error->message);
          g_clear_error (&error);
        }
    }
  else
    {
      name = gimp_text_gdyntext_parasite_name ();

      parasite = gimp_item_parasite_find (GIMP_ITEM (*layer), name);

      if (parasite)
        text = gimp_text_from_gdyntext_parasite (parasite);
    }

  if (text)
    {
      *layer = gimp_text_layer_from_layer (*layer, text);

      /*  let the text layer know what parasite was used to create it  */
      GIMP_TEXT_LAYER (*layer)->text_parasite = name;
    }

  return (text != NULL);
}

void
gimp_text_layer_xcf_save_prepare (GimpTextLayer *layer)
{
  GimpText *text;

  g_return_if_fail (GIMP_IS_TEXT_LAYER (layer));

  /*  If the layer has a text parasite already, it wasn't changed and we
   *  can simply save the original parasite back which is still attached.
   */
  if (layer->text_parasite)
    return;

  text = gimp_text_layer_get_text (layer);
  if (text)
    {
      GimpParasite *parasite = gimp_text_to_parasite (text);

      gimp_parasite_list_add (GIMP_ITEM (layer)->parasites, parasite);
    }
}

guint32
gimp_text_layer_get_xcf_flags (GimpTextLayer *text_layer)
{
  guint flags = 0;

  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (text_layer), 0);

  if (! text_layer->auto_rename)
    flags |= TEXT_LAYER_XCF_DONT_AUTO_RENAME;

  if (text_layer->modified)
    flags |= TEXT_LAYER_XCF_MODIFIED;

  return flags;
}

void
gimp_text_layer_set_xcf_flags (GimpTextLayer *text_layer,
                               guint32        flags)
{
  g_return_if_fail (GIMP_IS_TEXT_LAYER (text_layer));

  g_object_set (text_layer,
                "auto-rename", (flags & TEXT_LAYER_XCF_DONT_AUTO_RENAME) == 0,
                "modified",    (flags & TEXT_LAYER_XCF_MODIFIED)         != 0,
                NULL);
}


/**
 * gimp_text_layer_from_layer:
 * @layer: a #GimpLayer object
 * @text: a #GimpText object
 *
 * Converts a standard #GimpLayer and a #GimpText object into a
 * #GimpTextLayer. The new text layer takes ownership of the @text and
 * @layer objects.  The @layer object is rendered unusable by this
 * function. Don't even try to use if afterwards!
 *
 * This is a gross hack that is needed in order to load text layers
 * from XCF files in a backwards-compatible way. Please don't use it
 * for anything else!
 *
 * Return value: a newly allocated #GimpTextLayer object
 **/
static GimpLayer *
gimp_text_layer_from_layer (GimpLayer *layer,
                            GimpText  *text)
{
  GimpTextLayer *text_layer;
  GimpItem      *item;
  GimpDrawable  *drawable;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);

  text_layer = g_object_new (GIMP_TYPE_TEXT_LAYER, NULL);

  item     = GIMP_ITEM (text_layer);
  drawable = GIMP_DRAWABLE (text_layer);

  gimp_object_set_name (GIMP_OBJECT (text_layer),
                        gimp_object_get_name (GIMP_OBJECT (layer)));

  item->ID        = gimp_item_get_ID (GIMP_ITEM (layer));
  item->tattoo    = gimp_item_get_tattoo (GIMP_ITEM (layer));
  item->image     = gimp_item_get_image (GIMP_ITEM (layer));

  gimp_item_set_image (GIMP_ITEM (layer), NULL);
  g_hash_table_replace (item->image->gimp->item_table,
                        GINT_TO_POINTER (item->ID),
                        item);

  item->parasites = GIMP_ITEM (layer)->parasites;
  GIMP_ITEM (layer)->parasites = NULL;

  item->width     = gimp_item_width (GIMP_ITEM (layer));
  item->height    = gimp_item_height (GIMP_ITEM (layer));

  gimp_item_offsets (GIMP_ITEM (layer), &item->offset_x, &item->offset_y);

  item->visible   = gimp_item_get_visible (GIMP_ITEM (layer));
  item->linked    = gimp_item_get_linked (GIMP_ITEM (layer));

  drawable->tiles     = GIMP_DRAWABLE (layer)->tiles;
  GIMP_DRAWABLE (layer)->tiles = NULL;

  drawable->bytes     = gimp_drawable_bytes (GIMP_DRAWABLE (layer));
  drawable->type      = gimp_drawable_type (GIMP_DRAWABLE (layer));
  drawable->has_alpha = gimp_drawable_has_alpha (GIMP_DRAWABLE (layer));

  GIMP_LAYER (text_layer)->opacity    = gimp_layer_get_opacity (layer);
  GIMP_LAYER (text_layer)->mode       = gimp_layer_get_mode (layer);
  GIMP_LAYER (text_layer)->lock_alpha = gimp_layer_get_lock_alpha (layer);

  gimp_text_layer_set_text (text_layer, text);

  g_object_unref (text);
  g_object_unref (layer);

  return GIMP_LAYER (text_layer);
}
