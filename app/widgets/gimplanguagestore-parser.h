/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplanguagestore-parser.h
 * Copyright (C) 2008, 2009  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_LANGUAGE_STORE_PARSER_H__
#define __GIMP_LANGUAGE_STORE_PARSER_H__


void        gimp_language_store_parser_init          (void);

void        gimp_language_store_parser_clean         (void);

GHashTable* gimp_language_store_parser_get_languages (gboolean localization_only);

#endif  /* __GIMP_LANGUAGE_STORE_PARSER_H__ */
