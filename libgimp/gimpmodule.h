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

#ifndef __GIMP_MODULE_H__
#define __GIMP_MODULE_H__

#include <gmodule.h>

G_BEGIN_DECLS

/* For information look at the html documentation */


typedef struct _GimpModuleInfo GimpModuleInfo;

struct _GimpModuleInfo
{
  const gchar *purpose;
  const gchar *author;
  const gchar *version;
  const gchar *copyright;
  const gchar *date;
};


typedef gboolean (* GimpModuleRegisterFunc) (GTypeModule     *module,
                                             GimpModuleInfo **module_info);


G_END_DECLS

#endif /* __GIMP_MODULE_H__ */
