/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui.h
 * Copyright (C) 2017 Ell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PROP_GUI_EVAL_H__
#define __GIMP_PROP_GUI_EVAL_H__


gboolean   gimp_prop_eval_boolean (GObject     *config,
                                   GParamSpec  *pspec,
                                   const gchar *key,
                                   gboolean     default_value);

gchar    * gimp_prop_eval_string  (GObject     *config,
                                   GParamSpec  *pspec,
                                   const gchar *key,
                                   const gchar *default_value);


#endif /* __GIMP_PROP_GUI_EVAL_H__ */
