/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 *
 */

#ifndef __GFIG_DIALOG_H__
#define __GFIG_DIALOG_H__

extern gint   undo_level;  /* Last slot filled in -1 = no undo */
extern GList *undo_table[MAX_UNDO];

gboolean  gfig_dialog                      (GimpGfig    *gfig);
void      gfig_dialog_action_set_sensitive (const gchar *name,
                                            gboolean     sensitive);

void      options_update                   (GFigObj     *old_obj);
void      tool_option_page_update          (GtkWidget   *button,
                                            GtkWidget   *notebook);

#endif /* __GFIG_DIALOG_H__ */
