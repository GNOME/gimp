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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"
#include "print-page-layout.h"
#include "print-preview.h"

#include "libgimp/stdplugins-intl.h"


typedef struct
{
  PrintData       *data;
  gint             image_width;
  gint             image_height;
  GimpSizeEntry   *size_entry;
  GimpSizeEntry   *resolution_entry;
  GimpChainButton *chain;
  GtkWidget       *center_combo;
  GtkWidget       *area_label;
  GtkWidget       *preview;
  GtkAdjustment   *left_adj;
  GtkAdjustment   *right_adj;
  GtkAdjustment   *top_adj;
  GtkAdjustment   *bottom_adj;
} PrintSizeInfo;

enum
{
  BOTTOM,
  TOP,
  RIGHT,
  LEFT,
  WIDTH,
  HEIGHT
};


static void        print_page_setup_notify            (GtkPrintOperation *operation);
static void        update_custom_widget               (GtkPrintOperation *operation,
                                                       GtkWidget         *custom_widget,
                                                       GtkPageSetup      *page_setup,
                                                       GtkPrintSettings  *print_settings);

static GtkWidget * print_size_frame                   (PrintData    *data,
                                                       GtkSizeGroup *label_group,
                                                       GtkSizeGroup *entry_group);
static GtkWidget * print_offset_frame                 (PrintData    *data,
                                                       GtkSizeGroup *label_group,
                                                       GtkSizeGroup *entry_group);

static void        print_size_info_update_offsets     (void);
static void        print_size_info_size_changed       (GtkWidget     *widget);
static void        print_size_info_offset_max_changed (GtkAdjustment *adj,
                                                       gpointer       data);
static void        print_size_info_resolution_changed (GtkWidget     *widget);
static void        print_size_info_unit_changed       (GtkWidget     *widget);
static void        print_size_info_preview_offset_changed
                                                      (GtkWidget     *widget,
                                                       gdouble        offset_x,
                                                       gdouble        offset_y);
static void        print_size_info_center_changed     (GtkWidget     *widget);
static void        print_size_info_center_none        (void);
static void        print_size_info_use_full_page_toggled
                                                      (GtkWidget     *widget);

static void        print_size_info_set_resolution     (PrintSizeInfo *info,
                                                       gdouble        xres,
                                                       gdouble        yres);


static void        print_size_info_set_page_setup     (PrintSizeInfo *info);

static void        print_draw_crop_marks_toggled      (GtkWidget     *widget);

static void        print_resolution_load_defaults     (PrintSizeInfo *info);

static PrintSizeInfo  info;


GtkWidget *
print_page_layout_gui (PrintData   *data,
                       const gchar *help)
{
  GtkWidget    *main_hbox;
  GtkWidget    *main_vbox;
  GtkWidget    *button;
  GtkWidget    *frame;
  GtkPageSetup *setup;
  GtkSizeGroup *label_group;
  GtkSizeGroup *entry_group;

  memset (&info, 0, sizeof (PrintSizeInfo));

  info.data         = data;
  info.image_width  = gimp_drawable_get_width (data->drawable);
  info.image_height = gimp_drawable_get_height (data->drawable);

  setup = gtk_print_operation_get_default_page_setup (data->operation);
  if (! setup)
    {
      setup = gtk_page_setup_new ();
      gtk_print_operation_set_default_page_setup (data->operation, setup);
    }

  /*  main hbox  */
  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);

  /*  main vbox  */
  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), main_vbox, FALSE, FALSE, 0);
  gtk_widget_show (main_vbox);

  label_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  entry_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* size entry area for the image's print size */

  frame = print_size_frame (data, label_group, entry_group);
  gtk_widget_set_vexpand (frame, FALSE);
  gtk_widget_set_valign (frame, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* offset entry area for the image's offset position */

  frame = print_offset_frame (data, label_group, entry_group);
  gtk_widget_set_vexpand (frame, FALSE);
  gtk_widget_set_valign (frame, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_object_unref (label_group);
  g_object_unref (entry_group);

  button = gtk_check_button_new_with_mnemonic (_("Ignore Page _Margins"));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                data->use_full_page);
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (print_size_info_use_full_page_toggled),
                    NULL);
  gtk_widget_show (button);

  /* crop marks toggle */
  button = gtk_check_button_new_with_mnemonic (_("_Draw Crop Marks"));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                data->draw_crop_marks);
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (print_draw_crop_marks_toggled),
                    NULL);
  gtk_widget_show (button);

  /* preview */
  frame = gimp_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  info.preview = print_preview_new (setup, data->drawable);
  print_preview_set_use_full_page (PRINT_PREVIEW (info.preview),
                                   data->use_full_page);
  gtk_container_add (GTK_CONTAINER (frame), info.preview);
  gtk_widget_show (info.preview);

  g_signal_connect (info.preview, "offsets-changed",
                    G_CALLBACK (print_size_info_preview_offset_changed),
                    NULL);

  print_size_info_set_page_setup (&info);

  g_signal_connect_object (data->operation, "notify::default-page-setup",
                           G_CALLBACK (print_page_setup_notify),
                           main_hbox, 0);
  g_signal_connect_object (data->operation, "update-custom-widget",
                           G_CALLBACK (update_custom_widget),
                           main_hbox, 0);

  gimp_help_connect (main_hbox, NULL, gimp_standard_help_func, help, NULL, NULL);

  return main_hbox;
}

