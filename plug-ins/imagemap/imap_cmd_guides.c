/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 *
 */

#include <stdio.h>

#include "imap_cmd_create.h"
#include "imap_default_dialog.h"
#include "imap_cmd_guides.h"
#include "imap_main.h"
#include "imap_rectangle.h"
#include "imap_table.h"

typedef struct {
   DefaultDialog_t 	*dialog;
   GtkWidget		*image_dimensions;
   GtkWidget		*guide_bounds;
   GtkWidget		*width;
   GtkWidget		*height;
   GtkWidget		*left;
   GtkWidget		*top;
   GtkWidget		*horz_spacing;
   GtkWidget		*vert_spacing;
   GtkWidget		*no_across;
   GtkWidget		*no_down;
   GtkWidget		*base_url;

   ObjectList_t		*list;
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

   subcommand_start("Create Guides");
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
   redraw_preview();
}

static void
recalc_bounds(GtkWidget *widget, gpointer data)
{
   GuidesDialog_t *param = (GuidesDialog_t*) data;
   gint width, height, left, top, hspace, vspace, rows, cols;
   gint bound_w, bound_h;
   char bounds[128];

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

   sprintf(bounds, "Resulting Guide Bounds: %d,%d to %d,%d (%d areas)",
	   left, top, left + bound_w, top + bound_h, rows * cols);
   if (left + bound_w > get_image_width() || 
       top + bound_h > get_image_height()) {
      default_dialog_set_ok_sensitivity(param->dialog, FALSE);
   } else {
      default_dialog_set_ok_sensitivity(param->dialog, TRUE);
   }
   gtk_label_set_text(GTK_LABEL(param->guide_bounds), bounds);
}

static GuidesDialog_t*
make_guides_dialog()
{
   GuidesDialog_t *data = g_new(GuidesDialog_t, 1);
   DefaultDialog_t *dialog;
   GtkWidget *table;
   GtkWidget *label;
   GtkWidget *hbox;
   
   dialog = data->dialog = make_default_dialog("Create Guides");
   default_dialog_set_ok_cb(dialog, guides_ok_cb, data);
   
   label = gtk_label_new(
      "Guides are pre-defined rectangles covering the image. You define\n"
      "them by their width, height, and spacing from each other. This\n"
      "allows you to rapidly create the most common image map type -\n"
      "image collection of \"thumbnails\", suitable for navigation bars.");
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
   gtk_container_set_border_width(
      GTK_CONTAINER(GTK_DIALOG(dialog->dialog)->vbox), 10);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog->dialog)->vbox), label, 
		      TRUE, TRUE, 10);
   gtk_widget_show(label);
   
   data->image_dimensions = gtk_label_new("");
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog->dialog)->vbox), 
		     data->image_dimensions);
   gtk_widget_show(data->image_dimensions);

   data->guide_bounds = gtk_label_new("");
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog->dialog)->vbox), 
		     data->guide_bounds);
   gtk_widget_show(data->guide_bounds);

   table = gtk_table_new(4, 4, FALSE);
   gtk_container_set_border_width(GTK_CONTAINER(table), 10);
   gtk_table_set_row_spacings(GTK_TABLE(table), 10);
   gtk_table_set_col_spacings(GTK_TABLE(table), 10);
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog->dialog)->vbox), table);
   gtk_widget_show(table);

   create_label_in_table(table, 0, 0, "Width");
   data->width = create_spin_button_in_table(table, 0, 1, 32, 1, 100);
   gtk_signal_connect(GTK_OBJECT(data->width), "changed",
		      GTK_SIGNAL_FUNC(recalc_bounds), (gpointer) data);

   create_label_in_table(table, 0, 2, "Left Start at");
   data->left = create_spin_button_in_table(table, 0, 3, 0, 0, 100);
   gtk_signal_connect(GTK_OBJECT(data->left), "changed",
		      GTK_SIGNAL_FUNC(recalc_bounds), (gpointer) data);

   create_label_in_table(table, 1, 0, "Height");
   data->height = create_spin_button_in_table(table, 1, 1, 32, 1, 100);
   gtk_signal_connect(GTK_OBJECT(data->height), "changed",
		      GTK_SIGNAL_FUNC(recalc_bounds), (gpointer) data);

   create_label_in_table(table, 1, 2, "Top Start at");
   data->top = create_spin_button_in_table(table, 1, 3, 0, 0, 100);
   gtk_signal_connect(GTK_OBJECT(data->top), "changed",
		      GTK_SIGNAL_FUNC(recalc_bounds), (gpointer) data);

   create_label_in_table(table, 2, 0, "Horz. Spacing");
   data->horz_spacing = create_spin_button_in_table(table, 2, 1, 0, 0, 100);
   gtk_signal_connect(GTK_OBJECT(data->horz_spacing), "changed",
		      GTK_SIGNAL_FUNC(recalc_bounds), (gpointer) data);

   create_label_in_table(table, 2, 2, "No. Across");
   data->no_across = create_spin_button_in_table(table, 2, 3, 0, 0, 100);
   gtk_signal_connect(GTK_OBJECT(data->no_across), "changed",
		      GTK_SIGNAL_FUNC(recalc_bounds), (gpointer) data);

   create_label_in_table(table, 3, 0, "Vert. Spacing");
   data->vert_spacing = create_spin_button_in_table(table, 3, 1, 0, 0, 100);
   gtk_signal_connect(GTK_OBJECT(data->vert_spacing), "changed",
		      GTK_SIGNAL_FUNC(recalc_bounds), (gpointer) data);

   create_label_in_table(table, 3, 2, "No. Down");
   data->no_down = create_spin_button_in_table(table, 3, 3, 0, 0, 100);
   gtk_signal_connect(GTK_OBJECT(data->no_down), "changed",
		      GTK_SIGNAL_FUNC(recalc_bounds), (gpointer) data);

   hbox = gtk_hbox_new(FALSE, 1);
   gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
   gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog->dialog)->vbox), hbox);
   label = gtk_label_new("Base URL:");
   gtk_widget_show(label);
   gtk_container_add(GTK_CONTAINER(hbox), label);
   data->base_url = gtk_entry_new();
   gtk_container_add(GTK_CONTAINER(hbox), data->base_url);
   gtk_widget_show(data->base_url);
   gtk_widget_show(hbox);

   return data;
}

static void
init_guides_dialog(GuidesDialog_t *dialog, ObjectList_t *list)
{
   char dimension[128];

   dialog->list = list;
   sprintf(dimension, "Image dimensions: %d x %d", get_image_width(),
	   get_image_height());
   gtk_label_set_text(GTK_LABEL(dialog->image_dimensions), dimension);
   gtk_label_set_text(GTK_LABEL(dialog->guide_bounds), 
		      "Resulting Guide Bounds: 0,0 to 0,0 (0 areas)");
   gtk_widget_grab_focus(dialog->width);
}

static void
do_create_guides_dialog(ObjectList_t *list)
{
   static GuidesDialog_t *dialog;

   if (!dialog)
      dialog = make_guides_dialog();

   init_guides_dialog(dialog, list);
   default_dialog_show(dialog->dialog);
}

static gboolean guides_command_execute(Command_t *parent);

static CommandClass_t guides_command_class = {
   NULL,			/* guides_command_destruct */
   guides_command_execute,
   NULL,			/* guides_command_undo */
   NULL				/* guides_command_redo */
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
   return command_init(&command->parent, "Guides", &guides_command_class);
}

static gboolean
guides_command_execute(Command_t *parent)
{
   GuidesCommand_t *command = (GuidesCommand_t*) parent;
   do_create_guides_dialog(command->list);
   return FALSE;
}
