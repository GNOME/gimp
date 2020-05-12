/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpThrobber
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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

#include <gtk/gtk.h>

#include "gimpthrobber.h"


enum
{
  CLICKED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ICON_NAME,
  PROP_IMAGE
};
static guint         toolbutton_signals[LAST_SIGNAL] = { 0 };


static void      gimp_throbber_set_property         (GObject      *object,
                                                     guint         prop_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void      gimp_throbber_get_property         (GObject      *object,
                                                     guint         prop_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);
static void      gimp_throbber_finalize             (GObject      *object);

static gboolean  gimp_throbber_create_menu_proxy    (GtkToolItem  *tool_item);
static void      gimp_throbber_toolbar_reconfigured (GtkToolItem  *tool_item);
static void      gimp_throbber_button_clicked       (GtkWidget    *widget,
                                                     GimpThrobber *button);

static void      gimp_throbber_construct_contents   (GtkToolItem  *tool_item);



struct _GimpThrobberPrivate
{
  GtkWidget *button;
  GtkWidget *image;
  gchar     *icon_name;
};

G_DEFINE_TYPE_WITH_PRIVATE (GimpThrobber, gimp_throbber, GTK_TYPE_TOOL_ITEM)

static void
gimp_throbber_class_init (GimpThrobberClass *klass)
{
  GObjectClass     *object_class    = G_OBJECT_CLASS (klass);
  GtkToolItemClass *tool_item_class = GTK_TOOL_ITEM_CLASS (klass);

  object_class->set_property = gimp_throbber_set_property;
  object_class->get_property = gimp_throbber_get_property;
  object_class->finalize     = gimp_throbber_finalize;

  tool_item_class->create_menu_proxy    = gimp_throbber_create_menu_proxy;
  tool_item_class->toolbar_reconfigured = gimp_throbber_toolbar_reconfigured;

  g_object_class_install_property (object_class,
                                   PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        GTK_TYPE_IMAGE,
                                                        G_PARAM_READWRITE));

  toolbutton_signals[CLICKED] =
    g_signal_new ("clicked",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpThrobberClass, clicked),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
gimp_throbber_init (GimpThrobber *button)
{
  GimpThrobberPrivate *priv = gimp_throbber_get_instance_private (button);
  GtkToolItem *toolitem = GTK_TOOL_ITEM (button);

  gtk_tool_item_set_homogeneous (toolitem, TRUE);

  priv->button = g_object_new (GTK_TYPE_BUTTON,
                               "yalign",         0.0,
                               "focus-on-click", FALSE,
                               NULL);

  g_signal_connect_object (priv->button, "clicked",
                           G_CALLBACK (gimp_throbber_button_clicked),
                           button, 0);

  gtk_container_add (GTK_CONTAINER (button), priv->button);
  gtk_widget_show (priv->button);
}

static void
gimp_throbber_construct_contents (GtkToolItem *tool_item)
{
  GimpThrobber        *button = GIMP_THROBBER (tool_item);
  GimpThrobberPrivate *priv = gimp_throbber_get_instance_private (button);
  GtkWidget           *image;
  GtkToolbarStyle      style;

  if (priv->image && gtk_widget_get_parent (priv->image))
    gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (priv->image)),
                          priv->image);

  if (gtk_bin_get_child (GTK_BIN (priv->button)))
    gtk_widget_destroy (gtk_bin_get_child (GTK_BIN (priv->button)));

  style = gtk_tool_item_get_toolbar_style (tool_item);

  if (style == GTK_TOOLBAR_TEXT)
    {
      image = gtk_image_new_from_icon_name (priv->icon_name,
                                            GTK_ICON_SIZE_MENU);
    }
  else if (style == GTK_TOOLBAR_ICONS)
    {
      image = gtk_image_new_from_icon_name (priv->icon_name,
                                            GTK_ICON_SIZE_LARGE_TOOLBAR);
    }
  else if (priv->image)
    {
      image = priv->image;
    }
  else
    {
      image = gtk_image_new_from_icon_name (priv->icon_name,
                                            GTK_ICON_SIZE_DND);
    }

  gtk_container_add (GTK_CONTAINER (priv->button), image);
  gtk_widget_show (image);

  gtk_button_set_relief (GTK_BUTTON (priv->button),
                         gtk_tool_item_get_relief_style (tool_item));

  gtk_widget_queue_resize (GTK_WIDGET (button));
}

