/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligma.h
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

#ifndef __LIGMA_H__
#define __LIGMA_H__

#include <cairo.h>
#include <glib-object.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libligmabase/ligmabase.h>
#include <libligmacolor/ligmacolor.h>
#include <libligmaconfig/ligmaconfig.h>
#include <libligmamath/ligmamath.h>

#define __LIGMA_H_INSIDE__

#include <libligma/ligmaenums.h>
#include <libligma/ligmatypes.h>

#include <libligma/ligmabatchprocedure.h>
#include <libligma/ligmabrushselect.h>
#include <libligma/ligmachannel.h>
#include <libligma/ligmadisplay.h>
#include <libligma/ligmadrawable.h>
#include <libligma/ligmafontselect.h>
#include <libligma/ligmaligmarc.h>
#include <libligma/ligmagradientselect.h>
#include <libligma/ligmaimage.h>
#include <libligma/ligmaimagecolorprofile.h>
#include <libligma/ligmaimagemetadata.h>
#include <libligma/ligmaimageprocedure.h>
#include <libligma/ligmaitem.h>
#include <libligma/ligmalayer.h>
#include <libligma/ligmalayermask.h>
#include <libligma/ligmaloadprocedure.h>
#include <libligma/ligmapaletteselect.h>
#include <libligma/ligmaparamspecs.h>
#include <libligma/ligmapatternselect.h>
#include <libligma/ligmapdb.h>
#include <libligma/ligmaplugin.h>
#include <libligma/ligmaprocedureconfig.h>
#include <libligma/ligmaprocedure-params.h>
#include <libligma/ligmaprogress.h>
#include <libligma/ligmasaveprocedure.h>
#include <libligma/ligmaselection.h>
#include <libligma/ligmatextlayer.h>
#include <libligma/ligmathumbnailprocedure.h>
#include <libligma/ligmavectors.h>

#include <libligma/ligma_pdb_headers.h>

#undef __LIGMA_H_INSIDE__

#ifdef G_OS_WIN32
#include <stdlib.h> /* For __argc and __argv */
#endif

G_BEGIN_DECLS


#define ligma_get_data      ligma_pdb_get_data
#define ligma_get_data_size ligma_pdb_get_data_size
#define ligma_set_data      ligma_pdb_set_data


/**
 * LIGMA_MAIN:
 * @plug_in_type: The #GType of the plug-in's #LigmaPlugIn subclass
 *
 * A macro that expands to the appropriate main() function for the
 * platform being compiled for.
 *
 * To use this macro, simply place a line that contains just the code
 *
 * LIGMA_MAIN (MY_TYPE_PLUG_IN)
 *
 * at the toplevel of your file. No semicolon should be used.
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

#  define LIGMA_MAIN(plug_in_type)                       \
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
     return ligma_main (plug_in_type,                    \
                       __argc, __argv);                 \
   }                                                    \
                                                        \
   int                                                  \
   main (int argc, char *argv[])                        \
   {                                                    \
     /* Use __argc and __argv here, too, as they work   \
      * better with mingw-w64.                          \
      */                                                \
     return ligma_main (plug_in_type,                    \
                       __argc, __argv);                 \
   }
#else
#  define LIGMA_MAIN(plug_in_type)                       \
   int                                                  \
   main (int argc, char *argv[])                        \
   {                                                    \
     return ligma_main (plug_in_type,                    \
                       argc, argv);                     \
   }
#endif


/* The main procedure that must be called with the plug-in's
 * LigmaPlugIn subclass type and the 'argc' and 'argv' that are passed
 * to "main".
 */
gint                ligma_main                 (GType  plug_in_type,
                                               gint   argc,
                                               gchar *argv[]);

/* Return the LigmaPlugIn singleton of this plug-in process
 */
LigmaPlugIn        * ligma_get_plug_in          (void);

/* Return the LigmaPDB singleton of this plug-in process
 */
LigmaPDB           * ligma_get_pdb              (void);

/* Forcefully causes the ligma library to exit and
 * close down its connection to main ligma application.
 */
void                ligma_quit                 (void) G_GNUC_NORETURN;

/* Return various constants given by the LIGMA core at plug-in config time.
 */
guint               ligma_tile_width           (void) G_GNUC_CONST;
guint               ligma_tile_height          (void) G_GNUC_CONST;
gboolean            ligma_show_help_button     (void) G_GNUC_CONST;
gboolean            ligma_export_color_profile (void) G_GNUC_CONST;
gboolean            ligma_export_comment       (void) G_GNUC_CONST;
gboolean            ligma_export_exif          (void) G_GNUC_CONST;
gboolean            ligma_export_xmp           (void) G_GNUC_CONST;
gboolean            ligma_export_iptc          (void) G_GNUC_CONST;
gboolean            ligma_export_thumbnail     (void) G_GNUC_CONST;
gint                ligma_get_num_processors   (void) G_GNUC_CONST;
LigmaCheckSize       ligma_check_size           (void) G_GNUC_CONST;
LigmaCheckType       ligma_check_type           (void) G_GNUC_CONST;
const LigmaRGB     * ligma_check_custom_color1  (void) G_GNUC_CONST;
const LigmaRGB *     ligma_check_custom_color2  (void) G_GNUC_CONST;
LigmaDisplay       * ligma_default_display      (void) G_GNUC_CONST;
const gchar       * ligma_wm_class             (void) G_GNUC_CONST;
const gchar       * ligma_display_name         (void) G_GNUC_CONST;
gint                ligma_monitor_number       (void) G_GNUC_CONST;
guint32             ligma_user_time            (void) G_GNUC_CONST;
const gchar       * ligma_icon_theme_dir       (void) G_GNUC_CONST;

const gchar       * ligma_get_progname         (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __LIGMA_H__ */
