/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmodule.h (C) 1999 Austin Donnelly <austin@greenend.org.uk>
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

#ifndef __GIMPMODULE_H__
#define  __GIMPMODULE_H__

#include <gmodule.h>


typedef enum {
  GIMP_MODULE_OK,
  GIMP_MODULE_UNLOAD
} GimpModuleStatus;



typedef struct {
  void *shutdown_data;
  const char *purpose;
  const char *author;
  const char *version;
  const char *copyright;
  const char *date;
} GimpModuleInfo;



/* GIMP modules should G_MODULE_EXPORT a function named "module_init"
 * of the following type: */
typedef GimpModuleStatus (GimpModuleInitFunc) (GimpModuleInfo **);

#ifndef MODULE_COMPILATION	/* On Win32 this declaration clashes with
				 * the definition (which uses G_MODULE_EXPORT)
				 * and thus should be bypassed when compiling
				 * the module itself.
				 */
GimpModuleInitFunc module_init;
#endif

/* This function is called by the GIMP at startup, and should return
 * either GIMP_MODULE_OK if it sucessfully initialised or
 * GIMP_MODULE_UNLOAD if the module failed to hook whatever functions
 * it wanted.  GIMP_MODULE_UNLOAD causes the module to be closed, so
 * the module must not have registered any internal functions or given
 * out pointers to its data to anyone.
 *
 * If the module returns GIMP_MODULE_OK, it should also return a
 * GimpModuleInfo structure describing itself.
 */


/* If GIMP modules want to allow themselves to be unloaded, they
 * should G_MODULE_EXPORT a function named "module_unload" with the
 * following type: */
typedef void (GimpModuleUnloadFunc) (void *shutdown_data,
				      void (*completed_cb)(void *),
				      void *completed_data);

#ifndef MODULE_COMPILATION	/* The same as for module_init */

GimpModuleUnloadFunc module_unload;
#endif

/* GIMP calls this unload request function to ask a module to
 * prepare itself to be unloaded.  It is called with the value of
 * shutdown_data supplied in the GimpModuleInfo struct.  The module
 * should ensure that none of its code or data are being used, and
 * then call the supplied "completed_cb" callback function with the
 * data provided.  Typically the shutdown request function will queue
 * de-registration activities then return.  Only when the
 * de-registration has finished should the completed_cb be invoked. */



#endif /* __GIMPMODULE_H__ */
