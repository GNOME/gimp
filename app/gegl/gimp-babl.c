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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimp-gegl-types.h"

#include "gimp-babl.h"

#include "gimp-intl.h"


void
gimp_babl_init (void)
{
  babl_format_new ("name", "R u8",
                   babl_model ("RGBA"),
                   babl_type ("u8"),
                   babl_component ("R"),
                   NULL);
  babl_format_new ("name", "R' u8",
                   babl_model ("R'G'B'A"),
                   babl_type ("u8"),
                   babl_component ("R'"),
                   NULL);
  babl_format_new ("name", "G u8",
                   babl_model ("RGBA"),
                   babl_type ("u8"),
                   babl_component ("G"),
                   NULL);
  babl_format_new ("name", "G' u8",
                   babl_model ("R'G'B'A"),
                   babl_type ("u8"),
                   babl_component ("G'"),
                   NULL);
  babl_format_new ("name", "B u8",
                   babl_model ("RGBA"),
                   babl_type ("u8"),
                   babl_component ("B"),
                   NULL);
  babl_format_new ("name", "B' u8",
                   babl_model ("R'G'B'A"),
                   babl_type ("u8"),
                   babl_component ("B'"),
                   NULL);
  babl_format_new ("name", "A u8",
                   babl_model ("RGBA"),
                   babl_type ("u8"),
                   babl_component ("A"),
                   NULL);

  babl_format_new ("name", "R u16",
                   babl_model ("RGBA"),
                   babl_type ("u16"),
                   babl_component ("R"),
                   NULL);
  babl_format_new ("name", "R' u16",
                   babl_model ("R'G'B'A"),
                   babl_type ("u16"),
                   babl_component ("R'"),
                   NULL);
  babl_format_new ("name", "G u16",
                   babl_model ("RGBA"),
                   babl_type ("u16"),
                   babl_component ("G"),
                   NULL);
  babl_format_new ("name", "G' u16",
                   babl_model ("R'G'B'A"),
                   babl_type ("u16"),
                   babl_component ("G'"),
                   NULL);
  babl_format_new ("name", "B u16",
                   babl_model ("RGBA"),
                   babl_type ("u16"),
                   babl_component ("B"),
                   NULL);
  babl_format_new ("name", "B' u16",
                   babl_model ("R'G'B'A"),
                   babl_type ("u16"),
                   babl_component ("B'"),
                   NULL);
  babl_format_new ("name", "A u16",
                   babl_model ("RGBA"),
                   babl_type ("u16"),
                   babl_component ("A"),
                   NULL);

  babl_format_new ("name", "R u32",
                   babl_model ("RGBA"),
                   babl_type ("u32"),
                   babl_component ("R"),
                   NULL);
  babl_format_new ("name", "R' u32",
                   babl_model ("R'G'B'A"),
                   babl_type ("u32"),
                   babl_component ("R'"),
                   NULL);
  babl_format_new ("name", "G u32",
                   babl_model ("RGBA"),
                   babl_type ("u32"),
                   babl_component ("G"),
                   NULL);
  babl_format_new ("name", "G' u32",
                   babl_model ("R'G'B'A"),
                   babl_type ("u32"),
                   babl_component ("G'"),
                   NULL);
  babl_format_new ("name", "B u32",
                   babl_model ("RGBA"),
                   babl_type ("u32"),
                   babl_component ("B"),
                   NULL);
  babl_format_new ("name", "B' u32",
                   babl_model ("R'G'B'A"),
                   babl_type ("u32"),
                   babl_component ("B'"),
                   NULL);
  babl_format_new ("name", "A u32",
                   babl_model ("RGBA"),
                   babl_type ("u32"),
                   babl_component ("A"),
                   NULL);

  babl_format_new ("name", "R half",
                   babl_model ("RGBA"),
                   babl_type ("half"),
                   babl_component ("R"),
                   NULL);
  babl_format_new ("name", "R' half",
                   babl_model ("R'G'B'A"),
                   babl_type ("half"),
                   babl_component ("R'"),
                   NULL);
  babl_format_new ("name", "G half",
                   babl_model ("RGBA"),
                   babl_type ("half"),
                   babl_component ("G"),
                   NULL);
  babl_format_new ("name", "G' half",
                   babl_model ("R'G'B'A"),
                   babl_type ("half"),
                   babl_component ("G'"),
                   NULL);
  babl_format_new ("name", "B half",
                   babl_model ("RGBA"),
                   babl_type ("half"),
                   babl_component ("B"),
                   NULL);
  babl_format_new ("name", "B' half",
                   babl_model ("R'G'B'A"),
                   babl_type ("half"),
                   babl_component ("B'"),
                   NULL);
  babl_format_new ("name", "A half",
                   babl_model ("RGBA"),
                   babl_type ("half"),
                   babl_component ("A"),
                   NULL);

  babl_format_new ("name", "R float",
                   babl_model ("RGBA"),
                   babl_type ("float"),
                   babl_component ("R"),
                   NULL);
  babl_format_new ("name", "R' float",
                   babl_model ("R'G'B'A"),
                   babl_type ("float"),
                   babl_component ("R'"),
                   NULL);
  babl_format_new ("name", "G float",
                   babl_model ("RGBA"),
                   babl_type ("float"),
                   babl_component ("G"),
                   NULL);
  babl_format_new ("name", "G' float",
                   babl_model ("R'G'B'A"),
                   babl_type ("float"),
                   babl_component ("G'"),
                   NULL);
  babl_format_new ("name", "B float",
                   babl_model ("RGBA"),
                   babl_type ("float"),
                   babl_component ("B"),
                   NULL);
  babl_format_new ("name", "B' float",
                   babl_model ("R'G'B'A"),
                   babl_type ("float"),
                   babl_component ("B'"),
                   NULL);
  babl_format_new ("name", "A float",
                   babl_model ("RGBA"),
                   babl_type ("float"),
                   babl_component ("A"),
                   NULL);

  babl_format_new ("name", "R double",
                   babl_model ("RGBA"),
                   babl_type ("double"),
                   babl_component ("R"),
                   NULL);
  babl_format_new ("name", "R' double",
                   babl_model ("R'G'B'A"),
                   babl_type ("double"),
                   babl_component ("R'"),
                   NULL);
  babl_format_new ("name", "G double",
                   babl_model ("RGBA"),
                   babl_type ("double"),
                   babl_component ("G"),
                   NULL);
  babl_format_new ("name", "G' double",
                   babl_model ("R'G'B'A"),
                   babl_type ("double"),
                   babl_component ("G'"),
                   NULL);
  babl_format_new ("name", "B double",
                   babl_model ("RGBA"),
                   babl_type ("double"),
                   babl_component ("B"),
                   NULL);
  babl_format_new ("name", "B' double",
                   babl_model ("R'G'B'A"),
                   babl_type ("double"),
                   babl_component ("B'"),
                   NULL);
  babl_format_new ("name", "A double",
                   babl_model ("RGBA"),
                   babl_type ("double"),
                   babl_component ("A"),
                   NULL);
}

