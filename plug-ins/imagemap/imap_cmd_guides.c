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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "imap_commands.h"
#include "imap_default_dialog.h"
#include "imap_main.h"
#include "imap_rectangle.h"
#include "imap_ui_grid.h"

#include "libgimp/stdplugins-intl.h"

typedef struct {
   DefaultDialog_t      *dialog;
   GtkWidget            *image_dimensions;
   GtkWidget            *guide_bounds;
   GtkWidget            *width;
   GtkWidget            *height;
   GtkWidget            *left;
   GtkWidget            *top;
   GtkWidget            *horz_spacing;
   GtkWidget            *vert_spacing;
   GtkWidget            *no_across;
   GtkWidget            *no_down;
   GtkWidget            *base_url;

   ObjectList_t         *list;
} GuidesDialog_t;

static void
guides_ok_cb(gpointer data)
{
   GuidesDialog_t *param = (GuidesDialog_t*) data;
   gint y;
   int i, j;
   gint width, height, left, top, hspace, vspace, rows, cols;

   width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->width));
   height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->height));
   left = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->left));
   top = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->top));
   hspace = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->horz_spacing));
   vspace = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->vert_spacing));
   rows = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->no_down));
   cols = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->no_across));

   subcommand_start(_("Create Guides"));
   y = top;
   for (i = 0; i < rows; i++) {
      gint x = left;
      for (j = 0; j < cols; j++) {
         Object_t *obj = create_rectangle(x, y, width, height);
         Command_t *command = create_command_new(param->list, obj);

         object_set_url(obj, gtk_entry_get_text(GTK_ENTRY(param->base_url)));
         command_execute(command);
         x += width + hspace;
      }
      y += height + vspace;
   }
   subcommand_end();
}

static void
recalc_bounds(GtkWidget *widget, gpointer data)
{
   GuidesDialog_t *param = (GuidesDialog_t*) data;
   gint width, height, left, top, hspace, vspace, rows, cols;
   gint bound_w, bound_h;
   gchar *bounds;

   width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->width));
   height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->height));
   left = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->left));
   top = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->top));
   hspace = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->horz_spacing));
   vspace = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->vert_spacing));
   rows = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->no_down));
   cols = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(param->no_across));

   bound_w = (width + hspace) * cols - hspace;
   bound_h = (height + vspace) * rows - vspace;

   bounds = g_strdup_printf (_("Resulting Guide Bounds: %d,%d to %d,%d (%d areas)"),
                             left, top, left + bound_w, top + bound_h, rows * cols);
   if (left + bound_w > get_image_width() ||
       top + bound_h > get_image_height())
     {
       gtk_dialog_set_response_sensitive (GTK_DIALOG (param->dialog->dialog),
                                          GTK_RESPONSE_OK, FALSE);
     }
   else
     {
       gtk_dialog_set_response_sensitive (GTK_DIALOG (param->dialog->dialog),
                                          GTK_RESPONSE_OK, TRUE);
     }

   gtk_label_set_text(GTK_LABEL(param->guide_bounds), bounds);
   g_free (bounds);
}

