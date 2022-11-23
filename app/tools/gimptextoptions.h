/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_TEXT_OPTIONS_H__
#define __LIGMA_TEXT_OPTIONS_H__


#include "core/ligmatooloptions.h"


#define LIGMA_TYPE_TEXT_OPTIONS            (ligma_text_options_get_type ())
#define LIGMA_TEXT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEXT_OPTIONS, LigmaTextOptions))
#define LIGMA_TEXT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TEXT_OPTIONS, LigmaTextOptionsClass))
#define LIGMA_IS_TEXT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEXT_OPTIONS))
#define LIGMA_IS_TEXT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TEXT_OPTIONS))
#define LIGMA_TEXT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TEXT_OPTIONS, LigmaTextOptionsClass))


typedef struct _LigmaTextOptions      LigmaTextOptions;
typedef struct _LigmaToolOptionsClass LigmaTextOptionsClass;

struct _LigmaTextOptions
{
  LigmaToolOptions        tool_options;

  LigmaUnit               unit;
  gdouble                font_size;
  gboolean               antialias;
  LigmaTextHintStyle      hint_style;
  gchar                 *language;
  LigmaTextDirection      base_dir;
  LigmaTextJustification  justify;
  gdouble                indent;
  gdouble                line_spacing;
  gdouble                letter_spacing;
  LigmaTextBoxMode        box_mode;

  LigmaTextOutline        outline;
  LigmaFillStyle          outline_style;
  LigmaRGB                outline_foreground;
  LigmaPattern           *outline_pattern;
  gdouble                outline_width;
  LigmaUnit               outline_unit;
  LigmaCapStyle           outline_cap_style;
  LigmaJoinStyle          outline_join_style;
  gdouble                outline_miter_limit;
  gboolean               outline_antialias;
  gdouble                outline_dash_offset;
  GArray                *outline_dash_info;

  LigmaViewType           font_view_type;
  LigmaViewSize           font_view_size;

  gboolean               use_editor;

  /*  options gui  */
  GtkWidget             *size_entry;
};


GType       ligma_text_options_get_type     (void) G_GNUC_CONST;

void        ligma_text_options_connect_text (LigmaTextOptions *options,
                                            LigmaText        *text);

GtkWidget * ligma_text_options_gui          (LigmaToolOptions *tool_options);

GtkWidget * ligma_text_options_editor_new   (GtkWindow       *parent,
                                            Ligma            *ligma,
                                            LigmaTextOptions *options,
                                            LigmaMenuFactory *menu_factory,
                                            const gchar     *title,
                                            LigmaText        *text,
                                            LigmaTextBuffer  *text_buffer,
                                            gdouble          xres,
                                            gdouble          yres);


#endif /* __LIGMA_TEXT_OPTIONS_H__ */