static void
gimp_throbber_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpThrobber *button = GIMP_THROBBER (object);

  switch (prop_id)
    {
    case PROP_ICON_NAME:
      gimp_throbber_set_icon_name (button, g_value_get_string (value));
      break;

    case PROP_IMAGE:
      gimp_throbber_set_image (button, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_throbber_get_property (GObject         *object,
                              guint            prop_id,
                              GValue          *value,
                              GParamSpec      *pspec)
{
  GimpThrobber *button = GIMP_THROBBER (object);
  GimpThrobberPrivate *priv = gimp_throbber_get_instance_private (button);

  switch (prop_id)
    {
    case PROP_ICON_NAME:
      g_value_set_string (value, priv->icon_name);
      break;

    case PROP_IMAGE:
      g_value_set_object (value, priv->image);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_throbber_finalize (GObject *object)
{
  GimpThrobber *button = GIMP_THROBBER (object);
  GimpThrobberPrivate *priv = gimp_throbber_get_instance_private (button);

  g_free (priv->icon_name);
  g_clear_object (&priv->image);

  G_OBJECT_CLASS (gimp_throbber_parent_class)->finalize (object);
}

static void
gimp_throbber_button_clicked (GtkWidget    *widget,
                              GimpThrobber *button)
{
  g_signal_emit_by_name (button, "clicked");
}

static gboolean
gimp_throbber_create_menu_proxy (GtkToolItem *tool_item)
{
  gtk_tool_item_set_proxy_menu_item (tool_item, "gimp-throbber-menu-id", NULL);

  return FALSE;
}

static void
gimp_throbber_toolbar_reconfigured (GtkToolItem *tool_item)
{
  gimp_throbber_construct_contents (tool_item);
}

GtkToolItem *
gimp_throbber_new (const gchar *icon_name)
{
  return g_object_new (GIMP_TYPE_THROBBER,
                       "icon-name", icon_name,
                       NULL);
}

void
gimp_throbber_set_icon_name (GimpThrobber *button,
                             const gchar  *icon_name)
{
  GimpThrobberPrivate *priv = gimp_throbber_get_instance_private (button);
  gchar *old_icon_name;

  g_return_if_fail (GIMP_IS_THROBBER (button));

  old_icon_name = priv->icon_name;

  priv->icon_name = g_strdup (icon_name);
  gimp_throbber_construct_contents (GTK_TOOL_ITEM (button));

  g_object_notify (G_OBJECT (button), "icon-name");

  g_free (old_icon_name);
}

const gchar *
gimp_throbber_get_icon_name (GimpThrobber *button)
{
  GimpThrobberPrivate *priv = gimp_throbber_get_instance_private (button);

  g_return_val_if_fail (GIMP_IS_THROBBER (button), NULL);

  return priv->icon_name;
}

void
gimp_throbber_set_image (GimpThrobber *button,
                         GtkWidget    *image)
{
  GimpThrobberPrivate *priv = gimp_throbber_get_instance_private (button);

  g_return_if_fail (GIMP_IS_THROBBER (button));
  g_return_if_fail (image == NULL || GTK_IS_IMAGE (image));

  if (image != priv->image)
    {
      if (priv->image)
        {
          if (gtk_widget_get_parent (priv->image))
                gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (priv->image)),
                                      priv->image);

          g_object_unref (priv->image);
        }

      if (image)
        g_object_ref_sink (image);

      priv->image = image;

      gimp_throbber_construct_contents (GTK_TOOL_ITEM (button));

      g_object_notify (G_OBJECT (button), "image");
    }
}

GtkWidget *
gimp_throbber_get_image (GimpThrobber *button)
{
  GimpThrobberPrivate *priv = gimp_throbber_get_instance_private (button);

  g_return_val_if_fail (GIMP_IS_THROBBER (button), NULL);

  return priv->image;
}
