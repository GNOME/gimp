/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaaction-history.h
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_ACTION_HISTORY_H__
#define __LIGMA_ACTION_HISTORY_H__


typedef gboolean (* LigmaActionMatchFunc) (LigmaAction  *action,
                                          const gchar *keyword,
                                          gint        *section,
                                          Ligma        *ligma);


void       ligma_action_history_init                  (Ligma                *ligma);
void       ligma_action_history_exit                  (Ligma                *ligma);

void       ligma_action_history_clear                 (Ligma                *ligma);

GList    * ligma_action_history_search                (Ligma                *ligma,
                                                      LigmaActionMatchFunc  match_func,
                                                      const gchar         *keyword);

gboolean   ligma_action_history_is_blacklisted_action (const gchar         *action_name);
gboolean   ligma_action_history_is_excluded_action    (const gchar         *action_name);

void       ligma_action_history_action_activated      (LigmaAction          *action);


#endif  /* __LIGMA_ACTION_HISTORY_H__ */
