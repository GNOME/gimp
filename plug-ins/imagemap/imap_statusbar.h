/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2003 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#ifndef _IMAP_STATUSBAR_H
#define _IMAP_STATUSBAR_H

typedef struct {
   GtkWidget *status;
   GtkWidget *xy;
   GtkWidget *dimension;
   GtkWidget *zoom;

   gint status_id;
   gint message_id;

   gint zoom_id;
} StatusBar_t;

StatusBar_t *make_statusbar(GtkWidget *main_vbox, GtkWidget *window);
void statusbar_set_status(StatusBar_t *statusbar, const gchar *format, ...) G_GNUC_PRINTF(2,3);
void statusbar_clear_status(StatusBar_t *statusbar);
void statusbar_set_xy(StatusBar_t *statusbar, gint x, gint y);
void statusbar_clear_xy(StatusBar_t *statusbar);
void statusbar_set_dimension(StatusBar_t *statusbar, gint w, gint h);
void statusbar_clear_dimension(StatusBar_t *statusbar);
void statusbar_set_zoom(StatusBar_t *statusbar, gint factor);

#endif /* _IMAP_STATUSBAR_H */
