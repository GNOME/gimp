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

#include "ligmahsl.h"


/*
 * LIGMA_TYPE_HSL
 */

static LigmaHSL * ligma_hsl_copy (const LigmaHSL *hsl);


G_DEFINE_BOXED_TYPE (LigmaHSL, ligma_hsl, ligma_hsl_copy, g_free)

static LigmaHSL *
ligma_hsl_copy (const LigmaHSL *hsl)
{
  return g_memdup2 (hsl, sizeof (LigmaHSL));
}


/*  HSL functions  */

/**
 * ligma_hsl_set:
 * @hsl:
 * @h:
 * @s:
 * @l:
 *
 * Since: 2.8
 **/
void
ligma_hsl_set (LigmaHSL *hsl,
              gdouble  h,
              gdouble  s,
              gdouble  l)
{
  g_return_if_fail (hsl != NULL);

  hsl->h = h;
  hsl->s = s;
  hsl->l = l;
}

/**
 * ligma_hsl_set_alpha:
 * @hsl:
 * @a:
 *
 * Since: 2.10
 **/
void
ligma_hsl_set_alpha (LigmaHSL *hsl,
                    gdouble a)
{
  g_return_if_fail (hsl != NULL);

  hsl->a = a;
}
