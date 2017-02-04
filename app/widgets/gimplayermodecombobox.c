/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * gimplayermodecombobox.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp-layer-modes.h"

#include "gimplayermodecombobox.h"


/**
 * SECTION: gimplayermodecombobox
 * @title: GimpLayerModeComboBox
 * @short_description: A #GimpEnumComboBox subclass for selecting a layer mode.
 *
 * A #GtkComboBox subclass for selecting a layer mode
 **/


enum
{
  PROP_0,
  PROP_LAYER_MODE,
  PROP_GROUP,
  PROP_WITH_BEHIND,
  PROP_WITH_REPLACE
};


struct _GimpLayerModeComboBoxPrivate
{
  GimpLayerMode      layer_mode;
  GimpLayerModeGroup group;
  gboolean           with_behind;
  gboolean           with_replace;
};


static void     gimp_layer_mode_combo_box_constructed      (GObject               *object);
static void     gimp_layer_mode_combo_box_set_property     (GObject               *object,
                                                            guint                  prop_id,
                                                            const GValue          *value,
                                                            GParamSpec            *pspec);
static void     gimp_layer_mode_combo_box_get_property     (GObject               *object,
                                                            guint                  prop_id,
                                                            GValue                *value,
                                                            GParamSpec            *pspec);

static void     gimp_layer_mode_combo_box_changed          (GtkComboBox           *gtk_combo);

static void     gimp_layer_mode_combo_box_update_model     (GimpLayerModeComboBox *combo,
                                                            gboolean               change_mode);
static void     gimp_layer_mode_combo_box_insert_value     (GtkListStore          *store,
                                                            gint                   after,
                                                            gint                   insert_value);
static void     gimp_layer_mode_combo_box_insert_separator (GtkListStore          *store,
                                                            gint                   after,
                                                            gint                   separator_value);
static gboolean gimp_layer_mode_combo_box_separator_func   (GtkTreeModel          *model,
                                                            GtkTreeIter           *iter,
                                                            gpointer               data);


G_DEFINE_TYPE (GimpLayerModeComboBox, gimp_layer_mode_combo_box,
               GIMP_TYPE_ENUM_COMBO_BOX)

#define parent_class gimp_layer_mode_combo_box_parent_class


static void
gimp_layer_mode_combo_box_class_init (GimpLayerModeComboBoxClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  GtkComboBoxClass *combo_class  = GTK_COMBO_BOX_CLASS (klass);

  object_class->constructed  = gimp_layer_mode_combo_box_constructed;
  object_class->set_property = gimp_layer_mode_combo_box_set_property;
  object_class->get_property = gimp_layer_mode_combo_box_get_property;

  combo_class->changed       = gimp_layer_mode_combo_box_changed;

  g_object_class_install_property (object_class, PROP_LAYER_MODE,
                                   g_param_spec_enum ("layer-mode",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE,
                                                      GIMP_LAYER_MODE_NORMAL,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_GROUP,
                                   g_param_spec_enum ("group",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE_GROUP,
                                                      GIMP_LAYER_MODE_GROUP_DEFAULT,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_WITH_BEHIND,
                                   g_param_spec_boolean ("with-behind",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_WITH_REPLACE,
                                   g_param_spec_boolean ("with-replace",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpLayerModeComboBoxPrivate));
}

static void
gimp_layer_mode_combo_box_init (GimpLayerModeComboBox *combo)
{
  combo->priv = G_TYPE_INSTANCE_GET_PRIVATE (combo,
                                             GIMP_TYPE_LAYER_MODE_COMBO_BOX,
                                             GimpLayerModeComboBoxPrivate);

  gimp_layer_mode_combo_box_update_model (combo, FALSE);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo),
                                        gimp_layer_mode_combo_box_separator_func,
                                        GINT_TO_POINTER (-1),
                                        NULL);
}

static void
gimp_layer_mode_combo_box_constructed (GObject *object)
{
  GimpLayerModeComboBox *combo = GIMP_LAYER_MODE_COMBO_BOX (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 combo->priv->layer_mode);
}

static void
gimp_layer_mode_combo_box_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpLayerModeComboBox *combo = GIMP_LAYER_MODE_COMBO_BOX (object);

  switch (prop_id)
    {
    case PROP_LAYER_MODE:
      gimp_layer_mode_combo_box_set_mode (combo, g_value_get_enum (value));
      break;

    case PROP_GROUP:
      gimp_layer_mode_combo_box_set_group (combo, g_value_get_enum (value));
      break;

    case PROP_WITH_BEHIND:
      combo->priv->with_behind = g_value_get_boolean (value);
      break;

    case PROP_WITH_REPLACE:
      combo->priv->with_replace = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_layer_mode_combo_box_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpLayerModeComboBox *combo = GIMP_LAYER_MODE_COMBO_BOX (object);

  switch (prop_id)
    {
    case PROP_LAYER_MODE:
      g_value_set_enum (value, combo->priv->layer_mode);
      break;

    case PROP_GROUP:
      g_value_set_enum (value, combo->priv->group);
      break;

    case PROP_WITH_BEHIND:
      g_value_set_boolean (value, combo->priv->with_behind);
      break;

    case PROP_WITH_REPLACE:
      g_value_set_boolean (value, combo->priv->with_replace);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_layer_mode_combo_box_changed (GtkComboBox *gtk_combo)
{
  GimpLayerModeComboBox *combo = GIMP_LAYER_MODE_COMBO_BOX (gtk_combo);
  GimpLayerMode          mode;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo),
                                     (gint *) &mode))
    {
      combo->priv->layer_mode = mode;

      g_object_notify (G_OBJECT (combo), "layer-mode");
    }
}