static void
print_page_setup_notify (GtkPrintOperation *operation)
{
  GtkPageSetup *setup;

  setup = gtk_print_operation_get_default_page_setup (operation);

  print_size_info_set_page_setup (&info);
  print_preview_set_page_setup (PRINT_PREVIEW (info.preview), setup);
}

static void
update_custom_widget (GtkPrintOperation *operation,
                      GtkWidget         *custom_widget,
                      GtkPageSetup      *page_setup,
                      GtkPrintSettings  *print_settings)
{
  gtk_print_operation_set_default_page_setup (operation, page_setup);
}


#define SB_WIDTH 8

static GtkWidget *
print_size_frame (PrintData    *data,
                  GtkSizeGroup *label_group,
                  GtkSizeGroup *entry_group)
{
  GtkWidget     *entry;
  GtkWidget     *height;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *chain;
  GtkWidget     *frame;
  GtkWidget     *label;
  GtkWidget     *button;
  GtkAdjustment *adj;
  gdouble        image_width;
  gdouble        image_height;

  image_width  = (info.image_width *
                  gimp_unit_get_factor (data->unit) / data->xres);
  image_height = (info.image_height *
                  gimp_unit_get_factor (data->unit) / data->yres);

  frame = gimp_frame_new (_("Size"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  the print size entry  */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  entry = gimp_size_entry_new (1, data->unit, "%n",
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  info.size_entry = GIMP_SIZE_ENTRY (entry);

  gtk_grid_set_row_spacing (GTK_GRID (entry), 2);

  adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  height = gimp_spin_button_new (adj, 1, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (height), TRUE);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_grid_attach (GTK_GRID (entry), height, 1, 0, 1, 1);
  gtk_widget_show (height);

  gtk_size_group_add_widget (entry_group, height);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                _("_Width:"), 0, 0, 0.0);
  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("_Height:"), 1, 0, 0.0);

  gtk_size_group_add_widget (label_group, label);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0,
                                  data->xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 1,
                                  data->yres, FALSE);

  gimp_size_entry_set_value (GIMP_SIZE_ENTRY (entry), 0, image_width);
  gimp_size_entry_set_value (GIMP_SIZE_ENTRY (entry), 1, image_height);

  /*  the resolution entry  */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  entry = gimp_size_entry_new (1, data->image_unit,
                               _("pixels/%a"),
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  info.resolution_entry = GIMP_SIZE_ENTRY (entry);

  gtk_grid_set_row_spacing (GTK_GRID (entry), 2);

  adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  height = gimp_spin_button_new (adj, 1, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (height), TRUE);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_grid_attach (GTK_GRID (entry), height, 1, 0, 1, 1);
  gtk_widget_show (height);

  gtk_size_group_add_widget (entry_group, height);

  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("_X resolution:"), 0, 0, 0.0);
  gtk_size_group_add_widget (label_group, label);

  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("_Y resolution:"), 1, 0, 0.0);
  gtk_size_group_add_widget (label_group, label);

  button = gtk_button_new_with_mnemonic (_("_Load Defaults"));
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (print_resolution_load_defaults),
                            &info);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 0,
                                         GIMP_MIN_RESOLUTION,
                                         GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 1,
                                         GIMP_MIN_RESOLUTION,
                                         GIMP_MAX_RESOLUTION);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 0, data->xres);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 1, data->yres);

  chain = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  if (ABS (data->xres - data->yres) < GIMP_MIN_RESOLUTION)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain), TRUE);
  gtk_grid_attach (GTK_GRID (entry), chain, 2, 0, 1, 2);
  gtk_widget_show (chain);

  info.chain = GIMP_CHAIN_BUTTON (chain);

  g_signal_connect (info.resolution_entry, "value-changed",
                    G_CALLBACK (print_size_info_resolution_changed),
                    NULL);
  g_signal_connect (info.size_entry, "unit-changed",
                    G_CALLBACK (print_size_info_unit_changed),
                    NULL);

  return frame;
}

