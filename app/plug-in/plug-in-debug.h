/* The GIMP -- an image manipulation program
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

#ifndef __PLUG_IN_DEBUG_H__
#define __PLUG_IN_DEBUG_H__


typedef enum
{
  GIMP_DEBUG_WRAP_QUERY = 1 << 0,
  GIMP_DEBUG_WRAP_INIT  = 1 << 1,
  GIMP_DEBUG_WRAP_RUN   = 1 << 2,

  GIMP_DEBUG_WRAP_DEFAULT = GIMP_DEBUG_WRAP_RUN
} GimpDebugWrapFlag;


void       plug_in_debug_init  (Gimp               *gimp);
void       plug_in_debug_exit  (Gimp               *gimp);
                                                                                
gchar    **plug_in_debug_argv  (Gimp               *gimp,
                                const gchar        *name,
                                GimpDebugWrapFlag   flag,
                                gchar             **args);


#endif /* __PLUG_IN_DEBUG_H__ */
