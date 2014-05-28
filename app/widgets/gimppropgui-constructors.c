/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-constructors.c
 * Copyright (C) 2002-2014  Michael Natterer <mitch@gimp.org>
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

#include "gegl/gimp-gegl-utils.h"

#include "core/gimpcontext.h"

#include "gimppropgui.h"
#include "gimppropgui-constructors.h"
#include "gimppropwidgets.h"

#include "gimp-intl.h"


#define HAS_KEY(p,k,v) gimp_gegl_param_spec_has_key (p, k, v)


static void   gimp_prop_gui_chain_toggled (GimpChainButton *chain,
                                           GtkAdjustment   *x_adj);


/*  public functions  */

GtkWidget *
_gimp_prop_gui_new_generic (GObject              *config,
                            GParamSpec          **param_specs,
                            guint                 n_param_specs,
                            GimpContext          *context,
                            GimpCreatePickerFunc  create_picker_func,
                            gpointer              picker_creator)
{
  GtkWidget    *main_vbox;
  GtkSizeGroup *label_group;
  gint          i;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  label_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  for (i = 0; i < n_param_specs; i++)
    {
      GParamSpec  *pspec      = param_specs[i];
      GParamSpec  *next_pspec = NULL;

      if (i < n_param_specs - 1)
        next_pspec = param_specs[i + 1];

      if (next_pspec                        &&
          HAS_KEY (pspec,      "axis", "x") &&
          HAS_KEY (next_pspec, "axis", "y"))
        {
          GtkWidget     *widget_x;
          GtkWidget     *widget_y;
          const gchar   *label_x;
          const gchar   *label_y;
          GtkAdjustment *adj_x;
          GtkAdjustment *adj_y;
          GtkWidget     *hbox;
          GtkWidget     *vbox;
          GtkWidget     *chain;

          i++;

          widget_x = gimp_prop_widget_new (config, pspec, context,
                                           create_picker_func, picker_creator,
                                           &label_x);
          widget_y = gimp_prop_widget_new (config, next_pspec, context,
                                           create_picker_func, picker_creator,
                                           &label_y);

          adj_x = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget_x));
          adj_y = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget_y));

          hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
          gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
          gtk_widget_show (hbox);

          vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
          gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
          gtk_widget_show (vbox);

          gtk_box_pack_start (GTK_BOX (vbox), widget_x, FALSE, FALSE, 0);
          gtk_widget_show (widget_x);

          gtk_box_pack_start (GTK_BOX (vbox), widget_y, FALSE, FALSE, 0);
          gtk_widget_show (widget_y);

          chain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
          gtk_box_pack_end (GTK_BOX (hbox), chain, FALSE, FALSE, 0);
          gtk_widget_show (chain);

          if (! HAS_KEY (pspec, "unit", "pixel-coordinate")    &&
              ! HAS_KEY (pspec, "unit", "relative-coordinate") &&
              gtk_adjustment_get_value (adj_x) ==
              gtk_adjustment_get_value (adj_y))
            {
              GBinding *binding;

              gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain), TRUE);

              binding = g_object_bind_property (adj_x, "value",
                                                adj_y, "value",
                                                G_BINDING_BIDIRECTIONAL);

              g_object_set_data (G_OBJECT (chain), "binding", binding);
            }

          g_signal_connect (chain, "toggled",
                            G_CALLBACK (gimp_prop_gui_chain_toggled),
                            adj_x);

          g_object_set_data (G_OBJECT (adj_x), "y-adjustment", adj_y);

          if (create_picker_func &&
              (HAS_KEY (pspec, "unit", "pixel-coordinate") ||
               HAS_KEY (pspec, "unit", "relative-coordinate")))
            {
              GtkWidget *button;
              gchar     *pspec_name;

              pspec_name = g_strconcat (pspec->name, ":",
                                        next_pspec->name, NULL);

              button = create_picker_func (picker_creator,
                                           pspec_name,
                                           GIMP_STOCK_CURSOR,
                                           _("Pick coordinates from the image"));
              gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
              gtk_widget_show (button);

              g_object_weak_ref (G_OBJECT (button),
                                 (GWeakNotify) g_free, pspec_name);
            }
        }
      else
        {
          GtkWidget   *widget;
          const gchar *label;

          widget = gimp_prop_widget_new (config, pspec, context,
                                         create_picker_func, picker_creator,
                                         &label);

          if (widget && label)
            {
              GtkWidget *hbox;
              GtkWidget *l;

              hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
              gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
              gtk_widget_show (hbox);

              l = gtk_label_new_with_mnemonic (label);
              gtk_misc_set_alignment (GTK_MISC (l), 0.0, 0.5);
              gtk_size_group_add_widget (label_group, l);
              gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);
              gtk_widget_show (l);

              gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
              gtk_widget_show (widget);
            }
          else if (widget)
            {
              gtk_box_pack_start (GTK_BOX (main_vbox), widget, FALSE, FALSE, 0);
              gtk_widget_show (widget);
            }
        }
    }

  g_object_unref (label_group);

  return main_vbox;
}

