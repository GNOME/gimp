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

G_DEFINE_TYPE (GimpTag, gimp_tag, GIMP_TYPE_OBJECT)

#define parent_class gimp_tag_parent_class

static void
gimp_tag_class_init (GimpTagClass      *klass)
{
}

static void
gimp_tag_init (GimpTag         *tag)
{
  tag->tag              = 0;
  tag->collate_key      = 0;
}

GimpTag *
gimp_tag_new (const char *tag_string)
{
  GimpTag      *tag;
  gchar        *case_folded;
  gchar        *collate_key;

  g_return_val_if_fail (tag_string != NULL, NULL);
  g_return_val_if_fail (gimp_tag_string_is_valid (tag_string), NULL);

  tag = g_object_new (GIMP_TYPE_TAG, NULL);

  tag->tag = g_quark_from_string (tag_string);

  case_folded = g_utf8_casefold (tag_string, -1);
  collate_key = g_utf8_collate_key (case_folded, -1);
  tag->collate_key = g_quark_from_string (collate_key);
  g_free (collate_key);
  g_free (case_folded);

  return tag;
}

GimpTag *
gimp_tag_try_new (const char *tag_string)
{
  GimpTag      *tag;
  gchar        *case_folded;
  gchar        *collate_key;
  GQuark        tag_quark;
  GQuark        collate_key_quark;

  if (! tag_string
      || ! gimp_tag_string_is_valid (tag_string))
    {
     return NULL;
    }


  tag_quark = g_quark_from_string (tag_string);
  if (! tag_quark)
    {
      return NULL;
    }

  case_folded = g_utf8_casefold (tag_string, -1);
  collate_key = g_utf8_collate_key (case_folded, -1);
  collate_key_quark = g_quark_from_string (collate_key);
  g_free (collate_key);
  g_free (case_folded);

  if (! collate_key_quark)
    {
      return NULL;
    }

  tag = g_object_new (GIMP_TYPE_TAG, NULL);
  tag->tag = tag_quark;
  tag->collate_key = collate_key_quark;
  return tag;
}

const gchar *
gimp_tag_get_name (GimpTag           *tag)
{
  g_return_val_if_fail (GIMP_IS_TAG (tag), NULL);

  return g_quark_to_string (tag->tag);
}

guint
gimp_tag_get_hash (GimpTag       *tag)
{
  g_return_val_if_fail (GIMP_IS_TAG (tag), -1);

  return tag->collate_key;
}

gboolean
gimp_tag_equals (GimpTag             *tag,
                 GimpTag             *other)
{
  g_return_val_if_fail (GIMP_IS_TAG (tag), FALSE);
  g_return_val_if_fail (GIMP_IS_TAG (tag), FALSE);

  return tag->tag == other->tag;
}

int
gimp_tag_compare_func (const void         *p1,
                       const void         *p2)
{
  GimpTag      *t1 = GIMP_TAG (p1);
  GimpTag      *t2 = GIMP_TAG (p2);

  return g_strcmp0 (g_quark_to_string (t1->collate_key),
                    g_quark_to_string (t2->collate_key));
}

gint
gimp_tag_compare_with_string (GimpTag          *tag,
                              const char       *tag_string)
{
  gchar        *case_folded;
  const gchar  *collate_key;
  gchar        *collate_key2;
  gint          result;

  g_return_val_if_fail (GIMP_IS_TAG (tag), 0);
  g_return_val_if_fail (tag_string != NULL, 0);
  g_return_val_if_fail (! gimp_tag_string_is_valid (tag_string), 0);

  collate_key = g_quark_to_string (tag->collate_key);
  case_folded = g_utf8_casefold (tag_string, -1);
  collate_key2 = g_utf8_collate_key (case_folded, -1);
  result = g_strcmp0 (collate_key, collate_key2);
  g_free (collate_key2);
  g_free (case_folded);

  return result;
}

gboolean
gimp_tag_string_is_valid (const gchar      *string)
{
  g_return_val_if_fail (string, FALSE);

  if (g_utf8_strchr (string, -1, ','))
    {
      return FALSE;
    }

  return TRUE;
}

