/* LIGMA - The GNU Image Manipulation Program
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

typedef struct _LigmaPaintCore        LigmaPaintCore;
typedef struct _LigmaBrushCore        LigmaBrushCore;
typedef struct _LigmaSourceCore       LigmaSourceCore;

typedef struct _LigmaAirbrush         LigmaAirbrush;
typedef struct _LigmaClone            LigmaClone;
typedef struct _LigmaConvolve         LigmaConvolve;
typedef struct _LigmaDodgeBurn        LigmaDodgeBurn;
typedef struct _LigmaEraser           LigmaEraser;
typedef struct _LigmaHeal             LigmaHeal;
typedef struct _LigmaInk              LigmaInk;
typedef struct _LigmaMybrushCore      LigmaMybrushCore;
typedef struct _LigmaPaintbrush       LigmaPaintbrush;
typedef struct _LigmaPencil           LigmaPencil;
typedef struct _LigmaPerspectiveClone LigmaPerspectiveClone;
typedef struct _LigmaSmudge           LigmaSmudge;


/*  paint options  */

typedef struct _LigmaPaintOptions            LigmaPaintOptions;
typedef struct _LigmaSourceOptions           LigmaSourceOptions;

typedef struct _LigmaAirbrushOptions         LigmaAirbrushOptions;
typedef struct _LigmaCloneOptions            LigmaCloneOptions;
typedef struct _LigmaConvolveOptions         LigmaConvolveOptions;
typedef struct _LigmaDodgeBurnOptions        LigmaDodgeBurnOptions;
typedef struct _LigmaEraserOptions           LigmaEraserOptions;
typedef struct _LigmaInkOptions              LigmaInkOptions;
typedef struct _LigmaMybrushOptions          LigmaMybrushOptions;
typedef struct _LigmaPencilOptions           LigmaPencilOptions;
typedef struct _LigmaPerspectiveCloneOptions LigmaPerspectiveCloneOptions;
typedef struct _LigmaSmudgeOptions           LigmaSmudgeOptions;


/*  functions  */

typedef void (* LigmaPaintRegisterCallback) (Ligma        *ligma,
                                            GType        paint_type,
                                            GType        paint_options_type,
                                            const gchar *identifier,
                                            const gchar *blurb,
                                            const gchar *icon_name);

typedef void (* LigmaPaintRegisterFunc)     (Ligma                      *ligma,
                                            LigmaPaintRegisterCallback  callback);


#endif /* __PAINT_TYPES_H__ */
