/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-babl.c
 * Copyright (C) 2012 Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmacolor/ligmacolor.h"

#include "ligma-gegl-types.h"

#include "ligma-babl.h"

#include "ligma-intl.h"


void
ligma_babl_init (void)
{
  static const gchar *babl_types[] =
  {
    "u8",
    "u16",
    "u32",
    "half",
    "float",
    "double"
  };

  gint i;

  for (i = 0; i < G_N_ELEMENTS (babl_types); i++)
    {
      gchar name[16];

      g_snprintf (name, sizeof (name), "R %s", babl_types[i]);
      babl_format_new ("name", name,
                       babl_model ("RGBA"),
                       babl_type (babl_types[i]),
                       babl_component ("R"),
                       NULL);
      g_snprintf (name, sizeof (name), "R' %s", babl_types[i]);
      babl_format_new ("name", name,
                       babl_model ("R'G'B'A"),
                       babl_type (babl_types[i]),
                       babl_component ("R'"),
                       NULL);
      g_snprintf (name, sizeof (name), "R~ %s", babl_types[i]);
      babl_format_new ("name", name,
                       babl_model ("R~G~B~A"),
                       babl_type (babl_types[i]),
                       babl_component ("R~"),
                       NULL);

      g_snprintf (name, sizeof (name), "G %s", babl_types[i]);
      babl_format_new ("name", name,
                       babl_model ("RGBA"),
                       babl_type (babl_types[i]),
                       babl_component ("G"),
                       NULL);
      g_snprintf (name, sizeof (name), "G' %s", babl_types[i]);
      babl_format_new ("name", name,
                       babl_model ("R'G'B'A"),
                       babl_type (babl_types[i]),
                       babl_component ("G'"),
                       NULL);
      g_snprintf (name, sizeof (name), "G~ %s", babl_types[i]);
      babl_format_new ("name", name,
                       babl_model ("R~G~B~A"),
                       babl_type (babl_types[i]),
                       babl_component ("G~"),
                       NULL);

      g_snprintf (name, sizeof (name), "B %s", babl_types[i]);
      babl_format_new ("name", name,
                       babl_model ("RGBA"),
                       babl_type (babl_types[i]),
                       babl_component ("B"),
                       NULL);
      g_snprintf (name, sizeof (name), "B' %s", babl_types[i]);
      babl_format_new ("name", name,
                       babl_model ("R'G'B'A"),
                       babl_type (babl_types[i]),
                       babl_component ("B'"),
                       NULL);
      g_snprintf (name, sizeof (name), "B~ %s", babl_types[i]);
      babl_format_new ("name", name,
                       babl_model ("R~G~B~A"),
                       babl_type (babl_types[i]),
                       babl_component ("B~"),
                       NULL);

      g_snprintf (name, sizeof (name), "A %s", babl_types[i]);
      babl_format_new ("name", name,
                       babl_model ("RGBA"),
                       babl_type (babl_types[i]),
                       babl_component ("A"),
                       NULL);
    }
}

