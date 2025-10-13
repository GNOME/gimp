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
#include <glib-object.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pango.h>

#include <libgimpbase/gimpbase.h>
#include <libgimpcolor/gimpcolor.h>
#include <libgimpconfig/gimpconfig.h>
#include <libgimpmath/gimpmath.h>

#define __GIMP_H_INSIDE__

#include <libgimp/gimpenums.h>
#include <libgimp/gimptypes.h>

#include <libgimp/gimpbatchprocedure.h>
#include <libgimp/gimpbrush.h>
#include <libgimp/gimpchannel.h>
#include <libgimp/gimpdisplay.h>
#include <libgimp/gimpdrawable.h>
#include <libgimp/gimpdrawablefilter.h>
#include <libgimp/gimpdrawablefilterconfig.h>
#include <libgimp/gimpexportoptions.h>
#include <libgimp/gimpexportprocedure.h>
#include <libgimp/gimpfont.h>
#include <libgimp/gimpgimprc.h>
#include <libgimp/gimpgradient.h>
#include <libgimp/gimpgrouplayer.h>
#include <libgimp/gimpimage.h>
#include <libgimp/gimpimagecolorprofile.h>
#include <libgimp/gimpimagemetadata.h>
#include <libgimp/gimpimageprocedure.h>
#include <libgimp/gimpitem.h>
#include <libgimp/gimplayer.h>
#include <libgimp/gimplayermask.h>
#include <libgimp/gimplinklayer.h>
#include <libgimp/gimploadprocedure.h>
#include <libgimp/gimppalette.h>
#include <libgimp/gimpparamspecs.h>
#include <libgimp/gimppath.h>
#include <libgimp/gimppattern.h>
#include <libgimp/gimppdb.h>
#include <libgimp/gimpplugin.h>
#include <libgimp/gimpprocedureconfig.h>
#include <libgimp/gimpprocedure-params.h>
#include <libgimp/gimpprogress.h>
#include <libgimp/gimprasterizable.h>
#include <libgimp/gimpresource.h>
#include <libgimp/gimpselection.h>
#include <libgimp/gimptextlayer.h>
#include <libgimp/gimpthumbnailprocedure.h>
#include <libgimp/gimpvectorlayer.h>
#include <libgimp/gimpvectorloadprocedure.h>

#include <libgimp/gimp_pdb_headers.h>

#undef __GIMP_H_INSIDE__

#ifdef G_OS_WIN32
#include <stdlib.h> /* For __argc and __argv */
#endif

G_BEGIN_DECLS


/**
 * GIMP_MAIN:
 * @plug_in_type: The #GType of the plug-in's #GimpPlugIn subclass
 *
 * A macro that expands to the appropriate main() function for the
 * platform being compiled for.
 *
 * To use this macro, simply place a line that contains just the code:
 *
 * ```C
 * GIMP_MAIN (MY_TYPE_PLUG_IN)
 * ```
 *
 * at the toplevel of your file. No semicolon should be used.
 *
 * Non-C plug-ins won't have this function and should use
 * [func@Gimp.main] instead.
 *
 * Since: 3.0
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
                       __argc, __argv);                 \
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

/* Return the GimpPlugIn singleton of this plug-in process
 */
GimpPlugIn        * gimp_get_plug_in          (void);

/* Return the GimpPDB singleton of this plug-in process
 */
GimpPDB           * gimp_get_pdb              (void);

/* Forcefully causes the gimp library to exit and
 * close down its connection to main gimp application.
 */
void                gimp_quit                 (void) G_GNUC_NORETURN;

/* Return various constants given by the GIMP core at plug-in config time.
 */
guint               gimp_tile_width           (void) G_GNUC_CONST;
guint               gimp_tile_height          (void) G_GNUC_CONST;
gboolean            gimp_show_help_button     (void) G_GNUC_CONST;
gboolean            gimp_export_color_profile (void) G_GNUC_CONST;
gboolean            gimp_export_comment       (void) G_GNUC_CONST;
gboolean            gimp_export_exif          (void) G_GNUC_CONST;
gboolean            gimp_export_xmp           (void) G_GNUC_CONST;
gboolean            gimp_export_iptc          (void) G_GNUC_CONST;
gboolean            gimp_export_thumbnail     (void) G_GNUC_CONST;
gboolean            gimp_update_metadata      (void) G_GNUC_CONST;
gint                gimp_get_num_processors   (void) G_GNUC_CONST;
GimpCheckSize       gimp_check_size           (void) G_GNUC_CONST;
GimpCheckType       gimp_check_type           (void) G_GNUC_CONST;
const GeglColor   * gimp_check_custom_color1  (void) G_GNUC_CONST;
const GeglColor   * gimp_check_custom_color2  (void) G_GNUC_CONST;
GimpDisplay       * gimp_default_display      (void) G_GNUC_CONST;
const gchar       * gimp_wm_class             (void) G_GNUC_CONST;
const gchar       * gimp_display_name         (void) G_GNUC_CONST;
gint                gimp_monitor_number       (void) G_GNUC_CONST;
guint32             gimp_user_time            (void) G_GNUC_CONST;
const gchar       * gimp_icon_theme_dir       (void) G_GNUC_CONST;

const gchar       * gimp_get_progname         (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GIMP_H__ */
