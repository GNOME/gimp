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


static void    gimp_module_info_class_init (GimpModuleInfoObjClass *klass);
static void    gimp_module_info_init       (GimpModuleInfoObj      *mod);

static void    gimp_module_info_finalize   (GObject                *object);

static gsize   gimp_module_info_get_memsize (GimpObject            *object);


static guint module_info_signals[LAST_SIGNAL];

static GimpObjectClass *parent_class = NULL;


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

      module_info_type = g_type_register_static (GIMP_TYPE_OBJECT,
						 "GimpModuleInfoObj",
						 &module_info_info, 0);
    }

  return module_info_type;
}

static void
gimp_module_info_class_init (GimpModuleInfoObjClass *klass)
{
  GObjectClass    *object_class;
  GimpObjectClass *gimp_object_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  module_info_signals[MODIFIED] =
    g_signal_new ("modified",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpModuleInfoObjClass, modified),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize         = gimp_module_info_finalize;

  gimp_object_class->get_memsize = gimp_module_info_get_memsize;

  klass->modified                = NULL;
}

static void
gimp_module_info_init (GimpModuleInfoObj *module_info)
{
  module_info->fullpath          = NULL;
  module_info->state             = GIMP_MODULE_STATE_ERROR;
  module_info->on_disk           = FALSE;
  module_info->load_inhibit      = FALSE;
  module_info->refs              = 0;

  module_info->info              = NULL;
  module_info->module            = NULL;
  module_info->last_module_error = NULL;
  module_info->init              = NULL;
  module_info->unload            = NULL;
}

