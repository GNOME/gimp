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
#include <string.h>

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

/**
 * gimp_tag_new:
 * @tag_string: a tag name.
 *
 * Given tag name does not need to be valid. If it is not valid, an attempt
 * will be made to fix it.
 *
 * Return value: a new #GimpTag object, or NULL if tag string is invalid and
 * cannot be fixed.
 **/
GimpTag *
gimp_tag_new (const char *tag_string)
{
  GimpTag      *tag;
  gchar        *tag_name;
  gchar        *case_folded;
  gchar        *collate_key;

  g_return_val_if_fail (tag_string != NULL, NULL);

  tag_name = gimp_tag_string_make_valid (tag_string);
  if (! tag_name)
    {
      return NULL;
    }

  tag = g_object_new (GIMP_TYPE_TAG, NULL);

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
 * gimp_tag_try_new:
 * @tag_string: a tag name.
 *
 * Similar to gimp_tag_new(), but returns NULL if tag is surely not equal
 * to any of currently created tags. It is useful for tag querying to avoid
 * unneeded comparisons. If tag is created, however, it does not mean that
 * it would necessarily match with some other tag.
 *
 * Return value: new #GimpTag object, or NULL if tag will not match with any
 * other #GimpTag.
 **/
GimpTag *
gimp_tag_try_new (const char *tag_string)
{
  GimpTag      *tag;
  gchar        *tag_name;
  gchar        *case_folded;
  gchar        *collate_key;
  GQuark        tag_quark;
  GQuark        collate_key_quark;

  tag_name = gimp_tag_string_make_valid (tag_string);
  if (! tag_name)
    {
      return NULL;
    }

  tag_quark = g_quark_try_string (tag_name);
  if (! tag_quark)
    {
      g_free (tag_name);
      return NULL;
    }

  case_folded = g_utf8_casefold (tag_name, -1);
  collate_key = g_utf8_collate_key (case_folded, -1);
  collate_key_quark = g_quark_try_string (collate_key);
  g_free (collate_key);
  g_free (case_folded);
  g_free (tag_name);

  if (! collate_key_quark)
    {
      return NULL;
    }

  tag = g_object_new (GIMP_TYPE_TAG, NULL);
  tag->tag = tag_quark;
  tag->collate_key = collate_key_quark;
  return tag;
}

/**
 * gimp_tag_get_name:
 * @tag: a gimp tag.
 *
 * Return value: name of tag.
 **/
const gchar *
gimp_tag_get_name (GimpTag           *tag)
{
  g_return_val_if_fail (GIMP_IS_TAG (tag), NULL);

  return g_quark_to_string (tag->tag);
}

/**
 * gimp_tag_get_hash:
 * @tag: a gimp tag.
 *
 * Hashing function which is useful, for example, to store #GimpTag in
 * a #GHashTable.
 *
 * Return value: hash value for tag.
 **/
guint
gimp_tag_get_hash (GimpTag       *tag)
{
  g_return_val_if_fail (GIMP_IS_TAG (tag), -1);

  return tag->collate_key;
}

/**
 * gimp_tag_equals:
 * @tag:   a gimp tag.
 * @other: an other gimp tag to compare with.
 *
 * Compares tags for equality according to tag comparison rules.
 *
 * Return value: TRUE if tags are equal, FALSE otherwise.
 **/
gboolean
gimp_tag_equals (GimpTag             *tag,
                 GimpTag             *other)
{
  g_return_val_if_fail (GIMP_IS_TAG (tag), FALSE);
  g_return_val_if_fail (GIMP_IS_TAG (other), FALSE);

  return tag->tag == other->tag;
}

/**
 * gimp_tag_compare_func:
 * @p1: pointer to left-hand #GimpTag object.
 * @p2: pointer to right-hand #GimpTag object.
 *
 * Compares tags according to tag comparison rules. Useful for sorting
 * functions.
 *
 * Return value: meaning of return value is the same as in strcmp().
 **/
int
gimp_tag_compare_func (const void         *p1,
                       const void         *p2)
{
  GimpTag      *t1 = GIMP_TAG (p1);
  GimpTag      *t2 = GIMP_TAG (p2);

  return g_strcmp0 (g_quark_to_string (t1->collate_key),
                    g_quark_to_string (t2->collate_key));
}

/**
 * gimp_tag_compare_with_string:
 * @tag:        a #GimpTag object.
 * @tag_string: pointer to right-hand #GimpTag object.
 *
 * Compares tag and a string according to tag comparison rules. Similar to
 * gimp_tag_compare_func(), but can be used without creating temporary tag
 * object.
 *
 * Return value: meaning of return value is the same as in strcmp().
 **/
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

  collate_key = g_quark_to_string (tag->collate_key);
  case_folded = g_utf8_casefold (tag_string, -1);
  collate_key2 = g_utf8_collate_key (case_folded, -1);
  result = g_strcmp0 (collate_key, collate_key2);
  g_free (collate_key2);
  g_free (case_folded);

  return result;
}

