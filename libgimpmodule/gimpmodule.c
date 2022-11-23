/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmamodule.c
 * (C) 1999 Austin Donnelly <austin@ligma.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#include "libligmabase/ligmabase.h"

#include "ligmamodule.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmamodule
 * @title: LigmaModule
 * @short_description: A #GTypeModule subclass which implements module
 *                     loading using #GModule.
 * @see_also: #GModule, #GTypeModule
 *
 * #LigmaModule is a generic mechanism to dynamically load modules into
 * LIGMA. It is a #GTypeModule subclass, implementing module loading
 * using #GModule.  #LigmaModule does not know which functionality is
 * implemented by the modules, it just provides a framework to get
 * arbitrary #GType implementations loaded from disk.
 **/


enum
{
  MODIFIED,
  LAST_SIGNAL
};


struct _LigmaModulePrivate
{
  GFile           *file;       /* path to the module                       */
  gboolean         auto_load;  /* auto-load the module on creation         */
  gboolean         verbose;    /* verbose error reporting                  */

  LigmaModuleInfo  *info;       /* returned values from module_query        */
  LigmaModuleState  state;      /* what's happened to the module            */
  gchar           *last_error;

  gboolean         on_disk;    /* TRUE if file still exists                */

  GModule         *module;     /* handle on the module                     */

  LigmaModuleQueryFunc     query_module;
  LigmaModuleRegisterFunc  register_module;
};


static void       ligma_module_finalize       (GObject     *object);

static gboolean   ligma_module_load           (GTypeModule *module);
static void       ligma_module_unload         (GTypeModule *module);

static gboolean   ligma_module_open           (LigmaModule  *module);
static gboolean   ligma_module_close          (LigmaModule  *module);
static void       ligma_module_set_last_error (LigmaModule  *module,
                                              const gchar *error_str);
static void       ligma_module_modified       (LigmaModule  *module);

static LigmaModuleInfo * ligma_module_info_new  (guint32               abi_version,
                                               const gchar          *purpose,
                                               const gchar          *author,
                                               const gchar          *version,
                                               const gchar          *copyright,
                                               const gchar          *date);
static LigmaModuleInfo * ligma_module_info_copy (const LigmaModuleInfo *info);
static void             ligma_module_info_free (LigmaModuleInfo       *info);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaModule, ligma_module, G_TYPE_TYPE_MODULE)

#define parent_class ligma_module_parent_class

static guint module_signals[LAST_SIGNAL];


static void
ligma_module_class_init (LigmaModuleClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

  module_signals[MODIFIED] =
    g_signal_new ("modified",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaModuleClass, modified),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize = ligma_module_finalize;

  module_class->load     = ligma_module_load;
  module_class->unload   = ligma_module_unload;

  klass->modified        = NULL;
}

static void
ligma_module_init (LigmaModule *module)
{
  module->priv = ligma_module_get_instance_private (module);

  module->priv->state = LIGMA_MODULE_STATE_ERROR;
}

