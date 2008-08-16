/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
   gboolean		auto_convert;
   gdouble		threshold;
   gint			undo_levels;
   gint			mru_size;
   ColorSelData_t	colors;
   GdkGC 	       *normal_gc;
   GdkGC 	       *selected_gc;
} PreferencesData_t;

void do_preferences_dialog(void);
gboolean preferences_load(PreferencesData_t *data);
void preferences_save(PreferencesData_t *data);

#endif /* _IMAP_PREFERENCES_H */