/**
 * gimp_layer_mode_combo_box_new:
 * Foo.
 *
 * Return value: a new #GimpLayerModeComboBox.
 **/
GtkWidget *
gimp_layer_mode_combo_box_new (gboolean with_behind,
                               gboolean with_replace)
{
  return g_object_new (GIMP_TYPE_LAYER_MODE_COMBO_BOX,
                       "with-behind",  with_behind,
                       "with-replace", with_replace,
                       NULL);
}

void
gimp_layer_mode_combo_box_set_mode (GimpLayerModeComboBox *combo,
                                    GimpLayerMode          mode)
{
  g_return_if_fail (GIMP_IS_LAYER_MODE_COMBO_BOX (combo));

  if (mode != combo->priv->layer_mode)
    {
      GtkTreeModel *model;
      GtkTreeIter   dummy;

      model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

      g_object_freeze_notify (G_OBJECT (combo));

      if (! gimp_int_store_lookup_by_value (model, mode, &dummy))
        {
          combo->priv->group = gimp_layer_mode_get_group (mode);
          g_object_notify (G_OBJECT (combo), "group");

          gimp_layer_mode_combo_box_update_model (combo, FALSE);
        }

      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), mode);

      g_object_thaw_notify (G_OBJECT (combo));
    }
}

GimpLayerMode
gimp_layer_mode_combo_box_get_mode (GimpLayerModeComboBox *combo)
{
  g_return_val_if_fail (GIMP_IS_LAYER_MODE_COMBO_BOX (combo),
                        GIMP_LAYER_MODE_NORMAL);

  return combo->priv->layer_mode;
}

void
gimp_layer_mode_combo_box_set_group (GimpLayerModeComboBox *combo,
                                     GimpLayerModeGroup     group)
{
  g_return_if_fail (GIMP_IS_LAYER_MODE_COMBO_BOX (combo));

  if (group != combo->priv->group)
    {
      g_object_freeze_notify (G_OBJECT (combo));

      combo->priv->group = group;
      g_object_notify (G_OBJECT (combo), "group");

      gimp_layer_mode_combo_box_update_model (combo, TRUE);

      g_object_thaw_notify (G_OBJECT (combo));
    }
}

GimpLayerModeGroup
gimp_layer_mode_combo_box_get_group (GimpLayerModeComboBox *combo)
{
  g_return_val_if_fail (GIMP_IS_LAYER_MODE_COMBO_BOX (combo),
                        GIMP_LAYER_MODE_GROUP_DEFAULT);

  return combo->priv->group;
}


/*  private functions  */

