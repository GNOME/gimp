/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Colormap-Rotation plug-in. Exchanges two color ranges.
 *
 * Copyright (C) 1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                    Based on code from Pavel Grinfeld (pavel@ml.com)
 *
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

/*---------------------------------------------------------------------------
 * Change log:
 *
 * Version 2.0, 04 April 1999.
 *  Nearly complete rewrite, made plug-in stable.
 *  (Works with GIMP 1.1 and GTK+ 1.2)
 *
 * Version 1.0, 27 March 1997.
 *  Initial (unstable) release by Pavel Grinfeld
 *
 *---------------------------------------------------------------------------*/


/* Misc functions */

float  rcm_units_factor (gint        units);

gchar *rcm_units_string (gint        units);

void   rcm_set_pixmap   (GtkWidget **widget,
                         GtkWidget  *parent,
                         GtkWidget  *label_box,
                         gchar     **pixmap_data);


/* Ok Button */

void rcm_ok_callback    (GtkWidget *widget,
                         gpointer   data);


/* Circle buttons */

void rcm_360_degrees    (GtkWidget *button,
                         RcmCircle *circle);
void rcm_cw_ccw         (GtkWidget *button,
                         RcmCircle *circle);
void rcm_a_to_b         (GtkWidget *button,
                         RcmCircle *circle);


/* Misc: units buttons */

void rcm_switch_to_degrees         (GtkWidget *button,
                                    gpointer  *value);
void rcm_switch_to_radians         (GtkWidget *button,
                                    gpointer  *value);
void rcm_switch_to_radians_over_PI (GtkWidget *button,
                                    gpointer  *value);


/* Misc: Gray: mode buttons */

void rcm_switch_to_gray_to   (GtkWidget *button,
                              gpointer  *value);
void rcm_switch_to_gray_from (GtkWidget *button,
                              gpointer  *value);


/* Misc: Preview buttons */

void rcm_preview_as_you_drag  (GtkWidget *button,
                               gpointer  *value);

void rcm_combo_callback       (GtkWidget *widget,
                               gpointer   data);


/* Circle events */

gboolean rcm_expose_event        (GtkWidget *widget,
                                  GdkEvent  *event,
                                  RcmCircle *circle);
gboolean rcm_button_press_event  (GtkWidget *widget,
                                  GdkEvent  *event,
                                  RcmCircle *circle);
gboolean rcm_release_event       (GtkWidget *widget,
                                  GdkEvent  *event,
                                  RcmCircle *circle);
gboolean rcm_motion_notify_event (GtkWidget *widget,
                                  GdkEvent  *event,
                                  RcmCircle *circle);


/* Gray circle events */

gboolean rcm_gray_expose_event        (GtkWidget *widget,
                                       GdkEvent  *event,
                                       RcmGray   *circle);
gboolean rcm_gray_button_press_event  (GtkWidget *widget,
                                       GdkEvent  *event,
                                       RcmGray   *circle);
gboolean rcm_gray_release_event       (GtkWidget *widget,
                                       GdkEvent  *event,
                                       RcmGray   *circle);
gboolean rcm_gray_motion_notify_event (GtkWidget *widget,
                                       GdkEvent  *event,
                                       RcmGray   *circle);


/* Spinbuttons */

void rcm_set_alpha    (GtkWidget *entry,
                       gpointer   data);
void rcm_set_beta     (GtkWidget *entry,
                       gpointer   data);
void rcm_set_hue      (GtkWidget *entry,
                       gpointer   data);
void rcm_set_satur    (GtkWidget *entry,
                       gpointer   data);
void rcm_set_gray_sat (GtkWidget *entry,
                       gpointer   data);