void
gimp_babl_init_fishes (GimpInitStatusFunc status_callback)
{
  /* create a bunch of fishes - to decrease the initial lazy
   * initialization cost for some interactions
   */
  static const struct
  {
    const gchar *from_format;
    const gchar *to_format;
  }
  fishes[] =
  {
    { "Y' u8",          "RaGaBaA float" },
    { "Y u8",           "RaGaBaA float" },
    { "R'G'B'A u8",     "RaGaBaA float" },
    { "R'G'B'A float",  "R'G'B'A u8"    },
    { "R'G'B'A float",  "R'G'B' u8"     },
    { "R'G'B'A u8",     "RGBA float"    },
    { "RGBA float",     "R'G'B'A u8"    },
    { "RGBA float",     "R'G'B'A u8"    },
    { "RGBA float",     "R'G'B'A float" },
    { "Y' u8",          "R'G'B' u8"     },
    { "Y u8",           "Y float"       },
    { "R'G'B' u8",      "cairo-RGB24"   },
    { "R'G'B' u8",      "R'G'B'A float" },
    { "R'G'B' u8",      "R'G'B'A u8"    },
    { "R'G'B'A u8",     "R'G'B'A float" },
    { "R'G'B'A u8",     "cairo-ARGB32"  },
    { "R'G'B'A double", "RGBA float"    },
    { "R'G'B'A float",  "RGBA double"   },
    { "R'G'B' u8",      "RGB float"     },
    { "RGB float",      "R'G'B'A float" },
    { "R'G'B' u8",      "RGBA float"    },
    { "RaGaBaA float",  "R'G'B'A float" },
    { "RaGaBaA float",  "RGBA float"    },
    { "RGBA float",     "RaGaBaA float" },
    { "R'G'B' u8",      "RaGaBaA float" },
    { "cairo-ARGB32",   "R'G'B'A u8"    }
  };

  gint i;

  for (i = 0; i < G_N_ELEMENTS (fishes); i++)
    {
      status_callback (NULL, NULL,
                       (gdouble) (i + 1) /
                       (gdouble) G_N_ELEMENTS (fishes) * 0.8);

      babl_fish (babl_format (fishes[i].from_format),
                 babl_format (fishes[i].to_format));
    }
}

