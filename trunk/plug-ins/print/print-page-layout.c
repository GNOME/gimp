/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"
#include "print-page-layout.h"

#include "libgimp/stdplugins-intl.h"


typedef struct
{
  PrintData       *data;
  gint             image_width;
  gint             image_height;
  GimpSizeEntry   *size_entry;
  GimpSizeEntry   *resolution_entry;
  GimpChainButton *chain;
  GtkWidget       *area_label;
} PrintSizeInfo;


static void        run_page_setup_dialog              (GtkWidget     *widget,
                                                       PrintData     *data);

static GtkWidget * print_size_frame                   (PrintData     *data);

static void        print_size_info_size_changed       (GtkWidget     *widget);
static void        print_size_info_resolution_changed (GtkWidget     *widget);
static void        print_size_info_unit_changed       (GtkWidget     *widget);
static void        print_size_info_chain_toggled      (GtkWidget     *widget);

static void        print_size_info_set_size           (PrintSizeInfo *info,
                                                       gdouble        width,
                                                       gdouble        height);

static void        print_size_info_set_resolution     (PrintSizeInfo *info,
                                                       gdouble        xres,
                                                       gdouble        yres);


static void        print_size_info_set_page_setup     (PrintSizeInfo *info);


static PrintSizeInfo  info;


GtkWidget *
print_page_layout_gui (PrintData *data)
{
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *frame;

  memset (&info, 0, sizeof (PrintSizeInfo));

  info.data         = data;
  info.image_width  = gimp_image_width (data->image_id);
  info.image_height = gimp_image_height (data->image_id);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Adjust Page Size "
                                           "and Orientation"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (run_page_setup_dialog),
                    data);
  gtk_widget_show (button);

#if 0
  /* Commented out until the header becomes a little more configurable
   * and we can provide a user interface to include/exclude information.
   */
  button = gtk_check_button_new_with_label ("Print image header");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                data->show_info_header);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &data->show_info_header);
  gtk_widget_show (button);
#endif

  /* label for the printable area */

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Printable area:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  info.area_label = label;

  gtk_box_pack_start (GTK_BOX (hbox), info.area_label, FALSE, FALSE, 0);
  gtk_widget_show (info.area_label);

  /* size entry area for the image's print size */

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = print_size_frame (data);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  print_size_info_set_page_setup (&info);

  return main_vbox;
}

static void
run_page_setup_dialog (GtkWidget *widget,
                       PrintData *data)
{
  GtkPrintOperation *operation;
  GtkPrintSettings  *settings;
  GtkPageSetup      *page_setup;
  GtkWidget         *toplevel;

  operation = data->operation;

  /* find a transient parent if possible */
  toplevel = gtk_widget_get_toplevel (widget);
  if (! GTK_WIDGET_TOPLEVEL (toplevel))
    toplevel = NULL;

  settings = gtk_print_operation_get_print_settings (operation);
  if (! settings)
    settings = gtk_print_settings_new ();

  page_setup = gtk_print_operation_get_default_page_setup (operation);

  page_setup = gtk_print_run_page_setup_dialog (GTK_WINDOW (toplevel),
                                                page_setup, settings);

  gtk_print_operation_set_default_page_setup (operation, page_setup);

  print_size_info_set_page_setup (&info);
}

#define SB_WIDTH 8


