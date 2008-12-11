/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptag.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include "core-types.h"

#include "gimptag.h"


G_DEFINE_TYPE (GimpTag, gimp_tag, G_TYPE_OBJECT)

#define parent_class gimp_tag_parent_class


static void
gimp_tag_class_init (GimpTagClass *klass)
{
}

static void
gimp_tag_init (GimpTag *tag)
{
}

/**
 * gimp_tag_equals:
 * @tag:   a gimp tag.
 * @other: another gimp tag to compare with.
 *
 * Compares tags for equality according to tag comparison rules.
 *
 * Return value: TRUE if tags are equal, FALSE otherwise.
 **/
gboolean
gimp_tag_equals (const GimpTag *tag,
                 const GimpTag *other)
{
  g_return_val_if_fail (GIMP_IS_TAG (tag), FALSE);
  g_return_val_if_fail (GIMP_IS_TAG (other), FALSE);

  return FALSE;
}
