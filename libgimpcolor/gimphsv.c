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

#include "ligmacolortypes.h"

#include "ligmahsv.h"


/**
 * SECTION: ligmahsv
 * @title: LigmaHSV
 * @short_description: Definitions and Functions relating to HSV colors.
 *
 * Definitions and Functions relating to HSV colors.
 **/


/*
 * LIGMA_TYPE_HSV
 */

static LigmaHSV * ligma_hsv_copy (const LigmaHSV *hsv);


G_DEFINE_BOXED_TYPE (LigmaHSV, ligma_hsv, ligma_hsv_copy, g_free)

static LigmaHSV *
ligma_hsv_copy (const LigmaHSV *hsv)
{
  return g_memdup2 (hsv, sizeof (LigmaHSV));
}


/*  HSV functions  */

void
ligma_hsv_set (LigmaHSV *hsv,
              gdouble  h,
              gdouble  s,
              gdouble  v)
{
  g_return_if_fail (hsv != NULL);

  hsv->h = h;
  hsv->s = s;
  hsv->v = v;
}

void
ligma_hsv_clamp (LigmaHSV *hsv)
{
  g_return_if_fail (hsv != NULL);

  hsv->h -= (gint) hsv->h;

  if (hsv->h < 0)
    hsv->h += 1.0;

  hsv->s = CLAMP (hsv->s, 0.0, 1.0);
  hsv->v = CLAMP (hsv->v, 0.0, 1.0);
  hsv->a = CLAMP (hsv->a, 0.0, 1.0);
}

void
ligma_hsva_set (LigmaHSV *hsva,
               gdouble  h,
               gdouble  s,
               gdouble  v,
               gdouble  a)
{
  g_return_if_fail (hsva != NULL);

  hsva->h = h;
  hsva->s = s;
  hsva->v = v;
  hsva->a = a;
}