void
ligma_babl_init_fishes (LigmaInitStatusFunc status_callback)
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
  { "R~G~B~ u8",      N_("RGB") },
  { "RGB u16",        N_("RGB") },
  { "R'G'B' u16",     N_("RGB") },
  { "R~G~B~ u16",     N_("RGB") },
  { "RGB u32",        N_("RGB") },
  { "R'G'B' u32",     N_("RGB") },
  { "R~G~B~ u32",     N_("RGB") },
  { "RGB half",       N_("RGB") },
  { "R'G'B' half",    N_("RGB") },
  { "R~G~B~ half",    N_("RGB") },
  { "RGB float",      N_("RGB") },
  { "R'G'B' float",   N_("RGB") },
  { "R~G~B~ float",   N_("RGB") },
  { "RGB double",     N_("RGB") },
  { "R'G'B' double",  N_("RGB") },
  { "R~G~B~ double",  N_("RGB") },

  { "RGBA u8",        N_("RGB-alpha") },
  { "R'G'B'A u8",     N_("RGB-alpha") },
  { "R~G~B~A u8",     N_("RGB-alpha") },
  { "RGBA u16",       N_("RGB-alpha") },
  { "R'G'B'A u16",    N_("RGB-alpha") },
  { "R~G~B~A u16",    N_("RGB-alpha") },
  { "RGBA u32",       N_("RGB-alpha") },
  { "R'G'B'A u32",    N_("RGB-alpha") },
  { "R~G~B~A u32",    N_("RGB-alpha") },
  { "RGBA half",      N_("RGB-alpha") },
  { "R'G'B'A half",   N_("RGB-alpha") },
  { "R~G~B~A half",   N_("RGB-alpha") },
  { "RGBA float",     N_("RGB-alpha") },
  { "R'G'B'A float",  N_("RGB-alpha") },
  { "R~G~B~A float",  N_("RGB-alpha") },
  { "RGBA double",    N_("RGB-alpha") },
  { "R'G'B'A double", N_("RGB-alpha") },
  { "R~G~B~A double", N_("RGB-alpha") },

  { "Y u8",           N_("Grayscale") },
  { "Y' u8",          N_("Grayscale") },
  { "Y~ u8",          N_("Grayscale") },
  { "Y u16",          N_("Grayscale") },
  { "Y' u16",         N_("Grayscale") },
  { "Y~ u16",         N_("Grayscale") },
  { "Y u32",          N_("Grayscale") },
  { "Y' u32",         N_("Grayscale") },
  { "Y~ u32",         N_("Grayscale") },
  { "Y half",         N_("Grayscale") },
  { "Y' half",        N_("Grayscale") },
  { "Y~ half",        N_("Grayscale") },
  { "Y float",        N_("Grayscale") },
  { "Y' float",       N_("Grayscale") },
  { "Y~ float",       N_("Grayscale") },
  { "Y double",       N_("Grayscale") },
  { "Y' double",      N_("Grayscale") },
  { "Y~ double",      N_("Grayscale") },

  { "YA u8",          N_("Grayscale-alpha") },
  { "Y'A u8",         N_("Grayscale-alpha") },
  { "Y~A u8",         N_("Grayscale-alpha") },
  { "YA u16",         N_("Grayscale-alpha") },
  { "Y'A u16",        N_("Grayscale-alpha") },
  { "Y~A u16",        N_("Grayscale-alpha") },
  { "YA u32",         N_("Grayscale-alpha") },
  { "Y'A u32",        N_("Grayscale-alpha") },
  { "Y~A u32",        N_("Grayscale-alpha") },
  { "YA half",        N_("Grayscale-alpha") },
  { "Y'A half",       N_("Grayscale-alpha") },
  { "Y~A half",       N_("Grayscale-alpha") },
  { "YA float",       N_("Grayscale-alpha") },
  { "Y'A float",      N_("Grayscale-alpha") },
  { "Y~A float",      N_("Grayscale-alpha") },
  { "YA double",      N_("Grayscale-alpha") },
  { "Y'A double",     N_("Grayscale-alpha") },
  { "Y~A double",     N_("Grayscale-alpha") },

  { "R u8",           N_("Red component") },
  { "R' u8",          N_("Red component") },
  { "R~ u8",          N_("Red component") },
  { "R u16",          N_("Red component") },
  { "R' u16",         N_("Red component") },
  { "R~ u16",         N_("Red component") },
  { "R u32",          N_("Red component") },
  { "R' u32",         N_("Red component") },
  { "R~ u32",         N_("Red component") },
  { "R half",         N_("Red component") },
  { "R' half",        N_("Red component") },
  { "R~ half",        N_("Red component") },
  { "R float",        N_("Red component") },
  { "R' float",       N_("Red component") },
  { "R~ float",       N_("Red component") },
  { "R double",       N_("Red component") },
  { "R' double",      N_("Red component") },
  { "R~ double",      N_("Red component") },

  { "G u8",           N_("Green component") },
  { "G' u8",          N_("Green component") },
  { "G~ u8",          N_("Green component") },
  { "G u16",          N_("Green component") },
  { "G' u16",         N_("Green component") },
  { "G~ u16",         N_("Green component") },
  { "G u32",          N_("Green component") },
  { "G' u32",         N_("Green component") },
  { "G~ u32",         N_("Green component") },
  { "G half",         N_("Green component") },
  { "G' half",        N_("Green component") },
  { "G~ half",        N_("Green component") },
  { "G float",        N_("Green component") },
  { "G' float",       N_("Green component") },
  { "G~ float",       N_("Green component") },
  { "G double",       N_("Green component") },
  { "G' double",      N_("Green component") },
  { "G~ double",      N_("Green component") },

  { "B u8",           N_("Blue component") },
  { "B' u8",          N_("Blue component") },
  { "B~ u8",          N_("Blue component") },
  { "B u16",          N_("Blue component") },
  { "B' u16",         N_("Blue component") },
  { "B~ u16",         N_("Blue component") },
  { "B u32",          N_("Blue component") },
  { "B' u32",         N_("Blue component") },
  { "B~ u32",         N_("Blue component") },
  { "B half",         N_("Blue component") },
  { "B' half",        N_("Blue component") },
  { "B~ half",        N_("Blue component") },
  { "B float",        N_("Blue component") },
  { "B' float",       N_("Blue component") },
  { "B~ float",       N_("Blue component") },
  { "B double",       N_("Blue component") },
  { "B' double",      N_("Blue component") },
  { "B~ double",      N_("Blue component") },

  { "A u8",           N_("Alpha component") },
  { "A u16",          N_("Alpha component") },
  { "A u32",          N_("Alpha component") },
  { "A half",         N_("Alpha component") },
  { "A float",        N_("Alpha component") },
  { "A double",       N_("Alpha component") }
};

static GHashTable *babl_description_hash = NULL;

const gchar *
ligma_babl_format_get_description (const Babl *babl)
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
                                     babl_format_get_encoding (babl));

  if (description)
    return description;

  return g_strconcat ("ERROR: unknown Babl format ",
                      babl_format_get_encoding (babl), NULL);
}

