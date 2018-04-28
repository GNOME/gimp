/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmodule.c
 * (C) 1999 Austin Donnelly <austin@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "gimpmodule.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpmodule
 * @title: GimpModule
 * @short_description: A #GTypeModule subclass which implements module
 *                     loading using #GModule.
 * @see_also: #GModule, #GTypeModule
 *
 * A #GTypeModule subclass which implements module loading using #GModule.
 **/


enum
{
  MODIFIED,
  LAST_SIGNAL
};


static void       gimp_module_finalize       (GObject     *object);

static gboolean   gimp_module_load           (GTypeModule *module);
static void       gimp_module_unload         (GTypeModule *module);

static gboolean   gimp_module_open           (GimpModule  *module);
static gboolean   gimp_module_close          (GimpModule  *module);
static void       gimp_module_set_last_error (GimpModule  *module,
                                              const gchar *error_str);


G_DEFINE_TYPE (GimpModule, gimp_module, G_TYPE_TYPE_MODULE)

#define parent_class gimp_module_parent_class

static guint module_signals[LAST_SIGNAL];


static void
gimp_module_class_init (GimpModuleClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

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
  GimpModule *module = GIMP_MODULE (object);

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
  GimpModule *gimp_module = GIMP_MODULE (module);
  gpointer    func;

  g_return_val_if_fail (gimp_module->filename != NULL, FALSE);
  g_return_val_if_fail (gimp_module->module == NULL, FALSE);

  if (gimp_module->verbose)
    g_print ("Loading module '%s'\n",
             gimp_filename_to_utf8 (gimp_module->filename));

  if (! gimp_module_open (gimp_module))
    return FALSE;

  if (! gimp_module_query_module (gimp_module))
    return FALSE;

  /* find the gimp_module_register symbol */
  if (! g_module_symbol (gimp_module->module, "gimp_module_register", &func))
    {
      gimp_module_set_last_error (gimp_module,
                                  "Missing gimp_module_register() symbol");

      g_message (_("Module '%s' load error: %s"),
                 gimp_filename_to_utf8 (gimp_module->filename),
                 gimp_module->last_module_error);

      gimp_module_close (gimp_module);

      gimp_module->state = GIMP_MODULE_STATE_ERROR;

      return FALSE;
    }

  gimp_module->register_module = func;

  if (! gimp_module->register_module (module))
    {
      gimp_module_set_last_error (gimp_module,
                                  "gimp_module_register() returned FALSE");

      g_message (_("Module '%s' load error: %s"),
                 gimp_filename_to_utf8 (gimp_module->filename),
                 gimp_module->last_module_error);

      gimp_module_close (gimp_module);

      gimp_module->state = GIMP_MODULE_STATE_LOAD_FAILED;

      return FALSE;
    }

  gimp_module->state = GIMP_MODULE_STATE_LOADED;

  return TRUE;
}

static void
gimp_module_unload (GTypeModule *module)
{
  GimpModule *gimp_module = GIMP_MODULE (module);

  g_return_if_fail (gimp_module->module != NULL);

  if (gimp_module->verbose)
    g_print ("Unloading module '%s'\n",
             gimp_filename_to_utf8 (gimp_module->filename));

  gimp_module_close (gimp_module);
}


/*  public functions  */

/**
 * gimp_module_new:
 * @filename:     The filename of a loadable module.
 * @load_inhibit: Pass %TRUE to exclude this module from auto-loading.
 * @verbose:      Pass %TRUE to enable debugging output.
 *
 * Creates a new #GimpModule instance.
 *
 * Return value: The new #GimpModule object.
 **/
GimpModule *
gimp_module_new (const gchar *filename,
                 gboolean     load_inhibit,
                 gboolean     verbose)
{
  GimpModule *module;

  g_return_val_if_fail (filename != NULL, NULL);

  module = g_object_new (GIMP_TYPE_MODULE, NULL);

  module->filename     = g_strdup (filename);
  module->load_inhibit = load_inhibit ? TRUE : FALSE;
  module->verbose      = verbose ? TRUE : FALSE;
  module->on_disk      = TRUE;

  if (! module->load_inhibit)
    {
      if (gimp_module_load (G_TYPE_MODULE (module)))
        gimp_module_unload (G_TYPE_MODULE (module));
    }
  else
    {
      if (verbose)
        g_print ("Skipping module '%s'\n",
                 gimp_filename_to_utf8 (filename));

      module->state = GIMP_MODULE_STATE_NOT_LOADED;
    }

  return module;
}

/**
 * gimp_module_query_module:
 * @module: A #GimpModule.
 *
 * Queries the module without actually registering any of the types it
 * may implement. After successful query, the @info field of the
 * #GimpModule struct will be available for further inspection.
 *
 * Return value: %TRUE on success.
 **/
