/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2009 Martin Nordholts
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

#include <glib-object.h>

#include "core/core-types.h"

#include "base/base.h"

#include "config/gimpbaseconfig.h"

#include "core/gimp.h"

#include "tests.h"
#include "units.h"


/**
 * gimp_init_for_testing:
 *
 * Initialize the GIMP object system for unit testing. This is a
 * selected subset of the initialization happning in app_run().
 **/
Gimp *
gimp_init_for_testing (gboolean use_cpu_accel)
{
  Gimp *gimp = gimp_new ("Unit Tested GIMP", NULL, FALSE, TRUE, TRUE, TRUE,
                         FALSE, TRUE, TRUE, FALSE);

  units_init (gimp);

  gimp_load_config (gimp, NULL, NULL);

  base_init (GIMP_BASE_CONFIG (gimp->config), FALSE, use_cpu_accel);

  return gimp;
}
