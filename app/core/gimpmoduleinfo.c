/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmoduleinfo.c
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

#include "libgimp/gimpmodule.h"

#include "core-types.h"

#include "gimpmarshal.h"
#include "gimpmoduleinfo.h"

#include "libgimp/gimpintl.h"


enum
{
  MODIFIED,
  LAST_SIGNAL
};


static void       gimp_module_info_class_init (GimpModuleInfoObjClass *klass);
static void       gimp_module_info_init       (GimpModuleInfoObj      *mod);

static void       gimp_module_info_finalize       (GObject           *object);

static gboolean   gimp_module_info_load           (GTypeModule       *module);
static void       gimp_module_info_unload         (GTypeModule       *module);

static void       gimp_module_info_set_last_error (GimpModuleInfoObj *module_info,
                                                   const gchar       *error_str);


static guint module_info_signals[LAST_SIGNAL];

static GTypeModuleClass *parent_class = NULL;


GType
gimp_module_info_get_type (void)
{
  static GType module_info_type = 0;

  if (! module_info_type)
    {
      static const GTypeInfo module_info_info =
      {
        sizeof (GimpModuleInfoObjClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_module_info_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpModuleInfoObj),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_module_info_init,
      };

      module_info_type = g_type_register_static (G_TYPE_TYPE_MODULE,
						 "GimpModuleInfoObj",
						 &module_info_info, 0);
    }

  return module_info_type;
}

static void
gimp_module_info_class_init (GimpModuleInfoObjClass *klass)
{
  GObjectClass     *object_class;
  GTypeModuleClass *module_class;

  object_class = G_OBJECT_CLASS (klass);
  module_class = G_TYPE_MODULE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  module_info_signals[MODIFIED] =
    g_signal_new ("modified",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpModuleInfoObjClass, modified),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize = gimp_module_info_finalize;

  module_class->load     = gimp_module_info_load;
  module_class->unload   = gimp_module_info_unload;

  klass->modified        = NULL;
}

static void
gimp_module_info_init (GimpModuleInfoObj *module_info)
{
  module_info->filename          = NULL;
  module_info->verbose           = FALSE;
  module_info->state             = GIMP_MODULE_STATE_ERROR;
  module_info->on_disk           = FALSE;
  module_info->load_inhibit      = FALSE;

  module_info->module            = NULL;
  module_info->last_module_error = NULL;

  module_info->register_module   = NULL;
}

