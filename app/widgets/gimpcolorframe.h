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

#ifndef __LIGMA_COLOR_FRAME_H__
#define __LIGMA_COLOR_FRAME_H__


#define LIGMA_COLOR_FRAME_ROWS 6


#define LIGMA_TYPE_COLOR_FRAME            (ligma_color_frame_get_type ())
#define LIGMA_COLOR_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_FRAME, LigmaColorFrame))
#define LIGMA_COLOR_FRAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_FRAME, LigmaColorFrameClass))
#define LIGMA_IS_COLOR_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_FRAME))
#define LIGMA_IS_COLOR_FRAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_FRAME))
#define LIGMA_COLOR_FRAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_FRAME, LigmaColorFrameClass))


typedef struct _LigmaColorFrameClass LigmaColorFrameClass;

struct _LigmaColorFrame
{
  LigmaFrame           parent_instance;

  Ligma               *ligma;

  gboolean            sample_valid;
  gboolean            sample_average;
  const Babl         *sample_format;
  gdouble             pixel[4];
  LigmaRGB             color;
  gint                x;
  gint                y;

  LigmaColorPickMode   pick_mode;

  PangoEllipsizeMode  ellipsize;

  gboolean            has_number;
  gint                number;

  gboolean            has_color_area;
  gboolean            has_coords;

  GtkWidget          *combo;
  GtkWidget          *color_area;
  GtkWidget          *coords_box_x;
  GtkWidget          *coords_box_y;
  GtkWidget          *coords_label_x;
  GtkWidget          *coords_label_y;
  GtkWidget          *name_labels[LIGMA_COLOR_FRAME_ROWS];
  GtkWidget          *value_labels[LIGMA_COLOR_FRAME_ROWS];

  PangoLayout        *number_layout;

  LigmaImage          *image;

  LigmaColorConfig    *config;
  LigmaColorProfile   *view_profile;
  LigmaColorRenderingIntent
                      simulation_intent;
};

struct _LigmaColorFrameClass
{
  LigmaFrameClass      parent_class;
};


GType       ligma_color_frame_get_type           (void) G_GNUC_CONST;

GtkWidget * ligma_color_frame_new                (Ligma               *ligma);

void        ligma_color_frame_set_mode           (LigmaColorFrame     *frame,
                                                 LigmaColorPickMode   mode);

void        ligma_color_frame_set_ellipsize      (LigmaColorFrame     *frame,
                                                 PangoEllipsizeMode  ellipsize);

void        ligma_color_frame_set_has_number     (LigmaColorFrame     *frame,
                                                 gboolean            has_number);
void        ligma_color_frame_set_number         (LigmaColorFrame     *frame,
                                                 gint                number);

void        ligma_color_frame_set_has_color_area (LigmaColorFrame     *frame,
                                                 gboolean            has_color_area);
void        ligma_color_frame_set_has_coords     (LigmaColorFrame     *frame,
                                                 gboolean            has_coords);

void        ligma_color_frame_set_color          (LigmaColorFrame     *frame,
                                                 gboolean            sample_average,
                                                 const Babl         *format,
                                                 gpointer            pixel,
                                                 const LigmaRGB      *color,
                                                 gint                x,
                                                 gint                y);
void        ligma_color_frame_set_invalid        (LigmaColorFrame     *frame);

void        ligma_color_frame_set_color_config   (LigmaColorFrame     *frame,
                                                 LigmaColorConfig    *config);

void        ligma_color_frame_set_image          (LigmaColorFrame     *frame,
                                                 LigmaImage          *image);


#endif  /*  __LIGMA_COLOR_FRAME_H__  */
