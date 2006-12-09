/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpdashpattern.h
 * Copyright (C) 2003 Simon Budig
 * Copyright (C) 2005 Sven Neumann
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

#ifndef __GIMP_DASH_PATTERN_H__
#define __GIMP_DASH_PATTERN_H__


GArray * gimp_dash_pattern_from_preset   (GimpDashPreset  preset);

GArray * gimp_dash_pattern_from_segments (const gboolean *segments,
                                          gint            n_segments,
                                          gdouble         dash_length);
void     gimp_dash_pattern_segments_set  (GArray         *pattern,
                                          gboolean       *segments,
                                          gint            n_segments);

GArray * gimp_dash_pattern_from_value    (const GValue   *value);
void     gimp_dash_pattern_value_set     (GArray         *pattern,
                                          GValue         *value);

GArray * gimp_dash_pattern_copy          (GArray         *pattern);
void     gimp_dash_pattern_free          (GArray         *pattern);


#endif  /*  __GIMP_DASH_PATTERN_H__  */
