/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockwindow.c
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"

#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockwindow.h"
#include "gimpmenufactory.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"
#include "gimpwindow.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_UI_MANAGER_NAME,
};


struct _GimpDockWindowPrivate
{
  GimpContext   *context;

  gchar         *ui_manager_name;
  GimpUIManager *ui_manager;
  GQuark         image_flush_handler_id;
};

static GObject * gimp_dock_window_constructor       (GType                  type,
                                                     guint                  n_params,
                                                     GObjectConstructParam *params);
static void      gimp_dock_window_dispose           (GObject               *object);
static void      gimp_dock_window_set_property      (GObject               *object,
                                                     guint                  property_id,
                                                     const GValue          *value,
                                                     GParamSpec            *pspec);
static void      gimp_dock_window_get_property      (GObject               *object,
                                                     guint                  property_id,
                                                     GValue                *value,
                                                     GParamSpec            *pspec);
static void      gimp_dock_window_display_changed   (GimpDockWindow        *dock_window,
                                                     GimpObject            *display,
                                                     GimpContext           *context);
static void      gimp_dock_window_image_changed     (GimpDockWindow        *dock_window,
                                                     GimpImage             *image,
                                                     GimpContext           *context);
static void      gimp_dock_window_image_flush       (GimpImage             *image,
                                                     gboolean               invalidate_preview,
                                                     GimpDockWindow        *dock_window);


G_DEFINE_TYPE (GimpDockWindow, gimp_dock_window, GIMP_TYPE_WINDOW)

#define parent_class gimp_dock_window_parent_class

static void
gimp_dock_window_class_init (GimpDockWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor     = gimp_dock_window_constructor;
  object_class->dispose         = gimp_dock_window_dispose;
  object_class->set_property    = gimp_dock_window_set_property;
  object_class->get_property    = gimp_dock_window_get_property;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("gimp-context", NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_MANAGER_NAME,
                                   g_param_spec_string ("ui-manager-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpDockWindowPrivate));
}

static void
gimp_dock_window_init (GimpDockWindow *dock_window)
{
  dock_window->p = G_TYPE_INSTANCE_GET_PRIVATE (dock_window,
                                                GIMP_TYPE_DOCK_WINDOW,
                                                GimpDockWindowPrivate);
  dock_window->p->context                = NULL;
  dock_window->p->ui_manager_name        = NULL;
  dock_window->p->ui_manager             = NULL;
  dock_window->p->image_flush_handler_id = 0;

  gtk_window_set_resizable (GTK_WINDOW (dock_window), TRUE);
  gtk_window_set_focus_on_map (GTK_WINDOW (dock_window), FALSE);
}

static GObject *
gimp_dock_window_constructor (GType                  type,
                              guint                  n_params,
                              GObjectConstructParam *params)
{
  GObject        *object;
  GimpDockWindow *dock_window;
  GimpGuiConfig  *config;
  GtkAccelGroup  *accel_group;

  /* Init */
  object      = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  dock_window = GIMP_DOCK_WINDOW (object);
  config      = GIMP_GUI_CONFIG (dock_window->p->context->gimp->config);

  /* Setup hints */
  gimp_window_set_hint (GTK_WINDOW (dock_window), config->dock_window_hint);

  /* Make image window related keyboard shortcuts work also when a
   * dock window is the focused window
   */
  dock_window->p->ui_manager =
    gimp_menu_factory_manager_new (gimp_dock_get_dialog_factory (GIMP_DOCK (dock_window))->menu_factory,
                                   dock_window->p->ui_manager_name,
                                   dock_window,
                                   config->tearoff_menus);
  accel_group =
    gtk_ui_manager_get_accel_group (GTK_UI_MANAGER (dock_window->p->ui_manager));

  gtk_window_add_accel_group (GTK_WINDOW (dock_window), accel_group);

  g_signal_connect_object (dock_window->p->context, "display-changed",
                           G_CALLBACK (gimp_dock_window_display_changed),
                           dock_window,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (dock_window->p->context, "image-changed",
                           G_CALLBACK (gimp_dock_window_image_changed),
                           dock_window,
                           G_CONNECT_SWAPPED);

  dock_window->p->image_flush_handler_id =
    gimp_container_add_handler (dock_window->p->context->gimp->images, "flush",
                                G_CALLBACK (gimp_dock_window_image_flush),
                                dock_window);

  /* Done! */
  return object;
}

static void
gimp_dock_window_dispose (GObject *object)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (object);

  if (dock_window->p->image_flush_handler_id)
    {
      gimp_container_remove_handler (dock_window->p->context->gimp->images,
                                     dock_window->p->image_flush_handler_id);
      dock_window->p->image_flush_handler_id = 0;
    }

  if (dock_window->p->ui_manager)
    {
      g_object_unref (dock_window->p->ui_manager);
      dock_window->p->ui_manager = NULL;
    }

  if (dock_window->p->context)
    {
      g_object_unref (dock_window->p->context);
      dock_window->p->context = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_dock_window_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      dock_window->p->context = g_value_dup_object (value);
      break;

    case PROP_UI_MANAGER_NAME:
      g_free (dock_window->p->ui_manager_name);
      dock_window->p->ui_manager_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dock_window_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, dock_window->p->context);
      break;

    case PROP_UI_MANAGER_NAME:
      g_value_set_string (value, dock_window->p->ui_manager_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dock_window_display_changed (GimpDockWindow *dock_window,
                                  GimpObject     *display,
                                  GimpContext    *context)
{
  gimp_ui_manager_update (dock_window->p->ui_manager,
                          display);
}

static void
gimp_dock_window_image_changed (GimpDockWindow *dock_window,
                                GimpImage     *image,
                                GimpContext   *context)
{
  gimp_ui_manager_update (dock_window->p->ui_manager,
                          gimp_context_get_display (context));
}

static void
gimp_dock_window_image_flush (GimpImage      *image,
                              gboolean        invalidate_preview,
                              GimpDockWindow *dock_window)
{
  if (image == gimp_context_get_image (dock_window->p->context))
    {
      GimpObject *display = gimp_context_get_display (dock_window->p->context);

      if (display)
        gimp_ui_manager_update (dock_window->p->ui_manager, display);
    }
}

GimpUIManager *
gimp_dock_window_get_ui_manager (GimpDockWindow *dock_window)
{
  g_return_val_if_fail (GIMP_IS_DOCK_WINDOW (dock_window), NULL);

  return dock_window->p->ui_manager;
}
