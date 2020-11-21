/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimp-debug.h
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

#ifndef __GIMP_DEBUG_H__
#define __GIMP_DEBUG_H__

G_BEGIN_DECLS


typedef enum
{
  GIMP_DEBUG_PID            = 1 << 0,
  GIMP_DEBUG_FATAL_WARNINGS = 1 << 1,
  GIMP_DEBUG_QUERY          = 1 << 2,
  GIMP_DEBUG_INIT           = 1 << 3,
  GIMP_DEBUG_RUN            = 1 << 4,
  GIMP_DEBUG_QUIT           = 1 << 5,
  GIMP_DEBUG_FATAL_CRITICALS = 1 << 6,

} GimpDebugFlag;


void   _gimp_debug_init  (const gchar *basename);
void   _gimp_debug_configure (GimpStackTraceMode stack_trace_mode);
guint  _gimp_get_debug_flags (void);
void   _gimp_debug_stop  (void);


G_END_DECLS

#endif /* __GIMP_DEBUG_H__ */
