/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-babl.c
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "gimp-gegl-types.h"

#include "gimp-babl.h"

#include "gimp-intl.h"


void
gimp_babl_init (void)
{
  babl_format_new ("name", "R' u8",
                   babl_model ("R'G'B'A"),
                   babl_type ("u8"),
                   babl_component ("R'"),
                   NULL);
  babl_format_new ("name", "G' u8",
                   babl_model ("R'G'B'A"),
                   babl_type ("u8"),
                   babl_component ("G'"),
                   NULL);
  babl_format_new ("name", "B' u8",
                   babl_model ("R'G'B'A"),
                   babl_type ("u8"),
                   babl_component ("B'"),
                   NULL);
  babl_format_new ("name", "A u8",
                   babl_model ("R'G'B'A"),
                   babl_type ("u8"),
                   babl_component ("A"),
                   NULL);

  babl_format_new ("name", "A float",
                   babl_model ("R'G'B'A"),
                   babl_type ("float"),
                   babl_component ("A"),
                   NULL);

  babl_format_new ("name", "A double",
                   babl_model ("R'G'B'A"),
                   babl_type ("double"),
                   babl_component ("A"),
                   NULL);
}

static const struct
{
  const gchar *name;
  const gchar *description;
}
babl_descriptions[] =
{
  { "R'G'B' u8",  N_("RGB") },
  { "R'G'B'A u8", N_("RGB-alpha") },
  { "Y' u8",      N_("Grayscale") },
  { "Y'A u8",     N_("Grayscale-alpha") },
  { "R' u8",      N_("Red component") },
  { "G' u8",      N_("Green component") },
  { "B' u8",      N_("Blue component") },
  { "A u8",       N_("Alpha component") },
  { "A float",    N_("Alpha component") },
  { "A double",   N_("Alpha component") }
};

static GHashTable *babl_description_hash = NULL;

const gchar *
gimp_babl_get_description (const Babl *babl)
{
  const gchar *description;

  g_return_val_if_fail (babl != NULL, NULL);

  if (G_UNLIKELY (! babl_description_hash))
    {
      gint i;

      babl_description_hash = g_hash_table_new (g_str_hash,
                                                g_str_equal);

      for (i = 0; i < G_N_ELEMENTS (babl_descriptions); i++)
        g_hash_table_insert (babl_description_hash,
                             (gpointer) babl_descriptions[i].name,
                             gettext (babl_descriptions[i].description));
    }

  if (babl_format_is_palette (babl))
    {
      if (babl_format_has_alpha (babl))
        return _("Indexed-alpha");
      else
        return _("Indexed");
    }

  description = g_hash_table_lookup (babl_description_hash,
                                     babl_get_name (babl));

  if (description)
    return description;

  return g_strconcat ("ERROR: unknown Babl format ",
                      babl_get_name (babl), NULL);
}

GimpImageBaseType
gimp_babl_format_get_base_type (const Babl *format)
{
  g_return_val_if_fail (format != NULL, -1);

  if (format == babl_format ("Y u8")  ||
      format == babl_format ("Y' u8") ||
      format == babl_format ("Y'A u8"))
    {
      return GIMP_GRAY;
    }
  else if (format == babl_format ("R'G'B' u8") ||
           format == babl_format ("R'G'B'A u8"))
    {
      return GIMP_RGB;
    }
  else if (babl_format_is_palette (format))
    {
      return GIMP_INDEXED;
    }

  g_return_val_if_reached (-1);
}

GimpPrecision
gimp_babl_format_get_precision (const Babl *format)
{
  const Babl *type;

  g_return_val_if_fail (format != NULL, -1);

  type = babl_format_get_type (format, 0);

  if (type == babl_type ("u8"))
    return GIMP_PRECISION_U8;

  g_return_val_if_reached (-1);
}

const Babl *
gimp_babl_format (GimpImageBaseType  base_type,
                  GimpPrecision      precision,
                  gboolean           with_alpha)
{
  switch (base_type)
    {
    case GIMP_RGB:
      switch (precision)
        {
        case GIMP_PRECISION_U8:
          if (with_alpha)
            return babl_format ("R'G'B'A u8");
          else
            return babl_format ("R'G'B' u8");

        default:
          break;
        }
      break;

    case GIMP_GRAY:
      switch (precision)
        {
        case GIMP_PRECISION_U8:
          if (with_alpha)
            return babl_format ("Y'A u8");
          else
            return babl_format ("Y' u8");

        default:
          break;
        }
      break;

    case GIMP_INDEXED:
      /* need to use the image's api for this */
      break;
    }

  g_return_val_if_reached (NULL);
}
