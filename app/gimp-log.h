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
  GIMP_LOG_TOOL_EVENTS    = 1 << 0,
  GIMP_LOG_TOOL_FOCUS     = 1 << 1,
  GIMP_LOG_DND            = 1 << 2,
  GIMP_LOG_HELP           = 1 << 3,
  GIMP_LOG_DIALOG_FACTORY = 1 << 4,
  GIMP_LOG_SAVE_DIALOG    = 1 << 5,
  GIMP_LOG_IMAGE_SCALE    = 1 << 6
} GimpLogFlags;


#ifdef G_HAVE_ISO_VARARGS

#define GIMP_LOG(type, ...) \
        G_STMT_START { \
        if (gimp_log_flags & GIMP_LOG_##type) \
          gimp_log (G_STRFUNC, __LINE__, #type, __VA_ARGS__); \
        } G_STMT_END

#elif defined(G_HAVE_GNUC_VARARGS)

#define GIMP_LOG(type, format...) \
        G_STMT_START { \
        if (gimp_log_flags & GIMP_LOG_##type) \
          gimp_log (G_STRFUNC, __LINE__, #type, format); \
        } G_STMT_END

#else /* no varargs macros */

static void
GIMP_LOG (const gchar *function,
          gint         line,
          const gchar *domain,
          const gchar *format,
          ...)
{
  va_list args;
  va_start (args, format);
  gimp_logv (function, line, domain, format, args);
  va_end (args);
}

#endif  /* !__GNUC__ */


extern GimpLogFlags gimp_log_flags;


void   gimp_log_init (void);
void   gimp_log      (const gchar *function,
                      gint         line,
                      const gchar *domain,
                      const gchar *format,
                      ...) G_GNUC_PRINTF (4, 5);
void   gimp_logv     (const gchar *function,
                      gint         line,
                      const gchar *domain,
                      const gchar *format,
                      va_list      args);


#endif /* __GIMP_LOG_H__ */
