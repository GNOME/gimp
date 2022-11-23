/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmadashpattern.h
 * Copyright (C) 2003 Simon Budig
 * Copyright (C) 2005 Sven Neumann
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

#ifndef __LIGMA_DASH_PATTERN_H__
#define __LIGMA_DASH_PATTERN_H__


#define LIGMA_TYPE_DASH_PATTERN               (ligma_dash_pattern_get_type ())
#define LIGMA_VALUE_HOLDS_DASH_PATTERN(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_DASH_PATTERN))


GType            ligma_dash_pattern_get_type          (void) G_GNUC_CONST;

GArray         * ligma_dash_pattern_new_from_preset   (LigmaDashPreset  preset);
GArray         * ligma_dash_pattern_new_from_segments (const gboolean *segments,
                                                      gint            n_segments,
                                                      gdouble         dash_length);

void             ligma_dash_pattern_fill_segments     (GArray         *pattern,
                                                      gboolean       *segments,
                                                      gint            n_segments);

GArray         * ligma_dash_pattern_from_value_array  (LigmaValueArray *value_array);
LigmaValueArray * ligma_dash_pattern_to_value_array    (GArray         *pattern);

GArray         * ligma_dash_pattern_from_double_array (gint            n_dashes,
                                                      const gdouble  *dashes);
gdouble        * ligma_dash_pattern_to_double_array   (GArray         *pattern,
                                                      gint           *n_dashes);

GArray         * ligma_dash_pattern_copy              (GArray         *pattern);
void             ligma_dash_pattern_free              (GArray         *pattern);


#endif  /*  __LIGMA_DASH_PATTERN_H__  */
