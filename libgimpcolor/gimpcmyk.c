/* LIBLIGMA - The LIGMA Library
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

#include "libligmamath/ligmamath.h"

#include "ligmacolortypes.h"

#include "ligmacmyk.h"


/**
 * SECTION: ligmacmyk
 * @title: LigmaCMYK
 * @short_description: Definitions and Functions relating to CMYK colors.
 *
 * Definitions and Functions relating to CMYK colors.
 **/


/*
 * LIGMA_TYPE_CMYK
 */

static LigmaCMYK * ligma_cmyk_copy (const LigmaCMYK *cmyk);


G_DEFINE_BOXED_TYPE (LigmaCMYK, ligma_cmyk, ligma_cmyk_copy, g_free)

static LigmaCMYK *
ligma_cmyk_copy (const LigmaCMYK *cmyk)
{
  return g_memdup2 (cmyk, sizeof (LigmaCMYK));
}


/*  CMYK functions  */

/**
 * ligma_cmyk_set:
 * @cmyk:    A #LigmaCMYK structure which will hold the specified CMYK value.
 * @cyan:    The Cyan channel of the CMYK value
 * @magenta: The Magenta channel
 * @yellow:  The Yellow channel
 * @black:   The blacK channel
 *
 * Very basic initialiser for the internal #LigmaCMYK structure. Channel
 * values are doubles in the range 0 to 1.
 **/
void
ligma_cmyk_set (LigmaCMYK *cmyk,
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
 * ligma_cmyk_set_uchar:
 * @cmyk:    A #LigmaCMYK structure which will hold the specified CMYK value.
 * @cyan:    The Cyan channel of the CMYK value
 * @magenta: The Magenta channel
 * @yellow:  The Yellow channel
 * @black:   The blacK channel
 *
 * The same as ligma_cmyk_set(), except that channel values are
 * unsigned chars in the range 0 to 255.
 **/
void
ligma_cmyk_set_uchar (LigmaCMYK *cmyk,
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
 * ligma_cmyk_get_uchar:
 * @cmyk:    A #LigmaCMYK structure which will hold the specified CMYK value.
 * @cyan:    (out) (optional): The Cyan channel of the CMYK value
 * @magenta: (out) (optional): The Magenta channel
 * @yellow:  (out) (optional): The Yellow channel
 * @black:   (out) (optional): The blacK channel
 *
 * Retrieve individual channel values from a #LigmaCMYK structure. Channel
 * values are pointers to unsigned chars in the range 0 to 255.
 **/
void
ligma_cmyk_get_uchar (const LigmaCMYK *cmyk,
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
 * ligma_cmyka_set:
 * @cmyka:   A #LigmaCMYK structure which will hold the specified CMYKA value.
 * @cyan:    The Cyan channel of the CMYK value
 * @magenta: The Magenta channel
 * @yellow:  The Yellow channel
 * @black:   The blacK channel
 * @alpha:   The Alpha channel
 *
 * Initialiser for the internal #LigmaCMYK structure. Channel values are
 * doubles in the range 0 to 1.
 **/
void
ligma_cmyka_set (LigmaCMYK *cmyka,
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
 * ligma_cmyka_set_uchar:
 * @cmyka:   A #LigmaCMYK structure which will hold the specified CMYKA value.
 * @cyan:    The Cyan channel of the CMYK value
 * @magenta: The Magenta channel
 * @yellow:  The Yellow channel
 * @black:   The blacK channel
 * @alpha:   The Alpha channel
 *
 * The same as ligma_cmyka_set(), except that channel values are
 * unsigned chars in the range 0 to 255.
 **/
void
ligma_cmyka_set_uchar (LigmaCMYK *cmyka,
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
 * ligma_cmyka_get_uchar:
 * @cmyka:   A #LigmaCMYK structure which will hold the specified CMYKA value.
 * @cyan:    (out) (optional): The Cyan channel of the CMYK value
 * @magenta: (out) (optional): The Magenta channel
 * @yellow:  (out) (optional): The Yellow channel
 * @black:   (out) (optional): The blacK channel
 * @alpha:   (out) (optional): The Alpha channel
 *
 * Retrieve individual channel values from a #LigmaCMYK structure.
 * Channel values are pointers to unsigned chars in the range 0 to 255.
 **/
void
ligma_cmyka_get_uchar (const LigmaCMYK *cmyka,
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
