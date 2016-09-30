/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpwidgets-constructors.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static gboolean   gimp_paint_mode_menu_separator_func (GtkTreeModel *model,
                                                       GtkTreeIter  *iter,
                                                       gpointer      data);


/*  public functions  */

static void
gimp_enum_store_insert_value_after (GimpEnumStore *store,
                                    gint           after,
                                    gint           insert_value)
{
  GtkTreeIter iter;

  g_return_if_fail (GIMP_IS_ENUM_STORE (store));

  if (gimp_int_store_lookup_by_value (GTK_TREE_MODEL (store),
                                      after, &iter))
    {
      GEnumValue *enum_value;

      enum_value = g_enum_get_value (store->enum_class, insert_value);

      if (enum_value)
        {
          GtkTreeIter  value_iter;
          const gchar *desc;

          gtk_list_store_insert_after (GTK_LIST_STORE (store),
                                       &value_iter, &iter);

          desc = gimp_enum_value_get_desc (store->enum_class, enum_value);

          gtk_list_store_set (GTK_LIST_STORE (store), &value_iter,
                              GIMP_INT_STORE_VALUE, enum_value->value,
                              GIMP_INT_STORE_LABEL, desc,
                              -1);
        }
    }
}

static void
gimp_int_store_insert_separator_after (GimpIntStore *store,
                                       gint          after,
                                       gint          separator_value)
{
  GtkTreeIter iter;

  g_return_if_fail (GIMP_IS_INT_STORE (store));

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

GtkWidget *
gimp_paint_mode_menu_new (gboolean with_behind_mode,
                          gboolean with_replace_modes)
{
  GtkListStore *store;
  GtkWidget    *combo;

  store = gimp_enum_store_new_with_values (GIMP_TYPE_LAYER_MODE_EFFECTS,
                                           25,
                                           GIMP_NORMAL_MODE,
                                           GIMP_DISSOLVE_MODE,

                                           GIMP_LIGHTEN_ONLY_MODE,
                                           GIMP_SCREEN_MODE,
                                           GIMP_DODGE_MODE,
                                           GIMP_ADDITION_MODE,

                                           GIMP_DARKEN_ONLY_MODE,
                                           GIMP_MULTIPLY_MODE,
                                           GIMP_BURN_MODE,

                                           GIMP_NEW_OVERLAY_MODE,
                                           GIMP_SOFTLIGHT_MODE,
                                           GIMP_HARDLIGHT_MODE,

                                           GIMP_DIFFERENCE_MODE,
                                           GIMP_SUBTRACT_MODE,
                                           GIMP_GRAIN_EXTRACT_MODE,
                                           GIMP_GRAIN_MERGE_MODE,
                                           GIMP_DIVIDE_MODE,

                                           GIMP_HUE_MODE,
                                           GIMP_SATURATION_MODE,
                                           GIMP_COLOR_MODE,
                                           GIMP_VALUE_MODE,

                                           GIMP_LCH_HUE_MODE,
                                           GIMP_LCH_CHROMA_MODE,
                                           GIMP_LCH_COLOR_MODE,
                                           GIMP_LCH_LIGHTNESS_MODE);

  gimp_int_store_insert_separator_after (GIMP_INT_STORE (store),
                                         GIMP_DISSOLVE_MODE, -1);

  gimp_int_store_insert_separator_after (GIMP_INT_STORE (store),
                                         GIMP_ADDITION_MODE, -1);

  gimp_int_store_insert_separator_after (GIMP_INT_STORE (store),
                                         GIMP_BURN_MODE, -1);

  gimp_int_store_insert_separator_after (GIMP_INT_STORE (store),
                                         GIMP_HARDLIGHT_MODE, -1);

  gimp_int_store_insert_separator_after (GIMP_INT_STORE (store),
                                         GIMP_DIVIDE_MODE, -1);

  gimp_int_store_insert_separator_after (GIMP_INT_STORE (store),
                                         GIMP_VALUE_MODE, -1);

  if (with_behind_mode)
    {
      gimp_enum_store_insert_value_after (GIMP_ENUM_STORE (store),
                                          GIMP_DISSOLVE_MODE,
                                          GIMP_BEHIND_MODE);
      gimp_enum_store_insert_value_after (GIMP_ENUM_STORE (store),
                                          GIMP_BEHIND_MODE,
                                          GIMP_COLOR_ERASE_MODE);
    }

  if (with_replace_modes)
    {
      gimp_enum_store_insert_value_after (GIMP_ENUM_STORE (store),
                                          GIMP_NORMAL_MODE,
                                          GIMP_REPLACE_MODE);
      gimp_enum_store_insert_value_after (GIMP_ENUM_STORE (store),
                                          GIMP_COLOR_ERASE_MODE,
                                          GIMP_ERASE_MODE);
      gimp_enum_store_insert_value_after (GIMP_ENUM_STORE (store),
                                          GIMP_ERASE_MODE,
                                          GIMP_ANTI_ERASE_MODE);
    }

  combo = gimp_enum_combo_box_new_with_model (GIMP_ENUM_STORE (store));
  g_object_unref (store);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo),
                                        gimp_paint_mode_menu_separator_func,
                                        GINT_TO_POINTER (-1),
                                        NULL);

  return combo;
}

GtkWidget *
gimp_icon_button_new (const gchar *icon_name,
                      const gchar *label)
{
  GtkWidget *button;
  GtkWidget *image;

  button = gtk_button_new ();

  if (label)
    {
      GtkWidget *hbox;
      GtkWidget *lab;

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_container_add (GTK_CONTAINER (button), hbox);
      gtk_widget_show (hbox);

      image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);

      lab = gtk_label_new_with_mnemonic (label);
      gtk_label_set_mnemonic_widget (GTK_LABEL (lab), button);
      gtk_box_pack_start (GTK_BOX (hbox), lab, TRUE, TRUE, 0);
      gtk_widget_show (lab);
    }
  else
    {
      image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);
    }

  return button;
}

GtkWidget *
gimp_color_profile_label_new (GimpColorProfile *profile)
{
  GtkWidget   *expander;
  GtkWidget   *view;
  const gchar *label;

  g_return_val_if_fail (profile == NULL ||
                        GIMP_IS_COLOR_PROFILE (profile), NULL);

  if (profile)
    label = gimp_color_profile_get_label (profile);
  else
    label = C_("profile", "None");

  expander = gtk_expander_new (label);

  view = gimp_color_profile_view_new ();

  if (profile)
    gimp_color_profile_view_set_profile (GIMP_COLOR_PROFILE_VIEW (view),
                                         profile);
  else
    gimp_color_profile_view_set_error (GIMP_COLOR_PROFILE_VIEW (view),
                                       C_("profile", "None"));

  gtk_container_add (GTK_CONTAINER (expander), view);
  gtk_widget_show (view);

  return expander;
}


/*  private functions  */

static gboolean
gimp_paint_mode_menu_separator_func (GtkTreeModel *model,
                                     GtkTreeIter  *iter,
                                     gpointer      data)
{
  gint value;

  gtk_tree_model_get (model, iter, GIMP_INT_STORE_VALUE, &value, -1);

  return value == GPOINTER_TO_INT (data);
}
