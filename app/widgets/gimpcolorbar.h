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

#ifndef __GIMP_COLOR_BAR_H__
#define __GIMP_COLOR_BAR_H__


#define GIMP_TYPE_COLOR_BAR            (gimp_color_bar_get_type ())
#define GIMP_COLOR_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_BAR, GimpColorBar))
#define GIMP_COLOR_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_BAR, GimpColorBarClass))
#define GIMP_IS_COLOR_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_BAR))
#define GIMP_IS_COLOR_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_BAR))
#define GIMP_COLOR_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_BAR, GimpColorBarClass))


typedef struct _GimpColorBarClass  GimpColorBarClass;

struct _GimpColorBar
{
  GtkEventBox     parent_class;

  GtkOrientation  orientation;
  guchar          buf[3 * 256];
};

struct _GimpColorBarClass
{
  GtkEventBoxClass  parent_class;
};


GType       gimp_color_bar_get_type    (void) G_GNUC_CONST;

GtkWidget * gimp_color_bar_new         (GtkOrientation        orientation);

void        gimp_color_bar_set_color   (GimpColorBar         *bar,
                                        GeglColor            *color);
void        gimp_color_bar_set_channel (GimpColorBar         *bar,
                                        GimpHistogramChannel  channel);
void        gimp_color_bar_set_buffers (GimpColorBar         *bar,
                                        const guchar         *red,
                                        const guchar         *green,
                                        const guchar         *blue);


#endif  /*  __GIMP_COLOR_BAR_H__  */