static void
ligma_module_finalize (GObject *object)
{
  LigmaModule *module = LIGMA_MODULE (object);

  g_clear_object  (&module->priv->file);
  g_clear_pointer (&module->priv->info, ligma_module_info_free);
  g_clear_pointer (&module->priv->last_error, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
ligma_module_load (GTypeModule *module)
{
  LigmaModule *ligma_module = LIGMA_MODULE (module);
  gpointer    func;

  g_return_val_if_fail (ligma_module->priv->file != NULL, FALSE);
  g_return_val_if_fail (ligma_module->priv->module == NULL, FALSE);

  if (ligma_module->priv->verbose)
    g_print ("Loading module '%s'\n",
             ligma_file_get_utf8_name (ligma_module->priv->file));

  if (! ligma_module_open (ligma_module))
    return FALSE;

  if (! ligma_module_query_module (ligma_module))
    return FALSE;

  /* find the ligma_module_register symbol */
  if (! g_module_symbol (ligma_module->priv->module, "ligma_module_register",
                         &func))
    {
      ligma_module_set_last_error (ligma_module,
                                  "Missing ligma_module_register() symbol");

      g_message (_("Module '%s' load error: %s"),
                 ligma_file_get_utf8_name (ligma_module->priv->file),
                 ligma_module->priv->last_error);

      ligma_module_close (ligma_module);

      ligma_module->priv->state = LIGMA_MODULE_STATE_ERROR;

      return FALSE;
    }

  ligma_module->priv->register_module = func;

  if (! ligma_module->priv->register_module (module))
    {
      ligma_module_set_last_error (ligma_module,
                                  "ligma_module_register() returned FALSE");

      g_message (_("Module '%s' load error: %s"),
                 ligma_file_get_utf8_name (ligma_module->priv->file),
                 ligma_module->priv->last_error);

      ligma_module_close (ligma_module);

      ligma_module->priv->state = LIGMA_MODULE_STATE_LOAD_FAILED;

      return FALSE;
    }

  ligma_module->priv->state = LIGMA_MODULE_STATE_LOADED;

  return TRUE;
}

static void
ligma_module_unload (GTypeModule *module)
{
  LigmaModule *ligma_module = LIGMA_MODULE (module);

  g_return_if_fail (ligma_module->priv->module != NULL);

  if (ligma_module->priv->verbose)
    g_print ("Unloading module '%s'\n",
             ligma_file_get_utf8_name (ligma_module->priv->file));

  ligma_module_close (ligma_module);
}


/*  public functions  */

/**
 * ligma_module_new:
 * @file:      A #GFile pointing to a loadable module.
 * @auto_load: Pass %TRUE to exclude this module from auto-loading.
 * @verbose:   Pass %TRUE to enable debugging output.
 *
 * Creates a new #LigmaModule instance.
 *
 * Returns: The new #LigmaModule object.
 **/
LigmaModule *
ligma_module_new (GFile    *file,
                 gboolean  auto_load,
                 gboolean  verbose)
{
  LigmaModule *module;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (g_file_is_native (file), NULL);

  module = g_object_new (LIGMA_TYPE_MODULE, NULL);

  module->priv->file      = g_object_ref (file);
  module->priv->auto_load = auto_load ? TRUE : FALSE;
  module->priv->verbose   = verbose ? TRUE : FALSE;

  if (module->priv->auto_load)
    {
      if (ligma_module_load (G_TYPE_MODULE (module)))
        ligma_module_unload (G_TYPE_MODULE (module));
    }
  else
    {
      if (verbose)
        g_print ("Skipping module '%s'\n",
                 ligma_file_get_utf8_name (file));

      module->priv->state = LIGMA_MODULE_STATE_NOT_LOADED;
    }

  return module;
}

/**
 * ligma_module_get_file:
 * @module: A #LigmaModule
 *
 * Returns #GFile of the @module,
 *
 * Returns: (transfer none): The @module's #GFile.
 *
 * Since: 3.0
 **/
GFile *
ligma_module_get_file (LigmaModule *module)
{
  g_return_val_if_fail (LIGMA_IS_MODULE (module), NULL);

  return module->priv->file;
}

/**
 * ligma_module_set_auto_load:
 * @module:    A #LigmaModule
 * @auto_load: Pass %FALSE to exclude this module from auto-loading
 *
 * Sets the @auto_load property if the module. Emits "modified".
 *
 * Since: 3.0
 **/
void
ligma_module_set_auto_load (LigmaModule *module,
                           gboolean    auto_load)
{
  g_return_if_fail (LIGMA_IS_MODULE (module));

  if (auto_load != module->priv->auto_load)
    {
      module->priv->auto_load = auto_load ? TRUE : FALSE;

      ligma_module_modified (module);
    }
}

/**
 * ligma_module_get_auto_load:
 * @module: A #LigmaModule
 *
 * Returns whether this @module in automatically loaded at startup.
 *
 * Returns: The @module's 'auto_load' property.
 *
 * Since: 3.0
 **/
gboolean
ligma_module_get_auto_load (LigmaModule *module)
{
  g_return_val_if_fail (LIGMA_IS_MODULE (module), FALSE);

  return module->priv->auto_load;
}

/**
 * ligma_module_is_on_disk:
 * @module: A #LigmaModule
 *
 * Returns: Whether the @module is present on diak.
 *
 * Since: 3.0
 **/
gboolean
ligma_module_is_on_disk (LigmaModule *module)
{
  gboolean old_on_disk;

  g_return_val_if_fail (LIGMA_IS_MODULE (module), FALSE);

  old_on_disk = module->priv->on_disk;

  module->priv->on_disk =
    (g_file_query_file_type (module->priv->file,
                             G_FILE_QUERY_INFO_NONE, NULL) ==
     G_FILE_TYPE_REGULAR);

  if (module->priv->on_disk != old_on_disk)
    ligma_module_modified (module);

  return module->priv->on_disk;
}

/**
 * ligma_module_is_loaded:
 * @module: A #LigmaModule
 *
 * Returns: Whether the @module is currently loaded.
 *
 * Since: 3.0
 **/
gboolean
ligma_module_is_loaded (LigmaModule *module)
{
  g_return_val_if_fail (LIGMA_IS_MODULE (module), FALSE);

  return module->priv->module != NULL;
}

/**
 * ligma_module_get_info:
 * @module: A #LigmaModule
 *
 * Returns: (transfer none): The @module's #LigmaModuleInfo as provided
 *          by the actual module, or %NULL.
 *
 * Since: 3.0
 **/
const LigmaModuleInfo *
ligma_module_get_info (LigmaModule *module)
{
  g_return_val_if_fail (LIGMA_IS_MODULE (module), NULL);

  return module->priv->info;
}

/**
 * ligma_module_get_state:
 * @module: A #LigmaModule
 *
 * Returns: The @module's state.
 *
 * Since: 3.0
 **/
LigmaModuleState
ligma_module_get_state (LigmaModule *module)
{
  g_return_val_if_fail (LIGMA_IS_MODULE (module), LIGMA_MODULE_STATE_ERROR);

  return module->priv->state;
}

/**
 * ligma_module_get_last_error:
 * @module: A #LigmaModule
 *
 * Returns: The @module's last error message.
 *
 * Since: 3.0
 **/
const gchar *
ligma_module_get_last_error (LigmaModule *module)
{
  g_return_val_if_fail (LIGMA_IS_MODULE (module), NULL);

  return module->priv->last_error;
}

/**
 * ligma_module_query_module:
 * @module: A #LigmaModule.
 *
 * Queries the module without actually registering any of the types it
 * may implement. After successful query, ligma_module_get_info() can be
 * used to get further about the module.
 *
 * Returns: %TRUE on success.
 **/
gboolean
ligma_module_query_module (LigmaModule *module)
{
  const LigmaModuleInfo *info;
  gboolean              close_module = FALSE;
  gpointer              func;

  g_return_val_if_fail (LIGMA_IS_MODULE (module), FALSE);

  if (! module->priv->module)
    {
      if (! ligma_module_open (module))
        return FALSE;

      close_module = TRUE;
    }

  /* find the ligma_module_query symbol */
  if (! g_module_symbol (module->priv->module, "ligma_module_query", &func))
    {
      ligma_module_set_last_error (module,
                                  "Missing ligma_module_query() symbol");

      g_message (_("Module '%s' load error: %s"),
                 ligma_file_get_utf8_name (module->priv->file),
                 module->priv->last_error);

      ligma_module_close (module);

      module->priv->state = LIGMA_MODULE_STATE_ERROR;
      return FALSE;
    }

  module->priv->query_module = func;

  g_clear_pointer (&module->priv->info, ligma_module_info_free);

  info = module->priv->query_module (G_TYPE_MODULE (module));

  if (! info || info->abi_version != LIGMA_MODULE_ABI_VERSION)
    {
      ligma_module_set_last_error (module,
                                  info ?
                                  "module ABI version does not match" :
                                  "ligma_module_query() returned NULL");

      g_message (_("Module '%s' load error: %s"),
                 ligma_file_get_utf8_name (module->priv->file),
                 module->priv->last_error);

      ligma_module_close (module);

      module->priv->state = LIGMA_MODULE_STATE_ERROR;
      return FALSE;
    }

  module->priv->info = ligma_module_info_copy (info);

  if (close_module)
    return ligma_module_close (module);

  return TRUE;
}

/**
 * ligma_module_error_quark:
 *
 * This function is never called directly. Use LIGMA_MODULE_ERROR() instead.
 *
 * Returns: the #GQuark that defines the LIGMA module error domain.
 *
 * Since: 2.8
 **/
GQuark
ligma_module_error_quark (void)
{
  return g_quark_from_static_string ("ligma-module-error-quark");
}


/*  private functions  */

static gboolean
ligma_module_open (LigmaModule *module)
{
  gchar *path = g_file_get_path (module->priv->file);

  module->priv->module = g_module_open (path, 0);

  g_free (path);

  if (! module->priv->module)
    {
      module->priv->state = LIGMA_MODULE_STATE_ERROR;
      ligma_module_set_last_error (module, g_module_error ());

      g_message (_("Module '%s' load error: %s"),
                 ligma_file_get_utf8_name (module->priv->file),
                 module->priv->last_error);

      return FALSE;
    }

  return TRUE;
}

static gboolean
ligma_module_close (LigmaModule *module)
{
  g_module_close (module->priv->module); /* FIXME: error handling */
  module->priv->module          = NULL;
  module->priv->query_module    = NULL;
  module->priv->register_module = NULL;

  module->priv->state = LIGMA_MODULE_STATE_NOT_LOADED;

  return TRUE;
}

static void
ligma_module_set_last_error (LigmaModule  *module,
                            const gchar *error_str)
{
  if (module->priv->last_error)
    g_free (module->priv->last_error);

  module->priv->last_error = g_strdup (error_str);
}

static void
ligma_module_modified (LigmaModule *module)
{
  g_return_if_fail (LIGMA_IS_MODULE (module));

  g_signal_emit (module, module_signals[MODIFIED], 0);
}


/*  LigmaModuleInfo functions  */

/**
 * ligma_module_info_new:
 * @abi_version: The #LIGMA_MODULE_ABI_VERSION the module was compiled against.
 * @purpose:     The module's general purpose.
 * @author:      The module's author.
 * @version:     The module's version.
 * @copyright:   The module's copyright.
 * @date:        The module's release date.
 *
 * Creates a newly allocated #LigmaModuleInfo struct.
 *
 * Returns: The new #LigmaModuleInfo struct.
 **/
static LigmaModuleInfo *
ligma_module_info_new (guint32      abi_version,
                      const gchar *purpose,
                      const gchar *author,
                      const gchar *version,
                      const gchar *copyright,
                      const gchar *date)
{
  LigmaModuleInfo *info = g_slice_new0 (LigmaModuleInfo);

  info->abi_version = abi_version;
  info->purpose     = g_strdup (purpose);
  info->author      = g_strdup (author);
  info->version     = g_strdup (version);
  info->copyright   = g_strdup (copyright);
  info->date        = g_strdup (date);

  return info;
}

/**
 * ligma_module_info_copy:
 * @info: The #LigmaModuleInfo struct to copy.
 *
 * Copies a #LigmaModuleInfo struct.
 *
 * Returns: The new copy.
 **/
static LigmaModuleInfo *
ligma_module_info_copy (const LigmaModuleInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  return ligma_module_info_new (info->abi_version,
                               info->purpose,
                               info->author,
                               info->version,
                               info->copyright,
                               info->date);
}

/**
 * ligma_module_info_free:
 * @info: The #LigmaModuleInfo struct to free
 *
 * Frees the passed #LigmaModuleInfo.
 **/
static void
ligma_module_info_free (LigmaModuleInfo *info)
{
  g_return_if_fail (info != NULL);

  g_free (info->purpose);
  g_free (info->author);
  g_free (info->version);
  g_free (info->copyright);
  g_free (info->date);

  g_slice_free (LigmaModuleInfo, info);
}
