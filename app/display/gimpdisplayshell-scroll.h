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

#ifndef __SCROLL_H__
#define __SCROLL_H__


/*  app init and exit routines  */
void   init_scrolling             (void);
void   free_scrolling             (void);

/*  routines for scrolling the image via the scrollbars  */
void   scrollbar_disconnect       (GtkAdjustment  *adjuatment,
				   gpointer        data);
gint   scrollbar_vert_update      (GtkAdjustment  *adjuatment,
				   gpointer        data);
gint   scrollbar_horz_update      (GtkAdjustment  *adjuatment,
				   gpointer        data);

/*  routines for grabbing the image and scrolling via the pointer  */
void   start_grab_and_scroll      (GDisplay       *gdisp,
				   GdkEventButton *bevent);
void   end_grab_and_scroll        (GDisplay       *gdisp,
				   GdkEventButton *bevent);
void   grab_and_scroll            (GDisplay       *gdisp,
				   GdkEventMotion *mevent);
void   scroll_to_pointer_position (GDisplay       *gdisp,
				   GdkEventMotion *mevent);

/* generic scroll-by-offset function */
gint   scroll_display             (GDisplay       *,
				   gint            ,
				   gint            );


#endif  /*  __SCROLL_H__  */
