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

#ifndef __GIMP_BASE_H__
#define __GIMP_BASE_H__

#define __GIMP_BASE_H_INSIDE__

#include <gegl.h>

#include <libgimpbase/gimpbasetypes.h>

#include <libgimpbase/gimpchecks.h>
#include <libgimpbase/gimpchoice.h>
#include <libgimpbase/gimpcpuaccel.h>
#include <libgimpbase/gimpenv.h>
#include <libgimpbase/gimpexportoptions.h>
#include <libgimpbase/gimplimits.h>
#include <libgimpbase/gimpmemsize.h>
#include <libgimpbase/gimpmetadata.h>
#include <libgimpbase/gimpparamspecs.h>
#include <libgimpbase/gimpparasite.h>
#include <libgimpbase/gimprectangle.h>
#include <libgimpbase/gimpunit.h>
#include <libgimpbase/gimputils.h>
#include <libgimpbase/gimpversion.h>
#include <libgimpbase/gimpvaluearray.h>

#ifndef G_OS_WIN32
#include <libgimpbase/gimpsignal.h>
#endif

#undef __GIMP_BASE_H_INSIDE__

#endif  /* __GIMP_BASE_H__ */
