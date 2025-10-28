/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2004 Maurits Rijk  m.rijk@chello.nl
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

#include <stdio.h>

#include <gtk/gtk.h>

#include "libgimp/gimp.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "imap_commands.h"
#include "imap_default_dialog.h"
#include "imap_main.h"
#include "imap_rectangle.h"
#include "imap_ui_grid.h"

#include "libgimp/stdplugins-intl.h"

typedef struct {
  DefaultDialog_t      *dialog;

  ObjectList_t         *list;
  GimpDrawable         *drawable;

  GtkWidget            *alternate;
  GtkWidget            *all;
  GtkWidget            *left_border;
  GtkWidget            *right_border;
  GtkWidget            *upper_border;
  GtkWidget            *lower_border;
  GtkWidget            *url;
} GimpGuidesDialog_t;

static gint
guide_sort_func(gconstpointer a, gconstpointer b)
{
   return GPOINTER_TO_INT(a) - GPOINTER_TO_INT(b);
}

static void
gimp_guides_ok_cb(gpointer data)
{
   GimpGuidesDialog_t *param = (GimpGuidesDialog_t*) data;
   gint  guide_num;
   GSList *hguides, *hg;
   GSList *vguides, *vg;
   gboolean all;
   const gchar *url;
   GimpImage *image = gimp_item_get_image (GIMP_ITEM (param->drawable));

   /* First get some dialog values */

   all = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(param->all));

   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(param->left_border)))
      vguides = g_slist_append(NULL, GINT_TO_POINTER(0));
   else
      vguides = NULL;

   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(param->right_border)))
      vguides = g_slist_append(vguides,
                               GINT_TO_POINTER(gimp_image_get_width(image)));

   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(param->upper_border)))
      hguides = g_slist_append(NULL, GINT_TO_POINTER(0));
   else
      hguides = NULL;

   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(param->lower_border)))
      hguides = g_slist_append(hguides,
                               GINT_TO_POINTER(gimp_image_get_height(image)));

   url = gtk_entry_get_text(GTK_ENTRY(param->url));

   /* Next get all the GIMP guides */

   guide_num = gimp_image_find_next_guide(image, 0);

   while (guide_num > 0) {
      gint position = gimp_image_get_guide_position(image, guide_num);

      if (gimp_image_get_guide_orientation(image, guide_num)
          == GIMP_ORIENTATION_HORIZONTAL) {
         hguides = g_slist_insert_sorted(hguides, GINT_TO_POINTER(position),
                                         guide_sort_func);
      } else {                  /* GIMP_ORIENTATION_VERTICAL */
         vguides = g_slist_insert_sorted(vguides, GINT_TO_POINTER(position),
                                         guide_sort_func);
      }
      guide_num = gimp_image_find_next_guide(image, guide_num);
   }

   /* Create the areas */

   subcommand_start(_("Use GIMP Guides"));

   for (hg = hguides; hg && hg->next;
        hg = (all) ? hg->next : hg->next->next) {
      gint y = GPOINTER_TO_INT(hg->data);
      gint height = GPOINTER_TO_INT(hg->next->data) - y;
      for (vg = vguides; vg && vg->next;
           vg = (all) ? vg->next : vg->next->next) {
         gint x = GPOINTER_TO_INT(vg->data);
         gint width = GPOINTER_TO_INT(vg->next->data) - x;
         Object_t *obj = create_rectangle(x, y, width, height);
         Command_t *command = create_command_new(param->list, obj);

         object_set_url(obj, url);
         command_execute(command);
      }
   }

   subcommand_end();
   preview_redraw();
}

