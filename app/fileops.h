/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#ifndef __FILE_IO_H__
#define __FILE_IO_H__


#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <process.h>		/* For _getpid() */
#endif

#include "plug_in.h"
#include "gimpimageF.h"


void file_ops_pre_init               (void);
void file_ops_post_init              (void);
void file_open_callback              (GtkWidget *w,
				      gpointer   client_data);
void file_save_callback              (GtkWidget *w,
				      gpointer   client_data);
void file_save_as_callback           (GtkWidget *w,
				      gpointer   client_data);
void file_revert_callback            (GtkWidget *w,
				      gpointer   client_data);
void file_load_by_extension_callback (GtkWidget *w,
				      gpointer   client_data);
void file_save_by_extension_callback (GtkWidget *w,
				      gpointer   client_data);
int  file_open                       (char      *filename,
				      char      *raw_filename);
int  file_save                       (GimpImage*  gimage,
				      char      *filename,
				      char      *raw_filename,
				      gint      mode);

PlugInProcDef* file_proc_find        (GSList *procs,
				      char   *filename);

extern GSList *load_procs;
extern GSList *save_procs;

#endif /* FILE_IO_H */
