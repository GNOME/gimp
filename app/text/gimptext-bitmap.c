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

#define PANGO_ENABLE_ENGINE
#include <cairo.h>
#include <pango/pangocairo.h>

#include "text-types.h"

#include "gimptext-bitmap.h"


/* for compatibility with older freetype versions */
#ifndef FT_LOAD_TARGET_MONO
#define FT_LOAD_TARGET_MONO  FT_LOAD_MONOCHROME
#endif

void
gimp_text_render_bitmap (PangoFont            *font,
                         PangoGlyph            glyph,
                         cairo_font_options_t *options,
                         cairo_matrix_t       *trafo,
                         gint                  x,
                         gint                  y,
                         cairo_t              *cr)
{

  cairo_scaled_font_t *cfont;
  cairo_glyph_t        cglyph;

  cfont = pango_cairo_font_get_scaled_font ( (PangoCairoFont*) font);


  x = PANGO_PIXELS (x);
  y = PANGO_PIXELS (y);

  cglyph.x = x;
  cglyph.y = y;
  cglyph.index = glyph;

  cairo_set_scaled_font (cr, cfont);
  cairo_set_font_options (cr, options);

  cairo_transform (cr, trafo);

  cairo_set_source_rgb (cr, 0.5, 0.5, 0.5);

  cairo_show_glyphs (cr, &cglyph, 1);  

}
