/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_IMAGE_WINDOW_H__
#define __LIGMA_IMAGE_WINDOW_H__


#include "widgets/ligmawindow.h"


#define LIGMA_TYPE_IMAGE_WINDOW            (ligma_image_window_get_type ())
#define LIGMA_IMAGE_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_IMAGE_WINDOW, LigmaImageWindow))
#define LIGMA_IMAGE_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_IMAGE_WINDOW, LigmaImageWindowClass))
#define LIGMA_IS_IMAGE_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_IMAGE_WINDOW))
#define LIGMA_IS_IMAGE_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_IMAGE_WINDOW))
#define LIGMA_IMAGE_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_IMAGE_WINDOW, LigmaImageWindowClass))


typedef struct _LigmaImageWindowClass  LigmaImageWindowClass;

struct _LigmaImageWindow
{
  LigmaWindow  parent_instance;
};

struct _LigmaImageWindowClass
{
  LigmaWindowClass  parent_class;
};


GType              ligma_image_window_get_type             (void) G_GNUC_CONST;

LigmaImageWindow  * ligma_image_window_new                  (Ligma              *ligma,
                                                           LigmaImage         *image,
                                                           LigmaDialogFactory *dialog_factory,
                                                           GdkMonitor        *monitor);
void               ligma_image_window_destroy              (LigmaImageWindow   *window);

LigmaUIManager    * ligma_image_window_get_ui_manager       (LigmaImageWindow  *window);
LigmaDockColumns  * ligma_image_window_get_left_docks       (LigmaImageWindow  *window);
LigmaDockColumns  * ligma_image_window_get_right_docks      (LigmaImageWindow  *window);

void               ligma_image_window_add_shell            (LigmaImageWindow  *window,
                                                           LigmaDisplayShell *shell);
LigmaDisplayShell * ligma_image_window_get_shell            (LigmaImageWindow  *window,
                                                           gint              index);
void               ligma_image_window_remove_shell         (LigmaImageWindow  *window,
                                                           LigmaDisplayShell *shell);

gint               ligma_image_window_get_n_shells         (LigmaImageWindow  *window);

void               ligma_image_window_set_active_shell     (LigmaImageWindow  *window,
                                                           LigmaDisplayShell *shell);
LigmaDisplayShell * ligma_image_window_get_active_shell     (LigmaImageWindow  *window);

void               ligma_image_window_set_fullscreen       (LigmaImageWindow  *window,
                                                           gboolean          fullscreen);
gboolean           ligma_image_window_get_fullscreen       (LigmaImageWindow  *window);

void               ligma_image_window_set_show_menubar     (LigmaImageWindow  *window,
                                                           gboolean          show);
gboolean           ligma_image_window_get_show_menubar     (LigmaImageWindow  *window);

void               ligma_image_window_set_show_statusbar   (LigmaImageWindow  *window,
                                                           gboolean          show);
gboolean           ligma_image_window_get_show_statusbar   (LigmaImageWindow  *window);

gboolean           ligma_image_window_is_iconified         (LigmaImageWindow  *window);
gboolean           ligma_image_window_is_maximized         (LigmaImageWindow  *window);

gboolean           ligma_image_window_has_toolbox          (LigmaImageWindow  *window);

void               ligma_image_window_shrink_wrap          (LigmaImageWindow  *window,
                                                           gboolean          grow_only);

GtkWidget        * ligma_image_window_get_default_dockbook (LigmaImageWindow  *window);

void               ligma_image_window_keep_canvas_pos      (LigmaImageWindow  *window);
void               ligma_image_window_suspend_keep_pos     (LigmaImageWindow  *window);
void               ligma_image_window_resume_keep_pos      (LigmaImageWindow  *window);

void               ligma_image_window_update_tabs          (LigmaImageWindow  *window);

#endif /* __LIGMA_IMAGE_WINDOW_H__ */
