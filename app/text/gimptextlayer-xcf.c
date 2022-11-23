/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaText
 * Copyright (C) 2003  Sven Neumann <sven@ligma.org>
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
#include <cairo.h>

#include "libligmabase/ligmabase.h"
#include "text-types.h"

#include "core/ligma.h"
#include "core/ligmadrawable-private.h" /* eek */
#include "core/ligmaimage.h"
#include "core/ligmaparasitelist.h"

#include "ligmatext.h"
#include "ligmatext-parasite.h"
#include "ligmatextlayer.h"
#include "ligmatextlayer-xcf.h"

#include "ligma-intl.h"


enum
{
  TEXT_LAYER_XCF_NONE              = 0,
  TEXT_LAYER_XCF_DONT_AUTO_RENAME  = 1 << 0,
  TEXT_LAYER_XCF_MODIFIED          = 1 << 1
};


static LigmaLayer * ligma_text_layer_from_layer (LigmaLayer *layer,
                                               LigmaText  *text);


gboolean
ligma_text_layer_xcf_load_hack (LigmaLayer **layer)
{
  const gchar        *name;
  LigmaText           *text = NULL;
  const LigmaParasite *parasite;

  g_return_val_if_fail (layer != NULL, FALSE);
  g_return_val_if_fail (LIGMA_IS_LAYER (*layer), FALSE);

  name = ligma_text_parasite_name ();
  parasite = ligma_item_parasite_find (LIGMA_ITEM (*layer), name);

  if (parasite)
    {
      GError *error = NULL;

      text = ligma_text_from_parasite (parasite,
                                      ligma_item_get_image (LIGMA_ITEM (*layer))->ligma,
                                      &error);

      if (error)
        {
          ligma_message (ligma_item_get_image (LIGMA_ITEM (*layer))->ligma, NULL,
                        LIGMA_MESSAGE_ERROR,
                        _("Problems parsing the text parasite for layer '%s':\n"
                          "%s\n\n"
                          "Some text properties may be wrong. "
                          "Unless you want to edit the text layer, "
                          "you don't need to worry about this."),
                        ligma_object_get_name (*layer),
                        error->message);
          g_clear_error (&error);
        }
    }
  else
    {
      name = ligma_text_gdyntext_parasite_name ();

      parasite = ligma_item_parasite_find (LIGMA_ITEM (*layer), name);

      if (parasite)
        text = ligma_text_from_gdyntext_parasite (parasite);
    }

  if (text)
    {
      *layer = ligma_text_layer_from_layer (*layer, text);

      /*  let the text layer knows what parasite was used to create it  */
      LIGMA_TEXT_LAYER (*layer)->text_parasite = name;
    }

  return (text != NULL);
}

void
ligma_text_layer_xcf_save_prepare (LigmaTextLayer *layer)
{
  LigmaText *text;

  g_return_if_fail (LIGMA_IS_TEXT_LAYER (layer));

  /*  If the layer has a text parasite already, it wasn't changed and we
   *  can simply save the original parasite back which is still attached.
   */
  if (layer->text_parasite)
    return;

  text = ligma_text_layer_get_text (layer);
  if (text)
    {
      LigmaParasite *parasite = ligma_text_to_parasite (text);

      /*  Don't push an undo because the parasite only exists temporarily
       *  while the text layer is saved to XCF.
       */
      ligma_item_parasite_attach (LIGMA_ITEM (layer), parasite, FALSE);

      ligma_parasite_free (parasite);
    }
}

guint32
ligma_text_layer_get_xcf_flags (LigmaTextLayer *text_layer)
{
  guint flags = 0;

  g_return_val_if_fail (LIGMA_IS_TEXT_LAYER (text_layer), 0);

  if (! text_layer->auto_rename)
    flags |= TEXT_LAYER_XCF_DONT_AUTO_RENAME;

  if (text_layer->modified)
    flags |= TEXT_LAYER_XCF_MODIFIED;

  return flags;
}

void
ligma_text_layer_set_xcf_flags (LigmaTextLayer *text_layer,
                               guint32        flags)
{
  g_return_if_fail (LIGMA_IS_TEXT_LAYER (text_layer));

  g_object_set (text_layer,
                "auto-rename", (flags & TEXT_LAYER_XCF_DONT_AUTO_RENAME) == 0,
                "modified",    (flags & TEXT_LAYER_XCF_MODIFIED)         != 0,
                NULL);
}


/**
 * ligma_text_layer_from_layer:
 * @layer: a #LigmaLayer object
 * @text: a #LigmaText object
 *
 * Converts a standard #LigmaLayer and a #LigmaText object into a
 * #LigmaTextLayer. The new text layer takes ownership of the @text and
 * @layer objects.  The @layer object is rendered unusable by this
 * function. Don't even try to use if afterwards!
 *
 * This is a gross hack that is needed in order to load text layers
 * from XCF files in a backwards-compatible way. Please don't use it
 * for anything else!
 *
 * Returns: a newly allocated #LigmaTextLayer object
 **/
static LigmaLayer *
ligma_text_layer_from_layer (LigmaLayer *layer,
                            LigmaText  *text)
{
  LigmaTextLayer *text_layer;
  LigmaDrawable  *drawable;

  g_return_val_if_fail (LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (LIGMA_IS_TEXT (text), NULL);

  text_layer = g_object_new (LIGMA_TYPE_TEXT_LAYER,
                             "image", ligma_item_get_image (LIGMA_ITEM (layer)),
                             NULL);

  ligma_item_replace_item (LIGMA_ITEM (text_layer), LIGMA_ITEM (layer));

  drawable = LIGMA_DRAWABLE (text_layer);

  ligma_drawable_steal_buffer (drawable, LIGMA_DRAWABLE (layer));

  ligma_layer_set_opacity         (LIGMA_LAYER (text_layer),
                                  ligma_layer_get_opacity (layer), FALSE);
  ligma_layer_set_mode            (LIGMA_LAYER (text_layer),
                                  ligma_layer_get_mode (layer), FALSE);
  ligma_layer_set_blend_space     (LIGMA_LAYER (text_layer),
                                  ligma_layer_get_blend_space (layer), FALSE);
  ligma_layer_set_composite_space (LIGMA_LAYER (text_layer),
                                  ligma_layer_get_composite_space (layer), FALSE);
  ligma_layer_set_composite_mode  (LIGMA_LAYER (text_layer),
                                  ligma_layer_get_composite_mode (layer), FALSE);
  ligma_layer_set_lock_alpha      (LIGMA_LAYER (text_layer),
                                  ligma_layer_get_lock_alpha (layer), FALSE);

  ligma_text_layer_set_text (text_layer, text);

  g_object_unref (text);
  g_object_unref (layer);

  return LIGMA_LAYER (text_layer);
}