LigmaColorProfile *
ligma_babl_get_builtin_color_profile (LigmaImageBaseType base_type,
                                     LigmaTRCType       trc)
{
  static LigmaColorProfile *srgb_profile        = NULL;
  static LigmaColorProfile *linear_rgb_profile  = NULL;
  static LigmaColorProfile *gray_profile        = NULL;
  static LigmaColorProfile *linear_gray_profile = NULL;

  if (base_type == LIGMA_GRAY)
    {
      if (trc == LIGMA_TRC_LINEAR)
        {
          if (! linear_gray_profile)
            {
              linear_gray_profile = ligma_color_profile_new_d65_gray_linear ();
              g_object_add_weak_pointer (G_OBJECT (linear_gray_profile),
                                         (gpointer) &linear_gray_profile);
            }

          return linear_gray_profile;
        }
      else
        {
          if (! gray_profile)
            {
              gray_profile = ligma_color_profile_new_d65_gray_srgb_trc ();
              g_object_add_weak_pointer (G_OBJECT (gray_profile),
                                         (gpointer) &gray_profile);
            }

          return gray_profile;
        }
    }
  else
    {
      if (trc == LIGMA_TRC_LINEAR)
        {
          if (! linear_rgb_profile)
            {
              linear_rgb_profile = ligma_color_profile_new_rgb_srgb_linear ();
              g_object_add_weak_pointer (G_OBJECT (linear_rgb_profile),
                                         (gpointer) &linear_rgb_profile);
            }

          return linear_rgb_profile;
        }
      else
        {
          if (! srgb_profile)
            {
              srgb_profile = ligma_color_profile_new_rgb_srgb ();
              g_object_add_weak_pointer (G_OBJECT (srgb_profile),
                                         (gpointer) &srgb_profile);
            }

          return srgb_profile;
        }
    }
}

LigmaColorProfile *
ligma_babl_format_get_color_profile (const Babl *format)
{
  const Babl       *non_linear_format;
  const gchar      *icc;
  gint              icc_len;
  LigmaColorProfile *profile;
  LigmaColorProfile *retval = NULL;

  g_return_val_if_fail (format != NULL, NULL);

  if (ligma_babl_format_get_trc (format) == LIGMA_TRC_NON_LINEAR)
    {
      non_linear_format = format;
    }
  else
    {
      LigmaImageBaseType base_type = LIGMA_RGB;
      LigmaComponentType component_type;

      switch (ligma_babl_format_get_base_type (format))
        {
        case LIGMA_RGB:
        case LIGMA_INDEXED:
          base_type = LIGMA_RGB;
          break;

        case LIGMA_GRAY:
          base_type = LIGMA_GRAY;
          break;
        }

      component_type = ligma_babl_format_get_component_type (format);

      non_linear_format =
        ligma_babl_format (base_type,
                          ligma_babl_precision (component_type,
                                               LIGMA_TRC_NON_LINEAR),
                          babl_format_has_alpha (format),
                          babl_format_get_space (format));
    }

  icc = babl_space_get_icc (babl_format_get_space (non_linear_format),
                            &icc_len);

  profile = ligma_color_profile_new_from_icc_profile ((const guint8 *) icc,
                                                     icc_len, NULL);

  switch (ligma_babl_format_get_trc (format))
    {
    case LIGMA_TRC_LINEAR:
      retval = ligma_color_profile_new_linear_from_color_profile (profile);
      break;

    case LIGMA_TRC_NON_LINEAR:
      retval = g_object_ref (profile);
      break;

    case LIGMA_TRC_PERCEPTUAL:
      retval = ligma_color_profile_new_srgb_trc_from_color_profile (profile);
      break;
    }

  g_object_unref (profile);

  return retval;
}

LigmaImageBaseType
ligma_babl_format_get_base_type (const Babl *format)
{
  const gchar *name;

  g_return_val_if_fail (format != NULL, -1);

  name = babl_get_name (babl_format_get_model (format));

  if (! strcmp (name, "Y")   ||
      ! strcmp (name, "Y'")  ||
      ! strcmp (name, "Y~")  ||
      ! strcmp (name, "YA")  ||
      ! strcmp (name, "Y'A") ||
      ! strcmp (name, "Y~A"))
    {
      return LIGMA_GRAY;
    }
  else if (! strcmp (name, "RGB")        ||
           ! strcmp (name, "R'G'B'")     ||
           ! strcmp (name, "R~G~B~")     ||
           ! strcmp (name, "RGBA")       ||
           ! strcmp (name, "R'G'B'A")    ||
           ! strcmp (name, "R~G~B~A")    ||
           ! strcmp (name, "RaGaBaA")    ||
           ! strcmp (name, "R'aG'aB'aA") ||
           ! strcmp (name, "R~aG~aB~aA"))
    {
      return LIGMA_RGB;
    }
  else if (babl_format_is_palette (format))
    {
      return LIGMA_INDEXED;
    }

  g_return_val_if_reached (-1);
}