gboolean
g_unichar_is_sentence_terminal (gunichar c)
{
  switch (c)
    {
      /*
       * sentence terminal
       */

      case 0x0021: /* (!) Po EXCLAMATION MARK */
      case 0x002E: /* (.) Po FULL STOP */
      case 0x003F: /* (?) Po QUESTION MARK */
      case 0x0589: /* (։) Po ARMENIAN FULL STOP */
      case 0x061F: /* (؟) Po ARABIC QUESTION MARK */
      case 0x06D4: /* (۔) Po ARABIC FULL STOP */
      case 0x0700: /* (܀) Po SYRIAC END OF PARAGRAPH */
      case 0x0701: /* (܁) Po SYRIAC SUPRALINEAR FULL STOP */
      case 0x0702: /* (܂) Po SYRIAC SUBLINEAR FULL STOP */
      case 0x0964: /* (।) Po DEVANAGARI DANDA */
      case 0x104A: /* (၊) Po MYANMAR SIGN LITTLE SECTION */
      case 0x104B: /* (။) Po MYANMAR SIGN SECTION */
      case 0x1362: /* (።) Po ETHIOPIC FULL STOP */
      case 0x1367: /* (፧) Po ETHIOPIC QUESTION MARK */
      case 0x1368: /* (፨) Po ETHIOPIC PARAGRAPH SEPARATOR */
      case 0x166E: /* (᙮) Po CANADIAN SYLLABICS FULL STOP */
      case 0x1803: /* (᠃) Po MONGOLIAN FULL STOP */
      case 0x1809: /* (᠉) Po MONGOLIAN MANCHU FULL STOP */
      case 0x203C: /* (‼) Po DOUBLE EXCLAMATION MARK */
      case 0x203D: /* (‽) Po INTERROBANG */
      case 0x2047: /* (⁇) Po DOUBLE QUESTION MARK */
      case 0x2048: /* (⁈) Po QUESTION EXCLAMATION MARK */
      case 0x2049: /* (⁉) Po EXCLAMATION QUESTION MARK */
      case 0x3002: /* (。) Po IDEOGRAPHIC FULL STOP */
      case 0xFE52: /* (﹒) Po SMALL FULL STOP */
      case 0xFE57: /* (﹗) Po SMALL EXCLAMATION MARK */
      case 0xFF01: /* (！) Po FULLWIDTH EXCLAMATION MARK */
      case 0xFF0E: /* (．) Po FULLWIDTH FULL STOP */
      case 0xFF1F: /* (？) Po FULLWIDTH QUESTION MARK */
      case 0xFF61: /* (｡) Po HALFWIDTH IDEOGRAPHIC FULL STOP */
          return TRUE;

      default:
          return FALSE;
    }
}

