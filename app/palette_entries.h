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
#ifndef __PALETTE_ENTRIES_H__
#define __PALETTE_ENTRIES_H__

#include "gtk/gtk.h"
#include "general.h"

struct _PaletteEntries {
  char *name;
  char *filename;
  GSList *colors;
  int n_colors;
  int changed;
  GdkPixmap *pixmap;
};

typedef struct _PaletteEntries _PaletteEntries, *PaletteEntriesP;

struct _PaletteEntry {
  unsigned char color[3];
  char *name;
  int position;
};
typedef struct _PaletteEntry _PaletteEntry, *PaletteEntryP;

extern GSList * palette_entries_list;

#endif /* __PALETTE_H__ */
