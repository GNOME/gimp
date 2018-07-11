/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "imap_command.h"
#include "imap_file.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_misc.h"
#include "imap_mru.h"
#include "imap_preferences.h"
#include "imap_table.h"

#include "libgimp/stdplugins-intl.h"

typedef struct {
   DefaultDialog_t      *dialog;
   GtkWidget            *notebook;
   GtkWidget            *ncsa;
   GtkWidget            *cern;
   GtkWidget            *csim;
   GtkWidget            *prompt_for_area_info;
   GtkWidget            *require_default_url;
   GtkWidget            *show_area_handle;
   GtkWidget            *keep_circles_round;
   GtkWidget            *show_url_tip;
   GtkWidget            *use_doublesized;

   GtkWidget            *undo_levels;
   GtkWidget            *mru_size;

   GtkWidget            *normal_fg;
   GtkWidget            *normal_bg;
   GtkWidget            *selected_fg;
   GtkWidget            *selected_bg;
   GtkWidget            *interactive_fg;
   GtkWidget            *interactive_bg;

   GtkWidget            *threshold;
   GtkWidget            *auto_convert;

   PreferencesData_t    *old_data;
} PreferencesDialog_t;

static void get_button_colors (PreferencesDialog_t *dialog,
                               ColorSelData_t *colors);

static gint
parse_map_type(void)
{
   char *token = strtok(NULL, " )");
   if (!strcmp(token, "ncsa"))
      return NCSA;
   else if (!strcmp(token, "cern"))
      return CERN;
   return CSIM;
}

static gint
parse_yes_no(void)
{
   char *token = strtok(NULL, " )");
   return (gint) strcmp(token, "no");
}

static gint
parse_int(void)
{
   char *token = strtok(NULL, " )");
   return (gint) atoi(token);
}

static void
parse_color(GdkColor *gdk_color)
{
   gdk_color->red = (guint16) parse_int();
   gdk_color->green = (guint16) parse_int();
   gdk_color->blue = (guint16) parse_int();
}

static void
parse_mru_entry(void)
{
   char *filename = strtok(NULL, " )");
   mru_add(get_mru(), filename);
}

static void
parse_line(PreferencesData_t *data, char *line)
{
   char *token;
   ColorSelData_t *colors = &data->colors;

   line++;                      /* Skip '(' */
   token = strtok(line, " ");

   if (!strcmp(token, "default-map-type")) {
      data->default_map_type = parse_map_type();
   }else if (!strcmp(token, "prompt-for-area-info")) {
      data->prompt_for_area_info = parse_yes_no();
   } else if (!strcmp(token, "require-default-url")) {
      data->require_default_url = parse_yes_no();
   } else if (!strcmp(token, "show-area-handle")) {
      data->show_area_handle = parse_yes_no();
   } else if (!strcmp(token, "keep-circles-round")) {
      data->keep_circles_round = parse_yes_no();
   } else if (!strcmp(token, "show-url-tip")) {
      data->show_url_tip = parse_yes_no();
   } else if (!strcmp(token, "use-doublesized")) {
      data->use_doublesized = parse_yes_no();
   } else if (!strcmp(token, "mru-size")) {
      data->mru_size = parse_int();
      if (data->mru_size < 1)
        data->mru_size = 1;
   } else if (!strcmp(token, "undo-levels")) {
      data->undo_levels = parse_int();
      if (data->undo_levels < 1)
        data->undo_levels = 1;
   } else if (!strcmp(token, "normal-fg-color")) {
      parse_color(&colors->normal_fg);
   } else if (!strcmp(token, "normal-bg-color")) {
      parse_color(&colors->normal_bg);
   } else if (!strcmp(token, "selected-fg-color")) {
      parse_color(&colors->selected_fg);
   } else if (!strcmp(token, "selected-bg-color")) {
      parse_color(&colors->selected_bg);
   } else if (!strcmp(token, "interactive-fg-color")) {
      parse_color(&colors->interactive_fg);
   } else if (!strcmp(token, "interactive-bg-color")) {
      parse_color(&colors->interactive_bg);
   } else if (!strcmp(token, "mru-entry")) {
      parse_mru_entry();
   } else {
      /* Unrecognized, just ignore rest of line */
   }
}

