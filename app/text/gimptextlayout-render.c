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

#include "gimptext.h"
#include "gimptextlayout.h"
#include "gimptextlayout-private.h"
#include "gimptextlayout-render.h"


/*  Define to 1 to use the native PangoFT2 render routine.  */
#define USE_PANGOFT2_RENDER 0


/*  This file duplicates quite a lot of code from pangoft2.c.
 *  At some point all this should be folded back into Pango.
 */

static void  gimp_text_render_layout         (GimpTextLayout   *text_layout,
					      FT_Bitmap        *bitmap,
					      gint              x, 
					      gint              y);
static void  gimp_text_render_layout_line    (GimpTextLayout   *text_layout,
					      FT_Bitmap        *bitmap,
					      PangoLayoutLine  *line,
					      gint              x, 
					      gint              y);
static void  gimp_text_render                (GimpTextLayout   *text_layout,
					      FT_Bitmap        *bitmap,
					      PangoFont        *font,
					      PangoGlyphString *glyphs,
					      gint              x, 
					      gint              y);
static gint  gimp_text_layout_get_load_flags (GimpTextLayout   *text_layout);


TileManager *
gimp_text_layout_render (GimpTextLayout *layout,
                         gint            width,
                         gint            height)
{
  TileManager  *mask;
  FT_Bitmap     bitmap;
  PixelRegion   maskPR;
  gint          i;
  gint          x, y;

  g_return_val_if_fail (GIMP_IS_TEXT_LAYOUT (layout), NULL);

  gimp_text_layout_get_offsets (layout, &x, &y);

  bitmap.width = width;
  bitmap.rows  = height;
  bitmap.pitch = width;
  if (bitmap.pitch & 3)
    bitmap.pitch += 4 - (bitmap.pitch & 3);

  bitmap.buffer = g_malloc0 (bitmap.rows * bitmap.pitch);

#if USE_PANGOFT2_RENDER
  pango_ft2_render_layout (&bitmap, layout->layout, x, y);
#else
  gimp_text_render_layout (layout, &bitmap, x, y);
#endif

  mask = tile_manager_new (width, height, 1);
  pixel_region_init (&maskPR, mask, 0, 0, width, height, TRUE);

  for (i = 0; i < height; i++)
    pixel_region_set_row (&maskPR,
			  0, i, width, bitmap.buffer + i * bitmap.pitch);

  g_free (bitmap.buffer);

  return mask;
}

static void
gimp_text_render_layout (GimpTextLayout *text_layout,
			 FT_Bitmap      *bitmap,
			 gint            x, 
			 gint            y)
{
  PangoLayout     *layout = text_layout->layout;
  PangoLayoutIter *iter;

  g_return_if_fail (bitmap != NULL);
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  iter = pango_layout_get_iter (layout);

  do
    {
      PangoRectangle   rect;
      PangoLayoutLine *line;
      gint             baseline;
      
      line = pango_layout_iter_get_line (iter);
      
      pango_layout_iter_get_line_extents (iter, NULL, &rect);
      baseline = pango_layout_iter_get_baseline (iter);
      
      gimp_text_render_layout_line (text_layout,
				    bitmap,
				    line,
				    x + PANGO_PIXELS (rect.x),
				    y + PANGO_PIXELS (baseline));
    }
  while (pango_layout_iter_next_line (iter));

  pango_layout_iter_free (iter);
}

static void 
gimp_text_render_layout_line (GimpTextLayout  *text_layout,
			      FT_Bitmap       *bitmap,
			      PangoLayoutLine *line,
			      gint             x, 
			      gint             y)
{
  PangoRectangle  rect;
  GSList         *list;
  gint            x_off = 0;

  for (list = line->runs; list; list = list->next)
    {
      PangoLayoutRun *run = list->data;

      pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				  NULL, &rect);
      gimp_text_render (text_layout, bitmap,
			run->item->analysis.font, run->glyphs,
			x + PANGO_PIXELS (x_off), y);

      x_off += rect.width;
    }
}

