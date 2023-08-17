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

#ifndef __GIMP_RESOURCE_SELECT_BUTTON_H__
#define __GIMP_RESOURCE_SELECT_BUTTON_H__

G_BEGIN_DECLS

#define GIMP_TYPE_RESOURCE_SELECT_BUTTON (gimp_resource_select_button_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpResourceSelectButton, gimp_resource_select_button, GIMP, RESOURCE_SELECT_BUTTON, GtkBox)

struct _GimpResourceSelectButtonClass
{
  GtkBoxClass parent_class;

  /* Signals */
  void (* resource_set) (GimpResourceSelectButton *self,
                         GimpResource             *resource,
                         gboolean                  dialog_closing);

  /* Abstract methods and class variables */
  void (*draw_interior) (GimpResourceSelectButton *self);

  GType resource_type;

  /* Padding for future expansion */
  gpointer padding[8];
};

GimpResource * gimp_resource_select_button_get_resource     (GimpResourceSelectButton *self);
void           gimp_resource_select_button_set_resource     (GimpResourceSelectButton *self,
                                                             GimpResource             *resource);
GtkWidget    * gimp_resource_select_button_get_label        (GimpResourceSelectButton *widget);


/* API from below, used by subclasses e.g. GimpBrushSelectButton */

void           gimp_resource_select_button_set_drag_target  (GimpResourceSelectButton *self,
                                                             GtkWidget                *drag_region_widget,
                                                             const GtkTargetEntry     *drag_target);
void           gimp_resource_select_button_set_clickable    (GimpResourceSelectButton *self,
                                                             GtkWidget                *widget);

G_END_DECLS

#endif /* __GIMP_RESOURCE_SELECT_BUTTON_H__ */