LigmaComponentType
ligma_babl_format_get_component_type (const Babl *format)
{
  const Babl *type;

  g_return_val_if_fail (format != NULL, -1);

  type = babl_format_get_type (format, 0);

  if (type == babl_type ("u8"))
    return LIGMA_COMPONENT_TYPE_U8;
  else if (type == babl_type ("u16"))
    return LIGMA_COMPONENT_TYPE_U16;
  else if (type == babl_type ("u32"))
    return LIGMA_COMPONENT_TYPE_U32;
  else if (type == babl_type ("half"))
    return LIGMA_COMPONENT_TYPE_HALF;
  else if (type == babl_type ("float"))
    return LIGMA_COMPONENT_TYPE_FLOAT;
  else if (type == babl_type ("double"))
    return LIGMA_COMPONENT_TYPE_DOUBLE;

  g_return_val_if_reached (-1);
}

LigmaPrecision
ligma_babl_format_get_precision (const Babl *format)
{
  const Babl *type;

  g_return_val_if_fail (format != NULL, -1);

  type = babl_format_get_type (format, 0);

  switch (ligma_babl_format_get_trc (format))
    {
    case LIGMA_TRC_LINEAR:
      if (type == babl_type ("u8"))
        return LIGMA_PRECISION_U8_LINEAR;
      else if (type == babl_type ("u16"))
        return LIGMA_PRECISION_U16_LINEAR;
      else if (type == babl_type ("u32"))
        return LIGMA_PRECISION_U32_LINEAR;
      else if (type == babl_type ("half"))
        return LIGMA_PRECISION_HALF_LINEAR;
      else if (type == babl_type ("float"))
        return LIGMA_PRECISION_FLOAT_LINEAR;
      else if (type == babl_type ("double"))
        return LIGMA_PRECISION_DOUBLE_LINEAR;
      break;

    case LIGMA_TRC_NON_LINEAR:
      if (type == babl_type ("u8"))
        return LIGMA_PRECISION_U8_NON_LINEAR;
      else if (type == babl_type ("u16"))
        return LIGMA_PRECISION_U16_NON_LINEAR;
      else if (type == babl_type ("u32"))
        return LIGMA_PRECISION_U32_NON_LINEAR;
      else if (type == babl_type ("half"))
        return LIGMA_PRECISION_HALF_NON_LINEAR;
      else if (type == babl_type ("float"))
        return LIGMA_PRECISION_FLOAT_NON_LINEAR;
      else if (type == babl_type ("double"))
        return LIGMA_PRECISION_DOUBLE_NON_LINEAR;
      break;

    case LIGMA_TRC_PERCEPTUAL:
      if (type == babl_type ("u8"))
        return LIGMA_PRECISION_U8_PERCEPTUAL;
      else if (type == babl_type ("u16"))
        return LIGMA_PRECISION_U16_PERCEPTUAL;
      else if (type == babl_type ("u32"))
        return LIGMA_PRECISION_U32_PERCEPTUAL;
      else if (type == babl_type ("half"))
        return LIGMA_PRECISION_HALF_PERCEPTUAL;
      else if (type == babl_type ("float"))
        return LIGMA_PRECISION_FLOAT_PERCEPTUAL;
      else if (type == babl_type ("double"))
        return LIGMA_PRECISION_DOUBLE_PERCEPTUAL;
      break;
    }

  g_return_val_if_reached (-1);
}

LigmaTRCType
ligma_babl_format_get_trc (const Babl *format)
{
  const gchar *name;

  g_return_val_if_fail (format != NULL, FALSE);

  name = babl_get_name (babl_format_get_model (format));

  if (! strcmp (name, "Y")    ||
      ! strcmp (name, "YA")   ||
      ! strcmp (name, "RGB")  ||
      ! strcmp (name, "RGBA") ||
      ! strcmp (name, "RaGaBaA"))
    {
      return LIGMA_TRC_LINEAR;
    }
  else if (! strcmp (name, "Y'")      ||
           ! strcmp (name, "Y'A")     ||
           ! strcmp (name, "R'G'B'")  ||
           ! strcmp (name, "R'G'B'A") ||
           ! strcmp (name, "R'aG'aB'aA"))
    {
      return LIGMA_TRC_NON_LINEAR;
    }
  else if (! strcmp (name, "Y~")      ||
           ! strcmp (name, "Y~A")     ||
           ! strcmp (name, "R~G~B~")  ||
           ! strcmp (name, "R~G~B~A") ||
           ! strcmp (name, "R~aG~aB~aA"))
    {
      return LIGMA_TRC_PERCEPTUAL;
    }
  else if (babl_format_is_palette (format))
    {
      return LIGMA_TRC_NON_LINEAR;
    }

  g_return_val_if_reached (LIGMA_TRC_LINEAR);
}

