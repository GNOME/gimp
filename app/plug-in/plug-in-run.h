/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-run.h
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

#ifndef __PLUG_IN_RUN_H__
#define __PLUG_IN_RUN_H__


/*  Run a plug-in as if it were a procedure database procedure
 */
Argument * plug_in_run    (Gimp         *gimp,
                           GimpContext  *context,
                           GimpProgress *progress,
                           ProcRecord   *proc_rec,
                           Argument     *args,
                           gint          argc,
                           gboolean      synchronous,
                           gboolean      destroy_return_vals,
                           gint          gdisp_ID);

/*  Run the last plug-in again with the same arguments. Extensions
 *  are exempt from this "privelege".
 */
void       plug_in_repeat (Gimp         *gimp,
                           GimpContext  *context,
                           GimpProgress *progress,
                           gint          display_ID,
                           gint          image_ID,
                           gint          drawable_ID,
                           gboolean      with_interface);


#endif /* __PLUG_IN_RUN_H__ */
