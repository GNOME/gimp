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
#include <pango/pangoft2.h>

#include "text-types.h"

#include "gimptext-bitmap.h"


/* for compatibility with older freetype versions */
#ifndef FT_LOAD_TARGET_MONO
#define FT_LOAD_TARGET_MONO  FT_LOAD_MONOCHROME
#endif


void
gimp_text_render_bitmap (PangoFont  *font,
                         PangoGlyph  glyph,
                         FT_Int32    flags,
                         FT_Matrix  *trafo,
                         gint        x,
                         gint        y,
                         FT_Bitmap  *bitmap)
{
  FT_Face       face;
  gint          y_start, y_limit, x_start, x_limit;
  gint          ix, iy;
  const guchar *src;
  guchar       *dest;

  face = pango_fc_font_lock_face (PANGO_FC_FONT (font));

  FT_Set_Transform (face, trafo, NULL);

  FT_Load_Glyph (face, (FT_UInt) glyph, flags);
  FT_Render_Glyph (face->glyph,
                   (flags & FT_LOAD_TARGET_MONO ?
                    ft_render_mode_mono : ft_render_mode_normal));

  x = PANGO_PIXELS (x);
  y = PANGO_PIXELS (y);

  x_start = MAX (0, - (x + face->glyph->bitmap_left));
  x_limit = MIN (face->glyph->bitmap.width,
                 bitmap->width - (x + face->glyph->bitmap_left));

  y_start = MAX (0,  - (y - face->glyph->bitmap_top));
  y_limit = MIN (face->glyph->bitmap.rows,
                 bitmap->rows - (y - face->glyph->bitmap_top));

  src = face->glyph->bitmap.buffer + y_start * face->glyph->bitmap.pitch;

  dest = bitmap->buffer +
    (y_start + y - face->glyph->bitmap_top) * bitmap->pitch +
    x_start + x + face->glyph->bitmap_left;

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

  pango_fc_font_unlock_face (PANGO_FC_FONT (font));
}
