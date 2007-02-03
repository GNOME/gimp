/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __PAINT_TYPES_H__
#define __PAINT_TYPES_H__


#include "core/core-types.h"
#include "paint/paint-enums.h"


/*  objects  */

typedef struct _GimpPaintCore     GimpPaintCore;
typedef struct _GimpPaintCoreUndo GimpPaintCoreUndo;
typedef struct _GimpBrushCore     GimpBrushCore;
typedef struct _GimpInkUndo       GimpInkUndo;

typedef struct _GimpPaintOptions  GimpPaintOptions;


/*  functions  */

typedef void (* GimpPaintRegisterCallback) (Gimp        *gimp,
                                            GType        paint_type,
                                            GType        paint_options_type,
                                            const gchar *identifier,
                                            const gchar *blurb,
                                            const gchar *stock_id);

typedef void (* GimpPaintRegisterFunc)     (Gimp                      *gimp,
                                            GimpPaintRegisterCallback  callback);


#endif /* __PAINT_TYPES_H__ */
