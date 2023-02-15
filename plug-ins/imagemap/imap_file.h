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

#ifndef _IMAP_FILE_H
#define _IMAP_FILE_H

void do_file_open_dialog    (GSimpleAction *action,
                             GVariant      *parameter,
                             gpointer       user_data);
void do_file_save_as_dialog (GSimpleAction *action,
                             GVariant      *parameter,
                             gpointer       user_data);
void do_file_error_dialog   (const char    *error,
                             const char    *filename);

gboolean load_csim (const char* filename);
gboolean load_cern (const char* filename);
gboolean load_ncsa (const char* filename);

#endif /* _IMAP_FILE_H */
