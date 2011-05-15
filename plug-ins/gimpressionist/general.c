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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "infile.h"
#include "general.h"

#include "libgimp/stdplugins-intl.h"


#define COLORBUTTONWIDTH  30
#define COLORBUTTONHEIGHT 20


#define NUMGENERALBGRADIO 4

static GtkWidget     *general_bg_radio[NUMGENERALBGRADIO];
static GtkWidget     *general_paint_edges      = NULL;
static GtkAdjustment *general_dark_edge_adjust = NULL;
static GtkWidget     *general_tileable;
static GtkWidget     *general_drop_shadow      = NULL;
static GtkWidget     *general_color_button;
static GtkAdjustment *general_shadow_adjust    = NULL;
static GtkAdjustment *general_shadow_depth     = NULL;
static GtkAdjustment *general_shadow_blur      = NULL;
static GtkAdjustment *dev_thresh_adjust        = NULL;

static int
normalize_bg (int n)
{
  return (!img_has_alpha && (n == 3)) ? 1 : n;
}

static void
general_bg_callback (GtkWidget *wg, void *d)
{
  pcvals.general_background_type = normalize_bg (GPOINTER_TO_INT (d));
}

void
general_store (void)
{
  pcvals.general_paint_edges = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (general_paint_edges));
  pcvals.general_dark_edge = gtk_adjustment_get_value (GTK_ADJUSTMENT (general_dark_edge_adjust));
  pcvals.general_tileable = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (general_tileable));
  pcvals.general_drop_shadow = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (general_drop_shadow));
  pcvals.general_shadow_darkness = gtk_adjustment_get_value (GTK_ADJUSTMENT (general_shadow_adjust));
  pcvals.general_shadow_depth = gtk_adjustment_get_value (GTK_ADJUSTMENT (general_shadow_depth));
  pcvals.general_shadow_blur = gtk_adjustment_get_value (GTK_ADJUSTMENT (general_shadow_blur));
  pcvals.devthresh = gtk_adjustment_get_value (GTK_ADJUSTMENT (dev_thresh_adjust));
}

int
general_bg_type_input (int in)
{
  return CLAMP_UP_TO (in, NUMGENERALBGRADIO);
}

void
general_restore (void)
{
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (general_bg_radio[normalize_bg (pcvals.general_background_type)]),
     TRUE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (general_paint_edges),
                                pcvals.general_paint_edges);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (general_dark_edge_adjust),
                            pcvals.general_dark_edge);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (general_shadow_adjust),
                            pcvals.general_shadow_darkness);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (general_drop_shadow),
                                pcvals.general_drop_shadow);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (general_shadow_depth),
                            pcvals.general_shadow_depth);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (general_shadow_blur),
                            pcvals.general_shadow_blur);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (general_tileable),
                                pcvals.general_tileable);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (general_color_button),
                               &pcvals.color);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (dev_thresh_adjust),
                            pcvals.devthresh);
}

static void
select_color (GtkWidget *widget, gpointer data)
{
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (general_bg_radio[BG_TYPE_SOLID]),
     TRUE);
}

static GtkWidget *
create_general_button (GtkWidget    *box,
                       int           idx,
                       const gchar  *label,
                       const gchar  *help_string,
                       GSList      **radio_group)
{
  return create_radio_button (box, idx, general_bg_callback, label,
                              help_string, radio_group, general_bg_radio);
}

