/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaDisplayOptions
 * Copyright (C) 2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_DISPLAY_OPTIONS_H__
#define __LIGMA_DISPLAY_OPTIONS_H__


#define LIGMA_TYPE_DISPLAY_OPTIONS            (ligma_display_options_get_type ())
#define LIGMA_TYPE_DISPLAY_OPTIONS_FULLSCREEN (ligma_display_options_fullscreen_get_type ())
#define LIGMA_TYPE_DISPLAY_OPTIONS_NO_IMAGE   (ligma_display_options_no_image_get_type ())

#define LIGMA_DISPLAY_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DISPLAY_OPTIONS, LigmaDisplayOptions))
#define LIGMA_DISPLAY_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DISPLAY_OPTIONS, LigmaDisplayOptionsClass))
#define LIGMA_IS_DISPLAY_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DISPLAY_OPTIONS))
#define LIGMA_IS_DISPLAY_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DISPLAY_OPTIONS))
#define LIGMA_DISPLAY_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DISPLAY_OPTIONS, LigmaDisplayOptionsClass))


typedef struct _LigmaDisplayOptionsClass LigmaDisplayOptionsClass;

struct _LigmaDisplayOptions
{
  GObject                parent_instance;

  /*  LigmaImageWindow options  */
  gboolean               show_menubar;
  gboolean               show_statusbar;

  /*  LigmaDisplayShell options  */
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

  LigmaCanvasPaddingMode  padding_mode;
  LigmaRGB                padding_color;
  gboolean               padding_mode_set;
  gboolean               padding_in_show_all;
};

struct _LigmaDisplayOptionsClass
{
  GObjectClass           parent_class;
};


GType  ligma_display_options_get_type            (void) G_GNUC_CONST;
GType  ligma_display_options_fullscreen_get_type (void) G_GNUC_CONST;
GType  ligma_display_options_no_image_get_type   (void) G_GNUC_CONST;


#endif /* __LIGMA_DISPLAY_OPTIONS_H__ */
