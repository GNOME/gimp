/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpwidgets.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __GIMP_WIDGETS_H__
#define __GIMP_WIDGETS_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Widget Constructors
 */

GtkWidget * gimp_option_menu_new   (GtkSignalFunc       menu_item_callback,
				    gpointer            data,
				    gpointer            initial, /* user_data */

				    /* specify menu items as va_list:
				     *  gchar          *label,
				     *  gpointer        user_data,
				     *  GtkWidget     **widget_ptr,
				     */

				    ...);

GtkWidget * gimp_radio_group_new   (gboolean            in_frame,
				    gchar              *frame_title,

				    /* specify radio buttons as va_list:
				     *  gchar          *label,
				     *  GtkSignalFunc   callback,
				     *  gpointer        data,
				     *  gpointer        user_data,
				     *  GtkWidget     **widget_ptr,
				     *  gboolean        active,
				     */

				    ...);

GtkWidget * gimp_radio_group_new2  (gboolean            in_frame,
				    gchar              *frame_title,
				    GtkSignalFunc       callback,
				    gpointer            data,
				    gpointer            initial, /* user_data */

				    /* specify radio buttons as va_list:
				     *  gchar          *label,
				     *  gpointer        user_data,
				     *  GtkWidget     **widget_ptr,
				     */

				    ...);

GtkWidget * gimp_spin_button_new   (/* return value: */
				    GtkObject         **adjustment,

				    gfloat              value,
				    gfloat              lower,
				    gfloat              upper,
				    gfloat              step_increment,
				    gfloat              page_increment,
				    gfloat              page_size,
				    gfloat              climb_rate,
				    guint               digits);

GtkObject * gimp_scale_entry_new   (GtkTable           *table,
				    gint                column,
				    gint                row,
				    gchar              *text,
				    gint                scale_usize,
				    gint                spinbutton_usize,
				    gfloat              value,
				    gfloat              lower,
				    gfloat              upper,
				    gfloat              step_increment,
				    gfloat              page_increment,
				    guint               digits,
				    gchar              *tooltip,
				    gchar              *private_tip);

GtkWidget * gimp_random_seed_new   (gint               *seed,
				    gint               *use_time,
				    gint                time_true,
				    gint                time_false);

/*
 *  Standard Callbacks
 */

void gimp_toggle_button_update     (GtkWidget          *widget,
				    gpointer            data);

void gimp_menu_item_update         (GtkWidget          *widget,
				    gpointer            data);

void gimp_radio_button_update      (GtkWidget          *widget,
				    gpointer            data);

void gimp_int_adjustment_update    (GtkAdjustment      *adjustment,
				    gpointer            data);

void gimp_float_adjustment_update  (GtkAdjustment      *adjustment,
				    gpointer            data);

void gimp_double_adjustment_update (GtkAdjustment      *adjustment,
				    gpointer            data);

void gimp_unit_menu_update         (GtkWidget          *widget,
				    gpointer            data);

/*
 *  Helper Functions
 */

/*  add aligned label & widget to a two-column table  */
void gimp_table_attach_aligned     (GtkTable           *table,
				    gint                row,
				    gchar              *text,
				    gfloat              xalign,
				    gfloat              yalign,
				    GtkWidget          *widget,
				    gboolean            left_adjust);

#ifdef __cplusplus
}
#endif

#endif /* __GIMP_WIDGETS_H__ */
