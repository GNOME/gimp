/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmodule.c
 * (C) 1999 Austin Donnelly <austin@gimp.org>
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

#include "gimpmodule.h"

#include "libgimp/gimpintl.h"


enum
{
  MODIFIED,
  LAST_SIGNAL
};


static void       gimp_module_class_init     (GimpModuleClass *klass);
static void       gimp_module_init           (GimpModule      *module);

static void       gimp_module_finalize       (GObject         *object);

static gboolean   gimp_module_load           (GTypeModule     *module);
static void       gimp_module_unload         (GTypeModule     *module);

static void       gimp_module_set_last_error (GimpModule      *module,
                                              const gchar     *error_str);


static guint module_signals[LAST_SIGNAL];

static GTypeModuleClass *parent_class = NULL;


GType
gimp_module_get_type (void)
{
  static GType module_type = 0;

  if (! module_type)
    {
      static const GTypeInfo module_info =
      {
        sizeof (GimpModuleClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_module_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpModule),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_module_init,
      };

      module_type = g_type_register_static (G_TYPE_TYPE_MODULE,
                                            "GimpModule",
                                            &module_info, 0);
    }

  return module_type;
}

static void
gimp_module_class_init (GimpModuleClass *klass)
{
  GObjectClass     *object_class;
  GTypeModuleClass *module_class;

  object_class = G_OBJECT_CLASS (klass);
  module_class = G_TYPE_MODULE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  module_signals[MODIFIED] =
    g_signal_new ("modified",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpModuleClass, modified),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize = gimp_module_finalize;

  module_class->load     = gimp_module_load;
  module_class->unload   = gimp_module_unload;

  klass->modified        = NULL;
}

static void
gimp_module_init (GimpModule *module)
{
  module->filename          = NULL;
  module->verbose           = FALSE;
  module->state             = GIMP_MODULE_STATE_ERROR;
  module->on_disk           = FALSE;
  module->load_inhibit      = FALSE;

  module->module            = NULL;
  module->info              = NULL;
  module->last_module_error = NULL;

  module->query_module      = NULL;
  module->register_module   = NULL;
}

