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

#ifndef __GIMPDISPLAY_OPS_H__
#define __GIMPDISPLAY_OPS_H__


gulong  gdisplay_black_pixel        (GimpDisplay *gdisp);
gulong  gdisplay_gray_pixel         (GimpDisplay *gdisp);
gulong  gdisplay_white_pixel        (GimpDisplay *gdisp);
gulong  gdisplay_color_pixel        (GimpDisplay *gdisp);

void    gdisplay_xserver_resolution (gdouble     *xres,
				     gdouble     *yres);

void    gdisplay_new_view           (GimpDisplay *gdisp);
void    gdisplay_close_window       (GimpDisplay *gdisp,
				     gboolean     kill_it);
void    gdisplay_shrink_wrap        (GimpDisplay *gdisp);


#endif  /* __GIMPDISPLAY_OPS_H__ */
