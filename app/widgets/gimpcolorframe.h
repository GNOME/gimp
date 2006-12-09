/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_COLOR_FRAME_H__
#define __GIMP_COLOR_FRAME_H__


#include <gtk/gtkframe.h>


#define GIMP_COLOR_FRAME_ROWS 5


#define GIMP_TYPE_COLOR_FRAME            (gimp_color_frame_get_type ())
#define GIMP_COLOR_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_FRAME, GimpColorFrame))
#define GIMP_COLOR_FRAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_FRAME, GimpColorFrameClass))
#define GIMP_IS_COLOR_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_FRAME))
#define GIMP_IS_COLOR_FRAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_FRAME))
#define GIMP_COLOR_FRAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_FRAME, GimpColorFrameClass))


typedef struct _GimpColorFrameClass GimpColorFrameClass;

struct _GimpColorFrame
{
  GimpFrame           parent_instance;

  gboolean            sample_valid;
  GimpImageType       sample_type;
  GimpRGB             color;
  gint                color_index;

  GimpColorFrameMode  frame_mode;

  gboolean            has_number;
  gint                number;

  gboolean            has_color_area;

  GtkWidget          *menu;
  GtkWidget          *number_label;
  GtkWidget          *color_area;
  GtkWidget          *name_labels[GIMP_COLOR_FRAME_ROWS];
  GtkWidget          *value_labels[GIMP_COLOR_FRAME_ROWS];
};

struct _GimpColorFrameClass
{
  GimpFrameClass      parent_class;
};


GType       gimp_color_frame_get_type    (void) G_GNUC_CONST;

GtkWidget * gimp_color_frame_new         (void);

void        gimp_color_frame_set_mode           (GimpColorFrame     *frame,
                                                 GimpColorFrameMode  mode);
void        gimp_color_frame_set_has_number     (GimpColorFrame     *frame,
                                                 gboolean            has_number);
void        gimp_color_frame_set_number         (GimpColorFrame     *frame,
                                                 gint                number);


void        gimp_color_frame_set_has_color_area (GimpColorFrame     *frame,
                                                 gboolean            has_color_area);

void        gimp_color_frame_set_color          (GimpColorFrame     *frame,
                                                 GimpImageType       sample_type,
                                                 const GimpRGB      *color,
                                                 gint                color_index);
void        gimp_color_frame_set_invalid        (GimpColorFrame     *frame);


#endif  /*  __GIMP_COLOR_FRAME_H__  */
