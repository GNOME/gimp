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
#include "procedural_db.h"

#define FOREGROUND 0
#define BACKGROUND 1

/*  The states for updating a color in the palette via palette_set_* calls */
#define COLOR_NEW         0
#define COLOR_UPDATE_NEW  1
#define COLOR_UPDATE      2

void palettes_init (int no_data);
void palettes_free (void);
void palette_create (void);
void palette_free (void);
void palette_get_foreground (unsigned char *, unsigned char *, unsigned char *);
void palette_get_background (unsigned char *, unsigned char *, unsigned char *);
void palette_set_foreground (int, int, int);
void palette_set_background (int, int, int);
void palette_set_active_color (int, int, int, int);
void palette_set_default_colors (void);
void palette_swap_colors (void);
void palette_init_palettes (int no_data);
void palette_free_palettes (void);
void palette_import_image_new(GimpImage * gimage);
void palette_import_image_destroyed(GimpImage* gimage);
void palette_import_image_renamed(GimpImage* gimage);

#endif /* __PALETTE_H__ */
