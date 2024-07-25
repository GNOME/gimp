/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_TEXT_H__
#define __GIMP_TEXT_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_TEXT            (gimp_text_get_type ())
#define GIMP_TEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT, GimpText))
#define GIMP_TEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEXT, GimpTextClass))
#define GIMP_IS_TEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT))
#define GIMP_IS_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEXT))
#define GIMP_TEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TEXT, GimpTextClass))


typedef struct _GimpTextClass  GimpTextClass;

struct _GimpText
{
  GimpObject             parent_instance;

  gchar                 *text;
  gchar                 *markup;
  GimpFont              *font;
  GimpUnit              *unit;
  gdouble                font_size;
  gboolean               antialias;
  GimpTextHintStyle      hint_style;
  gboolean               kerning;
  gchar                 *language;
  GimpTextDirection      base_dir;
  GeglColor             *color;
  GimpCustomStyle        outline_style;
  GimpPattern           *outline_pattern;
  GeglColor             *outline_foreground;
  gdouble                outline_width;
  GimpCapStyle           outline_cap_style;
  GimpJoinStyle          outline_join_style;
  gdouble                outline_miter_limit;
  gboolean               outline_antialias;
  gdouble                outline_dash_offset;
  GArray                *outline_dash_info;
  GimpTextOutline        outline;
  GimpTextJustification  justify;
  gdouble                indent;
  gdouble                line_spacing;
  gdouble                letter_spacing;
  GimpTextBoxMode        box_mode;
  gdouble                box_width;
  gdouble                box_height;
  GimpUnit              *box_unit;
  GimpMatrix2            transformation;
  gdouble                offset_x;
  gdouble                offset_y;

  gdouble                border;
  Gimp                  *gimp;
};

struct _GimpTextClass
{
  GimpObjectClass        parent_class;

  void (* changed) (GimpText *text);
};


GType  gimp_text_get_type           (void) G_GNUC_CONST;

void   gimp_text_get_transformation (GimpText    *text,
                                     GimpMatrix3 *matrix);


#endif /* __GIMP_TEXT_H__ */
