/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimp.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_H__
#define __GIMP_H__

#include <glib.h>

#include <libgimp/gimpenums.h>
#include <libgimp/gimptypes.h>

#include <libgimp/gimpchannel.h>
#include <libgimp/gimpcolorspace.h>
#include <libgimp/gimpdrawable.h>
#include <libgimp/gimpfeatures.h>
#include <libgimp/gimpgradientselect.h>
#include <libgimp/gimpenv.h>
#include <libgimp/gimpimage.h>
#include <libgimp/gimplayer.h>
#include <libgimp/gimplimits.h>
#include <libgimp/gimpmath.h>
#include <libgimp/gimpparasite.h>
#include <libgimp/gimppixelrgn.h>
#include <libgimp/gimpproceduraldb.h>
#include <libgimp/gimpselection.h>
#include <libgimp/gimptile.h>
#include <libgimp/gimpunit.h>
#include <libgimp/gimputils.h>
#include <libgimp/gimpvector.h>

#include <libgimp/gimp_pdb.h>

#include <libgimp/gimpcompat.h>  /* to be removed in 1.3 */

#ifdef G_OS_WIN32
#  include <stdlib.h>		/* For _-argc and __argv */
#  ifdef LIBGIMP_COMPILATION
#    define GIMPVAR __declspec(dllexport)
#  else  /* !LIBGIMP_COMPILATION */
#    define GIMPVAR extern __declspec(dllimport)
#  endif /* !LIBGIMP_COMPILATION */
#else  /* !G_OS_WIN32 */
#  define GIMPVAR extern
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* more convenient names for some pdb functions */  

#define gimp_get_data                 gimp_procedural_db_get_data
#define gimp_get_data_size            gimp_procedural_db_get_data_size
#define gimp_set_data                 gimp_procedural_db_set_data
#define gimp_query_procedure          gimp_procedural_db_proc_info

#define gimp_image_convert_rgb        gimp_convert_rgb
#define gimp_image_convert_grayscale  gimp_convert_grayscale
#define gimp_image_convert_indexed    gimp_convert_indexed
#define gimp_image_duplicate          gimp_channel_ops_duplicate

#define gimp_drawable_offset          gimp_channel_ops_offset
#define gimp_drawable_image_id        gimp_drawable_image
#define gimp_drawable_bpp             gimp_drawable_bytes

#define gimp_layer_get_mask_id               gimp_layer_mask
#define gimp_layer_get_image_id              gimp_drawable_image
#define gimp_layer_is_floating_selection     gimp_layer_is_floating_sel
#define gimp_layer_get_preserve_transparency gimp_layer_get_preserve_trans
#define gimp_layer_set_preserve_transparency gimp_layer_set_preserve_trans

GIMPVAR guint gimp_major_version;
GIMPVAR guint gimp_minor_version;
GIMPVAR guint gimp_micro_version;

GIMPVAR GIOChannel *_readchannel;

typedef void (* GimpInitProc)  (void);
typedef void (* GimpQuitProc)  (void);
typedef void (* GimpQueryProc) (void);
typedef void (* GimpRunProc)   (gchar      *name,
				gint        nparams,
				GimpParam  *param,
				gint       *nreturn_vals,
				GimpParam **return_vals);


struct _GimpPlugInInfo
{
  /* called when the gimp application initially starts up */
  GimpInitProc  init_proc;

  /* called when the gimp application exits */
  GimpQuitProc  quit_proc;

  /* called by the gimp so that the plug-in can inform the
   *  gimp of what it does. (ie. installing a procedure database
   *  procedure).
   */
  GimpQueryProc query_proc;

  /* called to run a procedure the plug-in installed in the
   *  procedure database.
   */
  GimpRunProc   run_proc;
};

struct _GimpParamDef
{
  GimpPDBArgType  type;
  gchar          *name;
  gchar          *description;
};

struct _GimpParamColor
{
  guint8 red;
  guint8 green;
  guint8 blue;
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
  gint8             d_int8;
  gdouble           d_float;
  gchar            *d_string;
  gint32           *d_int32array;
  gint16           *d_int16array;
  gint8            *d_int8array;
  gdouble          *d_floatarray;
  gchar           **d_stringarray;
  GimpParamColor    d_color;
  GimpParamRegion   d_region;
  gint32            d_display;
  gint32            d_image;
  gint32            d_layer;
  gint32            d_layer_mask;
  gint32            d_channel;
  gint32            d_drawable;
  gint32            d_selection;
  gint32            d_boundary;
  gint32            d_path;
  gint32            d_unit;
  GimpParasite      d_parasite;
  gint32            d_tattoo;
  gint32            d_status;
};

