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

#ifndef __LIGMA_LOG_H__
#define __LIGMA_LOG_H__


typedef guint *LigmaLogHandler;


typedef enum
{
  LIGMA_LOG_TOOL_EVENTS        = 1 << 0,
  LIGMA_LOG_TOOL_FOCUS         = 1 << 1,
  LIGMA_LOG_DND                = 1 << 2,
  LIGMA_LOG_HELP               = 1 << 3,
  LIGMA_LOG_DIALOG_FACTORY     = 1 << 4,
  LIGMA_LOG_MENUS              = 1 << 5,
  LIGMA_LOG_SAVE_DIALOG        = 1 << 6,
  LIGMA_LOG_IMAGE_SCALE        = 1 << 7,
  LIGMA_LOG_SHADOW_TILES       = 1 << 8,
  LIGMA_LOG_SCALE              = 1 << 9,
  LIGMA_LOG_WM                 = 1 << 10,
  LIGMA_LOG_FLOATING_SELECTION = 1 << 11,
  LIGMA_LOG_SHM                = 1 << 12,
  LIGMA_LOG_TEXT_EDITING       = 1 << 13,
  LIGMA_LOG_KEY_EVENTS         = 1 << 14,
  LIGMA_LOG_AUTO_TAB_STYLE     = 1 << 15,
  LIGMA_LOG_INSTANCES          = 1 << 16,
  LIGMA_LOG_RECTANGLE_TOOL     = 1 << 17,
  LIGMA_LOG_BRUSH_CACHE        = 1 << 18,
  LIGMA_LOG_PROJECTION         = 1 << 19,
  LIGMA_LOG_XCF                = 1 << 20,
  LIGMA_LOG_MAGIC_MATCH        = 1 << 21
} LigmaLogFlags;


extern LigmaLogFlags ligma_log_flags;


void             ligma_log_init           (void);
void             ligma_log                (LigmaLogFlags    flags,
                                          const gchar    *function,
                                          gint            line,
                                          const gchar    *format,
                                          ...) G_GNUC_PRINTF (4, 5);
void             ligma_logv               (LigmaLogFlags    flags,
                                          const gchar    *function,
                                          gint            line,
                                          const gchar    *format,
                                          va_list         args) G_GNUC_PRINTF (4, 0);

LigmaLogHandler   ligma_log_set_handler    (gboolean        global,
                                          GLogLevelFlags  log_levels,
                                          GLogFunc        log_func,
                                          gpointer        user_data);
void             ligma_log_remove_handler (LigmaLogHandler  handler);


#ifdef G_HAVE_ISO_VARARGS

#define LIGMA_LOG(type, ...) \
        G_STMT_START { \
        if (ligma_log_flags & LIGMA_LOG_##type) \
          ligma_log (LIGMA_LOG_##type, G_STRFUNC, __LINE__, __VA_ARGS__);       \
        } G_STMT_END

#elif defined(G_HAVE_GNUC_VARARGS)

#define LIGMA_LOG(type, format...) \
        G_STMT_START { \
        if (ligma_log_flags & LIGMA_LOG_##type) \
          ligma_log (LIGMA_LOG_##type, G_STRFUNC, __LINE__, format);  \
        } G_STMT_END

#else /* no varargs macros */

/* need to expand all the short forms
 * to make them known constants at compile time
 */
#define TOOL_EVENTS        LIGMA_LOG_TOOL_EVENTS
#define TOOL_FOCUS         LIGMA_LOG_TOOL_FOCUS
#define DND                LIGMA_LOG_DND
#define HELP               LIGMA_LOG_HELP
#define DIALOG_FACTORY     LIGMA_LOG_DIALOG_FACTORY
#define MENUS              LIGMA_LOG_MENUS
#define SAVE_DIALOG        LIGMA_LOG_SAVE_DIALOG
#define IMAGE_SCALE        LIGMA_LOG_IMAGE_SCALE
#define SHADOW_TILES       LIGMA_LOG_SHADOW_TILES
#define SCALE              LIGMA_LOG_SCALE
#define WM                 LIGMA_LOG_WM
#define FLOATING_SELECTION LIGMA_LOG_FLOATING_SELECTION
#define SHM                LIGMA_LOG_SHM
#define TEXT_EDITING       LIGMA_LOG_TEXT_EDITING
#define KEY_EVENTS         LIGMA_LOG_KEY_EVENTS
#define AUTO_TAB_STYLE     LIGMA_LOG_AUTO_TAB_STYLE
#define INSTANCES          LIGMA_LOG_INSTANCES
#define RECTANGLE_TOOL     LIGMA_LOG_RECTANGLE_TOOL
#define BRUSH_CACHE        LIGMA_LOG_BRUSH_CACHE
#define PROJECTION         LIGMA_LOG_PROJECTION
#define XCF                LIGMA_LOG_XCF

#if 0 /* last resort */
#  define LIGMA_LOG /* nothing => no varargs, no log */
#endif

static void
LIGMA_LOG (LigmaLogFlags flags,
          const gchar *format,
          ...)
{
  va_list args;
  va_start (args, format);
  if (ligma_log_flags & flags)
    ligma_logv (type, "", 0, format, args);
  va_end (args);
}

#endif  /* !__GNUC__ */

#define geimnum(vienna)  ligma_l##vienna##l_dialog()
#define fnord(kosmoso)   void ligma_##kosmoso##bl_dialog(void);

#endif /* __LIGMA_LOG_H__ */
