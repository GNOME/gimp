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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"
#include "print-page-layout.h"

#include "libgimp/stdplugins-intl.h"

#define LAYOUT_PREVIEW_SIZE 200

typedef struct
{
  PrintData              *data;
  GimpSizeEntry          *size_entry;
  GimpSizeEntry          *resolution_entry;
  GimpChainButton        *chain;
} PrintSizeInfo;

static void        run_page_setup_dialog              (GtkWidget     *widget,
                                                       PrintData     *data);

static GtkWidget * print_size_frame                   (PrintData     *data);

static void        print_size_info_size_changed       (GtkWidget     *widget,
                                                       PrintSizeInfo *info);

static void        print_size_info_resolution_changed (GtkWidget     *widget,
                                                       PrintSizeInfo *info);

static void        print_size_info_set_size           (PrintSizeInfo *info,
                                                       gdouble        width,
                                                       gdouble        height);

static void        print_size_info_set_resolution     (PrintSizeInfo *info,
                                                       gdouble        xres,
                                                       gdouble        yres);

static void        print_size_info_unit_changed       (GtkWidget     *widget,
                                                       PrintSizeInfo *info);


GtkWidget *
print_page_layout_gui (PrintData *data)
{
  GtkWidget *main_vbox;
  GtkWidget *caption_vbox;
  GtkWidget *scroll;
  GtkWidget *text_view;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *frame;

  main_vbox = gtk_vbox_new (FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 10);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label (_("Page Setup"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 5);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (run_page_setup_dialog),
                    data);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_label (_("Show image info header"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                data->show_info_header);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 5);
  g_signal_connect (G_OBJECT (button), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &data->show_info_header);
  gtk_widget_show (button);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 10);
  gtk_widget_show (hbox);

  /* size entry area for the image's print size */
  frame = print_size_frame (data);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 10);
  gtk_widget_show (frame);



  caption_vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (main_vbox), caption_vbox, FALSE, FALSE, 2);
  gtk_widget_show (caption_vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (caption_vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Caption"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 10);
  gtk_widget_show (label);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, 300, 100);
  gtk_box_pack_start (GTK_BOX (caption_vbox), scroll, TRUE, TRUE, 0);
  gtk_widget_show (scroll);

  text_view = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(text_view), GTK_WRAP_CHAR);
  gtk_container_add (GTK_CONTAINER (scroll), text_view);
  data->caption_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  gtk_widget_show (text_view);

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

  /* needed for previewing */
  data->orientation = gtk_page_setup_get_orientation (page_setup);
}

#define SB_WIDTH 8

/*
 * the code below is copied from app/dialogs/print-size-dialog.c
 * with minimal changes.  Bleeah!
 */
static GtkWidget *
print_size_frame (PrintData *data)
{
  PrintSizeInfo *info;
  GtkWidget     *table;
  GtkWidget     *entry;
  GtkWidget     *label;
  GtkWidget     *width;
  GtkWidget     *height;
  GtkWidget     *hbox;
  GtkWidget     *chain;
  GtkWidget     *frame;
  GtkObject     *adj;
  GList         *focus_chain = NULL;
  gint           image_width;
  gint           image_height;

  info = g_new0 (PrintSizeInfo, 1);

  info->data = data;
  image_width = gimp_image_width (data->image_id);
  image_height = gimp_image_height (data->image_id);

  frame = gimp_frame_new (_("Print Size"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 12);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /*  the print size entry  */

  width = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (width), SB_WIDTH);

  height = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (height), SB_WIDTH);

  entry = gimp_size_entry_new (0, data->unit, "%p",
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE);
  info->size_entry = GIMP_SIZE_ENTRY (entry);

  label = gtk_label_new (_("Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 0, 2);
  gtk_widget_show (hbox);

  gtk_table_set_row_spacing (GTK_TABLE (entry), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 1, 6);

  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), height, 0, 1, 1, 2);
  gtk_widget_show (height);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (width), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), width, 0, 1, 0, 1);
  gtk_widget_show (width);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0,
                                  data->xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 1,
                                  data->yres, FALSE);

  gimp_size_entry_set_refval_boundaries
    (GIMP_SIZE_ENTRY (entry), 0, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries
    (GIMP_SIZE_ENTRY (entry), 1, GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 0, image_width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 1, image_height);

  /*  the resolution entry  */

  width = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (width), SB_WIDTH);

  height = gimp_spin_button_new (&adj, 1, 1, 1, 1, 10, 0, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (height), SB_WIDTH);

  label = gtk_label_new (_("X resolution:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), width);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new  (_("Y resolution:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 2, 4);
  gtk_widget_show (hbox);

  entry = gimp_size_entry_new (0, data->unit, _("pixels/%a"),
                               FALSE, FALSE, FALSE, SB_WIDTH,
                               GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  info->resolution_entry = GIMP_SIZE_ENTRY (entry);

  gtk_table_set_row_spacing (GTK_TABLE (entry), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 1, 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 2, 2);

  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (height), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), height, 0, 1, 1, 2);
  gtk_widget_show (height);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (width), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), width, 0, 1, 0, 1);
  gtk_widget_show (width);

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
  gtk_table_attach_defaults (GTK_TABLE (entry), chain, 1, 2, 0, 2);
  gtk_widget_show (chain);

  info->chain = GIMP_CHAIN_BUTTON (chain);

  focus_chain = g_list_prepend (focus_chain, GIMP_SIZE_ENTRY (entry)->unitmenu);
  focus_chain = g_list_prepend (focus_chain, chain);
  focus_chain = g_list_prepend (focus_chain, height);
  focus_chain = g_list_prepend (focus_chain, width);

  gtk_container_set_focus_chain (GTK_CONTAINER (entry), focus_chain);
  g_list_free (focus_chain);

  g_signal_connect (info->size_entry, "value-changed",
                    G_CALLBACK (print_size_info_size_changed),
                    info);
  g_signal_connect (info->resolution_entry, "value-changed",
                    G_CALLBACK (print_size_info_resolution_changed),
                    info);
  g_signal_connect (info->size_entry, "unit-changed",
                    G_CALLBACK (print_size_info_unit_changed),
                    info);
  return frame;
}