static void
gimp_module_info_finalize (GObject *object)
{
  GimpModuleInfoObj *mod;

  mod = GIMP_MODULE_INFO (object);

  /* if this trips, then we're onto some serious lossage in a moment */
  g_return_if_fail (mod->refs == 0);

  if (mod->last_module_error)
    {
      g_free (mod->last_module_error);
      mod->last_module_error = NULL;
    }

  if (mod->fullpath)
    {
      g_free (mod->fullpath);
      mod->fullpath = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gsize
gimp_module_info_get_memsize (GimpObject *object)
{
  GimpModuleInfoObj *module_info;
  gsize              memsize = 0;

  module_info = GIMP_MODULE_INFO (object);

  if (module_info->fullpath)
    memsize += strlen (module_info->fullpath) + 1;

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
}

GimpModuleInfoObj *
gimp_module_info_new (const gchar *filename)
{
  GimpModuleInfoObj *module_info;

  g_return_val_if_fail (filename != NULL, NULL);

  module_info = g_object_new (GIMP_TYPE_MODULE_INFO, NULL);

  module_info->fullpath = g_strdup (filename);
  module_info->on_disk  = TRUE;

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
  g_return_if_fail (module_info->fullpath != NULL);

  module_info->load_inhibit = FALSE;

  if (! inhibit_list || ! strlen (inhibit_list))
    return;

  p = strstr (inhibit_list, module_info->fullpath);
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

  pathlen = strlen (module_info->fullpath);

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


/*
 * FIXME: currently this will fail badly on EMX
 */
#ifdef __EMX__
extern void gimp_color_selector_register   (void);
extern void gimp_color_selector_unregister (void);
extern void dialog_register                (void);
extern void dialog_unregister              (void);

static struct main_funcs_struc
{
  gchar *name;
  void (* func) (void);
} 

gimp_main_funcs[] =
{
  { "gimp_color_selector_register",   gimp_color_selector_register },
  { "gimp_color_selector_unregister", gimp_color_selector_unregister },
  { "dialog_register",                dialog_register },
  { "dialog_unregister",              dialog_unregister },
  { NULL, NULL }
};
#endif

void
gimp_module_info_module_load (GimpModuleInfoObj *module_info, 
                              gboolean           verbose)
{
  gpointer symbol;

  g_return_if_fail (GIMP_IS_MODULE_INFO (module_info));
  g_return_if_fail (module_info->fullpath != NULL);
  g_return_if_fail (module_info->module == NULL);

  module_info->module = g_module_open (module_info->fullpath,
                                       G_MODULE_BIND_LAZY);

  if (! module_info->module)
    {
      module_info->state = GIMP_MODULE_STATE_ERROR;

      gimp_module_info_set_last_error (module_info, g_module_error ());

      if (verbose)
	g_message (_("Module '%s' load error:\n%s"),
		   module_info->fullpath, module_info->last_module_error);
      return;
    }

#ifdef __EMX__
  if (g_module_symbol (module_info->module, "gimp_main_funcs", &symbol))
    {
      *(struct main_funcs_struc **) symbol = gimp_main_funcs;
    }
#endif

  /* find the module_init symbol */
  if (! g_module_symbol (module_info->module, "module_init", &symbol))
    {
      module_info->state = GIMP_MODULE_STATE_ERROR;

      gimp_module_info_set_last_error (module_info,
                                       _("Missing module_init() symbol"));

      if (verbose)
	g_message (_("Module '%s' load error:\n%s"),
		   module_info->fullpath, module_info->last_module_error);
      g_module_close (module_info->module);
      module_info->module = NULL;
      return;
    }

  module_info->init = symbol;

  /* loaded modules are assumed to have a ref of 1 */
  gimp_module_info_module_ref (module_info);

  /* run module's initialisation */
  if (module_info->init (&module_info->info) == GIMP_MODULE_UNLOAD)
    {
      module_info->state = GIMP_MODULE_STATE_LOAD_FAILED;
      gimp_module_info_module_unref (module_info);
      module_info->info = NULL;
      return;
    }

  /* module is now happy */
  module_info->state = GIMP_MODULE_STATE_LOADED_OK;

  /* do we have an unload function? */
  if (g_module_symbol (module_info->module, "module_unload", &symbol))
    {
      module_info->unload = symbol;
    }
}

static void
gimp_module_info_module_unload_completed_callback (gpointer data)
{
  GimpModuleInfoObj *module_info;

  module_info = (GimpModuleInfoObj *) data;

  g_return_if_fail (module_info->state == GIMP_MODULE_STATE_UNLOAD_REQUESTED);

  /* lose the ref we gave this module when we loaded it,
   * since the module's now happy to be unloaded.
   */
  gimp_module_info_module_unref (module_info);

  module_info->info  = NULL;
  module_info->state = GIMP_MODULE_STATE_UNLOADED_OK;

  gimp_module_info_modified (module_info);
}

static gboolean
gimp_module_info_module_idle_unref (gpointer data)
{
  GimpModuleInfoObj *module_info;

  module_info = (GimpModuleInfoObj *) data;

  gimp_module_info_module_unref (module_info);

  return FALSE;
}

void
gimp_module_info_module_unload (GimpModuleInfoObj *module_info,
                                gboolean           verbose)
{
  g_return_if_fail (GIMP_IS_MODULE_INFO (module_info));
  g_return_if_fail (module_info->module != NULL);
  g_return_if_fail (module_info->unload != NULL);

  if (module_info->state == GIMP_MODULE_STATE_UNLOAD_REQUESTED)
    return;

  module_info->state = GIMP_MODULE_STATE_UNLOAD_REQUESTED;

  /* Send the unload request.  Need to ref the module so we don't
   * accidentally unload it while this call is in progress (eg if the
   * callback is called before the unload function returns).
   */
  gimp_module_info_module_ref (module_info);

  module_info->unload (module_info->info->shutdown_data,
                       gimp_module_info_module_unload_completed_callback,
                       module_info);

  g_idle_add (gimp_module_info_module_idle_unref, module_info);
}

void
gimp_module_info_module_ref (GimpModuleInfoObj *module_info)
{
  g_return_if_fail (GIMP_IS_MODULE_INFO (module_info));
  g_return_if_fail (module_info->refs >= 0);
  g_return_if_fail (module_info->module != NULL);

  module_info->refs++;
}

void
gimp_module_info_module_unref (GimpModuleInfoObj *module_info)
{
  g_return_if_fail (GIMP_IS_MODULE_INFO (module_info));
  g_return_if_fail (module_info->refs > 0);
  g_return_if_fail (module_info->module != NULL);

  module_info->refs--;

  if (module_info->refs == 0)
    {
      g_module_close (module_info->module);
      module_info->module = NULL;
    }
}
