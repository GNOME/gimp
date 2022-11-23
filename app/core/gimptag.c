/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatag.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "core-types.h"

#include "ligmatag.h"


#define LIGMA_TAG_INTERNAL_PREFIX "ligma:"


G_DEFINE_TYPE (LigmaTag, ligma_tag, G_TYPE_OBJECT)

#define parent_class ligma_tag_parent_class


static void
ligma_tag_class_init (LigmaTagClass *klass)
{
}

static void
ligma_tag_init (LigmaTag *tag)
{
  tag->tag         = 0;
  tag->collate_key = 0;
  tag->internal    = FALSE;
}

/**
 * ligma_tag_new:
 * @tag_string: a tag name.
 *
 * If given tag name is not valid, an attempt will be made to fix it.
 *
 * Returns: (nullable) (transfer full): a new #LigmaTag object,
 *          or %NULL if tag string is invalid and cannot be fixed.
 **/
LigmaTag *
ligma_tag_new (const char *tag_string)
{
  LigmaTag *tag;
  gchar   *tag_name;
  gchar   *case_folded;
  gchar   *collate_key;

  g_return_val_if_fail (tag_string != NULL, NULL);

  tag_name = ligma_tag_string_make_valid (tag_string);
  if (! tag_name)
    return NULL;

  tag = g_object_new (LIGMA_TYPE_TAG, NULL);

  tag->tag = g_quark_from_string (tag_name);

  case_folded = g_utf8_casefold (tag_name, -1);
  collate_key = g_utf8_collate_key (case_folded, -1);
  tag->collate_key = g_quark_from_string (collate_key);
  g_free (collate_key);
  g_free (case_folded);
  g_free (tag_name);

  return tag;
}

/**
 * ligma_tag_try_new:
 * @tag_string: a tag name.
 *
 * Similar to ligma_tag_new(), but returns %NULL if tag is surely not equal
 * to any of currently created tags. It is useful for tag querying to avoid
 * unneeded comparisons. If tag is created, however, it does not mean that
 * it would necessarily match with some other tag.
 *
 * Returns: (nullable) (transfer full): new #LigmaTag object,
 *          or %NULL if tag will not match with any other #LigmaTag.
 **/
LigmaTag *
ligma_tag_try_new (const char *tag_string)
{
  LigmaTag *tag;
  gchar   *tag_name;
  gchar   *case_folded;
  gchar   *collate_key;
  GQuark   tag_quark;
  GQuark   collate_key_quark;

  tag_name = ligma_tag_string_make_valid (tag_string);
  if (! tag_name)
    return NULL;

  case_folded = g_utf8_casefold (tag_name, -1);
  collate_key = g_utf8_collate_key (case_folded, -1);
  collate_key_quark = g_quark_try_string (collate_key);
  g_free (collate_key);
  g_free (case_folded);

  if (! collate_key_quark)
    {
      g_free (tag_name);
      return NULL;
    }

  tag_quark = g_quark_from_string (tag_name);
  g_free (tag_name);
  if (! tag_quark)
    return NULL;

  tag = g_object_new (LIGMA_TYPE_TAG, NULL);
  tag->tag = tag_quark;
  tag->collate_key = collate_key_quark;

  return tag;
}

/**
 * ligma_tag_get_internal:
 * @tag: a ligma tag.
 *
 * Retrieve internal status of the tag.
 *
 * Returns: internal status of tag. Internal tags are not saved.
 **/
gboolean
ligma_tag_get_internal (LigmaTag *tag)
{
  g_return_val_if_fail (LIGMA_IS_TAG (tag), FALSE);

  return tag->internal;
}

/**
 * ligma_tag_set_internal:
 * @tag: a ligma tag.
 * @internal: desired tag internal status
 *
 * Set internal status of the tag. Internal tags are usually automatically
 * generated and will not be saved into users tag cache.
 *
 **/
void
ligma_tag_set_internal (LigmaTag *tag, gboolean internal)
{
  g_return_if_fail (LIGMA_IS_TAG (tag));

  tag->internal = internal;
}