static const struct
{
  const gchar *name;
  const gchar *description;
}
babl_descriptions[] =
{
  { "RGB u8",         N_("RGB") },
  { "R'G'B' u8",      N_("RGB") },
  { "RGB u16",        N_("RGB") },
  { "R'G'B' u16",     N_("RGB") },
  { "RGB u32",        N_("RGB") },
  { "R'G'B' u32",     N_("RGB") },
  { "RGB half",       N_("RGB") },
  { "R'G'B' half",    N_("RGB") },
  { "RGB float",      N_("RGB") },
  { "R'G'B' float",   N_("RGB") },
  { "RGB double",     N_("RGB") },
  { "R'G'B' double",  N_("RGB") },

  { "RGBA u8",        N_("RGB-alpha") },
  { "R'G'B'A u8",     N_("RGB-alpha") },
  { "RGBA u16",       N_("RGB-alpha") },
  { "R'G'B'A u16",    N_("RGB-alpha") },
  { "RGBA u32",       N_("RGB-alpha") },
  { "R'G'B'A u32",    N_("RGB-alpha") },
  { "RGBA half",      N_("RGB-alpha") },
  { "R'G'B'A half",   N_("RGB-alpha") },
  { "RGBA float",     N_("RGB-alpha") },
  { "R'G'B'A float",  N_("RGB-alpha") },
  { "RGBA double",    N_("RGB-alpha") },
  { "R'G'B'A double", N_("RGB-alpha") },

  { "Y u8",           N_("Grayscale") },
  { "Y' u8",          N_("Grayscale") },
  { "Y u16",          N_("Grayscale") },
  { "Y' u16",         N_("Grayscale") },
  { "Y u32",          N_("Grayscale") },
  { "Y' u32",         N_("Grayscale") },
  { "Y half",         N_("Grayscale") },
  { "Y' half",        N_("Grayscale") },
  { "Y float",        N_("Grayscale") },
  { "Y' float",       N_("Grayscale") },
  { "Y double",       N_("Grayscale") },
  { "Y' double",      N_("Grayscale") },

  { "YA u8",          N_("Grayscale-alpha") },
  { "Y'A u8",         N_("Grayscale-alpha") },
  { "YA u16",         N_("Grayscale-alpha") },
  { "Y'A u16",        N_("Grayscale-alpha") },
  { "YA u32",         N_("Grayscale-alpha") },
  { "Y'A u32",        N_("Grayscale-alpha") },
  { "YA half",        N_("Grayscale-alpha") },
  { "Y'A half",       N_("Grayscale-alpha") },
  { "YA float",       N_("Grayscale-alpha") },
  { "Y'A float",      N_("Grayscale-alpha") },
  { "YA double",      N_("Grayscale-alpha") },
  { "Y'A double",     N_("Grayscale-alpha") },

  { "R u8",           N_("Red component") },
  { "R' u8",          N_("Red component") },
  { "R u16",          N_("Red component") },
  { "R' u16",         N_("Red component") },
  { "R u32",          N_("Red component") },
  { "R' u32",         N_("Red component") },
  { "R half",         N_("Red component") },
  { "R' half",        N_("Red component") },
  { "R float",        N_("Red component") },
  { "R' float",       N_("Red component") },
  { "R double",       N_("Red component") },
  { "R' double",      N_("Red component") },

  { "G u8",           N_("Green component") },
  { "G' u8",          N_("Green component") },
  { "G u16",          N_("Green component") },
  { "G' u16",         N_("Green component") },
  { "G u32",          N_("Green component") },
  { "G' u32",         N_("Green component") },
  { "G half",         N_("Green component") },
  { "G' half",        N_("Green component") },
  { "G float",        N_("Green component") },
  { "G' float",       N_("Green component") },
  { "G double",       N_("Green component") },
  { "G' double",      N_("Green component") },

  { "B u8",           N_("Blue component") },
  { "B' u8",          N_("Blue component") },
  { "B u16",          N_("Blue component") },
  { "B' u16",         N_("Blue component") },
  { "B u32",          N_("Blue component") },
  { "B' u32",         N_("Blue component") },
  { "B half",         N_("Blue component") },
  { "B' half",        N_("Blue component") },
  { "B float",        N_("Blue component") },
  { "B' float",       N_("Blue component") },
  { "B double",       N_("Blue component") },
  { "B' double",      N_("Blue component") },

  { "A u8",          N_("Alpha component") },
  { "A u16",         N_("Alpha component") },
  { "A u32",         N_("Alpha component") },
  { "A half",        N_("Alpha component") },
  { "A float",       N_("Alpha component") },
  { "A double",      N_("Alpha component") }
};

