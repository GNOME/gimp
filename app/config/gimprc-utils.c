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

#include "config.h"

#include <glib-object.h>

#include "gimprc-utils.h"


gchar *
gimp_rc_build_data_path (const gchar *name)
{
  return g_strconcat ("${gimp_dir}", G_DIR_SEPARATOR_S, name,
                      G_SEARCHPATH_SEPARATOR_S,
                      "${gimp_data_dir}", G_DIR_SEPARATOR_S, name,
                      NULL);
}

gchar *
gimp_rc_build_writable_path (const gchar *name)
{
  return g_strconcat ("${gimp_dir}", G_DIR_SEPARATOR_S, name, NULL);
}

gchar *
gimp_rc_build_plug_in_path (const gchar *name)
{
  return g_strconcat ("${gimp_dir}", G_DIR_SEPARATOR_S, name,
                      G_SEARCHPATH_SEPARATOR_S,
                      "${gimp_plug_in_dir}", G_DIR_SEPARATOR_S, name,
                      NULL);
}