void
create_generalpage (GtkNotebook *notebook)
{
  GtkWidget *box1, *box2, *box3, *box4, *thispage;
  GtkWidget *label, *tmpw, *frame, *table;
  GSList    * radio_group = NULL;

  label = gtk_label_new_with_mnemonic (_("_General"));

  thispage = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_widget_show (thispage);

  frame = gimp_frame_new (_("Background"));
  gtk_box_pack_start (GTK_BOX (thispage), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  box3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), box3);
  gtk_widget_show (box3);

  create_general_button (box3,
                         BG_TYPE_KEEP_ORIGINAL,
                         _("Keep original"),
                         _("Preserve the original image as a background"),
                         &radio_group);

  create_general_button (box3,
                         BG_TYPE_FROM_PAPER,
                         _("From paper"),
                         _("Copy the texture of the selected paper as a background"),
                         &radio_group);

  box4 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (box3), box4, FALSE, FALSE, 0);
  gtk_widget_show (box4);

  create_general_button (box4,
                         BG_TYPE_SOLID,
                         _("Solid"),
                         _("Solid colored background"),
                         &radio_group);

  general_color_button = gimp_color_button_new (_("Color"),
                                                COLORBUTTONWIDTH,
                                                COLORBUTTONHEIGHT,
                                                &pcvals.color,
                                                GIMP_COLOR_AREA_FLAT);
  g_signal_connect (general_color_button, "clicked",
                    G_CALLBACK (select_color), NULL);
  g_signal_connect (general_color_button, "color-changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &pcvals.color);
  gtk_box_pack_start (GTK_BOX (box4), general_color_button, FALSE, FALSE, 0);
  gtk_widget_show (general_color_button);

  tmpw = create_general_button (box3,
                                BG_TYPE_TRANSPARENT,
                                _("Transparent"),
                                _("Use a transparent background; Only the strokes painted will be visible"),
                                &radio_group);

  if (!img_has_alpha)
    gtk_widget_set_sensitive (tmpw, FALSE);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (general_bg_radio[pcvals.general_background_type]), TRUE);

  box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (thispage), box1, FALSE, FALSE, 0);
  gtk_widget_show (box1);

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);

  tmpw = gtk_check_button_new_with_label ( _("Paint edges"));
  general_paint_edges = tmpw;
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gimp_help_set_help_data (tmpw,
                           _("Selects if to place strokes all the way out to the edges of the image"),
                           NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw),
                                pcvals.general_paint_edges);

  general_tileable = tmpw = gtk_check_button_new_with_label ( _("Tileable"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gimp_help_set_help_data (tmpw,
                           _("Selects if the resulting image should be seamlessly tileable"),
                           NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw),
                                pcvals.general_tileable);

  tmpw = gtk_check_button_new_with_label ( _("Drop shadow"));
  general_drop_shadow = tmpw;
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gimp_help_set_help_data (tmpw,
                           _("Adds a shadow effect to each brush stroke"),
                           NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw),
                                pcvals.general_drop_shadow);

  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (box1), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  general_dark_edge_adjust =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                          _("Edge darken:"),
                          150, 6, pcvals.general_dark_edge,
                          0.0, 1.0, 0.01, 0.1, 2,
                          TRUE, 0, 0,
                          _("How much to \"darken\" the edges of each brush stroke"),
                          NULL);

  general_shadow_adjust =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                          _("Shadow darken:"),
                          150, 6, pcvals.general_shadow_darkness,
                          0.0, 99.0, 0.1, 1, 2,
                          TRUE, 0, 0,
                          _("How much to \"darken\" the drop shadow"),
                          NULL);

  general_shadow_depth =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                          _("Shadow depth:"),
                          150, 6, pcvals.general_shadow_depth,
                          0, 99, 1, 5, 0,
                          TRUE, 0, 0,
                          _("The depth of the drop shadow, i.e. how far apart from the object it should be"),
                          NULL);

  general_shadow_blur =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                          _("Shadow blur:"),
                          150, 6, pcvals.general_shadow_blur,
                          0, 99, 1, 5, 0,
                          TRUE, 0, 0,
                          _("How much to blur the drop shadow"),
                          NULL);

  dev_thresh_adjust =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
                          _("Deviation threshold:"),
                          150, 6, pcvals.devthresh,
                          0.0, 1.0, 0.01, 0.01, 2,
                          TRUE, 0, 0,
                          _("A bailout-value for adaptive selections"),
                          NULL);

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
