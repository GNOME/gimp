/* LIBLIGMA - The LIGMA Library
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

#ifndef __LIGMA_BASE_H__
#define __LIGMA_BASE_H__

#define __LIGMA_BASE_H_INSIDE__

#include <libligmabase/ligmabasetypes.h>

#include <libligmabase/ligmachecks.h>
#include <libligmabase/ligmacpuaccel.h>
#include <libligmabase/ligmaenv.h>
#include <libligmabase/ligmalimits.h>
#include <libligmabase/ligmamemsize.h>
#include <libligmabase/ligmametadata.h>
#include <libligmabase/ligmaparamspecs.h>
#include <libligmabase/ligmaparasite.h>
#include <libligmabase/ligmarectangle.h>
#include <libligmabase/ligmaunit.h>
#include <libligmabase/ligmautils.h>
#include <libligmabase/ligmaversion.h>
#include <libligmabase/ligmavaluearray.h>

#ifndef G_OS_WIN32
#include <libligmabase/ligmasignal.h>
#endif

#undef __LIGMA_BASE_H_INSIDE__

#endif  /* __LIGMA_BASE_H__ */
