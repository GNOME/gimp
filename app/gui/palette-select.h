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

#ifndef __PALETTE_SELECT_H_
#define __PALETTE_SELECT_H_

#define SM_PREVIEW_WIDTH (96 + 2)
#define SM_PREVIEW_HEIGHT (33 + 2)

struct _PaletteSelect {
  GtkWidget *shell;
  GtkWidget *clist;
  GdkGC *gc;
  gchar *callback_name;
};

typedef struct _PaletteSelect PaletteSelect, *PaletteSelectP;

void              palette_select_set_text_all(PaletteEntriesP entries);
PaletteSelectP    palette_new_selection(gchar * title,gchar * initial_palette);
void              palette_select_clist_insert_all(PaletteEntriesP p_entries);
void              palette_select_set_text_all(PaletteEntriesP entries);
void              palette_select_refresh_all(void);
void              palette_clist_init(GtkWidget *clist,GtkWidget *shell,GdkGC *gc);
void              palette_select_palette_init(void);
void              palette_create_edit(PaletteEntriesP entries);
void              palette_insert_clist(GtkWidget *clist, 
				       GtkWidget *shell,
				       GdkGC     *gc,
				       PaletteEntriesP p_entries,gint pos);


extern            PaletteSelectP top_level_palette;

#endif  /* __PALETTE_SELECT_H_ */
