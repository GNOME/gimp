/* The GIMP -- an image manipulation program
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

#include <string.h>

#include <glib-object.h>

#include "core/core-types.h"

#include "core/gimp.h"

#include "plug-in-debug.h"


struct _GimpPlugInDebug
{
  gchar  *name;
  guint   flags;
  gchar **args;
};


static const GDebugKey gimp_debug_wrap_keys[] = {
  {"query", GIMP_DEBUG_WRAP_QUERY},
  {"init",  GIMP_DEBUG_WRAP_INIT},
  {"run",   GIMP_DEBUG_WRAP_RUN},
  {"on",    GIMP_DEBUG_WRAP_DEFAULT}
};


void
plug_in_debug_init (Gimp *gimp)
{
  GimpPlugInDebug  *dbg;
  const gchar      *wrap, *wrapper;
  gchar            *debug_string;
  gchar           **args;
  GError           *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  wrap = g_getenv ("GIMP_PLUGIN_DEBUG_WRAP");
  wrapper = g_getenv ("GIMP_PLUGIN_DEBUG_WRAPPER");

  if (!(wrap && wrapper))
    return;

  if (!g_shell_parse_argv (wrapper, NULL, &args, &error))
    {
      g_warning ("Unable to parse debug wrapper: \"%s\"\n%s",
                 wrapper, error->message);
      g_error_free (error);
      return;
    }

  dbg = g_new (GimpPlugInDebug, 1);

  dbg->args  = args;

  debug_string = strchr (wrap, ',');

  if (debug_string)
    {
      dbg->name = g_strndup (wrap, debug_string - wrap);
      dbg->flags = g_parse_debug_string (debug_string + 1,
                                         gimp_debug_wrap_keys,
                                         G_N_ELEMENTS (gimp_debug_wrap_keys));
    }
  else
    {
      dbg->name = g_strdup (wrap);
      dbg->flags = GIMP_DEBUG_WRAP_DEFAULT;
    }

  gimp->plug_in_debug = dbg;
}

void
plug_in_debug_exit (Gimp *gimp)
{
  GimpPlugInDebug *dbg;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  dbg = gimp->plug_in_debug;

  if (dbg == NULL)
    return;

  if (dbg->name)
    g_free (dbg->name);

  if (dbg->args)
    g_strfreev (dbg->args);

  g_free (dbg);

  gimp->plug_in_debug = NULL;
}

gchar **
plug_in_debug_argv (Gimp               *gimp,
		    const gchar        *name,
                    GimpDebugWrapFlag   flag,
                    gchar             **args)
{
  GimpPlugInDebug  *dbg;
  GPtrArray        *argv;
  gchar           **arg;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (gimp->plug_in_debug != NULL, NULL);
  g_return_val_if_fail (args != NULL, NULL);

  dbg = gimp->plug_in_debug;

  if (!(dbg->flags & flag) || (strcmp (dbg->name, name) != 0))
    return NULL;

  argv = g_ptr_array_sized_new (8);

  for (arg = gimp->plug_in_debug->args; *arg != NULL; arg++)
    g_ptr_array_add (argv, *arg);

  for (arg = args; *arg != NULL; arg++)
    g_ptr_array_add (argv, *arg);

  g_ptr_array_add (argv, NULL);

  return (gchar **) g_ptr_array_free (argv, FALSE);
}
