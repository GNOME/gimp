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
 * <https://www.gnu.org/licenses/>.
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
 * #GimpModule is a generic mechanism to dynamically load modules into
 * GIMP. It is a #GTypeModule subclass, implementing module loading
 * using #GModule.  #GimpModule does not know which functionality is
 * implemented by the modules, it just provides a framework to get
 * arbitrary #GType implementations loaded from disk.
 **/


enum
{
  PROP_0,
  PROP_AUTO_LOAD,
  PROP_ON_DISK,
  N_PROPS
};
static GParamSpec *obj_props[N_PROPS] = { NULL, };

typedef struct _GimpModulePrivate
{
  GFile           *file;       /* path to the module                       */
  gboolean         auto_load;  /* auto-load the module on creation         */
  gboolean         verbose;    /* verbose error reporting                  */

  GimpModuleInfo  *info;       /* returned values from module_query        */
  GimpModuleState  state;      /* what's happened to the module            */
  gchar           *last_error;

  gboolean         on_disk;    /* TRUE if file still exists                */

  GModule         *module;     /* handle on the module                     */

  GimpModuleQueryFunc     query_module;
  GimpModuleRegisterFunc  register_module;
} GimpModulePrivate;

#define GET_PRIVATE(obj) ((GimpModulePrivate *) gimp_module_get_instance_private ((GimpModule *) (obj)))