gboolean
preferences_load(PreferencesData_t *data)
{
   FILE *in;
   char buf[256];
   gchar *filename;

   filename = gimp_personal_rc_file ("imagemaprc");

   in = g_fopen(filename, "rb");
   g_free(filename);
   if (in) {
      while (fgets(buf, sizeof(buf), in)) {
         if (*buf != '\n' && *buf != '#') {
            parse_line(data, buf);
         }
      }
      fclose(in);
      return TRUE;
   }
   return FALSE;
}

void
preferences_save(PreferencesData_t *data)
{
   FILE *out;
   gchar *filename;
   ColorSelData_t *colors = &data->colors;

   filename = gimp_personal_rc_file ("imagemaprc");

   out = g_fopen(filename, "wb");
   if (out) {
      fprintf(out, "# Image map plug-in resource file\n\n");
      if (data->default_map_type == NCSA)
         fprintf(out, "(default-map-type ncsa)\n");
      else if (data->default_map_type == CERN)
         fprintf(out, "(default-map-type cern)\n");
      else
         fprintf(out, "(default-map-type csim)\n");

      fprintf(out, "(prompt-for-area-info %s)\n",
              (data->prompt_for_area_info) ? "yes" : "no");
      fprintf(out, "(require-default-url %s)\n",
              (data->require_default_url) ? "yes" : "no");
      fprintf(out, "(show-area-handle %s)\n",
              (data->show_area_handle) ? "yes" : "no");
      fprintf(out, "(keep-circles-round %s)\n",
              (data->keep_circles_round) ? "yes" : "no");
      fprintf(out, "(show-url-tip %s)\n",
              (data->show_url_tip) ? "yes" : "no");
      fprintf(out, "(use-doublesized %s)\n",
              (data->use_doublesized) ? "yes" : "no");

      fprintf(out, "(undo-levels %d)\n", data->undo_levels);
      fprintf(out, "(mru-size %d)\n", data->mru_size);

      fprintf(out, "(normal-fg-color %d %d %d)\n",
              colors->normal_fg.red, colors->normal_fg.green,
              colors->normal_fg.blue);
      fprintf(out, "(normal-bg-color %d %d %d)\n",
              colors->normal_bg.red, colors->normal_bg.green,
              colors->normal_bg.blue);
      fprintf(out, "(selected-fg-color %d %d %d)\n",
              colors->selected_fg.red, colors->selected_fg.green,
              colors->selected_fg.blue);
      fprintf(out, "(selected-bg-color %d %d %d)\n",
              colors->selected_bg.red, colors->selected_bg.green,
              colors->selected_bg.blue);
      fprintf(out, "(interactive-fg-color %d %d %d)\n",
              colors->interactive_fg.red, colors->interactive_fg.green,
              colors->interactive_fg.blue);
      fprintf(out, "(interactive-bg-color %d %d %d)\n",
              colors->interactive_bg.red, colors->interactive_bg.green,
              colors->interactive_bg.blue);

      mru_write(get_mru(), out);

      fclose(out);
   } else {
      do_file_error_dialog( _("Couldn't save resource file:"), filename);
   }
   g_free(filename);
}

static void
preferences_ok_cb(gpointer data)
{
   PreferencesDialog_t *param = (PreferencesDialog_t*) data;
   PreferencesData_t *old_data = param->old_data;
   ColorSelData_t *colors = &old_data->colors;
   MRU_t *mru = get_mru();

   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(param->cern)))
      old_data->default_map_type = CERN;
   else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(param->ncsa)))
      old_data->default_map_type = NCSA;
   else
      old_data->default_map_type = CSIM;

   old_data->prompt_for_area_info = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(param->prompt_for_area_info));
   old_data->require_default_url = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(param->require_default_url));
   old_data->show_area_handle = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(param->show_area_handle));
   old_data->keep_circles_round = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(param->keep_circles_round));
   old_data->show_url_tip = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(param->show_url_tip));
   old_data->use_doublesized = gtk_toggle_button_get_active(
      GTK_TOGGLE_BUTTON(param->use_doublesized));

   old_data->mru_size =
      gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->mru_size));
   old_data->undo_levels =
      gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->undo_levels));
   mru_set_size(mru, old_data->mru_size);
   menu_build_mru_items(mru);
   command_list_set_undo_level(old_data->undo_levels);

   get_button_colors (param, colors);

   set_sash_size(old_data->use_doublesized);
   preview_redraw();
}

