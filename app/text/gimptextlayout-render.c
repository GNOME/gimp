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
#include "gimptext-private.h"
#include "gimptextlayout.h"
#include "gimptextlayout-render.h"


/*  This file duplicates quite a lot of code from pangoft2.c.
 *  At some point all this should be folded back into Pango.
 */

static void  gimp_text_layout_render_line    (GimpTextLayout     *layout,
					      PangoLayoutLine    *line,
					      GimpTextRenderFunc  render_func,
					      gint                x, 
					      gint                y,
					      gpointer            render_data);
static void  gimp_text_layout_render_glyphs  (GimpTextLayout     *layout,
					      PangoFont          *font,
					      PangoGlyphString   *glyphs,
					      GimpTextRenderFunc  render_func,
					      gint                x, 
					      gint                y,
					      gpointer            render_data);
static gint  gimp_text_layout_render_flags   (GimpTextLayout     *layout);



void
gimp_text_layout_render (GimpTextLayout     *layout,
			 GimpTextRenderFunc  render_func,
			 gpointer            render_data)
{
  PangoLayoutIter *iter;
  gint             x, y;

  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (render_func != NULL);

  gimp_text_layout_get_offsets (layout, &x, &y);

  iter = pango_layout_get_iter (layout->layout);

  do
    {
      PangoRectangle   rect;
      PangoLayoutLine *line;
      gint             baseline;
      
      line = pango_layout_iter_get_line (iter);
      
      pango_layout_iter_get_line_extents (iter, NULL, &rect);
      baseline = pango_layout_iter_get_baseline (iter);
      
      gimp_text_layout_render_line (layout, line,
				    render_func,
				    x + PANGO_PIXELS (rect.x),
				    y + PANGO_PIXELS (baseline),
				    render_data);
    }
  while (pango_layout_iter_next_line (iter));
}

static void 
gimp_text_layout_render_line (GimpTextLayout     *layout,
			      PangoLayoutLine    *line,
			      GimpTextRenderFunc  render_func,
			      gint                x, 
			      gint                y,
			      gpointer            render_data)
{
  PangoRectangle  rect;
  GSList         *list;
  gint            x_off = 0;

  for (list = line->runs; list; list = list->next)
    {
      PangoLayoutRun *run = list->data;

      pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				  NULL, &rect);
      gimp_text_layout_render_glyphs (layout,
				      run->item->analysis.font, run->glyphs,
				      render_func,
				      x + PANGO_PIXELS (x_off), y,
				      render_data);

      x_off += rect.width;
    }
}

static void 
gimp_text_layout_render_glyphs (GimpTextLayout     *layout,
				PangoFont          *font,
				PangoGlyphString   *glyphs,
				GimpTextRenderFunc  render_func,
				gint                x, 
				gint                y,
				gpointer            render_data)
{
  PangoGlyphInfo *gi;
  gint            flags;
  gint            i;
  gint            x_position = 0;

  flags = gimp_text_layout_render_flags (layout);

  for (i = 0, gi = glyphs->glyphs; i < glyphs->num_glyphs; i++, gi++)
    {
      if (gi->glyph)
	{
	  render_func (font, gi->glyph, flags,
		       x + PANGO_PIXELS (x_position + gi->geometry.x_offset),
		       y + PANGO_PIXELS (gi->geometry.y_offset),
		       render_data);
	}

      x_position += glyphs->glyphs[i].geometry.width;
    }
}

static gint
gimp_text_layout_render_flags (GimpTextLayout *layout)
{
  GimpText *text  = layout->text;
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
