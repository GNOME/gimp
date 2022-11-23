/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmanavigationeditor.h
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@ligma.org>
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

#ifndef __LIGMA_NAVIGATION_EDITOR_H__
#define __LIGMA_NAVIGATION_EDITOR_H__


#include "widgets/ligmaeditor.h"


#define LIGMA_TYPE_NAVIGATION_EDITOR            (ligma_navigation_editor_get_type ())
#define LIGMA_NAVIGATION_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_NAVIGATION_EDITOR, LigmaNavigationEditor))
#define LIGMA_NAVIGATION_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_NAVIGATION_EDITOR, LigmaNavigationEditorClass))
#define LIGMA_IS_NAVIGATION_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_NAVIGATION_EDITOR))
#define LIGMA_IS_NAVIGATION_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_NAVIGATION_EDITOR))
#define LIGMA_NAVIGATION_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_NAVIGATION_EDITOR, LigmaNavigationEditorClass))


typedef struct _LigmaNavigationEditorClass  LigmaNavigationEditorClass;

struct _LigmaNavigationEditor
{
  LigmaEditor        parent_instance;

  LigmaContext      *context;
  LigmaDisplayShell *shell;

  LigmaImageProxy   *image_proxy;

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

struct _LigmaNavigationEditorClass
{
  LigmaEditorClass  parent_class;
};


GType       ligma_navigation_editor_get_type  (void) G_GNUC_CONST;

GtkWidget * ligma_navigation_editor_new       (LigmaMenuFactory  *menu_factory);
void        ligma_navigation_editor_popup     (LigmaDisplayShell *shell,
                                              GtkWidget        *widget,
                                              GdkEvent         *event,
                                              gint              click_x,
                                              gint              click_y);


#endif  /*  __LIGMA_NAVIGATION_EDITOR_H__  */
