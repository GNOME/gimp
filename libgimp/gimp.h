/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#include <libgimp/gimpcolorspace.h>
#include <libgimp/gimpfeatures.h>
#include <libgimp/gimpenv.h>
#include <libgimp/gimplimits.h>
#include <libgimp/gimpmath.h>
#include <libgimp/gimpparasite.h>
#include <libgimp/gimppixelrgn.h>
#include <libgimp/gimptile.h>
#include <libgimp/gimpunit.h>
#include <libgimp/gimputils.h>
#include <libgimp/gimpvector.h>

#include <libgimp/gimpchannel_pdb.h>
#include <libgimp/gimpdisplay_pdb.h>
#include <libgimp/gimpdrawable_pdb.h>
#include <libgimp/gimpgradient_pdb.h>
#include <libgimp/gimphelp_pdb.h>
#include <libgimp/gimpimage_pdb.h>
#include <libgimp/gimplayer_pdb.h>
#include <libgimp/gimppalette_pdb.h>
#include <libgimp/gimpparasite_pdb.h>
#include <libgimp/gimpselection_pdb.h>
#include <libgimp/gimpunit_pdb.h>

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
  gint32            d_channel;
  gint32            d_drawable;
  gint32            d_selection;
  gint32            d_boundary;
  gint32            d_path;
  GimpParasite      d_parasite;
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

/* Specify a range of data to be associated with 'id'.
 *  The data will exist for as long as the main gimp
 *  application is running.
 */
void        gimp_set_data            (gchar    *id,
				      gpointer  data,
				      guint32   length);

/* Retrieve the piece of data stored within the main
 *  gimp application specified by 'id'. The data is
 *  stored in the supplied buffer.  Make sure enough
 *  space is allocated.
 */
void        gimp_get_data            (gchar    *id,
				      gpointer  data);

/* Get the size in bytes of the data stored by a gimp_get_data
 * id. As size of zero may indicate that there is no such
 * identifier in the database.
 */
guint32     gimp_get_data_size       (gchar    *id);

/* Initialize the progress bar with "message". If "message"
 *  is NULL, the message displayed in the progress window will
 *  be the name of the plug-in.
 */
void        gimp_progress_init       (gchar    *message);

/* Update the progress bar. If the progress bar has not been
 *  initialized then it will be automatically initialized as if
 *  "gimp_progress_init (NULL)" were called. "percentage" is a
 *  value between 0 and 1.
 */
void        gimp_progress_update     (gdouble   percentage);

/* Returns the default gdisplay (given at plug-in config time).
 */
gint32      gimp_default_display     (void);


/* Pops up a dialog box with "message". Useful for status and
 * error reports. If "message" is NULL, do nothing.
 */
void        gimp_message             (const gchar    *message);


/* Query the gimp application's procedural database.
 *  The arguments are regular expressions which select
 *  which procedure names will be returned in 'proc_names'.
 */
void        gimp_query_database      (gchar          *name_regexp,
				      gchar          *blurb_regexp,
				      gchar          *help_regexp,
				      gchar          *author_regexp,
				      gchar          *copyright_regexp,
				      gchar          *date_regexp,
				      gchar          *proc_type_regexp,
				      gint           *nprocs,
				      gchar        ***proc_names);

/* Query the gimp application's procedural database
 *  regarding a particular procedure.
 */
gboolean    gimp_query_procedure     (gchar          *proc_name,
				      gchar         **proc_blurb,
				      gchar         **proc_help,
				      gchar         **proc_author,
				      gchar         **proc_copyright,
				      gchar         **proc_date,
				      gint           *proc_type,
				      gint           *nparams,
				      gint           *nreturn_vals,
				      GimpParamDef  **params,
				      GimpParamDef  **return_vals);

/* Query the gimp application regarding all open images.
 *  The list of open image id's is returned in 'image_ids'.
 */
gint32    * gimp_query_images        (gint         *nimages);


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

/* Install a load file format handler in the procedure database.
 */
void        gimp_register_magic_load_handler (gchar *name,
					      gchar *extensions,
					      gchar *prefixes,
					      gchar *magics);

/* Install a load file format handler in the procedure database.
 */
void        gimp_register_load_handler       (gchar *name,
					      gchar *extensions,
					      gchar *prefixes);

/* Install a save file format handler in the procedure database.
 */
void        gimp_register_save_handler       (gchar *name,
					      gchar *extensions,
					      gchar *prefixes);

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


/****************************************
 *           Localisation               *
 ****************************************/

void        gimp_plugin_domain_add           (gchar *domain_name);
void        gimp_plugin_domain_add_with_path (gchar *domain_name, 
					      gchar *domain_path);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_H__ */
