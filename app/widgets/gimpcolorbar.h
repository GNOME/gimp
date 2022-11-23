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

#ifndef __LIGMA_COLOR_BAR_H__
#define __LIGMA_COLOR_BAR_H__


#define LIGMA_TYPE_COLOR_BAR            (ligma_color_bar_get_type ())
#define LIGMA_COLOR_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_BAR, LigmaColorBar))
#define LIGMA_COLOR_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_BAR, LigmaColorBarClass))
#define LIGMA_IS_COLOR_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_BAR))
#define LIGMA_IS_COLOR_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_BAR))
#define LIGMA_COLOR_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_BAR, LigmaColorBarClass))


typedef struct _LigmaColorBarClass  LigmaColorBarClass;

struct _LigmaColorBar
{
  GtkEventBox     parent_class;

  GtkOrientation  orientation;
  guchar          buf[3 * 256];
};

struct _LigmaColorBarClass
{
  GtkEventBoxClass  parent_class;
};


GType       ligma_color_bar_get_type    (void) G_GNUC_CONST;

GtkWidget * ligma_color_bar_new         (GtkOrientation        orientation);

void        ligma_color_bar_set_color   (LigmaColorBar         *bar,
                                        const LigmaRGB        *color);
void        ligma_color_bar_set_channel (LigmaColorBar         *bar,
                                        LigmaHistogramChannel  channel);
void        ligma_color_bar_set_buffers (LigmaColorBar         *bar,
                                        const guchar         *red,
                                        const guchar         *green,
                                        const guchar         *blue);


#endif  /*  __LIGMA_COLOR_BAR_H__  */
