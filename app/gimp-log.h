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

#ifndef __GIMP_LOG_H__
#define __GIMP_LOG_H__


typedef enum
{
  GIMP_LOG_TOOLS = 1 << 0
} GimpLogFlags;


#define GIMP_LOG(type, format...) \
        G_STMT_START { \
        if (gimp_log_flags & GIMP_LOG_##type) \
          gimp_log (G_STRFUNC, __LINE__, #type, format); \
        } G_STMT_END


extern GimpLogFlags gimp_log_flags;


void   gimp_log_init (void);
void   gimp_log      (const gchar *function,
                      gint         line,
                      const gchar *domain,
                      const gchar *format,
                      ...) G_GNUC_PRINTF (4, 5);


#endif /* __GIMP_LOG_H__ */
