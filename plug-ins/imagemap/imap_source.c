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

#include <stdarg.h>
#include <stdio.h>

#include "imap_default_dialog.h"
#include "imap_main.h"
#include "imap_source.h"

static void 
save_to_view(gpointer param, const char* format, ...)
{
   va_list ap;
   char scratch[1024];

   va_start(ap, format);
   vsprintf(scratch, format, ap);
   va_end(ap);

   gtk_text_insert(GTK_TEXT(param), NULL, NULL, NULL, scratch, -1);
}


void 
do_source_dialog(void)
{
   static DefaultDialog_t *dialog;
   static GtkWidget *text;
   if (!dialog) {
      GtkWidget *window;

      dialog = make_default_dialog("View Source");
      default_dialog_hide_cancel_button(dialog);
      default_dialog_hide_apply_button(dialog);

      text = gtk_text_new(NULL, NULL);
      gtk_widget_show(text);

      window = gtk_scrolled_window_new(GTK_TEXT(text)->hadj, 
				       GTK_TEXT(text)->vadj);
      gtk_widget_set_usize(window, 640, 400);
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog->dialog)->vbox), window, 
			 TRUE, TRUE, 0);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window), 
				     GTK_POLICY_AUTOMATIC, 
				     GTK_POLICY_AUTOMATIC);
      gtk_widget_show(window);
      gtk_container_add(GTK_CONTAINER(window), text);
      
   }
   gtk_text_freeze(GTK_TEXT(text));
   gtk_editable_delete_text(GTK_EDITABLE(text), 0, -1);
   dump_output(text, save_to_view);
   gtk_text_thaw(GTK_TEXT(text));

   default_dialog_show(dialog);
}
