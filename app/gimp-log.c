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

#include "glib-object.h"

#include "gimp-log.h"


GimpLogFlags gimp_log_flags = 0;


void
gimp_log_init (void)
{
  const gchar *env_log_val = g_getenv ("GIMP_LOG");

  if (env_log_val)
    {
      const GDebugKey log_keys[] =
      {
        { "tools",          GIMP_LOG_TOOLS          },
        { "dnd",            GIMP_LOG_DND            },
        { "help",           GIMP_LOG_HELP           },
        { "dialog-factory", GIMP_LOG_DIALOG_FACTORY },
        { "save-dialog",    GIMP_LOG_SAVE_DIALOG    },
        { "image-scale",    GIMP_LOG_IMAGE_SCALE    }
      };

      gimp_log_flags = g_parse_debug_string (env_log_val,
                                             log_keys,
                                             G_N_ELEMENTS (log_keys));
    }
}

void
gimp_log (const gchar *function,
          gint         line,
          const gchar *domain,
          const gchar *format,
          ...)
{
  va_list args;

  va_start (args, format);
  gimp_logv (function, line, domain, format, args);
  va_end (args);
}

void
gimp_logv (const gchar *function,
           gint         line,
           const gchar *domain,
           const gchar *format,
           va_list      args)
{
  gchar *message;

  if (format)
    message = g_strdup_vprintf (format, args);
  else
    message = g_strdup ("called");

  g_log (domain, G_LOG_LEVEL_DEBUG,
         "%s(%d): %s", function, line, message);

  g_free (message);
}
