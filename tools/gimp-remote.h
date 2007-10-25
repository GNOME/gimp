/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-remote.h
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

#ifndef __GIMP_REMOTE_H__
#define __GIMP_REMOTE_H__


GdkWindow * gimp_remote_find_toolbox (GdkDisplay  *display,
                                      GdkScreen   *screen);
void        gimp_remote_launch       (GdkScreen   *screen,
                                      const gchar *argv0,
                                      const gchar *startup_id,
                                      gboolean     no_splash,
                                      GString     *file_list) G_GNUC_NORETURN;
gboolean    gimp_remote_drop_files   (GdkDisplay  *display,
                                      GdkWindow   *window,
                                      GString     *file_list);
void        gimp_remote_print_id     (GdkWindow   *window);


#endif /* __GIMP_REMOTE_H__ */
