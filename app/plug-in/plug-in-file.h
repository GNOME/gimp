/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-file.h
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

#ifndef __PLUG_IN_FILE_H__
#define __PLUG_IN_FILE_H__


gboolean   plug_in_file_register_load_handler (Gimp        *gimp,
                                               const gchar *name,
                                               const gchar *extensions,
                                               const gchar *prefixes,
                                               const gchar *magics);
gboolean   plug_in_file_register_save_handler (Gimp        *gimp,
                                               const gchar *name,
                                               const gchar *extensions,
                                               const gchar *prefixes);

gboolean   plug_in_file_register_mime_type    (Gimp        *gimp,
                                               const gchar *name,
                                               const gchar *mime_type);

gboolean   plug_in_file_register_thumb_loader (Gimp        *gimp,
                                               const gchar *load_proc,
                                               const gchar *thumb_proc);


#endif /* __PLUG_IN_FILE_H__ */
