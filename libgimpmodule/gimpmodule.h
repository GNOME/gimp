/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmodule.h
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

#ifndef __GIMP_MODULE_H__
#define __GIMP_MODULE_H__

#include <gio/gio.h>
#include <gmodule.h>

#define __GIMP_MODULE_H_INSIDE__

#include <libgimpmodule/gimpmoduletypes.h>

#include <libgimpmodule/gimpmoduledb.h>

#undef __GIMP_MODULE_H_INSIDE__

G_BEGIN_DECLS


/**
 * GIMP_MODULE_ABI_VERSION:
 *
 * The version of the module system's ABI. Modules put this value into
 * #GimpModuleInfo's @abi_version field so the code loading the modules
 * can check if it was compiled against the same module ABI the modules
 * are compiled against.
 *
 *  GIMP_MODULE_ABI_VERSION is incremented each time one of the
 *  following changes:
 *
 *  - the libgimpmodule implementation (if the change affects modules).
 *
 *  - one of the classes implemented by modules (currently #GimpColorDisplay,
 *    #GimpColorSelector and #GimpController).
 **/
#define GIMP_MODULE_ABI_VERSION 0x0005


/**
 * GimpModuleState:
 * @GIMP_MODULE_STATE_ERROR:       Missing gimp_module_register() function
 *                                 or other error.
 * @GIMP_MODULE_STATE_LOADED:      An instance of a type implemented by
 *                                 this module is allocated.
 * @GIMP_MODULE_STATE_LOAD_FAILED: gimp_module_register() returned %FALSE.
 * @GIMP_MODULE_STATE_NOT_LOADED:  There are no instances allocated of
 *                                 types implemented by this module.
 *
 * The possible states a #GimpModule can be in.
 **/
typedef enum
{
  GIMP_MODULE_STATE_ERROR,
  GIMP_MODULE_STATE_LOADED,
  GIMP_MODULE_STATE_LOAD_FAILED,
  GIMP_MODULE_STATE_NOT_LOADED
} GimpModuleState;


#define GIMP_MODULE_ERROR (gimp_module_error_quark ())

GQuark  gimp_module_error_quark (void) G_GNUC_CONST;

/**
 * GimpModuleError:
 * @GIMP_MODULE_FAILED: Generic error condition
 *
 * Types of errors returned by modules
 **/
typedef enum
{
  GIMP_MODULE_FAILED
} GimpModuleError;


/**
 * GimpModuleInfo:
 * @abi_version: The #GIMP_MODULE_ABI_VERSION the module was compiled against.
 * @purpose:     The module's general purpose.
 * @author:      The module's author.
 * @version:     The module's version.
 * @copyright:   The module's copyright.
 * @date:        The module's release date.
 *
 * This structure contains information about a loadable module.
 **/
struct _GimpModuleInfo
{
  guint32  abi_version;
  gchar   *purpose;
  gchar   *author;
  gchar   *version;
  gchar   *copyright;
  gchar   *date;
};


/**
 * GimpModuleQueryFunc:
 * @module:  The module responsible for this loadable module.
 *
 * The signature of the query function a loadable GIMP module must
 * implement. In the module, the function must be called [func@Module.query].
 *
 * [class@Module] will copy the returned [struct@ModuleInfo], so the
 * module doesn't need to keep these values around (however in most
 * cases the module will just return a pointer to a constant
 * structure).
 *
 * Returns: The info struct describing the module.
 **/
typedef const GimpModuleInfo * (* GimpModuleQueryFunc)    (GTypeModule *module);

/**
 * GimpModuleRegisterFunc:
 * @module:  The module responsible for this loadable module.
 *
 * The signature of the register function a loadable GIMP module must
 * implement.  In the module, the function must be called
 * [func@Module.register].
 *
 * When this function is called, the module should register all the types
 * it implements with the passed @module.
 *
 * Returns: Whether the registration was successful
 **/
typedef gboolean               (* GimpModuleRegisterFunc) (GTypeModule *module);


/* GimpModules have to implement these */
G_MODULE_EXPORT const GimpModuleInfo * gimp_module_query    (GTypeModule *module);
G_MODULE_EXPORT gboolean               gimp_module_register (GTypeModule *module);


#define GIMP_TYPE_MODULE (gimp_module_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpModule, gimp_module, GIMP, MODULE, GTypeModule)

struct _GimpModuleClass
{
  GTypeModuleClass  parent_class;

  void (* modified) (GimpModule *module);

  /* Padding for future expansion */
  void (* _gimp_reserved0) (void);
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
  void (* _gimp_reserved9) (void);
};


GimpModule           * gimp_module_new            (GFile           *file,
                                                   gboolean         auto_load,
                                                   gboolean         verbose);

GFile                * gimp_module_get_file       (GimpModule      *module);

void                   gimp_module_set_auto_load  (GimpModule      *module,
                                                   gboolean         auto_load);
gboolean               gimp_module_get_auto_load  (GimpModule      *module);

gboolean               gimp_module_is_on_disk     (GimpModule      *module);
gboolean               gimp_module_is_loaded      (GimpModule      *module);

const GimpModuleInfo * gimp_module_get_info       (GimpModule      *module);
GimpModuleState        gimp_module_get_state      (GimpModule      *module);
const gchar          * gimp_module_get_last_error (GimpModule      *module);

gboolean               gimp_module_query_module   (GimpModule      *module);


G_END_DECLS

#endif  /* __GIMP_MODULE_H__ */
