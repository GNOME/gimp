/***************************************************************************/
/* GCK - The General Convenience Kit. Generally useful conveniece routines */
/* for GIMP plug-in writers and users of the GDK/GTK libraries.            */
/* Copyright (C) 1996 Tom Bech                                             */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,   */
/* USA.                                                                    */
/***************************************************************************/

#ifndef __GCKUI_H__
#define __GCKUI_H__

#include "gck.h"

#ifdef __cplusplus
extern "C" {
#endif

void                  gck_cursor_set                 (GdkWindow *window,
                                                      GdkCursorType cursortype);

void                  gck_auto_show                  (gint flag);

GckApplicationWindow *gck_application_window_new     (char *name);
void                  gck_application_window_destroy (GckApplicationWindow *appwin);

GckDialogWindow      *gck_dialog_window_new          (char *name,
                                                      GckPosition ActionPos,
                                                      GtkSignalFunc ok_pressed_func,
                                                      GtkSignalFunc cancel_pressed_func,
                                                      GtkSignalFunc help_pressed_func);

void                  gck_dialog_window_destroy      (GckDialogWindow *dialog);

GtkWidget            *gck_vseparator_new             (GtkWidget *container);
GtkWidget            *gck_hseparator_new             (GtkWidget *container);

GtkWidget            *gck_frame_new                  (char *name,GtkWidget *container,
                                                      GtkShadowType shadowtype,
                                                      gint expand,gint fill,gint padding,
                                                      gint borderwidth);

GtkWidget            *gck_label_new                  (char *name,GtkWidget *container);
GtkWidget            *gck_label_aligned_new          (char *name,GtkWidget *container,
                                                      gdouble xalign,gdouble yalign);

GtkWidget            *gck_drawing_area_new           (GtkWidget *container,
                                                      gint width,gint height,
                                                      gint event_mask,
                                                      GtkSignalFunc event_handler);

GtkWidget            *gck_hscale_new                 (char *name,GtkWidget *container,
                                                      GckScaleValues *svals,
                                                      GtkSignalFunc value_changed_func);

GtkWidget            *gck_vscale_new                 (char *name,GtkWidget *container,
                                                      GckScaleValues *svals,
                                                      GtkSignalFunc value_changed_func);

GtkWidget            *gck_entryfield_new             (char *name,GtkWidget *container,
                                                      double initial_value,
                                                      GtkSignalFunc valuechangedfunc);

GtkWidget            *gck_entryfield_text_new        (char *name,GtkWidget *container,
                                                      char *initial_text,
                                                      GtkSignalFunc textchangedfunc);

GtkWidget            *gck_pushbutton_new             (char *name,GtkWidget *container,
                                                      gint expand,gint fill,gint padding,
                                                      GtkSignalFunc button_clicked_func);

GtkWidget            *gck_pushbutton_pixmap_new      (char *name,
                                                      GdkPixmap *pixm,
                                                      GdkBitmap *mask,
                                                      GtkWidget *container,
                                                      gint expand,gint fill,gint padding,
                                                      GtkSignalFunc button_clicked_func);

GtkWidget            *gck_togglebutton_pixmap_new    (char *name,
                                                      GdkPixmap *pixm,
                                                      GdkBitmap *mask,
                                                      GtkWidget *container,
                                                      gint expand,gint fill,gint padding,
                                                      GtkSignalFunc button_toggled_func);

GtkWidget            *gck_checkbutton_new            (char *name,GtkWidget *container,
                                                      gint value,
                                                      GtkSignalFunc status_changed_func);

GtkWidget            *gck_radiobutton_new            (char *name,GtkWidget *container,
                                                      GtkWidget *previous,
                                                      GtkSignalFunc status_changed_func);

GtkWidget            *gck_radiobutton_pixmap_new     (char *name, 
                                                      GdkPixmap *pixm,
                                                      GdkBitmap *mask,
                                                      GtkWidget *container,
                                                      GtkWidget *previous,
                                                      GtkSignalFunc status_changed_func);

GtkWidget            *gck_pixmap_new                 (GdkPixmap *pixm,
                                                      GdkBitmap *mask,
                                                      GtkWidget *container);

GtkWidget            *gck_vbox_new                   (GtkWidget *Container,
                                                      gint homogenous,gint expand,gint fill,
                                                      gint spacing,gint padding,
                                                      gint borderwidth);

GtkWidget            *gck_hbox_new                   (GtkWidget *container,
                                                      gint homogenous,gint expand,gint fill,
                                                      gint spacing,gint padding,
                                                      gint borderwidth);

GtkWidget            *gck_menu_bar_new               (GtkWidget *container,
                                                      GckMenuItem menu_items[],
                                                      GtkAccelGroup *acc_group);

GtkWidget            *gck_menu_new                   (GckMenuItem *menu_items,
                                                      GtkAccelGroup *acc_group);

GtkWidget            *gck_option_menu_new            (char *name,GtkWidget *container,
                                                      gint expand,gint fill,
                                                      gint padding,
                                                      char *item_labels[],
                                                      GtkSignalFunc item_selected_func,
                                                      gpointer data);

GtkWidget            *gck_image_menu_new             (char *name,GtkWidget *container,
                                                      gint expand,gint fill,
                                                      gint padding,
                                                      gint constrain,
                                                      GtkSignalFunc item_selected_func);

#ifdef __cplusplus
}
#endif

#endif