/**
 * ligma_tag_get_name:
 * @tag: a ligma tag.
 *
 * Retrieve name of the tag.
 *
 * Returns: name of tag.
 **/
const gchar *
ligma_tag_get_name (LigmaTag *tag)
{
  g_return_val_if_fail (LIGMA_IS_TAG (tag), NULL);

  return g_quark_to_string (tag->tag);
}

/**
 * ligma_tag_get_hash:
 * @tag: a ligma tag.
 *
 * Hashing function which is useful, for example, to store #LigmaTag in
 * a #GHashTable.
 *
 * Returns: hash value for tag.
 **/
guint
ligma_tag_get_hash (LigmaTag *tag)
{
  g_return_val_if_fail (LIGMA_IS_TAG (tag), -1);

  return tag->collate_key;
}

/**
 * ligma_tag_equals:
 * @tag:   a ligma tag.
 * @other: another ligma tag to compare with.
 *
 * Compares tags for equality according to tag comparison rules.
 *
 * Returns: TRUE if tags are equal, FALSE otherwise.
 **/
gboolean
ligma_tag_equals (LigmaTag *tag,
                 LigmaTag *other)
{
  g_return_val_if_fail (LIGMA_IS_TAG (tag), FALSE);
  g_return_val_if_fail (LIGMA_IS_TAG (other), FALSE);

  return tag->collate_key == other->collate_key;
}

/**
 * ligma_tag_compare_func:
 * @p1: pointer to left-hand #LigmaTag object.
 * @p2: pointer to right-hand #LigmaTag object.
 *
 * Compares tags according to tag comparison rules. Useful for sorting
 * functions.
 *
 * Returns: meaning of return value is the same as in strcmp().
 **/
int
ligma_tag_compare_func (const void *p1,
                       const void *p2)
{
  LigmaTag      *t1 = LIGMA_TAG (p1);
  LigmaTag      *t2 = LIGMA_TAG (p2);

  return g_strcmp0 (g_quark_to_string (t1->collate_key),
                    g_quark_to_string (t2->collate_key));
}

/**
 * ligma_tag_compare_with_string:
 * @tag:        a #LigmaTag object.
 * @tag_string: the string to compare to.
 *
 * Compares tag and a string according to tag comparison rules. Similar to
 * ligma_tag_compare_func(), but can be used without creating temporary tag
 * object.
 *
 * Returns: meaning of return value is the same as in strcmp().
 **/
gint
ligma_tag_compare_with_string (LigmaTag     *tag,
                              const gchar *tag_string)
{
  gchar        *case_folded;
  const gchar  *collate_key;
  gchar        *collate_key2;
  gint          result;

  g_return_val_if_fail (LIGMA_IS_TAG (tag), 0);
  g_return_val_if_fail (tag_string != NULL, 0);

  collate_key = g_quark_to_string (tag->collate_key);
  case_folded = g_utf8_casefold (tag_string, -1);
  collate_key2 = g_utf8_collate_key (case_folded, -1);
  result = g_strcmp0 (collate_key, collate_key2);
  g_free (collate_key2);
  g_free (case_folded);

  return result;
}

/**
 * ligma_tag_has_prefix:
 * @tag:           a #LigmaTag object.
 * @prefix_string: the prefix to compare to.
 *
 * Compares tag and a prefix according to tag comparison rules. Similar to
 * ligma_tag_compare_with_string(), but does not work on the collate key
 * because that can't be matched partially.
 *
 * Returns: wheher #tag starts with @prefix_string.
 **/
gboolean
ligma_tag_has_prefix (LigmaTag     *tag,
                     const gchar *prefix_string)
{
  gchar    *case_folded1;
  gchar    *case_folded2;
  gboolean  has_prefix;

  g_return_val_if_fail (LIGMA_IS_TAG (tag), FALSE);
  g_return_val_if_fail (prefix_string != NULL, FALSE);

  case_folded1 = g_utf8_casefold (g_quark_to_string (tag->tag), -1);
  case_folded2 = g_utf8_casefold (prefix_string, -1);

  has_prefix = g_str_has_prefix (case_folded1, case_folded2);

  g_free (case_folded1);
  g_free (case_folded2);

  g_printerr ("'%s' has prefix '%s': %d\n",
              g_quark_to_string (tag->tag), prefix_string, has_prefix);

  return has_prefix;
}

