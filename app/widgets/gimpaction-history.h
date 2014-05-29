/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpaction-history.h
 * Copyright (C) 2013  Jehan <jehan at girinstud.io>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_ACTION_HISTORY_H__
#define __GIMP_ACTION_HISTORY_H__

typedef gboolean   (* GimpActionMatchFunc)       (GtkAction           *action,
                                                  const gchar         *keyword,
                                                  gint                *section,
                                                  gboolean             match_fuzzy);

void       gimp_action_history_init              (GimpGuiConfig       *config);
void       gimp_action_history_exit              (GimpGuiConfig       *config);

void       gimp_action_history_activate_callback (GtkAction           *action,
                                                  gpointer             user_data);

void       gimp_action_history_empty             (void);

GList *    gimp_action_history_search            (const gchar         *keyword,
                                                  GimpActionMatchFunc  match_func,
                                                  GimpGuiConfig       *config);

gboolean   gimp_action_history_excluded_action   (const gchar         *action_name);

#endif  /* __GIMP_ACTION_HISTORY_H__ */
