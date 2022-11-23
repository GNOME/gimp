/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewrenderer-frame.h
 * Copyright (C) 2004 Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_VIEW_RENDERER_FRAME_H__
#define __LIGMA_VIEW_RENDERER_FRAME_H__


GdkPixbuf * ligma_view_renderer_get_frame_pixbuf (LigmaViewRenderer *renderer,
                                                 GtkWidget        *widget,
                                                 gint              width,
                                                 gint              height);

void        ligma_view_renderer_get_frame_size   (gint             *width,
                                                 gint             *height);


#endif /* __LIGMA_VIEW_RENDERER_FRAME_H__ */
