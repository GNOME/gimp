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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_CANVAS_H__
#define __GIMP_CANVAS_H__


#include "widgets/gimpoverlaybox.h"


typedef enum
{
  GIMP_CANVAS_STYLE_BLACK,
  GIMP_CANVAS_STYLE_WHITE,
  GIMP_CANVAS_STYLE_RENDER,
  GIMP_CANVAS_STYLE_XOR,
  GIMP_CANVAS_STYLE_XOR_DASHED,
  GIMP_CANVAS_STYLE_XOR_DOTTED,
  GIMP_CANVAS_STYLE_SELECTION_IN,
  GIMP_CANVAS_STYLE_SELECTION_OUT,
  GIMP_CANVAS_STYLE_LAYER_BOUNDARY,
  GIMP_CANVAS_STYLE_LAYER_GROUP_BOUNDARY,
  GIMP_CANVAS_STYLE_SAMPLE_POINT_NORMAL,
  GIMP_CANVAS_STYLE_SAMPLE_POINT_ACTIVE,
  GIMP_CANVAS_STYLE_LAYER_MASK_ACTIVE,
  GIMP_CANVAS_STYLE_CUSTOM,
  GIMP_CANVAS_NUM_STYLES
} GimpCanvasStyle;


#define GIMP_CANVAS_NUM_STIPPLES  8
#define GIMP_CANVAS_EVENT_MASK   (GDK_EXPOSURE_MASK            | \
                                  GDK_POINTER_MOTION_MASK      |  \
                                  GDK_POINTER_MOTION_HINT_MASK |  \
                                  GDK_BUTTON_PRESS_MASK        |  \
                                  GDK_BUTTON_RELEASE_MASK      |  \
                                  GDK_STRUCTURE_MASK           |  \
                                  GDK_ENTER_NOTIFY_MASK        |  \
                                  GDK_LEAVE_NOTIFY_MASK        |  \
                                  GDK_FOCUS_CHANGE_MASK        |  \
                                  GDK_KEY_PRESS_MASK           |  \
                                  GDK_KEY_RELEASE_MASK         |  \
                                  GDK_PROXIMITY_OUT_MASK)


#define GIMP_TYPE_CANVAS            (gimp_canvas_get_type ())
#define GIMP_CANVAS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS, GimpCanvas))
#define GIMP_CANVAS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS, GimpCanvasClass))
#define GIMP_IS_CANVAS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS))
#define GIMP_IS_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS))
#define GIMP_CANVAS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS, GimpCanvasClass))


typedef struct _GimpCanvasClass GimpCanvasClass;

struct _GimpCanvas
{
  GimpOverlayBox     parent_instance;

  GimpDisplayConfig *config;

  GdkGC             *gc[GIMP_CANVAS_NUM_STYLES];
  GdkBitmap         *stipple[GIMP_CANVAS_NUM_STIPPLES];
  PangoLayout       *layout;
};

struct _GimpCanvasClass
{
  GimpOverlayBoxClass  parent_class;
};


GType        gimp_canvas_get_type          (void) G_GNUC_CONST;

GtkWidget  * gimp_canvas_new               (GimpDisplayConfig  *config);

void         gimp_canvas_draw_cursor       (GimpCanvas         *canvas,
                                            gint                x,
                                            gint                y);
void         gimp_canvas_draw_point        (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            gint                x,
                                            gint                y);
void         gimp_canvas_draw_points       (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            GdkPoint           *points,
                                            gint                num_points);
void         gimp_canvas_draw_line         (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            gint                x1,
                                            gint                y1,
                                            gint                x2,
                                            gint                y2);
void         gimp_canvas_draw_lines        (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            GdkPoint           *points,
                                            gint                num_points);
void         gimp_canvas_draw_rectangle    (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            gboolean            filled,
                                            gint                x,
                                            gint                y,
                                            gint                width,
                                            gint                height);
void         gimp_canvas_draw_arc          (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            gboolean            filled,
                                            gint                x,
                                            gint                y,
                                            gint                width,
                                            gint                height,
                                            gint                angle1,
                                            gint                angle2);
void         gimp_canvas_draw_polygon      (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            gboolean            filled,
                                            GdkPoint           *points,
                                            gint                num_points);
void         gimp_canvas_draw_segments     (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            GdkSegment         *segments,
                                            gint                num_segments);
void         gimp_canvas_draw_text         (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            gint                x,
                                            gint                y,
                                            const gchar        *format,
                                            ...) G_GNUC_PRINTF (5, 6);
void         gimp_canvas_draw_rgb          (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            gint                x,
                                            gint                y,
                                            gint                width,
                                            gint                height,
                                            guchar             *rgb_buf,
                                            gint                rowstride,
                                            gint                xdith,
                                            gint                ydith);
void         gimp_canvas_draw_drop_zone    (GimpCanvas         *canvas,
                                            cairo_t            *cr);

void         gimp_canvas_set_clip_rect     (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            const GdkRectangle *rect);
void         gimp_canvas_set_clip_region   (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            const GdkRegion    *region);
void         gimp_canvas_set_stipple_index (GimpCanvas         *canvas,
                                            GimpCanvasStyle     style,
                                            guint               index);
void         gimp_canvas_set_custom_gc     (GimpCanvas         *canvas,
                                            GdkGC              *gc);
void         gimp_canvas_set_bg_color      (GimpCanvas         *canvas,
                                            GimpRGB            *color);


#endif /*  __GIMP_CANVAS_H__  */
