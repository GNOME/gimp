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
#include <pango/pangoft2.h>

#include "text-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpimage.h"

#include "gimptext.h"
#include "gimptextlayer.h"
#include "gimptextlayout.h"


static void      gimp_text_layer_class_init    (GimpTextLayerClass *klass);
static void      gimp_text_layer_init          (GimpTextLayer      *layer);
static void      gimp_text_layer_finalize      (GObject            *object);

static gsize     gimp_text_layer_get_memsize   (GimpObject         *object);
static TempBuf * gimp_text_layer_get_preview   (GimpViewable       *viewable,
						gint                width,
						gint                height);

static gboolean  gimp_text_layer_render        (GimpTextLayer      *layer);
static void      gimp_text_layer_render_layout (GimpTextLayer      *layer,
                                                GimpTextLayout     *layout);


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

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_text_layer_finalize;

  gimp_object_class->get_memsize = gimp_text_layer_get_memsize;

  viewable_class->get_preview = gimp_text_layer_get_preview;
}

static void
gimp_text_layer_init (GimpTextLayer *layer)
{
  layer->text = NULL;
}

static void
gimp_text_layer_finalize (GObject *object)
{
  GimpTextLayer *layer;

  layer = GIMP_TEXT_LAYER (object);

  if (layer->text)
    {
      g_object_unref (layer->text);
      layer->text = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
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

  g_signal_connect_object (text, "notify",
                           G_CALLBACK (gimp_text_layer_render),
                           layer, G_CONNECT_SWAPPED);

  if (!gimp_text_layer_render (layer))
    {
      g_object_unref (layer);
      return NULL;
    }

  return GIMP_LAYER (layer);
}

GimpText *
gimp_text_layer_get_text (GimpTextLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (layer), NULL);
  
  return layer->text;
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

static gboolean
gimp_text_layer_render (GimpTextLayer *layer)
{
  GimpImage      *image;
  GimpDrawable   *drawable;
  GimpTextLayout *layout;
  gint            width;
  gint            height;
  gchar          *name;
  gchar          *newline;

  image = gimp_item_get_image (GIMP_ITEM (layer));

  layout = gimp_text_layout_new (layer->text, image);

  gimp_text_layout_get_size (layout, &width, &height);

  if (! gimp_text_layout_get_size (layout, &width, &height))
    {
      g_object_unref (layout);
      return FALSE;
    }

  drawable = GIMP_DRAWABLE (layer);

  if (width  != gimp_drawable_width (drawable) ||
      height != gimp_drawable_height (drawable))
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            0, 0,
                            gimp_drawable_width (drawable),
                            gimp_drawable_height (drawable));


      drawable->width  = width;
      drawable->height = height;

      if (drawable->tiles)
        tile_manager_destroy (drawable->tiles);

      drawable->tiles = tile_manager_new (width, height, drawable->bytes);

      gimp_viewable_size_changed (GIMP_VIEWABLE (layer));
    }

  newline = strchr (layer->text->text, '\n');
  if (newline)
    {
      name = g_strndup (layer->text->text, newline - layer->text->text);
      gimp_object_set_name (GIMP_OBJECT (layer), name);
      g_free (name);
    }
  else
  {
    gimp_object_set_name (GIMP_OBJECT (layer), layer->text->text);
  }

  gimp_text_layer_render_layout (layer, layout);
  g_object_unref (layout);

  gimp_drawable_update (drawable, 0, 0, width, height);
  gimp_image_flush (image);

  return TRUE;
}

static void
gimp_text_layer_render_layout (GimpTextLayer  *layer,
			       GimpTextLayout *layout)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (layer);
  TileManager  *mask;
  PixelRegion   textPR;
  PixelRegion   maskPR;
  gint          width;
  gint          height;
 
  gimp_drawable_fill (drawable, &layer->text->color);

  width  = gimp_drawable_width  (drawable);
  height = gimp_drawable_height (drawable);

  mask = gimp_text_layout_render (layout);

  pixel_region_init (&textPR,
		     GIMP_DRAWABLE (layer)->tiles, 0, 0, width, height, TRUE);
  pixel_region_init (&maskPR,
		     mask, 0, 0, width, height, FALSE);

  apply_mask_to_region (&textPR, &maskPR, OPAQUE_OPACITY);
  
  tile_manager_destroy (mask);
}