static GHashTable *babl_description_hash = NULL;

const gchar *
gimp_babl_format_get_description (const Babl *babl)
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

GimpColorProfile *
gimp_babl_format_get_color_profile (const Babl *format)
{
  static GimpColorProfile *srgb_profile        = NULL;
  static GimpColorProfile *linear_rgb_profile  = NULL;
  static GimpColorProfile *gray_profile        = NULL;
  static GimpColorProfile *linear_gray_profile = NULL;

  g_return_val_if_fail (format != NULL, NULL);

  if (gimp_babl_format_get_base_type (format) == GIMP_GRAY)
    {
      if (gimp_babl_format_get_linear (format))
        {
          if (! linear_gray_profile)
            {
              linear_gray_profile = gimp_color_profile_new_d65_gray_linear ();
              g_object_add_weak_pointer (G_OBJECT (linear_gray_profile),
                                         (gpointer) &linear_gray_profile);
            }

          return linear_gray_profile;
        }
      else
        {
          if (! gray_profile)
            {
              gray_profile = gimp_color_profile_new_d65_gray_srgb_trc ();
              g_object_add_weak_pointer (G_OBJECT (gray_profile),
                                         (gpointer) &gray_profile);
            }

          return gray_profile;
        }
    }
  else
    {
      if (gimp_babl_format_get_linear (format))
        {
          if (! linear_rgb_profile)
            {
              linear_rgb_profile = gimp_color_profile_new_rgb_srgb_linear ();
              g_object_add_weak_pointer (G_OBJECT (linear_rgb_profile),
                                         (gpointer) &linear_rgb_profile);
            }

          return linear_rgb_profile;
        }
      else
        {
          if (! srgb_profile)
            {
              srgb_profile = gimp_color_profile_new_rgb_srgb ();
              g_object_add_weak_pointer (G_OBJECT (srgb_profile),
                                         (gpointer) &srgb_profile);
            }

          return srgb_profile;
        }
    }
}

GimpImageBaseType
gimp_babl_format_get_base_type (const Babl *format)
{
  const Babl *model;

  g_return_val_if_fail (format != NULL, -1);

  model = babl_format_get_model (format);

  if (model == babl_model ("Y")  ||
      model == babl_model ("Y'") ||
      model == babl_model ("YA") ||
      model == babl_model ("Y'A"))
    {
      return GIMP_GRAY;
    }
  else if (model == babl_model ("RGB")     ||
           model == babl_model ("R'G'B'")  ||
           model == babl_model ("RGBA")    ||
           model == babl_model ("R'G'B'A") ||
           model == babl_model ("RaGaBaA") ||
           model == babl_model ("R'aG'aB'aA"))
    {
      return GIMP_RGB;
    }
  else if (babl_format_is_palette (format))
    {
      return GIMP_INDEXED;
    }

  g_return_val_if_reached (-1);
}

GimpComponentType
gimp_babl_format_get_component_type (const Babl *format)
{
  const Babl *type;

  g_return_val_if_fail (format != NULL, -1);

  type = babl_format_get_type (format, 0);

  if (type == babl_type ("u8"))
    return GIMP_COMPONENT_TYPE_U8;
  else if (type == babl_type ("u16"))
    return GIMP_COMPONENT_TYPE_U16;
  else if (type == babl_type ("u32"))
    return GIMP_COMPONENT_TYPE_U32;
  else if (type == babl_type ("half"))
    return GIMP_COMPONENT_TYPE_HALF;
  else if (type == babl_type ("float"))
    return GIMP_COMPONENT_TYPE_FLOAT;
  else if (type == babl_type ("double"))
    return GIMP_COMPONENT_TYPE_DOUBLE;

  g_return_val_if_reached (-1);
}

