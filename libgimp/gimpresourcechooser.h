/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_RESOURCE_CHOOSER_H__
#define __GIMP_RESOURCE_CHOOSER_H__

G_BEGIN_DECLS

#define GIMP_TYPE_RESOURCE_CHOOSER (gimp_resource_chooser_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpResourceChooser, gimp_resource_chooser, GIMP, RESOURCE_CHOOSER, GtkBox)

struct _GimpResourceChooserClass
{
  GtkBoxClass parent_class;

  /* Signals */
  void (* resource_set) (GimpResourceChooser *chooser,
                         GimpResource        *resource,
                         gboolean             dialog_closing);

  /* Abstract methods and class variables */
  void (*draw_interior) (GimpResourceChooser *chooser);

  GType resource_type;

  /* Padding for future expansion */
  gpointer padding[8];
};

GimpResource * gimp_resource_chooser_get_resource     (GimpResourceChooser  *chooser);
void           gimp_resource_chooser_set_resource     (GimpResourceChooser  *chooser,
                                                       GimpResource         *resource);
GtkWidget    * gimp_resource_chooser_get_label        (GimpResourceChooser  *widget);


/* API from below, used by subclasses e.g. GimpBrushChooser */

G_GNUC_INTERNAL void  _gimp_resource_chooser_set_drag_target (GimpResourceChooser  *chooser,
                                                              GtkWidget            *drag_region_widget,
                                                              const GtkTargetEntry *drag_target);
G_GNUC_INTERNAL void  _gimp_resource_chooser_set_clickable   (GimpResourceChooser  *chooser,
                                                              GtkWidget            *widget);

G_END_DECLS

#endif /* __GIMP_RESOURCE_CHOOSER_H__ */
