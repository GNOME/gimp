/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpDisplayOptions
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_DISPLAY_OPTIONS_H__
#define __GIMP_DISPLAY_OPTIONS_H__


#define GIMP_TYPE_DISPLAY_OPTIONS            (gimp_display_options_get_type ())
#define GIMP_TYPE_DISPLAY_OPTIONS_FULLSCREEN (gimp_display_options_fullscreen_get_type ())
#define GIMP_TYPE_DISPLAY_OPTIONS_NO_IMAGE   (gimp_display_options_no_image_get_type ())

#define GIMP_DISPLAY_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DISPLAY_OPTIONS, GimpDisplayOptions))
#define GIMP_DISPLAY_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DISPLAY_OPTIONS, GimpDisplayOptionsClass))
#define GIMP_IS_DISPLAY_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DISPLAY_OPTIONS))
#define GIMP_IS_DISPLAY_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DISPLAY_OPTIONS))
#define GIMP_DISPLAY_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DISPLAY_OPTIONS, GimpDisplayOptionsClass))


typedef struct _GimpDisplayOptionsClass GimpDisplayOptionsClass;

struct _GimpDisplayOptions
{
  GObject                parent_instance;

  /*  GimpImageWindow options  */
  gboolean               show_menubar;
  gboolean               show_statusbar;

  /*  GimpDisplayShell options  */
  gboolean               show_rulers;
  gboolean               show_scrollbars;
  gboolean               show_selection;
  gboolean               show_layer_boundary;
  gboolean               show_canvas_boundary;
  gboolean               show_guides;
  gboolean               show_grid;
  gboolean               show_sample_points;

  gboolean               snap_to_guides;
  gboolean               snap_to_grid;
  gboolean               snap_to_canvas;
  gboolean               snap_to_path;
  gboolean               snap_to_bbox;
  gboolean               snap_to_equidistance;

  GimpCanvasPaddingMode  padding_mode;
  GeglColor             *padding_color;
  gboolean               padding_mode_set;
  gboolean               padding_in_show_all;
};

struct _GimpDisplayOptionsClass
{
  GObjectClass           parent_class;
};


GType  gimp_display_options_get_type            (void) G_GNUC_CONST;
GType  gimp_display_options_fullscreen_get_type (void) G_GNUC_CONST;
GType  gimp_display_options_no_image_get_type   (void) G_GNUC_CONST;


#endif /* __GIMP_DISPLAY_OPTIONS_H__ */