GimpPrecision
gimp_babl_format_get_precision (const Babl *format)
{
  const Babl *type;

  g_return_val_if_fail (format != NULL, -1);

  type = babl_format_get_type (format, 0);

  if (gimp_babl_format_get_linear (format))
    {
      if (type == babl_type ("u8"))
        return GIMP_PRECISION_U8_LINEAR;
      else if (type == babl_type ("u16"))
        return GIMP_PRECISION_U16_LINEAR;
      else if (type == babl_type ("u32"))
        return GIMP_PRECISION_U32_LINEAR;
      else if (type == babl_type ("half"))
        return GIMP_PRECISION_HALF_LINEAR;
      else if (type == babl_type ("float"))
        return GIMP_PRECISION_FLOAT_LINEAR;
      else if (type == babl_type ("double"))
        return GIMP_PRECISION_DOUBLE_LINEAR;
    }
  else
    {
      if (type == babl_type ("u8"))
        return GIMP_PRECISION_U8_GAMMA;
      else if (type == babl_type ("u16"))
        return GIMP_PRECISION_U16_GAMMA;
      else if (type == babl_type ("u32"))
        return GIMP_PRECISION_U32_GAMMA;
      else if (type == babl_type ("half"))
        return GIMP_PRECISION_HALF_GAMMA;
      else if (type == babl_type ("float"))
        return GIMP_PRECISION_FLOAT_GAMMA;
      else if (type == babl_type ("double"))
        return GIMP_PRECISION_DOUBLE_GAMMA;
    }

  g_return_val_if_reached (-1);
}

gboolean
gimp_babl_format_get_linear (const Babl *format)
{
  const Babl *model;

  g_return_val_if_fail (format != NULL, FALSE);

  model = babl_format_get_model (format);

  if (model == babl_model ("Y")    ||
      model == babl_model ("YA")   ||
      model == babl_model ("RGB")  ||
      model == babl_model ("RGBA") ||
      model == babl_model ("RaGaBaA"))
    {
      return TRUE;
    }
  else if (model == babl_model ("Y'")      ||
           model == babl_model ("Y'A")     ||
           model == babl_model ("R'G'B'")  ||
           model == babl_model ("R'G'B'A") ||
           model == babl_model ("R'aG'aB'aA"))
    {
      return FALSE;
    }
  else if (babl_format_is_palette (format))
    {
      return FALSE;
    }

  g_return_val_if_reached (FALSE);
}

GimpComponentType
gimp_babl_component_type (GimpPrecision precision)
{
  switch (precision)
    {
    case GIMP_PRECISION_U8_LINEAR:
    case GIMP_PRECISION_U8_GAMMA:
      return GIMP_COMPONENT_TYPE_U8;

    case GIMP_PRECISION_U16_LINEAR:
    case GIMP_PRECISION_U16_GAMMA:
      return GIMP_COMPONENT_TYPE_U16;

    case GIMP_PRECISION_U32_LINEAR:
    case GIMP_PRECISION_U32_GAMMA:
      return GIMP_COMPONENT_TYPE_U32;

    case GIMP_PRECISION_HALF_LINEAR:
    case GIMP_PRECISION_HALF_GAMMA:
      return GIMP_COMPONENT_TYPE_HALF;

    case GIMP_PRECISION_FLOAT_LINEAR:
    case GIMP_PRECISION_FLOAT_GAMMA:
      return GIMP_COMPONENT_TYPE_FLOAT;

    case GIMP_PRECISION_DOUBLE_LINEAR:
    case GIMP_PRECISION_DOUBLE_GAMMA:
      return GIMP_COMPONENT_TYPE_DOUBLE;
    }

  g_return_val_if_reached (-1);
}

gboolean
gimp_babl_linear (GimpPrecision precision)
{
  switch (precision)
    {
    case GIMP_PRECISION_U8_LINEAR:
    case GIMP_PRECISION_U16_LINEAR:
    case GIMP_PRECISION_U32_LINEAR:
    case GIMP_PRECISION_HALF_LINEAR:
    case GIMP_PRECISION_FLOAT_LINEAR:
    case GIMP_PRECISION_DOUBLE_LINEAR:
      return TRUE;

    case GIMP_PRECISION_U8_GAMMA:
    case GIMP_PRECISION_U16_GAMMA:
    case GIMP_PRECISION_U32_GAMMA:
    case GIMP_PRECISION_HALF_GAMMA:
    case GIMP_PRECISION_FLOAT_GAMMA:
    case GIMP_PRECISION_DOUBLE_GAMMA:
      return FALSE;
    }

  g_return_val_if_reached (FALSE);
}

