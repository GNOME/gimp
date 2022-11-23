/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugindebug.h
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

#ifndef __LIGMA_PLUG_IN_DEBUG_H__
#define __LIGMA_PLUG_IN_DEBUG_H__


typedef enum
{
  LIGMA_DEBUG_WRAP_QUERY = 1 << 0,
  LIGMA_DEBUG_WRAP_INIT  = 1 << 1,
  LIGMA_DEBUG_WRAP_RUN   = 1 << 2,

  LIGMA_DEBUG_WRAP_DEFAULT = LIGMA_DEBUG_WRAP_RUN
} LigmaDebugWrapFlag;


LigmaPlugInDebug  * ligma_plug_in_debug_new  (void);
void               ligma_plug_in_debug_free (LigmaPlugInDebug    *debug);

gchar           ** ligma_plug_in_debug_argv (LigmaPlugInDebug    *debug,
                                            const gchar        *name,
                                            LigmaDebugWrapFlag   flag,
                                            const gchar       **args);


#endif /* __LIGMA_PLUG_IN_DEBUG_H__ */
