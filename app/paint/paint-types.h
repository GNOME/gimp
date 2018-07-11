/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __PAINT_TYPES_H__
#define __PAINT_TYPES_H__


#include "core/core-types.h"
#include "paint/paint-enums.h"


/*  paint cores  */

typedef struct _GimpPaintCore        GimpPaintCore;
typedef struct _GimpBrushCore        GimpBrushCore;
typedef struct _GimpSourceCore       GimpSourceCore;

typedef struct _GimpAirbrush         GimpAirbrush;
typedef struct _GimpClone            GimpClone;
typedef struct _GimpConvolve         GimpConvolve;
typedef struct _GimpDodgeBurn        GimpDodgeBurn;
typedef struct _GimpEraser           GimpEraser;
typedef struct _GimpHeal             GimpHeal;
typedef struct _GimpInk              GimpInk;
typedef struct _GimpMybrushCore      GimpMybrushCore;
typedef struct _GimpPaintbrush       GimpPaintbrush;
typedef struct _GimpPencil           GimpPencil;
typedef struct _GimpPerspectiveClone GimpPerspectiveClone;
typedef struct _GimpSmudge           GimpSmudge;


/*  paint options  */

typedef struct _GimpPaintOptions            GimpPaintOptions;
typedef struct _GimpSourceOptions           GimpSourceOptions;

typedef struct _GimpAirbrushOptions         GimpAirbrushOptions;
typedef struct _GimpCloneOptions            GimpCloneOptions;
typedef struct _GimpConvolveOptions         GimpConvolveOptions;
typedef struct _GimpDodgeBurnOptions        GimpDodgeBurnOptions;
typedef struct _GimpEraserOptions           GimpEraserOptions;
typedef struct _GimpInkOptions              GimpInkOptions;
typedef struct _GimpMybrushOptions          GimpMybrushOptions;
typedef struct _GimpPencilOptions           GimpPencilOptions;
typedef struct _GimpPerspectiveCloneOptions GimpPerspectiveCloneOptions;
typedef struct _GimpSmudgeOptions           GimpSmudgeOptions;


/*  functions  */

typedef void (* GimpPaintRegisterCallback) (Gimp        *gimp,
                                            GType        paint_type,
                                            GType        paint_options_type,
                                            const gchar *identifier,
                                            const gchar *blurb,
                                            const gchar *icon_name);

typedef void (* GimpPaintRegisterFunc)     (Gimp                      *gimp,
                                            GimpPaintRegisterCallback  callback);


#endif /* __PAINT_TYPES_H__ */
