/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * gimplayermodebox.c
 * Copyright (C) 2017  Michael Natterer <mitch@gimp.org>
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
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gimplayermodebox.h"
#include "gimplayermodecombobox.h"

#include "gimp-intl.h"


/**
 * SECTION: gimplayermodebox
 * @title: GimpLayerModeBox
 * @short_description: A #GtkBox subclass for selecting a layer mode.
 *
 * A #GtkBox subclass for selecting a layer mode
 **/


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_LAYER_MODE
};


struct _GimpLayerModeBoxPrivate
{
  GimpLayerModeContext  context;
  GimpLayerMode         layer_mode;

  GtkWidget            *mode_combo;
  GtkWidget            *group_combo;
};


static void   gimp_layer_mode_box_constructed  (GObject      *object);
static void   gimp_layer_mode_box_set_property (GObject      *object,
                                                guint         prop_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void   gimp_layer_mode_box_get_property (GObject      *object,
                                                guint         prop_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (GimpLayerModeBox, gimp_layer_mode_box, GTK_TYPE_BOX)

#define parent_class gimp_layer_mode_box_parent_class


static void
gimp_layer_mode_box_class_init (GimpLayerModeBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_layer_mode_box_constructed;
  object_class->set_property = gimp_layer_mode_box_set_property;
  object_class->get_property = gimp_layer_mode_box_get_property;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_flags ("context",
                                                       NULL, NULL,
                                                       GIMP_TYPE_LAYER_MODE_CONTEXT,
                                                       GIMP_LAYER_MODE_CONTEXT_ALL,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LAYER_MODE,
                                   g_param_spec_enum ("layer-mode",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE,
                                                      GIMP_LAYER_MODE_NORMAL,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
gimp_layer_mode_box_init (GimpLayerModeBox *box)
{
  box->priv = gimp_layer_mode_box_get_instance_private (box);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (box), 4);
}

static void
gimp_layer_mode_box_constructed (GObject *object)
{
  GimpLayerModeBox *box = GIMP_LAYER_MODE_BOX (object);
  GtkWidget        *mode_combo;
  GtkWidget        *group_combo;
  GtkTreeModel     *group_model;
  gint              i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  box->priv->mode_combo = mode_combo =
    gimp_layer_mode_combo_box_new (box->priv->context);
  gtk_box_pack_start (GTK_BOX (box), mode_combo, TRUE, TRUE, 0);
  gtk_widget_show (mode_combo);

  g_object_bind_property (object,                "context",
                          G_OBJECT (mode_combo), "context",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  g_object_bind_property (object,                "layer-mode",
                          G_OBJECT (mode_combo), "layer-mode",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  box->priv->group_combo = group_combo =
    gimp_prop_enum_combo_box_new (G_OBJECT (mode_combo),
                                  "group", 0, 0);
  gimp_int_combo_box_set_layout (GIMP_INT_COMBO_BOX (group_combo),
                                 GIMP_INT_COMBO_BOX_LAYOUT_ICON_ONLY);
  gtk_box_pack_start (GTK_BOX (box), group_combo, FALSE, FALSE, 0);
  gtk_widget_show (group_combo);

  gimp_help_set_help_data (group_combo,
                           _("Switch to another group of modes"),
                           NULL);

  group_model = gtk_combo_box_get_model (GTK_COMBO_BOX (group_combo));

  for (i = 0; i < 2; i++)
    {
      static const gchar *icons[] =
      {
        "gimp-reset",
        "gimp-wilber-eek"
      };

      GtkTreeIter iter;

      if (gimp_int_store_lookup_by_value (group_model, i, &iter))
        gtk_list_store_set (GTK_LIST_STORE (group_model), &iter,
                            GIMP_INT_STORE_ICON_NAME, icons[i],
                            -1);
    }
}

static void
gimp_layer_mode_box_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpLayerModeBox *box = GIMP_LAYER_MODE_BOX (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      gimp_layer_mode_box_set_context (box, g_value_get_flags (value));
      break;

    case PROP_LAYER_MODE:
      gimp_layer_mode_box_set_mode (box, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_layer_mode_box_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpLayerModeBox *box = GIMP_LAYER_MODE_BOX (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_value_set_flags (value, box->priv->context);
      break;

    case PROP_LAYER_MODE:
      g_value_set_enum (value, box->priv->layer_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


/**
 * gimp_layer_mode_box_new:
 * Foo.
 *
 * Return value: a new #GimpLayerModeBox.
 **/
GtkWidget *
gimp_layer_mode_box_new (GimpLayerModeContext context)
{
  return g_object_new (GIMP_TYPE_LAYER_MODE_BOX,
                       "context", context,
                       NULL);
}

void
gimp_layer_mode_box_set_context (GimpLayerModeBox     *box,
                                 GimpLayerModeContext  context)
{
  g_return_if_fail (GIMP_IS_LAYER_MODE_BOX (box));

  if (context != box->priv->context)
    {
      box->priv->context = context;

      g_object_notify (G_OBJECT (box), "context");
    }
}

GimpLayerModeContext
gimp_layer_mode_box_get_context (GimpLayerModeBox *box)
{
  g_return_val_if_fail (GIMP_IS_LAYER_MODE_BOX (box),
                        GIMP_LAYER_MODE_CONTEXT_ALL);

  return box->priv->context;
}

void
gimp_layer_mode_box_set_mode (GimpLayerModeBox *box,
                              GimpLayerMode     mode)
{
  g_return_if_fail (GIMP_IS_LAYER_MODE_BOX (box));

  if (mode != box->priv->layer_mode)
    {
      box->priv->layer_mode = mode;

      g_object_notify (G_OBJECT (box), "layer-mode");
    }
}

GimpLayerMode
gimp_layer_mode_box_get_mode (GimpLayerModeBox *box)
{
  g_return_val_if_fail (GIMP_IS_LAYER_MODE_BOX (box),
                        GIMP_LAYER_MODE_NORMAL);

  return box->priv->layer_mode;
}

void
gimp_layer_mode_box_set_label (GimpLayerModeBox *box,
                               const gchar      *label)
{
  g_return_if_fail (GIMP_IS_LAYER_MODE_BOX (box));

  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (box->priv->mode_combo),
                                label);
}

void
gimp_layer_mode_box_set_ellipsize (GimpLayerModeBox   *box,
                                   PangoEllipsizeMode  mode)
{
  g_return_if_fail (GIMP_IS_LAYER_MODE_BOX (box));

  g_object_set (box->priv->mode_combo,
                "ellipsize", mode,
                NULL);
}
