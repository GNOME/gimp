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

#ifndef __GIMP_PROGRESS_H__
#define __GIMP_PROGRESS_H__


GimpProgress * gimp_progress_start            (GimpDisplay   *gdisp,
                                               const gchar   *message,
                                               gboolean       important,
                                               GCallback      cancel_callback,
                                               gpointer       cancel_data);
GimpProgress * gimp_progress_restart          (GimpProgress  *progress,
                                               const gchar   *message,
                                               GCallback      cancel_callback,
                                               gpointer       cancel_data);
void           gimp_progress_update           (GimpProgress  *progress,
                                               gdouble        percentage);
void           gimp_progress_step             (GimpProgress  *progress);
void           gimp_progress_end              (GimpProgress  *progress);

void           gimp_progress_update_and_flush (gint           min,
                                               gint           max,
                                               gint           current,
                                               gpointer       data);


#endif /* __GIMP_PROGRESS_H__ */