static GtkWidget *
print_offset_frame (PrintData    *data,
                    GtkSizeGroup *label_group,
                    GtkSizeGroup *entry_group)
{
  GtkWidget *entry;
  GtkWidget *spinner;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *grid;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *combo;

  frame = gimp_frame_new (_("Position"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  the offset entry  */

  entry = GTK_WIDGET (info.size_entry);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* left */
  info.left_adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinner = gimp_spin_button_new (info.left_adj, 1, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinner), NULL);
  gtk_grid_attach (GTK_GRID (grid), spinner, 1, 0, 1, 1);
  gtk_widget_show (spinner);

  label = gtk_label_new_with_mnemonic (_("_Left:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinner);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_size_group_add_widget (label_group, label);
  gtk_widget_show (label);

  /* right */
  info.right_adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinner = gimp_spin_button_new (info.right_adj, 1, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);

  g_signal_connect (info.right_adj, "value-changed",
                    G_CALLBACK (print_size_info_offset_max_changed),
                    GINT_TO_POINTER (LEFT));

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinner), NULL);
  gtk_grid_attach (GTK_GRID (grid), spinner, 3, 0, 1, 1);
  gtk_widget_show (spinner);

  label = gtk_label_new_with_mnemonic (_("_Right:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinner);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 0, 1, 1);
  gtk_widget_show (label);

  /* top */
  info.top_adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinner = gimp_spin_button_new (info.top_adj, 1, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinner), NULL);
  gtk_grid_attach (GTK_GRID (grid), spinner, 1, 1, 1, 1);
  gtk_widget_show (spinner);

  label = gtk_label_new_with_mnemonic (_("_Top:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinner);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_size_group_add_widget (label_group, label);
  gtk_widget_show (label);

  /* bottom */
  info.bottom_adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinner = gimp_spin_button_new (info.bottom_adj, 1, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);

  g_signal_connect (info.bottom_adj, "value-changed",
                    G_CALLBACK (print_size_info_offset_max_changed),
                    GINT_TO_POINTER (TOP));

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinner), NULL);
  gtk_grid_attach (GTK_GRID (grid), spinner, 3, 1, 1, 1);
  gtk_widget_show (spinner);

  label = gtk_label_new_with_mnemonic (_("_Bottom:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinner);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1, 1);
  gtk_widget_show (label);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), LEFT,   72.0, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), RIGHT,  72.0, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), TOP,    72.0, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), BOTTOM, 72.0, FALSE);

  print_size_info_update_offsets ();

  g_signal_connect (info.size_entry, "value-changed",
                    G_CALLBACK (print_size_info_size_changed),
                    NULL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("C_enter:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_size_group_add_widget (label_group, label);
  gtk_widget_show (label);

  /*  if and how to center the image on the page  */
  combo = gimp_int_combo_box_new (C_("center-mode", "None"), CENTER_NONE,
                                  _("Horizontally"),         CENTER_HORIZONTALLY,
                                  _("Vertically"),           CENTER_VERTICALLY,
                                  _("Both"),                 CENTER_BOTH,
                                  NULL);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              data->center,
                              G_CALLBACK (print_size_info_center_changed),
                              NULL, NULL);

  info.center_combo = combo;

  g_signal_connect (info.left_adj, "value-changed",
                    G_CALLBACK (print_size_info_center_none),
                    NULL);
  g_signal_connect (info.right_adj, "value-changed",
                    G_CALLBACK (print_size_info_center_none),
                    NULL);
  g_signal_connect (info.top_adj, "value-changed",
                    G_CALLBACK (print_size_info_center_none),
                    NULL);
  g_signal_connect (info.bottom_adj, "value-changed",
                    G_CALLBACK (print_size_info_center_none),
                    NULL);

  return frame;
}