static GuidesDialog_t*
make_guides_dialog (void)
{
   GuidesDialog_t *data = g_new(GuidesDialog_t, 1);
   DefaultDialog_t *dialog;
   GtkWidget *grid;
   GtkWidget *label;
   GtkWidget *hbox;

   dialog = data->dialog = make_default_dialog(_("Create Guides"));
   default_dialog_set_ok_cb (dialog, guides_ok_cb, data);

   hbox = gimp_hint_box_new (
      _("Guides are pre-defined rectangles covering the image. You define "
        "them by their width, height, and spacing from each other. This "
        "allows you to rapidly create the most common image map type - "
        "image collection of \"thumbnails\", suitable for navigation bars."));
   gtk_box_pack_start (GTK_BOX (dialog->vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show (hbox);

   data->image_dimensions = gtk_label_new ("");
   gtk_label_set_xalign (GTK_LABEL (data->image_dimensions), 0.0);
   gtk_box_pack_start (GTK_BOX (dialog->vbox),
                       data->image_dimensions, FALSE, FALSE, 0);
   gtk_widget_show (data->image_dimensions);

   data->guide_bounds = gtk_label_new ("");
   gtk_label_set_xalign (GTK_LABEL (data->guide_bounds), 0.0);
   gtk_box_pack_start (GTK_BOX (dialog->vbox),
                       data->guide_bounds, FALSE, FALSE, 0);
   gtk_widget_show (data->guide_bounds);

   grid = default_dialog_add_grid (dialog);

   label = create_label_in_grid (grid, 0, 0, _("_Width:"));
   data->width = create_spin_button_in_grid (grid, label, 0, 1, 32, 1, 100);
   g_signal_connect (data->width, "changed",
                     G_CALLBACK(recalc_bounds), (gpointer) data);

   label = create_label_in_grid (grid, 0, 2, _("_Left start at:"));
   data->left = create_spin_button_in_grid  (grid, label, 0, 3, 0, 0, 100);
   g_signal_connect (data->left, "changed",
                     G_CALLBACK(recalc_bounds), (gpointer) data);

   label = create_label_in_grid (grid, 1, 0, _("_Height:"));
   data->height = create_spin_button_in_grid (grid, label, 1, 1, 32, 1, 100);
   g_signal_connect (data->height, "changed",
                     G_CALLBACK(recalc_bounds), (gpointer) data);

   label = create_label_in_grid (grid, 1, 2, _("_Top start at:"));
   data->top = create_spin_button_in_grid (grid, label, 1, 3, 0, 0, 100);
   g_signal_connect (data->top, "changed",
                     G_CALLBACK(recalc_bounds), (gpointer) data);

   label = create_label_in_grid (grid, 2, 0, _("_Horz. spacing:"));
   data->horz_spacing = create_spin_button_in_grid (grid, label, 2, 1, 0, 0,
                                                     100);
   g_signal_connect (data->horz_spacing, "changed",
                     G_CALLBACK(recalc_bounds), (gpointer) data);

   label = create_label_in_grid (grid, 2, 2, _("_No. across:"));
   data->no_across = create_spin_button_in_grid (grid, label, 2, 3, 0, 0,
                                                 100);
   g_signal_connect (data->no_across, "changed",
                     G_CALLBACK(recalc_bounds), (gpointer) data);

   label = create_label_in_grid (grid, 3, 0, _("_Vert. spacing:"));
   data->vert_spacing = create_spin_button_in_grid (grid, label, 3, 1, 0, 0,
                                                    100);
   g_signal_connect (data->vert_spacing, "changed",
                     G_CALLBACK(recalc_bounds), (gpointer) data);

   label = create_label_in_grid (grid, 3, 2, _("No. _down:"));
   data->no_down = create_spin_button_in_grid (grid, label, 3, 3, 0, 0, 100);
   g_signal_connect (data->no_down, "changed",
                     G_CALLBACK(recalc_bounds), (gpointer) data);

   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
   gtk_box_pack_start (GTK_BOX (dialog->vbox), hbox, TRUE, TRUE, 0);
   gtk_widget_show(hbox);

   label = gtk_label_new_with_mnemonic(_("Base _URL:"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   gtk_widget_show(label);

   data->base_url = gtk_entry_new ();
   gtk_box_pack_start (GTK_BOX (hbox), data->base_url, TRUE, TRUE, 0);
   gtk_widget_show(data->base_url);

   gtk_label_set_mnemonic_widget (GTK_LABEL (label), data->base_url);

   return data;
}

static void
init_guides_dialog(GuidesDialog_t *dialog, ObjectList_t *list)
{
   gchar *dimension;

   dialog->list = list;
   dimension = g_strdup_printf (_("Image dimensions: %d × %d"),
                                get_image_width(),
                                get_image_height());
   gtk_label_set_text (GTK_LABEL(dialog->image_dimensions), dimension);
   g_free (dimension);
   gtk_label_set_text (GTK_LABEL(dialog->guide_bounds),
                       _("Resulting Guide Bounds: 0,0 to 0,0 (0 areas)"));
   gtk_widget_grab_focus (dialog->width);
}

static void
do_create_guides_dialog_local (ObjectList_t *list)
{
   static GuidesDialog_t *dialog;

   if (!dialog)
      dialog = make_guides_dialog();

   init_guides_dialog(dialog, list);
   default_dialog_show(dialog->dialog);
}

static CmdExecuteValue_t guides_command_execute(Command_t *parent);

static CommandClass_t guides_command_class = {
   NULL,                        /* guides_command_destruct */
   guides_command_execute,
   NULL,                        /* guides_command_undo */
   NULL                         /* guides_command_redo */
};

typedef struct {
   Command_t parent;
   ObjectList_t *list;
} GuidesCommand_t;

Command_t*
guides_command_new(ObjectList_t *list)
{
   GuidesCommand_t *command = g_new(GuidesCommand_t, 1);
   command->list = list;
   return command_init(&command->parent, _("Guides"), &guides_command_class);
}

static CmdExecuteValue_t
guides_command_execute(Command_t *parent)
{
   GuidesCommand_t *command = (GuidesCommand_t*) parent;
   do_create_guides_dialog_local (command->list);
   return CMD_DESTRUCT;
}