struct _GimpParam
{
  GimpPDBArgType type;
  GimpParamData  data;
};


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

#  define MAIN()			\
   static int				\
   win32_gimp_main (int argc, char **argv)	\
   {					\
     extern void set_gimp_PLUG_IN_INFO_PTR(GimpPlugInInfo *);	\
     set_gimp_PLUG_IN_INFO_PTR(&PLUG_IN_INFO);	\
     return gimp_main (argc, argv);	\
   }					\
					\
   struct HINSTANCE__;			\
   int _stdcall				\
   WinMain (struct HINSTANCE__ *hInstance, \
	    struct HINSTANCE__ *hPrevInstance,	\
	    char *lpszCmdLine,		\
	    int   nCmdShow)		\
   {					\
     return win32_gimp_main (__argc, __argv);	\
   }					\
					\
   int					\
   main (int argc, char *argv[])	\
   {					\
     return win32_gimp_main (argc, argv);	\
   }
#else
#ifndef __EMX__
#  define MAIN()			\
   int					\
   main (int argc, char *argv[])	\
   {					\
     return gimp_main (argc, argv);	\
   }
#else
#  define MAIN()				\
   int						\
   main (int argc, char *argv[])		\
   {						\
     set_gimp_PLUG_IN_INFO(&PLUG_IN_INFO);	\
     return gimp_main (argc, argv);		\
   }
#endif
#endif


/* The main procedure that should be called with the
 *  'argc' and 'argv' that are passed to "main".
 */
gint        gimp_main                (gint      argc,
				      gchar    *argv[]);

/* Forcefully causes the gimp library to exit and
 *  close down its connection to main gimp application.
 */
void G_GNUC_NORETURN gimp_quit       (void);

/* Returns the default gdisplay (given at plug-in config time).
 */
gint32      gimp_default_display     (void);


/* Install a procedure in the procedure database.
 */
void        gimp_install_procedure   (gchar        *name,
				      gchar        *blurb,
				      gchar        *help,
				      gchar        *author,
				      gchar        *copyright,
				      gchar        *date,
				      gchar        *menu_path,
				      gchar        *image_types,
				      gint          type,
				      gint          nparams,
				      gint          nreturn_vals,
				      GimpParamDef *params,
				      GimpParamDef *return_vals);

/* Install a temporary procedure in the procedure database.
 */
void        gimp_install_temp_proc   (gchar        *name,
				      gchar        *blurb,
				      gchar        *help,
				      gchar        *author,
				      gchar        *copyright,
				      gchar        *date,
				      gchar        *menu_path,
				      gchar        *image_types,
				      gint          type,
				      gint          nparams,
				      gint          nreturn_vals,
				      GimpParamDef *params,
				      GimpParamDef *return_vals,
				      GimpRunProc   run_proc);

/* Uninstall a temporary procedure
 */
void        gimp_uninstall_temp_proc (gchar        *name);

/* Run a procedure in the procedure database. The parameters are
 *  specified via the variable length argument list. The return
 *  values are returned in the 'GimpParam*' array.
 */
GimpParam * gimp_run_procedure     (gchar     *name,
				    gint      *nreturn_vals,
				    ...);

/* Run a procedure in the procedure database. The parameters are
 *  specified as an array of GimpParam.  The return
 *  values are returned in the 'GimpParam*' array.
 */
GimpParam * gimp_run_procedure2    (gchar     *name,
				    gint      *nreturn_vals,
				    gint       nparams,
				    GimpParam *params);

/* Destroy the an array of parameters. This is useful for
 *  destroying the return values returned by a call to
 *  'gimp_run_procedure'.
 */
void        gimp_destroy_params    (GimpParam    *params,
				    gint          nparams);

/* Destroy the an array of GimpParamDef's. This is useful for
 *  destroying the return values returned by a call to
 *  'gimp_query_procedure'.
 */
void        gimp_destroy_paramdefs (GimpParamDef *paramdefs,
				    gint          nparams);

gdouble     gimp_gamma             (void);
gboolean    gimp_install_cmap      (void);
gboolean    gimp_use_xshm          (void);
guchar    * gimp_color_cube        (void);
gint        gimp_min_colors        (void);
void        gimp_request_wakeups   (void);

gchar     * gimp_get_progname      (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_H__ */
