/* GIMP - The GNU Image Manipulation Program
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

#pragma once

#include "widgets/gimpwindow.h"


#define GIMP_TYPE_IMAGE_WINDOW            (gimp_image_window_get_type ())
#define GIMP_IMAGE_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_WINDOW, GimpImageWindow))
#define GIMP_IMAGE_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_IMAGE_WINDOW, GimpImageWindowClass))
#define GIMP_IS_IMAGE_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_WINDOW))
#define GIMP_IS_IMAGE_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_IMAGE_WINDOW))
#define GIMP_IMAGE_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_IMAGE_WINDOW, GimpImageWindowClass))


typedef struct _GimpImageWindowClass  GimpImageWindowClass;

struct _GimpImageWindow
{
  GimpWindow  parent_instance;
};

struct _GimpImageWindowClass
{
  GimpWindowClass  parent_class;
};


GType              gimp_image_window_get_type             (void) G_GNUC_CONST;

GimpImageWindow  * gimp_image_window_new                  (Gimp              *gimp,
                                                           GimpImage         *image,
                                                           GimpDialogFactory *dialog_factory,
                                                           GdkMonitor        *monitor);
void               gimp_image_window_destroy              (GimpImageWindow   *window);

GimpDockColumns  * gimp_image_window_get_left_docks       (GimpImageWindow  *window);
GimpDockColumns  * gimp_image_window_get_right_docks      (GimpImageWindow  *window);

void               gimp_image_window_add_shell            (GimpImageWindow  *window,
                                                           GimpDisplayShell *shell);
GimpDisplayShell * gimp_image_window_get_shell            (GimpImageWindow  *window,
                                                           gint              index);
void               gimp_image_window_remove_shell         (GimpImageWindow  *window,
                                                           GimpDisplayShell *shell);

gint               gimp_image_window_get_n_shells         (GimpImageWindow  *window);

void               gimp_image_window_set_active_shell     (GimpImageWindow  *window,
                                                           GimpDisplayShell *shell);
GimpDisplayShell * gimp_image_window_get_active_shell     (GimpImageWindow  *window);

GimpMenuModel    * gimp_image_window_get_menubar_model    (GimpImageWindow  *window);

void               gimp_image_window_set_fullscreen       (GimpImageWindow  *window,
                                                           gboolean          fullscreen);
gboolean           gimp_image_window_get_fullscreen       (GimpImageWindow  *window);

void               gimp_image_window_set_show_menubar     (GimpImageWindow  *window,
                                                           gboolean          show);
gboolean           gimp_image_window_get_show_menubar     (GimpImageWindow  *window);

void               gimp_image_window_set_show_statusbar   (GimpImageWindow  *window,
                                                           gboolean          show);
gboolean           gimp_image_window_get_show_statusbar   (GimpImageWindow  *window);

gboolean           gimp_image_window_is_iconified         (GimpImageWindow  *window);
gboolean           gimp_image_window_is_maximized         (GimpImageWindow  *window);

gboolean           gimp_image_window_has_toolbox          (GimpImageWindow  *window);

void               gimp_image_window_shrink_wrap          (GimpImageWindow  *window,
                                                           gboolean          grow_only);

GtkWidget        * gimp_image_window_get_default_dockbook (GimpImageWindow  *window);

void               gimp_image_window_keep_canvas_pos      (GimpImageWindow  *window);
void               gimp_image_window_suspend_keep_pos     (GimpImageWindow  *window);
void               gimp_image_window_resume_keep_pos      (GimpImageWindow  *window);

void               gimp_image_window_update_tabs          (GimpImageWindow  *window);