static void
get_button_color (GtkWidget *button, GdkColor *color)
{
  GimpRGB rgb;
  gimp_color_button_get_color (GIMP_COLOR_BUTTON (button), &rgb);
  color->red = rgb.r * 0xffff;
  color->green = rgb.g * 0xffff;
  color->blue = rgb.b * 0xffff;
}

static void
get_button_colors(PreferencesDialog_t *dialog, ColorSelData_t *colors)
{
  get_button_color (dialog->normal_fg, &colors->normal_fg);
  get_button_color (dialog->normal_bg, &colors->normal_bg);
  get_button_color (dialog->selected_fg, &colors->selected_fg);
  get_button_color (dialog->selected_bg, &colors->selected_bg);
  get_button_color (dialog->interactive_fg, &colors->interactive_fg);
  get_button_color (dialog->interactive_bg, &colors->interactive_bg);
}

static void
set_button_color (GtkWidget *button, GdkColor *color)
{
  GimpRGB rgb;
  gimp_rgb_set (&rgb, color->red, color->green, color->blue);
  gimp_rgb_multiply (&rgb, 1.0 / 0xffff);
  gimp_color_button_set_color (GIMP_COLOR_BUTTON (button), &rgb);
}

static void
set_button_colors(PreferencesDialog_t *dialog, ColorSelData_t *colors)
{
  set_button_color (dialog->normal_fg, &colors->normal_fg);
  set_button_color (dialog->normal_bg, &colors->normal_bg);
  set_button_color (dialog->selected_fg, &colors->selected_fg);
  set_button_color (dialog->selected_bg, &colors->selected_bg);
  set_button_color (dialog->interactive_fg, &colors->interactive_fg);
  set_button_color (dialog->interactive_bg, &colors->interactive_bg);
}

static GtkWidget*
create_tab(GtkWidget *notebook, const gchar *label, gint rows, gint cols)
{
   GtkWidget *table;
   GtkWidget *vbox;

   vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
   gtk_widget_show(vbox);

   table = gtk_table_new(rows, cols, FALSE);
   gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(table), 12);
   gtk_table_set_row_spacings(GTK_TABLE(table), 6);
   gtk_table_set_col_spacings(GTK_TABLE(table), 6);
   gtk_widget_show(table);

   gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
                            gtk_label_new_with_mnemonic(label));

   return table;
}

static void
create_general_tab(PreferencesDialog_t *data, GtkWidget *notebook)
{
   GtkWidget *table = create_tab(notebook, _("General"), 7, 2);
   GtkWidget *frame;
   GtkWidget *hbox;

   frame = gimp_frame_new( _("Default Map Type"));
   gtk_widget_show(frame);
   gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 2, 0, 1);
   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
   gtk_container_add(GTK_CONTAINER(frame), hbox);
   gtk_widget_show(hbox);
   data->ncsa = gtk_radio_button_new_with_mnemonic_from_widget(NULL, "_NCSA");
   gtk_box_pack_start(GTK_BOX(hbox), data->ncsa, TRUE, TRUE, 10);
   gtk_widget_show(data->ncsa);
   data->cern = gtk_radio_button_new_with_mnemonic_from_widget(
      GTK_RADIO_BUTTON(data->ncsa), "C_ERN");
   gtk_box_pack_start(GTK_BOX(hbox), data->cern, TRUE, TRUE, 10);
   gtk_widget_show(data->cern);
   data->csim = gtk_radio_button_new_with_mnemonic_from_widget(
      GTK_RADIO_BUTTON(data->cern), "C_SIM");
   gtk_box_pack_start(GTK_BOX(hbox), data->csim, TRUE, TRUE, 10);
   gtk_widget_show(data->csim);

   data->prompt_for_area_info =
      create_check_button_in_table(table, 1, 0, _("_Prompt for area info"));
   data->require_default_url =
      create_check_button_in_table(table, 2, 0, _("_Require default URL"));
   data->show_area_handle =
      create_check_button_in_table(table, 3, 0, _("Show area _handles"));
   data->keep_circles_round =
      create_check_button_in_table(table, 4, 0, _("_Keep NCSA circles true"));
   data->show_url_tip =
      create_check_button_in_table(table, 5, 0, _("Show area URL _tip"));
   data->use_doublesized =
      create_check_button_in_table(table, 6, 0,
                                   _("_Use double-sized grab handles"));
   gtk_widget_show(frame);
}

