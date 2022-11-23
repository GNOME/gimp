/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadockwindow.h
 * Copyright (C) 2001-2005 Michael Natterer <mitch@ligma.org>
 * Copyright (C)      2009 Martin Nordholts <martinn@src.gnome.org>
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

#ifndef __LIGMA_DOCK_WINDOW_H__
#define __LIGMA_DOCK_WINDOW_H__


#include "widgets/ligmawindow.h"


#define LIGMA_TYPE_DOCK_WINDOW            (ligma_dock_window_get_type ())
#define LIGMA_DOCK_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DOCK_WINDOW, LigmaDockWindow))
#define LIGMA_DOCK_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DOCK_WINDOW, LigmaDockWindowClass))
#define LIGMA_IS_DOCK_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DOCK_WINDOW))
#define LIGMA_IS_DOCK_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DOCK_WINDOW))
#define LIGMA_DOCK_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DOCK_WINDOW, LigmaDockWindowClass))


typedef struct _LigmaDockWindowClass    LigmaDockWindowClass;
typedef struct _LigmaDockWindowPrivate  LigmaDockWindowPrivate;

/**
 * LigmaDockWindow:
 *
 * A top-level window containing LigmaDocks.
 */
struct _LigmaDockWindow
{
  LigmaWindow  parent_instance;

  LigmaDockWindowPrivate *p;
};

struct _LigmaDockWindowClass
{
  LigmaWindowClass  parent_class;
};


GType               ligma_dock_window_get_type               (void) G_GNUC_CONST;
GtkWidget         * ligma_dock_window_new                    (const gchar       *role,
                                                             const gchar       *ui_manager_name,
                                                             gboolean           allow_dockbook_absence,
                                                             LigmaDialogFactory *factory,
                                                             LigmaContext       *context);
gint                ligma_dock_window_get_id                 (LigmaDockWindow    *dock_window);
void                ligma_dock_window_add_dock               (LigmaDockWindow    *dock_window,
                                                             LigmaDock          *dock,
                                                             gint               index);
void                ligma_dock_window_remove_dock            (LigmaDockWindow    *dock_window,
                                                             LigmaDock          *dock);
LigmaContext       * ligma_dock_window_get_context            (LigmaDockWindow    *dock);
gboolean            ligma_dock_window_get_auto_follow_active (LigmaDockWindow    *menu_dock);
void                ligma_dock_window_set_auto_follow_active (LigmaDockWindow    *menu_dock,
                                                             gboolean           show);
gboolean            ligma_dock_window_get_show_image_menu    (LigmaDockWindow    *menu_dock);
void                ligma_dock_window_set_show_image_menu    (LigmaDockWindow    *menu_dock,
                                                             gboolean           show);
void                ligma_dock_window_setup                  (LigmaDockWindow    *dock_window,
                                                             LigmaDockWindow    *template);
gboolean            ligma_dock_window_has_toolbox            (LigmaDockWindow    *dock_window);

LigmaDockWindow    * ligma_dock_window_from_dock              (LigmaDock          *dock);



#endif /* __LIGMA_DOCK_WINDOW_H__ */