gboolean
g_unichar_is_terminal_punctuation (gunichar    c)
{
  switch (c)
    {
      /*
       * sentence terminal
       */

      case 0x0021: /* (!) Po EXCLAMATION MARK */
      case 0x002E: /* (.) Po FULL STOP */
      case 0x003F: /* (?) Po QUESTION MARK */
      case 0x0589: /* (։) Po ARMENIAN FULL STOP */
      case 0x061F: /* (؟) Po ARABIC QUESTION MARK */
      case 0x06D4: /* (۔) Po ARABIC FULL STOP */
      case 0x0700: /* (܀) Po SYRIAC END OF PARAGRAPH */
      case 0x0701: /* (܁) Po SYRIAC SUPRALINEAR FULL STOP */
      case 0x0702: /* (܂) Po SYRIAC SUBLINEAR FULL STOP */
      case 0x0964: /* (।) Po DEVANAGARI DANDA */
      case 0x104A: /* (၊) Po MYANMAR SIGN LITTLE SECTION */
      case 0x104B: /* (။) Po MYANMAR SIGN SECTION */
      case 0x1362: /* (።) Po ETHIOPIC FULL STOP */
      case 0x1367: /* (፧) Po ETHIOPIC QUESTION MARK */
      case 0x1368: /* (፨) Po ETHIOPIC PARAGRAPH SEPARATOR */
      case 0x166E: /* (᙮) Po CANADIAN SYLLABICS FULL STOP */
      case 0x1803: /* (᠃) Po MONGOLIAN FULL STOP */
      case 0x1809: /* (᠉) Po MONGOLIAN MANCHU FULL STOP */
      case 0x203C: /* (‼) Po DOUBLE EXCLAMATION MARK */
      case 0x203D: /* (‽) Po INTERROBANG */
      case 0x2047: /* (⁇) Po DOUBLE QUESTION MARK */
      case 0x2048: /* (⁈) Po QUESTION EXCLAMATION MARK */
      case 0x2049: /* (⁉) Po EXCLAMATION QUESTION MARK */
      case 0x3002: /* (。) Po IDEOGRAPHIC FULL STOP */
      case 0xFE52: /* (﹒) Po SMALL FULL STOP */
      case 0xFE57: /* (﹗) Po SMALL EXCLAMATION MARK */
      case 0xFF01: /* (！) Po FULLWIDTH EXCLAMATION MARK */
      case 0xFF0E: /* (．) Po FULLWIDTH FULL STOP */
      case 0xFF1F: /* (？) Po FULLWIDTH QUESTION MARK */
      case 0xFF61: /* (｡) Po HALFWIDTH IDEOGRAPHIC FULL STOP */

          /*
           *   B. Terminal_Punctuation but not in Sentence_Terminal:
           */

      case 0x002C: /* (,) Po COMMA */
      case 0x003A: /* (:) Po COLON */
      case 0x003B: /* (;) Po SEMICOLON */
      case 0x037E: /* (;) Po GREEK QUESTION MARK */
      case 0x0387: /* (·) Po GREEK ANO TELEIA */
      case 0x060C: /* (،) Po ARABIC COMMA */
      case 0x061B: /* (؛) Po ARABIC SEMICOLON */
      case 0x0703: /* (܃) Po SYRIAC SUPRALINEAR COLON */
      case 0x0704: /* (܄) Po SYRIAC SUBLINEAR COLON */
      case 0x0705: /* (܅) Po SYRIAC HORIZONTAL COLON */
      case 0x0706: /* (܆) Po SYRIAC COLON SKEWED LEFT */
      case 0x0707: /* (܇) Po SYRIAC COLON SKEWED RIGHT */
      case 0x0708: /* (܈) Po SYRIAC SUPRALINEAR COLON SKEWED LEFT */
      case 0x0709: /* (܉) Po SYRIAC SUBLINEAR COLON SKEWED RIGHT */
      case 0x070A: /* (܊) Po SYRIAC CONTRACTION */
      case 0x070C: /* (܌) Po SYRIAC HARKLEAN METOBELUS */
      case 0x0965: /* (॥) Po DEVANAGARI DOUBLE DANDA */
      case 0x0E5A: /* (๚) Po THAI CHARACTER ANGKHANKHU */
      case 0x0E5B: /* (๛) Po THAI CHARACTER KHOMUT */
      case 0x1361: /* (፡) Po ETHIOPIC WORDSPACE */
      case 0x1363: /* (፣) Po ETHIOPIC COMMA */
      case 0x1364: /* (፤) Po ETHIOPIC SEMICOLON */
      case 0x1365: /* (፥) Po ETHIOPIC COLON */
      case 0x1366: /* (፦) Po ETHIOPIC PREFACE COLON */
      case 0x166D: /* (᙭) Po CANADIAN SYLLABICS CHI SIGN */
      case 0x16EB: /* (᛫) Po RUNIC SINGLE PUNCTUATION */
      case 0x16EC: /* (᛬) Po RUNIC MULTIPLE PUNCTUATION */
      case 0x16ED: /* (᛭) Po RUNIC CROSS PUNCTUATION */
      case 0x17D4: /* (។) Po KHMER SIGN KHAN */
      case 0x17D5: /* (៕) Po KHMER SIGN BARIYOOSAN */
      case 0x17D6: /* (៖) Po KHMER SIGN CAMNUC PII KUUH */
      case 0x17DA: /* (៚) Po KHMER SIGN KOOMUUT */
      case 0x1802: /* (᠂) Po MONGOLIAN COMMA */
      case 0x1804: /* (᠄) Po MONGOLIAN COLON */
      case 0x1805: /* (᠅) Po MONGOLIAN FOUR DOTS */
      case 0x1808: /* (᠈) Po MONGOLIAN MANCHU COMMA */
      case 0x1944: /* (᥄) Po LIMBU EXCLAMATION MARK */
      case 0x1945: /* (᥅) Po LIMBU QUESTION MARK */
      case 0x3001: /* (、) Po IDEOGRAPHIC COMMA */
      case 0xFE50: /* (﹐) Po SMALL COMMA */
      case 0xFE51: /* (﹑) Po SMALL IDEOGRAPHIC COMMA */
      case 0xFE54: /* (﹔) Po SMALL SEMICOLON */
      case 0xFE55: /* (﹕) Po SMALL COLON */
      case 0xFE56: /* (﹖) Po SMALL QUESTION MARK */
      case 0xFF0C: /* (，) Po FULLWIDTH COMMA */
      case 0xFF1A: /* (：) Po FULLWIDTH COLON */
      case 0xFF1B: /* (；) Po FULLWIDTH SEMICOLON */
      case 0xFF64: /* (､) Po HALFWIDTH IDEOGRAPHIC COMMA */
          return TRUE;

      default:
          return FALSE;
    }
}

