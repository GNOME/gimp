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

#include "libgimp/gimpenums.h"
#include "libgimp/gimpcolorspace.h"
#include "libgimp/gimpfeatures.h"
#include "libgimp/gimpenv.h"
#include "libgimp/gimplimits.h"
#include "libgimp/gimpmath.h"
#include "libgimp/parasite.h"
#include "libgimp/parasiteP.h"
#include "libgimp/gimpunit.h"
#include "libgimp/gimpvector.h"

#include "libgimp/gimpcompat.h"	/* to be removed in 1.3 */

#ifdef G_OS_WIN32
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

typedef struct _GPlugInInfo  GPlugInInfo;
typedef struct _GTile        GTile;
typedef struct _GDrawable    GDrawable;
typedef struct _GPixelRgn    GPixelRgn;
typedef struct _GParamDef    GParamDef;
typedef struct _GParamColor  GParamColor;
typedef struct _GParamRegion GParamRegion;
typedef union  _GParamData   GParamData;
typedef struct _GParam       GParam;
typedef void   (* GRunProc) (char    *name,
			     int      nparams,
			     GParam  *param,
			     int     *nreturn_vals,
			     GParam **return_vals);


struct _GPlugInInfo
{
  /* called when the gimp application initially starts up */
  void (*init_proc) (void);

  /* called when the gimp application exits */
  void (*quit_proc) (void);

  /* called by the gimp so that the plug-in can inform the
   *  gimp of what it does. (ie. installing a procedure database
   *  procedure).
   */
  void (*query_proc) (void);

  /* called to run a procedure the plug-in installed in the
   *  procedure database.
   */
  GRunProc run_proc;
};

struct _GTile
{
  guint ewidth;        /* the effective width of the tile */
  guint eheight;       /* the effective height of the tile */
  guint bpp;           /* the bytes per pixel (1, 2, 3 or 4 ) */
  guint tile_num;      /* the number of this tile within the drawable */
  guint16 ref_count;   /* reference count for the tile */
  guint dirty : 1;     /* is the tile dirty? has it been modified? */
  guint shadow: 1;     /* is this a shadow tile */
  guchar *data;        /* the pixel data for the tile */
  GDrawable *drawable; /* the drawable this tile came from */
};

struct _GDrawable
{
  gint32 id;            /* drawable ID */
  guint width;          /* width of drawble */
  guint height;         /* height of drawble */
  guint bpp;            /* bytes per pixel of drawable */
  guint ntile_rows;     /* # of tile rows */
  guint ntile_cols;     /* # of tile columns */
  GTile *tiles;         /* the normal tiles */
  GTile *shadow_tiles;  /* the shadow tiles */
};

struct _GPixelRgn
{
  guchar *data;         /* pointer to region data */
  GDrawable *drawable;  /* pointer to drawable */
  guint bpp;            /* bytes per pixel */
  guint rowstride;      /* bytes per pixel row */
  guint x, y;           /* origin */
  guint w, h;           /* width and height of region */
  guint dirty : 1;      /* will this region be dirtied? */
  guint shadow : 1;     /* will this region use the shadow or normal tiles */
  guint process_count;  /* used internally */
};

struct _GParamDef
{
  GParamType type;
  char *name;
  char *description;
};

struct _GParamColor
{
  guint8 red;
  guint8 green;
  guint8 blue;
};

struct _GParamRegion
{
  gint32 x;
  gint32 y;
  gint32 width;
  gint32 height;
};

union _GParamData
{
  gint32 d_int32;
  gint16 d_int16;
  gint8 d_int8;
  gdouble d_float;
  gchar *d_string;
  gint32 *d_int32array;
  gint16 *d_int16array;
  gint8 *d_int8array;
  gdouble *d_floatarray;
  gchar **d_stringarray;
  GParamColor d_color;
  GParamRegion d_region;
  gint32 d_display;
  gint32 d_image;
  gint32 d_layer;
  gint32 d_channel;
  gint32 d_drawable;
  gint32 d_selection;
  gint32 d_boundary;
  gint32 d_path;
  Parasite d_parasite;
  gint32 d_status;
};

