/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropwidgets.c
 * Copyright (C) 2002-2004  Michael Natterer <mitch@gimp.org>
 *                          Sven Neumann <sven@gimp.org>
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

#include <string.h>
#include <stdlib.h>

#include <gegl.h>
#include <gegl-paramspecs.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"

#include "gimpcolorpanel.h"
#include "gimpspinscale.h"
#include "gimpproptable.h"
#include "gimppropwidgets.h"

#include "gimp-intl.h"


static void
gimp_prop_table_chain_toggled (GimpChainButton *chain,
                               GtkAdjustment   *x_adj)
{
  GtkAdjustment *y_adj;

  y_adj = g_object_get_data (G_OBJECT (x_adj), "y-adjustment");

  if (gimp_chain_button_get_active (chain))
    {
      GBinding *binding;

      binding = g_object_bind_property (x_adj, "value",
                                        y_adj,   "value",
                                        G_BINDING_BIDIRECTIONAL);

      g_object_set_data (G_OBJECT (chain), "binding", binding);
    }
  else
    {
      GBinding *binding;

      binding = g_object_get_data (G_OBJECT (chain), "binding");

      g_object_unref (binding);
      g_object_set_data (G_OBJECT (chain), "binding", NULL);
    }
}

static void
gimp_prop_table_new_seed_clicked (GtkButton     *button,
                                  GtkAdjustment *adj)
{
  guint32 value = g_random_int_range (gtk_adjustment_get_lower (adj),
                                      gtk_adjustment_get_upper (adj));

  gtk_adjustment_set_value (adj, value);
}

