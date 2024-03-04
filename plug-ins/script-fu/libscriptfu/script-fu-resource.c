/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <libgimp/gimp.h>

#include "script-fu-types.h"
#include "script-fu-resource.h"

/* This encapsulates the implementation of the SFResourceType.
 * It knows how to manipulate a SFArg of that type.
 *
 * Separate because it is likely to change.
 *
 * Likely to change:
 *  - default and resettable
 */


/* Set is at registration time.
 * The name may be invalid.
 * We can't then check for validity because Gimp is not done initializing resources.
 * Checking for validity must be done later.
 */

void
sf_resource_set_default (SFResourceType *arg_value, gchar * name_of_default)
{
  /* Store the name and later put to ParamSpecResource.name_of_default.*/
  /* FIXME */
  /* like .default_value.name_of_default = g_strdup (sc->vptr->string_value (default_spec));*/

  /* Not store the resource value.
   * We can't look up resource by name now, at registration time,
   * because Gimp is not done initializing Resources.
   *
   * Instead, set the default to the "invalid ID" value, -1
   */
  /* FIXME arg->default_value.sfa_resource.history = -1; */
}

/* Return a default value.
 * Ensure the value is acceptable by a ResourceChooser widget.
 * The value can be NULL, when the name_of_default is invalid.
 * A NULL will make a ResourceChooser get from context.
 */
GimpResource*
sf_resource_get_default (SFResourceType *arg_value)
{
  /* FIXME, NULL since default not implemented. */
  return NULL;
}

gchar *
sf_arg_get_name_of_default (SFArg  *arg)
{
  /* Return the name stored at registration time. */
  /* FIXME */
  return "Default";
}