static GtkWidget *
print_size_frame (PrintData *data)
{
  GtkWidget    *entry;
  GtkWidget    *height;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *chain;
  GtkWidget    *frame;
  GtkWidget    *label;
  GtkSizeGroup *label_group;
  GtkSizeGroup *entry_group;
  GtkObject    *adj;
  gdouble       image_width;
  gdouble       image_height;

  image_width  = (info.image_width *
                  gimp_unit_get_factor (data->unit) / data->xres);
  image_height = (info.image_height *
                  gimp_unit_get_factor (data->unit) / data->yres);

  frame = gimp_frame_new (_("Image Size"));

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  label_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  entry_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  the print size entry  */

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  entry = gimp_size_entry_new (1, data->unit, "%p",
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_NONE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  info.size_entry = GIMP_SIZE_ENTRY (entry);

  gtk_table_set_row_spacings (GTK_TABLE (entry), 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 0, 6);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 2, 6);

  height = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), height, 1, 2, 0, 1);
  gtk_widget_show (height);

  gtk_size_group_add_widget (entry_group, height);
  g_object_unref (entry_group);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                _("_Width:"), 0, 0, 0.0);
  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("_Height:"), 1, 0, 0.0);

  gtk_size_group_add_widget (label_group, label);
  g_object_unref (label_group);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0,
                                  data->xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 1,
                                  data->yres, FALSE);

  gimp_size_entry_set_value (GIMP_SIZE_ENTRY (entry), 0, image_width);
  gimp_size_entry_set_value (GIMP_SIZE_ENTRY (entry), 1, image_height);

  /*  the resolution entry  */

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  entry = gimp_size_entry_new (1, gimp_image_get_unit (data->image_id),
                               _("pixels/%a"),
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  info.resolution_entry = GIMP_SIZE_ENTRY (entry);

  gtk_table_set_row_spacings (GTK_TABLE (entry), 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 0, 6);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 2, 6);

  height = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), height, 1, 2, 0, 1);
  gtk_widget_show (height);

  gtk_size_group_add_widget (entry_group, height);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                _("_X resolution:"), 0, 0, 0.0);
  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("_Y resolution:"), 1, 0, 0.0);

  gtk_size_group_add_widget (label_group, label);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 0,
                                         GIMP_MIN_RESOLUTION,
                                         GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 1,
                                         GIMP_MIN_RESOLUTION,
                                         GIMP_MAX_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 0, data->xres);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 1, data->yres);

  chain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain), TRUE);
  gtk_table_attach (GTK_TABLE (entry), chain, 2, 3, 0, 2,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (chain);

  info.chain = GIMP_CHAIN_BUTTON (chain);

  g_signal_connect (info.size_entry, "value-changed",
                    G_CALLBACK (print_size_info_size_changed),
                    NULL);
  g_signal_connect (info.resolution_entry, "value-changed",
                    G_CALLBACK (print_size_info_resolution_changed),
                    NULL);
  g_signal_connect (info.size_entry, "unit-changed",
                    G_CALLBACK (print_size_info_unit_changed),
                    NULL);
  g_signal_connect (info.chain, "toggled",
                    G_CALLBACK (print_size_info_chain_toggled),
                    NULL);

  return frame;
}

static void
print_size_info_size_changed (GtkWidget *widget)
{
  gdouble width;
  gdouble height;
  gdouble xres;
  gdouble yres;
  gdouble scale;

  scale = gimp_unit_get_factor (gimp_size_entry_get_unit (info.size_entry));

  width  = gimp_size_entry_get_value (info.size_entry, 0);
  height = gimp_size_entry_get_value (info.size_entry, 1);

  xres = scale * info.image_width  / MAX (0.0001, width);
  yres = scale * info.image_height / MAX (0.0001, height);

  print_size_info_set_resolution (&info, xres, yres);
}

static void
print_size_info_resolution_changed (GtkWidget *widget)
{
  GimpSizeEntry *entry = info.resolution_entry;
  gdouble        xres  = gimp_size_entry_get_refval (entry, 0);
  gdouble        yres  = gimp_size_entry_get_refval (entry, 1);

  print_size_info_set_resolution (&info, xres, yres);
}

static void
print_size_info_unit_changed (GtkWidget *widget)
{
  PrintData     *data   = info.data;
  GimpSizeEntry *entry  = info.size_entry;
  gdouble        factor = gimp_unit_get_factor (data->unit);
  gdouble        w, h;

  data->unit = gimp_size_entry_get_unit (entry);

  factor = gimp_unit_get_factor (data->unit) / factor;

  w = gimp_size_entry_get_value (entry, 0) * factor;
  h = gimp_size_entry_get_value (entry, 1) * factor;

  print_size_info_set_page_setup (&info);
  print_size_info_set_size (&info, w, h);
}

static void
print_size_info_chain_toggled (GtkWidget *widget)
{
  print_size_info_set_page_setup (&info);
}

