/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_MODULE_TYPES_H__
#define __GIMP_MODULE_TYPES_H__


#include <libgimpbase/gimpbasetypes.h>


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#ifndef GIMP_DISABLE_DEPRECATED
/*
 * GIMP_MODULE_PARAM_SERIALIZE is deprecated, use
 * GIMP_CONFIG_PARAM_SERIALIZE instead.
 */
#define GIMP_MODULE_PARAM_SERIALIZE (1 << (0 + G_PARAM_USER_SHIFT))
#endif


typedef struct _GimpModule     GimpModule;
typedef struct _GimpModuleInfo GimpModuleInfo;
typedef struct _GimpModuleDB   GimpModuleDB;


G_END_DECLS

#endif  /* __GIMP_MODULE_TYPES_H__ */
