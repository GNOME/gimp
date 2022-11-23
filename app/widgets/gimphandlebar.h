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

#ifndef __LIGMA_HANDLE_BAR_H__
#define __LIGMA_HANDLE_BAR_H__


#define LIGMA_TYPE_HANDLE_BAR            (ligma_handle_bar_get_type ())
#define LIGMA_HANDLE_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_HANDLE_BAR, LigmaHandleBar))
#define LIGMA_HANDLE_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_HANDLE_BAR, LigmaHandleBarClass))
#define LIGMA_IS_HANDLE_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_HANDLE_BAR))
#define LIGMA_IS_HANDLE_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_HANDLE_BAR))
#define LIGMA_HANDLE_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_HANDLE_BAR, LigmaHandleBarClass))


typedef struct _LigmaHandleBarClass  LigmaHandleBarClass;

struct _LigmaHandleBar
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

struct _LigmaHandleBarClass
{
  GtkEventBoxClass   parent_class;
};


GType       ligma_handle_bar_get_type       (void) G_GNUC_CONST;

GtkWidget * ligma_handle_bar_new            (GtkOrientation  orientation);

void        ligma_handle_bar_set_adjustment (LigmaHandleBar  *bar,
                                            gint            handle_no,
                                            GtkAdjustment  *adjustment);

void        ligma_handle_bar_set_limits     (LigmaHandleBar  *bar,
                                            gdouble         lower,
                                            gdouble         upper);
void        ligma_handle_bar_unset_limits   (LigmaHandleBar  *bar);
gboolean    ligma_handle_bar_get_limits     (LigmaHandleBar  *bar,
                                            gdouble        *lower,
                                            gdouble        *upper);

void        ligma_handle_bar_connect_events (LigmaHandleBar  *bar,
                                            GtkWidget      *event_source);


#endif  /*  __LIGMA_HANDLE_BAR_H__  */
