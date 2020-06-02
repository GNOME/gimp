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

#ifndef __GIMP_HANDLE_BAR_H__
#define __GIMP_HANDLE_BAR_H__


#define GIMP_TYPE_HANDLE_BAR            (gimp_handle_bar_get_type ())
#define GIMP_HANDLE_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HANDLE_BAR, GimpHandleBar))
#define GIMP_HANDLE_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HANDLE_BAR, GimpHandleBarClass))
#define GIMP_IS_HANDLE_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_HANDLE_BAR))
#define GIMP_IS_HANDLE_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HANDLE_BAR))
#define GIMP_HANDLE_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_HANDLE_BAR, GimpHandleBarClass))


typedef struct _GimpHandleBarClass  GimpHandleBarClass;

struct _GimpHandleBar
{
  GtkEventBox     parent_class;

  GtkOrientation  orientation;

  GtkAdjustment  *slider_adj[3];
  gboolean        limits_set;
  gdouble         lower;
  gdouble         upper;

  gint            slider_pos[3];
  gint            active_slider;
};

struct _GimpHandleBarClass
{
  GtkEventBoxClass   parent_class;
};


GType       gimp_handle_bar_get_type       (void) G_GNUC_CONST;

GtkWidget * gimp_handle_bar_new            (GtkOrientation  orientation);

void        gimp_handle_bar_set_adjustment (GimpHandleBar  *bar,
                                            gint            handle_no,
                                            GtkAdjustment  *adjustment);

void        gimp_handle_bar_set_limits     (GimpHandleBar  *bar,
                                            gdouble         lower,
                                            gdouble         upper);
void        gimp_handle_bar_unset_limits   (GimpHandleBar  *bar);
gboolean    gimp_handle_bar_get_limits     (GimpHandleBar  *bar,
                                            gdouble        *lower,
                                            gdouble        *upper);

void        gimp_handle_bar_connect_events (GimpHandleBar  *bar,
                                            GtkWidget      *event_source);


#endif  /*  __GIMP_HANDLE_BAR_H__  */
