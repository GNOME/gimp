/* GIMP - The GNU Image Manipulation Program
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

#define PANGO_ENABLE_ENGINE 1

#include "config.h"

#include <glib-object.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <pango/pango-font.h>

#include "text-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "gimptext.h"
#include "gimptext-private.h"
#include "gimptextlayout.h"
#include "gimptextlayout-render.h"


/* for compatibility with older freetype versions */
#ifndef FT_LOAD_TARGET_MONO
#define FT_LOAD_TARGET_MONO  FT_LOAD_MONOCHROME
#endif


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
static cairo_font_options_t *
             gimp_text_layout_render_flags   (GimpTextLayout     *layout);
static void  gimp_text_layout_render_trafo   (GimpTextLayout     *layout,
                                              cairo_matrix_t     *trafo);



void
gimp_text_layout_render (GimpTextLayout     *layout,
                         GimpTextRenderFunc  render_func,
                         gpointer            render_data)
{
  PangoLayoutIter *iter;
  PangoRectangle   rect;
  gint             x, y;

  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (render_func != NULL);

  gimp_text_layout_get_offsets (layout, &x, &y);

  x *= PANGO_SCALE;
  y *= PANGO_SCALE;

  pango_layout_get_extents (layout->layout, NULL, &rect);

  /* If the width of the layout is > 0, then the text-box is FIXED
   * and the layout position should be offset if the alignment
   * is centered or right-aligned*/
  if (pango_layout_get_width (layout->layout) > 0)
    switch (pango_layout_get_alignment (layout->layout))
      {
      case PANGO_ALIGN_LEFT:
        break;

      case PANGO_ALIGN_RIGHT:
        x += pango_layout_get_width (layout->layout) - rect.width;
        break;

      case PANGO_ALIGN_CENTER:
        x += (pango_layout_get_width (layout->layout) - rect.width) / 2;
        break;
      }

  iter = pango_layout_get_iter (layout->layout);

  do
    {
      PangoLayoutLine *line;
      gint             baseline;

      line = pango_layout_iter_get_line_readonly (iter);

      pango_layout_iter_get_line_extents (iter, NULL, &rect);
      baseline = pango_layout_iter_get_baseline (iter);

      gimp_text_layout_render_line (layout, line,
                                    render_func,
                                    x + rect.x,
                                    y + baseline,
                                    render_data);
    }
  while (pango_layout_iter_next_line (iter));

  pango_layout_iter_free (iter);
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
                                      x + x_off, y,
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
  PangoGlyphInfo       *gi;
  cairo_font_options_t *flags;
  cairo_matrix_t        trafo;
  double                pos_x;
  double                pos_y;
  gint                  i;
  gint                  x_position = 0;

  flags = gimp_text_layout_render_flags (layout);
  gimp_text_layout_render_trafo (layout, &trafo);

  for (i = 0, gi = glyphs->glyphs; i < glyphs->num_glyphs; i++, gi++)
    {
      if (gi->glyph != PANGO_GLYPH_EMPTY)
        {

          pos_x = x + x_position + gi->geometry.x_offset;
          pos_y = y + gi->geometry.y_offset;

          cairo_matrix_transform_point (&trafo, &pos_x, &pos_y);

          render_func (font, gi->glyph, flags, &trafo,
                       pos_x, pos_y,
                       render_data);
        }

      x_position += glyphs->glyphs[i].geometry.width;
    }
}

static cairo_font_options_t *
gimp_text_layout_render_flags (GimpTextLayout *layout)
{
  GimpText             *text = layout->text;
  cairo_font_options_t *flags;

  flags = cairo_font_options_create ();

  cairo_font_options_set_antialias (flags, (text->antialias ?
                                            CAIRO_ANTIALIAS_DEFAULT :
                                            CAIRO_ANTIALIAS_NONE));

  switch (text->hint_style)
    {
    case GIMP_TEXT_HINT_STYLE_NONE:
      cairo_font_options_set_hint_style (flags, CAIRO_HINT_STYLE_NONE);
      break;

    case GIMP_TEXT_HINT_STYLE_SLIGHT:
      cairo_font_options_set_hint_style (flags, CAIRO_HINT_STYLE_SLIGHT);
      break;

    case GIMP_TEXT_HINT_STYLE_MEDIUM:
      cairo_font_options_set_hint_style (flags, CAIRO_HINT_STYLE_MEDIUM);
      break;

    case GIMP_TEXT_HINT_STYLE_FULL:
      cairo_font_options_set_hint_style (flags, CAIRO_HINT_STYLE_FULL);
      break;
    }

  return flags;
}

static void
gimp_text_layout_render_trafo (GimpTextLayout *layout,
                               cairo_matrix_t *trafo)
{
  GimpText      *text = layout->text;
  const gdouble  norm = 1.0 / layout->yres * layout->xres;

  trafo->xx = text->transformation.coeff[0][0] * norm;
  trafo->xy = text->transformation.coeff[0][1] * 1.0;
  trafo->yx = text->transformation.coeff[1][0] * norm;
  trafo->yy = text->transformation.coeff[1][1] * 1.0;
  trafo->x0 = 0;
  trafo->y0 = 0;
}
