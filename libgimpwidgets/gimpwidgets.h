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

#include "gimpsizeentry.h"

#ifdef __cplusplus
extern "C" {
#endif

/* For information look into the C source or the html documentation */

/*
 *  Widget Constructors
 */

GtkWidget * gimp_option_menu_new   (gboolean            menu_only,

				    /* specify menu items as va_list:
				     *  gchar          *label,
				     *  GtkSignalFunc   callback,
				     *  gpointer        data,
				     *  gpointer        user_data,
				     *  GtkWidget     **widget_ptr,
				     *  gboolean        active
				     */

				    ...);

GtkWidget * gimp_option_menu_new2  (gboolean            menu_only,
				    GtkSignalFunc       menu_item_callback,
				    gpointer            data,
				    gpointer            initial, /* user_data */

				    /* specify menu items as va_list:
				     *  gchar          *label,
				     *  gpointer        user_data,
				     *  GtkWidget     **widget_ptr,
				     */

				    ...);

void  gimp_option_menu_set_history (GtkOptionMenu      *option_menu,
				    gpointer            user_data);

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
				    GtkSignalFunc       radio_button_callback,
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

#define GIMP_SCALE_ENTRY_LABEL(adj) \
        GTK_LABEL (gtk_object_get_data (GTK_OBJECT(adj), "label"))

#define GIMP_SCALE_ENTRY_SCALE(adj) \
        GTK_HSCALE (gtk_object_get_data (GTK_OBJECT(adj), "scale"))
#define GIMP_SCALE_ENTRY_SCALE_ADJ(adj) \
        gtk_range_get_adjustment \
        (GTK_RANGE (gtk_object_get_data (GTK_OBJECT (adj), "scale")))

#define GIMP_SCALE_ENTRY_SPINBUTTON(adj) \
        GTK_SPIN_BUTTON (gtk_object_get_data (GTK_OBJECT (adj), "spinbutton"))
#define GIMP_SCALE_ENTRY_SPINBUTTON_ADJ(adj) \
        gtk_spin_button_get_adjustment \
        (GTK_SPIN_BUTTON (gtk_object_get_data (GTK_OBJECT (adj), "spinbutton")))

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
				    gboolean            constrain,
				    gfloat              unconstrained_lower,
				    gfloat              unconstrained_upper,
				    gchar              *tooltip,
				    gchar              *help_data);

#define GIMP_RANDOM_SEED_SPINBUTTON(hbox) \
        GTK_SPIN_BUTTON (gtk_object_get_data (GTK_OBJECT( hbox), "spinbutton"))
#define GIMP_RANDOM_SEED_SPINBUTTON_ADJ(hbox) \
        gtk_spin_button_get_adjustment \
        (GTK_SPIN_BUTTON (gtk_object_get_data (GTK_OBJECT (hbox), "spinbutton")))

#define GIMP_RANDOM_SEED_TOGGLEBUTTON(hbox) \
        GTK_TOGGLE_BUTTON (gtk_object_get_data (GTK_OBJECT (hbox), \
                                                "togglebutton"))

GtkWidget * gimp_random_seed_new   (gint               *seed,
				    gint               *use_time,
				    gint                time_true,
				    gint                time_false);

#define GIMP_COORDINATES_CHAINBUTTON(sizeentry) \
        GIMP_CHAIN_BUTTON (gtk_object_get_data (GTK_OBJECT (sizeentry), \
                                                "chainbutton"))

GtkWidget * gimp_coordinates_new   (GimpUnit            unit,
				    gchar              *unit_format,
				    gboolean            menu_show_pixels,
				    gboolean            menu_show_percent,
				    gint                spinbutton_usize,
				    GimpSizeEntryUpdatePolicy  update_policy,

				    gboolean            chainbutton_active,
				    gboolean            chain_constrains_ratio,

				    gchar              *xlabel,
				    gdouble             x,
				    gdouble             xres,
				    gdouble             lower_boundary_x,
				    gdouble             upper_boundary_x,
				    gdouble             xsize_0,   /* % */
				    gdouble             xsize_100, /* % */

				    gchar              *ylabel,
				    gdouble             y,
				    gdouble             yres,
				    gdouble             lower_boundary_y,
				    gdouble             upper_boundary_y,
				    gdouble             ysize_0,   /* % */
				    gdouble             ysize_100  /* % */);

GtkWidget * gimp_pixmap_button_new (gchar             **xpm_data,
				    gchar              *text);

/*
 *  Standard Callbacks
 */

void gimp_toggle_button_sensitive_update (GtkToggleButton *toggle_button);

void gimp_toggle_button_update           (GtkWidget       *widget,
					  gpointer         data);

void gimp_radio_button_update            (GtkWidget       *widget,
					  gpointer         data);

void gimp_menu_item_update               (GtkWidget       *widget,
					  gpointer         data);

void gimp_int_adjustment_update          (GtkAdjustment   *adjustment,
					  gpointer         data);

void gimp_float_adjustment_update        (GtkAdjustment   *adjustment,
					  gpointer         data);

void gimp_double_adjustment_update       (GtkAdjustment   *adjustment,
					  gpointer         data);

void gimp_unit_menu_update               (GtkWidget       *widget,
					  gpointer         data);

/*
 *  Helper Functions
 */

void gimp_table_attach_aligned     (GtkTable           *table,
				    gint                column,
				    gint                row,
				    gchar              *label_text,
				    gfloat              xalign,
				    gfloat              yalign,
				    GtkWidget          *widget,
				    gint                colspan,
				    gboolean            left_align);

#ifdef __cplusplus
}
#endif

#endif /* __GIMP_WIDGETS_H__ */
