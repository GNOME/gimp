/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaText
 * Copyright (C) 2002-2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_TEXT_H__
#define __LIGMA_TEXT_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_TEXT            (ligma_text_get_type ())
#define LIGMA_TEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEXT, LigmaText))
#define LIGMA_TEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TEXT, LigmaTextClass))
#define LIGMA_IS_TEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEXT))
#define LIGMA_IS_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TEXT))
#define LIGMA_TEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TEXT, LigmaTextClass))


typedef struct _LigmaTextClass  LigmaTextClass;

struct _LigmaText
{
  LigmaObject             parent_instance;

  gchar                 *text;
  gchar                 *markup;
  gchar                 *font;
  LigmaUnit               unit;
  gdouble                font_size;
  gboolean               antialias;
  LigmaTextHintStyle      hint_style;
  gboolean               kerning;
  gchar                 *language;
  LigmaTextDirection      base_dir;
  LigmaRGB                color;
  LigmaFillStyle          outline_style;
  LigmaPattern           *outline_pattern;
  LigmaRGB                outline_foreground;
  gdouble                outline_width;
  LigmaCapStyle           outline_cap_style;
  LigmaJoinStyle          outline_join_style;
  gdouble                outline_miter_limit;
  gboolean               outline_antialias;
  gdouble                outline_dash_offset;
  GArray                *outline_dash_info;
  LigmaTextOutline        outline;
  LigmaTextJustification  justify;
  gdouble                indent;
  gdouble                line_spacing;
  gdouble                letter_spacing;
  LigmaTextBoxMode        box_mode;
  gdouble                box_width;
  gdouble                box_height;
  LigmaUnit               box_unit;
  LigmaMatrix2            transformation;
  gdouble                offset_x;
  gdouble                offset_y;

  gdouble                border;
  Ligma                  *ligma;
};

struct _LigmaTextClass
{
  LigmaObjectClass        parent_class;

  void (* changed) (LigmaText *text);
};


GType  ligma_text_get_type           (void) G_GNUC_CONST;

void   ligma_text_get_transformation (LigmaText    *text,
                                     LigmaMatrix3 *matrix);


#endif /* __LIGMA_TEXT_H__ */
