/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2004  Sven Neumann <sven@gimp.org>
 *
 * Some of this code was copied from Pango (pangox-fontmap.c)
 * and was originally written by Owen Taylor <otaylor@redhat.com>.
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

#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "text-types.h"

#include "gimptext.h"
#include "gimptext-xlfd.h"


#define XLFD_MAX_FIELD_LEN 64

/* These are the field numbers in the X Logical Font Description fontnames,
   e.g. -adobe-courier-bold-o-normal--25-180-100-100-m-150-iso8859-1 */
enum
{
  XLFD_FOUNDRY       = 0,
  XLFD_FAMILY        = 1,
  XLFD_WEIGHT        = 2,
  XLFD_SLANT         = 3,
  XLFD_SET_WIDTH     = 4,
  XLFD_ADD_STYLE     = 5,
  XLFD_PIXELS        = 6,
  XLFD_POINTS        = 7,
  XLFD_RESOLUTION_X  = 8,
  XLFD_RESOLUTION_Y  = 9,
  XLFD_SPACING       = 10,
  XLFD_AVERAGE_WIDTH = 11,
  XLFD_CHARSET       = 12,
  XLFD_NUM_FIELDS
};

static gchar * gimp_text_get_xlfd_field (const gchar *fontname,
                                         gint         field_num,
                                         gchar       *buffer);
static gchar * launder_font_name        (gchar       *name);


/**
 * gimp_text_font_name_from_xlfd:
 * @xlfd: X Logical Font Description
 *
 * Attempts to extract a meaningful font name from the "family",
 * "weight", "slant" and "stretch" fields of an X Logical Font
 * Description.
 *
 * Return value: a newly allocated string.
 **/
gchar *
gimp_text_font_name_from_xlfd (const gchar *xlfd)
{
  gchar *fields[4];
  gchar  buffers[4][XLFD_MAX_FIELD_LEN];
  gint   i = 0;

  /*  family  */
  fields[i] = gimp_text_get_xlfd_field (xlfd, XLFD_FAMILY, buffers[i]);
  if (fields[i])
    i++;

  /*  weight  */
  fields[i] = gimp_text_get_xlfd_field (xlfd, XLFD_WEIGHT, buffers[i]);
  if (fields[i] && strcmp (fields[i], "medium"))
    i++;

  /*  slant  */
  fields[i] = gimp_text_get_xlfd_field (xlfd, XLFD_SLANT, buffers[i]);
  if (fields[i])
    {
      switch (*fields[i])
        {
        case 'i':
          strcpy (buffers[i], "italic");
          i++;
          break;
        case 'o':
          strcpy (buffers[i], "oblique");
          i++;
          break;
        case 'r':
          break;
        }
    }

  /*  stretch  */
  fields[i] = gimp_text_get_xlfd_field (xlfd, XLFD_SET_WIDTH, buffers[i]);
  if (fields[i] && strcmp (fields[i], "normal"))
    i++;

  if (i < 4)
    fields[i] = NULL;

  return launder_font_name (g_strconcat (fields[0], " ",
                                         fields[1], " ",
                                         fields[2], " ",
                                         fields[3], NULL));
}

/**
 * gimp_text_font_size_from_xlfd:
 * @xlfd: X Logical Font Description
 * @size: return location for the font size
 * @size_unit: return location for the font size unit
 *
 * Attempts to extract the font size from an X Logical Font
 * Description.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 **/
gboolean
gimp_text_font_size_from_xlfd (const gchar *xlfd,
                               gdouble     *size,
                               GimpUnit    *size_unit)
{
  gchar  buffer[XLFD_MAX_FIELD_LEN];
  gchar *field;

  if (!xlfd)
    return FALSE;

  field = gimp_text_get_xlfd_field (xlfd, XLFD_PIXELS, buffer);
  if (field)
    {
      *size      = atoi (field);
      *size_unit = GIMP_UNIT_PIXEL;
      return TRUE;
    }

  field = gimp_text_get_xlfd_field (xlfd, XLFD_POINTS, buffer);
  if (field)
    {
      *size      = atoi (field) / 10.0;
      *size_unit = GIMP_UNIT_POINT;
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_text_set_font_from_xlfd:
 * @text: a #GimpText object
 * @xlfd: X Logical Font Description
 *
 * Attempts to extract font name and font size from @xlfd and sets
 * them on the #GimpText object.
 **/
void
gimp_text_set_font_from_xlfd (GimpText    *text,
                              const gchar *xlfd)
{
  gchar    *font;
  gdouble   size;
  GimpUnit  size_unit;

  g_return_if_fail (GIMP_IS_TEXT (text));

  if (!xlfd)
    return;

  font = gimp_text_font_name_from_xlfd (xlfd);

#if GIMP_TEXT_DEBUG
  g_printerr ("XLFD: %s  font: %s\n", xlfd, font ? font : "(null)");
#endif

  if (gimp_text_font_size_from_xlfd (xlfd, &size, &size_unit))
    {
      g_object_set (text,
                    "font-size",          size,
                    "font-size-unit",     size_unit,
                    font ? "font" : NULL, font,
                    NULL);
    }
  else if (font)
    {
      g_object_set (text,
                    "font", font,
                    NULL);
    }

  g_free (font);
}

/**
 * gimp_text_get_xlfd_field:
 * @fontname: an XLFD fontname
 * @field_num: field index
 * @buffer: buffer of at least XLFD_MAX_FIELD_LEN chars
 *
 * Fills the buffer with the specified field from the X Logical Font
 * Description name, and returns it. Note: For the charset field, we
 * also return the encoding, e.g. 'iso8859-1'.
 *
 * This function is basically copied from pangox-fontmap.c.
 *
 * Returns: a pointer to the filled buffer or %NULL if fontname is
 * %NULL, the field is longer than XFLD_MAX_FIELD_LEN or it contains
 * just an asterisk.
 **/
static gchar *
gimp_text_get_xlfd_field (const gchar *fontname,
                          gint         field_num,
                          gchar       *buffer)
{
  const gchar *t1, *t2;
  gchar       *p;
  gint         countdown, num_dashes;
  gsize        len;

  if (!fontname)
    return NULL;

  /* we assume this is a valid fontname...that is, it has 14 fields */

  for (t1 = fontname, countdown = field_num; *t1 && (countdown >= 0); t1++)
    if (*t1 == '-')
      countdown--;

  num_dashes = (field_num == XLFD_CHARSET) ? 2 : 1;

  for (t2 = t1; *t2; t2++)
    {
      if (*t2 == '-' && --num_dashes == 0)
        break;
    }

  if (t2 > t1)
    {
      /* Check we don't overflow the buffer */
      len = (gsize) t2 - (gsize) t1;
      if (len > XLFD_MAX_FIELD_LEN - 1)
        return NULL;

      if (*t1 == '*')
        return NULL;

      strncpy (buffer, t1, len);
      buffer[len] = 0;

      /* Convert to lower case. */
      for (p = buffer; *p; p++)
        *p = g_ascii_tolower (*p);
    }
  else
    {
      return NULL;
    }

  return buffer;
}

/* Guard against font names that end in numbers being interpreted as a
 * font size in pango font descriptions
 */
static gchar *
launder_font_name (gchar *name)
{
  gchar *laundered_name;
  gchar  last_char;

  last_char = name[strlen (name) - 1];

  if (g_ascii_isdigit (last_char) || last_char == '.')
    {
      laundered_name = g_strconcat (name, ",", NULL);
      g_free (name);

      return laundered_name;
    }
  else
    return name;
}
