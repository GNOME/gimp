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
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_MODULE_H__
#define __GIMP_MODULE_H__

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
#define GIMP_MODULE_ABI_VERSION 0x0004


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

typedef enum
{
  GIMP_MODULE_FAILED             /* generic error condition */
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
 * @module:  The #GimpModule responsible for this loadable module.
 * @Returns: The #GimpModuleInfo struct describing the module.
 *
 * The signature of the query function a loadable GIMP module must
 * implement.  In the module, the function must be called
 * gimp_module_query().
 *
 * #GimpModule will copy the returned #GimpModuleInfo struct, so the
 * module doesn't need to keep these values around (however in most
 * cases the module will just return a pointer to a constant
 * structure).
 **/
typedef const GimpModuleInfo * (* GimpModuleQueryFunc)    (GTypeModule *module);

/**
 * GimpModuleRegisterFunc:
 * @module:  The #GimpModule responsible for this loadable module.
 * @Returns: %TRUE on success, %FALSE otherwise.
 *
 * The signature of the register function a loadable GIMP module must
 * implement.  In the module, the function must be called
 * gimp_module_register().
 *
 * When this function is called, the module should register all the types
 * it implements with the passed @module.
 **/
typedef gboolean               (* GimpModuleRegisterFunc) (GTypeModule *module);


/* GimpModules have to implement these */
G_MODULE_EXPORT const GimpModuleInfo * gimp_module_query    (GTypeModule *module);
G_MODULE_EXPORT gboolean               gimp_module_register (GTypeModule *module);


#define GIMP_TYPE_MODULE            (gimp_module_get_type ())
#define GIMP_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MODULE, GimpModule))
#define GIMP_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MODULE, GimpModuleClass))
#define GIMP_IS_MODULE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MODULE))
#define GIMP_IS_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MODULE))
#define GIMP_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MODULE, GimpModuleClass))


typedef struct _GimpModuleClass GimpModuleClass;

/**
 * GimpModule:
 * @filename:
 * @verbose:
 * @state:
 * @on_disk:
 * @load_inhibit:
 * @info:
 * @last_module_error:
 *
 * #GimpModule is a generic mechanism to dynamically load modules into
 * GIMP.  It is a #GTypeModule subclass, implementing module loading
 * using #GModule.  #GimpModule does not know which functionality is
 * implemented by the modules, it just provides a framework to get
 * arbitrary #GType implementations loaded from disk.
 **/
struct _GimpModule
{
  GTypeModule      parent_instance;

  /*< public >*/
  gchar           *filename;     /* path to the module                       */
  gboolean         verbose;      /* verbose error reporting                  */
  GimpModuleState  state;        /* what's happened to the module            */
  gboolean         on_disk;      /* TRUE if file still exists                */
  gboolean         load_inhibit; /* user requests not to load at boot time   */

  /* stuff from now on may be NULL depending on the state the module is in   */
  /*< private >*/
  GModule         *module;       /* handle on the module                     */

  /*< public >*/
  GimpModuleInfo  *info;         /* returned values from module_query        */
  gchar           *last_module_error;

  /*< private >*/
  GimpModuleQueryFunc     query_module;
  GimpModuleRegisterFunc  register_module;
};

struct _GimpModuleClass
{
  GTypeModuleClass  parent_class;

  void (* modified) (GimpModule *module);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType         gimp_module_get_type         (void) G_GNUC_CONST;

GimpModule  * gimp_module_new              (const gchar     *filename,
                                            gboolean         load_inhibit,
                                            gboolean         verbose);

gboolean      gimp_module_query_module     (GimpModule      *module);

void          gimp_module_modified         (GimpModule      *module);
void          gimp_module_set_load_inhibit (GimpModule      *module,
                                            gboolean         load_inhibit);

const gchar * gimp_module_state_name       (GimpModuleState  state);


/*  GimpModuleInfo functions  */

GimpModuleInfo * gimp_module_info_new  (guint32               abi_version,
                                        const gchar          *purpose,
                                        const gchar          *author,
                                        const gchar          *version,
                                        const gchar          *copyright,
                                        const gchar          *date);
GimpModuleInfo * gimp_module_info_copy (const GimpModuleInfo *info);
void             gimp_module_info_free (GimpModuleInfo       *info);


G_END_DECLS

#endif  /* __GIMP_MODULE_H__ */
