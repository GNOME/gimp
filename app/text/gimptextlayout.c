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

#include "core/gimpimage.h"

#include "gimptext.h"
#include "gimptextlayout.h"


struct _GimpTextLayout
{
  GObject         object;

  GimpText       *text;
  PangoLayout    *layout;
  PangoRectangle  extents;
};

struct _GimpTextLayoutClass
{
  GObjectClass   parent_class;
};


static void      gimp_text_layout_class_init  (GimpTextLayoutClass *klass);
static void      gimp_text_layout_init        (GimpTextLayout      *layout);
static void      gimp_text_layout_finalize    (GObject             *object);

static void      gimp_text_layout_position    (GimpTextLayout      *layout);

static PangoContext * gimp_image_get_pango_context (GimpImage      *image);


static GQuark         gimp_text_context_quark = 0;
static GObjectClass * parent_class            = NULL;


GType
gimp_text_layout_get_type (void)
{
  static GType layout_type = 0;

  if (! layout_type)
    {
      static const GTypeInfo layout_info =
      {
        sizeof (GimpTextLayoutClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_text_layout_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpTextLayout),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_text_layout_init,
      };

      layout_type = g_type_register_static (G_TYPE_OBJECT,
                                            "GimpTextLayout",
                                            &layout_info, 0);
    }

  return layout_type;
}

static void
gimp_text_layout_class_init (GimpTextLayoutClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_text_layout_finalize;
}

static void
gimp_text_layout_init (GimpTextLayout *layout)
{
  layout->text   = NULL;
  layout->layout = NULL;
}

static void
gimp_text_layout_finalize (GObject *object)
{
  GimpTextLayout *layout;

  layout = GIMP_TEXT_LAYOUT (object);

  if (layout->text)
    {
      g_object_unref (layout->text);
      layout->text = NULL;
    }
  if (layout->layout)
    {
      g_object_unref (layout->layout);
      layout->layout = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


GimpTextLayout *
gimp_text_layout_new (GimpText  *text,
                      GimpImage *image)
{
  GimpTextLayout       *layout;
  PangoContext         *context;
  PangoFontDescription *font_desc;
  gint                  size;

  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  font_desc = pango_font_description_from_string (text->font);
  g_return_val_if_fail (font_desc != NULL, NULL);
  if (!font_desc)
    return NULL;

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

  layout = g_object_new (GIMP_TYPE_TEXT_LAYOUT, NULL);
  layout->text   = g_object_ref (text);
  layout->layout = pango_layout_new (context);

  g_object_unref (context);

  pango_layout_set_font_description (layout->layout, font_desc);
  pango_font_description_free (font_desc);

  pango_layout_set_text (layout->layout, text->text, -1);
  pango_layout_set_alignment (layout->layout, text->alignment);

  gimp_text_layout_position (layout);

  return layout;
}

gboolean
gimp_text_layout_get_size (GimpTextLayout *layout,
                           gint           *width,
                           gint           *height)
{
  g_return_val_if_fail (GIMP_IS_TEXT_LAYOUT (layout), FALSE);

  if (width)
    *width = layout->extents.width;
  if (height)
    *height = layout->extents.height;

  return (layout->extents.width > 0 && layout->extents.height > 0);
}

void
gimp_text_layout_get_offsets (GimpTextLayout *layout,
                              gint           *x,
                              gint           *y)
{
  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));

  if (x)
    *x = layout->extents.x;
  if (y)
    *y = layout->extents.y;
}

TileManager *
gimp_text_layout_render (GimpTextLayout *layout)
{
  TileManager  *mask;
  FT_Bitmap     bitmap;
  PixelRegion   maskPR;
  gint          i;
  gint          x, y;
  gint          width, height;

  g_return_val_if_fail (GIMP_IS_TEXT_LAYOUT (layout), NULL);

  gimp_text_layout_get_size    (layout, &width, &height);
  gimp_text_layout_get_offsets (layout, &x, &y);

  bitmap.width = width;
  bitmap.rows  = height;
  bitmap.pitch = width;
  if (bitmap.pitch & 3)
    bitmap.pitch += 4 - (bitmap.pitch & 3);

  bitmap.buffer = g_malloc0 (bitmap.rows * bitmap.pitch);
      
  pango_ft2_render_layout (&bitmap, layout->layout, x, y);

  mask = tile_manager_new (width, height, 1);
  pixel_region_init (&maskPR, mask, 0, 0, width, height, TRUE);

  for (i = 0; i < height; i++)
    pixel_region_set_row (&maskPR,
			  0, i, width, bitmap.buffer + i * bitmap.pitch);

  g_free (bitmap.buffer);

  return mask;
}

