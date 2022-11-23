/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaConfig typedefs
 * Copyright (C) 2001-2002  Sven Neumann <sven@ligma.org>
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

#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__


#include "libligmaconfig/ligmaconfigtypes.h"

#include "config/config-enums.h"


#define LIGMA_OPACITY_TRANSPARENT      0.0
#define LIGMA_OPACITY_OPAQUE           1.0


typedef struct _LigmaGeglConfig       LigmaGeglConfig;
typedef struct _LigmaCoreConfig       LigmaCoreConfig;
typedef struct _LigmaDisplayConfig    LigmaDisplayConfig;
typedef struct _LigmaGuiConfig        LigmaGuiConfig;
typedef struct _LigmaDialogConfig     LigmaDialogConfig;
typedef struct _LigmaEarlyRc          LigmaEarlyRc;
typedef struct _LigmaPluginConfig     LigmaPluginConfig;
typedef struct _LigmaRc               LigmaRc;

typedef struct _LigmaXmlParser        LigmaXmlParser;

typedef struct _LigmaDisplayOptions   LigmaDisplayOptions;

/* should be in core/core-types.h */
typedef struct _LigmaGrid             LigmaGrid;
typedef struct _LigmaTemplate         LigmaTemplate;


/* for now these are defines, but can be turned into something
 * fancier for nicer debugging
 */
#define ligma_assert             g_assert
#define ligma_assert_not_reached g_assert_not_reached


#endif /* __CONFIG_TYPES_H__ */
