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
#define __GIMPMODULE_H__

#include <gmodule.h>

/* For information look at the html documentation */

typedef enum
{
  GIMP_MODULE_OK,
  GIMP_MODULE_UNLOAD
} GimpModuleStatus;


typedef struct _GimpModuleInfo GimpModuleInfo;

struct _GimpModuleInfo
{
  gpointer     shutdown_data;

  const gchar *purpose;
  const gchar *author;
  const gchar *version;
  const gchar *copyright;
  const gchar *date;
};


/*  Module initialization  */

typedef GimpModuleStatus (* GimpModuleInitFunc) (GimpModuleInfo **module_info);

#ifndef MODULE_COMPILATION

/*  On Win32 this declaration clashes with the definition
 *  (which uses G_MODULE_EXPORT) and thus should be bypassed
 *  when compiling the module itself.
 */
GimpModuleInitFunc module_init;

#endif /* ! MODULE_COMPILATION */


/*  Module unload  */

typedef void (* GimpModuleCompletedCB) (gpointer               completed_data);

typedef void (* GimpModuleUnloadFunc)  (gpointer               shutdown_data,
					GimpModuleCompletedCB  completed_cb,
					gpointer               completed_data);

#ifndef MODULE_COMPILATION

/*  The same as for module_init.
 */
GimpModuleUnloadFunc module_unload;

#endif /* ! MODULE_COMPILATION */

#endif /* __GIMPMODULE_H__ */
