/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>
#include <errno.h>

#include <glib-object.h>

#include "core/core-types.h"

#include "xcf-private.h"
#include "xcf-seek.h"

#include "gimp-intl.h"

gboolean
xcf_seek_pos (XcfInfo  *info,
              guint     pos,
              GError  **error)
{
  if (info->cp != pos)
    {
      info->cp = pos;
      if (fseek (info->fp, info->cp, SEEK_SET) == -1)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Could not seek in XCF file: %s"),
                       g_strerror (errno));

          return FALSE;
        }
    }

  return TRUE;
}

gboolean
xcf_seek_end (XcfInfo  *info,
              GError  **error)
{
  if (fseek (info->fp, 0, SEEK_END) == -1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not seek in XCF file: %s"),
                   g_strerror (errno));

      return FALSE;
    }

  info->cp = ftell (info->fp);

  if (fseek (info->fp, 0, SEEK_END) == -1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not seek in XCF file: %s"),
                   g_strerror (errno));

      return FALSE;
    }

  return TRUE;
}
