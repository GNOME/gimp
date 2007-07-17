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

#ifndef __GIMP_TEXT_LAYOUT_PRIVATE_H__
#define __GIMP_TEXT_LAYOUT_PRIVATE_H__

/*  The purpose of this extra header file is to hide any Pango or
 *  FreeType types from the rest of the gimp core.
 */


struct _GimpTextLayout
{
  GObject         object;

  GimpText       *text;
  gdouble         xres;
  gdouble         yres;
  PangoLayout    *layout;
  PangoRectangle  extents;
};

struct _GimpTextLayoutClass
{
  GObjectClass   parent_class;
};


typedef  void (* GimpTextRenderFunc) (PangoFont  *font,
                                      PangoGlyph  glyph,
                                      FT_Int32    load_flags,
                                      FT_Matrix  *tranform,
                                      gint        x,
                                      gint        y,
                                      gpointer    render_data);


#endif /* __GIMP_TEXT_LAYOUT_PRIVATE_H__ */
