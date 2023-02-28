/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenubar.c
 * Copyright (C) 2023 Jehan
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "gimpmenubar.h"
#include "gimpmenushell.h"
#include "gimpuimanager.h"


/**
 * GimpMenuBar:
 *
 * Our own menu bar widget.
 *
 * We cannot use the simpler gtk_application_set_menubar() because it lacks
 * tooltip support and unfortunately GTK does not plan to implement this:
 * https://gitlab.gnome.org/GNOME/gtk/-/issues/785
 * This is why we need to implement our own GimpMenuBar subclass.
 */

enum
{
  PROP_0 = GIMP_MENU_SHELL_PROP_LAST,
  PROP_MODEL
};


struct _GimpMenuBarPrivate
{
  GMenuModel *model;
};


/*  local function prototypes  */

static void   gimp_menu_bar_constructed             (GObject             *object);
static void   gimp_menu_bar_dispose                 (GObject             *object);
static void   gimp_menu_bar_set_property            (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void   gimp_menu_bar_get_property            (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpMenuBar, gimp_menu_bar, GTK_TYPE_MENU_BAR,
                         G_ADD_PRIVATE (GimpMenuBar)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_MENU_SHELL, NULL))

#define parent_class gimp_menu_bar_parent_class


/*  private functions  */

static void
gimp_menu_bar_class_init (GimpMenuBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_menu_bar_constructed;
  object_class->dispose      = gimp_menu_bar_dispose;
  object_class->get_property = gimp_menu_bar_get_property;
  object_class->set_property = gimp_menu_bar_set_property;

  gimp_menu_shell_install_properties (object_class);

  g_object_class_install_property (object_class, PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        NULL, NULL,
                                                        G_TYPE_MENU_MODEL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_menu_bar_init (GimpMenuBar *bar)
{
  bar->priv = gimp_menu_bar_get_instance_private (bar);

  gimp_menu_shell_init (GIMP_MENU_SHELL (bar));
}

static void
gimp_menu_bar_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
gimp_menu_bar_dispose (GObject *object)
{
  GimpMenuBar *bar = GIMP_MENU_BAR (object);

  g_clear_object (&bar->priv->model);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_menu_bar_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpMenuBar *bar = GIMP_MENU_BAR (object);

  switch (property_id)
    {
    case PROP_MODEL:
      bar->priv->model = g_value_dup_object (value);
      gimp_menu_shell_fill (GIMP_MENU_SHELL (bar), bar->priv->model, "ui-added", FALSE);
      break;

    default:
      gimp_menu_shell_set_property (object, property_id, value, pspec);
      break;
    }
}

static void
gimp_menu_bar_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpMenuBar *bar = GIMP_MENU_BAR (object);

  switch (property_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, bar->priv->model);
      break;

    default:
      gimp_menu_shell_get_property (object, property_id, value, pspec);
      break;
    }
}


/* Public functions */

GtkWidget *
gimp_menu_bar_new (GMenuModel    *model,
                   GimpUIManager *manager)
{
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (manager) &&
                        G_IS_MENU_MODEL (model), NULL);

  return g_object_new (GIMP_TYPE_MENU_BAR,
                       "model",   model,
                       "manager", manager,
                       NULL);
}
