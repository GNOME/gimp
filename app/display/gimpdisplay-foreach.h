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

#ifndef __GIMP_DISPLAY_FOREACH_H__
#define __GIMP_DISPLAY_FOREACH_H__


void          gdisplays_foreach                 (GFunc                 func,
                                                 gpointer              user_data);
GimpDisplay * gdisplays_check_valid             (GimpDisplay          *gdisp,
                                                 GimpImage            *gimage);
void          gdisplays_reconnect               (GimpImage            *old,
                                                 GimpImage            *new);
void          gdisplays_update_title            (GimpImage            *gimage);
void          gdisplays_resize_cursor_label     (GimpImage            *gimage);
void          gdisplays_setup_scale             (GimpImage            *gimage);
void          gdisplays_update_area             (GimpImage            *gimage,
                                                 gint                  x,
                                                 gint                  y,
                                                 gint                  w,
                                                 gint                  h);
void          gdisplays_expose_guides           (GimpImage            *gimage);
void          gdisplays_expose_guide            (GimpImage            *gimage,
                                                 GimpGuide            *guide);
void          gdisplays_update_full             (GimpImage            *gimage);
void          gdisplays_shrink_wrap             (GimpImage            *gimage);
void          gdisplays_expose_full             (void);
void          gdisplays_selection_visibility    (GimpImage            *gimage,
                                                 GimpSelectionControl  control);
gboolean      gdisplays_dirty                   (void);
void          gdisplays_delete                  (void);
void          gdisplays_flush                   (void);
void          gdisplays_flush_now               (void);
void          gdisplays_nav_preview_resized     (void);
void          gdisplays_set_busy                (void);
void          gdisplays_unset_busy              (void);


#endif /*  __GIMP_DISPLAY_FOREACH_H__  */
