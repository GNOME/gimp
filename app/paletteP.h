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
#ifndef __PALETTE_P_H__
#define __PALETTE_P_H__

#include <gtk/gtk.h>

#include "gimpimageF.h"
#include "palette_entries.h"

void   palette_clist_init           (GtkWidget      *clist,
				     GtkWidget      *shell,
				     GdkGC          *gc);
void   palette_clist_insert         (GtkWidget      *clist, 
				     GtkWidget      *shell,
				     GdkGC          *gc,
				     PaletteEntries *entries,
				     gint            pos);

void   palette_select_palette_init  (void);
void   palette_create_edit          (PaletteEntries *entries);

void   palette_import_image_renamed (GimpImage      *gimage);

#endif /* __PALETTE_P_H__ */