GimpPrecision
gimp_babl_precision (GimpComponentType component,
                     gboolean          linear)
{
  switch (component)
    {
    case GIMP_COMPONENT_TYPE_U8:
      if (linear)
        return GIMP_PRECISION_U8_LINEAR;
      else
        return GIMP_PRECISION_U8_GAMMA;

    case GIMP_COMPONENT_TYPE_U16:
      if (linear)
        return GIMP_PRECISION_U16_LINEAR;
      else
        return GIMP_PRECISION_U16_GAMMA;

    case GIMP_COMPONENT_TYPE_U32:
      if (linear)
        return GIMP_PRECISION_U32_LINEAR;
      else
        return GIMP_PRECISION_U32_GAMMA;

    case GIMP_COMPONENT_TYPE_HALF:
      if (linear)
        return GIMP_PRECISION_HALF_LINEAR;
      else
        return GIMP_PRECISION_HALF_GAMMA;

    case GIMP_COMPONENT_TYPE_FLOAT:
      if (linear)
        return GIMP_PRECISION_FLOAT_LINEAR;
      else
        return GIMP_PRECISION_FLOAT_GAMMA;

    case GIMP_COMPONENT_TYPE_DOUBLE:
      if (linear)
        return GIMP_PRECISION_DOUBLE_LINEAR;
      else
        return GIMP_PRECISION_DOUBLE_GAMMA;

    default:
      break;
    }

  g_return_val_if_reached (-1);
}

gboolean
gimp_babl_is_valid (GimpImageBaseType base_type,
                    GimpPrecision     precision)
{
  switch (base_type)
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      return TRUE;

    case GIMP_INDEXED:
      switch (precision)
        {
        case GIMP_PRECISION_U8_GAMMA:
          return TRUE;

        default:
          return FALSE;
        }
    }

  g_return_val_if_reached (FALSE);
}

