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
 * <http://www.gnu.org/licenses/>.
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
#include <libgimp/gimppaletteselect.h>
#include <libgimp/gimppatternselect.h>
#include <libgimp/gimppixbuf.h>
#include <libgimp/gimppixelfetcher.h>
#include <libgimp/gimppixelrgn.h>
#include <libgimp/gimpplugin.h>
#include <libgimp/gimpproceduraldb.h>
#include <libgimp/gimpprogress.h>
#include <libgimp/gimpregioniterator.h>
#include <libgimp/gimpselection.h>
#include <libgimp/gimptile.h>

#include <libgimp/gimp_pdb_headers.h>

#undef __GIMP_H_INSIDE__

#ifdef G_OS_WIN32
#include <stdlib.h> /* For __argc and __argv */
#endif

G_BEGIN_DECLS


#define gimp_get_data         gimp_procedural_db_get_data
#define gimp_get_data_size    gimp_procedural_db_get_data_size
#define gimp_set_data         gimp_procedural_db_set_data


typedef void (* GimpInitProc)  (void);
typedef void (* GimpQuitProc)  (void);
typedef void (* GimpQueryProc) (void);
typedef void (* GimpRunProc)   (const gchar      *name,
                                gint              n_params,
                                const GimpParam  *param,
                                gint             *n_return_vals,
                                GimpParam       **return_vals);


/**
 * GimpPlugInInfo:
 * @init_proc:  called when the gimp application initially starts up
 * @quit_proc:  called when the gimp application exits
 * @query_proc: called by gimp so that the plug-in can inform the
 *              gimp of what it does. (ie. installing a procedure database
 *              procedure).
 * @run_proc:   called to run a procedure the plug-in installed in the
 *              procedure database.
 **/
struct _GimpPlugInInfo
{
  GimpInitProc  init_proc;
  GimpQuitProc  quit_proc;
  GimpQueryProc query_proc;
  GimpRunProc   run_proc;
};

struct _GimpParamDef
{
  GimpPDBArgType  type;
  gchar          *name;
  gchar          *description;
};

struct _GimpParamRegion
{
  gint32 x;
  gint32 y;
  gint32 width;
  gint32 height;
};

union _GimpParamData
{
  gint32            d_int32;
  gint16            d_int16;
  guint8            d_int8;
  gdouble           d_float;
  gchar            *d_string;
  gint32           *d_int32array;
  gint16           *d_int16array;
  guint8           *d_int8array;
  gdouble          *d_floatarray;
  gchar           **d_stringarray;
  GimpRGB          *d_colorarray;
  GimpRGB           d_color;
  gint32            d_display;
  gint32            d_image;
  gint32            d_item;
  gint32            d_layer;
  gint32            d_layer_mask;
  gint32            d_channel;
  gint32            d_drawable;
  gint32            d_selection;
  gint32            d_boundary;
  gint32            d_vectors;
  gint32            d_unit;
  GimpParasite      d_parasite;
  gint32            d_tattoo;
  GimpPDBStatusType d_status;
};

struct _GimpParam
{
  GimpPDBArgType type;
  GimpParamData  data;
};



/**
 * MAIN:
 *
 * A macro that expands to the appropriate main() function for the
 * platform being compiled for.
 *
 * To use this macro, simply place a line that contains just the code
 * MAIN() at the toplevel of your file.  No semicolon should be used.
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

#  define MAIN()                                        \
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
     return gimp_main (&PLUG_IN_INFO, __argc, __argv);  \
   }                                                    \
                                                        \
   int                                                  \
   main (int argc, char *argv[])                        \
   {                                                    \
     /* Use __argc and __argv here, too, as they work   \
      * better with mingw-w64.                          \
      */                                                \
     return gimp_main (&PLUG_IN_INFO, __argc, __argv);  \
   }
#else
#  define MAIN()                                        \
   int                                                  \
   main (int argc, char *argv[])                        \
   {                                                    \
     return gimp_main (&PLUG_IN_INFO, argc, argv);      \
   }
#endif


/* The main procedure that must be called with the PLUG_IN_INFO structure
 * and the 'argc' and 'argv' that are passed to "main".
 */
gint           gimp_main                (const GimpPlugInInfo *info,
                                         gint                  argc,
                                         gchar                *argv[]);

/* Forcefully causes the gimp library to exit and
 *  close down its connection to main gimp application.
 */
