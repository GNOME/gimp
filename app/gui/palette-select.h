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

#ifndef __PALETTE_SELECT_H__
#define __PALETTE_SELECT_H__


typedef struct _PaletteSelect PaletteSelect;

struct _PaletteSelect
{
  GimpContext *context;
  gchar       *callback_name;

  GtkWidget   *shell;
  GtkWidget   *view;
};


PaletteSelect * palette_select_new             (Gimp          *gimp,
                                                GimpContext   *context,
                                                const gchar   *title,
                                                const gchar   *initial_palette,
                                                const gchar   *callback_name);
void            palette_select_free            (PaletteSelect *palette_select);

PaletteSelect * palette_select_get_by_callback (const gchar   *callback_name);
void            palette_select_dialogs_check   (void);


#endif  /* __PALETTE_SELECT_H__ */