static void
print_size_info_size_changed (GtkWidget     *widget,
                              PrintSizeInfo *info)
{
  gdouble    width;
  gdouble    height;
  gdouble    xres;
  gdouble    yres;
  gdouble    scale;
  PrintData *data         = info->data;
  gint       image_width  = gimp_image_width (data->image_id);
  gint       image_height = gimp_image_height (data->image_id);

  scale = gimp_unit_get_factor (gimp_size_entry_get_unit (info->size_entry));

  width  = gimp_size_entry_get_value (info->size_entry, 0);
  height = gimp_size_entry_get_value (info->size_entry, 1);

  xres = scale * image_width  / MAX (0.001, width);
  yres = scale * image_height / MAX (0.001, height);

  xres = CLAMP (xres, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);
  yres = CLAMP (yres, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);

  print_size_info_set_resolution (info, xres, yres);
  print_size_info_set_size (info, image_width, image_height);
}

static void
print_size_info_resolution_changed (GtkWidget     *widget,
                                    PrintSizeInfo *info)
{
  GimpSizeEntry *entry = info->resolution_entry;
  gdouble        xres  = gimp_size_entry_get_refval (entry, 0);
  gdouble        yres  = gimp_size_entry_get_refval (entry, 1);

  print_size_info_set_resolution (info, xres, yres);
}

static void
print_size_info_unit_changed (GtkWidget     *widget,
                              PrintSizeInfo *info)
{
  PrintData *data = info->data;
  GimpSizeEntry *entry = info->resolution_entry;

  data->unit = gimp_size_entry_get_unit (entry);
}

static void
print_size_info_set_size (PrintSizeInfo *info,
                          gdouble        width,
                          gdouble        height)
{
  g_signal_handlers_block_by_func (info->size_entry,
                                   print_size_info_size_changed,
                                   info);

  gimp_size_entry_set_refval (info->size_entry, 0, width);
  gimp_size_entry_set_refval (info->size_entry, 1, height);

  g_signal_handlers_unblock_by_func (info->size_entry,
                                     print_size_info_size_changed,
                                     info);
}

static void
print_size_info_set_resolution (PrintSizeInfo *info,
                                gdouble        xres,
                                gdouble        yres)
{
  PrintData *data = info->data;

  if (info->chain && gimp_chain_button_get_active (info->chain))
    {
      if (xres != data->xres)
        {
          yres = xres;
        }
      else
        {
          xres = yres;
        }
    }

  data->xres = xres;
  data->yres = yres;
  data->print_size_changed = TRUE;

  g_signal_handlers_block_by_func (info->resolution_entry,
                                   print_size_info_resolution_changed,
                                   info);

  gimp_size_entry_set_refval (info->resolution_entry, 0, xres);
  gimp_size_entry_set_refval (info->resolution_entry, 1, yres);

  g_signal_handlers_unblock_by_func (info->resolution_entry,
                                     print_size_info_resolution_changed,
                                     info);

  g_signal_handlers_block_by_func (info->size_entry,
                                   print_size_info_size_changed,
                                   info);

  gimp_size_entry_set_resolution (info->size_entry, 0, xres, TRUE);
  gimp_size_entry_set_resolution (info->size_entry, 1, yres, TRUE);

  g_signal_handlers_unblock_by_func (info->size_entry,
                                     print_size_info_size_changed,
                                     info);
}

