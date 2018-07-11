/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2002 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#include <stdarg.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include "imap_default_dialog.h"
#include "imap_main.h"
#include "imap_source.h"

#include "libgimp/stdplugins-intl.h"

static void   save_to_view (GtkTextBuffer *buffer,
                            const char    *format,
                            ...) G_GNUC_PRINTF(2,3);

static void
save_to_view(GtkTextBuffer *buffer, const char* format, ...)
{
   va_list ap;
   char scratch[1024];
   GtkTextIter iter;

   va_start(ap, format);
   vsprintf(scratch, format, ap);
   va_end(ap);

   gtk_text_buffer_get_end_iter(buffer, &iter);
   gtk_text_buffer_insert(buffer, &iter, scratch, -1);
}

void
do_source_dialog(void)
{
   static DefaultDialog_t *dialog;
   static GtkWidget *text;
   static GtkTextBuffer *buffer;

   if (!dialog)
     {
      GtkWidget *swin;

      dialog = make_default_dialog(_("View Source"));
      default_dialog_hide_cancel_button(dialog);
      default_dialog_hide_apply_button(dialog);

      buffer = gtk_text_buffer_new(NULL);

      text = gtk_text_view_new_with_buffer(buffer);
      gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
      gtk_widget_show(text);

      swin = gtk_scrolled_window_new(NULL, NULL);
      gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin),
                                          GTK_SHADOW_IN);
      gtk_widget_set_size_request(swin, 400, 300);
      gtk_box_pack_start(GTK_BOX(dialog->vbox), swin, TRUE, TRUE, 0);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
                                     GTK_POLICY_AUTOMATIC,
                                     GTK_POLICY_AUTOMATIC);
      gtk_widget_show(swin);
      gtk_container_add(GTK_CONTAINER(swin), text);
   }
   gtk_text_buffer_set_text(buffer, "", -1);
   dump_output(buffer, (OutputFunc_t) save_to_view);

   default_dialog_show(dialog);
}
