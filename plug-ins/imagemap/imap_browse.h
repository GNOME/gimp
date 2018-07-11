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

#ifndef _IMAP_BROWSE_H
#define _IMAP_BROWSE_H

typedef gchar* (*BrowseFilter_t) (const gchar *, gpointer data);

typedef struct
{
   const gchar    *name;
   BrowseFilter_t  filter;
   gpointer        filter_data;
   GtkWidget      *hbox;
   GtkWidget      *file;
   GtkWidget      *button;
   GtkWidget      *file_chooser;
} BrowseWidget_t;

BrowseWidget_t * browse_widget_new          (const gchar    *name);
void             browse_widget_set_filename (BrowseWidget_t *browse,
                                             const gchar    *filename);
void             browse_widget_set_filter   (BrowseWidget_t *browse,
                                             BrowseFilter_t  filter,
                                             gpointer        data);

#endif /* _IMAP_BROWSE_H */
