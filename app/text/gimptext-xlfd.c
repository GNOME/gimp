/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "gimptext-xlfd.h"


/*  Most of this code was copied from pangox-fontmap.c  */


#define XLFD_MAX_FIELD_LEN 64

/* These are the field numbers in the X Logical Font Description fontnames,
   e.g. -adobe-courier-bold-o-normal--25-180-100-100-m-150-iso8859-1 */
enum
{
  XLFD_FOUNDRY		= 0,
  XLFD_FAMILY		= 1,
  XLFD_WEIGHT		= 2,
  XLFD_SLANT		= 3,
  XLFD_SET_WIDTH	= 4,
  XLFD_ADD_STYLE	= 5,
  XLFD_PIXELS		= 6,
  XLFD_POINTS		= 7,
  XLFD_RESOLUTION_X	= 8,
  XLFD_RESOLUTION_Y	= 9,
  XLFD_SPACING		= 10,
  XLFD_AVERAGE_WIDTH	= 11,
  XLFD_CHARSET		= 12,
  XLFD_NUM_FIELDS     
};


/** gimp_text_get_xlfd_field()
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
 * %NULL or the field is longer than XFLD_MAX_FIELD_LEN.
 **/
static gchar *
gimp_text_get_xlfd_field (const gchar *fontname,
                          gint         field_num,
                          gchar       *buffer)
{
  const gchar *t1, *t2;
  gchar       *p;
  gint         countdown, len, num_dashes;

  if (!fontname)
    return NULL;
  
  /* we assume this is a valid fontname...that is, it has 14 fields */

  countdown = field_num;
  t1 = fontname;
  while (*t1 && (countdown >= 0))
    if (*t1++ == '-')
      countdown--;
  
  num_dashes = (field_num == XLFD_CHARSET) ? 2 : 1;
  for (t2 = t1; *t2; t2++)
    { 
      if (*t2 == '-' && --num_dashes == 0)
	break;
    }
  
  if (t1 != t2)
    {
      /* Check we don't overflow the buffer */
      len = (long) t2 - (long) t1;
      if (len > XLFD_MAX_FIELD_LEN - 1)
	return NULL;
      strncpy (buffer, t1, len);
      buffer[len] = 0;
      /* Convert to lower case. */
      for (p = buffer; *p; p++)
	*p = g_ascii_tolower (*p);
    }
  else
    strcpy(buffer, "(nil)");
  
  return buffer;
}

gchar *
gimp_text_font_name_from_xlfd (const gchar *xlfd)
{
  gchar *fields[4];
  gchar  buffers[4][XLFD_MAX_FIELD_LEN];
  gint   i, j;

  for (i = 0, j = 0; i < 4; i++)
    {
      fields[j] = gimp_text_get_xlfd_field (xlfd, XLFD_FAMILY + i, buffers[i]);

      if (fields[j] && *fields[j] == '*')
        fields[j] = NULL;
      
      if (fields[j])
        j++;
    }
  
  return g_strconcat (fields[0], " ",
                      fields[1], " ",
                      fields[2], " ",
                      fields[3], NULL);
}

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
  if (field && *field != '*')
    {
      *size      = atoi (field);
      *size_unit = GIMP_UNIT_PIXEL;
      return TRUE;
    }

  field = gimp_text_get_xlfd_field (xlfd, XLFD_POINTS, buffer);
  if (field && *field != '*')
    {
      *size      = atoi (field);
      *size_unit = GIMP_UNIT_POINT;
      return TRUE;
    }

  return FALSE;
}
