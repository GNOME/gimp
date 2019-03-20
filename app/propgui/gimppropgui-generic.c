/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-generic.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

#include "propgui-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimpcontext.h"

#include "widgets/gimppropwidgets.h"

#include "gimppropgui.h"
#include "gimppropgui-generic.h"

#include "gimp-intl.h"


#define HAS_KEY(p,k,v) gimp_gegl_param_spec_has_key (p, k, v)


static void   gimp_prop_gui_chain_toggled (GimpChainButton *chain,
                                           GtkAdjustment   *x_adj);


/*  public functions  */

GtkWidget *
_gimp_prop_gui_new_generic (GObject                  *config,
                            GParamSpec              **param_specs,
                            guint                     n_param_specs,
                            GeglRectangle            *area,
                            GimpContext              *context,
                            GimpCreatePickerFunc      create_picker_func,
                            GimpCreateControllerFunc  create_controller_func,
                            gpointer                  creator)
{
  GtkWidget    *main_vbox;
  GtkSizeGroup *label_group;
  GList        *chains = NULL;
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

          widget_x = gimp_prop_widget_new_from_pspec (config, pspec,
                                                      area, context,
                                                      create_picker_func,
                                                      create_controller_func,
                                                      creator,
                                                      &label_x);
          widget_y = gimp_prop_widget_new_from_pspec (config, next_pspec,
                                                      area, context,
                                                      create_picker_func,
                                                      create_controller_func,
                                                      creator,
                                                      &label_y);

          adj_x = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget_x));
          adj_y = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget_y));

          hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
          gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
          gtk_widget_show (hbox);

          gimp_prop_gui_bind_container (widget_x, hbox);

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

          g_object_set_data_full (G_OBJECT (chain), "x-property",
                                  g_strdup (pspec->name), g_free);
          g_object_set_data_full (G_OBJECT (chain), "y-property",
                                  g_strdup (next_pspec->name), g_free);

          chains = g_list_prepend (chains, chain);

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

              button = create_picker_func (creator,
                                           pspec_name,
                                           GIMP_ICON_CURSOR,
                                           _("Pick coordinates from the image"),
                                           /* pick_abyss = */ TRUE,
                                           NULL, NULL);
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
          gboolean     expand = FALSE;

          widget = gimp_prop_widget_new_from_pspec (config, pspec,
                                                    area, context,
                                                    create_picker_func,
                                                    create_controller_func,
                                                    creator,
                                                    &label);

          if (GTK_IS_SCROLLED_WINDOW (widget))
            expand = TRUE;

          if (widget && label)
            {
              GtkWidget *l;

              l = gtk_label_new_with_mnemonic (label);
              gtk_label_set_xalign (GTK_LABEL (l), 0.0);
              gtk_widget_show (l);

              gimp_prop_gui_bind_label (widget, l);

              if (GTK_IS_SCROLLED_WINDOW (widget))
                {
                  GtkWidget *frame;

                  /* don't set as frame title, it should not be bold */
                  gtk_box_pack_start (GTK_BOX (main_vbox), l, FALSE, FALSE, 0);

                  frame = gimp_frame_new (NULL);
                  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
                  gtk_widget_show (frame);

                  gtk_container_add (GTK_CONTAINER (frame), widget);
                  gtk_widget_show (widget);

                  gimp_prop_gui_bind_container (widget, frame);
                }
              else
                {
                  GtkWidget *hbox;

                  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
                  gtk_box_pack_start (GTK_BOX (main_vbox), hbox,
                                      expand, expand, 0);
                  gtk_widget_show (hbox);

                  gtk_size_group_add_widget (label_group, l);
                  gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);

                  gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
                  gtk_widget_show (widget);

                  gimp_prop_gui_bind_container (widget, hbox);
                }
            }
          else if (widget)
            {
              gtk_box_pack_start (GTK_BOX (main_vbox), widget,
                                  expand, expand, 0);
              gtk_widget_show (widget);
            }
        }
    }

  g_object_unref (label_group);

  g_object_set_data_full (G_OBJECT (main_vbox), "chains", chains,
                          (GDestroyNotify) g_list_free);

  return main_vbox;
}


/*  private functions  */

static void
gimp_prop_gui_chain_toggled (GimpChainButton *chain,
                             GtkAdjustment   *x_adj)
{
  GBinding      *binding;
  GtkAdjustment *y_adj;

  binding = g_object_get_data (G_OBJECT (chain), "binding");
  y_adj   = g_object_get_data (G_OBJECT (x_adj), "y-adjustment");

  if (gimp_chain_button_get_active (chain))
    {
      if (! binding)
        binding = g_object_bind_property (x_adj, "value",
                                          y_adj, "value",
                                          G_BINDING_BIDIRECTIONAL);
    }
  else
    {
      g_clear_object (&binding);
    }
  g_object_set_data (G_OBJECT (chain), "binding", binding);
}
