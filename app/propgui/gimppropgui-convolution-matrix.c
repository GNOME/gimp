/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-convolution-matrix.c
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "propgui-types.h"

#include "core/gimpcontext.h"

#include "gimppropgui.h"
#include "gimppropgui-convolution-matrix.h"
#include "gimppropgui-generic.h"

#include "gimp-intl.h"


static const gchar *
convolution_matrix_prop_name (gint x,
                              gint y)
{
  static const gchar * const prop_names[5][5] = {
    {"a1", "a2", "a3", "a4", "a5"},
    {"b1", "b2", "b3", "b4", "b5"},
    {"c1", "c2", "c3", "c4", "c5"},
    {"d1", "d2", "d3", "d4", "d5"},
    {"e1", "e2", "e3", "e4", "e5"}};

  return prop_names[x][y];
}

static void
convolution_matrix_rotate_flip (GtkWidget *button,
                                GObject   *config)
{
  gint rotate = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                                    "convolution-matrix-rotate"));
  gint flip   = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                                    "convolution-matrix-flip"));
  gint x, y;

  while (rotate--)
    {
      for (y = 0; y < 2; y++)
        {
          for (x = y; x < 4 - y; x++)
            {
              gint    i;
              gdouble temp;

              g_object_get (config,
                            convolution_matrix_prop_name (x, y), &temp,
                            NULL);

              for (i = 0; i < 4; i++)
                {
                  gint    next_x, next_y;
                  gdouble val;

                  next_x = 4 - y;
                  next_y = x;

                  if  (i < 3)
                    {
                      g_object_get (config,
                                    convolution_matrix_prop_name (next_x, next_y), &val,
                                    NULL);
                    }
                  else
                    {
                      val = temp;
                    }

                  g_object_set (config,
                                convolution_matrix_prop_name (x, y), val,
                                NULL);

                  x = next_x;
                  y = next_y;
                }
            }
        }
    }

  while (flip--)
    {
      for (y = 0; y < 5; y++)
        {
          for (x = 0; x < 2; x++)
            {
              gdouble val1, val2;

              g_object_get (config,
                            convolution_matrix_prop_name (x, y), &val1,
                            NULL);
              g_object_get (config,
                            convolution_matrix_prop_name (4 - x, y), &val2,
                            NULL);

              g_object_set (config,
                            convolution_matrix_prop_name (x, y), val2,
                            NULL);
              g_object_set (config,
                            convolution_matrix_prop_name (4 - x, y), val1,
                            NULL);
            }
        }
    }
}

GtkWidget *
_gimp_prop_gui_new_convolution_matrix (GObject                  *config,
                                       GParamSpec              **param_specs,
                                       guint                     n_param_specs,
                                       GeglRectangle            *area,
                                       GimpContext              *context,
                                       GimpCreatePickerFunc      create_picker_func,
                                       GimpCreateControllerFunc  create_controller_func,
                                       gpointer                  creator)
{
  GtkWidget   *main_vbox;
  GtkWidget   *vbox;
  GtkWidget   *grid;
  GtkWidget   *hbox;
  GtkWidget   *scale;
  GtkWidget   *vbox2;
  const gchar *label;
  gint         x, y;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /* matrix */

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  for (y = 0; y < 5; y++)
    {
      for (x = 0; x < 5; x++)
        {
          GtkWidget *spin;

          spin = gimp_prop_spin_button_new (config,
                                            convolution_matrix_prop_name (x, y),
                                            1.0, 10.0, 2);
          gtk_entry_set_width_chars (GTK_ENTRY (spin), 8);
          gtk_grid_attach (GTK_GRID (grid), spin, y, x, 1, 1);
          gtk_widget_show (spin);
        }
    }

  /* rotate / flip buttons */
  {
    typedef struct
    {
      const gchar *tooltip;
      const gchar *icon_name;
      gint         rotate;
      gint         flip;
    } ButtonInfo;

    gint             i;
    const ButtonInfo buttons[] = {
      {
        .tooltip   = _("Rotate matrix 90° counter-clockwise"),
        .icon_name = GIMP_ICON_OBJECT_ROTATE_270,
        .rotate    = 1,
        .flip      = 0
      },
      {
        .tooltip   = _("Rotate matrix 90° clockwise"),
        .icon_name = GIMP_ICON_OBJECT_ROTATE_90,
        .rotate    = 3,
        .flip      = 0
      },
      {
        .tooltip   = _("Flip matrix horizontally"),
        .icon_name = GIMP_ICON_OBJECT_FLIP_HORIZONTAL,
        .rotate    = 0,
        .flip      = 1
      },
      {
        .tooltip   = _("Flip matrix vertically"),
        .icon_name = GIMP_ICON_OBJECT_FLIP_VERTICAL,
        .rotate    = 2,
        .flip      = 1
      }};

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    for (i = 0; i < G_N_ELEMENTS (buttons); i++)
      {
        const ButtonInfo *info = &buttons[i];
        GtkWidget        *button;
        GtkWidget        *image;

        button = gtk_button_new ();
        gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
        gtk_widget_set_tooltip_text (button, info->tooltip);
        gtk_widget_set_can_focus (button, FALSE);
        gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
        gtk_widget_show (button);

        image = gtk_image_new_from_icon_name (info->icon_name,
                                              GTK_ICON_SIZE_BUTTON);
        gtk_container_add (GTK_CONTAINER (button), image);
        gtk_widget_show (image);

        g_object_set_data (G_OBJECT (button),
                           "convolution-matrix-rotate",
                           GINT_TO_POINTER (info->rotate));
        g_object_set_data (G_OBJECT (button),
                           "convolution-matrix-flip",
                           GINT_TO_POINTER (info->flip));

        g_signal_connect (button, "clicked",
                          G_CALLBACK (convolution_matrix_rotate_flip),
                          config);
      }
  }

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /* divisor / offset spin scales */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  scale = gimp_prop_widget_new (config, "divisor",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_widget_new (config, "offset",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  /* rest of the properties */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox2 = _gimp_prop_gui_new_generic (config,
                                      param_specs + 27, 4,
                                      area, context,
                                      create_picker_func,
                                      create_controller_func,
                                      creator);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
  gtk_widget_show (vbox2);

  vbox2 = _gimp_prop_gui_new_generic (config,
                                      param_specs + 31,
                                      n_param_specs - 31,
                                      area, context,
                                      create_picker_func,
                                      create_controller_func,
                                      creator);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
  gtk_widget_show (vbox2);

  return main_vbox;
}
