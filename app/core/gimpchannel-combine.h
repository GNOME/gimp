/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_CHANNEL_COMBINE_H__
#define __LIGMA_CHANNEL_COMBINE_H__


void   ligma_channel_combine_rect         (LigmaChannel    *mask,
                                          LigmaChannelOps  op,
                                          gint            x,
                                          gint            y,
                                          gint            w,
                                          gint            h);
void   ligma_channel_combine_ellipse      (LigmaChannel    *mask,
                                          LigmaChannelOps  op,
                                          gint            x,
                                          gint            y,
                                          gint            w,
                                          gint            h,
                                          gboolean        antialias);
void   ligma_channel_combine_ellipse_rect (LigmaChannel    *mask,
                                          LigmaChannelOps  op,
                                          gint            x,
                                          gint            y,
                                          gint            w,
                                          gint            h,
                                          gdouble         rx,
                                          gdouble         ry,
                                          gboolean        antialias);
void   ligma_channel_combine_mask         (LigmaChannel    *mask,
                                          LigmaChannel    *add_on,
                                          LigmaChannelOps  op,
                                          gint            off_x,
                                          gint            off_y);
void   ligma_channel_combine_buffer       (LigmaChannel    *mask,
                                          GeglBuffer     *add_on_buffer,
                                          LigmaChannelOps  op,
                                          gint            off_x,
                                          gint            off_y);

void   ligma_channel_combine_items        (LigmaChannel    *mask,
                                          GList          *items,
                                          LigmaChannelOps  op);


#endif /* __LIGMA_CHANNEL_COMBINE_H__ */
