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

#ifndef __GIMP_CONFIG_H__
#define __GIMP_CONFIG_H__

#include <gegl.h>

#include <libgimpbase/gimpbase.h>

#define __GIMP_CONFIG_H_INSIDE__

#include <libgimpconfig/gimpconfigtypes.h>

#include <libgimpconfig/gimpconfig-deserialize.h>
#include <libgimpconfig/gimpconfig-error.h>
#include <libgimpconfig/gimpconfig-iface.h>
#include <libgimpconfig/gimpconfig-params.h>
#include <libgimpconfig/gimpconfig-path.h>
#include <libgimpconfig/gimpconfig-register.h>
#include <libgimpconfig/gimpconfig-serialize.h>
#include <libgimpconfig/gimpconfig-utils.h>
#include <libgimpconfig/gimpconfigwriter.h>
#include <libgimpconfig/gimpscanner.h>

#include <libgimpconfig/gimpcolorconfig.h>

#undef __GIMP_CONFIG_H_INSIDE__

#endif  /* __GIMP_CONFIG_H__ */