static void
gimp_enum_store_add_value (GtkListStore *store,
                           GEnumValue   *value)
{
  GtkTreeIter  iter = { 0, };
  const gchar *desc;
  gchar       *stripped;

  desc = gimp_enum_value_get_desc (GIMP_ENUM_STORE (store)->enum_class, value);

  /* no mnemonics in combo boxes */
  stripped = gimp_strip_uline (desc);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      GIMP_INT_STORE_VALUE, value->value,
                      GIMP_INT_STORE_LABEL, stripped,
                      -1);

  g_free (stripped);
}

static GtkListStore *
gimp_enum_store_new_from_array (GType       enum_type,
                                gint        n_values,
                                const gint *values)
{
  GtkListStore *store;
  GEnumValue   *value;
  gint          i;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (n_values > 1, NULL);
  g_return_val_if_fail (values != NULL, NULL);

  store = g_object_new (GIMP_TYPE_ENUM_STORE,
                        "enum-type", enum_type,
                        NULL);

  for (i = 0; i < n_values; i++)
    {
      value = g_enum_get_value (GIMP_ENUM_STORE (store)->enum_class, values[i]);
      if (value)
        gimp_enum_store_add_value (store, value);
    }

  return store;
}

static void
gimp_layer_mode_combo_box_fix_default_store (GimpLayerModeComboBox *combo,
                                             GtkListStore          *store)
{
  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_DISSOLVE, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_ADDITION, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_BURN, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_HARD_MIX, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_DIVIDE, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_LCH_LIGHTNESS, -1);

  if (combo->priv->with_behind)
    {
      gimp_layer_mode_combo_box_insert_value (store,
                                              GIMP_LAYER_MODE_DISSOLVE,
                                              GIMP_LAYER_MODE_BEHIND);
      gimp_layer_mode_combo_box_insert_value (store,
                                              GIMP_LAYER_MODE_BEHIND,
                                              GIMP_LAYER_MODE_COLOR_ERASE);
    }

  if (combo->priv->with_replace)
    {
      gimp_layer_mode_combo_box_insert_value (store,
                                              GIMP_LAYER_MODE_NORMAL,
                                              GIMP_LAYER_MODE_REPLACE);
      gimp_layer_mode_combo_box_insert_value (store,
                                              GIMP_LAYER_MODE_COLOR_ERASE,
                                              GIMP_LAYER_MODE_ERASE);
      gimp_layer_mode_combo_box_insert_value (store,
                                              GIMP_LAYER_MODE_ERASE,
                                              GIMP_LAYER_MODE_ANTI_ERASE);
    }
}

static void
gimp_layer_mode_combo_box_fix_linear_store (GimpLayerModeComboBox *combo,
                                            GtkListStore          *store)
{
  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_DISSOLVE, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_ADDITION_LINEAR, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_BURN_LINEAR, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_HARD_MIX_LINEAR, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_DIVIDE_LINEAR, -1);

  if (combo->priv->with_behind)
    {
      gimp_layer_mode_combo_box_insert_value (store,
                                              GIMP_LAYER_MODE_DISSOLVE,
                                              GIMP_LAYER_MODE_BEHIND_LINEAR);
    }

  if (combo->priv->with_replace)
    {
      gimp_layer_mode_combo_box_insert_value (store,
                                              GIMP_LAYER_MODE_NORMAL_LINEAR,
                                              GIMP_LAYER_MODE_REPLACE);
      gimp_layer_mode_combo_box_insert_value (store,
                                              GIMP_LAYER_MODE_REPLACE,
                                              GIMP_LAYER_MODE_ERASE);
      gimp_layer_mode_combo_box_insert_value (store,
                                              GIMP_LAYER_MODE_ERASE,
                                              GIMP_LAYER_MODE_ANTI_ERASE);
    }
}

static void
gimp_layer_mode_combo_box_fix_perceptual_store (GimpLayerModeComboBox *combo,
                                                GtkListStore          *store)
{
  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_DISSOLVE, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_ADDITION, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_BURN, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_HARD_MIX, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_DIVIDE, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_HSV_VALUE, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_LCH_LIGHTNESS, -1);

  if (combo->priv->with_behind)
    {
      gimp_layer_mode_combo_box_insert_value (store,
                                              GIMP_LAYER_MODE_DISSOLVE,
                                              GIMP_LAYER_MODE_BEHIND);
      gimp_layer_mode_combo_box_insert_value (store,
                                              GIMP_LAYER_MODE_BEHIND,
                                              GIMP_LAYER_MODE_COLOR_ERASE);
    }
}