static void       gimp_module_get_property   (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);
static void       gimp_module_set_property   (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void       gimp_module_finalize       (GObject      *object);

static gboolean   gimp_module_load           (GTypeModule  *module);
static void       gimp_module_unload         (GTypeModule  *module);

static gboolean   gimp_module_open           (GimpModule   *module);
static gboolean   gimp_module_close          (GimpModule   *module);
static void       gimp_module_set_last_error (GimpModule   *module,
                                              const gchar  *error_str);

static GimpModuleInfo * gimp_module_info_new  (guint32               abi_version,
                                               const gchar          *purpose,
                                               const gchar          *author,
                                               const gchar          *version,
                                               const gchar          *copyright,
                                               const gchar          *date);
static GimpModuleInfo * gimp_module_info_copy (const GimpModuleInfo *info);
static void             gimp_module_info_free (GimpModuleInfo       *info);


G_DEFINE_TYPE_WITH_PRIVATE (GimpModule, gimp_module, G_TYPE_TYPE_MODULE)

#define parent_class gimp_module_parent_class


static void
gimp_module_class_init (GimpModuleClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

  object_class->set_property = gimp_module_set_property;
  object_class->get_property = gimp_module_get_property;
  object_class->finalize = gimp_module_finalize;

  obj_props[PROP_AUTO_LOAD] = g_param_spec_boolean ("auto-load", "auto-load", "auto-load",
                                                    FALSE, GIMP_PARAM_READWRITE);
  obj_props[PROP_ON_DISK] = g_param_spec_boolean ("on-disk", "on-disk", "on-disk",
                                                   FALSE, GIMP_PARAM_READABLE);
  g_object_class_install_properties (object_class, N_PROPS, obj_props);

  module_class->load     = gimp_module_load;
  module_class->unload   = gimp_module_unload;
}

static void
gimp_module_init (GimpModule *module)
{
  GET_PRIVATE (module)->state = GIMP_MODULE_STATE_ERROR;
}

static void
gimp_module_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpModule *module = GIMP_MODULE (object);

  switch (property_id)
    {
    case PROP_AUTO_LOAD:
      gimp_module_set_auto_load (module, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_module_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpModule *module = GIMP_MODULE (object);

  switch (property_id)
    {
    case PROP_AUTO_LOAD:
      g_value_set_boolean (value, gimp_module_get_auto_load (module));
      break;

    case PROP_ON_DISK:
      g_value_set_boolean (value, gimp_module_is_on_disk (module));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_module_finalize (GObject *object)
{
  GimpModule        *module = GIMP_MODULE (object);
  GimpModulePrivate *priv   = GET_PRIVATE (module);

  g_clear_object  (&priv->file);
  g_clear_pointer (&priv->info, gimp_module_info_free);
  g_clear_pointer (&priv->last_error, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_module_load (GTypeModule *module)
{
  GimpModule        *gimp_module = GIMP_MODULE (module);
  GimpModulePrivate *priv        = GET_PRIVATE (gimp_module);
  gpointer    func;

  g_return_val_if_fail (priv->file != NULL, FALSE);
  g_return_val_if_fail (priv->module == NULL, FALSE);

  if (priv->verbose)
    g_print ("Loading module '%s'\n",
             gimp_file_get_utf8_name (priv->file));

  if (! gimp_module_open (gimp_module))
    return FALSE;

  if (! gimp_module_query_module (gimp_module))
    return FALSE;

  /* find the gimp_module_register symbol */
  if (! g_module_symbol (priv->module, "gimp_module_register",
                         &func))
    {
      gimp_module_set_last_error (gimp_module,
                                  "Missing gimp_module_register() symbol");

      g_message (_("Module '%s' load error: %s"),
                 gimp_file_get_utf8_name (priv->file),
                 priv->last_error);

      gimp_module_close (gimp_module);

      priv->state = GIMP_MODULE_STATE_ERROR;

      return FALSE;
    }

  priv->register_module = func;

  if (! priv->register_module (module))
    {
      gimp_module_set_last_error (gimp_module,
                                  "gimp_module_register() returned FALSE");

      g_message (_("Module '%s' load error: %s"),
                 gimp_file_get_utf8_name (priv->file),
                 priv->last_error);

      gimp_module_close (gimp_module);

      priv->state = GIMP_MODULE_STATE_LOAD_FAILED;

      return FALSE;
    }

  priv->state = GIMP_MODULE_STATE_LOADED;

  return TRUE;
}

static void
gimp_module_unload (GTypeModule *module)
{
  GimpModule        *gimp_module = GIMP_MODULE (module);
  GimpModulePrivate *priv        = GET_PRIVATE (gimp_module);

  g_return_if_fail (priv->module != NULL);

  if (priv->verbose)
    g_print ("Unloading module '%s'\n",
             gimp_file_get_utf8_name (priv->file));

  gimp_module_close (gimp_module);
}


/*  public functions  */

/**
 * gimp_module_new:
 * @file:      A #GFile pointing to a loadable module.
 * @auto_load: Pass %TRUE to exclude this module from auto-loading.
 * @verbose:   Pass %TRUE to enable debugging output.
 *
 * Creates a new #GimpModule instance.
 *
 * Returns: The new #GimpModule object.
 **/
GimpModule *
gimp_module_new (GFile    *file,
                 gboolean  auto_load,
                 gboolean  verbose)
{
  GimpModule        *module;
  GimpModulePrivate *priv;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (g_file_is_native (file), NULL);

  module = g_object_new (GIMP_TYPE_MODULE, NULL);
  priv   = GET_PRIVATE (module);

  priv->file      = g_object_ref (file);
  priv->auto_load = auto_load ? TRUE : FALSE;
  priv->verbose   = verbose ? TRUE : FALSE;

  if (priv->auto_load)
    {
      if (gimp_module_load (G_TYPE_MODULE (module)))
        gimp_module_unload (G_TYPE_MODULE (module));
    }
  else
    {
      if (verbose)
        g_print ("Skipping module '%s'\n",
                 gimp_file_get_utf8_name (file));

      priv->state = GIMP_MODULE_STATE_NOT_LOADED;
    }

  return module;
}

/**
 * gimp_module_get_file:
 * @module: A #GimpModule
 *
 * Returns #GFile of the @module,
 *
 * Returns: (transfer none): The @module's #GFile.
 *
 * Since: 3.0
 **/
GFile *
gimp_module_get_file (GimpModule *module)
{
  g_return_val_if_fail (GIMP_IS_MODULE (module), NULL);

  return GET_PRIVATE (module)->file;
}

/**
 * gimp_module_set_auto_load:
 * @module:    A #GimpModule
 * @auto_load: Pass %FALSE to exclude this module from auto-loading
 *
 * Sets the @auto_load property of the module
 *
 * Since: 3.0
 **/
void
gimp_module_set_auto_load (GimpModule *module,
                           gboolean    auto_load)
{
  GimpModulePrivate *priv;

  g_return_if_fail (GIMP_IS_MODULE (module));

  priv = GET_PRIVATE (module);

  if (auto_load != priv->auto_load)
    {
      priv->auto_load = auto_load;

      g_object_notify_by_pspec (G_OBJECT (module), obj_props[PROP_AUTO_LOAD]);
    }
}

/**
 * gimp_module_get_auto_load:
 * @module: A #GimpModule
 *
 * Returns whether this @module in automatically loaded at startup.
 *
 * Returns: The @module's 'auto_load' property.
 *
 * Since: 3.0
 **/
gboolean
gimp_module_get_auto_load (GimpModule *module)
{
  g_return_val_if_fail (GIMP_IS_MODULE (module), FALSE);

  return GET_PRIVATE (module)->auto_load;
}

/**
 * gimp_module_is_on_disk:
 * @module: A #GimpModule
 *
 * Returns: Whether the @module is present on diak.
 *
 * Since: 3.0
 **/
gboolean
gimp_module_is_on_disk (GimpModule *module)
{
  GimpModulePrivate *priv;
  gboolean           old_on_disk;

  g_return_val_if_fail (GIMP_IS_MODULE (module), FALSE);

  priv = GET_PRIVATE (module);

  old_on_disk = priv->on_disk;

  priv->on_disk =
    (g_file_query_file_type (priv->file,
                             G_FILE_QUERY_INFO_NONE, NULL) ==
     G_FILE_TYPE_REGULAR);

  if (priv->on_disk != old_on_disk)
    g_object_notify_by_pspec (G_OBJECT (module), obj_props[PROP_ON_DISK]);

  return priv->on_disk;
}

/**
 * gimp_module_is_loaded:
 * @module: A #GimpModule
 *
 * Returns: Whether the @module is currently loaded.
 *
 * Since: 3.0
 **/
gboolean
gimp_module_is_loaded (GimpModule *module)
{
  g_return_val_if_fail (GIMP_IS_MODULE (module), FALSE);

  return GET_PRIVATE (module)->module != NULL;
}

/**
 * gimp_module_get_info:
 * @module: A #GimpModule
 *
 * Returns: (transfer none): The @module's #GimpModuleInfo as provided
 *          by the actual module, or %NULL.
 *
 * Since: 3.0
 **/
const GimpModuleInfo *
gimp_module_get_info (GimpModule *module)
{
  g_return_val_if_fail (GIMP_IS_MODULE (module), NULL);

  return GET_PRIVATE (module)->info;
}

/**
 * gimp_module_get_state:
 * @module: A #GimpModule
 *
 * Returns: The @module's state.
 *
 * Since: 3.0
 **/
GimpModuleState
gimp_module_get_state (GimpModule *module)
{
  g_return_val_if_fail (GIMP_IS_MODULE (module), GIMP_MODULE_STATE_ERROR);

  return GET_PRIVATE (module)->state;
}

/**
 * gimp_module_get_last_error:
 * @module: A #GimpModule
 *
 * Returns: The @module's last error message.
 *
 * Since: 3.0
 **/
const gchar *
gimp_module_get_last_error (GimpModule *module)
{
  g_return_val_if_fail (GIMP_IS_MODULE (module), NULL);

  return GET_PRIVATE (module)->last_error;
}

/**
 * gimp_module_query_module:
 * @module: A #GimpModule.
 *
 * Queries the module without actually registering any of the types it
 * may implement. After successful query, gimp_module_get_info() can be
 * used to get further about the module.
 *
 * Returns: %TRUE on success.
 **/
gboolean
gimp_module_query_module (GimpModule *module)
{
  const GimpModuleInfo *info;
  GimpModulePrivate    *priv;
  gboolean              close_module = FALSE;
  gpointer              func;

  g_return_val_if_fail (GIMP_IS_MODULE (module), FALSE);

  priv = GET_PRIVATE (module);

  if (! priv->module)
    {
      if (! gimp_module_open (module))
        return FALSE;

      close_module = TRUE;
    }

  /* find the gimp_module_query symbol */
  if (! g_module_symbol (priv->module, "gimp_module_query", &func))
    {
      gimp_module_set_last_error (module,
                                  "Missing gimp_module_query() symbol");

      g_message (_("Module '%s' load error: %s"),
                 gimp_file_get_utf8_name (priv->file),
                 priv->last_error);

      gimp_module_close (module);

      priv->state = GIMP_MODULE_STATE_ERROR;
      return FALSE;
    }

  priv->query_module = func;

  g_clear_pointer (&priv->info, gimp_module_info_free);

  info = priv->query_module (G_TYPE_MODULE (module));

  if (! info || info->abi_version != GIMP_MODULE_ABI_VERSION)
    {
      gimp_module_set_last_error (module,
                                  info ?
                                  "module ABI version does not match" :
                                  "gimp_module_query() returned NULL");

      g_message (_("Module '%s' load error: %s"),
                 gimp_file_get_utf8_name (priv->file),
                 priv->last_error);

      gimp_module_close (module);

      priv->state = GIMP_MODULE_STATE_ERROR;
      return FALSE;
    }

  priv->info = gimp_module_info_copy (info);

  if (close_module)
    return gimp_module_close (module);

  return TRUE;
}

/**
 * gimp_module_error_quark:
 *
 * This function is never called directly. Use GIMP_MODULE_ERROR() instead.
 *
 * Returns: the #GQuark that defines the GIMP module error domain.
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
  GimpModulePrivate *priv = GET_PRIVATE (module);
  gchar             *path = g_file_get_path (priv->file);

  priv->module = g_module_open (path, 0);

  g_free (path);

  if (! priv->module)
    {
      priv->state = GIMP_MODULE_STATE_ERROR;
      gimp_module_set_last_error (module, g_module_error ());

      g_message (_("Module '%s' load error: %s"),
                 gimp_file_get_utf8_name (priv->file),
                 priv->last_error);

      return FALSE;
    }

  return TRUE;
}

static gboolean
gimp_module_close (GimpModule *module)
{
  GimpModulePrivate *priv = GET_PRIVATE (module);

  g_module_close (priv->module); /* FIXME: error handling */
  priv->module          = NULL;
  priv->query_module    = NULL;
  priv->register_module = NULL;

  priv->state = GIMP_MODULE_STATE_NOT_LOADED;

  return TRUE;
}

static void
gimp_module_set_last_error (GimpModule  *module,
                            const gchar *error_str)
{
  GimpModulePrivate *priv = GET_PRIVATE (module);

  if (priv->last_error)
    g_free (priv->last_error);

  priv->last_error = g_strdup (error_str);
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
 * Returns: The new #GimpModuleInfo struct.
 **/
static GimpModuleInfo *
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
 * Returns: The new copy.
 **/
static GimpModuleInfo *
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
static void
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