static void
gimp_module_info_finalize (GObject *object)
{
  GimpModuleInfoObj *mod;

  mod = GIMP_MODULE_INFO (object);

  if (mod->last_module_error)
    {
      g_free (mod->last_module_error);
      mod->last_module_error = NULL;
    }

  if (mod->filename)
    {
      g_free (mod->filename);
      mod->filename = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_module_info_load (GTypeModule *module)
{
  GimpModuleInfoObj    *module_info;
  const GimpModuleInfo *info;
  gpointer              symbol;
  gboolean              retval;

  g_return_val_if_fail (GIMP_IS_MODULE_INFO (module), FALSE);

  module_info = GIMP_MODULE_INFO (module);

  g_return_val_if_fail (module_info->filename != NULL, FALSE);
  g_return_val_if_fail (module_info->module == NULL, FALSE);

  if (module_info->verbose)
    g_print (_("loading module: '%s'\n"), module_info->filename);

  module_info->module = g_module_open (module_info->filename,
                                       G_MODULE_BIND_LAZY);

  if (! module_info->module)
    {
      module_info->state = GIMP_MODULE_STATE_ERROR;
      gimp_module_info_set_last_error (module_info, g_module_error ());

      if (module_info->verbose)
	g_message (_("Module '%s' load error:\n%s"),
		   module_info->filename, module_info->last_module_error);
      return FALSE;
    }

  /* find the gimp_module_query symbol */
  if (! g_module_symbol (module_info->module, "gimp_module_query", &symbol))
    {
      module_info->state = GIMP_MODULE_STATE_ERROR;

      gimp_module_info_set_last_error (module_info,
                                       _("Missing gimp_module_query() symbol"));

      if (module_info->verbose)
	g_message (_("Module '%s' load error:\n%s"),
		   module_info->filename, module_info->last_module_error);
      g_module_close (module_info->module);
      module_info->module = NULL;
      return FALSE;
    }

  module_info->query_module = symbol;

  info = module_info->query_module (module);

  if (! info)
    {
      module_info->state = GIMP_MODULE_STATE_ERROR;

      gimp_module_info_set_last_error (module_info,
                                       _("gimp_module_query() returned NULL"));

      if (module_info->verbose)
	g_message (_("Module '%s' load error:\n%s"),
		   module_info->filename, module_info->last_module_error);
      g_module_close (module_info->module);
      module_info->module       = NULL;
      module_info->query_module = NULL;
      return FALSE;
    }

  g_free ((gchar *) module_info->info.purpose);
  module_info->info.purpose = g_strdup (info->purpose);

  g_free ((gchar *) module_info->info.author);
  module_info->info.author = g_strdup (info->author);

  g_free ((gchar *) module_info->info.version);
  module_info->info.version = g_strdup (info->version);

  g_free ((gchar *) module_info->info.copyright);
  module_info->info.copyright = g_strdup (info->copyright);

  g_free ((gchar *) module_info->info.date);
  module_info->info.date = g_strdup (info->date);

  /* find the gimp_module_register symbol */
  if (! g_module_symbol (module_info->module, "gimp_module_register", &symbol))
    {
      module_info->state = GIMP_MODULE_STATE_ERROR;

      gimp_module_info_set_last_error (module_info,
                                       _("Missing gimp_module_register() symbol"));

      if (module_info->verbose)
	g_message (_("Module '%s' load error:\n%s"),
		   module_info->filename, module_info->last_module_error);
      g_module_close (module_info->module);
      module_info->module = NULL;
      return FALSE;
    }

  module_info->register_module = symbol;

  retval = module_info->register_module (module);

  if (retval)
    module_info->state = GIMP_MODULE_STATE_LOADED_OK;
  else
    module_info->state = GIMP_MODULE_STATE_LOAD_FAILED;

  return retval;
}

static void
gimp_module_info_unload (GTypeModule *module)
{
  GimpModuleInfoObj *module_info;

  g_return_if_fail (GIMP_IS_MODULE_INFO (module));

  module_info = GIMP_MODULE_INFO (module);

  g_return_if_fail (module_info->module != NULL);

  g_module_close (module_info->module); /* FIXME: error handling */
  module_info->module          = NULL;
  module_info->query_module    = NULL;
  module_info->register_module = NULL;

  module_info->state = GIMP_MODULE_STATE_UNLOADED_OK;
}

GimpModuleInfoObj *
gimp_module_info_new (const gchar *filename,
                      const gchar *inhibit_list,
                      gboolean     verbose)
{
  GimpModuleInfoObj *module_info;

  g_return_val_if_fail (filename != NULL, NULL);

  module_info = g_object_new (GIMP_TYPE_MODULE_INFO, NULL);

  module_info->filename = g_strdup (filename);
  module_info->verbose  = verbose ? TRUE : FALSE;
  module_info->on_disk  = TRUE;

  gimp_module_info_set_load_inhibit (module_info, inhibit_list);

  if (! module_info->load_inhibit)
    {
      if (gimp_module_info_load (G_TYPE_MODULE (module_info)))
        gimp_module_info_unload (G_TYPE_MODULE (module_info));
    }
  else
    {
      if (verbose)
	g_print (_("skipping module: '%s'\n"), filename);

      module_info->state = GIMP_MODULE_STATE_UNLOADED_OK;
    }

  return module_info;
}

void
gimp_module_info_modified (GimpModuleInfoObj *module_info)
{
  g_return_if_fail (GIMP_IS_MODULE_INFO (module_info));

  g_signal_emit (G_OBJECT (module_info), module_info_signals[MODIFIED], 0);
}

void
gimp_module_info_set_load_inhibit (GimpModuleInfoObj *module_info,
                                   const gchar       *inhibit_list)
{
  gchar       *p;
  gint         pathlen;
  const gchar *start;
  const gchar *end;

  g_return_if_fail (GIMP_IS_MODULE_INFO (module_info));
  g_return_if_fail (module_info->filename != NULL);

  module_info->load_inhibit = FALSE;

  if (! inhibit_list || ! strlen (inhibit_list))
    return;

  p = strstr (inhibit_list, module_info->filename);
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

  pathlen = strlen (module_info->filename);

  if ((end - start) == pathlen)
    module_info->load_inhibit = TRUE;
}

static void
gimp_module_info_set_last_error (GimpModuleInfoObj *module_info,
                                 const gchar       *error_str)
{
  if (module_info->last_module_error)
    g_free (module_info->last_module_error);

  module_info->last_module_error = g_strdup (error_str);
}
