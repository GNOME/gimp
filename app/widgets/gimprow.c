/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprow.c
 * Copyright (C) 2001-2006 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/gimpviewable.h"

#include "gimprow.h"


enum
{
  PROP_0,
  PROP_VIEWABLE,
  N_PROPS
};

static GParamSpec *obj_props[N_PROPS] = { NULL, };


typedef struct _GimpRowPrivate GimpRowPrivate;

struct _GimpRowPrivate
{
  GimpViewable *viewable;

  GtkWidget    *icon;
  GtkWidget    *label;
};

#define GET_PRIVATE(obj) \
  ((GimpRowPrivate *) gimp_row_get_instance_private ((GimpRow *) obj))


static void   gimp_row_constructed           (GObject          *object);
static void   gimp_row_dispose               (GObject          *object);
static void   gimp_row_set_property          (GObject          *object,
                                              guint             property_id,
                                              const GValue     *value,
                                              GParamSpec       *pspec);
static void   gimp_row_get_property          (GObject          *object,
                                              guint             property_id,
                                              GValue           *value,
                                              GParamSpec       *pspec);

static void   gimp_row_viewable_icon_changed (GimpViewable     *viewable,
                                              const GParamSpec *pspec,
                                              GimpRow          *row);
static void   gimp_row_viewable_name_changed (GimpViewable     *viewable,
                                              GimpRow          *row);


G_DEFINE_TYPE_WITH_PRIVATE (GimpRow, gimp_row, GTK_TYPE_LIST_BOX_ROW)

#define parent_class gimp_row_parent_class


static void
gimp_row_class_init (GimpRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_row_constructed;
  object_class->dispose      = gimp_row_dispose;
  object_class->set_property = gimp_row_set_property;
  object_class->get_property = gimp_row_get_property;

  obj_props[PROP_VIEWABLE] =
    g_param_spec_object ("viewable",
                         NULL, NULL,
                         GIMP_TYPE_VIEWABLE,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT);

  g_object_class_install_properties (object_class, N_PROPS, obj_props);
}

static void
gimp_row_init (GimpRow *row)
{
}

static void
gimp_row_constructed (GObject *object)
{
  GimpRowPrivate *priv = GET_PRIVATE (object);
  GtkWidget      *box;
  const gchar    *icon_name;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (object), box);
  gtk_widget_set_visible (box, TRUE);

  icon_name = gimp_viewable_get_icon_name (GIMP_VIEWABLE (priv->viewable));
  priv->icon = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (box), priv->icon, FALSE, FALSE, 0);
  gtk_widget_set_visible (priv->icon, TRUE);

  priv->label = gtk_label_new (gimp_object_get_name (priv->viewable));
  gtk_label_set_xalign (GTK_LABEL (priv->label), 0.0);
  gtk_box_pack_start (GTK_BOX (box), priv->label, TRUE, TRUE, 0);
  gtk_widget_set_visible (priv->label, TRUE);
}

static void
gimp_row_dispose (GObject *object)
{
  gimp_row_set_viewable (GIMP_ROW (object), NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_row_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_VIEWABLE:
      gimp_row_set_viewable (GIMP_ROW (object), g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_row_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  switch (property_id)
    {
    case PROP_VIEWABLE:
      g_value_set_object (value, gimp_row_get_viewable (GIMP_ROW (object)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GtkWidget *
gimp_row_new (GimpViewable *viewable)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  return g_object_new (GIMP_TYPE_ROW,
                       "viewable", viewable,
                       NULL);
}

void
gimp_row_set_viewable (GimpRow      *row,
                       GimpViewable *viewable)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  g_return_if_fail (GIMP_IS_ROW (row));
  g_return_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable));

  if (priv->viewable)
    {
      g_signal_handlers_disconnect_by_func (priv->viewable,
                                            gimp_row_viewable_icon_changed,
                                            row);
      g_signal_handlers_disconnect_by_func (priv->viewable,
                                            gimp_row_viewable_name_changed,
                                            row);
    }

  g_set_object (&priv->viewable, viewable);

  if (priv->viewable)
    {
      g_signal_connect (priv->viewable,
                        GIMP_VIEWABLE_GET_CLASS (viewable)->name_changed_signal,
                        G_CALLBACK (gimp_row_viewable_name_changed),
                        row);
      gimp_row_viewable_name_changed (priv->viewable, row);

      g_signal_connect (priv->viewable, "notify::icon-name",
                        G_CALLBACK (gimp_row_viewable_icon_changed),
                        row);
      gimp_row_viewable_icon_changed (priv->viewable, NULL, row);
    }

  g_object_notify_by_pspec (G_OBJECT (row), obj_props[PROP_VIEWABLE]);
}

GimpViewable *
gimp_row_get_viewable (GimpRow *row)
{
  g_return_val_if_fail (GIMP_IS_ROW (row), NULL);

  return GET_PRIVATE (row)->viewable;
}

GtkWidget *
gimp_row_create (gpointer item,
                 gpointer user_data)
{
  return gimp_row_new (item);
}


/*  private functions  */

static void
gimp_row_viewable_icon_changed (GimpViewable     *viewable,
                                const GParamSpec *pspec,
                                GimpRow          *row)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  if (priv->icon)
    gtk_image_set_from_icon_name (GTK_IMAGE (priv->icon),
                                  gimp_viewable_get_icon_name (viewable),
                                  GTK_ICON_SIZE_BUTTON);
}

static void
gimp_row_viewable_name_changed (GimpViewable *viewable,
                                GimpRow      *row)
{
  GimpRowPrivate *priv = GET_PRIVATE (row);

  if (priv->label)
    gtk_label_set_text (GTK_LABEL (priv->label),
                        gimp_object_get_name (viewable));
}