/**
 * ligma_tag_string_make_valid:
 * @tag_string: a text string.
 *
 * Tries to create a valid tag string from given @tag_string.
 *
 * Returns: (transfer full) (nullable): a newly allocated tag string in case
 * given @tag_string was valid or could be fixed, otherwise %NULL. Allocated
 * value should be freed using g_free().
 **/
gchar *
ligma_tag_string_make_valid (const gchar *tag_string)
{
  gchar    *tag;
  GString  *buffer;
  gchar    *tag_cursor;
  gunichar  c;

  g_return_val_if_fail (tag_string, NULL);

  tag = g_utf8_normalize (tag_string, -1, G_NORMALIZE_ALL);
  if (! tag)
    return NULL;

  tag = g_strstrip (tag);
  if (! *tag)
    {
      g_free (tag);
      return NULL;
    }

  buffer = g_string_new ("");
  tag_cursor = tag;
  if (g_str_has_prefix (tag_cursor, LIGMA_TAG_INTERNAL_PREFIX))
    {
      tag_cursor += strlen (LIGMA_TAG_INTERNAL_PREFIX);
    }
  do
    {
      c = g_utf8_get_char (tag_cursor);
      tag_cursor = g_utf8_next_char (tag_cursor);
      if (g_unichar_isprint (c)
          && ! ligma_tag_is_tag_separator (c))
        {
          g_string_append_unichar (buffer, c);
        }
    } while (c);

  g_free (tag);
  tag = g_string_free (buffer, FALSE);
  tag = g_strstrip (tag);

  if (! *tag)
    {
      g_free (tag);
      return NULL;
    }

  return tag;
}

/**
 * ligma_tag_is_tag_separator:
 * @c: Unicode character.
 *
 * Defines a set of characters that are considered tag separators. The
 * tag separators are hand-picked from the set of characters with the
 * Terminal_Punctuation property as specified in the version 5.1.0 of
 * the Unicode Standard.
 *
 * Returns: %TRUE if the character is a tag separator.
 */
gboolean
ligma_tag_is_tag_separator (gunichar c)
{
  switch (c)
    {
    case 0x002C: /* COMMA */
    case 0x060C: /* ARABIC COMMA */
    case 0x07F8: /* NKO COMMA */
    case 0x1363: /* ETHIOPIC COMMA */
    case 0x1802: /* MONGOLIAN COMMA */
    case 0x1808: /* MONGOLIAN MANCHU COMMA */
    case 0x3001: /* IDEOGRAPHIC COMMA */
    case 0xA60D: /* VAI COMMA */
    case 0xFE50: /* SMALL COMMA */
    case 0xFF0C: /* FULLWIDTH COMMA */
    case 0xFF64: /* HALFWIDTH IDEOGRAPHIC COMMA */
      return TRUE;

    default:
      return FALSE;
    }
}

/**
 * ligma_tag_or_null_ref:
 * @tag: a #LigmaTag
 *
 * A simple wrapper around g_object_ref() that silently accepts %NULL.
 **/
void
ligma_tag_or_null_ref (LigmaTag *tag_or_null)
{
  if (tag_or_null)
    {
      g_return_if_fail (LIGMA_IS_TAG (tag_or_null));

      g_object_ref (tag_or_null);
    }
}

/**
 * ligma_tag_or_null_unref:
 * @tag: a #LigmaTag
 *
 * A simple wrapper around g_object_unref() that silently accepts %NULL.
 **/
void
ligma_tag_or_null_unref (LigmaTag *tag_or_null)
{
  if (tag_or_null)
    {
      g_return_if_fail (LIGMA_IS_TAG (tag_or_null));

      g_object_unref (tag_or_null);
    }
}