/**
 * gimp_tag_string_make_valid:
 * @string: a text string.
 *
 * Tries to create a valid tag string from given @string.
 *
 * Return value: a newly allocated tag string in case given @string was valid
 * or could be fixed, otherwise NULL. Allocated value should be freed using
 * g_free().
 **/
gchar *
gimp_tag_string_make_valid (const gchar      *string)
{
  gchar        *tag;
  GString      *tag_string;
  gchar        *tag_cursor;
  gunichar      c;

  g_return_val_if_fail (string, NULL);

  tag = g_strdup (string);
  tag = g_strstrip (tag);
  if (! *tag)
    {
      g_free (tag);
      return NULL;
    }

  tag_string = g_string_new ("");
  tag_cursor = tag;
  if (g_str_has_prefix (tag_cursor, GIMP_TAG_INTERNAL_PREFIX))
    {
      tag_cursor += strlen (GIMP_TAG_INTERNAL_PREFIX);
    }
  do
    {
      c = g_utf8_get_char (tag_cursor);
      tag_cursor = g_utf8_next_char (tag_cursor);
      if (g_unichar_isprint (c)
          && ! g_unichar_is_terminal_punctuation (c))
        {
          g_string_append_unichar (tag_string, c);
        }
    } while (c);

  g_free (tag);
  tag = g_string_free (tag_string, FALSE);
  tag = g_strstrip (tag);

  if (! *tag)
    {
      g_free (tag);
      return NULL;
    }

  return tag;
}

