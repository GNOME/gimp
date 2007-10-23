/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_CHANNEL_COMBINE_H__
#define __GIMP_CHANNEL_COMBINE_H__


void   gimp_channel_add_segment          (GimpChannel    *mask,
                                          gint            x,
                                          gint            y,
                                          gint            width,
                                          gint            value);
void   gimp_channel_sub_segment          (GimpChannel    *mask,
                                          gint            x,
                                          gint            y,
                                          gint            width,
                                          gint            value);
void   gimp_channel_combine_rect         (GimpChannel    *mask,
                                          GimpChannelOps  op,
                                          gint            x,
                                          gint            y,
                                          gint            w,
                                          gint            h);
void   gimp_channel_combine_ellipse      (GimpChannel    *mask,
                                          GimpChannelOps  op,
                                          gint            x,
                                          gint            y,
                                          gint            w,
                                          gint            h,
                                          gboolean        antialias);
void   gimp_channel_combine_ellipse_rect (GimpChannel    *mask,
                                          GimpChannelOps  op,
                                          gint            x,
                                          gint            y,
                                          gint            w,
                                          gint            h,
                                          gdouble         a,
                                          gdouble         b,
                                          gboolean        antialias);
void   gimp_channel_combine_mask         (GimpChannel    *mask,
                                          GimpChannel    *add_on,
                                          GimpChannelOps  op,
                                          gint            off_x,
                                          gint            off_y);


#endif /* __GIMP_CHANNEL_COMBINE_H__ */