GimpComponentType
gimp_babl_is_bounded (GimpPrecision precision)
{
  switch (gimp_babl_component_type (precision))
    {
    case GIMP_COMPONENT_TYPE_U8:
    case GIMP_COMPONENT_TYPE_U16:
    case GIMP_COMPONENT_TYPE_U32:
      return TRUE;

    case GIMP_COMPONENT_TYPE_HALF:
    case GIMP_COMPONENT_TYPE_FLOAT:
    case GIMP_COMPONENT_TYPE_DOUBLE:
      return FALSE;
    }

  g_return_val_if_reached (FALSE);
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
        case GIMP_PRECISION_U8_LINEAR:
          if (with_alpha)
            return babl_format ("RGBA u8");
          else
            return babl_format ("RGB u8");

        case GIMP_PRECISION_U8_GAMMA:
          if (with_alpha)
            return babl_format ("R'G'B'A u8");
          else
            return babl_format ("R'G'B' u8");

        case GIMP_PRECISION_U16_LINEAR:
          if (with_alpha)
            return babl_format ("RGBA u16");
          else
            return babl_format ("RGB u16");

        case GIMP_PRECISION_U16_GAMMA:
          if (with_alpha)
            return babl_format ("R'G'B'A u16");
          else
            return babl_format ("R'G'B' u16");

        case GIMP_PRECISION_U32_LINEAR:
          if (with_alpha)
            return babl_format ("RGBA u32");
          else
            return babl_format ("RGB u32");

        case GIMP_PRECISION_U32_GAMMA:
          if (with_alpha)
            return babl_format ("R'G'B'A u32");
          else
            return babl_format ("R'G'B' u32");

        case GIMP_PRECISION_HALF_LINEAR:
          if (with_alpha)
            return babl_format ("RGBA half");
          else
            return babl_format ("RGB half");

        case GIMP_PRECISION_HALF_GAMMA:
          if (with_alpha)
            return babl_format ("R'G'B'A half");
          else
            return babl_format ("R'G'B' half");

        case GIMP_PRECISION_FLOAT_LINEAR:
          if (with_alpha)
            return babl_format ("RGBA float");
          else
            return babl_format ("RGB float");

        case GIMP_PRECISION_FLOAT_GAMMA:
          if (with_alpha)
            return babl_format ("R'G'B'A float");
          else
            return babl_format ("R'G'B' float");

        case GIMP_PRECISION_DOUBLE_LINEAR:
          if (with_alpha)
            return babl_format ("RGBA double");
          else
            return babl_format ("RGB double");

        case GIMP_PRECISION_DOUBLE_GAMMA:
          if (with_alpha)
            return babl_format ("R'G'B'A double");
          else
            return babl_format ("R'G'B' double");

        default:
          break;
        }
      break;

    case GIMP_GRAY:
      switch (precision)
        {
        case GIMP_PRECISION_U8_LINEAR:
          if (with_alpha)
            return babl_format ("YA u8");
          else
            return babl_format ("Y u8");

        case GIMP_PRECISION_U8_GAMMA:
          if (with_alpha)
            return babl_format ("Y'A u8");
          else
            return babl_format ("Y' u8");

        case GIMP_PRECISION_U16_LINEAR:
          if (with_alpha)
            return babl_format ("YA u16");
          else
            return babl_format ("Y u16");

        case GIMP_PRECISION_U16_GAMMA:
          if (with_alpha)
            return babl_format ("Y'A u16");
          else
            return babl_format ("Y' u16");

        case GIMP_PRECISION_U32_LINEAR:
          if (with_alpha)
            return babl_format ("YA u32");
          else
            return babl_format ("Y u32");

        case GIMP_PRECISION_U32_GAMMA:
          if (with_alpha)
            return babl_format ("Y'A u32");
          else
            return babl_format ("Y' u32");

        case GIMP_PRECISION_HALF_LINEAR:
          if (with_alpha)
            return babl_format ("YA half");
          else
            return babl_format ("Y half");

        case GIMP_PRECISION_HALF_GAMMA:
          if (with_alpha)
            return babl_format ("Y'A half");
          else
            return babl_format ("Y' half");

        case GIMP_PRECISION_FLOAT_LINEAR:
          if (with_alpha)
            return babl_format ("YA float");
          else
            return babl_format ("Y float");

        case GIMP_PRECISION_FLOAT_GAMMA:
          if (with_alpha)
            return babl_format ("Y'A float");
          else
            return babl_format ("Y' float");

        case GIMP_PRECISION_DOUBLE_LINEAR:
          if (with_alpha)
            return babl_format ("YA double");
          else
            return babl_format ("Y double");

        case GIMP_PRECISION_DOUBLE_GAMMA:
          if (with_alpha)
            return babl_format ("Y'A double");
          else
            return babl_format ("Y' double");

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

const Babl *
gimp_babl_mask_format (GimpPrecision precision)
{
  switch (gimp_babl_component_type (precision))
    {
    case GIMP_COMPONENT_TYPE_U8:     return babl_format ("Y u8");
    case GIMP_COMPONENT_TYPE_U16:    return babl_format ("Y u16");
    case GIMP_COMPONENT_TYPE_U32:    return babl_format ("Y u32");
    case GIMP_COMPONENT_TYPE_HALF:   return babl_format ("Y half");
    case GIMP_COMPONENT_TYPE_FLOAT:  return babl_format ("Y float");
    case GIMP_COMPONENT_TYPE_DOUBLE: return babl_format ("Y double");
    }

  g_return_val_if_reached (NULL);
}

const Babl *
gimp_babl_component_format (GimpImageBaseType base_type,
                            GimpPrecision     precision,
                            gint              index)
{
  switch (base_type)
    {
    case GIMP_RGB:
      switch (precision)
        {
        case GIMP_PRECISION_U8_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("R u8");
            case 1: return babl_format ("G u8");
            case 2: return babl_format ("B u8");
            case 3: return babl_format ("A u8");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_U8_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("R' u8");
            case 1: return babl_format ("G' u8");
            case 2: return babl_format ("B' u8");
            case 3: return babl_format ("A u8");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_U16_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("R u16");
            case 1: return babl_format ("G u16");
            case 2: return babl_format ("B u16");
            case 3: return babl_format ("A u16");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_U16_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("R' u16");
            case 1: return babl_format ("G' u16");
            case 2: return babl_format ("B' u16");
            case 3: return babl_format ("A u16");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_U32_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("R u32");
            case 1: return babl_format ("G u32");
            case 2: return babl_format ("B u32");
            case 3: return babl_format ("A u32");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_U32_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("R' u32");
            case 1: return babl_format ("G' u32");
            case 2: return babl_format ("B' u32");
            case 3: return babl_format ("A u32");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_HALF_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("R half");
            case 1: return babl_format ("G half");
            case 2: return babl_format ("B half");
            case 3: return babl_format ("A half");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_HALF_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("R' half");
            case 1: return babl_format ("G' half");
            case 2: return babl_format ("B' half");
            case 3: return babl_format ("A half");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_FLOAT_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("R float");
            case 1: return babl_format ("G float");
            case 2: return babl_format ("B float");
            case 3: return babl_format ("A float");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_FLOAT_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("R' float");
            case 1: return babl_format ("G' float");
            case 2: return babl_format ("B' float");
            case 3: return babl_format ("A float");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_DOUBLE_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("R double");
            case 1: return babl_format ("G double");
            case 2: return babl_format ("B double");
            case 3: return babl_format ("A double");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_DOUBLE_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("R' double");
            case 1: return babl_format ("G' double");
            case 2: return babl_format ("B' double");
            case 3: return babl_format ("A double");
            default:
              break;
            }
          break;

        default:
          break;
        }
      break;

    case GIMP_GRAY:
      switch (precision)
        {
        case GIMP_PRECISION_U8_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y u8");
            case 1: return babl_format ("A u8");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_U8_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("Y' u8");
            case 1: return babl_format ("A u8");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_U16_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y u16");
            case 1: return babl_format ("A u16");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_U16_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("Y' u16");
            case 1: return babl_format ("A u16");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_U32_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y u32");
            case 1: return babl_format ("A u32");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_U32_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("Y' u32");
            case 1: return babl_format ("A u32");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_HALF_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y half");
            case 1: return babl_format ("A half");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_HALF_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("Y' half");
            case 1: return babl_format ("A half");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_FLOAT_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y float");
            case 1: return babl_format ("A float");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_FLOAT_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("Y' float");
            case 1: return babl_format ("A float");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_DOUBLE_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y double");
            case 1: return babl_format ("A double");
            default:
              break;
            }
          break;

        case GIMP_PRECISION_DOUBLE_GAMMA:
          switch (index)
            {
            case 0: return babl_format ("Y' double");
            case 1: return babl_format ("A double");
            default:
              break;
            }
          break;

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

const Babl *
gimp_babl_format_change_component_type (const Babl        *format,
                                        GimpComponentType  component)
{
  g_return_val_if_fail (format != NULL, NULL);

  return gimp_babl_format (gimp_babl_format_get_base_type (format),
                           gimp_babl_precision (
                             component,
                             gimp_babl_format_get_linear (format)),
                           babl_format_has_alpha (format));
}

const Babl *
gimp_babl_format_change_linear (const Babl *format,
                                gboolean    linear)
{
  g_return_val_if_fail (format != NULL, NULL);

  return gimp_babl_format (gimp_babl_format_get_base_type (format),
                           gimp_babl_precision (
                             gimp_babl_format_get_component_type (format),
                             linear),
                           babl_format_has_alpha (format));
}

gchar **
gimp_babl_print_pixel (const Babl *format,
                       gpointer    pixel)
{
  GimpPrecision   precision;
  gint            n_components;
  guchar          tmp_pixel[32];
  gchar         **strings;

  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (pixel != NULL, NULL);

  precision = gimp_babl_format_get_precision (format);

  if (babl_format_is_palette (format))
    {
      const Babl *f = gimp_babl_format (GIMP_RGB, precision,
                                        babl_format_has_alpha (format));

      babl_process (babl_fish (format, f), pixel, tmp_pixel, 1);

      format = f;
      pixel  = tmp_pixel;
    }

  n_components = babl_format_get_n_components (format);

  strings = g_new0 (gchar *, n_components + 1);

  switch (gimp_babl_format_get_component_type (format))
    {
    case GIMP_COMPONENT_TYPE_U8:
      {
        guchar *color = pixel;
        gint    i;

        for (i = 0; i < n_components; i++)
          strings[i] = g_strdup_printf ("%d", color[i]);
      }
      break;

    case GIMP_COMPONENT_TYPE_U16:
      {
        guint16 *color = pixel;
        gint     i;

        for (i = 0; i < n_components; i++)
          strings[i] = g_strdup_printf ("%u", color[i]);
      }
      break;

    case GIMP_COMPONENT_TYPE_U32:
      {
        guint32 *color = pixel;
        gint     i;

        for (i = 0; i < n_components; i++)
          strings[i] = g_strdup_printf ("%u", color[i]);
      }
      break;

    case GIMP_COMPONENT_TYPE_HALF:
      {
        GimpPrecision p;
        const Babl   *f;

        p = gimp_babl_precision (GIMP_COMPONENT_TYPE_FLOAT,
                                 gimp_babl_format_get_linear (format));

        f = gimp_babl_format (gimp_babl_format_get_base_type (format),
                              p,
                              babl_format_has_alpha (format));

        babl_process (babl_fish (format, f), pixel, tmp_pixel, 1);

        pixel = tmp_pixel;
      }
      /* fall through */

    case GIMP_COMPONENT_TYPE_FLOAT:
      {
        gfloat *color = pixel;
        gint    i;

        for (i = 0; i < n_components; i++)
          strings[i] = g_strdup_printf ("%0.6f", color[i]);
      }
      break;

    case GIMP_COMPONENT_TYPE_DOUBLE:
      {
        gdouble *color = pixel;
        gint     i;

        for (i = 0; i < n_components; i++)
          strings[i] = g_strdup_printf ("%0.6f", color[i]);
      }
      break;
    }

  return strings;
}
