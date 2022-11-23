/* LIGMA - The GNU Image Manipulation Program
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

#include "glib-object.h"

#include "ligma-debug.h"
#include "ligma-log.h"


static const GDebugKey log_keys[] =
{
  { "tool-events",        LIGMA_LOG_TOOL_EVENTS        },
  { "tool-focus",         LIGMA_LOG_TOOL_FOCUS         },
  { "dnd",                LIGMA_LOG_DND                },
  { "help",               LIGMA_LOG_HELP               },
  { "dialog-factory",     LIGMA_LOG_DIALOG_FACTORY     },
  { "menus",              LIGMA_LOG_MENUS              },
  { "save-dialog",        LIGMA_LOG_SAVE_DIALOG        },
  { "image-scale",        LIGMA_LOG_IMAGE_SCALE        },
  { "shadow-tiles",       LIGMA_LOG_SHADOW_TILES       },
  { "scale",              LIGMA_LOG_SCALE              },
  { "wm",                 LIGMA_LOG_WM                 },
  { "floating-selection", LIGMA_LOG_FLOATING_SELECTION },
  { "shm",                LIGMA_LOG_SHM                },
  { "text-editing",       LIGMA_LOG_TEXT_EDITING       },
  { "key-events",         LIGMA_LOG_KEY_EVENTS         },
  { "auto-tab-style",     LIGMA_LOG_AUTO_TAB_STYLE     },
  { "instances",          LIGMA_LOG_INSTANCES          },
  { "rectangle-tool",     LIGMA_LOG_RECTANGLE_TOOL     },
  { "brush-cache",        LIGMA_LOG_BRUSH_CACHE        },
  { "projection",         LIGMA_LOG_PROJECTION         },
  { "xcf",                LIGMA_LOG_XCF                }
};

static const gchar * const log_domains[] =
{
  "Ligma",
  "Ligma-Actions",
  "Ligma-Base",
  "Ligma-Composite",
  "Ligma-Config",
  "Ligma-Core",
  "Ligma-Dialogs",
  "Ligma-Display",
  "Ligma-File",
  "Ligma-GEGL",
  "Ligma-GUI",
  "Ligma-Menus",
  "Ligma-Operations",
  "Ligma-PDB",
  "Ligma-Paint",
  "Ligma-Paint-Funcs",
  "Ligma-Plug-In",
  "Ligma-Text",
  "Ligma-Tools",
  "Ligma-Vectors",
  "Ligma-Widgets",
  "Ligma-XCF",
  "LibLigmaBase",
  "LibLigmaColor",
  "LibLigmaConfig",
  "LibLigmaMath",
  "LibLigmaModule",
  "LibLigmaThumb",
  "LibLigmaWidgets",
  "GEGL",
  NULL
};


LigmaLogFlags ligma_log_flags = 0;


void
ligma_log_init (void)
{
  const gchar *env_log_val = g_getenv ("LIGMA_LOG");

  if (! env_log_val)
    env_log_val = g_getenv ("LIGMA_DEBUG");

  if (env_log_val)
    g_setenv ("G_MESSAGES_DEBUG", env_log_val, TRUE);

  if (env_log_val)
    {
      /*  g_parse_debug_string() has special treatment of the string 'help',
       *  but we want to use it for the LIGMA_LOG_HELP domain. "list-all"
       *  is a replacement for "help" in LIGMA.
       */
      if (g_ascii_strcasecmp (env_log_val, "list-all") == 0)
        ligma_log_flags = g_parse_debug_string ("help",
                                               log_keys,
                                               G_N_ELEMENTS (log_keys));
      else if (g_ascii_strcasecmp (env_log_val, "help") == 0)
        ligma_log_flags = LIGMA_LOG_HELP;
      else
        ligma_log_flags = g_parse_debug_string (env_log_val,
                                               log_keys,
                                               G_N_ELEMENTS (log_keys));

      if (ligma_log_flags & LIGMA_LOG_INSTANCES)
        {
          ligma_debug_enable_instances ();
        }
      else if (! ligma_log_flags)
        {
          /* If the environment variable was set but no log flags are
           * set as a result, let's assume one is not sure how to use
           * the log flags and output the list of keys as a helper.
           */
          ligma_log_flags = g_parse_debug_string ("help",
                                                 log_keys,
                                                 G_N_ELEMENTS (log_keys));
        }
    }
}

void
ligma_log (LigmaLogFlags  flags,
          const gchar  *function,
          gint          line,
          const gchar  *format,
          ...)
{
  va_list args;

  va_start (args, format);
  ligma_logv (flags, function, line, format, args);
  va_end (args);
}

void
ligma_logv (LigmaLogFlags  flags,
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

LigmaLogHandler
ligma_log_set_handler (gboolean       global,
                      GLogLevelFlags log_levels,
                      GLogFunc       log_func,
                      gpointer       user_data)
{
  LigmaLogHandler handler;
  gint           n;
  gint           i;

  g_return_val_if_fail (log_func != NULL, NULL);

  n = G_N_ELEMENTS (log_domains) - (global ? 1 : 0);

  handler = g_new (guint, n + 1);

  handler[0] = n;

  for (i = 0; i < n; i++)
    {
      handler[i + 1] = g_log_set_handler (log_domains[i], log_levels,
                                          log_func, user_data);
    }

  return handler;
}

void
ligma_log_remove_handler (LigmaLogHandler handler)
{
  gint n;
  gint i;

  g_return_if_fail (handler != NULL);

  n = handler[0];

  for (i = 0; i < n; i++)
    g_log_remove_handler (log_domains[i], handler[i + 1]);

  g_free (handler);
}