GtkWidget *
gimp_prop_table_new (GObject              *config,
                     GType                 owner_type,
                     GimpContext          *context,
                     GimpCreatePickerFunc  create_picker_func,
                     gpointer              picker_creator)
{
  GtkWidget     *table;
  GtkSizeGroup  *size_group;
  GParamSpec   **param_specs;
  guint          n_param_specs;
  gint           i;
  gint           row = 0;
  GParamSpec    *last_pspec = NULL;
  GtkAdjustment *last_x_adj = NULL;
  gint           last_x_row = 0;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);

  param_specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                                &n_param_specs);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  table = gtk_table_new (3, 1, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);

  for (i = 0; i < n_param_specs; i++)
    {
      GParamSpec  *pspec  = param_specs[i];
      GtkWidget   *widget = NULL;
      const gchar *label  = NULL;

      /*  ignore properties of parent classes of owner_type  */
      if (! g_type_is_a (pspec->owner_type, owner_type))
        continue;

      if (G_IS_PARAM_SPEC_STRING (pspec))
        {
          static GQuark multiline_quark = 0;

          if (! multiline_quark)
            multiline_quark = g_quark_from_static_string ("multiline");

          if (GIMP_IS_PARAM_SPEC_CONFIG_PATH (pspec))
            {
              widget = gimp_prop_file_chooser_button_new (config,
                                                          pspec->name,
                                                          g_param_spec_get_nick (pspec),
                                                          GTK_FILE_CHOOSER_ACTION_OPEN);
            }
          else if (g_param_spec_get_qdata (pspec, multiline_quark))
            {
              GtkTextBuffer *buffer;
              GtkWidget     *view;

              buffer = gimp_prop_text_buffer_new (config, pspec->name, -1);
              view = gtk_text_view_new_with_buffer (buffer);

              widget = gtk_scrolled_window_new (NULL, NULL);
              gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget),
                                                   GTK_SHADOW_IN);
              gtk_container_add (GTK_CONTAINER (widget), view);
              gtk_widget_show (view);
            }
          else
            {
              widget = gimp_prop_entry_new (config, pspec->name, -1);
            }

          label  = g_param_spec_get_nick (pspec);
        }
      else if (G_IS_PARAM_SPEC_BOOLEAN (pspec))
        {
          widget = gimp_prop_check_button_new (config, pspec->name,
                                               g_param_spec_get_nick (pspec));
        }
      else if (G_IS_PARAM_SPEC_ENUM (pspec))
        {
          widget = gimp_prop_enum_combo_box_new (config, pspec->name, 0, 0);
          gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (widget),
                                        g_param_spec_get_nick (pspec));
        }
      else if (GEGL_IS_PARAM_SPEC_SEED (pspec))
        {
          GtkAdjustment *adj;
          GtkWidget     *scale;
          GtkWidget     *button;

          widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

          scale = gimp_prop_spin_scale_new (config, pspec->name,
                                            g_param_spec_get_nick (pspec),
                                            1.0, 10.0, 0);
          gtk_box_pack_start (GTK_BOX (widget), scale, TRUE, TRUE, 0);
          gtk_widget_show (scale);

          button = gtk_button_new_with_label (_("New Seed"));
          gtk_box_pack_start (GTK_BOX (widget), button, FALSE, FALSE, 0);
          gtk_widget_show (button);

          adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (scale));

          g_signal_connect (button, "clicked",
                            G_CALLBACK (gimp_prop_table_new_seed_clicked),
                            adj);
        }
      else if (G_IS_PARAM_SPEC_INT (pspec)   ||
               G_IS_PARAM_SPEC_UINT (pspec)  ||
               G_IS_PARAM_SPEC_FLOAT (pspec) ||
               G_IS_PARAM_SPEC_DOUBLE (pspec))
        {
          GtkAdjustment *adj;
          gdouble        value;
          gdouble        lower;
          gdouble        upper;
          gdouble        step   = 1.0;
          gdouble        page   = 10.0;
          gint           digits = (G_IS_PARAM_SPEC_FLOAT (pspec) ||
                                   G_IS_PARAM_SPEC_DOUBLE (pspec)) ? 2 : 0;

          if (GEGL_IS_PARAM_SPEC_DOUBLE (pspec))
            {
              GeglParamSpecDouble *gspec = GEGL_PARAM_SPEC_DOUBLE (pspec);

              lower = gspec->ui_minimum;
              upper = gspec->ui_maximum;
            }
          else if (GEGL_IS_PARAM_SPEC_INT (pspec))
            {
              GeglParamSpecInt *gspec = GEGL_PARAM_SPEC_INT (pspec);

              lower = gspec->ui_minimum;
              upper = gspec->ui_maximum;
            }
          else
            {
              _gimp_prop_widgets_get_numeric_values (config, pspec,
                                                     &value, &lower, &upper,
                                                     G_STRFUNC);
            }

          if ((upper - lower < 10.0) &&
              (G_IS_PARAM_SPEC_FLOAT (pspec) ||
               G_IS_PARAM_SPEC_DOUBLE (pspec)))
            {
              step   = 0.1;
              page   = 1.0;
              digits = 3;
            }

          widget = gimp_prop_spin_scale_new (config, pspec->name,
                                             g_param_spec_get_nick (pspec),
                                             step, page, digits);

          adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));

          if (g_str_has_suffix (pspec->name, "x") ||
              g_str_has_suffix (pspec->name, "width"))
            {
              last_pspec = pspec;
              last_x_adj = adj;
              last_x_row = row;
            }
          else if ((g_str_has_suffix (pspec->name, "y") ||
                    g_str_has_suffix (pspec->name, "height")) &&
                   last_pspec != NULL &&
                   last_x_adj != NULL &&
                   last_x_row == row - 1)
            {
              GtkWidget *chain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);

              gtk_table_attach (GTK_TABLE (table), chain,
                                3, 4, last_x_row, row + 1,
                                GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL,
                                0, 0);
              gtk_widget_show (chain);

              if (gtk_adjustment_get_value (last_x_adj) ==
                  gtk_adjustment_get_value (adj))
                {
                  GBinding *binding;

                  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain), TRUE);

                  binding = g_object_bind_property (last_x_adj, "value",
                                                    adj,        "value",
                                                    G_BINDING_BIDIRECTIONAL);

                  g_object_set_data (G_OBJECT (chain), "binding", binding);
                }

              g_signal_connect (chain, "toggled",
                                G_CALLBACK (gimp_prop_table_chain_toggled),
                                last_x_adj);

              g_object_set_data (G_OBJECT (last_x_adj), "y-adjustment", adj);

              if (create_picker_func)
                {
                  GtkWidget *button;
                  gchar     *pspec_name;

                  pspec_name = g_strconcat (last_pspec->name, ":",
                                            pspec->name, NULL);

                  button = create_picker_func (picker_creator,
                                               pspec_name,
                                               GIMP_STOCK_CURSOR,
                                               _("Pick coordinates from the image"));
                  gtk_table_attach (GTK_TABLE (table), button,
                                    4, 5, last_x_row, row + 1,
                                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL,
                                    0, 0);
                  gtk_widget_show (button);

                  g_object_weak_ref (G_OBJECT (button),
                                     (GWeakNotify) g_free, pspec_name);
                }
            }
        }
      else if (GIMP_IS_PARAM_SPEC_RGB (pspec))
        {
          GtkWidget *button;

          widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

          button = gimp_prop_color_button_new (config, pspec->name,
                                               g_param_spec_get_nick (pspec),
                                               128, 24,
                                               GIMP_COLOR_AREA_SMALL_CHECKS);
          gimp_color_button_set_update (GIMP_COLOR_BUTTON (button), TRUE);
          gimp_color_panel_set_context (GIMP_COLOR_PANEL (button), context);
          gtk_box_pack_start (GTK_BOX (widget), button, TRUE, TRUE, 0);
          gtk_widget_show (button);

          if (create_picker_func)
            {
              button = create_picker_func (picker_creator,
                                           pspec->name,
                                           GIMP_STOCK_COLOR_PICKER_GRAY,
                                           _("Pick color from the image"));
              gtk_box_pack_start (GTK_BOX (widget), button, FALSE, FALSE, 0);
              gtk_widget_show (button);
            }

          label = g_param_spec_get_nick (pspec);
        }
      else
        {
          g_warning ("%s: not supported: %s (%s)\n", G_STRFUNC,
                     g_type_name (G_TYPE_FROM_INSTANCE (pspec)), pspec->name);
        }

      if (widget)
        {
          if (label)
            {
              gimp_table_attach_aligned (GTK_TABLE (table), 0, row,
                                         label, 0.0, 0.5,
                                         widget, 2, FALSE);
            }
          else
            {
              gtk_table_attach_defaults (GTK_TABLE (table), widget,
                                         0, 3, row, row + 1);
              gtk_widget_show (widget);
            }

          row++;
        }
    }

  g_object_unref (size_group);

  g_free (param_specs);

  return table;
}