static void
print_size_info_get_page_dimensions (PrintSizeInfo *info,
                                     gdouble       *page_width,
                                     gdouble       *page_height,
                                     GtkUnit        unit)
{
  GtkPageSetup *setup;

  setup = gtk_print_operation_get_default_page_setup (info->data->operation);

  if (info->data->use_full_page)
    {
      *page_width = gtk_page_setup_get_paper_width (setup, unit);
      *page_height = gtk_page_setup_get_paper_height (setup, unit);
    }
  else
    {
      *page_width = gtk_page_setup_get_page_width (setup, unit);
      *page_height = gtk_page_setup_get_page_height (setup, unit);
    }

}

static void
gimp_size_info_get_max_offsets (gdouble *offset_x_max,
                                gdouble *offset_y_max)
{
  gdouble width;
  gdouble height;

  print_size_info_get_page_dimensions (&info, &width, &height, GTK_UNIT_POINTS);

  *offset_x_max = width - 72.0 * info.image_width / info.data->xres;
  *offset_x_max = MAX (0, *offset_x_max);

  *offset_y_max = height - 72.0 * info.image_height / info.data->yres;
  *offset_y_max = MAX (0, *offset_y_max);
}

static void
print_size_info_center_none_block (void)
{
  g_signal_handlers_block_by_func (info.left_adj,
                                   print_size_info_center_none, NULL);
  g_signal_handlers_block_by_func (info.right_adj,
                                   print_size_info_center_none, NULL);
  g_signal_handlers_block_by_func (info.top_adj,
                                   print_size_info_center_none, NULL);
  g_signal_handlers_block_by_func (info.bottom_adj,
                                   print_size_info_center_none, NULL);
}

static void
print_size_info_center_none_unblock (void)
{
  g_signal_handlers_unblock_by_func (info.left_adj,
                                     print_size_info_center_none, NULL);
  g_signal_handlers_unblock_by_func (info.right_adj,
                                     print_size_info_center_none, NULL);
  g_signal_handlers_unblock_by_func (info.top_adj,
                                     print_size_info_center_none, NULL);
  g_signal_handlers_unblock_by_func (info.bottom_adj,
                                     print_size_info_center_none, NULL);
}

static void
print_size_info_update_offsets (void)
{
  PrintData *data = info.data;
  gdouble    offset_x_max;
  gdouble    offset_y_max;

  gimp_size_info_get_max_offsets (&offset_x_max, &offset_y_max);

  g_signal_handlers_block_by_func (info.size_entry,
                                   print_size_info_size_changed, NULL);

  print_size_info_center_none_block ();

  gimp_size_entry_set_refval_boundaries (info.size_entry, LEFT,
                                         0, offset_x_max);
  gimp_size_entry_set_refval_boundaries (info.size_entry, RIGHT,
                                         0, offset_x_max);
  gimp_size_entry_set_refval_boundaries (info.size_entry, TOP,
                                         0, offset_y_max);
  gimp_size_entry_set_refval_boundaries (info.size_entry, BOTTOM,
                                         0, offset_y_max);

  switch (data->center)
    {
    case CENTER_NONE:
      break;

    case CENTER_HORIZONTALLY:
      data->offset_x = offset_x_max / 2.0;
      break;

    case CENTER_VERTICALLY:
      data->offset_y = offset_y_max / 2.0;
      break;

    case CENTER_BOTH:
      data->offset_x = offset_x_max / 2.0;
      data->offset_y = offset_y_max / 2.0;
      break;
    }

  gimp_size_entry_set_refval (info.size_entry, LEFT,
                              data->offset_x);
  gimp_size_entry_set_refval (info.size_entry, RIGHT,
                              offset_x_max - data->offset_x);
  gimp_size_entry_set_refval (info.size_entry, TOP,
                              info.data->offset_y);
  gimp_size_entry_set_refval (info.size_entry, BOTTOM,
                              offset_y_max - data->offset_y);

  print_size_info_center_none_unblock ();

  g_signal_handlers_unblock_by_func (info.size_entry,
                                     print_size_info_size_changed, NULL);
}

