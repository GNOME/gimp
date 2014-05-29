/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimp3migration.h
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_3_MIGRATION_H__
#define __GIMP_3_MIGRATION_H__


/* This file is evil. Its purpose is to keep GIMP's gtk3-port branch
 * manageable, and contains functions that are only in GTK+ 3.x but
 * are *not* in GTK+ 2.x. Please just ignore the uglyness and move
 * along. This file will be removed in GIMP 3.
 */

#if 0

GtkWidget * gtk_box_new        (GtkOrientation  orientation,
                                gint            spacing);
GtkWidget * gtk_button_box_new (GtkOrientation  orientation);
GtkWidget * gtk_paned_new      (GtkOrientation  orientation);
GtkWidget * gtk_scale_new      (GtkOrientation  orientation,
                                GtkAdjustment  *adjustment);
GtkWidget * gtk_scrollbar_new  (GtkOrientation  orientation,
                                GtkAdjustment  *adjustment);
GtkWidget * gtk_separator_new  (GtkOrientation  orientation);


typedef enum
{
  GDK_MODIFIER_INTENT_PRIMARY_ACCELERATOR,
  GDK_MODIFIER_INTENT_CONTEXT_MENU,
  GDK_MODIFIER_INTENT_EXTEND_SELECTION,
  GDK_MODIFIER_INTENT_MODIFY_SELECTION,
  GDK_MODIFIER_INTENT_NO_TEXT_INPUT
} GdkModifierIntent;

gboolean        gdk_event_triggers_context_menu (const GdkEvent    *event);
GdkModifierType gdk_keymap_get_modifier_mask    (GdkKeymap         *keymap,
                                                 GdkModifierIntent  intent);
GdkModifierType gtk_widget_get_modifier_mask    (GtkWidget         *widget,
                                                 GdkModifierIntent  intent);

gboolean        gdk_cairo_get_clip_rectangle    (cairo_t           *cr,
                                                 GdkRectangle      *rect);
void            gdk_screen_get_monitor_workarea (GdkScreen         *screen,
                                                 gint               monitor_num,
                                                 GdkRectangle      *dest);

void            gtk_label_set_xalign            (GtkLabel          *label,
                                                 gfloat             xalign);
gfloat          gtk_label_get_xalign            (GtkLabel          *label);

void            gtk_label_set_yalign            (GtkLabel          *label,
                                                 gfloat             yalign);
gfloat          gtk_label_get_yalign            (GtkLabel          *label);

#endif


#endif /* __GIMP_3_MIGRATION_H__ */