static void
create_menu_tab(PreferencesDialog_t *data, GtkWidget *notebook)
{
   GtkWidget *table = create_tab(notebook, _("Menu"), 2, 2);
   GtkWidget *label;

   label = create_label_in_table(table, 0, 0,
                                 _("Number of _undo levels (1 - 99):"));
   data->undo_levels = create_spin_button_in_table(table, label, 0, 1, 1, 1,
                                                   99);

   label = create_label_in_table(table, 1, 0,
                                 _("Number of M_RU entries (1 - 16):"));
   data->mru_size = create_spin_button_in_table(table, label, 1, 1, 1, 1, 16);
}

static GtkWidget*
create_color_field(PreferencesDialog_t *data, GtkWidget *table, gint row,
                   gint col)
{
   GimpRGB color = {0.0, 0.0, 0.0, 1.0};
   GtkWidget *area = gimp_color_button_new (_("Select Color"), 16, 8, &color,
                                            GIMP_COLOR_AREA_FLAT);
   gimp_color_button_set_update (GIMP_COLOR_BUTTON (area), TRUE);
   gtk_table_attach_defaults (GTK_TABLE (table), area, col, col + 1, row,
                              row + 1);
   gtk_widget_show (area);

   return area;
}

static void
create_colors_tab(PreferencesDialog_t *data, GtkWidget *notebook)
{
   GtkWidget *table = create_tab(notebook, _("Colors"), 3, 3);

   create_label_in_table(table, 0, 0, _("Normal:"));
   data->normal_fg = create_color_field(data, table, 0, 1);
   data->normal_bg = create_color_field(data, table, 0, 2);

   create_label_in_table(table, 1, 0, _("Selected:"));
   data->selected_fg = create_color_field(data, table, 1, 1);
   data->selected_bg = create_color_field(data, table, 1, 2);

   create_label_in_table(table, 2, 0, _("Interaction:"));
   data->interactive_fg = create_color_field(data, table, 2, 1);
   data->interactive_bg = create_color_field(data, table, 2, 2);
}

#ifdef _NOT_READY_YET_
static void
create_contiguous_regions_tab(PreferencesDialog_t *data, GtkWidget *notebook)
{
   GtkWidget *table = create_tab(notebook, _("Co_ntiguous Region"), 2, 2);
   GtkWidget *label;

   label = create_label_in_table(table, 0, 0,
                                 _("_Threshold:"));
   data->auto_convert =
      create_check_button_in_table(table, 1, 0, _("_Automatically convert"));
}
#endif

static PreferencesDialog_t*
create_preferences_dialog(void)
{
   PreferencesDialog_t *data = g_new(PreferencesDialog_t, 1);
   DefaultDialog_t *dialog;
   GtkWidget *notebook;

   data->dialog = dialog = make_default_dialog( _("General Preferences"));
   default_dialog_set_ok_cb(dialog, preferences_ok_cb, (gpointer) data);

   data->notebook = notebook = gtk_notebook_new();
   gtk_box_pack_start (GTK_BOX (data->dialog->vbox), notebook, TRUE, TRUE, 0);
   create_general_tab(data, notebook);
   create_menu_tab(data, notebook);
   create_colors_tab(data, notebook);
#ifdef _NOT_READY_YET_
   create_contiguous_regions_tab(data, notebook);
#endif
   gtk_widget_show(notebook);

   return data;
}

void
do_preferences_dialog(void)
{
   static PreferencesDialog_t *dialog;
   PreferencesData_t *old_data;
   GtkWidget *map_type;

   if (!dialog) {
      dialog = create_preferences_dialog();
   }
   gtk_notebook_set_current_page(GTK_NOTEBOOK(dialog->notebook), 0);
   dialog->old_data = old_data = get_preferences();

   if (old_data->default_map_type == CERN)
      map_type = dialog->cern;
   else if (old_data->default_map_type == NCSA)
      map_type = dialog->ncsa;
   else
      map_type = dialog->csim;
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(map_type), TRUE);

   gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(dialog->prompt_for_area_info),
      old_data->prompt_for_area_info);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->require_default_url),
                                old_data->require_default_url);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->show_area_handle),
                                old_data->show_area_handle);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->keep_circles_round),
                                old_data->keep_circles_round);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->show_url_tip),
                                old_data->show_url_tip);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->use_doublesized),
                                old_data->use_doublesized);

   gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->undo_levels),
                             old_data->undo_levels);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->mru_size),
                             old_data->mru_size);

   set_button_colors(dialog, &old_data->colors);

   default_dialog_show(dialog->dialog);
}
