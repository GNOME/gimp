/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 *
 */

#ifndef __GFIG_DIALOG_H__
#define __GFIG_DIALOG_H__

#define RESPONSE_UNDO    1
#define RESPONSE_CLEAR   2
#define RESPONSE_SAVE    3
#define RESPONSE_PAINT   4

gint   undo_water_mark;  /* Last slot filled in -1 = no undo */
GList *undo_table[MAX_UNDO];

gint   gfig_dialog             (void);
void   update_options          (GFigObj *old_obj);

void   tool_option_page_update (GtkWidget *button,
                                GtkWidget *notebook);

#endif /* __GFIG_DIALOG_H__ */