static void
print_size_info_set_size (PrintSizeInfo *info,
                          gdouble        width,
                          gdouble        height)
{
  g_signal_handlers_block_by_func (info->size_entry,
                                   print_size_info_size_changed,
                                   NULL);

  gimp_size_entry_set_value (info->size_entry, 0, width);
  gimp_size_entry_set_value (info->size_entry, 1, height);

  g_signal_handlers_unblock_by_func (info->size_entry,
                                     print_size_info_size_changed,
                                     NULL);
}

static void
print_size_info_set_resolution (PrintSizeInfo *info,
                                gdouble        xres,
                                gdouble        yres)
{
  PrintData *data = info->data;

  xres = CLAMP (xres, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);
  yres = CLAMP (yres, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);

  if (info->chain && gimp_chain_button_get_active (info->chain))
    {
      if (xres != data->xres)
        yres = xres;
      else
        xres = yres;
    }

  data->xres = xres;
  data->yres = yres;

  g_signal_handlers_block_by_func (info->resolution_entry,
                                   print_size_info_resolution_changed,
                                   NULL);

  gimp_size_entry_set_refval (info->resolution_entry, 0, xres);
  gimp_size_entry_set_refval (info->resolution_entry, 1, yres);

  g_signal_handlers_unblock_by_func (info->resolution_entry,
                                     print_size_info_resolution_changed,
                                     NULL);

  g_signal_handlers_block_by_func (info->size_entry,
                                   print_size_info_size_changed,
                                   NULL);

  gimp_size_entry_set_value (info->size_entry, 0,
                             info->image_width *
                             gimp_unit_get_factor (data->unit) / xres);
  gimp_size_entry_set_value (info->size_entry, 1,
                             info->image_height *
                             gimp_unit_get_factor (data->unit) / yres);

  g_signal_handlers_unblock_by_func (info->size_entry,
                                     print_size_info_size_changed,
                                     NULL);
}

static void
print_size_info_set_page_setup (PrintSizeInfo *info)
{
  GtkPageSetup *setup;
  PrintData    *data = info->data;
  gchar        *format;
  gchar        *text;
  gdouble       page_width;
  gdouble       page_height;
  gdouble       x;
  gdouble       y;

  setup = gtk_print_operation_get_default_page_setup (data->operation);
  if (! setup)
    {
      setup = gtk_page_setup_new ();
      gtk_print_operation_set_default_page_setup (data->operation, setup);
    }

  page_width  = (gtk_page_setup_get_page_width (setup, GTK_UNIT_INCH) *
                 gimp_unit_get_factor (data->unit));
  page_height = (gtk_page_setup_get_page_height (setup, GTK_UNIT_INCH) *
                 gimp_unit_get_factor (data->unit));

  format = g_strdup_printf ("%%.%df x %%.%df %s",
                            gimp_unit_get_digits (data->unit),
                            gimp_unit_get_digits (data->unit),
                            gimp_unit_get_plural (data->unit));
  text = g_strdup_printf (format, page_width, page_height);
  g_free (format);

  gtk_label_set_text (GTK_LABEL (info->area_label), text);
  g_free (text);

  x = page_width;
  y = page_height;

  if (info->chain && gimp_chain_button_get_active (info->chain))
    {
      gdouble ratio;

      ratio = (gdouble) info->image_width / (gdouble) info->image_height;

      if (ratio < 1.0)
        x = y * ratio;
      else
        y = x / ratio;
    }

  gimp_size_entry_set_value_boundaries (info->size_entry, 0, 0.0, x);
  gimp_size_entry_set_value_boundaries (info->size_entry, 1, 0.0, y);

  x = info->image_width / page_width;
  y = info->image_height / page_height;

  if (info->chain && gimp_chain_button_get_active (info->chain))
    {
      gdouble max = MAX (x, y);

      x = y = max;
    }

  gimp_size_entry_set_refval_boundaries (info->resolution_entry, 0,
                                         x, GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (info->resolution_entry, 1,
                                         y, GIMP_MAX_RESOLUTION);

 /* FIXME: is this still needed at all? */
  data->orientation = gtk_page_setup_get_orientation (setup);
}