static void
gimp_module_finalize (GObject *object)
{
  GimpModule *module;

  module = GIMP_MODULE (object);

  if (module->info)
    {
      gimp_module_info_free (module->info);
      module->info = NULL;
    }

  if (module->last_module_error)
    {
      g_free (module->last_module_error);
      module->last_module_error = NULL;
    }

  if (module->filename)
    {
      g_free (module->filename);
      module->filename = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_module_load (GTypeModule *module)
{
  GimpModule           *gimp_module;
  const GimpModuleInfo *info;
  gpointer              symbol;
  gboolean              retval;

  g_return_val_if_fail (GIMP_IS_MODULE (module), FALSE);

  gimp_module = GIMP_MODULE (module);

  g_return_val_if_fail (gimp_module->filename != NULL, FALSE);
  g_return_val_if_fail (gimp_module->module == NULL, FALSE);

  if (gimp_module->verbose)
    g_print (_("Loading module: '%s'\n"), gimp_module->filename);

  gimp_module->module = g_module_open (gimp_module->filename,
                                       G_MODULE_BIND_LAZY);

  if (! gimp_module->module)
    {
      gimp_module->state = GIMP_MODULE_STATE_ERROR;
      gimp_module_set_last_error (gimp_module, g_module_error ());

      if (gimp_module->verbose)
	g_message (_("Module '%s' load error:\n%s"),
                   gimp_module->filename, gimp_module->last_module_error);
      return FALSE;
    }

  /* find the gimp_module_query symbol */
  if (! g_module_symbol (gimp_module->module, "gimp_module_query", &symbol))
    {
      gimp_module->state = GIMP_MODULE_STATE_ERROR;

      gimp_module_set_last_error (gimp_module,
                                  "Missing gimp_module_query() symbol");

      if (gimp_module->verbose)
	g_print (_("Module '%s' load error:\n%s"),
                 gimp_module->filename, gimp_module->last_module_error);
      g_module_close (gimp_module->module);
      gimp_module->module = NULL;
      return FALSE;
    }

  gimp_module->query_module = symbol;

  info = gimp_module->query_module (module);

  if (gimp_module->info)
    {
      gimp_module_info_free (gimp_module->info);
      gimp_module->info = NULL;
    }

  if (! info)
    {
      gimp_module->state = GIMP_MODULE_STATE_ERROR;

      gimp_module_set_last_error (gimp_module,
                                  "gimp_module_query() returned NULL");

      if (gimp_module->verbose)
	g_message (_("Module '%s' load error:\n%s"),
		   gimp_module->filename, gimp_module->last_module_error);
      g_module_close (gimp_module->module);
      gimp_module->module       = NULL;
      gimp_module->query_module = NULL;
      return FALSE;
    }

  gimp_module->info = gimp_module_info_copy (info);

  /* find the gimp_module_register symbol */
  if (! g_module_symbol (gimp_module->module, "gimp_module_register", &symbol))
    {
      gimp_module->state = GIMP_MODULE_STATE_ERROR;

      gimp_module_set_last_error (gimp_module,
                                  "Missing gimp_module_register() symbol");

      if (gimp_module->verbose)
	g_message (_("Module '%s' load error:\n%s"),
		   gimp_module->filename, gimp_module->last_module_error);
      g_module_close (gimp_module->module);
      gimp_module->module = NULL;
      return FALSE;
    }

  gimp_module->register_module = symbol;

  retval = gimp_module->register_module (module);

  if (retval)
    gimp_module->state = GIMP_MODULE_STATE_LOADED_OK;
  else
    gimp_module->state = GIMP_MODULE_STATE_LOAD_FAILED;

  return retval;
}

static void
gimp_module_unload (GTypeModule *module)
{
  GimpModule *gimp_module;

  g_return_if_fail (GIMP_IS_MODULE (module));

  gimp_module = GIMP_MODULE (module);

  g_return_if_fail (gimp_module->module != NULL);

  g_module_close (gimp_module->module); /* FIXME: error handling */
  gimp_module->module          = NULL;
  gimp_module->query_module    = NULL;
  gimp_module->register_module = NULL;

  gimp_module->state = GIMP_MODULE_STATE_UNLOADED_OK;
}

GimpModule *
gimp_module_new (const gchar *filename,
                 const gchar *inhibit_list,
                 gboolean     verbose)
{
  GimpModule *module;

  g_return_val_if_fail (filename != NULL, NULL);

  module = g_object_new (GIMP_TYPE_MODULE, NULL);

  module->filename = g_strdup (filename);
  module->verbose  = verbose ? TRUE : FALSE;
  module->on_disk  = TRUE;

  gimp_module_set_load_inhibit (module, inhibit_list);

  if (! module->load_inhibit)
    {
      if (gimp_module_load (G_TYPE_MODULE (module)))
        gimp_module_unload (G_TYPE_MODULE (module));
    }
  else
    {
      if (verbose)
	g_print (_("Skipping module: '%s'\n"), filename);

      module->state = GIMP_MODULE_STATE_UNLOADED_OK;
    }

  return module;
}

void
gimp_module_modified (GimpModule *module)
{
  g_return_if_fail (GIMP_IS_MODULE (module));

  g_signal_emit (G_OBJECT (module), module_signals[MODIFIED], 0);
}

void
gimp_module_set_load_inhibit (GimpModule  *module,
                              const gchar *inhibit_list)
{
  gchar       *p;
  gint         pathlen;
  const gchar *start;
  const gchar *end;

  g_return_if_fail (GIMP_IS_MODULE (module));
  g_return_if_fail (module->filename != NULL);

  module->load_inhibit = FALSE;

  if (! inhibit_list || ! strlen (inhibit_list))
    return;

  p = strstr (inhibit_list, module->filename);
  if (!p)
    return;

  /* we have a substring, but check for colons either side */
  start = p;
  while (start != inhibit_list && *start != G_SEARCHPATH_SEPARATOR)
    start--;

  if (*start == G_SEARCHPATH_SEPARATOR)
    start++;

  end = strchr (p, G_SEARCHPATH_SEPARATOR);
  if (! end)
    end = inhibit_list + strlen (inhibit_list);

  pathlen = strlen (module->filename);

  if ((end - start) == pathlen)
    module->load_inhibit = TRUE;
}

static void
gimp_module_set_last_error (GimpModule  *module,
                            const gchar *error_str)
{
  if (module->last_module_error)
    g_free (module->last_module_error);

  module->last_module_error = g_strdup (error_str);
}


/*  GimpModuleInfo functions  */

GimpModuleInfo *
gimp_module_info_new (const gchar    *purpose,
                      const gchar    *author,
                      const gchar    *version,
                      const gchar    *copyright,
                      const gchar    *date)
{
  GimpModuleInfo *info;

  info = g_new0 (GimpModuleInfo, 1);

  info->purpose   = g_strdup (purpose);
  info->author    = g_strdup (author);
  info->version   = g_strdup (version);
  info->copyright = g_strdup (copyright);
  info->date      = g_strdup (date);

  return info;
}

GimpModuleInfo *
gimp_module_info_copy (const GimpModuleInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  return gimp_module_info_new (info->purpose,
                               info->author,
                               info->version,
                               info->copyright,
                               info->date);
}

void
gimp_module_info_free (GimpModuleInfo *info)
{
  g_return_if_fail (info != NULL);

  g_free (info->purpose);
  g_free (info->author);
  g_free (info->version);
  g_free (info->copyright);
  g_free (info->date);

  g_free (info);
}
