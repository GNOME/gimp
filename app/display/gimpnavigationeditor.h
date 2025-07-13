/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpnavigationeditor.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@gimp.org>
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

#pragma once

#include "widgets/gimpeditor.h"


#define GIMP_TYPE_NAVIGATION_EDITOR            (gimp_navigation_editor_get_type ())
#define GIMP_NAVIGATION_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_NAVIGATION_EDITOR, GimpNavigationEditor))
#define GIMP_NAVIGATION_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_NAVIGATION_EDITOR, GimpNavigationEditorClass))
#define GIMP_IS_NAVIGATION_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_NAVIGATION_EDITOR))
#define GIMP_IS_NAVIGATION_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_NAVIGATION_EDITOR))
#define GIMP_NAVIGATION_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_NAVIGATION_EDITOR, GimpNavigationEditorClass))


typedef struct _GimpNavigationEditorClass  GimpNavigationEditorClass;

struct _GimpNavigationEditor
{
  GimpEditor        parent_instance;

  GimpContext      *context;
  GimpDisplayShell *shell;

  GimpImageProxy   *image_proxy;

  GtkWidget        *view;
  GtkWidget        *zoom_label;
  GtkAdjustment    *zoom_adjustment;

  GtkWidget        *zoom_out_button;
  GtkWidget        *zoom_in_button;
  GtkWidget        *zoom_100_button;
  GtkWidget        *zoom_fit_in_button;
  GtkWidget        *zoom_fill_button;
  GtkWidget        *shrink_wrap_button;

  guint             scale_timeout;
};

struct _GimpNavigationEditorClass
{
  GimpEditorClass  parent_class;
};


GType       gimp_navigation_editor_get_type  (void) G_GNUC_CONST;

GtkWidget * gimp_navigation_editor_new       (GimpMenuFactory  *menu_factory);
void        gimp_navigation_editor_popup     (GimpDisplayShell *shell,
                                              GtkWidget        *widget,
                                              GdkEvent         *event,
                                              gint              click_x,
                                              gint              click_y);