gboolean
gimp_module_query_module (GimpModule *module)
{
  const GimpModuleInfo *info;
  gboolean              close_module = FALSE;
  gpointer              func;

  g_return_val_if_fail (GIMP_IS_MODULE (module), FALSE);

  if (! module->module)
    {
      if (! gimp_module_open (module))
        return FALSE;

      close_module = TRUE;
    }

  /* find the gimp_module_query symbol */
  if (! g_module_symbol (module->module, "gimp_module_query", &func))
    {
      gimp_module_set_last_error (module,
                                  "Missing gimp_module_query() symbol");

      g_message (_("Module '%s' load error: %s"),
                 gimp_filename_to_utf8 (module->filename),
                 module->last_module_error);

      gimp_module_close (module);

      module->state = GIMP_MODULE_STATE_ERROR;
      return FALSE;
    }

  module->query_module = func;

  if (module->info)
    {
      gimp_module_info_free (module->info);
      module->info = NULL;
    }

  info = module->query_module (G_TYPE_MODULE (module));

  if (! info || info->abi_version != GIMP_MODULE_ABI_VERSION)
    {
      gimp_module_set_last_error (module,
                                  info ?
                                  "module ABI version does not match" :
                                  "gimp_module_query() returned NULL");

      g_message (_("Module '%s' load error: %s"),
                 gimp_filename_to_utf8 (module->filename),
                 module->last_module_error);

      gimp_module_close (module);

      module->state = GIMP_MODULE_STATE_ERROR;
      return FALSE;
    }

  module->info = gimp_module_info_copy (info);

  if (close_module)
    return gimp_module_close (module);

  return TRUE;
}

/**
 * gimp_module_modified:
 * @module: A #GimpModule.
 *
 * Emits the "modified" signal. Call it whenever you have modified the module
 * manually (which you shouldn't do).
 **/
void
gimp_module_modified (GimpModule *module)
{
  g_return_if_fail (GIMP_IS_MODULE (module));

  g_signal_emit (module, module_signals[MODIFIED], 0);
}

/**
 * gimp_module_set_load_inhibit:
 * @module:       A #GimpModule.
 * @load_inhibit: Pass %TRUE to exclude this module from auto-loading.
 *
 * Sets the @load_inhibit property if the module. Emits "modified".
 **/
void
gimp_module_set_load_inhibit (GimpModule *module,
                              gboolean    load_inhibit)
{
  g_return_if_fail (GIMP_IS_MODULE (module));

  if (load_inhibit != module->load_inhibit)
    {
      module->load_inhibit = load_inhibit ? TRUE : FALSE;

      gimp_module_modified (module);
    }
}

/**
 * gimp_module_state_name:
 * @state: A #GimpModuleState.
 *
 * Returns the translated textual representation of a #GimpModuleState.
 * The returned string must not be freed.
 *
 * Return value: The @state's name.
 **/
const gchar *
gimp_module_state_name (GimpModuleState state)
{
  static const gchar * const statenames[] =
  {
    N_("Module error"),
    N_("Loaded"),
    N_("Load failed"),
    N_("Not loaded")
  };

  g_return_val_if_fail (state >= GIMP_MODULE_STATE_ERROR &&
                        state <= GIMP_MODULE_STATE_NOT_LOADED, NULL);

  return gettext (statenames[state]);
}

/**
 * gimp_module_error_quark:
 *
 * This function is never called directly. Use GIMP_MODULE_ERROR() instead.
 *
 * Return value: the #GQuark that defines the GIMP module error domain.
 *
 * Since: 2.8
 **/
GQuark
gimp_module_error_quark (void)
{
  return g_quark_from_static_string ("gimp-module-error-quark");
}


/*  private functions  */

static gboolean
gimp_module_open (GimpModule *module)
{
  module->module = g_module_open (module->filename, 0);

  if (! module->module)
    {
      module->state = GIMP_MODULE_STATE_ERROR;
      gimp_module_set_last_error (module, g_module_error ());

      g_message (_("Module '%s' load error: %s"),
                 gimp_filename_to_utf8 (module->filename),
                 module->last_module_error);

      return FALSE;
    }

  return TRUE;
}

static gboolean
gimp_module_close (GimpModule *module)
{
  g_module_close (module->module); /* FIXME: error handling */
  module->module          = NULL;
  module->query_module    = NULL;
  module->register_module = NULL;

  module->state = GIMP_MODULE_STATE_NOT_LOADED;

  return TRUE;
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

/**
 * gimp_module_info_new:
 * @abi_version: The #GIMP_MODULE_ABI_VERSION the module was compiled against.
 * @purpose:     The module's general purpose.
 * @author:      The module's author.
 * @version:     The module's version.
 * @copyright:   The module's copyright.
 * @date:        The module's release date.
 *
 * Creates a newly allocated #GimpModuleInfo struct.
 *
 * Return value: The new #GimpModuleInfo struct.
 **/
GimpModuleInfo *
gimp_module_info_new (guint32      abi_version,
                      const gchar *purpose,
                      const gchar *author,
                      const gchar *version,
                      const gchar *copyright,
                      const gchar *date)
{
  GimpModuleInfo *info = g_slice_new0 (GimpModuleInfo);

  info->abi_version = abi_version;
  info->purpose     = g_strdup (purpose);
  info->author      = g_strdup (author);
  info->version     = g_strdup (version);
  info->copyright   = g_strdup (copyright);
  info->date        = g_strdup (date);

  return info;
}

/**
 * gimp_module_info_copy:
 * @info: The #GimpModuleInfo struct to copy.
 *
 * Copies a #GimpModuleInfo struct.
 *
 * Return value: The new copy.
 **/
GimpModuleInfo *
gimp_module_info_copy (const GimpModuleInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  return gimp_module_info_new (info->abi_version,
                               info->purpose,
                               info->author,
                               info->version,
                               info->copyright,
                               info->date);
}

/**
 * gimp_module_info_free:
 * @info: The #GimpModuleInfo struct to free
 *
 * Frees the passed #GimpModuleInfo.
 **/
void
gimp_module_info_free (GimpModuleInfo *info)
{
  g_return_if_fail (info != NULL);

  g_free (info->purpose);
  g_free (info->author);
  g_free (info->version);
  g_free (info->copyright);
  g_free (info->date);

  g_slice_free (GimpModuleInfo, info);
}
