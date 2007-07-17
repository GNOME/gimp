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
#include <pango/pangoft2.h>
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
static FT_Int32   gimp_text_layout_render_flags (GimpTextLayout  *layout);
static void       gimp_text_layout_render_trafo (GimpTextLayout  *layout,
                                                 FT_Matrix       *trafo);



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

  x *= PANGO_SCALE;
  y *= PANGO_SCALE;

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
  PangoGlyphInfo *gi;
  FT_Int32        flags;
  FT_Matrix       trafo;
  FT_Vector       pos;
  gint            i;
  gint            x_position = 0;

  flags = gimp_text_layout_render_flags (layout);
  gimp_text_layout_render_trafo (layout, &trafo);

  for (i = 0, gi = glyphs->glyphs; i < glyphs->num_glyphs; i++, gi++)
    {
      if (gi->glyph != PANGO_GLYPH_EMPTY)
        {
          pos.x = x + x_position + gi->geometry.x_offset;
          pos.y = y + gi->geometry.y_offset;

          FT_Vector_Transform (&pos, &trafo);

          render_func (font, gi->glyph, flags, &trafo,
                       pos.x, pos.y,
                       render_data);
        }

      x_position += glyphs->glyphs[i].geometry.width;
    }
}

static FT_Int32
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

static void
gimp_text_layout_render_trafo (GimpTextLayout *layout,
                               FT_Matrix      *trafo)
{
  GimpText *text = layout->text;

  trafo->xx = text->transformation.coeff[0][0] * 65536.0 / layout->yres * layout->xres;
  trafo->xy = text->transformation.coeff[0][1] * 65536.0;
  trafo->yx = text->transformation.coeff[1][0] * 65536.0 / layout->yres * layout->xres;
  trafo->yy = text->transformation.coeff[1][1] * 65536.0;
}
