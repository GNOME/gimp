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

#ifndef __FILE_OPEN_H__
#define __FILE_OPEN_H__


extern GSList *load_procs;


GimpImage * file_open_image             (Gimp          *gimp,
					 const gchar   *filename,
					 const gchar   *raw_filename,
					 const gchar   *open_mode,
					 PlugInProcDef *file_proc,
					 RunModeType    run_mode,
					 gint          *status);

gchar     * file_open_absolute_filename (const gchar   *name);


#endif /* __FILE_OPEN_H__ */
