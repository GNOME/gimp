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

#pragma once


typedef guint *GimpLogHandler;


typedef enum
{
  GIMP_LOG_TOOL_EVENTS        = 1 << 0,
  GIMP_LOG_TOOL_FOCUS         = 1 << 1,
  GIMP_LOG_DND                = 1 << 2,
  GIMP_LOG_HELP               = 1 << 3,
  GIMP_LOG_DIALOG_FACTORY     = 1 << 4,
  GIMP_LOG_MENUS              = 1 << 5,
  GIMP_LOG_SAVE_DIALOG        = 1 << 6,
  GIMP_LOG_IMAGE_SCALE        = 1 << 7,
  GIMP_LOG_SHADOW_TILES       = 1 << 8,
  GIMP_LOG_SCALE              = 1 << 9,
  GIMP_LOG_WM                 = 1 << 10,
  GIMP_LOG_FLOATING_SELECTION = 1 << 11,
  GIMP_LOG_SHM                = 1 << 12,
  GIMP_LOG_TEXT_EDITING       = 1 << 13,
  GIMP_LOG_KEY_EVENTS         = 1 << 14,
  GIMP_LOG_AUTO_TAB_STYLE     = 1 << 15,
  GIMP_LOG_INSTANCES          = 1 << 16,
  GIMP_LOG_RECTANGLE_TOOL     = 1 << 17,
  GIMP_LOG_BRUSH_CACHE        = 1 << 18,
  GIMP_LOG_PROJECTION         = 1 << 19,
  GIMP_LOG_XCF                = 1 << 20,
  GIMP_LOG_MAGIC_MATCH        = 1 << 21
} GimpLogFlags;


extern GimpLogFlags gimp_log_flags;


void             gimp_log_init           (void);
void             gimp_log                (GimpLogFlags    flags,
                                          const gchar    *function,
                                          gint            line,
                                          const gchar    *format,
                                          ...) G_GNUC_PRINTF (4, 5);
void             gimp_logv               (GimpLogFlags    flags,
                                          const gchar    *function,
                                          gint            line,
                                          const gchar    *format,
                                          va_list         args) G_GNUC_PRINTF (4, 0);

GimpLogHandler   gimp_log_set_handler    (gboolean        global,
                                          GLogLevelFlags  log_levels,
                                          GLogFunc        log_func,
                                          gpointer        user_data);
void             gimp_log_remove_handler (GimpLogHandler  handler);


#ifdef G_HAVE_ISO_VARARGS

#define GIMP_LOG(type, ...) \
        G_STMT_START { \
        if (gimp_log_flags & GIMP_LOG_##type) \
          gimp_log (GIMP_LOG_##type, G_STRFUNC, __LINE__, __VA_ARGS__);       \
        } G_STMT_END

#elif defined(G_HAVE_GNUC_VARARGS)

#define GIMP_LOG(type, format...) \
        G_STMT_START { \
        if (gimp_log_flags & GIMP_LOG_##type) \
          gimp_log (GIMP_LOG_##type, G_STRFUNC, __LINE__, format);  \
        } G_STMT_END

#else /* no varargs macros */

/* need to expand all the short forms
 * to make them known constants at compile time
 */
#define TOOL_EVENTS        GIMP_LOG_TOOL_EVENTS
#define TOOL_FOCUS         GIMP_LOG_TOOL_FOCUS
#define DND                GIMP_LOG_DND
#define HELP               GIMP_LOG_HELP
#define DIALOG_FACTORY     GIMP_LOG_DIALOG_FACTORY
#define MENUS              GIMP_LOG_MENUS
#define SAVE_DIALOG        GIMP_LOG_SAVE_DIALOG
#define IMAGE_SCALE        GIMP_LOG_IMAGE_SCALE
#define SHADOW_TILES       GIMP_LOG_SHADOW_TILES
#define SCALE              GIMP_LOG_SCALE
#define WM                 GIMP_LOG_WM
#define FLOATING_SELECTION GIMP_LOG_FLOATING_SELECTION
#define SHM                GIMP_LOG_SHM
#define TEXT_EDITING       GIMP_LOG_TEXT_EDITING
#define KEY_EVENTS         GIMP_LOG_KEY_EVENTS
#define AUTO_TAB_STYLE     GIMP_LOG_AUTO_TAB_STYLE
#define INSTANCES          GIMP_LOG_INSTANCES
#define RECTANGLE_TOOL     GIMP_LOG_RECTANGLE_TOOL
#define BRUSH_CACHE        GIMP_LOG_BRUSH_CACHE
#define PROJECTION         GIMP_LOG_PROJECTION
#define XCF                GIMP_LOG_XCF

#if 0 /* last resort */
#  define GIMP_LOG /* nothing => no varargs, no log */
#endif

static void
GIMP_LOG (GimpLogFlags flags,
          const gchar *format,
          ...)
{
  va_list args;
  va_start (args, format);
  if (gimp_log_flags & flags)
    gimp_logv (type, "", 0, format, args);
  va_end (args);
}

#endif  /* !__GNUC__ */

#define geimnum(vienna)  gimp_l##vienna##l_dialog()
#define fnord(kosmoso)   void gimp_##kosmoso##bl_dialog(void);
