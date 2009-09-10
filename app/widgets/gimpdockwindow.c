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

#include "gimpdockwindow.h"
#include "gimpwidgets-utils.h"
#include "gimpwindow.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
};


struct _GimpDockWindowPrivate
{
  GimpContext *context;
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

  g_type_class_add_private (klass, sizeof (GimpDockWindowPrivate));
}

static void
gimp_dock_window_init (GimpDockWindow *dock_window)
{
  dock_window->p = G_TYPE_INSTANCE_GET_PRIVATE (dock_window,
                                                GIMP_TYPE_DOCK_WINDOW,
                                                GimpDockWindowPrivate);
  dock_window->p->context = NULL;
}

static GObject *
gimp_dock_window_constructor (GType                  type,
                              guint                  n_params,
                              GObjectConstructParam *params)
{
  GObject        *object;
  GimpDockWindow *dock_window;
  GimpGuiConfig  *config;

  object      = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  dock_window = GIMP_DOCK_WINDOW (object);
  config      = GIMP_GUI_CONFIG (dock_window->p->context->gimp->config);

  gimp_window_set_hint (GTK_WINDOW (dock_window), config->dock_window_hint);

  return object;
}

static void
gimp_dock_window_dispose (GObject *object)
{
  GimpDockWindow *dock_window = GIMP_DOCK_WINDOW (object);

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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
