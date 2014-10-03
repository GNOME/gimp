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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "glib-object.h"

#include "gimp-debug.h"
#include "gimp-log.h"


static const GDebugKey log_keys[] =
{
  { "tool-events",        GIMP_LOG_TOOL_EVENTS        },
  { "tool-focus",         GIMP_LOG_TOOL_FOCUS         },
  { "dnd",                GIMP_LOG_DND                },
  { "help",               GIMP_LOG_HELP               },
  { "dialog-factory",     GIMP_LOG_DIALOG_FACTORY     },
  { "menus",              GIMP_LOG_MENUS              },
  { "save-dialog",        GIMP_LOG_SAVE_DIALOG        },
  { "image-scale",        GIMP_LOG_IMAGE_SCALE        },
  { "shadow-tiles",       GIMP_LOG_SHADOW_TILES       },
  { "scale",              GIMP_LOG_SCALE              },
  { "wm",                 GIMP_LOG_WM                 },
  { "floating-selection", GIMP_LOG_FLOATING_SELECTION },
  { "shm",                GIMP_LOG_SHM                },
  { "text-editing",       GIMP_LOG_TEXT_EDITING       },
  { "key-events",         GIMP_LOG_KEY_EVENTS         },
  { "auto-tab-style",     GIMP_LOG_AUTO_TAB_STYLE     },
  { "instances",          GIMP_LOG_INSTANCES          },
  { "rectangle-tool",     GIMP_LOG_RECTANGLE_TOOL     },
  { "brush-cache",        GIMP_LOG_BRUSH_CACHE        },
  { "projection",         GIMP_LOG_PROJECTION         },
  { "xcf",                GIMP_LOG_XCF                }
};


GimpLogFlags gimp_log_flags = 0;


void
gimp_log_init (void)
{
  const gchar *env_log_val = g_getenv ("GIMP_LOG");

  if (! env_log_val)
    env_log_val = g_getenv ("GIMP_DEBUG");

  if (env_log_val)
    g_setenv ("G_MESSAGES_DEBUG", env_log_val, TRUE);

  if (env_log_val)
    {
      /*  g_parse_debug_string() has special treatment of the string 'help',
       *  but we want to use it for the GIMP_LOG_HELP domain
       */
      if (g_ascii_strcasecmp (env_log_val, "help") == 0)
        gimp_log_flags = GIMP_LOG_HELP;
      else
        gimp_log_flags = g_parse_debug_string (env_log_val,
                                               log_keys,
                                               G_N_ELEMENTS (log_keys));

      if (gimp_log_flags & GIMP_LOG_INSTANCES)
        gimp_debug_enable_instances ();
    }
}

void
gimp_log (GimpLogFlags  flags,
          const gchar  *function,
          gint          line,
          const gchar  *format,
          ...)
{
  va_list args;

  va_start (args, format);
  gimp_logv (flags, function, line, format, args);
  va_end (args);
}

void
gimp_logv (GimpLogFlags  flags,
           const gchar  *function,
           gint          line,
           const gchar  *format,
           va_list       args)
{
  const gchar *domain = "unknown";
  gchar       *message;
  gint         i;

  for (i = 0; i < G_N_ELEMENTS (log_keys); i++)
    if (log_keys[i].value == flags)
      {
        domain = log_keys[i].key;
        break;
      }

  if (format)
    message = g_strdup_vprintf (format, args);
  else
    message = g_strdup ("called");

  g_log (domain, G_LOG_LEVEL_DEBUG,
         "%s(%d): %s", function, line, message);

  g_free (message);
}