void           gimp_quit                (void) G_GNUC_NORETURN;


/* Install a procedure in the procedure database.
 */
void           gimp_install_procedure   (const gchar        *name,
                                         const gchar        *blurb,
                                         const gchar        *help,
                                         const gchar        *author,
                                         const gchar        *copyright,
                                         const gchar        *date,
                                         const gchar        *menu_label,
                                         const gchar        *image_types,
                                         GimpPDBProcType     type,
                                         gint                n_params,
                                         gint                n_return_vals,
                                         const GimpParamDef *params,
                                         const GimpParamDef *return_vals);

/* Install a temporary procedure in the procedure database.
 */
void           gimp_install_temp_proc   (const gchar        *name,
                                         const gchar        *blurb,
                                         const gchar        *help,
                                         const gchar        *author,
                                         const gchar        *copyright,
                                         const gchar        *date,
                                         const gchar        *menu_label,
                                         const gchar        *image_types,
                                         GimpPDBProcType     type,
                                         gint                n_params,
                                         gint                n_return_vals,
                                         const GimpParamDef *params,
                                         const GimpParamDef *return_vals,
                                         GimpRunProc         run_proc);

/* Uninstall a temporary procedure
 */
void           gimp_uninstall_temp_proc (const gchar        *name);

/* Notify the main GIMP application that the extension is ready to run
 */
void           gimp_extension_ack       (void);

/* Enable asynchronous processing of temp_procs
 */
void           gimp_extension_enable    (void);

/* Process one temp_proc and return
 */
void           gimp_extension_process   (guint            timeout);

/* Run a procedure in the procedure database. The parameters are
 *  specified via the variable length argument list. The return
 *  values are returned in the 'GimpParam*' array.
 */
GimpParam    * gimp_run_procedure       (const gchar     *name,
                                         gint            *n_return_vals,
                                         ...);

/* Run a procedure in the procedure database. The parameters are
 *  specified as an array of GimpParam.  The return
 *  values are returned in the 'GimpParam*' array.
 */
GimpParam    * gimp_run_procedure2      (const gchar     *name,
                                         gint            *n_return_vals,
                                         gint             n_params,
                                         const GimpParam *params);

/* Destroy the an array of parameters. This is useful for
 *  destroying the return values returned by a call to
 *  'gimp_run_procedure'.
 */
void           gimp_destroy_params      (GimpParam       *params,
                                         gint             n_params);

/* Destroy the an array of GimpParamDef's. This is useful for
 *  destroying the return values returned by a call to
 *  'gimp_procedural_db_proc_info'.
 */
void           gimp_destroy_paramdefs   (GimpParamDef    *paramdefs,
                                         gint             n_params);

/* Retrieve the error message for the last procedure call.
 */
const gchar  * gimp_get_pdb_error       (void);

/* Retrieve the return status for the last procedure call.
 */
GimpPDBStatusType gimp_get_pdb_status   (void);

/* Return various constants given by the GIMP core at plug-in config time.
 */
guint          gimp_tile_width          (void) G_GNUC_CONST;
guint          gimp_tile_height         (void) G_GNUC_CONST;
gint           gimp_shm_ID              (void) G_GNUC_CONST;
guchar       * gimp_shm_addr            (void) G_GNUC_CONST;
gdouble        gimp_gamma               (void) G_GNUC_CONST;
gboolean       gimp_show_tool_tips      (void) G_GNUC_CONST;
gboolean       gimp_show_help_button    (void) G_GNUC_CONST;
gboolean       gimp_export_exif         (void) G_GNUC_CONST;
gboolean       gimp_export_xmp          (void) G_GNUC_CONST;
gboolean       gimp_export_iptc         (void) G_GNUC_CONST;
GimpCheckSize  gimp_check_size          (void) G_GNUC_CONST;
GimpCheckType  gimp_check_type          (void) G_GNUC_CONST;
gint32         gimp_default_display     (void) G_GNUC_CONST;
const gchar  * gimp_wm_class            (void) G_GNUC_CONST;
const gchar  * gimp_display_name        (void) G_GNUC_CONST;
gint           gimp_monitor_number      (void) G_GNUC_CONST;
guint32        gimp_user_time           (void) G_GNUC_CONST;

const gchar  * gimp_get_progname        (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GIMP_H__ */
