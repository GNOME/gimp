/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmaconfig-utils.h"

#include "paint/ligmainkoptions.h"

#include "widgets/ligmablobeditor.h"
#include "widgets/ligmapropwidgets.h"

#include "ligmainkoptions-gui.h"
#include "ligmapaintoptions-gui.h"

#include "ligma-intl.h"


GtkWidget *
ligma_ink_options_gui (LigmaToolOptions *tool_options)
{
  GObject        *config      = G_OBJECT (tool_options);
  LigmaInkOptions *ink_options = LIGMA_INK_OPTIONS (tool_options);
  GtkWidget      *vbox        = ligma_paint_options_gui (tool_options);
  GtkWidget      *frame;
  GtkWidget      *vbox2;
  GtkWidget      *scale;
  GtkWidget      *blob_box;
  GtkWidget      *hbox;
  GtkWidget      *editor;
  GtkSizeGroup   *size_group;

  /* adjust sliders */
  frame = ligma_frame_new (_("Adjustment"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /*  size slider  */
  scale = ligma_prop_spin_scale_new (config, "size",
                                    1.0, 2.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* angle adjust slider */
  scale = ligma_prop_spin_scale_new (config, "tilt-angle",
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* sens sliders */
  frame = ligma_frame_new (_("Sensitivity"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /* size sens slider */
  scale = ligma_prop_spin_scale_new (config, "size-sensitivity",
                                    0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* tilt sens slider */
  scale = ligma_prop_spin_scale_new (config, "tilt-sensitivity",
                                    0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* velocity sens slider */
  scale = ligma_prop_spin_scale_new (config, "vel-sensitivity",
                                    0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  /* Blob shape widgets */
  frame = ligma_frame_new (_("Shape"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

  /* Blob type radiobuttons */
  blob_box = ligma_prop_enum_icon_box_new (config, "blob-type",
                                          "ligma-shape", 0, 0);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (blob_box),
                                  GTK_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (hbox), blob_box, FALSE, FALSE, 0);

  gtk_size_group_add_widget (size_group, blob_box);
  g_object_unref (size_group);

  /* Blob editor */
  frame = gtk_aspect_frame_new (NULL, 0.0, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  gtk_size_group_add_widget (size_group, frame);

  editor = ligma_blob_editor_new (ink_options->blob_type,
                                 ink_options->blob_aspect,
                                 ink_options->blob_angle);
  gtk_container_add (GTK_CONTAINER (frame), editor);
  gtk_widget_show (editor);

  ligma_config_connect (config, G_OBJECT (editor), "blob-type");
  ligma_config_connect (config, G_OBJECT (editor), "blob-aspect");
  ligma_config_connect (config, G_OBJECT (editor), "blob-angle");

  return vbox;
}
