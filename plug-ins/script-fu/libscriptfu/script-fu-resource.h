/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __SCRIPT_FU_RESOURCE_H__
#define __SCRIPT_FU_RESOURCE_H__


/* ScriptFu stores Resources as int IDs. */
typedef guint32 SFResourceType;


/* Methods on SFResourceType. */
void          sf_resource_set_default (SFResourceType *arg_value,
                                       gchar          *name_of_default);
GimpResource* sf_resource_get_default (SFResourceType *arg_value);

gchar*        sf_resource_get_name_of_default (SFResourceType *arg);

#endif /*  __SCRIPT_FU_RESOURCE__  */
