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

#include <glib-object.h>
#include <pango/pangoft2.h>

#include "libgimpbase/gimpbase.h"

#include "text-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpimage.h"

#include "gimptext.h"
#include "gimptext-render.h"
#include "gimptextlayer.h"


static void      gimp_text_layer_class_init    (GimpTextLayerClass *klass);
static void      gimp_text_layer_init          (GimpTextLayer      *layer);
static void      gimp_text_layer_finalize      (GObject            *object);

static gsize     gimp_text_layer_get_memsize   (GimpObject         *object);

static TempBuf * gimp_text_layer_get_preview   (GimpViewable       *viewable,
						gint                width,
						gint                height);

static void          gimp_text_layer_ensure_context (GimpTextLayer *layer);
static PangoLayout * gimp_text_layer_layout_new     (GimpTextLayer *layer,
						     gint          *border);
static void          gimp_text_layer_render_layout  (GimpTextLayer *layer,
						     PangoLayout   *layout,
						     gint           x,
						     gint           y);


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
  layer->text    = NULL;
  layer->context = NULL;
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
  if (layer->context)
    {
      g_object_unref (layer->context);
      layer->context = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpLayer *
gimp_text_layer_new (GimpImage *gimage,
		     GimpText  *text)
{
  GimpTextLayer  *layer;
  PangoLayout    *layout;
  PangoRectangle  ink;
  PangoRectangle  logical;
  gint            border;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);

  if (!text->text)
    return NULL;

  layer = g_object_new (GIMP_TYPE_TEXT_LAYER, NULL);

  layer->text = g_object_ref (text);
  gimp_item_set_image (GIMP_ITEM (layer), gimage);

  layout = gimp_text_layer_layout_new (layer, &border);

  pango_layout_get_pixel_extents (layout, &ink, &logical);

  g_print ("ink rect: %d x %d @ %d, %d\n", 
           ink.width, ink.height, ink.x, ink.y);
  g_print ("logical rect: %d x %d @ %d, %d\n", 
           logical.width, logical.height, logical.x, logical.y);

  if (ink.width < 1 || ink.height < 1)
    {
      g_object_unref (layout);
      g_object_unref (layer);

      return NULL;
    }

  if (ink.width  > 8192)  ink.width  = 8192;
  if (ink.height > 8192)  ink.height = 8192;

  gimp_drawable_configure (GIMP_DRAWABLE (layer),
			   gimage,
                           ink.width  + 2 * border,
			   ink.height + 2 * border,
                           gimp_image_base_type_with_alpha (gimage),
                           text->text  /* name */);
  
  gimp_text_layer_render_layout (layer, layout,
				 border - ink.x, border - ink.y);
  g_object_unref (layout);

  return GIMP_LAYER (layer);
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

static void
gimp_text_layer_ensure_context (GimpTextLayer *layer)
{
  GimpImage *image;
  gdouble    xres, yres;

  if (layer->context)
    return;

  image = gimp_item_get_image (GIMP_ITEM (layer));
  gimp_image_get_resolution (image, &xres, &yres);

  layer->context = pango_ft2_get_context (xres, yres);

  g_signal_connect_object (image, "resolution_changed",
			   G_CALLBACK (g_object_unref),
			   layer->context, G_CONNECT_SWAPPED);

  g_object_add_weak_pointer (G_OBJECT (layer->context),
			     (gpointer *) &layer->context);
}

static PangoLayout *
gimp_text_layer_layout_new (GimpTextLayer *layer,
			    gint          *border)
{
  GimpText             *text = layer->text;
  PangoLayout          *layout;
  PangoFontDescription *font_desc;
  gint                  size;

  font_desc = pango_font_description_from_string (text->font);
  g_return_val_if_fail (font_desc != NULL, NULL);
  if (!font_desc)
    return NULL;

  switch (text->unit)
    {
    case GIMP_UNIT_PIXEL:
      size    = PANGO_SCALE * text->size;
      *border = text->border;
      break;

    default:
      {
	GimpImage *image;
	gdouble    xres, yres;
	gdouble    factor;

	factor = gimp_unit_get_factor (text->unit);
	g_return_val_if_fail (factor > 0.0, NULL);

	image = gimp_item_get_image (GIMP_ITEM (layer));
	gimp_image_get_resolution (image, &xres, &yres);

	size    = (gdouble) PANGO_SCALE * text->size * yres / factor;
	*border = text->border * yres / factor;
      }
      break;
    }
  
  if (size > 1)
    pango_font_description_set_size (font_desc, size);

  gimp_text_layer_ensure_context (layer);

  layout = pango_layout_new (layer->context);

  pango_layout_set_font_description (layout, font_desc);
  pango_font_description_free (font_desc);

  pango_layout_set_text (layout, text->text, -1);

  return layout;
}

static void
gimp_text_layer_render_layout (GimpTextLayer *layer,
			       PangoLayout   *layout,
			       gint           x,
			       gint           y)
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

  mask = gimp_text_render_layout (layout, x, y, width, height);

  pixel_region_init (&textPR,
		     GIMP_DRAWABLE (layer)->tiles, 0, 0, width, height, TRUE);
  pixel_region_init (&maskPR,
		     mask, 0, 0, width, height, FALSE);

  apply_mask_to_region (&textPR, &maskPR, OPAQUE_OPACITY);
  
  tile_manager_destroy (mask);
}


