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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpconfig-utils.h"

#include "paint/gimpinkoptions.h"

#include "widgets/gimpblobeditor.h"
#include "widgets/gimppropwidgets.h"

#include "gimpinkoptions-gui.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"


static GtkWidget * blob_image_new (GimpInkBlobType blob_type);


GtkWidget *
gimp_ink_options_gui (GimpToolOptions *tool_options)
{
  GObject        *config      = G_OBJECT (tool_options);
  GimpInkOptions *ink_options = GIMP_INK_OPTIONS (tool_options);
  GtkWidget      *vbox        = gimp_paint_options_gui (tool_options);
  GtkWidget      *frame;
  GtkWidget      *vbox2;
  GtkWidget      *scale;
  GtkWidget      *blob_vbox;
  GtkWidget      *hbox;
  GtkWidget      *editor;

  /* adjust sliders */
  frame = gimp_frame_new (_("Adjustment"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /*  size slider  */
  scale = gimp_prop_spin_scale_new (config, "size",
                                    _("Size"),
                                    1.0, 2.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* angle adjust slider */
  scale = gimp_prop_spin_scale_new (config, "tilt-angle",
                                    _("Angle"),
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* sens sliders */
  frame = gimp_frame_new (_("Sensitivity"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /* size sens slider */
  scale = gimp_prop_spin_scale_new (config, "size-sensitivity",
                                    _("Size"),
                                    0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* tilt sens slider */
  scale = gimp_prop_spin_scale_new (config, "tilt-sensitivity",
                                    _("Tilt"),
                                    0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* velocity sens slider */
  scale = gimp_prop_spin_scale_new (config, "vel-sensitivity",
                                    _("Speed"),
                                    0.01, 0.1, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  bottom hbox */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Blob type radiobuttons */
  frame = gimp_prop_enum_radio_frame_new (config, "blob-type",
                                          _("Type"), 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  {
    GtkWidget       *frame_child = gtk_bin_get_child (GTK_BIN (frame));
    GList           *children;
    GList           *list;
    GimpInkBlobType  blob_type;

    children = gtk_container_get_children (GTK_CONTAINER (frame_child));

    for (list = children, blob_type = GIMP_INK_BLOB_TYPE_ELLIPSE;
         list;
         list = g_list_next (list), blob_type++)
      {
        GtkWidget *radio = GTK_WIDGET (list->data);
        GtkWidget *blob;

        gtk_container_remove (GTK_CONTAINER (radio),
                              gtk_bin_get_child (GTK_BIN (radio)));

        blob = blob_image_new (blob_type);
        gtk_container_add (GTK_CONTAINER (radio), blob);
        gtk_widget_show (blob);
      }

    g_list_free (children);
  }

  /* Blob shape widget */
  frame = gimp_frame_new (_("Shape"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  blob_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), blob_vbox);
  gtk_widget_show (blob_vbox);

  frame = gtk_aspect_frame_new (NULL, 0.0, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (blob_vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  editor = gimp_blob_editor_new (ink_options->blob_type,
                                 ink_options->blob_aspect,
                                 ink_options->blob_angle);
  gtk_widget_set_size_request (editor, 60, 60);
  gtk_container_add (GTK_CONTAINER (frame), editor);
  gtk_widget_show (editor);

  gimp_config_connect (config, G_OBJECT (editor), NULL);

  return vbox;
}

static GtkWidget *
blob_image_new (GimpInkBlobType blob_type)
{
  const gchar *stock_id = NULL;

  switch (blob_type)
    {
    case GIMP_INK_BLOB_TYPE_ELLIPSE:
      stock_id = GIMP_STOCK_SHAPE_CIRCLE;
      break;

    case GIMP_INK_BLOB_TYPE_SQUARE:
      stock_id = GIMP_STOCK_SHAPE_SQUARE;
      break;

    case GIMP_INK_BLOB_TYPE_DIAMOND:
      stock_id = GIMP_STOCK_SHAPE_DIAMOND;
      break;
    }

  return gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);
}