static void
gimp_text_layout_position (GimpTextLayout *layout)
{
  GimpText       *text;
  PangoRectangle  ink;
  PangoRectangle  logical;
  gint            x1, y1;
  gint            x2, y2;
  gboolean        fixed;

  layout->extents.x      = 0;
  layout->extents.x      = 0;
  layout->extents.width  = 0;
  layout->extents.height = 0;
  
  text = layout->text;

  fixed = (text->fixed_width > 1 && text->fixed_height > 1);

  pango_layout_get_pixel_extents (layout->layout, &ink, &logical);

  g_print ("ink rect: %d x %d @ %d, %d\n", 
           ink.width, ink.height, ink.x, ink.y);
  g_print ("logical rect: %d x %d @ %d, %d\n", 
           logical.width, logical.height, logical.x, logical.y);

  if (!fixed)
    {
      if (ink.width < 1 || ink.height < 1)
        return;

      /* sanity checks for insane font sizes */
      if (ink.width  > 8192)  ink.width  = 8192;
      if (ink.height > 8192)  ink.height = 8192;
    }

  x1 = MIN (0, logical.x);
  y1 = MIN (0, logical.y);
  x2 = MAX (ink.x + ink.width,  logical.x + logical.width);
  y2 = MAX (ink.y + ink.height, logical.y + logical.height);

  layout->extents.width  = fixed ? text->fixed_width  : x2 - x1;
  layout->extents.height = fixed ? text->fixed_height : y2 - y1;

  /* border should only be used by the compatibility API;
     we assume that gravity is CENTER
   */
  if (text->border)
    {
      fixed = TRUE;

      layout->extents.width  += 2 * text->border;
      layout->extents.height += 2 * text->border;
    }

  layout->extents.x = 0;
  layout->extents.y = 0;

  if (!fixed)
    return;

  switch (text->gravity)
    {
    case GIMP_GRAVITY_NORTH_WEST:
    case GIMP_GRAVITY_SOUTH_WEST:
    case GIMP_GRAVITY_WEST:
      break;
      
    case GIMP_GRAVITY_NONE:
    case GIMP_GRAVITY_CENTER:
    case GIMP_GRAVITY_NORTH:
    case GIMP_GRAVITY_SOUTH:
      layout->extents.x += (layout->extents.width - logical.width) / 2;
      break;
      
    case GIMP_GRAVITY_NORTH_EAST:
    case GIMP_GRAVITY_SOUTH_EAST:
    case GIMP_GRAVITY_EAST:
      layout->extents.x += (layout->extents.width - logical.width);
      break;
    }

  switch (text->gravity)
    {
    case GIMP_GRAVITY_NORTH:
    case GIMP_GRAVITY_NORTH_WEST:
    case GIMP_GRAVITY_NORTH_EAST:
      break;

    case GIMP_GRAVITY_NONE:
    case GIMP_GRAVITY_CENTER:
    case GIMP_GRAVITY_WEST:
    case GIMP_GRAVITY_EAST:
      layout->extents.y += (layout->extents.height - logical.height) / 2;
      break;

    case GIMP_GRAVITY_SOUTH:
    case GIMP_GRAVITY_SOUTH_WEST:
    case GIMP_GRAVITY_SOUTH_EAST:
      layout->extents.y += (layout->extents.height - logical.height);
      break;
    }
}


static void
detach_pango_context (GObject *image)
{
  g_object_set_qdata (image, gimp_text_context_quark, NULL);
}

static PangoContext *
gimp_image_get_pango_context (GimpImage *image)
{
  PangoContext *context;

  if (!gimp_text_context_quark)
    gimp_text_context_quark = g_quark_from_static_string ("pango-context");

  context = (PangoContext *) g_object_get_qdata (G_OBJECT (image),
                                                 gimp_text_context_quark);

  if (!context)
    {
      gdouble xres, yres;
      
      gimp_image_get_resolution (image, &xres, &yres);

      context = pango_ft2_get_context (xres, yres);

      g_signal_connect_object (image, "resolution_changed",
                               G_CALLBACK (detach_pango_context),
                               context, 0);

      g_object_set_qdata_full (G_OBJECT (image),
                               gimp_text_context_quark, context,
                               (GDestroyNotify) g_object_unref);
    }

  return g_object_ref (context);
}
