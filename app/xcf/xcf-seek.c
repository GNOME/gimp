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

#include <gio/gio.h>

#include "core/core-types.h"

#include "xcf-private.h"
#include "xcf-seek.h"

#include "gimp-intl.h"


gboolean
xcf_seek_pos (XcfInfo  *info,
              goffset   pos,
              GError  **error)
{
  if (info->cp != pos)
    {
      GError *my_error = NULL;

      info->cp = pos;

      if (! g_seekable_seek (info->seekable, info->cp, G_SEEK_SET,
                             NULL, &my_error))
        {
          g_propagate_prefixed_error (error, my_error,
                                      _("Could not seek in XCF file: "));
          return FALSE;
        }

      gimp_assert (info->cp == g_seekable_tell (info->seekable));
    }

  return TRUE;
}
