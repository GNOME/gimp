/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimp.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_H__
#define __GIMP_H__

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libgimpbase/gimpbase.h>
#include <libgimpcolor/gimpcolor.h>
#include <libgimpconfig/gimpconfig.h>
#include <libgimpmath/gimpmath.h>

#define __GIMP_H_INSIDE__

#include <libgimp/gimpenums.h>
#include <libgimp/gimptypes.h>

#include <libgimp/gimpbrushselect.h>
#include <libgimp/gimpchannel.h>
#include <libgimp/gimpdrawable.h>
#include <libgimp/gimpfontselect.h>
#include <libgimp/gimpgimprc.h>
#include <libgimp/gimpgradientselect.h>
#include <libgimp/gimpimage.h>
#include <libgimp/gimpimagecolorprofile.h>
#include <libgimp/gimplayer.h>
#include <libgimp/gimplegacy.h>
#include <libgimp/gimppaletteselect.h>
#include <libgimp/gimpparamspecs.h>
#include <libgimp/gimppatternselect.h>
#include <libgimp/gimppixbuf.h>
#include <libgimp/gimpplugin.h>
#include <libgimp/gimpproceduraldb.h>
#include <libgimp/gimpprocedure.h>
#include <libgimp/gimpprogress.h>
#include <libgimp/gimpselection.h>

#include <libgimp/gimp_pdb_headers.h>

#undef __GIMP_H_INSIDE__

#ifdef G_OS_WIN32
#include <stdlib.h> /* For __argc and __argv */
#endif

G_BEGIN_DECLS


#define gimp_get_data         gimp_procedural_db_get_data
#define gimp_get_data_size    gimp_procedural_db_get_data_size
#define gimp_set_data         gimp_procedural_db_set_data


/**
 * GIMP_MAIN:
 * @plug_in_type: The #GType of the plug-in's #GimpPlugIn subclass
 *
 * A macro that expands to the appropriate main() function for the
 * platform being compiled for.
 *
 * To use this macro, simply place a line that contains just the code
 *
 * GIMP_MAIN (MY_TYPE_PLUG_IN)
 *
 * at the toplevel of your file. No semicolon should be used.
 **/

#ifdef G_OS_WIN32

/* Define WinMain() because plug-ins are built as GUI applications. Also
 * define a main() in case some plug-in still is built as a console
 * application.
 */
#  ifdef __GNUC__
#    ifndef _stdcall
#      define _stdcall __attribute__((stdcall))
#    endif
#  endif

#  define GIMP_MAIN(plug_in_type)                       \
   struct HINSTANCE__;                                  \
                                                        \
   int _stdcall                                         \
   WinMain (struct HINSTANCE__ *hInstance,              \
            struct HINSTANCE__ *hPrevInstance,          \
            char *lpszCmdLine,                          \
            int   nCmdShow);                            \
                                                        \
   int _stdcall                                         \
   WinMain (struct HINSTANCE__ *hInstance,              \
            struct HINSTANCE__ *hPrevInstance,          \
            char *lpszCmdLine,                          \
            int   nCmdShow)                             \
   {                                                    \
     return gimp_main (plug_in_type,                    \
                       _argc, __argv);                  \
   }                                                    \
                                                        \
   int                                                  \
   main (int argc, char *argv[])                        \
   {                                                    \
     /* Use __argc and __argv here, too, as they work   \
      * better with mingw-w64.                          \
      */                                                \
     return gimp_main (plug_in_type,                    \
                       __argc, __argv);                 \
   }
#else
#  define GIMP_MAIN(plug_in_type)                       \
   int                                                  \
   main (int argc, char *argv[])                        \
   {                                                    \
     return gimp_main (plug_in_type,                    \
                       argc, argv);                     \
   }
#endif


/* The main procedure that must be called with the plug-in's
 * GimpPlugIn subclass type and the 'argc' and 'argv' that are passed
 * to "main".
 */
gint                gimp_main                 (GType  plug_in_type,
                                               gint   argc,
                                               gchar *argv[]);

/* Forcefully causes the gimp library to exit and
 *  close down its connection to main gimp application.
 */
void                gimp_quit                 (void) G_GNUC_NORETURN;

/* Enable asynchronous processing of temp_procs
 */
void                gimp_extension_enable     (void);

/* Process one temp_proc and return
 */
void                gimp_extension_process    (guint  timeout);

/* Run a procedure in the procedure database. The parameters are
 *  specified as a GimpValueArray, so are the return values.
 *
 * FIXME this API is not final!
 */
GimpValueArray    * gimp_run_procedure_with_array (const gchar    *name,
                                                   GimpValueArray *arguments);

/* Retrieve the error message and return status for the last procedure
 * call.
 */
const gchar       * gimp_get_pdb_error        (void);
GimpPDBStatusType   gimp_get_pdb_status       (void);

/* Return various constants given by the GIMP core at plug-in config time.
 */
guint               gimp_tile_width           (void) G_GNUC_CONST;
guint               gimp_tile_height          (void) G_GNUC_CONST;
gint                gimp_shm_ID               (void) G_GNUC_CONST;
guchar            * gimp_shm_addr             (void) G_GNUC_CONST;
gboolean            gimp_show_help_button     (void) G_GNUC_CONST;
gboolean            gimp_export_color_profile (void) G_GNUC_CONST;
gboolean            gimp_export_exif          (void) G_GNUC_CONST;
gboolean            gimp_export_xmp           (void) G_GNUC_CONST;
gboolean            gimp_export_iptc          (void) G_GNUC_CONST;
GimpCheckSize       gimp_check_size           (void) G_GNUC_CONST;
GimpCheckType       gimp_check_type           (void) G_GNUC_CONST;
gint32              gimp_default_display      (void) G_GNUC_CONST;
const gchar       * gimp_wm_class             (void) G_GNUC_CONST;
const gchar       * gimp_display_name         (void) G_GNUC_CONST;
gint                gimp_monitor_number       (void) G_GNUC_CONST;
guint32             gimp_user_time            (void) G_GNUC_CONST;
const gchar       * gimp_icon_theme_dir       (void) G_GNUC_CONST;

const gchar       * gimp_get_progname         (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GIMP_H__ */
