/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpstock.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_STOCK_H__
#define __GIMP_STOCK_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


#define GIMP_STOCK_ANCHOR       "gimp-anchor"
#define GIMP_STOCK_DELETE       "gimp-delete"
#define GIMP_STOCK_DUPLICATE    "gimp-duplicate"
#define GIMP_STOCK_EDIT         "gimp-edit"
#define GIMP_STOCK_LINKED       "gimp-linked"
#define GIMP_STOCK_LOWER        "gimp-lower"
#define GIMP_STOCK_NEW          "gimp-new"
#define GIMP_STOCK_PASTE        "gimp-paste"
#define GIMP_STOCK_PASTE_AS_NEW "gimp-paste-as-new"
#define GIMP_STOCK_PASTE_INTO   "gimp-paste-into"
#define GIMP_STOCK_RAISE        "gimp-raise"
#define GIMP_STOCK_REFRESH      "gimp-refresh"
#define GIMP_STOCK_RESET        "gimp-reset"
#define GIMP_STOCK_STROKE       "gimp-stroke"
#define GIMP_STOCK_TO_PATH      "gimp-to-path"
#define GIMP_STOCK_TO_SELECTION "gimp-to-selection"
#define GIMP_STOCK_VISIBLE      "gimp-visible"
#define GIMP_STOCK_ZOOM_IN      "gimp-zoom-in"
#define GIMP_STOCK_ZOOM_OUT     "gimp-zoom-out"


void   gimp_stock_init (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_STOCK_H__ */
