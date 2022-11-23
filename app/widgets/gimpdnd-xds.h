/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmadnd-xds.c
 * Copyright (C) 2005  Sven Neumann <sven@ligma.org>
 *
 * Saving Files via Drag-and-Drop:
 * The Direct Save Protocol for the X Window System
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

#ifndef __LIGMA_DND_XDS_H__
#define __LIGMA_DND_XDS_H__


void  ligma_dnd_xds_source_set (GdkDragContext   *context,
                               LigmaImage        *image);
void  ligma_dnd_xds_save_image (GdkDragContext   *context,
                               LigmaImage        *image,
                               GtkSelectionData *selection);


#endif /* __LIGMA_DND_XDS_H__ */
