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

#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__


#include <stdio.h>


void            file_dialog_show      (GtkWidget     *filesel);
void            file_dialog_hide      (GtkWidget     *filesel);

void            file_update_name      (PlugInProcDef *proc,
				       GtkWidget     *filesel);
void            file_update_menus     (GSList        *procs,
				       gint           image_type);

PlugInProcDef * file_proc_find        (GSList        *procs,
				       const gchar   *filename);

/* Return values are 0: no match, 1: magic match, 2: size match */
gint            file_check_magic_list (GSList        *magics_list,
				       gint           headsize,
				       guchar        *head,
				       FILE          *ifp);


TempBuf       * make_thumb_tempbuf    (GimpImage     *gimage);
guchar        * readXVThumb           (const gchar   *fnam,
				       gint          *w,
				       gint          *h,
				       gchar        **imginfo /* caller frees if != NULL */);
gboolean        file_save_thumbnail   (GimpImage     *gimage,
				       const char    *full_source_filename,
				       TempBuf       *tempbuf);


#endif /* __FILE_UTILS_H__ */
