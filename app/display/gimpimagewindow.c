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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "widgets/gimpmenufactory.h"
#include "widgets/gimpuimanager.h"

#include "gimpimagewindow.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_MENU_FACTORY
};


/*  local function prototypes  */

static GObject * gimp_image_window_constructor  (GType         type,
                                                 guint         n_params,
                                                 GObjectConstructParam *params);
static void      gimp_image_window_finalize     (GObject      *object);
static void      gimp_image_window_set_property (GObject      *object,
                                                 guint         property_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void      gimp_image_window_get_property (GObject      *object,
                                                 guint         property_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);

static void      gimp_image_window_destroy      (GtkObject    *object);


G_DEFINE_TYPE (GimpImageWindow, gimp_image_window, GIMP_TYPE_WINDOW)

#define parent_class gimp_image_window_parent_class


static void
gimp_image_window_class_init (GimpImageWindowClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_image_window_constructor;
  object_class->finalize     = gimp_image_window_finalize;
  object_class->set_property = gimp_image_window_set_property;
  object_class->get_property = gimp_image_window_get_property;

  gtk_object_class->destroy  = gimp_image_window_destroy;

  g_object_class_install_property (object_class, PROP_MENU_FACTORY,
                                   g_param_spec_object ("menu-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_MENU_FACTORY,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_image_window_init (GimpImageWindow *window)
{
}

static GObject *
gimp_image_window_constructor (GType                  type,
                               guint                  n_params,
                               GObjectConstructParam *params)
{
  GObject         *object;
  GimpImageWindow *window;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  window = GIMP_IMAGE_WINDOW (object);

  g_assert (GIMP_IS_UI_MANAGER (window->menubar_manager));

  return object;
}

static void
gimp_image_window_finalize (GObject *object)
{
  GimpImageWindow *window = GIMP_IMAGE_WINDOW (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_image_window_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpImageWindow *window = GIMP_IMAGE_WINDOW (object);

  switch (property_id)
    {
    case PROP_MENU_FACTORY:
      {
        GimpMenuFactory *factory = g_value_get_object (value);

        window->menubar_manager = gimp_menu_factory_manager_new (factory,
                                                                 "<Image>",
                                                                 window,
                                                                 FALSE);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_window_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpImageWindow *window = GIMP_IMAGE_WINDOW (object);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_image_window_destroy (GtkObject *object)
{
  GimpImageWindow *window = GIMP_IMAGE_WINDOW (object);

  if (window->menubar_manager)
    {
      g_object_unref (window->menubar_manager);
      window->menubar_manager = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}
