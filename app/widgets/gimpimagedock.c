/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagedock.c
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"

#include "gimpdialogfactory.h"
#include "gimpimagedock.h"
#include "gimpmenufactory.h"
#include "gimpuimanager.h"

#include "gimp-log.h"


static GObject * gimp_image_dock_constructor  (GType                  type,
                                               guint                  n_params,
                                               GObjectConstructParam *params);

static void      gimp_image_dock_destroy      (GtkObject             *object);

static void      gimp_image_dock_display_changed  (GimpContext       *context,
                                                   GimpObject        *display,
                                                   GimpImageDock     *dock);
static void      gimp_image_dock_image_changed    (GimpContext       *context,
                                                   GimpImage         *image,
                                                   GimpImageDock     *dock);
static void      gimp_image_dock_image_flush      (GimpImage         *image,
                                                   gboolean           invalidate_preview,
                                                   GimpImageDock     *dock);


G_DEFINE_TYPE (GimpImageDock, gimp_image_dock, GIMP_TYPE_DOCK)

#define parent_class gimp_image_dock_parent_class


static void
gimp_image_dock_class_init (GimpImageDockClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);

  object_class->constructor = gimp_image_dock_constructor;

  gtk_object_class->destroy = gimp_image_dock_destroy;

  klass->ui_manager_name    = "<Dock>";
}

static void
gimp_image_dock_init (GimpImageDock *dock)
{
  dock->ui_manager             = NULL;
  dock->image_flush_handler_id = 0;
}

static GObject *
gimp_image_dock_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject       *object;
  GimpImageDock *dock;
  GimpGuiConfig *config;
  GtkAccelGroup *accel_group;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  dock = GIMP_IMAGE_DOCK (object);

  config = GIMP_GUI_CONFIG (gimp_dock_get_context (GIMP_DOCK (dock))->gimp->config);

  dock->ui_manager =
    gimp_menu_factory_manager_new (gimp_dock_get_dialog_factory (GIMP_DOCK (dock))->menu_factory,
                                   GIMP_IMAGE_DOCK_GET_CLASS (dock)->ui_manager_name,
                                   dock,
                                   config->tearoff_menus);

  accel_group =
    gtk_ui_manager_get_accel_group (GTK_UI_MANAGER (dock->ui_manager));

  gtk_window_add_accel_group (GTK_WINDOW (object), accel_group);

  dock->image_flush_handler_id =
    gimp_container_add_handler (gimp_dock_get_context (GIMP_DOCK (dock))->gimp->images, "flush",
                                G_CALLBACK (gimp_image_dock_image_flush),
                                dock);

  g_signal_connect_object (gimp_dock_get_context (GIMP_DOCK (dock)), "display-changed",
                           G_CALLBACK (gimp_image_dock_display_changed),
                           dock, 0);
  g_signal_connect_object (gimp_dock_get_context (GIMP_DOCK (dock)), "image-changed",
                           G_CALLBACK (gimp_image_dock_image_changed),
                           dock, 0);

  return object;
}

static void
gimp_image_dock_destroy (GtkObject *object)
{
  GimpImageDock *dock = GIMP_IMAGE_DOCK (object);

  if (dock->image_flush_handler_id)
    {
      gimp_container_remove_handler (gimp_dock_get_context (GIMP_DOCK (dock))->gimp->images,
                                     dock->image_flush_handler_id);
      dock->image_flush_handler_id = 0;
    }

  if (dock->ui_manager)
    {
      g_object_unref (dock->ui_manager);
      dock->ui_manager = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_image_dock_display_changed (GimpContext   *context,
                                 GimpObject    *display,
                                 GimpImageDock *dock)
{
  gimp_ui_manager_update (dock->ui_manager, display);
}

static void
gimp_image_dock_image_changed (GimpContext   *context,
                               GimpImage     *image,
                               GimpImageDock *dock)
{
  gimp_ui_manager_update (dock->ui_manager, gimp_context_get_display (context));
}

static void
gimp_image_dock_image_flush (GimpImage     *image,
                             gboolean       invalidate_preview,
                             GimpImageDock *dock)
{
  if (image == gimp_context_get_image (gimp_dock_get_context (GIMP_DOCK (dock))))
    {
      GimpObject *display = gimp_context_get_display (gimp_dock_get_context (GIMP_DOCK (dock)));

      if (display)
        gimp_ui_manager_update (dock->ui_manager, display);
    }
}

GimpUIManager *
gimp_image_dock_get_ui_manager (GimpImageDock *image_dock)
{
  g_return_val_if_fail (GIMP_IS_IMAGE_DOCK (image_dock), NULL);

  return image_dock->ui_manager;
}