static void
invert_segment_clicked (GtkWidget *button,
                        GtkWidget *dial)
{
  gdouble  alpha, beta;
  gboolean clockwise;

  g_object_get (dial,
                "alpha",     &alpha,
                "beta",      &beta,
                "clockwise", &clockwise,
                NULL);

  g_object_set (dial,
                "alpha",     beta,
                "beta",      alpha,
                "clockwise", ! clockwise,
                NULL);
}

static GtkWidget *
gimp_prop_angle_range_box_new (GObject    *config,
                               GParamSpec *alpha_pspec,
                               GParamSpec *beta_pspec,
                               GParamSpec *clockwise_pspec)
{
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *scale;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *dial;

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  scale = gimp_prop_spin_scale_new (config, alpha_pspec->name,
                                    g_param_spec_get_nick (alpha_pspec),
                                    1.0, 15.0, 2);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (scale), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_spin_scale_new (config, beta_pspec->name,
                                    g_param_spec_get_nick (beta_pspec),
                                    1.0, 15.0, 2);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (scale), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gimp_prop_check_button_new (config, clockwise_pspec->name,
                                       g_param_spec_get_nick (clockwise_pspec));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Invert Segment");
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  dial = gimp_prop_angle_range_dial_new (config,
                                         alpha_pspec->name,
                                         beta_pspec->name,
                                         clockwise_pspec->name);
  gtk_box_pack_start (GTK_BOX (main_hbox), dial, FALSE, FALSE, 0);
  gtk_widget_show (dial);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (invert_segment_clicked),
                    dial);

  return main_hbox;
}

GtkWidget *
_gimp_prop_gui_new_color_rotate (GObject              *config,
                                 GParamSpec          **param_specs,
                                 guint                 n_param_specs,
                                 GimpContext          *context,
                                 GimpCreatePickerFunc  create_picker_func,
                                 gpointer              picker_creator)
{
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *box;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  frame = gimp_frame_new (_("Source Range"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box = gimp_prop_angle_range_box_new (config,
                                       param_specs[1],
                                       param_specs[2],
                                       param_specs[0]);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  frame = gimp_frame_new (_("Destination Range"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box = gimp_prop_angle_range_box_new (config,
                                       param_specs[4],
                                       param_specs[5],
                                       param_specs[3]);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  frame = gimp_frame_new (_("Gray Handling"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box = _gimp_prop_gui_new_generic (config,
                                    param_specs + 6,
                                    n_param_specs - 6,
                                    context,
                                    create_picker_func,
                                    picker_creator);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  return main_vbox;
}

GtkWidget *
_gimp_prop_gui_new_convolution_matrix (GObject              *config,
                                       GParamSpec          **param_specs,
                                       guint                 n_param_specs,
                                       GimpContext          *context,
                                       GimpCreatePickerFunc  create_picker_func,
                                       gpointer              picker_creator)
{
  GtkWidget   *main_vbox;
  GtkWidget   *table;
  GtkWidget   *hbox;
  GtkWidget   *scale;
  GtkWidget   *vbox;
  const gchar *label;
  gint          x, y;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  table = gtk_table_new (5, 5, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  for (y = 0; y < 5; y++)
    {
      for (x = 0; x < 5; x++)
        {
          GtkWidget *spin;
          gchar      prop_name[3] = { 0, };

          prop_name[0] = "abcde"[y];
          prop_name[1] = "12345"[x];

          spin = gimp_prop_spin_button_new (config, prop_name,
                                            1.0, 10.0, 2);
          gtk_entry_set_width_chars (GTK_ENTRY (spin), 8);
          gtk_table_attach (GTK_TABLE (table), spin,
                            x, x + 1, y, y + 1,
                            GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND,
                            0, 0);
          gtk_widget_show (spin);
        }
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  scale = gimp_prop_widget_new (config, param_specs[25],
                                context, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_widget_new (config, param_specs[26],
                                context, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = _gimp_prop_gui_new_generic (config,
                                     param_specs + 27, 4,
                                     context,
                                     create_picker_func,
                                     picker_creator);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  vbox = _gimp_prop_gui_new_generic (config,
                                     param_specs + 31,
                                     n_param_specs - 31,
                                     context,
                                     create_picker_func,
                                     picker_creator);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  return main_vbox;
}


/*  private functions  */

static void
gimp_prop_gui_chain_toggled (GimpChainButton *chain,
                             GtkAdjustment   *x_adj)
{
  GtkAdjustment *y_adj;

  y_adj = g_object_get_data (G_OBJECT (x_adj), "y-adjustment");

  if (gimp_chain_button_get_active (chain))
    {
      GBinding *binding;

      binding = g_object_bind_property (x_adj, "value",
                                        y_adj, "value",
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
