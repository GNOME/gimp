/* GIMP - The GNU Image Manipulation Program
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

#pragma once

#include "core/gimptooloptions.h"


#define GIMP_TYPE_TEXT_OPTIONS            (gimp_text_options_get_type ())
#define GIMP_TEXT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT_OPTIONS, GimpTextOptions))
#define GIMP_TEXT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEXT_OPTIONS, GimpTextOptionsClass))
#define GIMP_IS_TEXT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT_OPTIONS))
#define GIMP_IS_TEXT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEXT_OPTIONS))
#define GIMP_TEXT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TEXT_OPTIONS, GimpTextOptionsClass))


typedef struct _GimpTextOptions      GimpTextOptions;
typedef struct _GimpToolOptionsClass GimpTextOptionsClass;

struct _GimpTextOptions
{
  GimpToolOptions           tool_options;

  GimpUnit                 *unit;
  gdouble                   font_size;
  gboolean                  antialias;
  GimpTextHintStyle         hint_style;
  gchar                    *language;
  GimpTextDirection         base_dir;
  GimpTextJustification     justify;
  gdouble                   indent;
  gdouble                   line_spacing;
  gdouble                   letter_spacing;
  GimpTextBoxMode           box_mode;

  GimpTextOutline           outline;
  GimpCustomStyle           outline_style;
  GeglColor                *outline_foreground;
  GimpPattern              *outline_pattern;
  gdouble                   outline_width;
  GimpUnit                 *outline_unit;
  GimpTextOutlineDirection  outline_direction;
  GimpCapStyle              outline_cap_style;
  GimpJoinStyle             outline_join_style;
  gdouble                   outline_miter_limit;
  gboolean                  outline_antialias;
  gdouble                   outline_dash_offset;
  GArray                   *outline_dash_info;

  GimpViewType              font_view_type;
  GimpViewSize              font_view_size;

  gboolean                  use_editor;
  gboolean                  show_on_canvas;

  /*  options gui  */
  GtkWidget                *size_entry;
};


GType       gimp_text_options_get_type     (void) G_GNUC_CONST;

void        gimp_text_options_connect_text (GimpTextOptions *options,
                                            GimpText        *text);

GtkWidget * gimp_text_options_gui          (GimpToolOptions *tool_options);

GtkWidget * gimp_text_options_editor_new   (GtkWindow       *parent,
                                            Gimp            *gimp,
                                            GimpTextOptions *options,
                                            GimpMenuFactory *menu_factory,
                                            const gchar     *title,
                                            GimpText        *text,
                                            GimpTextBuffer  *text_buffer,
                                            gdouble          xres,
                                            gdouble          yres);
