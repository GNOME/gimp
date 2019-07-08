/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-recursive-transform.c
 * Copyright (C) 2018 Ell
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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "propgui-types.h"

#include "core/gimpcontext.h"

#include "gimppropgui.h"
#include "gimppropgui-generic.h"
#include "gimppropgui-recursive-transform.h"

#include "gimp-intl.h"


#define MAX_N_TRANSFORMS 10


static void
add_transform (GtkButton *button,
               GObject   *config)
{
  gchar *transform;
  gchar *new_transform;

  g_object_get (config,
                "transform", &transform,
                NULL);

  new_transform = g_strdup_printf ("%s;matrix (1, 0, 0, 0, 1, 0, 0, 0, 1)",
                                   transform);

  g_object_set (config,
                "transform", new_transform,
                NULL);

  g_free (transform);
  g_free (new_transform);
}

static void
duplicate_transform (GtkButton *button,
                     GObject   *config)
{
  gchar *transform;
  gchar *new_transform;
  gchar *delim;

  g_object_get (config,
                "transform", &transform,
                NULL);

  delim = strrchr (transform, ';');

  if (! delim)
    delim = transform;
  else
    delim++;

  new_transform = g_strdup_printf ("%s;%s", transform, delim);

  g_object_set (config,
                "transform", new_transform,
                NULL);

  g_free (transform);
  g_free (new_transform);
}

static void
remove_transform (GtkButton *button,
                  GObject   *config)
{
  gchar *transform;
  gchar *delim;

  g_object_get (config,
                "transform", &transform,
                NULL);

  delim = strrchr (transform, ';');

  if (delim)
    {
      *delim = '\0';

      g_object_set (config,
                    "transform", transform,
                    NULL);
    }

  g_free (transform);
}

static void
transform_grids_callback (GObject                    *config,
                          GeglRectangle              *area,
                          const GimpMatrix3          *transforms,
                          gint                        n_transforms)
{
  GString *transforms_str = g_string_new (NULL);
  gchar   *transform_str;
  gint     i;

  g_object_set_data_full (G_OBJECT (config), "area",
                          g_memdup (area, sizeof (GeglRectangle)),
                          (GDestroyNotify) g_free);

  for (i = 0; i < n_transforms; i++)
    {
      if (i > 0)
        g_string_append (transforms_str, ";");

      transform_str = gegl_matrix3_to_string ((GeglMatrix3 *) &transforms[i]);

      g_string_append (transforms_str, transform_str);

      g_free (transform_str);
    }

  transform_str = g_string_free (transforms_str, FALSE);

  g_object_set (config,
                "transform", transform_str,
                NULL);

  g_free (transform_str);
}

static void
config_notify (GObject          *config,
               const GParamSpec *pspec,
               gpointer          set_data)
{
  GtkWidget                             *add_button;
  GtkWidget                             *duplicate_button;
  GtkWidget                             *remove_button;
  GimpControllerTransformGridsCallback   set_func;
  GeglRectangle                         *area;
  gchar                                 *transform_str;
  gchar                                **transform_strs;
  GimpMatrix3                           *transforms;
  gint                                   n_transforms;
  gint                                   i;

  add_button       = g_object_get_data (G_OBJECT (config), "add-transform-button");
  duplicate_button = g_object_get_data (G_OBJECT (config), "duplicate-transform-button");
  remove_button    = g_object_get_data (G_OBJECT (config), "remove-transform-button");
  set_func         = g_object_get_data (G_OBJECT (config), "set-func");
  area             = g_object_get_data (G_OBJECT (config), "area");

  g_object_get (config,
                "transform", &transform_str,
                NULL);

  transform_strs = g_strsplit (transform_str, ";", -1);

  g_free (transform_str);

  for (n_transforms = 0; transform_strs[n_transforms]; n_transforms++);

  transforms = g_new (GimpMatrix3, n_transforms);

  for (i = 0; i < n_transforms; i++)
    {
      gegl_matrix3_parse_string ((GeglMatrix3 *) &transforms[i],
                                 transform_strs[i]);
    }

  set_func (set_data, area, transforms, n_transforms);

  g_strfreev (transform_strs);
  g_free (transforms);

  gtk_widget_set_sensitive (add_button,       n_transforms < MAX_N_TRANSFORMS);
  gtk_widget_set_sensitive (duplicate_button, n_transforms < MAX_N_TRANSFORMS);
  gtk_widget_set_sensitive (remove_button,    n_transforms > 1);
}

GtkWidget *
_gimp_prop_gui_new_recursive_transform (GObject                  *config,
                                        GParamSpec              **param_specs,
                                        guint                     n_param_specs,
                                        GeglRectangle            *area,
                                        GimpContext              *context,
                                        GimpCreatePickerFunc      create_picker_func,
                                        GimpCreateControllerFunc  create_controller_func,
                                        gpointer                  creator)
{
  GtkWidget *vbox;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  /* skip the "transform" property, which is controlled by a transform-grid
   * controller.
   */
  if (create_controller_func)
    {
      param_specs++;
      n_param_specs--;
    }

  vbox = _gimp_prop_gui_new_generic (config,
                                     param_specs, n_param_specs,
                                     area, context,
                                     create_picker_func,
                                     create_controller_func,
                                     creator);

  if (create_controller_func)
    {
      GtkWidget *outer_vbox;
      GtkWidget *hbox;
      GtkWidget *button;
      GtkWidget *image;
      GCallback  set_func;
      gpointer   set_data;

      outer_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL,
                                gtk_box_get_spacing (GTK_BOX (vbox)));

      gtk_box_pack_start (GTK_BOX (outer_vbox), vbox, FALSE, FALSE, 0);
      gtk_widget_show (vbox);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
      gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
      gtk_box_pack_start (GTK_BOX (outer_vbox), hbox, FALSE, FALSE, 2);
      gtk_widget_show (hbox);

      button = gtk_button_new ();
      gimp_help_set_help_data (button,
                               _("Add transform"),
                               NULL);
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      image = gtk_image_new_from_icon_name (GIMP_ICON_LIST_ADD,
                                            GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (add_transform),
                        config);

      g_object_set_data (config, "add-transform-button", button);

      button = gtk_button_new ();
      gimp_help_set_help_data (button,
                               _("Duplicate transform"),
                               NULL);
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      image = gtk_image_new_from_icon_name (GIMP_ICON_OBJECT_DUPLICATE,
                                            GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (duplicate_transform),
                        config);

      g_object_set_data (config, "duplicate-transform-button", button);

      button = gtk_button_new ();
      gimp_help_set_help_data (button,
                               _("Remove transform"),
                               NULL);
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      image = gtk_image_new_from_icon_name (GIMP_ICON_LIST_REMOVE,
                                            GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (remove_transform),
                        config);

      g_object_set_data (config, "remove-transform-button", button);

      vbox = outer_vbox;

      set_func = create_controller_func (creator,
                                         GIMP_CONTROLLER_TYPE_TRANSFORM_GRIDS,
                                         _("Recursive Transform: "),
                                         (GCallback) transform_grids_callback,
                                         config,
                                         &set_data);

      g_object_set_data (G_OBJECT (config), "set-func", set_func);

      g_object_set_data_full (G_OBJECT (config), "area",
                              g_memdup (area, sizeof (GeglRectangle)),
                              (GDestroyNotify) g_free);

      config_notify (config, NULL, set_data);

      g_signal_connect (config, "notify",
                        G_CALLBACK (config_notify),
                        set_data);
    }

  return vbox;
}
