/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagedock.c
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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


static GObject * gimp_image_dock_constructor  (GType                  type,
                                               guint                  n_params,
                                               GObjectConstructParam *params);

static void      gimp_image_dock_destroy      (GtkObject             *object);

static void      gimp_image_dock_display_changed  (GimpContext       *context,
                                                   GimpObject        *display,
                                                   GimpImageDock     *dock);
static void      gimp_image_dock_image_flush      (GimpImage         *image,
                                                   gboolean           invalidate_preview,
                                                   GimpImageDock     *dock);

static void      gimp_image_dock_notify_transient (GimpConfig        *config,
                                                   GParamSpec        *pspec,
                                                   GimpDock          *dock);


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

  config = GIMP_GUI_CONFIG (GIMP_DOCK (dock)->context->gimp->config);

  dock->ui_manager =
    gimp_menu_factory_manager_new (GIMP_DOCK (dock)->dialog_factory->menu_factory,
                                   GIMP_IMAGE_DOCK_GET_CLASS (dock)->ui_manager_name,
                                   dock,
                                   config->tearoff_menus);

  accel_group =
    gtk_ui_manager_get_accel_group (GTK_UI_MANAGER (dock->ui_manager));

  gtk_window_add_accel_group (GTK_WINDOW (object), accel_group);

  dock->image_flush_handler_id =
    gimp_container_add_handler (GIMP_DOCK (dock)->context->gimp->images, "flush",
                                G_CALLBACK (gimp_image_dock_image_flush),
                                dock);

  g_signal_connect_object (GIMP_DOCK (dock)->context, "display-changed",
                           G_CALLBACK (gimp_image_dock_display_changed),
                           dock, 0);

  g_signal_connect_object (GIMP_DOCK (dock)->context->gimp->config,
                           "notify::transient-docks",
                           G_CALLBACK (gimp_image_dock_notify_transient),
                           dock, 0);

  return object;
}

static void
gimp_image_dock_destroy (GtkObject *object)
{
  GimpImageDock *dock = GIMP_IMAGE_DOCK (object);

  if (dock->image_flush_handler_id)
    {
      gimp_container_remove_handler (GIMP_DOCK (dock)->context->gimp->images,
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

  if (GIMP_GUI_CONFIG (context->gimp->config)->transient_docks)
    {
      GtkWindow *parent = NULL;

      if (display)
        g_object_get (display, "shell", &parent, NULL);

      gtk_window_set_transient_for (GTK_WINDOW (dock), parent);

      if (parent)
        g_object_unref (parent);
    }
}

static void
gimp_image_dock_image_flush (GimpImage     *image,
                             gboolean       invalidate_preview,
                             GimpImageDock *dock)
{
  if (image == gimp_context_get_image (GIMP_DOCK (dock)->context))
    {
      GimpObject *display = gimp_context_get_display (GIMP_DOCK (dock)->context);

      if (display)
        gimp_ui_manager_update (dock->ui_manager, display);
    }
}

static void
gimp_image_dock_notify_transient (GimpConfig *config,
                                  GParamSpec *pspec,
                                  GimpDock   *dock)
{
  if (GIMP_GUI_CONFIG (config)->transient_docks)
    {
      gimp_image_dock_display_changed (dock->context,
                                       gimp_context_get_display (dock->context),
                                       GIMP_IMAGE_DOCK (dock));
    }
  else
    {
      gtk_window_set_transient_for (GTK_WINDOW (dock), NULL);
    }
}