static void
gimp_layer_mode_combo_box_fix_legacy_store (GimpLayerModeComboBox *combo,
                                            GtkListStore          *store)
{
  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_DISSOLVE, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_ADDITION_LEGACY, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_BURN_LEGACY, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_HARDLIGHT_LEGACY, -1);

  gimp_layer_mode_combo_box_insert_separator (store,
                                              GIMP_LAYER_MODE_DIVIDE_LEGACY, -1);
}

static void
gimp_layer_mode_combo_box_update_model (GimpLayerModeComboBox *combo,
                                        gboolean               change_mode)
{
  GtkListStore        *store;
  const GimpLayerMode *modes;
  gint                 n_modes;

  modes = gimp_layer_mode_get_group_array (combo->priv->group, &n_modes);
  store = gimp_enum_store_new_from_array (GIMP_TYPE_LAYER_MODE,
                                          n_modes, (gint *) modes);

  switch (combo->priv->group)
    {
    case GIMP_LAYER_MODE_GROUP_DEFAULT:
      gimp_layer_mode_combo_box_fix_default_store (combo, store);
      break;

    case GIMP_LAYER_MODE_GROUP_LINEAR:
      gimp_layer_mode_combo_box_fix_linear_store (combo, store);
      break;

    case GIMP_LAYER_MODE_GROUP_PERCEPTUAL:
      gimp_layer_mode_combo_box_fix_perceptual_store (combo, store);
      break;

    case GIMP_LAYER_MODE_GROUP_LEGACY:
      gimp_layer_mode_combo_box_fix_legacy_store (combo, store);
      break;

    default:
      g_return_if_reached ();
    }

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (store));
  g_object_unref (store);

  if (change_mode)
    {
      GimpLayerMode new_mode;

      if (gimp_layer_mode_get_for_group (combo->priv->layer_mode,
                                         combo->priv->group,
                                         &new_mode))
        {
          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), new_mode);
        }
      else
        {
          GtkTreeIter iter;

          /*  switch to the first mode, which will be one of the "normal"  */
          gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
        }
    }
}

static void
gimp_layer_mode_combo_box_insert_value (GtkListStore *store,
                                        gint          after,
                                        gint          insert_value)
{
  GtkTreeIter iter;

  if (gimp_int_store_lookup_by_value (GTK_TREE_MODEL (store),
                                      after, &iter))
    {
      GEnumValue *enum_value;

      enum_value = g_enum_get_value (GIMP_ENUM_STORE (store)->enum_class,
                                     insert_value);

      if (enum_value)
        {
          GtkTreeIter  value_iter;
          const gchar *desc;

          gtk_list_store_insert_after (GTK_LIST_STORE (store),
                                       &value_iter, &iter);

          desc = gimp_enum_value_get_desc (GIMP_ENUM_STORE (store)->enum_class,
                                           enum_value);

          gtk_list_store_set (GTK_LIST_STORE (store), &value_iter,
                              GIMP_INT_STORE_VALUE, enum_value->value,
                              GIMP_INT_STORE_LABEL, desc,
                              -1);
        }
    }
}

static void
gimp_layer_mode_combo_box_insert_separator (GtkListStore *store,
                                            gint          after,
                                            gint          separator_value)
{
  GtkTreeIter iter;

  if (gimp_int_store_lookup_by_value (GTK_TREE_MODEL (store),
                                      after, &iter))
    {
      GtkTreeIter sep_iter;

      gtk_list_store_insert_after (GTK_LIST_STORE (store),
                                   &sep_iter, &iter);
      gtk_list_store_set (GTK_LIST_STORE (store), &sep_iter,
                          GIMP_INT_STORE_VALUE, separator_value,
                          -1);
    }
}

static gboolean
gimp_layer_mode_combo_box_separator_func (GtkTreeModel *model,
                                          GtkTreeIter  *iter,
                                          gpointer      data)
{
  gint value;

  gtk_tree_model_get (model, iter, GIMP_INT_STORE_VALUE, &value, -1);

  return value == GPOINTER_TO_INT (data);
}
