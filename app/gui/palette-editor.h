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
#ifndef __PALETTE_H__
#define __PALETTE_H__

#include <glib.h>

/*  The states for updating a color in the palette via palette_set_* calls */
#define COLOR_NEW         0
#define COLOR_UPDATE_NEW  1
#define COLOR_UPDATE      2

void   palettes_init            (gboolean no_data);
void   palettes_free            (void);

void   palette_dialog_create    (void);
void   palette_dialog_free      (void);

void   palette_set_active_color (gint     r,
				 gint     g,
				 gint     b,
				 gint     state);

#endif /* __PALETTE_H__ */