static void
print_size_info_center_changed (GtkWidget *combo)
{
  gint value;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &value))
    {
      info.data->center = value;

      print_size_info_update_offsets ();

      if (info.preview)
        print_preview_set_image_offsets (PRINT_PREVIEW (info.preview),
                                         info.data->offset_x,
                                         info.data->offset_y);
    }
}

static void
print_size_info_center_none (void)
{
  /* return early if we are called from a unit change */
  if (gimp_size_entry_get_unit (info.size_entry) != info.data->unit)
    return;

  info.data->center = CENTER_NONE;

  if (info.center_combo)
    {
      g_signal_handlers_block_by_func (info.center_combo,
                                       print_size_info_center_changed, NULL);

      info.data->center = CENTER_NONE;

      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (info.center_combo),
                                     info.data->center);

      g_signal_handlers_unblock_by_func (info.center_combo,
                                         print_size_info_center_changed, NULL);
    }
}

static void
print_size_info_preview_offset_changed (GtkWidget *widget,
                                        gdouble    offset_x,
                                        gdouble    offset_y)
{
  print_size_info_center_none ();

  info.data->offset_x = offset_x;
  info.data->offset_y = offset_y;

  print_size_info_update_offsets ();
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

  width  = gimp_size_entry_get_value (info.size_entry, WIDTH);
  height = gimp_size_entry_get_value (info.size_entry, HEIGHT);

  xres = scale * info.image_width  / MAX (0.0001, width);
  yres = scale * info.image_height / MAX (0.0001, height);

  print_size_info_set_resolution (&info, xres, yres);

  info.data->offset_x = gimp_size_entry_get_refval (info.size_entry, LEFT);
  info.data->offset_y = gimp_size_entry_get_refval (info.size_entry, TOP);

  print_preview_set_image_offsets (PRINT_PREVIEW (info.preview),
                                   info.data->offset_x,
                                   info.data->offset_y);
}

static void
print_size_info_offset_max_changed (GtkAdjustment *adj,
                                    gpointer       data)
{
  guint index = GPOINTER_TO_INT (data);

  /* return early if we are called from a unit change */
  if (gimp_size_entry_get_unit (info.size_entry) != info.data->unit)
    return;

  g_signal_handlers_block_by_func (info.size_entry,
                                   print_size_info_size_changed, NULL);

  gimp_size_entry_set_value (info.size_entry, index,
                             gtk_adjustment_get_upper (adj) -
                             gtk_adjustment_get_value (adj));

  g_signal_handlers_unblock_by_func (info.size_entry,
                                     print_size_info_size_changed, NULL);
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
print_size_info_use_full_page_toggled (GtkWidget *widget)
{
  gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  info.data->use_full_page = active;

  print_size_info_set_page_setup (&info);

  print_preview_set_use_full_page (PRINT_PREVIEW(info.preview), active);
}

static void
print_size_info_unit_changed (GtkWidget *widget)
{
  info.data->unit = gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (widget));

  print_size_info_set_page_setup (&info);
}

