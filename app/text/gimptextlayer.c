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

static PangoLayout  * gimp_text_layer_layout_new      (GimpTextLayer  *layer);
static gboolean       gimp_text_layer_render          (GimpTextLayer  *layer);
static gboolean       gimp_text_layer_position_layout (GimpTextLayer  *layer,
                                                       PangoLayout    *layout,
                                                       PangoRectangle *pos);
static void           gimp_text_layer_render_layout   (GimpTextLayer  *layer,
                                                       PangoLayout    *layout,
                                                       gint            x,
                                                       gint            y);

static PangoContext * gimp_image_get_pango_context    (GimpImage      *image);


static GimpLayerClass *parent_class = NULL;
static GQuark          gimp_text_layer_context_quark;


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

  gimp_text_layer_context_quark = g_quark_from_static_string ("pango-context");
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
  PangoLayout    *layout;
  PangoRectangle  pos;
  gchar          *name;
  gchar          *newline;

  layout = gimp_text_layer_layout_new (layer);

  if (!gimp_text_layer_position_layout (layer, layout, &pos))
    {
      g_object_unref (layout);
      return FALSE;
    }

  image    = gimp_item_get_image (GIMP_ITEM (layer));
  drawable = GIMP_DRAWABLE (layer);

  if (pos.width  != gimp_drawable_width (drawable) ||
      pos.height != gimp_drawable_height (drawable))
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer),
                            0, 0,
                            gimp_drawable_width (drawable),
                            gimp_drawable_height (drawable));


      drawable->width  = pos.width;
      drawable->height = pos.height;

      if (drawable->tiles)
        tile_manager_destroy (drawable->tiles);

      drawable->tiles = tile_manager_new (drawable->width,
                                          drawable->height,
                                          drawable->bytes);

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

  gimp_text_layer_render_layout (layer, layout, pos.x, pos.y);
  g_object_unref (layout);

  gimp_drawable_update (drawable, 0, 0, pos.width, pos.height);
  gimp_image_flush (image);

  return TRUE;
}

static PangoLayout *
gimp_text_layer_layout_new (GimpTextLayer *layer)
{
  GimpText             *text = layer->text;
  GimpImage            *image;
  PangoContext         *context;
  PangoLayout          *layout;
  PangoFontDescription *font_desc;
  gint                  size;

  font_desc = pango_font_description_from_string (text->font);
  g_return_val_if_fail (font_desc != NULL, NULL);
  if (!font_desc)
    return NULL;

  image = gimp_item_get_image (GIMP_ITEM (layer));

  switch (text->font_size_unit)
    {
    case GIMP_UNIT_PIXEL:
      size = PANGO_SCALE * text->font_size;
      break;

    default:
      {
	gdouble xres, yres;
	gdouble factor;

	factor = gimp_unit_get_factor (text->font_size_unit);
	g_return_val_if_fail (factor > 0.0, NULL);

	gimp_image_get_resolution (image, &xres, &yres);

	size = (gdouble) PANGO_SCALE * text->font_size * yres / factor;
      }
      break;
    }
  
  if (size > 1)
    pango_font_description_set_size (font_desc, size);

  context = gimp_image_get_pango_context (image);

  layout = pango_layout_new (context);
  g_object_unref (context);

  pango_layout_set_font_description (layout, font_desc);
  pango_font_description_free (font_desc);

  pango_layout_set_text (layout, text->text, -1);

  return layout;
}

static gboolean
gimp_text_layer_position_layout (GimpTextLayer  *layer,
                                 PangoLayout    *layout,
                                 PangoRectangle *pos)
{
  GimpText       *text;
  PangoRectangle  ink;
  PangoRectangle  logical;
  gint            x1, y1;
  gint            x2, y2;
  gboolean        fixed;

  text = layer->text;
  fixed = (text->fixed_width > 1 && text->fixed_height > 1);

  pango_layout_get_pixel_extents (layout, &ink, &logical);

  g_print ("ink rect: %d x %d @ %d, %d\n", 
           ink.width, ink.height, ink.x, ink.y);
  g_print ("logical rect: %d x %d @ %d, %d\n", 
           logical.width, logical.height, logical.x, logical.y);

  if (!fixed)
    {
      if (ink.width < 1 || ink.height < 1)
        return FALSE;

      /* sanity checks for insane font sizes */
      if (ink.width  > 8192)  ink.width  = 8192;
      if (ink.height > 8192)  ink.height = 8192;
    }

  x1 = MIN (0, logical.x);
  y1 = MIN (0, logical.y);
  x2 = MAX (ink.x + ink.width,  logical.x + logical.width);
  y2 = MAX (ink.y + ink.height, logical.y + logical.height);

  pos->width  = fixed ? text->fixed_width  : x2 - x1;
  pos->height = fixed ? text->fixed_height : y2 - y1;

  /* border should only be used by the compatibility API;
     we assume that gravity is CENTER
   */
  if (text->border)
    {
      fixed = TRUE;

      pos->width  += 2 * text->border;
      pos->height += 2 * text->border;
    }

  pos->x = - ink.x;
  pos->y = - ink.y;

  if (!fixed)
    return TRUE;

  switch (text->gravity)
    {
    case GIMP_GRAVITY_NORTH_WEST:
    case GIMP_GRAVITY_SOUTH_WEST:
    case GIMP_GRAVITY_WEST:
      break;
      
    case GIMP_GRAVITY_CENTER:
    case GIMP_GRAVITY_NORTH:
    case GIMP_GRAVITY_SOUTH:
      pos->x += (pos->width - logical.width) / 2;
      break;
      
    case GIMP_GRAVITY_NORTH_EAST:
    case GIMP_GRAVITY_SOUTH_EAST:
    case GIMP_GRAVITY_EAST:
      pos->x += (pos->width - logical.width);
      break;
    }

  switch (text->gravity)
    {
    case GIMP_GRAVITY_NORTH:
    case GIMP_GRAVITY_NORTH_WEST:
    case GIMP_GRAVITY_NORTH_EAST:
      break;

    case GIMP_GRAVITY_CENTER:
    case GIMP_GRAVITY_WEST:
    case GIMP_GRAVITY_EAST:
      pos->y += (pos->height - logical.height) / 2;
      break;

    case GIMP_GRAVITY_SOUTH:
    case GIMP_GRAVITY_SOUTH_WEST:
    case GIMP_GRAVITY_SOUTH_EAST:
      pos->y += (pos->height - logical.height);
      break;
    }

  return TRUE;
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


static void
detach_pango_context (GObject *image)
{
  g_object_set_qdata (image, gimp_text_layer_context_quark, NULL);
}

static PangoContext *
gimp_image_get_pango_context (GimpImage *image)
{
  PangoContext *context;

  context = (PangoContext *) g_object_get_qdata (G_OBJECT (image),
                                                 gimp_text_layer_context_quark);

  if (!context)
    {
      gdouble xres, yres;
      
      gimp_image_get_resolution (image, &xres, &yres);

      context = pango_ft2_get_context (xres, yres);

      g_signal_connect_object (image, "resolution_changed",
                               G_CALLBACK (detach_pango_context),
                               context, 0);

      g_object_set_qdata_full (G_OBJECT (image),
                               gimp_text_layer_context_quark, context,
                               (GDestroyNotify) g_object_unref);
    }

  return g_object_ref (context);
}