LigmaComponentType
ligma_babl_component_type (LigmaPrecision precision)
{
  switch (precision)
    {
    case LIGMA_PRECISION_U8_LINEAR:
    case LIGMA_PRECISION_U8_NON_LINEAR:
    case LIGMA_PRECISION_U8_PERCEPTUAL:
      return LIGMA_COMPONENT_TYPE_U8;

    case LIGMA_PRECISION_U16_LINEAR:
    case LIGMA_PRECISION_U16_NON_LINEAR:
    case LIGMA_PRECISION_U16_PERCEPTUAL:
      return LIGMA_COMPONENT_TYPE_U16;

    case LIGMA_PRECISION_U32_LINEAR:
    case LIGMA_PRECISION_U32_NON_LINEAR:
    case LIGMA_PRECISION_U32_PERCEPTUAL:
      return LIGMA_COMPONENT_TYPE_U32;

    case LIGMA_PRECISION_HALF_LINEAR:
    case LIGMA_PRECISION_HALF_NON_LINEAR:
    case LIGMA_PRECISION_HALF_PERCEPTUAL:
      return LIGMA_COMPONENT_TYPE_HALF;

    case LIGMA_PRECISION_FLOAT_LINEAR:
    case LIGMA_PRECISION_FLOAT_NON_LINEAR:
    case LIGMA_PRECISION_FLOAT_PERCEPTUAL:
      return LIGMA_COMPONENT_TYPE_FLOAT;

    case LIGMA_PRECISION_DOUBLE_LINEAR:
    case LIGMA_PRECISION_DOUBLE_NON_LINEAR:
    case LIGMA_PRECISION_DOUBLE_PERCEPTUAL:
      return LIGMA_COMPONENT_TYPE_DOUBLE;
    }

  g_return_val_if_reached (-1);
}

LigmaTRCType
ligma_babl_trc (LigmaPrecision precision)
{
  switch (precision)
    {
    case LIGMA_PRECISION_U8_LINEAR:
    case LIGMA_PRECISION_U16_LINEAR:
    case LIGMA_PRECISION_U32_LINEAR:
    case LIGMA_PRECISION_HALF_LINEAR:
    case LIGMA_PRECISION_FLOAT_LINEAR:
    case LIGMA_PRECISION_DOUBLE_LINEAR:
      return LIGMA_TRC_LINEAR;

    case LIGMA_PRECISION_U8_NON_LINEAR:
    case LIGMA_PRECISION_U16_NON_LINEAR:
    case LIGMA_PRECISION_U32_NON_LINEAR:
    case LIGMA_PRECISION_HALF_NON_LINEAR:
    case LIGMA_PRECISION_FLOAT_NON_LINEAR:
    case LIGMA_PRECISION_DOUBLE_NON_LINEAR:
      return LIGMA_TRC_NON_LINEAR;

    case LIGMA_PRECISION_U8_PERCEPTUAL:
    case LIGMA_PRECISION_U16_PERCEPTUAL:
    case LIGMA_PRECISION_U32_PERCEPTUAL:
    case LIGMA_PRECISION_HALF_PERCEPTUAL:
    case LIGMA_PRECISION_FLOAT_PERCEPTUAL:
    case LIGMA_PRECISION_DOUBLE_PERCEPTUAL:
      return LIGMA_TRC_PERCEPTUAL;
    }

  g_return_val_if_reached (LIGMA_TRC_LINEAR);
}

LigmaPrecision
ligma_babl_precision (LigmaComponentType component,
                     LigmaTRCType       trc)
{
  switch (component)
    {
    case LIGMA_COMPONENT_TYPE_U8:
      switch (trc)
        {
        case LIGMA_TRC_LINEAR:     return LIGMA_PRECISION_U8_LINEAR;
        case LIGMA_TRC_NON_LINEAR: return LIGMA_PRECISION_U8_NON_LINEAR;
        case LIGMA_TRC_PERCEPTUAL: return LIGMA_PRECISION_U8_PERCEPTUAL;
        default:
          break;
        }

    case LIGMA_COMPONENT_TYPE_U16:
      switch (trc)
        {
        case LIGMA_TRC_LINEAR:     return LIGMA_PRECISION_U16_LINEAR;
        case LIGMA_TRC_NON_LINEAR: return LIGMA_PRECISION_U16_NON_LINEAR;
        case LIGMA_TRC_PERCEPTUAL: return LIGMA_PRECISION_U16_PERCEPTUAL;
        default:
          break;
        }

    case LIGMA_COMPONENT_TYPE_U32:
      switch (trc)
        {
        case LIGMA_TRC_LINEAR:     return LIGMA_PRECISION_U32_LINEAR;
        case LIGMA_TRC_NON_LINEAR: return LIGMA_PRECISION_U32_NON_LINEAR;
        case LIGMA_TRC_PERCEPTUAL: return LIGMA_PRECISION_U32_PERCEPTUAL;
        default:
          break;
        }

    case LIGMA_COMPONENT_TYPE_HALF:
      switch (trc)
        {
        case LIGMA_TRC_LINEAR:     return LIGMA_PRECISION_HALF_LINEAR;
        case LIGMA_TRC_NON_LINEAR: return LIGMA_PRECISION_HALF_NON_LINEAR;
        case LIGMA_TRC_PERCEPTUAL: return LIGMA_PRECISION_HALF_PERCEPTUAL;
        default:
          break;
        }

    case LIGMA_COMPONENT_TYPE_FLOAT:
      switch (trc)
        {
        case LIGMA_TRC_LINEAR:     return LIGMA_PRECISION_FLOAT_LINEAR;
        case LIGMA_TRC_NON_LINEAR: return LIGMA_PRECISION_FLOAT_NON_LINEAR;
        case LIGMA_TRC_PERCEPTUAL: return LIGMA_PRECISION_FLOAT_PERCEPTUAL;
        default:
          break;
        }

    case LIGMA_COMPONENT_TYPE_DOUBLE:
      switch (trc)
        {
        case LIGMA_TRC_LINEAR:     return LIGMA_PRECISION_DOUBLE_LINEAR;
        case LIGMA_TRC_NON_LINEAR: return LIGMA_PRECISION_DOUBLE_NON_LINEAR;
        case LIGMA_TRC_PERCEPTUAL: return LIGMA_PRECISION_DOUBLE_PERCEPTUAL;
        default:
          break;
        }

    default:
      break;
    }

  g_return_val_if_reached (-1);
}

