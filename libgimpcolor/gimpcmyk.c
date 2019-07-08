/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "gimpcolortypes.h"

#include "gimpcmyk.h"


/**
 * SECTION: gimpcmyk
 * @title: GimpCMYK
 * @short_description: Definitions and Functions relating to CMYK colors.
 *
 * Definitions and Functions relating to CMYK colors.
 **/


/*
 * GIMP_TYPE_CMYK
 */

static GimpCMYK * gimp_cmyk_copy (const GimpCMYK *cmyk);


GType
gimp_cmyk_get_type (void)
{
  static GType cmyk_type = 0;

  if (!cmyk_type)
    cmyk_type = g_boxed_type_register_static ("GimpCMYK",
                                              (GBoxedCopyFunc) gimp_cmyk_copy,
                                              (GBoxedFreeFunc) g_free);

  return cmyk_type;
}

static GimpCMYK *
gimp_cmyk_copy (const GimpCMYK *cmyk)
{
  return g_memdup (cmyk, sizeof (GimpCMYK));
}


/*  CMYK functions  */

/**
 * gimp_cmyk_set:
 * @cmyk:    A #GimpCMYK structure which will hold the specified CMYK value.
 * @cyan:    The Cyan channel of the CMYK value
 * @magenta: The Magenta channel
 * @yellow:  The Yellow channel
 * @black:   The blacK channel
 *
 * Very basic initialiser for the internal #GimpCMYK structure. Channel
 * values are doubles in the range 0 to 1.
 **/
void
gimp_cmyk_set (GimpCMYK *cmyk,
               gdouble   cyan,
               gdouble   magenta,
               gdouble   yellow,
               gdouble   black)
{
  g_return_if_fail (cmyk != NULL);

  cmyk->c = cyan;
  cmyk->m = magenta;
  cmyk->y = yellow;
  cmyk->k = black;
}

/**
 * gimp_cmyk_set_uchar:
 * @cmyk:    A #GimpCMYK structure which will hold the specified CMYK value.
 * @cyan:    The Cyan channel of the CMYK value
 * @magenta: The Magenta channel
 * @yellow:  The Yellow channel
 * @black:   The blacK channel
 *
 * The same as gimp_cmyk_set(), except that channel values are
 * unsigned chars in the range 0 to 255.
 **/
void
gimp_cmyk_set_uchar (GimpCMYK *cmyk,
                     guchar    cyan,
                     guchar    magenta,
                     guchar    yellow,
                     guchar    black)
{
  g_return_if_fail (cmyk != NULL);

  cmyk->c = (gdouble) cyan    / 255.0;
  cmyk->m = (gdouble) magenta / 255.0;
  cmyk->y = (gdouble) yellow  / 255.0;
  cmyk->k = (gdouble) black   / 255.0;
}

/**
 * gimp_cmyk_get_uchar:
 * @cmyk:    A #GimpCMYK structure which will hold the specified CMYK value.
 * @cyan:    The Cyan channel of the CMYK value
 * @magenta: The Magenta channel
 * @yellow:  The Yellow channel
 * @black:   The blacK channel
 *
 * Retrieve individual channel values from a #GimpCMYK structure. Channel
 * values are pointers to unsigned chars in the range 0 to 255.
 **/
void
gimp_cmyk_get_uchar (const GimpCMYK *cmyk,
                     guchar         *cyan,
                     guchar         *magenta,
                     guchar         *yellow,
                     guchar         *black)
{
  g_return_if_fail (cmyk != NULL);

  if (cyan)    *cyan    = ROUND (CLAMP (cmyk->c, 0.0, 1.0) * 255.0);
  if (magenta) *magenta = ROUND (CLAMP (cmyk->m, 0.0, 1.0) * 255.0);
  if (yellow)  *yellow  = ROUND (CLAMP (cmyk->y, 0.0, 1.0) * 255.0);
  if (black)   *black   = ROUND (CLAMP (cmyk->k, 0.0, 1.0) * 255.0);
}


/*  CMYKA functions  */

/**
 * gimp_cmyka_set:
 * @cmyka:   A #GimpCMYK structure which will hold the specified CMYKA value.
 * @cyan:    The Cyan channel of the CMYK value
 * @magenta: The Magenta channel
 * @yellow:  The Yellow channel
 * @black:   The blacK channel
 * @alpha:   The Alpha channel
 *
 * Initialiser for the internal #GimpCMYK structure. Channel values are
 * doubles in the range 0 to 1.
 **/
void
gimp_cmyka_set (GimpCMYK *cmyka,
                gdouble   cyan,
                gdouble   magenta,
                gdouble   yellow,
                gdouble   black,
                gdouble   alpha)
{
  g_return_if_fail (cmyka != NULL);

  cmyka->c = cyan;
  cmyka->m = magenta;
  cmyka->y = yellow;
  cmyka->k = black;
  cmyka->a = alpha;
}

/**
 * gimp_cmyka_set_uchar:
 * @cmyka:   A #GimpCMYK structure which will hold the specified CMYKA value.
 * @cyan:    The Cyan channel of the CMYK value
 * @magenta: The Magenta channel
 * @yellow:  The Yellow channel
 * @black:   The blacK channel
 * @alpha:   The Alpha channel
 *
 * The same as gimp_cmyka_set(), except that channel values are
 * unsigned chars in the range 0 to 255.
 **/
void
gimp_cmyka_set_uchar (GimpCMYK *cmyka,
                      guchar    cyan,
                      guchar    magenta,
                      guchar    yellow,
                      guchar    black,
                      guchar    alpha)
{
  g_return_if_fail (cmyka != NULL);

  cmyka->c = (gdouble) cyan    / 255.0;
  cmyka->m = (gdouble) magenta / 255.0;
  cmyka->y = (gdouble) yellow  / 255.0;
  cmyka->k = (gdouble) black   / 255.0;
  cmyka->a = (gdouble) alpha   / 255.0;
}
/**
 * gimp_cmyka_get_uchar:
 * @cmyka:   A #GimpCMYK structure which will hold the specified CMYKA value.
 * @cyan:    The Cyan channel of the CMYK value
 * @magenta: The Magenta channel
 * @yellow:  The Yellow channel
 * @black:   The blacK channel
 * @alpha:   The Alpha channel
 *
 * Retrieve individual channel values from a #GimpCMYK structure.
 * Channel values are pointers to unsigned chars in the range 0 to 255.
 **/
void
gimp_cmyka_get_uchar (const GimpCMYK *cmyka,
                      guchar         *cyan,
                      guchar         *magenta,
                      guchar         *yellow,
                      guchar         *black,
                      guchar         *alpha)
{
  g_return_if_fail (cmyka != NULL);

  if (cyan)    *cyan    = ROUND (CLAMP (cmyka->c, 0.0, 1.0) * 255.0);
  if (magenta) *magenta = ROUND (CLAMP (cmyka->m, 0.0, 1.0) * 255.0);
  if (yellow)  *yellow  = ROUND (CLAMP (cmyka->y, 0.0, 1.0) * 255.0);
  if (black)   *black   = ROUND (CLAMP (cmyka->k, 0.0, 1.0) * 255.0);
  if (alpha)   *alpha   = ROUND (CLAMP (cmyka->a, 0.0, 1.0) * 255.0);
}
