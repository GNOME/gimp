/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#ifndef _IMAP_PREFERENCES_H
#define _IMAP_PREFERENCES_H

#include "imap_default_dialog.h"

typedef struct {
   GdkColor normal_fg;
   GdkColor normal_bg;
   GdkColor selected_fg;
   GdkColor selected_bg;
} ColorSelData_t;

typedef struct {
   gint 		default_map_type;
   gboolean 		prompt_for_area_info;
   gboolean 		require_default_url;
   gboolean 		show_area_handle;
   gboolean 		keep_circles_round;
   gboolean 		show_url_tip;
   gboolean 		use_doublesized;
   gint			undo_levels;
   gint			mru_size;
   ColorSelData_t	colors;
   GdkGC 	       *normal_gc;
   GdkGC 	       *selected_gc;
} PreferencesData_t;

typedef struct {
   DefaultDialog_t 	*dialog;
   GtkWidget		*notebook;
   GtkWidget		*ncsa;
   GtkWidget		*cern;
   GtkWidget		*csim;
   GtkWidget		*prompt_for_area_info;
   GtkWidget		*require_default_url;
   GtkWidget		*show_area_handle;
   GtkWidget		*keep_circles_round;
   GtkWidget		*show_url_tip;
   GtkWidget		*use_doublesized;

   GtkWidget		*undo_levels;
   GtkWidget		*mru_size;

   GtkWidget		*normal_fg;
   GtkWidget		*normal_bg;
   GtkWidget		*selected_fg;
   GtkWidget		*selected_bg;
   GtkWidget		*color_sel_dlg;
   GtkWidget		*color_sel;

   PreferencesData_t	*old_data;
   ColorSelData_t	old_colors;
   ColorSelData_t	new_colors;
} PreferencesDialog_t;

void do_preferences_dialog();
gboolean preferences_load(PreferencesData_t *data);
void preferences_save(PreferencesData_t *data);

#endif /* _IMAP_PREFERENCES_H */