gboolean
ligma_babl_is_valid (LigmaImageBaseType base_type,
                    LigmaPrecision     precision)
{
  switch (base_type)
    {
    case LIGMA_RGB:
    case LIGMA_GRAY:
      return TRUE;

    case LIGMA_INDEXED:
      switch (precision)
        {
        case LIGMA_PRECISION_U8_NON_LINEAR:
          return TRUE;

        default:
          return FALSE;
        }
    }

  g_return_val_if_reached (FALSE);
}

LigmaComponentType
ligma_babl_is_bounded (LigmaPrecision precision)
{
  switch (ligma_babl_component_type (precision))
    {
    case LIGMA_COMPONENT_TYPE_U8:
    case LIGMA_COMPONENT_TYPE_U16:
    case LIGMA_COMPONENT_TYPE_U32:
      return TRUE;

    case LIGMA_COMPONENT_TYPE_HALF:
    case LIGMA_COMPONENT_TYPE_FLOAT:
    case LIGMA_COMPONENT_TYPE_DOUBLE:
      return FALSE;
    }

  g_return_val_if_reached (FALSE);
}

const Babl *
ligma_babl_format (LigmaImageBaseType  base_type,
                  LigmaPrecision      precision,
                  gboolean           with_alpha,
                  const Babl        *space)
{
  switch (base_type)
    {
    case LIGMA_RGB:
      switch (precision)
        {
        case LIGMA_PRECISION_U8_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("RGBA u8", space);
          else
            return babl_format_with_space ("RGB u8", space);

        case LIGMA_PRECISION_U8_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("R'G'B'A u8", space);
          else
            return babl_format_with_space ("R'G'B' u8", space);

        case LIGMA_PRECISION_U8_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("R~G~B~A u8", space);
          else
            return babl_format_with_space ("R~G~B~ u8", space);

        case LIGMA_PRECISION_U16_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("RGBA u16", space);
          else
            return babl_format_with_space ("RGB u16", space);

        case LIGMA_PRECISION_U16_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("R'G'B'A u16", space);
          else
            return babl_format_with_space ("R'G'B' u16", space);

        case LIGMA_PRECISION_U16_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("R~G~B~A u16", space);
          else
            return babl_format_with_space ("R~G~B~ u16", space);

        case LIGMA_PRECISION_U32_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("RGBA u32", space);
          else
            return babl_format_with_space ("RGB u32", space);

        case LIGMA_PRECISION_U32_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("R'G'B'A u32", space);
          else
            return babl_format_with_space ("R'G'B' u32", space);

        case LIGMA_PRECISION_U32_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("R~G~B~A u32", space);
          else
            return babl_format_with_space ("R~G~B~ u32", space);

        case LIGMA_PRECISION_HALF_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("RGBA half", space);
          else
            return babl_format_with_space ("RGB half", space);

        case LIGMA_PRECISION_HALF_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("R'G'B'A half", space);
          else
            return babl_format_with_space ("R'G'B' half", space);

        case LIGMA_PRECISION_HALF_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("R~G~B~A half", space);
          else
            return babl_format_with_space ("R~G~B~ half", space);

        case LIGMA_PRECISION_FLOAT_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("RGBA float", space);
          else
            return babl_format_with_space ("RGB float", space);

        case LIGMA_PRECISION_FLOAT_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("R'G'B'A float", space);
          else
            return babl_format_with_space ("R'G'B' float", space);

        case LIGMA_PRECISION_FLOAT_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("R~G~B~A float", space);
          else
            return babl_format_with_space ("R~G~B~ float", space);

        case LIGMA_PRECISION_DOUBLE_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("RGBA double", space);
          else
            return babl_format_with_space ("RGB double", space);

        case LIGMA_PRECISION_DOUBLE_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("R'G'B'A double", space);
          else
            return babl_format_with_space ("R'G'B' double", space);

        case LIGMA_PRECISION_DOUBLE_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("R~G~B~A double", space);
          else
            return babl_format_with_space ("R~G~B~ double", space);

        default:
          break;
        }
      break;

    case LIGMA_GRAY:
      switch (precision)
        {
        case LIGMA_PRECISION_U8_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("YA u8", space);
          else
            return babl_format_with_space ("Y u8", space);

        case LIGMA_PRECISION_U8_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("Y'A u8", space);
          else
            return babl_format_with_space ("Y' u8", space);

        case LIGMA_PRECISION_U8_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("Y~A u8", space);
          else
            return babl_format_with_space ("Y~ u8", space);

        case LIGMA_PRECISION_U16_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("YA u16", space);
          else
            return babl_format_with_space ("Y u16", space);

        case LIGMA_PRECISION_U16_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("Y'A u16", space);
          else
            return babl_format_with_space ("Y' u16", space);

        case LIGMA_PRECISION_U16_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("Y~A u16", space);
          else
            return babl_format_with_space ("Y~ u16", space);

        case LIGMA_PRECISION_U32_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("YA u32", space);
          else
            return babl_format_with_space ("Y u32", space);

        case LIGMA_PRECISION_U32_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("Y'A u32", space);
          else
            return babl_format_with_space ("Y' u32", space);

        case LIGMA_PRECISION_U32_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("Y~A u32", space);
          else
            return babl_format_with_space ("Y~ u32", space);

        case LIGMA_PRECISION_HALF_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("YA half", space);
          else
            return babl_format_with_space ("Y half", space);

        case LIGMA_PRECISION_HALF_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("Y'A half", space);
          else
            return babl_format_with_space ("Y' half", space);

        case LIGMA_PRECISION_HALF_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("Y~A half", space);
          else
            return babl_format_with_space ("Y~ half", space);

        case LIGMA_PRECISION_FLOAT_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("YA float", space);
          else
            return babl_format_with_space ("Y float", space);

        case LIGMA_PRECISION_FLOAT_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("Y'A float", space);
          else
            return babl_format_with_space ("Y' float", space);

        case LIGMA_PRECISION_FLOAT_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("Y~A float", space);
          else
            return babl_format_with_space ("Y~ float", space);

        case LIGMA_PRECISION_DOUBLE_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("YA double", space);
          else
            return babl_format_with_space ("Y double", space);

        case LIGMA_PRECISION_DOUBLE_NON_LINEAR:
          if (with_alpha)
            return babl_format_with_space ("Y'A double", space);
          else
            return babl_format_with_space ("Y' double", space);

        case LIGMA_PRECISION_DOUBLE_PERCEPTUAL:
          if (with_alpha)
            return babl_format_with_space ("Y~A double", space);
          else
            return babl_format_with_space ("Y~ double", space);

        default:
          break;
        }
      break;

    case LIGMA_INDEXED:
      /* need to use the image's api for this */
      break;
    }

  g_return_val_if_reached (NULL);
}