struct _GParam
{
  GParamType type;
  GParamData data;
};


#ifdef G_OS_WIN32
/* Define WinMain() because plug-ins are built as GUI applications. Also
 * define a main() in case some plug-in still is built as a console
 * application.
 */
#  ifdef __GNUC__
#    define _stdcall __attribute__((stdcall))
#  endif

#  define MAIN()			\
   static int				\
   win32_gimp_main (int argc, char **argv)	\
   {					\
     extern void set_gimp_PLUG_IN_INFO_PTR(GPlugInInfo *);	\
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
int gimp_main (int   argc,
	       char *argv[]);

/* Forcefully causes the gimp library to exit and
 *  close down its connection to main gimp application.
 */
void G_GNUC_NORETURN gimp_quit (void);

/* Specify a range of data to be associated with 'id'.
 *  The data will exist for as long as the main gimp
 *  application is running.
 */
void gimp_set_data (gchar *  id,
		    gpointer data,
		    guint32  length);

/* Retrieve the piece of data stored within the main
 *  gimp application specified by 'id'. The data is
 *  stored in the supplied buffer.  Make sure enough
 *  space is allocated.
 */
void gimp_get_data (gchar *  id,
		    gpointer data);

/* Get the size in bytes of the data stored by a gimp_get_data
 * id. As size of zero may indicate that there is no such
 * identifier in the database.
 */
guint32 gimp_get_data_size (gchar * id);

/* Initialize the progress bar with "message". If "message"
 *  is NULL, the message displayed in the progress window will
 *  be the name of the plug-in.
 */
void gimp_progress_init (char *message);

/* Update the progress bar. If the progress bar has not been
 *  initialized then it will be automatically initialized as if
 *  "gimp_progress_init (NULL)" were called. "percentage" is a
 *  value between 0 and 1.
 */
void gimp_progress_update (gdouble percentage);

/* Returns the default gdisplay (given at plug-in config time).
 */
gint32 gimp_default_display (void);


/* Pops up a dialog box with "message". Useful for status and
 * error reports. If "message" is NULL, do nothing.
 */
void gimp_message (const gchar *message);


/* Query the gimp application's procedural database.
 *  The arguments are regular expressions which select
 *  which procedure names will be returned in 'proc_names'.
 */
void gimp_query_database (char   *name_regexp,
			  char   *blurb_regexp,
			  char   *help_regexp,
			  char   *author_regexp,
			  char   *copyright_regexp,
			  char   *date_regexp,
			  char   *proc_type_regexp,
			  int    *nprocs,
			  char ***proc_names);

/* Query the gimp application's procedural database
 *  regarding a particular procedure.
 */
gint gimp_query_procedure  (char       *proc_name,
			    char      **proc_blurb,
			    char      **proc_help,
			    char      **proc_author,
			    char      **proc_copyright,
			    char      **proc_date,
			    int        *proc_type,
			    int        *nparams,
			    int        *nreturn_vals,
			    GParamDef  **params,
			    GParamDef  **return_vals);

/* Query the gimp application regarding all open images.
 *  The list of open image id's is returned in 'image_ids'.
 */
gint32* gimp_query_images (int *nimages);


/* Install a procedure in the procedure database.
 */
void gimp_install_procedure (char      *name,
			     char      *blurb,
			     char      *help,
			     char      *author,
			     char      *copyright,
			     char      *date,
			     char      *menu_path,
			     char      *image_types,
			     int        type,
			     int        nparams,
			     int        nreturn_vals,
			     GParamDef *params,
			     GParamDef *return_vals);

/* Install a temporary procedure in the procedure database.
 */
void gimp_install_temp_proc (char      *name,
			     char      *blurb,
			     char      *help,
			     char      *author,
			     char      *copyright,
			     char      *date,
			     char      *menu_path,
			     char      *image_types,
			     int        type,
			     int        nparams,
			     int        nreturn_vals,
			     GParamDef *params,
			     GParamDef *return_vals,
			     GRunProc   run_proc);

/* Uninstall a temporary procedure
 */
void gimp_uninstall_temp_proc (char *name);

/* Install a load file format handler in the procedure database.
 */
void gimp_register_magic_load_handler (char *name,
				       char *extensions,
				       char *prefixes,
				       char *magics);

/* Install a load file format handler in the procedure database.
 */
void gimp_register_load_handler (char *name,
				 char *extensions,
				 char *prefixes);

/* Install a save file format handler in the procedure database.
 */
void gimp_register_save_handler (char *name,
				 char *extensions,
				 char *prefixes);

/* Run a procedure in the procedure database. The parameters are
 *  specified via the variable length argument list. The return
 *  values are returned in the 'GParam*' array.
 */
GParam* gimp_run_procedure (char *name,
			    int  *nreturn_vals,
			    ...);

/* Run a procedure in the procedure database. The parameters are
 *  specified as an array of GParam.  The return
 *  values are returned in the 'GParam*' array.
 */
GParam* gimp_run_procedure2 (char   *name,
			     int    *nreturn_vals,
			     int     nparams,
			     GParam *params);

/* Destroy the an array of parameters. This is useful for
 *  destroying the return values returned by a call to
 *  'gimp_run_procedure'.
 */
void gimp_destroy_params (GParam *params,
			  int     nparams);

/* Destroy the an array of GParamDef's. This is useful for
 *  destroying the return values returned by a call to
 *  'gimp_query_procedure'.
 */
void gimp_destroy_paramdefs (GParamDef *paramdefs,
			     int        nparams);

gdouble  gimp_gamma        (void);
gint     gimp_install_cmap (void);
gint     gimp_use_xshm     (void);
guchar*  gimp_color_cube   (void);
void     gimp_request_wakeups (void);


/****************************************
 *              Images                  *
 ****************************************/

gint32     gimp_image_new                   (guint      width,
					     guint      height,
					     GImageType type);
gint32     gimp_image_duplicate             (gint32     image_ID);
void       gimp_image_delete                (gint32     image_ID);
guint      gimp_image_width                 (gint32     image_ID);
guint      gimp_image_height                (gint32     image_ID);
GImageType gimp_image_base_type             (gint32     image_ID);
gint32     gimp_image_floating_selection    (gint32     image_ID);
void       gimp_image_add_channel           (gint32     image_ID,
					     gint32     channel_ID,
					     gint       position);
void       gimp_image_add_layer             (gint32     image_ID,
					     gint32     layer_ID,
					     gint       position);
void       gimp_image_add_layer_mask        (gint32     image_ID,
					     gint32     layer_ID,
					     gint32     mask_ID);
void       gimp_image_clean_all             (gint32     image_ID);
void       gimp_image_undo_disable          (gint32     image_ID);
void       gimp_image_undo_enable           (gint32     image_ID);
void       gimp_image_undo_freeze           (gint32     image_ID);
void       gimp_image_undo_thaw             (gint32     image_ID);
void       gimp_undo_push_group_start       (gint32     image_ID);
void       gimp_undo_push_group_end         (gint32     image_ID);
void       gimp_image_clean_all             (gint32     image_ID);
gint32     gimp_image_flatten               (gint32     image_ID);
void       gimp_image_lower_channel         (gint32     image_ID,
					     gint32     channel_ID);
void       gimp_image_lower_layer           (gint32     image_ID,
					     gint32     layer_ID);
gint32     gimp_image_merge_visible_layers  (gint32     image_ID,
					     GimpMergeType merge_type);
gint32     gimp_image_pick_correlate_layer  (gint32     image_ID,
					     gint       x,
					     gint       y);
void       gimp_image_raise_channel         (gint32     image_ID,
					     gint32     channel_ID);
void       gimp_image_raise_layer           (gint32     image_ID,
					     gint32     layer_ID);
void       gimp_image_remove_channel        (gint32     image_ID,
					     gint32     channel_ID);
void       gimp_image_remove_layer          (gint32     image_ID,
					     gint32     layer_ID);
void       gimp_image_remove_layer_mask     (gint32     image_ID,
					     gint32     layer_ID,
					     gint       mode);
void       gimp_image_resize                (gint32     image_ID,
					     guint      new_width,
					     guint      new_height,
					     gint       offset_x,
					     gint       offset_y);
gint32     gimp_image_get_active_channel    (gint32     image_ID);
gint32     gimp_image_get_active_layer      (gint32     image_ID);
gint32*    gimp_image_get_channels          (gint32     image_ID,
					     gint      *nchannels);
guchar*    gimp_image_get_cmap              (gint32     image_ID,
					     gint      *ncolors);
gint       gimp_image_get_component_active  (gint32     image_ID,
					     gint       component);
gint       gimp_image_get_component_visible (gint32     image_ID,
					     gint       component);
char*      gimp_image_get_filename          (gint32     image_ID);
gint32*    gimp_image_get_layers            (gint32     image_ID,
					     gint      *nlayers);
gint32     gimp_image_get_selection         (gint32     image_ID);
void       gimp_image_set_active_channel    (gint32     image_ID,
					     gint32     channel_ID);
void       gimp_image_set_active_layer      (gint32     image_ID,
					     gint32     layer_ID);
void       gimp_image_set_cmap              (gint32     image_ID,
					     guchar    *cmap,
					     gint       ncolors);
void       gimp_image_set_component_active  (gint32     image_ID,
					     gint       component,
					     gint       active);
void       gimp_image_set_component_visible (gint32     image_ID,
					     gint       component,
					     gint       visible);
void       gimp_image_set_filename          (gint32     image_ID,
					     char      *name);
Parasite  *gimp_image_parasite_find         (gint32     image_ID,
					     const char *name);
void       gimp_image_parasite_attach       (gint32      image_ID,
					     const Parasite *p);
void       gimp_image_attach_new_parasite   (gint32      image_ID,
					     const char *name, 
					     int         flags,
					     int         size, 
					     const void *data);
void       gimp_image_parasite_detach       (gint32      image_ID,
					     const char *name);
void       gimp_image_set_resolution        (gint32     image_ID,
					     double     xresolution,
					     double     yresolution);
void       gimp_image_get_resolution        (gint32     image_ID,
					     double     *xresolution,
					     double     *yresolution);
void       gimp_image_set_unit              (gint32     image_ID,
					     GimpUnit   unit);
GimpUnit   gimp_image_get_unit              (gint32     image_ID);
gint32     gimp_image_get_layer_by_tattoo   (gint32     image_ID,
					     gint32     tattoo);
gint32     gimp_image_get_channel_by_tattoo (gint32     image_ID,
					     gint32     tattoo);

guchar *   gimp_image_get_thumbnail_data    (gint32     image_ID,
					     gint      *width,
					     gint      *height,
					     gint      *bytes);
void       gimp_image_convert_rgb           (gint32     image_ID);
void       gimp_image_convert_grayscale     (gint32     image_ID);
void       gimp_image_convert_indexed       (gint32     image_ID,
					     GimpConvertDitherType  dither_type,
					     GimpConvertPaletteType palette_type,
					     gint       num_colors,
					     gint       alpha_dither,
					     gint       remove_unused,
					     gchar     *palette);

/****************************************
 *              Guides                  *
 ****************************************/

gint32       gimp_image_add_hguide              (gint32     image_ID,
						 gint32     yposition);
gint32       gimp_image_add_vguide              (gint32     image_ID,
						 gint32     xposition);
void         gimp_image_delete_guide            (gint32     image_ID,
						 gint32     guide_ID);
gint32       gimp_image_find_next_guide         (gint32     image_ID,
						 gint32     guide_ID);
GOrientation gimp_image_get_guide_orientation   (gint32     image_ID,
					         gint32     guide_ID);
gint32       gimp_image_get_guide_position      (gint32     image_ID,
						 gint32     guide_ID);


/****************************************
 *             Displays                 *
 ****************************************/

gint32 gimp_display_new    (gint32 image_ID);
void   gimp_display_delete (gint32 display_ID);
void   gimp_displays_flush (void);


/****************************************
 *              Layers                  *
 ****************************************/

gint32        gimp_layer_new                       (gint32        image_ID,
						    char         *name,
						    guint         width,
						    guint         height,
						    GDrawableType  type,
						    gdouble       opacity,
						    GLayerMode    mode);
gint32        gimp_layer_copy                      (gint32        layer_ID);
void          gimp_layer_delete                    (gint32        layer_ID);
guint         gimp_layer_width                     (gint32        layer_ID);
guint         gimp_layer_height                    (gint32        layer_ID);
guint         gimp_layer_bpp                       (gint32        layer_ID);
GDrawableType gimp_layer_type                      (gint32        layer_ID);
void          gimp_layer_add_alpha                 (gint32        layer_ID);
gint32        gimp_layer_create_mask               (gint32        layer_ID,
						    GimpAddMaskType mask_type);
void          gimp_layer_resize                    (gint32        layer_ID,
						    guint         new_width,
						    guint         new_height,
						    gint          offset_x,
						    gint          offset_y);
void          gimp_layer_scale                     (gint32        layer_ID,
						    guint         new_width,
						    guint         new_height,
						    gint          local_origin);
void          gimp_layer_translate                 (gint32        layer_ID,
						    gint          offset_x,
						    gint          offset_y);
gint          gimp_layer_is_floating_selection     (gint32        layer_ID);
gint32        gimp_layer_get_image_id              (gint32        layer_ID);
gint32        gimp_layer_get_mask_id               (gint32        layer_ID);
gint          gimp_layer_get_apply_mask            (gint32        layer_ID);
gint          gimp_layer_get_edit_mask             (gint32        layer_ID);
GLayerMode    gimp_layer_get_mode                  (gint32        layer_ID);
char*         gimp_layer_get_name                  (gint32        layer_ID);
gdouble       gimp_layer_get_opacity               (gint32        layer_ID);
gint          gimp_layer_get_preserve_transparency (gint32        layer_ID);
gint          gimp_layer_get_show_mask             (gint32        layer_ID);
gint          gimp_layer_get_visible               (gint32        layer_ID);
void          gimp_layer_set_apply_mask            (gint32        layer_ID,
						    gint          apply_mask);
void          gimp_layer_set_edit_mask             (gint32        layer_ID,
						    gint          edit_mask);
void          gimp_layer_set_mode                  (gint32        layer_ID,
						    GLayerMode    mode);
void          gimp_layer_set_name                  (gint32        layer_ID,
						    char         *name);
void          gimp_layer_set_offsets               (gint32        layer_ID,
						    gint          offset_x,
						    gint          offset_y);
void          gimp_layer_set_opacity               (gint32        layer_ID,
						    gdouble       opacity);
void          gimp_layer_set_preserve_transparency (gint32        layer_ID,
						    gint          preserve_transparency);
void          gimp_layer_set_show_mask             (gint32        layer_ID,
						    gint          show_mask);
void          gimp_layer_set_visible               (gint32        layer_ID,
						    gint          visible);
gint32        gimp_layer_get_tattoo                (gint32        layer_ID);


/****************************************
 *             Channels                 *
 ****************************************/

gint32  gimp_channel_new             (gint32   image_ID,
				      char    *name,
				      guint    width,
				      guint    height,
				      gdouble  opacity,
				      guchar  *color);
gint32  gimp_channel_copy            (gint32   channel_ID);
void    gimp_channel_delete          (gint32   channel_ID);
guint   gimp_channel_width           (gint32   channel_ID);
guint   gimp_channel_height          (gint32   channel_ID);
gint32  gimp_channel_get_image_id    (gint32   channel_ID);
gint32  gimp_channel_get_layer_id    (gint32   channel_ID);
void    gimp_channel_get_color       (gint32   channel_ID,
				      guchar  *red,
				      guchar  *green,
				      guchar  *blue);
char*   gimp_channel_get_name        (gint32   channel_ID);
gdouble gimp_channel_get_opacity     (gint32   channel_ID);
gint    gimp_channel_get_show_masked (gint32   channel_ID);
gint    gimp_channel_get_visible     (gint32   channel_ID);
void    gimp_channel_set_color       (gint32   channel_ID,
				      guchar   red,
				      guchar   green,
				      guchar   blue);
void    gimp_channel_set_name        (gint32   channel_ID,
				      char    *name);
void    gimp_channel_set_opacity     (gint32   channel_ID,
				      gdouble  opacity);
void    gimp_channel_set_show_masked (gint32   channel_ID,
				      gint     show_masked);
void    gimp_channel_set_visible     (gint32   channel_ID,
				      gint     visible);
gint32  gimp_channel_get_tattoo      (gint32   channel_ID);


/****************************************
 *             GDrawables                *
 ****************************************/

GDrawable*    gimp_drawable_get          (gint32     drawable_ID);
void          gimp_drawable_detach       (GDrawable *drawable);
void          gimp_drawable_flush        (GDrawable *drawable);
void          gimp_drawable_delete       (GDrawable *drawable);
void          gimp_drawable_update       (gint32     drawable_ID,
					  gint       x,
					  gint       y,
					  guint      width,
					  guint      height);
void          gimp_drawable_merge_shadow (gint32     drawable_ID,
					  gint       undoable);
gint32        gimp_drawable_image_id     (gint32     drawable_ID);
char*         gimp_drawable_name         (gint32     drawable_ID);
guint         gimp_drawable_width        (gint32     drawable_ID);
guint         gimp_drawable_height       (gint32     drawable_ID);
guint         gimp_drawable_bpp          (gint32     drawable_ID);
GDrawableType gimp_drawable_type         (gint32     drawable_ID);
gint          gimp_drawable_visible      (gint32     drawable_ID);
gint          gimp_drawable_is_channel   (gint32     drawable_ID);
gint          gimp_drawable_is_rgb       (gint32     drawable_ID);
gint          gimp_drawable_is_gray      (gint32     drawable_ID);
gint          gimp_drawable_has_alpha    (gint32     drawable_ID);
gint          gimp_drawable_is_indexed   (gint32     drawable_ID);
gint          gimp_drawable_is_layer     (gint32     drawable_ID);
gint          gimp_drawable_is_layer_mask(gint32     drawable_ID);
gint          gimp_drawable_mask_bounds  (gint32     drawable_ID,
					  gint      *x1,
					  gint      *y1,
					  gint      *x2,
					  gint      *y2);
void          gimp_drawable_offsets      (gint32     drawable_ID,
					  gint      *offset_x,
					  gint      *offset_y);
void          gimp_drawable_fill         (gint32     drawable_ID,
					  GimpFillType fill_type);
void          gimp_drawable_set_name     (gint32     drawable_ID,
					  char      *name);
void          gimp_drawable_set_visible  (gint32     drawable_ID,
					  gint       visible);
GTile*        gimp_drawable_get_tile     (GDrawable *drawable,
					  gint       shadow,
					  gint       row,
					  gint       col);
GTile*        gimp_drawable_get_tile2    (GDrawable *drawable,
					  gint       shadow,
					  gint       x,
					  gint       y);
Parasite*     gimp_drawable_parasite_find       (gint32          drawable,
						 const char     *name);
void          gimp_drawable_parasite_attach     (gint32          drawable,
						 const Parasite *p);
void          gimp_drawable_attach_new_parasite (gint32          drawable,
						 const char     *name, 
						 int             flags,
						 int             size, 
						 const void     *data);
void          gimp_drawable_parasite_detach     (gint32          drawable,
						 const char     *name);
guchar*       gimp_drawable_get_thumbnail_data  (gint32          drawable_ID,
						 gint           *width,
						 gint           *height,
						 gint           *bytes);


/****************************************
 *              Selections              *
 ****************************************/

gint32        gimp_selection_bounds      (gint32     image_ID,
					  gint32    *non_empty,
					  gint32    *x1,
					  gint32    *y1,
					  gint32    *x2,
					  gint32    *y2);
gint32        gimp_selection_float       (gint32     image_ID, 
					  gint32     drawable_ID,
					  gint32     x_offset,
					  gint32     y_offset);
gint32        gimp_selection_is_empty    (gint32     image_ID);
void          gimp_selection_none        (gint32     image_ID);


/****************************************
 *               GTiles                  *
 ****************************************/

void  gimp_tile_ref          (GTile   *tile);
void  gimp_tile_ref_zero     (GTile   *tile);
void  gimp_tile_unref        (GTile   *tile,
			      int     dirty);
void  gimp_tile_flush        (GTile   *tile);
void  gimp_tile_cache_size   (gulong  kilobytes);
void  gimp_tile_cache_ntiles (gulong  ntiles);
guint gimp_tile_width        (void);
guint gimp_tile_height       (void);


/****************************************
 *           Pixel Regions              *
 ****************************************/

void     gimp_pixel_rgn_init      (GPixelRgn *pr,
				   GDrawable *drawable,
				   int        x,
				   int        y,
				   int        width,
				   int        height,
				   int        dirty,
				   int        shadow);
void     gimp_pixel_rgn_resize    (GPixelRgn *pr,
				   int        x,
				   int        y,
				   int        width,
				   int        height);
void     gimp_pixel_rgn_get_pixel (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y);
void     gimp_pixel_rgn_get_row   (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        width);
void     gimp_pixel_rgn_get_col   (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        height);
void     gimp_pixel_rgn_get_rect  (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        width,
				   int        height);
void     gimp_pixel_rgn_set_pixel (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y);
void     gimp_pixel_rgn_set_row   (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        width);
void     gimp_pixel_rgn_set_col   (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        height);
void     gimp_pixel_rgn_set_rect  (GPixelRgn *pr,
				   guchar    *buf,
				   int        x,
				   int        y,
				   int        width,
				   int        height);
gpointer gimp_pixel_rgns_register (int        nrgns,
				   ...);
gpointer gimp_pixel_rgns_register2(int        nrgns,
                                   GPixelRgn **prs);
gpointer gimp_pixel_rgns_process  (gpointer   pri_ptr);


/****************************************
 *            The Palette               *
 ****************************************/

void gimp_palette_get_background (guchar *red,
				  guchar *green,
				  guchar *blue);
void gimp_palette_get_foreground (guchar *red,
				  guchar *green,
				  guchar *blue);
void gimp_palette_set_background (guchar  red,
				  guchar  green,
				  guchar  blue);
void gimp_palette_set_foreground (guchar  red,
				  guchar  green,
				  guchar  blue);

/****************************************
 *            Gradients                 *
 ****************************************/

char**   gimp_gradients_get_list       (gint    *num_gradients);
char*    gimp_gradients_get_active     (void);
void     gimp_gradients_set_active     (char    *name);
gdouble* gimp_gradients_sample_uniform (gint     num_samples);
gdouble* gimp_gradients_sample_custom  (gint     num_samples,
					gdouble *positions);

/****************************************
 *            Parasites                 *
 ****************************************/

Parasite *gimp_parasite_find       (const char     *name);
void      gimp_parasite_attach     (const Parasite *p);
void      gimp_attach_new_parasite (const char     *name, 
				    int             flags,
				    int             size, 
				    const void     *data);
void      gimp_parasite_detach     (const char     *name);

/****************************************
 *                Help                  *
 ****************************************/

void      gimp_plugin_help_func    (gchar *help_data);
void      gimp_help                (gchar *help_data);

/****************************************
 *           Localisation               *
 ****************************************/

void      gimp_plugin_domain_add           (gchar *domain_name);
void      gimp_plugin_domain_add_with_path (gchar *domain_name, 
					    gchar *domain_path);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GIMP_H__ */
