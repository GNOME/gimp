/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "paint/gimpcloneoptions.h"

#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpcloneoptions-gui.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"


static gboolean
gimp_clone_options_sync_source (GBinding     *binding,
                                const GValue *source_value,
                                GValue       *target_value,
                                gpointer      user_data)
{
  GimpCloneType type = g_value_get_enum (source_value);

  g_value_set_boolean (target_value,
                       type == GPOINTER_TO_INT (user_data));

  return TRUE;
}

GtkWidget *
gimp_clone_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *frame;
  GtkWidget *combo;
  GtkWidget *source_vbox;
  GtkWidget *button;
  GtkWidget *hbox;

  /*  the source frame  */
  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the source type menu  */
  combo = gimp_prop_enum_combo_box_new (config, "clone-type", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Source"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), combo);
  gtk_widget_show (combo);

  source_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), source_vbox);
  gtk_widget_show (source_vbox);

  button = gimp_prop_check_button_new (config, "sample-merged", NULL);
  gtk_box_pack_start (GTK_BOX (source_vbox), button, FALSE, FALSE, 0);

  g_object_bind_property_full (config, "clone-type",
                               button, "visible",
                               G_BINDING_SYNC_CREATE,
                               gimp_clone_options_sync_source,
                               NULL,
                               GINT_TO_POINTER (GIMP_CLONE_IMAGE), NULL);

  hbox = gimp_prop_pattern_box_new (NULL, GIMP_CONTEXT (tool_options),
                                    NULL, 2,
                                    "pattern-view-type", "pattern-view-size");
  gtk_box_pack_start (GTK_BOX (source_vbox), hbox, FALSE, FALSE, 0);

  g_object_bind_property_full (config, "clone-type",
                               hbox,   "visible",
                               G_BINDING_SYNC_CREATE,
                               gimp_clone_options_sync_source,
                               NULL,
                               GINT_TO_POINTER (GIMP_CLONE_PATTERN), NULL);

  combo = gimp_prop_enum_combo_box_new (config, "align-mode", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Alignment"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  return vbox;
}