const Babl *
ligma_babl_mask_format (LigmaPrecision precision)
{
  switch (ligma_babl_component_type (precision))
    {
    case LIGMA_COMPONENT_TYPE_U8:     return babl_format ("Y u8");
    case LIGMA_COMPONENT_TYPE_U16:    return babl_format ("Y u16");
    case LIGMA_COMPONENT_TYPE_U32:    return babl_format ("Y u32");
    case LIGMA_COMPONENT_TYPE_HALF:   return babl_format ("Y half");
    case LIGMA_COMPONENT_TYPE_FLOAT:  return babl_format ("Y float");
    case LIGMA_COMPONENT_TYPE_DOUBLE: return babl_format ("Y double");
    }

  g_return_val_if_reached (NULL);
}

const Babl *
ligma_babl_component_format (LigmaImageBaseType base_type,
                            LigmaPrecision     precision,
                            gint              index)
{
  switch (base_type)
    {
    case LIGMA_RGB:
      switch (precision)
        {
        case LIGMA_PRECISION_U8_LINEAR:
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

        case LIGMA_PRECISION_U8_NON_LINEAR:
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

        case LIGMA_PRECISION_U8_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("R~ u8");
            case 1: return babl_format ("G~ u8");
            case 2: return babl_format ("B~ u8");
            case 3: return babl_format ("A u8");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_U16_LINEAR:
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

        case LIGMA_PRECISION_U16_NON_LINEAR:
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

        case LIGMA_PRECISION_U16_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("R~ u16");
            case 1: return babl_format ("G~ u16");
            case 2: return babl_format ("B~ u16");
            case 3: return babl_format ("A u16");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_U32_LINEAR:
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

        case LIGMA_PRECISION_U32_NON_LINEAR:
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

        case LIGMA_PRECISION_U32_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("R~ u32");
            case 1: return babl_format ("G~ u32");
            case 2: return babl_format ("B~ u32");
            case 3: return babl_format ("A u32");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_HALF_LINEAR:
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

        case LIGMA_PRECISION_HALF_NON_LINEAR:
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

        case LIGMA_PRECISION_HALF_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("R~ half");
            case 1: return babl_format ("G~ half");
            case 2: return babl_format ("B~ half");
            case 3: return babl_format ("A half");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_FLOAT_LINEAR:
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

        case LIGMA_PRECISION_FLOAT_NON_LINEAR:
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

        case LIGMA_PRECISION_FLOAT_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("R~ float");
            case 1: return babl_format ("G~ float");
            case 2: return babl_format ("B~ float");
            case 3: return babl_format ("A float");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_DOUBLE_LINEAR:
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

        case LIGMA_PRECISION_DOUBLE_NON_LINEAR:
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

        case LIGMA_PRECISION_DOUBLE_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("R~ double");
            case 1: return babl_format ("G~ double");
            case 2: return babl_format ("B~ double");
            case 3: return babl_format ("A double");
            default:
              break;
            }
          break;

        default:
          break;
        }
      break;

    case LIGMA_GRAY:
      switch (precision)
        {
        case LIGMA_PRECISION_U8_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y u8");
            case 1: return babl_format ("A u8");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_U8_NON_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y' u8");
            case 1: return babl_format ("A u8");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_U8_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("Y~ u8");
            case 1: return babl_format ("A u8");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_U16_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y u16");
            case 1: return babl_format ("A u16");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_U16_NON_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y' u16");
            case 1: return babl_format ("A u16");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_U16_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("Y~ u16");
            case 1: return babl_format ("A u16");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_U32_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y u32");
            case 1: return babl_format ("A u32");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_U32_NON_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y' u32");
            case 1: return babl_format ("A u32");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_U32_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("Y~ u32");
            case 1: return babl_format ("A u32");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_HALF_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y half");
            case 1: return babl_format ("A half");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_HALF_NON_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y' half");
            case 1: return babl_format ("A half");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_HALF_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("Y~ half");
            case 1: return babl_format ("A half");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_FLOAT_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y float");
            case 1: return babl_format ("A float");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_FLOAT_NON_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y' float");
            case 1: return babl_format ("A float");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_FLOAT_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("Y~ float");
            case 1: return babl_format ("A float");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_DOUBLE_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y double");
            case 1: return babl_format ("A double");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_DOUBLE_NON_LINEAR:
          switch (index)
            {
            case 0: return babl_format ("Y' double");
            case 1: return babl_format ("A double");
            default:
              break;
            }
          break;

        case LIGMA_PRECISION_DOUBLE_PERCEPTUAL:
          switch (index)
            {
            case 0: return babl_format ("Y~ double");
            case 1: return babl_format ("A double");
            default:
              break;
            }
          break;

        default:
          break;
        }
      break;

    case LIGMA_INDEXED:
      /* need to use the image's api for this */
      break;
    }

  g_return_val_if_reached (NULL);
}

