/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpConfig typedefs
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
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

#pragma once

#include "libgimpconfig/gimpconfigtypes.h"

#include "config/config-enums.h"


#define GIMP_OPACITY_TRANSPARENT      0.0
#define GIMP_OPACITY_OPAQUE           1.0


typedef struct _GimpGeglConfig       GimpGeglConfig;
typedef struct _GimpCoreConfig       GimpCoreConfig;
typedef struct _GimpDisplayConfig    GimpDisplayConfig;
typedef struct _GimpGuiConfig        GimpGuiConfig;
typedef struct _GimpDialogConfig     GimpDialogConfig;
typedef struct _GimpEarlyRc          GimpEarlyRc;
typedef struct _GimpPluginConfig     GimpPluginConfig;
typedef struct _GimpRc               GimpRc;

typedef struct _GimpXmlParser        GimpXmlParser;

typedef struct _GimpDisplayOptions   GimpDisplayOptions;

/* should be in core/core-types.h */
typedef struct _GimpGrid             GimpGrid;
typedef struct _GimpTemplate         GimpTemplate;


/* for now these are defines, but can be turned into something
 * fancier for nicer debugging
 */
#define gimp_assert             g_assert
#define gimp_assert_not_reached g_assert_not_reached


#if ! GLIB_CHECK_VERSION(2, 76, 0)
/* Function copied from GLib 2.76.0 and made available privately so that
 * we don't have to bump our dependency requirement.
 *
 * TODO: remove this when GLib requirement moves over 2.76.
 */
static inline gboolean g_set_str (char       **str_pointer,
                                  const char  *new_str);

static inline gboolean
g_set_str (char       **str_pointer,
           const char  *new_str)
{
  char *copy;

  if (*str_pointer == new_str ||
      (*str_pointer && new_str && strcmp (*str_pointer, new_str) == 0))
    return FALSE;

  copy = g_strdup (new_str);
  g_free (*str_pointer);
  *str_pointer = copy;

  return TRUE;
}
#endif