static void 
gimp_text_render (GimpTextLayout   *text_layout,
		  FT_Bitmap        *bitmap,
		  PangoFont        *font,
		  PangoGlyphString *glyphs,
		  gint              x, 
		  gint              y)
{
  PangoGlyphInfo *gi;
  FT_Face         face;
  gint            flags;
  gint            i;
  gint            x_position = 0;
  gint            ix, iy, ixoff, iyoff, y_start, y_limit, x_start, x_limit;
  const guchar   *src;
  guchar         *dest;

  face = pango_ft2_font_get_face (font);
  flags = gimp_text_layout_get_load_flags (text_layout);

  for (i = 0, gi = glyphs->glyphs; i < glyphs->num_glyphs; i++, gi++)
    {
      if (gi->glyph)
	{
	  FT_Load_Glyph (face, (FT_UInt) gi->glyph, flags);
	  FT_Render_Glyph (face->glyph,
			   (flags & FT_LOAD_TARGET_MONO ?
			    ft_render_mode_mono : ft_render_mode_normal));
	  
	  ixoff = x + PANGO_PIXELS (x_position + gi->geometry.x_offset);
	  iyoff = y + PANGO_PIXELS (gi->geometry.y_offset);
	  
	  x_start = MAX (0, - (ixoff + face->glyph->bitmap_left));
	  x_limit = MIN (face->glyph->bitmap.width,
			 bitmap->width - (ixoff + face->glyph->bitmap_left));

	  y_start = MAX (0,  - (iyoff - face->glyph->bitmap_top));
	  y_limit = MIN (face->glyph->bitmap.rows,
			 bitmap->rows - (iyoff - face->glyph->bitmap_top));

	  src = face->glyph->bitmap.buffer +
	    y_start * face->glyph->bitmap.pitch;

	  dest = bitmap->buffer +
	    (y_start + iyoff - face->glyph->bitmap_top) * bitmap->pitch +
	    x_start + ixoff + face->glyph->bitmap_left;

	  switch (face->glyph->bitmap.pixel_mode)
	    {
	    case ft_pixel_mode_grays:
	      src += x_start;
	      for (iy = y_start; iy < y_limit; iy++)
		{
		  const guchar *s = src;
		  guchar       *d = dest;

		  for (ix = x_start; ix < x_limit; ix++)
		    {
		      switch (*s)
			{
			case 0:
			  break;
			case 0xff:
			  *d = 0xff;
			default:
			  *d = MIN ((gushort) *d + (const gushort) *s, 0xff);
			  break;
			}

		      s++;
		      d++;
		    }

		  dest += bitmap->pitch;
		  src  += face->glyph->bitmap.pitch;
		}
	      break;

	    case ft_pixel_mode_mono:
	      src += x_start / 8;
	      for (iy = y_start; iy < y_limit; iy++)
		{
		  const guchar *s = src;
		  guchar       *d = dest;

		  for (ix = x_start; ix < x_limit; ix++)
		    {
		      if ((*s) & (1 << (7 - (ix % 8))))
			*d |= 0xff;
		      
		      if ((ix % 8) == 7)
			s++;
		      d++;
		    }

		  dest += bitmap->pitch;
		  src  += face->glyph->bitmap.pitch;
		}
	      break;

	    default:
	      break;
	    }
	}

      x_position += glyphs->glyphs[i].geometry.width;
    }
}

static gint
gimp_text_layout_get_load_flags (GimpTextLayout *text_layout)
{
  GimpText *text  = text_layout->text;
  gint      flags;

  if (text->antialias)
    flags = FT_LOAD_NO_BITMAP;
  else
    flags = FT_LOAD_TARGET_MONO;

  if (!text->hinting)
   flags |= FT_LOAD_NO_HINTING;

  if (text->autohint)
    flags |= FT_LOAD_FORCE_AUTOHINT;

  return flags;
}
