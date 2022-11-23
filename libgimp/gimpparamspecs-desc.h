/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaparamspecs-desc.h
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

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_PARAM_SPECS_DESC_H__
#define __LIGMA_PARAM_SPECS_DESC_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


gchar * ligma_param_spec_get_desc (GParamSpec *pspec);


G_END_DECLS

#endif  /*  __LIGMA_PARAM_SPECS_DESC_H__  */
