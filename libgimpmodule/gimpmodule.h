/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmamodule.h
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

#ifndef __LIGMA_MODULE_H__
#define __LIGMA_MODULE_H__

#include <gio/gio.h>
#include <gmodule.h>

#define __LIGMA_MODULE_H_INSIDE__

#include <libligmamodule/ligmamoduletypes.h>

#include <libligmamodule/ligmamoduledb.h>

#undef __LIGMA_MODULE_H_INSIDE__

G_BEGIN_DECLS


/**
 * LIGMA_MODULE_ABI_VERSION:
 *
 * The version of the module system's ABI. Modules put this value into
 * #LigmaModuleInfo's @abi_version field so the code loading the modules
 * can check if it was compiled against the same module ABI the modules
 * are compiled against.
 *
 *  LIGMA_MODULE_ABI_VERSION is incremented each time one of the
 *  following changes:
 *
 *  - the libligmamodule implementation (if the change affects modules).
 *
 *  - one of the classes implemented by modules (currently #LigmaColorDisplay,
 *    #LigmaColorSelector and #LigmaController).
 **/
#define LIGMA_MODULE_ABI_VERSION 0x0005


/**
 * LigmaModuleState:
 * @LIGMA_MODULE_STATE_ERROR:       Missing ligma_module_register() function
 *                                 or other error.
 * @LIGMA_MODULE_STATE_LOADED:      An instance of a type implemented by
 *                                 this module is allocated.
 * @LIGMA_MODULE_STATE_LOAD_FAILED: ligma_module_register() returned %FALSE.
 * @LIGMA_MODULE_STATE_NOT_LOADED:  There are no instances allocated of
 *                                 types implemented by this module.
 *
 * The possible states a #LigmaModule can be in.
 **/
typedef enum
{
  LIGMA_MODULE_STATE_ERROR,
  LIGMA_MODULE_STATE_LOADED,
  LIGMA_MODULE_STATE_LOAD_FAILED,
  LIGMA_MODULE_STATE_NOT_LOADED
} LigmaModuleState;


#define LIGMA_MODULE_ERROR (ligma_module_error_quark ())

GQuark  ligma_module_error_quark (void) G_GNUC_CONST;

/**
 * LigmaModuleError:
 * @LIGMA_MODULE_FAILED: Generic error condition
 *
 * Types of errors returned by modules
 **/
typedef enum
{
  LIGMA_MODULE_FAILED
} LigmaModuleError;


/**
 * LigmaModuleInfo:
 * @abi_version: The #LIGMA_MODULE_ABI_VERSION the module was compiled against.
 * @purpose:     The module's general purpose.
 * @author:      The module's author.
 * @version:     The module's version.
 * @copyright:   The module's copyright.
 * @date:        The module's release date.
 *
 * This structure contains information about a loadable module.
 **/
struct _LigmaModuleInfo
{
  guint32  abi_version;
  gchar   *purpose;
  gchar   *author;
  gchar   *version;
  gchar   *copyright;
  gchar   *date;
};


/**
 * LigmaModuleQueryFunc:
 * @module:  The module responsible for this loadable module.
 *
 * The signature of the query function a loadable LIGMA module must
 * implement. In the module, the function must be called [func@Module.query].
 *
 * [class@Module] will copy the returned [struct@ModuleInfo], so the
 * module doesn't need to keep these values around (however in most
 * cases the module will just return a pointer to a constant
 * structure).
 *
 * Returns: The info struct describing the module.
 **/
typedef const LigmaModuleInfo * (* LigmaModuleQueryFunc)    (GTypeModule *module);

/**
 * LigmaModuleRegisterFunc:
 * @module:  The module responsible for this loadable module.
 *
 * The signature of the register function a loadable LIGMA module must
 * implement.  In the module, the function must be called
 * [func@Module.register].
 *
 * When this function is called, the module should register all the types
 * it implements with the passed @module.
 *
 * Returns: Whether the registration was succesfull
 **/
typedef gboolean               (* LigmaModuleRegisterFunc) (GTypeModule *module);


/* LigmaModules have to implement these */
G_MODULE_EXPORT const LigmaModuleInfo * ligma_module_query    (GTypeModule *module);
G_MODULE_EXPORT gboolean               ligma_module_register (GTypeModule *module);


#define LIGMA_TYPE_MODULE            (ligma_module_get_type ())
#define LIGMA_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MODULE, LigmaModule))
#define LIGMA_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MODULE, LigmaModuleClass))
#define LIGMA_IS_MODULE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MODULE))
#define LIGMA_IS_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MODULE))
#define LIGMA_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MODULE, LigmaModuleClass))


typedef struct _LigmaModulePrivate LigmaModulePrivate;
typedef struct _LigmaModuleClass   LigmaModuleClass;

struct _LigmaModule
{
  GTypeModule        parent_instance;

  LigmaModulePrivate *priv;
};

struct _LigmaModuleClass
{
  GTypeModuleClass  parent_class;

  void (* modified) (LigmaModule *module);

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType                  ligma_module_get_type       (void) G_GNUC_CONST;

LigmaModule           * ligma_module_new            (GFile           *file,
                                                   gboolean         auto_load,
                                                   gboolean         verbose);

GFile                * ligma_module_get_file       (LigmaModule      *module);

void                   ligma_module_set_auto_load  (LigmaModule      *module,
                                                   gboolean         auto_load);
gboolean               ligma_module_get_auto_load  (LigmaModule      *module);

gboolean               ligma_module_is_on_disk     (LigmaModule      *module);
gboolean               ligma_module_is_loaded      (LigmaModule      *module);

const LigmaModuleInfo * ligma_module_get_info       (LigmaModule      *module);
LigmaModuleState        ligma_module_get_state      (LigmaModule      *module);
const gchar          * ligma_module_get_last_error (LigmaModule      *module);

gboolean               ligma_module_query_module   (LigmaModule      *module);


G_END_DECLS

#endif  /* __LIGMA_MODULE_H__ */