static GimpGuidesDialog_t*
make_gimp_guides_dialog(void)
{
   GimpGuidesDialog_t *data = g_new(GimpGuidesDialog_t, 1);
   DefaultDialog_t *dialog;
   GtkWidget *grid, *frame, *hbox, *vbox;
   GtkWidget *label;

   dialog = data->dialog = make_default_dialog(_("Use GIMP Guides"));
   default_dialog_set_ok_cb(dialog, gimp_guides_ok_cb, data);
   grid = default_dialog_add_grid (dialog);

   frame = gimp_frame_new(_("Create"));
   gtk_widget_show(frame);
   gtk_grid_attach (GTK_GRID (grid), frame, 0, 0, 1, 1);

   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
   gtk_container_add(GTK_CONTAINER(frame), hbox);
   gtk_widget_show(hbox);

   data->alternate =
      gtk_radio_button_new_with_mnemonic_from_widget(NULL, _("Al_ternate"));
   gtk_box_pack_start(GTK_BOX(hbox), data->alternate, FALSE, FALSE, 0);
   gtk_widget_show(data->alternate);

   data->all = gtk_radio_button_new_with_mnemonic_from_widget(
      GTK_RADIO_BUTTON(data->alternate), _("A_ll"));
   gtk_box_pack_start(GTK_BOX(hbox), data->all, FALSE, FALSE, 0);
   gtk_widget_show(data->all);

   frame = gimp_frame_new(_("Add Additional Guides"));
   gtk_widget_show(frame);
   gtk_grid_attach (GTK_GRID (grid), frame, 0, 1, 1, 1);

   vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
   gtk_container_add(GTK_CONTAINER(frame), vbox);
   gtk_widget_show(vbox);

   data->left_border = gtk_check_button_new_with_mnemonic(_("L_eft border"));
   gtk_container_add(GTK_CONTAINER(vbox), data->left_border);
   gtk_widget_show(data->left_border);

   data->right_border = gtk_check_button_new_with_mnemonic(_("_Right border"));
   gtk_container_add(GTK_CONTAINER(vbox), data->right_border);
   gtk_widget_show(data->right_border);

   data->upper_border = gtk_check_button_new_with_mnemonic(_("_Upper border"));
   gtk_container_add(GTK_CONTAINER(vbox), data->upper_border);
   gtk_widget_show(data->upper_border);

   data->lower_border = gtk_check_button_new_with_mnemonic(_("Lo_wer border"));
   gtk_container_add(GTK_CONTAINER(vbox), data->lower_border);
   gtk_widget_show(data->lower_border);

   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
   gtk_grid_attach (GTK_GRID (grid), hbox, 0, 2, 2, 1);
   gtk_widget_show(hbox);

   label = gtk_label_new_with_mnemonic(_("_Base URL:"));
   gtk_widget_show(label);
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

   data->url = gtk_entry_new();
   gtk_container_add(GTK_CONTAINER(hbox), data->url);
   gtk_widget_show(data->url);

   gtk_label_set_mnemonic_widget (GTK_LABEL (label), data->url);

   return data;
}

static void
init_gimp_guides_dialog (GimpGuidesDialog_t *dialog,
                         ObjectList_t *list,
                         GimpDrawable *drawable)
{
   dialog->list = list;
   dialog->drawable = drawable;
}

static void
do_create_gimp_guides_dialog (ObjectList_t *list,
                              GimpDrawable *drawable)
{
   static GimpGuidesDialog_t *dialog;

   if (!dialog)
      dialog = make_gimp_guides_dialog();

   init_gimp_guides_dialog(dialog, list, drawable);
   default_dialog_show(dialog->dialog);
}

static CmdExecuteValue_t gimp_guides_command_execute(Command_t *parent);

static CommandClass_t gimp_guides_command_class = {
   NULL,                        /* guides_command_destruct */
   gimp_guides_command_execute,
   NULL,                        /* guides_command_undo */
   NULL                         /* guides_command_redo */
};

typedef struct {
  Command_t parent;
  ObjectList_t *list;
  GimpDrawable *drawable;
} GimpGuidesCommand_t;

Command_t*
gimp_guides_command_new (ObjectList_t *list,
                         GimpDrawable *drawable)
{
   GimpGuidesCommand_t *command = g_new(GimpGuidesCommand_t, 1);
   command->list = list;
   command->drawable = drawable;
   return command_init(&command->parent, _("Use GIMP Guides"),
                       &gimp_guides_command_class);
}

static CmdExecuteValue_t
gimp_guides_command_execute(Command_t *parent)
{
   GimpGuidesCommand_t *command = (GimpGuidesCommand_t*) parent;
   do_create_gimp_guides_dialog(command->list, command->drawable);
   return CMD_DESTRUCT;
}