static void
print_size_info_set_resolution (PrintSizeInfo *info,
                                gdouble        xres,
                                gdouble        yres)
{
  PrintData *data = info->data;
  gdouble    offset_x;
  gdouble    offset_y;
  gdouble    offset_x_max;
  gdouble    offset_y_max;

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

  gimp_size_entry_set_value (info->size_entry, WIDTH,
                             info->image_width *
                             gimp_unit_get_factor (data->unit) / xres);
  gimp_size_entry_set_value (info->size_entry, HEIGHT,
                             info->image_height *
                             gimp_unit_get_factor (data->unit) / yres);

  g_signal_handlers_unblock_by_func (info->size_entry,
                                     print_size_info_size_changed,
                                     NULL);

  gimp_size_info_get_max_offsets (&offset_x_max, &offset_y_max);

  offset_x = gimp_size_entry_get_refval (info->size_entry, LEFT);
  offset_y = gimp_size_entry_get_refval (info->size_entry, TOP);

  offset_x = CLAMP (offset_x, 0, offset_x_max);
  offset_y = CLAMP (offset_y, 0, offset_y_max);

  data->offset_x = offset_x;
  data->offset_y = offset_y;

  print_size_info_update_offsets ();

  print_preview_set_image_dpi (PRINT_PREVIEW (info->preview),
                               data->xres, data->yres);
  print_preview_set_image_offsets (PRINT_PREVIEW (info->preview),
                                   data->offset_x, data->offset_y);
  print_preview_set_image_offsets_max (PRINT_PREVIEW (info->preview),
                                       offset_x_max, offset_y_max);
}

static void
print_size_info_set_page_setup (PrintSizeInfo *info)
{
  PrintData    *data = info->data;
  gdouble       page_width;
  gdouble       page_height;
  gdouble       x;
  gdouble       y;

  print_size_info_get_page_dimensions (info,
                                       &page_width, &page_height,
                                       GTK_UNIT_INCH);

  page_width  *= gimp_unit_get_factor (data->unit);
  page_height *= gimp_unit_get_factor (data->unit);

  if (info->area_label)
    {
      gchar *format;
      gchar *text;

      format = g_strdup_printf ("%%.%df x %%.%df %s",
                                gimp_unit_get_digits (data->unit),
                                gimp_unit_get_digits (data->unit),
                                gimp_unit_get_name (data->unit));
      text = g_strdup_printf (format, page_width, page_height);
      g_free (format);

      gtk_label_set_text (GTK_LABEL (info->area_label), text);
      g_free (text);
    }

  x = page_width;
  y = page_height;

  if (info->chain && gimp_chain_button_get_active (info->chain))
    {
      gdouble ratio_x = page_width  / (gdouble) info->image_width;
      gdouble ratio_y = page_height / (gdouble) info->image_height;

      if (ratio_x < ratio_y)
        y = (gdouble) info->image_height * ratio_x;
      else
        x = (gdouble) info->image_width  * ratio_y;
    }

  gimp_size_entry_set_value_boundaries (info->size_entry, WIDTH,
                                        page_width  / 100.0, x);
  gimp_size_entry_set_value_boundaries (info->size_entry, HEIGHT,
                                        page_height / 100.0, y);

  print_size_info_get_page_dimensions (info,
                                       &page_width, &page_height,
                                       GTK_UNIT_POINTS);

  x = (gdouble) info->image_width  / page_width  * 72.0;
  y = (gdouble) info->image_height / page_height * 72.0;

  if (info->chain && gimp_chain_button_get_active (info->chain))
    {
      gdouble max = MAX (x, y);

      x = y = max;
    }

  data->min_xres = x;
  data->min_yres = y;
  gimp_size_entry_set_refval_boundaries (info->resolution_entry, 0,
                                         x, GIMP_MAX_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (info->resolution_entry, 1,
                                         y, GIMP_MAX_RESOLUTION);
}

static void
print_draw_crop_marks_toggled (GtkWidget *widget)
{
  gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  info.data->draw_crop_marks = active;
}

static void
print_resolution_load_defaults (PrintSizeInfo *info)
{
  gdouble xres;
  gdouble yres;

  gimp_image_get_resolution (info->data->image, &xres, &yres);

  gimp_size_entry_set_refval (info->resolution_entry, 0, xres);
  gimp_size_entry_set_refval (info->resolution_entry, 1, yres);
  print_size_info_resolution_changed (GTK_WIDGET (info->resolution_entry));
}
