/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugindebug.c
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

#include <string.h>

#include <glib-object.h>

#include "plug-in-types.h"

#include "ligmaplugindebug.h"


struct _LigmaPlugInDebug
{
  gchar  *name;
  guint   flags;
  gchar **args;
};


static const GDebugKey ligma_debug_wrap_keys[] =
{
  { "query", LIGMA_DEBUG_WRAP_QUERY   },
  { "init",  LIGMA_DEBUG_WRAP_INIT    },
  { "run",   LIGMA_DEBUG_WRAP_RUN     },
  { "on",    LIGMA_DEBUG_WRAP_DEFAULT }
};


LigmaPlugInDebug *
ligma_plug_in_debug_new (void)
{
  LigmaPlugInDebug  *debug;
  const gchar      *wrap, *wrapper;
  gchar            *debug_string;
  gchar           **args;
  GError           *error = NULL;

  wrap = g_getenv ("LIGMA_PLUGIN_DEBUG_WRAP");
  wrapper = g_getenv ("LIGMA_PLUGIN_DEBUG_WRAPPER");

  if (!(wrap && wrapper))
    return NULL;

  if (!g_shell_parse_argv (wrapper, NULL, &args, &error))
    {
      g_warning ("Unable to parse debug wrapper: \"%s\"\n%s",
                 wrapper, error->message);
      g_error_free (error);
      return NULL;
    }

  debug = g_slice_new (LigmaPlugInDebug);

  debug->args  = args;

  debug_string = strchr (wrap, ',');

  if (debug_string)
    {
      debug->name = g_strndup (wrap, debug_string - wrap);
      debug->flags = g_parse_debug_string (debug_string + 1,
                                           ligma_debug_wrap_keys,
                                           G_N_ELEMENTS (ligma_debug_wrap_keys));
    }
  else
    {
      debug->name = g_strdup (wrap);
      debug->flags = LIGMA_DEBUG_WRAP_DEFAULT;
    }

  return debug;
}

void
ligma_plug_in_debug_free (LigmaPlugInDebug *debug)
{
  g_return_if_fail (debug != NULL);

  if (debug->name)
    g_free (debug->name);

  if (debug->args)
    g_strfreev (debug->args);

  g_slice_free (LigmaPlugInDebug, debug);
}

gchar **
ligma_plug_in_debug_argv (LigmaPlugInDebug    *debug,
                         const gchar        *name,
                         LigmaDebugWrapFlag   flag,
                         const gchar       **args)
{
  GPtrArray  *argv;
  gchar     **arg;
  gchar      *basename;

  g_return_val_if_fail (debug != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (args != NULL, NULL);

  basename = g_path_get_basename (name);

  if (!(debug->flags & flag) || (strcmp (debug->name, basename) != 0))
    {
      g_free (basename);
      return NULL;
    }

  g_free (basename);

  argv = g_ptr_array_sized_new (8);

  for (arg = debug->args; *arg != NULL; arg++)
    g_ptr_array_add (argv, *arg);

  for (arg = (gchar **) args; *arg != NULL; arg++)
    g_ptr_array_add (argv, *arg);

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}
