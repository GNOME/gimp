/* gap_filter_iterators.h
 *
 * 1997.12.18 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * - Headers for XXX_Iterator_ALT Procedures 
 *
 * for now i made some Iterator Plugins using the ending _ALT,
 * If New plugins were added to the gimp, or existing ones were updated,
 * the Authors should supply original _Iterator Procedures
 * (without the _ALT ending)
 * This Procedures are then used instead of my (Hacked _ALT) versions.
 * to modify the settings for the plugin when called step by step
 * on animated multilayer Images.
 * without name conflicts.
 */

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

#ifndef _GAP_FILTER_ITERATORS_H
#define _GAP_FILTER_ITERATORS_H

typedef gint (*t_iter_ALT_func) (GimpRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct);

 
typedef struct t_iter_ALT_tab
{
  char            *proc_name;
  t_iter_ALT_func  proc_func;
} t_iter_ALT_tab;

#endif