const Babl *
ligma_babl_format_change_component_type (const Babl        *format,
                                        LigmaComponentType  component)
{
  g_return_val_if_fail (format != NULL, NULL);

  return ligma_babl_format (ligma_babl_format_get_base_type (format),
                           ligma_babl_precision (
                             component,
                             ligma_babl_format_get_trc (format)),
                           babl_format_has_alpha (format),
                           babl_format_get_space (format));
}

const Babl *
ligma_babl_format_change_trc (const Babl  *format,
                             LigmaTRCType  trc)
{
  g_return_val_if_fail (format != NULL, NULL);

  return ligma_babl_format (ligma_babl_format_get_base_type (format),
                           ligma_babl_precision (
                             ligma_babl_format_get_component_type (format),
                             trc),
                           babl_format_has_alpha (format),
                           babl_format_get_space (format));
}

gchar **
ligma_babl_print_pixel (const Babl *format,
                       gpointer    pixel)
{
  LigmaPrecision   precision;
  gint            n_components;
  guchar          tmp_pixel[32];
  gchar         **strings;

  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (pixel != NULL, NULL);

  precision = ligma_babl_format_get_precision (format);

  if (babl_format_is_palette (format))
    {
      const Babl *f = ligma_babl_format (LIGMA_RGB, precision,
                                        babl_format_has_alpha (format),
                                        babl_format_get_space (format));

      babl_process (babl_fish (format, f), pixel, tmp_pixel, 1);

      format = f;
      pixel  = tmp_pixel;
    }

  n_components = babl_format_get_n_components (format);

  strings = g_new0 (gchar *, n_components + 1);

  switch (ligma_babl_format_get_component_type (format))
    {
    case LIGMA_COMPONENT_TYPE_U8:
      {
        guchar *color = pixel;
        gint    i;

        for (i = 0; i < n_components; i++)
          strings[i] = g_strdup_printf ("%d", color[i]);
      }
      break;

    case LIGMA_COMPONENT_TYPE_U16:
      {
        guint16 *color = pixel;
        gint     i;

        for (i = 0; i < n_components; i++)
          strings[i] = g_strdup_printf ("%u", color[i]);
      }
      break;

    case LIGMA_COMPONENT_TYPE_U32:
      {
        guint32 *color = pixel;
        gint     i;

        for (i = 0; i < n_components; i++)
          strings[i] = g_strdup_printf ("%u", color[i]);
      }
      break;

    case LIGMA_COMPONENT_TYPE_HALF:
      {
        LigmaPrecision p;
        const Babl   *f;

        p = ligma_babl_precision (LIGMA_COMPONENT_TYPE_FLOAT,
                                 ligma_babl_format_get_trc (format));

        f = ligma_babl_format (ligma_babl_format_get_base_type (format),
                              p,
                              babl_format_has_alpha (format),
                              babl_format_get_space (format));

        babl_process (babl_fish (format, f), pixel, tmp_pixel, 1);

        pixel = tmp_pixel;
      }
      /* fall through */

    case LIGMA_COMPONENT_TYPE_FLOAT:
      {
        gfloat *color = pixel;
        gint    i;

        for (i = 0; i < n_components; i++)
          strings[i] = g_strdup_printf ("%0.6f", color[i]);
      }
      break;

    case LIGMA_COMPONENT_TYPE_DOUBLE:
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
