/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GFlare plug-in -- lense flare effect by using custom gradients
 * Copyright (C) 1997 Eiichi Takamori <taka@ma1.sekyou.ne.jp>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * A fair proportion of this code was taken from GIMP & Script-fu
 * copyrighted by Spencer Kimball and Peter Mattis, and from Gradient
 * Editor copyrighted by Federico Mena Quintero. (See copyright notice
 * below) Thanks for senior GIMP hackers!!
 *
 * The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Gradient editor module copyight (C) 1996-1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
 */

/*
  version 0.27
  Changed so that it works with GIMP 1.1.x
  Default problem solved
  martweb@gmx.net
 */

#ifdef RCSID
static char rcsid[] = "$Id$";
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "asupsample.h"
#include "gtkmultioptionmenu.h"

/* #define DEBUG */

#ifdef DEBUG
#define DEBUG_PRINT(X) printf X
#else
#define DEBUG_PRINT(X)
#endif

#define BOUNDS(a,x,y)	((a < x) ? x : ((a > y) ? y : a))
#define LUMINOSITY(PIX) (PIX[0] * 0.30 + PIX[1] * 0.59 + PIX[2] * 0.11)
#define OFFSETOF(t,f)	((int) ((char*) &((t*) 0)->f))

#define GRADIENT_NAME_MAX 256
#define GRADIENT_RESOLUTION 360

#define GFLARE_NAME_MAX 256
#define GFLARE_FILE_HEADER   "GIMP GFlare 0.25\n"
#define SFLARE_NUM	30

#define DLG_PREVIEW_WIDTH 256
#define DLG_PREVIEW_HEIGHT 256
#define DLG_PREVIEW_MASK   GDK_EXPOSURE_MASK | \
			   GDK_BUTTON_PRESS_MASK
#define DLG_LISTBOX_WIDTH 80
#define DLG_LISTBOX_HEIGHT 40

#define ED_PREVIEW_WIDTH 256
#define ED_PREVIEW_HEIGHT 256

#define GM_PREVIEW_WIDTH	80
#define GM_PREVIEW_HEIGHT	16
#define GM_MENU_MAX   20

#define ENTSCALE_SCALE_WIDTH 100
#define ENTSCALE_ENTRY_WIDTH 50

#define ENTRY_WIDTH 40

#ifndef OPAQUE
#define OPAQUE	   255
#endif
#define GRAY50	   128

/* drawable coord to preview coord */
#define DX2PX(DX)  ((double) PREVIEW_WIDTH * ((DX) - dinfo->win.x0) / (dinfo->win.x1 - dinfo->win.x0))
#define DY2PY(DY)  ((double) PREVIEW_HEIGHT * ((DY) - dinfo->win.y0) / (dinfo->win.y1 - dinfo->win.y0))
#define GRADIENT_CACHE_SIZE	32

#define CALC_GLOW	0x01
#define CALC_RAYS	0x02
#define CALC_SFLARE	0x04

typedef struct _Preview Preview;

typedef gchar	GradientName[GRADIENT_NAME_MAX];

typedef enum
{
  GF_NORMAL = 0,
  GF_ADDITION,
  GF_OVERLAY,
  GF_SCREEN,
  GF_NUM_MODES
} GFlareMode;

typedef enum
{
  GF_CIRCLE = 0,
  GF_POLYGON,
  GF_NUM_SHAPES
} GFlareShape;

typedef struct {
  gchar		*name;
  gchar		*filename;
  gdouble	glow_opacity;
  GFlareMode	glow_mode;
  gdouble	rays_opacity;
  GFlareMode	rays_mode;
  gdouble	sflare_opacity;
  GFlareMode	sflare_mode;
  GradientName	glow_radial;
  GradientName	glow_angular;
  GradientName	glow_angular_size;
  gdouble	glow_size;
  gdouble	glow_rotation;
  gdouble	glow_hue;
  GradientName	rays_radial;
  GradientName	rays_angular;
  GradientName	rays_angular_size;
  gdouble	rays_size;
  gdouble	rays_rotation;
  gdouble	rays_hue;
  gint		rays_nspikes;
  gdouble	rays_thickness;
  GradientName	sflare_radial;
  GradientName	sflare_sizefac;
  GradientName	sflare_probability;
  gdouble	sflare_size;
  gdouble	sflare_rotation;
  gdouble	sflare_hue;
  GFlareShape	sflare_shape;
  gint		sflare_nverts;
  gint		sflare_seed;
} GFlare;

typedef struct {
  FILE		*fp;
  int		error;
} GFlareFile;


typedef enum {
  PAGE_SETTINGS,
  PAGE_SELECTOR,
  PAGE_GENERAL,
  PAGE_GLOW,
  PAGE_RAYS,
  PAGE_SFLARE
} PageNum;


typedef struct {
  gint		init;
  GFlare	*gflare;
  GtkWidget	*shell;
  Preview	*preview;
  struct {
    gdouble x0, y0, x1, y1;
  }		pwin;
  gint		update_preview;
  GtkWidget	*notebook;
  GtkWidget	*xentry;
  gint		xentry_id;
  GtkWidget	*yentry;
  gint		yentry_id;
  GtkWidget	*asupsample_frame;
  GtkWidget	*selector_list;
  gint		init_params_done;
} GFlareDialog;

typedef void	(*GFlareEditorCallback) (gint updated, gpointer data);

typedef struct {
  gint		init;
  gint		run;
  GFlareEditorCallback	callback;
  gpointer	calldata;
  GFlare	*target_gflare;
  GFlare	*gflare;
  GtkWidget	*shell;
  Preview	*preview;
  GtkWidget	*notebook;
  PageNum	cur_page;
  GtkWidget	*polygon_entry;
  GtkWidget	*polygon_toggle;
  gint		init_params_done;
} GFlareEditor;

typedef struct
{
  gdouble	x0;
  gdouble	y0;
  gdouble	x1;
  gdouble	y1;
} CalcBounds;

typedef struct
{
  gint		init;
  gint		type;
  GFlare *	gflare;
  gdouble	xcenter;
  gdouble	ycenter;
  gdouble	radius;
  gdouble	rotation;
  gdouble	hue;
  gdouble	vangle;
  gdouble	vlength;

  gint		glow_opacity;
  CalcBounds	glow_bounds;
  guchar *	glow_radial;
  guchar *	glow_angular;
  guchar *	glow_angular_size;
  gdouble	glow_radius;
  gdouble	glow_rotation;

  gint		rays_opacity;
  CalcBounds	rays_bounds;
  guchar *	rays_radial;
  guchar *	rays_angular;
  guchar *	rays_angular_size;
  gdouble	rays_radius;
  gdouble	rays_rotation;
  gdouble	rays_spike_mod;
  gdouble	rays_thinness;

  gint		sflare_opacity;
  GList *	sflare_list;
  guchar *	sflare_radial;
  guchar *	sflare_sizefac;
  guchar *	sflare_probability;
  gdouble	sflare_radius;
  gdouble	sflare_rotation;
  GFlareShape	sflare_shape;
  gdouble	sflare_angle;
  gdouble	sflare_factor;
} CalcParams;

/*
 * What's the difference between (structure) CalcParams and GFlare ?
 * well, radius and lengths are actual length for CalcParams where
 * they are typically 0 to 100 for GFlares, and angles are M_PI based
 * (radian) for CalcParams where they are degree for GFlares. et cetra.
 * This is because convienience for dialog processing and for calculating.
 * these conversion is taken place in calc init routines. see below.
 */

typedef struct
{
  gdouble	xcenter;
  gdouble	ycenter;
  gdouble	radius;
  CalcBounds	bounds;
} CalcSFlare;


typedef struct
{
  gint			is_color;
  gint			has_alpha;
  gint			x1, y1, x2, y2;		/* mask bounds */
  gint			tile_width, tile_height;
			/* these values don't belong to drawable, though. */
} DrawableInfo;

typedef struct
{
  GTile *	tile;
  gint		col;
  gint		row;
  gint		shadow;
  gint		dirty;
} TileKeeper;

typedef struct _GradientMenu	GradientMenu;
typedef void (*GradientMenuCallback) ( gchar *gradient_name,
				       gpointer data );
struct _GradientMenu {
  GtkWidget		*preview;
  GtkWidget		*option_menu;
  GradientMenuCallback	callback;
  gpointer		callback_data;
  GradientName		gradient_name;
};

typedef gint (*PreviewInitFunc) (Preview *preview, gpointer data);
typedef void (*PreviewRenderFunc) (Preview *preview, guchar *buffer, gint y, gpointer data);
typedef void (*PreviewDeinitFunc) (Preview *preview, gpointer data);

struct _Preview
{
  GtkWidget		*widget;
  gint			width;
  gint			height;
  PreviewInitFunc	init_func;
  gpointer		init_data;
  PreviewRenderFunc	render_func;
  gpointer		render_data;
  PreviewDeinitFunc	deinit_func;
  gpointer		deinit_data;
  guint			timeout_tag;
  guint			idle_tag;
  gint			init_done;
  gint			current_y;
  gint			drawn_y;
  guchar		*buffer;
}; /* Preview */

typedef struct
{
  gint			tag;
  gint			got_gradients;
  gint			current_y;
  gint			drawn_y;
  PreviewRenderFunc	render_func;
  guchar		*buffer;
} PreviewIdle;

typedef struct _GradientCacheItem  GradientCacheItem;
struct _GradientCacheItem
{
  GradientCacheItem	*next;
  GradientCacheItem	*prev;
  GradientName		name;
  guchar		values[4*GRADIENT_RESOLUTION];
};



typedef enum {
  ENTSCALE_INT,
  ENTSCALE_DOUBLE
} EntscaleType;

typedef void (*EntscaleCallbackFunc) (gdouble new_val, gpointer data);

typedef struct {
  GtkObject		*adjustment;
  GtkWidget		*entry;
  EntscaleType		type;
  gchar			fmt_string[16];
  gint			constraint;
  EntscaleCallbackFunc	callback;
  gpointer		call_data;
} Entscale;


typedef struct {
  gint		xcenter;
  gint		ycenter;
  gdouble	radius;
  gdouble	rotation;
  gdouble	hue;
  gdouble	vangle;
  gdouble	vlength;
  gint		use_asupsample;
  gint		asupsample_max_depth;
  gdouble	asupsample_threshold;
  gchar		gflare_name[GFLARE_NAME_MAX];
} PluginValues;

typedef struct {
  gint		run;
} PluginInterface;

typedef void (*QueryFunc) (GtkWidget *, gpointer, gpointer);

/***
***	Global Functions Prototypes
**/

/* *** INSERT-FILE "proto/gflare.c.pro" *** */
/* gflare.c */
int main (int argc, char **argv);
void plugin_query (void);
void plugin_run (gchar *name, gint nparams, GParam *param, gint *nreturn_vals, GParam **return_vals);
void plug_in_parse_gflare_path (void);
GFlare *gflare_new_with_default (gchar *new_name);
GFlare *gflare_dup (GFlare *src, gchar *new_name);
void gflare_copy (GFlare *dest, GFlare *src);
GFlare *gflare_load (char *filename, char *name);
void gflare_save (GFlare *gflare);
void gflare_name_copy (gchar *dest, gchar *src);
gint gflares_list_insert (GFlare *gflare);
GFlare *gflares_list_lookup (gchar *name);
gint gflares_list_index (GFlare *gflare);
gint gflares_list_remove (GFlare *gflare);
void gflares_list_load_all (void);
void gflares_list_free_all (void);
void calc_init_params (GFlare *gflare, gint calc_type, gdouble xcenter, gdouble ycenter, gdouble radius, gdouble rotation, gdouble hue, gdouble vangle, gdouble vlength);
int calc_init_progress (void);
void calc_deinit (void);
void calc_glow_pix (guchar *dest_pix, gdouble x, gdouble y);
void calc_rays_pix (guchar *dest_pix, gdouble x, gdouble y);
void calc_sflare_pix (guchar *dest_pix, gdouble x, gdouble y, guchar *src_pix);
void calc_gflare_pix (guchar *dest_pix, gdouble x, gdouble y, guchar *src_pix);
int dlg_run (void);
void dlg_preview_calc_window (void);
void ed_preview_calc_window (void);
GtkWidget *ed_mode_menu_new (GFlareMode *mode_var);
Preview *preview_new (gint width, gint height, PreviewInitFunc init_func, gpointer init_data, PreviewRenderFunc render_func, gpointer render_data, PreviewDeinitFunc deinit_func, gpointer deinit_data);
void preview_free (Preview *preview);
void preview_render_start (Preview *preview);
void preview_render_end (Preview *preview);
void preview_rgba_to_rgb (guchar *dest, gint x, gint y, guchar *src);
void gradient_menu_init (void);
void gradient_menu_rescan (void);
GradientMenu *gradient_menu_new (GradientMenuCallback callback, gpointer callback_data, gchar *default_gradient_name);
void gradient_menu_destroy (GradientMenu *gm);
void gradient_name_copy (gchar *dest, gchar *src);
void gradient_name_encode (guchar *dest, guchar *src);
void gradient_name_decode (guchar *dest, guchar *src);
void gradient_init (void);
void gradient_free (void);
char **gradient_get_list (gint *num_gradients);
void gradient_get_values (gchar *gradient_name, guchar *values, gint nvalues);
void gradient_cache_flush (void);
void entscale_new (GtkWidget *table, gint x, gint y, gchar *caption, EntscaleType type, gpointer variable, gdouble min, gdouble max, gdouble step, gint constraint, EntscaleCallbackFunc callback, gpointer call_data);
GtkWidget *query_string_box (char *title, char *message, char *initial, QueryFunc callback, gpointer data);

/* *** INSERT-FILE-END *** */

/**
***	Variables
**/

GPlugInInfo PLUG_IN_INFO =
{
  NULL,	   /* init_proc */
  NULL,	   /* quit_proc */
  plugin_query,	  /* query_proc */
  plugin_run,	   /* run_proc */
};

PluginValues pvals =
{
  128,				/* xcenter */
  128,				/* ycenter */
  100.0,			/* radius */
  0.0,				/* rotation */
  0.0,				/* hue */
  60.0,				/* vangle */
  400.0,			/* vlength */
  FALSE,			/* use_asupsample */
  3,				/* asupsample_max_depth */
  0.2,				/* asupsample_threshold */
  "Default"			/* gflare_name */
};

PluginInterface pint =
{
  FALSE				/* run */
};

GFlare default_gflare =
{
  NULL,				/* name */
  NULL,				/* filename */
  100,				/* glow_opacity */
  GF_NORMAL,			/* glow_mode */
  100,				/* rays_opacity */
  GF_NORMAL,			/* rays_mode */
  100,				/* sflare_opacity */
  GF_NORMAL,			/* sflare_mode */
  "%red_grad",			/* glow_radial */
  "%white",			/* glow_angular */
  "%white",			/* glow_angular_size */
  100.0,			/* glow_size */
  0.0,				/* glow_rotation */
  0.0,				/* glow_hue */
  "%white_grad",		/* rays_radial */
  "%random",			/* rays_angular */
  "%random",			/* rays_angular_size */
  100.0,			/* rays_size */
  0.0,				/* rays_rotation */
  0.0,				/* rays_hue */
  40,				/* rays_nspikes */
  20.0,				/* rays_thickness */
  "%white_grad",		/* sflare_radial */
  "%random",			/* sflare_sizefac */
  "%random",			/* sflare_probability */
  40.0,				/* sflare_size */
  0.0,				/* sflare_rotation */
  0.0,				/* sflare_hue */
  GF_CIRCLE,			/* sflare_shape */
  6,				/* sflare_nverts */
  1				/* sflare_seed */
};

/* These are keywords to be written to disk files specifying flares. */
/* They are not translated since we want gflare files to be compatible
   across languages. */
static gchar *gflare_modes[] =
{
  "NORMAL",
  "ADDITION",
  "OVERLAY",
  "SCREEN"
};

static gchar *gflare_shapes[] =
{
  "CIRCLE",
  "POLYGON"
};

/* These are for menu entries, so they are translated. */
static gchar *gflare_menu_modes[] =
{
  N_("Normal"),
  N_("Addition"),
  N_("Overlay"),
  N_("Screen")
};

static GDrawable *		drawable;
static DrawableInfo		dinfo;
static TileKeeper *		tk_read;
static TileKeeper *		tk_write;
static GFlareDialog *		dlg = NULL;
static GFlareEditor *		ed = NULL;
static GList *			gflares_list = NULL;
static gint			num_gflares = 0;
static GList *			gflare_path_list;
static CalcParams		calc;
GList *				gradient_menus;
static char **			gradient_names = NULL;
static int			num_gradient_names = 0;
static GradientCacheItem *	gradient_cache_head  = NULL;
static gint			gradient_cache_count = 0;


static char *internal_gradients[] =
{
  "%white", "%white_grad", "%red_grad", "%blue_grad", "%yellow_grad", "%random"
};
static int internal_ngradients = sizeof (internal_gradients) / sizeof (internal_gradients[0]);

#ifdef DEBUG
static int	get_values_external_count = 0;
static clock_t	get_values_external_clock = 0;
#endif


/**
***	+++ Static Functions Prototypes
**/

/* *** INSERT-FILE "proto/gflare.c.prs" *** */
 static void plugin_do (void);
 static void plugin_do_non_asupsample (void);
 static void plugin_do_asupsample (void);
 static void plugin_render_func (double x, double y, color_t *color, gpointer data);
 static void plugin_put_pixel_func (int ix, int iy, color_t color, gpointer data);
 static void plugin_progress_func (int y1, int y2, int curr_y, gpointer data);
 static TileKeeper *tile_keeper_new (gint shadow);
 static guchar *tile_keeper_provide (TileKeeper *tk, gint ix, gint iy, gint dirty);
 static void tile_keeper_free (TileKeeper *tk);
 static GFlare *gflare_new (void);
 static void gflare_free (GFlare *gflare);
 static void gflare_read_int (gint *intvar, GFlareFile *gf);
 static void gflare_read_double (gdouble *dblvar, GFlareFile *gf);
 static void gflare_read_gradient_name (gchar *name, GFlareFile *gf);
 static void gflare_read_shape (GFlareShape *shape, GFlareFile *gf);
 static void gflare_read_mode (GFlareMode *mode, GFlareFile *gf);
 static void gflare_write_gradient_name (gchar *name, FILE *fp);
 static gint calc_sample_one_gradient (void);
 static void calc_place_sflare (void);
 static void calc_get_gradient (guchar *pix, guchar *gradient, gdouble pos);
 static gdouble fmod_positive (gdouble x, gdouble m);
 static void calc_paint_func (guchar *dest, guchar *src1, guchar *src2, gint opacity, GFlareMode mode);
 static void calc_combine (guchar *dest, guchar *src1, guchar *src2, gint opacity);
 static void calc_addition (guchar *dest, guchar *src1, guchar *src2);
 static void calc_screen (guchar *dest, guchar *src1, guchar *src2);
 static void calc_overlay (guchar *dest, guchar *src1, guchar *src2);
 static void dlg_ok_callback (GtkWidget *widget, gpointer data);
 static void dlg_close_callback (GtkWidget *widget, gpointer data);
 static void dlg_setup_gflare (void);
 static void dlg_page_map_callback (GtkWidget *widget, gpointer data);
 static gint dlg_preview_handle_event (GtkWidget *widget, GdkEvent *event);
 static gint ed_preview_handle_event (GtkWidget *widget, GdkEvent *event);
 static void dlg_preview_update (void);
 static gint dlg_preview_init_func (Preview *preview, gpointer data);
 static void dlg_preview_render_func (Preview *preview, guchar *dest, gint y, gpointer data);
 static void dlg_preview_deinit_func (Preview *preview, gpointer data);
 static void dlg_make_page_settings (GFlareDialog *dlg, GtkWidget *notebook);
 static void dlg_position_entry_callback (GtkWidget *widget, gpointer data);
 static void dlg_entscale_callback (gdouble new_val, gpointer data);
 static void dlg_use_asupsample_callback (GtkWidget *widget, gpointer data);
 static void dlg_iscale_callback (GtkAdjustment *adjustment, gpointer data);
 static void dlg_dscale_callback (GtkAdjustment *adjustment, gpointer data);
 static void dlg_update_preview_callback (GtkWidget *widget, gpointer data);
 static void dlg_make_page_selector (GFlareDialog *dlg, GtkWidget *notebook);
 static void dlg_selector_setup_listbox (void);
 static void dlg_selector_insert (GFlare *gflare, int pos, int select);
 static void dlg_selector_list_item_callback (GtkWidget *widget, gpointer data);
 static void dlg_selector_new_callback (GtkWidget *widget, gpointer data);
 static void dlg_selector_new_ok_callback (GtkWidget *widget, gpointer client_data, gpointer call_data);
 static void dlg_selector_edit_callback (GtkWidget *widget, gpointer data);
 static void dlg_selector_edit_done_callback (gint updated, gpointer data);
 static void dlg_selector_copy_callback (GtkWidget *widget, gpointer data);
 static void dlg_selector_copy_ok_callback (GtkWidget *widget, gpointer client_data, gpointer call_data);
 static void dlg_selector_delete_callback (GtkWidget *widget, gpointer data);
 static void dlg_selector_delete_ok_callback (GtkWidget *widget, gpointer client_data);
 static void dlg_selector_delete_cancel_callback (GtkWidget *widget, gpointer client_data);
 static void ed_run (GFlare *target_gflare, GFlareEditorCallback callback, gpointer calldata);
 static void ed_close_callback (GtkWidget *widget, gpointer data);
 static void ed_ok_callback (GtkWidget *widget, gpointer data);
 static void ed_rescan_callback (GtkWidget *widget, gpointer data);
 static void ed_make_page_general (GFlareEditor *ed, GtkWidget *notebook);
 static void ed_make_page_glow (GFlareEditor *ed, GtkWidget *notebook);
 static void ed_make_page_rays (GFlareEditor *ed, GtkWidget *notebook);
 static void ed_make_page_sflare (GFlareEditor *ed, GtkWidget *notebook);
 static void ed_put_mode_menu (GtkWidget *table, gint x, gint y, gchar *caption, GtkWidget *option_menu);
 static void ed_put_gradient_menu (GtkWidget *table, gint x, gint y, gchar *caption, GradientMenu *gm);
 static void ed_entscale_callback (gdouble new_val, gpointer data);
 static void ed_mode_menu_callback (GtkWidget *widget, gpointer data);
 static void ed_gradient_menu_callback (gchar *gradient_name, gpointer data);
 static void ed_shape_radio_callback (GtkWidget *widget, gpointer data);
 static void ed_ientry_callback (GtkWidget *widget, gpointer data);
 static void ed_page_map_callback (GtkWidget *widget, gpointer data);
 static void ed_preview_update (void);
 static gint ed_preview_init_func (Preview *preview, gpointer data);
 static void ed_preview_deinit_func (Preview *preview, gpointer data);
 static void ed_preview_render_func (Preview *preview, guchar *buffer, gint y, gpointer data);
 static void ed_preview_render_general (guchar *buffer, gint y);
 static void ed_preview_render_glow (guchar *buffer, gint y);
 static void ed_preview_render_rays (guchar *buffer, gint y);
 static void ed_preview_render_sflare (guchar *buffer, gint y);
 static gint preview_render_start_2 (Preview *preview);
 static gint preview_handle_idle (Preview *preview);
 static void gm_gradient_get_list (void);
 static GtkWidget *gm_menu_new (GradientMenu *gm, gchar *default_gradient_name);
 static GtkWidget *gm_menu_create_sub_menus (GradientMenu *gm, gint start_n, gchar **active_name_ptr, gchar *default_gradient_name);
 static void gm_menu_item_callback (GtkWidget *w, gpointer data);
 static void gm_preview_draw (GtkWidget *preview, gchar *gradient_name);
 static void gm_option_menu_destroy_callback (GtkWidget *w, gpointer data);
 static void gradient_get_values_internal (gchar *gradient_name, guchar *values, gint nvalues);
 static void gradient_get_blend (guchar *fg, guchar *bg, guchar *values, gint nvalues);
 static void gradient_get_random (gint seed, guchar *values, gint nvalues);
 static void gradient_get_default (gchar *name, guchar *values, gint nvalues);
 static void gradient_get_values_external (gchar *gradient_name, guchar *values, gint nvalues);
 static void gradient_get_values_real_external (gchar *gradient_name, guchar *values, gint nvalues);
 static GradientCacheItem *gradient_cache_lookup (gchar *name, gint *found);
 static void gradient_cache_zorch (void);
 static int entscale_get_precision (gdouble step);
 static void entscale_destroy_callback (GtkWidget *widget, gpointer data);
 static void entscale_scale_update (GtkAdjustment *adjustment, gpointer data);
 static void entscale_entry_update (GtkWidget *widget, gpointer data);
 static void query_box_cancel_callback (GtkWidget *w, gpointer client_data);
 static void query_box_ok_callback (GtkWidget *w, gpointer client_data);

/* *** INSERT-FILE-END *** */


/*************************************************************************/
/**									**/
/**		+++ Plug-in Interfaces					**/
/**									**/
/*************************************************************************/

MAIN ();

void
plugin_query()
{
  static GParamDef args[]=
    {
      { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
      { PARAM_IMAGE, "image", "Input image (unused)" },
      { PARAM_DRAWABLE, "drawable", "Input drawable" },
      { PARAM_STRING, "gflare_name", "The name of GFlare" },
      { PARAM_INT32, "xcenter", "X coordinate of center of GFlare" },
      { PARAM_INT32, "ycenter", "Y coordinate of center of GFlare" },
      { PARAM_FLOAT, "radius",	"Radius of GFlare (pixel)" },
      { PARAM_FLOAT, "rotation", "Rotation of GFlare (degree)" },
      { PARAM_FLOAT, "hue", "Hue rotation of GFlare (degree)" },
      { PARAM_FLOAT, "vangle", "Vector angle for second flares (degree)" },
      { PARAM_FLOAT, "vlength", "Vector length for second flares (percentage to Radius)" },
      { PARAM_INT32, "use_asupsample", "Whether it uses or not adaptive supersampling while rendering (boolean)" },
      { PARAM_INT32, "asupsample_max_depth", "Max depth for adaptive supersampling"},
      { PARAM_FLOAT, "asupsample_threshold", "Threshold for adaptive supersampling"}
   };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;
  gchar	 *help_string =
   _(" This plug-in produces a lense flare effect using custom gradients."
    " In interactive call, the user can edit his/her own favorite lense flare"
    " (GFlare) and render it. Edited gflare is saved automatically to"
    " the directory in gflare-path, if it is defined in gimprc."
    " In non-interactive call, the user can only render one of GFlare"
    " which has been stored in gflare-path already.");

  INIT_I18N();

  gimp_install_procedure ("plug_in_gflare",
			  _("Produce lense flare effect using custom gradients"),
			  help_string,
			  _("Eiichi Takamori"),
			  _("Eiichi Takamori, and a lot of GIMP people"),
			  "1997",
			  N_("<Image>/Filters/Light Effects/GFlare..."),
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

void
plugin_run (gchar   *name,
     gint    nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam  **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  /* Initialize */
  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  /*
   *	Get the specified drawable and its info (global variable)
   */

  drawable = gimp_drawable_get (param[2].data.d_drawable);
  dinfo.is_color  = gimp_drawable_is_rgb (drawable->id);
  dinfo.has_alpha = gimp_drawable_has_alpha (drawable->id);
  gimp_drawable_mask_bounds (drawable->id, &dinfo.x1, &dinfo.y1,
			     &dinfo.x2, &dinfo.y2);
  dinfo.tile_width = gimp_tile_width ();
  dinfo.tile_height = gimp_tile_height ();

  /*
   *	Start gradient caching
   */

  gradient_init ();

  /*
   *	Parse gflare path from gimprc and load gflares
   */

  plug_in_parse_gflare_path ();
  gflares_list_load_all ();

  gimp_tile_cache_ntiles ( drawable->width / gimp_tile_width () + 2);


  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();

      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_gflare", &pvals);

      /*  First acquire information with a dialog  */
      if (! dlg_run ())
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
#if 0
      printf("Currently non interactive call of gradient flare is not supported\n");
      status = STATUS_CALLING_ERROR;
      break;
#endif
      if (nparams != 14)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  gflare_name_copy (pvals.gflare_name, param[3].data.d_string);
	  pvals.xcenter		     = param[4].data.d_int32;
	  pvals.ycenter		     = param[5].data.d_int32;
	  pvals.radius		     = param[6].data.d_float;
	  pvals.rotation	     = param[7].data.d_float;
	  pvals.hue		     = param[8].data.d_float;
	  pvals.vangle		     = param[9].data.d_float;
	  pvals.vlength		     = param[10].data.d_float;
	  pvals.use_asupsample	     = param[11].data.d_int32;
	  pvals.asupsample_max_depth = param[12].data.d_int32;
	  pvals.asupsample_threshold = param[13].data.d_float;
	}
      if (pvals.radius <= 0)
	status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_gflare", &pvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) || gimp_drawable_is_gray (drawable->id))
	{
	  gimp_progress_init (_("Gradient Flare..."));
	  plugin_do ();

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /*  Store data  */
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_gflare", &pvals, sizeof (PluginValues));
	}
      else
	{
	  /* gimp_message ("gflare: cannot operate on indexed color images"); */
	  status = STATUS_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;

  /*
   *	Deinitialization
   */
  gradient_free ();
  gimp_drawable_detach (drawable);
}

/*
 *	Query gimprc for gflare-path, and parse it.
 *	This code is based on script_fu_find_scripts ()
 */
void
plug_in_parse_gflare_path ()
{
  GParam *return_vals;
  gint nreturn_vals;
  gchar *path_string;
  gchar *home;
  gchar *path;
  gchar *token;
  gchar *next_token;
  struct stat filestat;
  gint	err;

  gflare_path_list = NULL;

  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "gflare-path",
				    PARAM_END);

  if (return_vals[0].data.d_status != STATUS_SUCCESS || return_vals[1].data.d_string == NULL)
    {
      DEBUG_PRINT(("No gflare-path in gimprc: gflare_path_list is NULL\n"));
      gimp_destroy_params (return_vals, nreturn_vals);
      return;
    }

  path_string = g_strdup (return_vals[1].data.d_string);
  gimp_destroy_params (return_vals, nreturn_vals);

  /* Set local path to contain temp_path, where (supposedly)
   * there may be working files.
   */
  home = getenv ("HOME");

  /* Search through all directories in the  path */

  next_token = path_string;
  token = strtok (next_token, G_SEARCHPATH_SEPARATOR_S );

  while (token)
    {
      if (*token == '\0')
	{
	  token = strtok (NULL, G_SEARCHPATH_SEPARATOR_S);
	  continue;
	}

      if (*token == '~')
	{
	  path = g_malloc (strlen (home) + strlen (token) + 2);
	  sprintf (path, "%s%s", home, token + 1);
	}
      else
	{
	  path = g_malloc (strlen (token) + 2);
	  strcpy (path, token);
	} /* else */

      /* Check if directory exists */
      err = stat (path, &filestat);

      if (!err && S_ISDIR (filestat.st_mode))
	{
	  if (path[strlen (path) - 1] != '/')
	    strcat (path, "/");

	  DEBUG_PRINT(("Added `%s' to gflare_path_list\n", path));
	  gflare_path_list = g_list_append (gflare_path_list, path);
	}
      else
	{
	  DEBUG_PRINT(("Not found `%s'\n", path));
	  g_free (path);
	}
      token = strtok (NULL, G_SEARCHPATH_SEPARATOR_S);
    }
  g_free (path_string);

}


static void
plugin_do ()
{
  GFlare	*gflare;

  gflare = gflares_list_lookup (pvals.gflare_name);
  if (gflare == NULL)
    {
      /* FIXME */
      g_warning ("Not found %s\n", pvals.gflare_name);
      return;
    }

  /* Initialize calc params and gradients */
  calc_init_params (gflare, CALC_GLOW | CALC_RAYS | CALC_SFLARE,
		    pvals.xcenter, pvals.ycenter,
		    pvals.radius, pvals.rotation, pvals.hue,
		    pvals.vangle, pvals.vlength);
  while (calc_init_progress ()) ;

  /* Render it ! */
  if (pvals.use_asupsample)
    plugin_do_asupsample ();
  else
    plugin_do_non_asupsample ();

  /* Clean up */
  calc_deinit ();
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, dinfo.x1, dinfo.y1,
			(dinfo.x2 - dinfo.x1), (dinfo.y2 - dinfo.y1));
}

/* these routines should be almost rewritten anyway */

static void
plugin_do_non_asupsample ()
{
  GPixelRgn	src_rgn, dest_rgn;
  gpointer	pr;
  guchar	*src_row, *dest_row;
  guchar	*src, *dest;
  gint		row, col;
  gint		x, y;
  gint		b;
  gint		progress, max_progress;
  guchar	src_pix[4], dest_pix[4];

  progress = 0;
  max_progress = (dinfo.x2 - dinfo.x1) * (dinfo.y2 - dinfo.y1);

  gimp_pixel_rgn_init (&src_rgn, drawable, dinfo.x1, dinfo.y1, dinfo.x2, dinfo.y2, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, dinfo.x1, dinfo.y1, dinfo.x2, dinfo.y2, TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
       pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      src_row  = src_rgn.data;
      dest_row = dest_rgn.data;

      for ( row = 0, y = src_rgn.y; row < src_rgn.h; row++, y++)
	{
	  src = src_row;
	  dest = dest_row;

	  for ( col = 0, x = src_rgn.x; col < src_rgn.w; col++, x++)
	    {
	      for (b = 0; b < 3; b++)
		src_pix[b] = dinfo.is_color ? src[b] : src[0];
	      src_pix[3] = dinfo.has_alpha ? src[src_rgn.bpp - 1] : OPAQUE;

	      calc_gflare_pix (dest_pix, x, y, src_pix);

	      if (dinfo.is_color)
		for (b = 0; b < 3; b++)
		  dest[b] = dest_pix[b];
	      else
		dest[0] = LUMINOSITY (dest_pix);

	      if (dinfo.has_alpha)
		dest[src_rgn.bpp - 1] = dest_pix[3];

	      src  += src_rgn.bpp;
	      dest += dest_rgn.bpp;
	    }
	  src_row  += src_rgn.rowstride;
	  dest_row += dest_rgn.rowstride;
	}
      /* Update progress */
      progress += src_rgn.w * src_rgn.h;
      gimp_progress_update ((double) progress / (double) max_progress);
    }
}

static void
plugin_do_asupsample ()
{
  tk_read  = tile_keeper_new (FALSE);
  tk_write = tile_keeper_new (TRUE);

  adaptive_supersample_area (dinfo.x1, dinfo.y1, dinfo.x2 - 1, dinfo.y2 - 1,
			     pvals.asupsample_max_depth,
			     pvals.asupsample_threshold,
			     (render_func_t) plugin_render_func,
			     NULL,
			     (put_pixel_func_t) plugin_put_pixel_func,
			     NULL,
			     (progress_func_t) plugin_progress_func,
			     NULL);
  tile_keeper_free (tk_read);
  tile_keeper_free (tk_write);
}

/*
  Adaptive supersampling callback functions

  These routines may look messy, since adaptive supersampling needs
  pixel values in `double' (from 0.0 to 1.0) but calc_*_pix () returns
  guchar values. */

static void
plugin_render_func (double x, double y, color_t *color, gpointer data)
{
  guchar	src_pix[4];
  guchar	flare_pix[4];
  guchar	*src;
  gint		b;
  gint		ix, iy;

  /* translate (0.5, 0.5) before convert to `int' so that it can surely
     point the center of pixel */
  ix = floor (x + 0.5);
  iy = floor (y + 0.5);

  src = tile_keeper_provide (tk_read, ix, iy, FALSE);

  for (b = 0; b < 3; b++)
    src_pix[b] = dinfo.is_color ? src[b] : src[0];
  src_pix[3] = dinfo.has_alpha ? src[drawable->bpp - 1] : OPAQUE;

  calc_gflare_pix (flare_pix, x, y, src_pix);

  color->r = flare_pix[0] / 255.0;
  color->g = flare_pix[1] / 255.0;
  color->b = flare_pix[2] / 255.0;
  color->a = flare_pix[3] / 255.0;
}

static void
plugin_put_pixel_func (int ix, int iy, color_t color, gpointer data)
{
  guchar	*dest;

  dest = tile_keeper_provide (tk_write, ix, iy, TRUE);

  if (dinfo.is_color)
    {
      dest[0] = color.r * 255;
      dest[1] = color.g * 255;
      dest[2] = color.b * 255;
    }
  else
    dest[0] = (color.r * 0.30 + color.g * 0.59 + color.b * 0.11) * 255;

  if (dinfo.has_alpha)
    dest[drawable->bpp - 1] = color.a * 255;
}

static void
plugin_progress_func (int y1, int y2, int curr_y, gpointer data)
{
  gimp_progress_update ((double) curr_y / (double) (y2 - y1));
}

/**
***	The Tile Keeper
**/

static TileKeeper *
tile_keeper_new (gint shadow)
{
  TileKeeper	*tk;

  tk = g_new0 (TileKeeper, 1);
  tk->tile	  = NULL;
  tk->col	  = 0;
  tk->row	  = 0;
  tk->shadow	  = shadow;
  tk->dirty	  = FALSE;

  return tk;
}

/*
  Return the pointer to specified pixel in allocated tile
  */
static guchar *
tile_keeper_provide (TileKeeper *tk, gint ix, gint iy, gint dirty)
{
  static guchar black[4];
  gint		col, row, offx, offy;

  if (ix < 0 || ix >= drawable->width || iy < 0 || iy >= drawable->height)
    {
      black[0] = black[1] = black[2] = black[3] = 0;
      return black;
    }

  col  = ix / dinfo.tile_width;
  row  = iy / dinfo.tile_height;
  offx = ix % dinfo.tile_width;
  offy = iy % dinfo.tile_height;

  if (tk->tile == NULL || col != tk->col || row != tk->row)
    {
      if (tk->tile)
	gimp_tile_unref (tk->tile, tk->dirty);
      tk->col	= col;
      tk->row	= row;
      tk->tile	= gimp_drawable_get_tile (drawable, tk->shadow, row, col);
      tk->dirty = FALSE;
      gimp_tile_ref (tk->tile);
    }

  tk->dirty |= dirty;
  return tk->tile->data + (offy * tk->tile->ewidth + offx) * tk->tile->bpp;
}

static void
tile_keeper_free (TileKeeper *tk)
{
  if (tk->tile)
    gimp_tile_unref (tk->tile, tk->dirty);
  g_free (tk);
}


/*************************************************************************/
/**									**/
/**		+++ GFlare Routines					**/
/**									**/
/*************************************************************************/

/*
 *	These code are more or less based on Quartic's gradient.c,
 *	other gimp sources, and script-fu.
 */

static GFlare *
gflare_new ()
{
  GFlare	*gflare;

  gflare = g_new0 (GFlare, 1);
  gflare->name = NULL;
  gflare->filename = NULL;
  return gflare;
}

GFlare *
gflare_new_with_default (gchar *new_name)
{
  DEBUG_PRINT (("gflare_new_with_default %s\n", new_name));

  return gflare_dup (&default_gflare, new_name);
}

GFlare *
gflare_dup (GFlare *src, gchar *new_name)
{
  GFlare	*dest;

  DEBUG_PRINT (("gflare_dup %s\n", new_name));

  dest = g_new0 (GFlare, 1);

  memcpy (dest, src, sizeof(GFlare));

  dest->name = g_strdup (new_name);
  dest->filename = NULL;

  return dest;
}

void
gflare_copy (GFlare *dest, GFlare *src)
{
  gchar *name, *filename;

  DEBUG_PRINT (("gflare_copy\n"));

  name = dest->name;
  filename = dest->filename;

  memcpy (dest, src, sizeof (GFlare));

  dest->name = name;
  dest->filename =filename;
}


static void
gflare_free (GFlare *gflare)
{
  g_assert (gflare != NULL);

  if (gflare->name)
    g_free (gflare->name);
  if (gflare->filename)
    g_free (gflare->filename);
  g_free (gflare);
}

GFlare *
gflare_load (char *filename, char *name)
{
  FILE		*fp;
  GFlareFile	*gf;
  GFlare	*gflare;
  gchar		header[256];

  DEBUG_PRINT (("gflare_load: %s, %s\n", filename, name));
  g_assert (filename != NULL);

  fp = fopen (filename, "r");
  if (!fp)
    {
      g_warning ("not found: %s", filename);
      return NULL;
    }

  if (fgets (header, sizeof(header), fp) == NULL
      || strcmp (header, GFLARE_FILE_HEADER) != 0)
    {
      g_warning (_("not valid GFlare file: %s"), filename);
      fclose (fp);
      return NULL;
    }

  gf = g_new (GFlareFile, 1);
  gf->fp = fp;
  gf->error = FALSE;

  gflare = gflare_new ();
  gflare->name = g_strdup (name);
  gflare->filename = g_strdup (filename);

  gflare_read_double   (&gflare->glow_opacity, gf);
  gflare_read_mode     (&gflare->glow_mode, gf);
  gflare_read_double   (&gflare->rays_opacity, gf);
  gflare_read_mode     (&gflare->rays_mode, gf);
  gflare_read_double   (&gflare->sflare_opacity, gf);
  gflare_read_mode     (&gflare->sflare_mode, gf);

  gflare_read_gradient_name (gflare->glow_radial, gf);
  gflare_read_gradient_name (gflare->glow_angular, gf);
  gflare_read_gradient_name (gflare->glow_angular_size, gf);
  gflare_read_double   (&gflare->glow_size, gf);
  gflare_read_double   (&gflare->glow_rotation, gf);
  gflare_read_double   (&gflare->glow_hue, gf);

  gflare_read_gradient_name (gflare->rays_radial, gf);
  gflare_read_gradient_name (gflare->rays_angular, gf);
  gflare_read_gradient_name (gflare->rays_angular_size, gf);
  gflare_read_double   (&gflare->rays_size, gf);
  gflare_read_double   (&gflare->rays_rotation, gf);
  gflare_read_double   (&gflare->rays_hue, gf);
  gflare_read_int      (&gflare->rays_nspikes, gf);
  gflare_read_double   (&gflare->rays_thickness, gf);

  gflare_read_gradient_name (gflare->sflare_radial, gf);
  gflare_read_gradient_name (gflare->sflare_sizefac, gf);
  gflare_read_gradient_name (gflare->sflare_probability, gf);
  gflare_read_double   (&gflare->sflare_size, gf);
  gflare_read_double   (&gflare->sflare_hue, gf);
  gflare_read_double   (&gflare->sflare_rotation, gf);
  gflare_read_shape    (&gflare->sflare_shape, gf);
  gflare_read_int      (&gflare->sflare_nverts, gf);
  gflare_read_int      (&gflare->sflare_seed, gf);

  fclose (gf->fp);

  if (gf->error)
    {
      g_warning (_("invalid formatted GFlare file: %s\n)"), filename);
      g_free (gflare);
      g_free (gf);
      return NULL;
    }

  DEBUG_PRINT (("Loaded %s\n", filename));
  g_free (gf);

  return gflare;
}

static void
gflare_read_int (gint *intvar, GFlareFile *gf)
{
  if (gf->error)
    return;

  if (fscanf (gf->fp, "%d", intvar) != 1)
    gf->error = TRUE;
}

static void
gflare_read_double (gdouble *dblvar, GFlareFile *gf)
{
  if (gf->error)
    return;

  if (fscanf (gf->fp, "%le", dblvar) != 1)
    gf->error = TRUE;
}

static void
gflare_read_gradient_name (GradientName name, GFlareFile *gf)
{
  gchar		tmp[1024], dec[1024];

  if (gf->error)
    return;

  /* FIXME: this is buggy */

  if (fscanf (gf->fp, "%s", tmp) == 1)
    {
      /* @GRADIENT_NAME */
      gradient_name_decode ((guchar*) dec, (guchar*) tmp);
      gradient_name_copy (name, tmp);
      DEBUG_PRINT (("read_gradient_name: \"%s\" => \"%s\"\n", tmp, dec));
    }
  else
    gf->error = TRUE;
}

static void
gflare_read_shape (GFlareShape *shape, GFlareFile *gf)
{
  gchar tmp[1024];
  gint	i;

  if (gf->error)
    return;

  if (fscanf (gf->fp, "%s", tmp) == 1)
    {
      for (i = 0; i < GF_NUM_SHAPES; i++)
	if (strcmp (tmp, gflare_shapes[i]) == 0)
	  {
	    *shape = i;
	    return;
	  }
    }
  gf->error = TRUE;
}

static void
gflare_read_mode (GFlareMode *mode, GFlareFile *gf)
{
  gchar tmp[1024];
  gint	i;

  if (gf->error)
    return;

  if (fscanf (gf->fp, "%s", tmp) == 1)
    {
      for (i = 0; i < GF_NUM_MODES; i++)
	if (strcmp (tmp, gflare_modes[i]) == 0)
	  {
	    *mode = i;
	    return;
	  }
    }
  gf->error = TRUE;
}

void
gflare_save (GFlare *gflare)
{
  FILE	*fp;
  gchar *path;
  static int  message_ok = FALSE;
  char *message =
    _("GFlare `%s' is not saved.	If you add a new entry in gimprc, like:\n"
    "(gflare-path \"${gimp_dir}/gflare\")\n"
    "and make a directory ~/.gimp-1.1/gflare, then you can save your own GFlare's\n"
    "into that directory.");

  if (gflare->filename == NULL)
    {
      if (gflare_path_list == NULL)
	{
	  if (!message_ok)
	    g_message (message,
		       gflare->name);
	  message_ok = TRUE;
	  return;
	}

      /* get first entry of path */
      path = gflare_path_list->data;

      gflare->filename = g_malloc (strlen (path) + strlen (gflare->name) + 2);
      sprintf (gflare->filename, "%s%s", path, gflare->name);
    }

  fp = fopen (gflare->filename, "w");
  if (!fp)
    {
      g_warning (_("could not open \"%s\""), gflare->filename);
      return;
    }

  fprintf (fp, "%s", GFLARE_FILE_HEADER);
  fprintf (fp, "%f %s\n", gflare->glow_opacity, gflare_modes[gflare->glow_mode]);
  fprintf (fp, "%f %s\n", gflare->rays_opacity, gflare_modes[gflare->rays_mode]);
  fprintf (fp, "%f %s\n", gflare->sflare_opacity, gflare_modes[gflare->sflare_mode]);

  gflare_write_gradient_name (gflare->glow_radial, fp);
  gflare_write_gradient_name (gflare->glow_angular, fp);
  gflare_write_gradient_name (gflare->glow_angular_size, fp);
  fprintf (fp, "%f %f %f\n", gflare->glow_size, gflare->glow_rotation, gflare->glow_hue);

  gflare_write_gradient_name (gflare->rays_radial, fp);
  gflare_write_gradient_name (gflare->rays_angular, fp);
  gflare_write_gradient_name (gflare->rays_angular_size, fp);
  fprintf (fp, "%f %f %f\n", gflare->rays_size, gflare->rays_rotation, gflare->rays_hue);
  fprintf (fp, "%d %f\n", gflare->rays_nspikes, gflare->rays_thickness);

  gflare_write_gradient_name (gflare->sflare_radial, fp);
  gflare_write_gradient_name (gflare->sflare_sizefac, fp);
  gflare_write_gradient_name (gflare->sflare_probability, fp);
  fprintf (fp, "%f %f %f\n", gflare->sflare_size, gflare->sflare_rotation, gflare->sflare_hue);
  fprintf (fp, "%s %d %d\n", gflare_shapes[gflare->sflare_shape], gflare->sflare_nverts, gflare->sflare_seed);

  fclose (fp);
  DEBUG_PRINT (("Saved %s\n", gflare->filename));
}

static void
gflare_write_gradient_name (GradientName name, FILE *fp)
{
  gchar		enc[1024];

  /* @GRADIENT_NAME */

  /* encode white spaces and control characters (if any) */
  gradient_name_encode ((guchar*) enc, (guchar*) name);

  fprintf (fp, "%s\n", enc);
  DEBUG_PRINT (("write_gradient_name: \"%s\" => \"%s\"\n", name, enc));
}

void
gflare_name_copy (gchar *dest, gchar *src)
{
  strncpy (dest, src, GFLARE_NAME_MAX);
  dest[GFLARE_NAME_MAX-1] = '\0';
}


/*************************************************************************/
/**									**/
/**		+++ GFlares List					**/
/**									**/
/*************************************************************************/

gint
gflares_list_insert (GFlare *gflare)
{
  GList		*tmp;
  GFlare	*g;
  int		n;

  /*
   *	Insert gflare in alphabetical order
   */

  n = 0;
  tmp = gflares_list;

  while (tmp) {
    g = tmp->data;

    if (strcmp (gflare->name, g->name) <= 0)
      break;
    n++;
    tmp = tmp->next;
  }

  num_gflares++;
  gflares_list = g_list_insert (gflares_list, gflare, n);

  DEBUG_PRINT (("gflares_list_insert %s => %d\n", gflare->name, n));

  return n;
}

GFlare *
gflares_list_lookup (gchar *name)
{
  GList		*tmp;
  GFlare	*gflare;

  DEBUG_PRINT (("gflares_list_lookup %s\n", name));

  tmp = gflares_list;
  while (tmp)
    {
      gflare = tmp->data;
      tmp = tmp->next;
      if (strcmp (gflare->name, name) == 0)
	return gflare;
    }
  return NULL;
}

gint
gflares_list_index (GFlare *gflare)
{
  GList		*tmp;
  gint		n;

  DEBUG_PRINT (("gflares_list_index %s\n", gflare->name));

  n = 0;
  tmp = gflares_list;
  while (tmp)
    {
      if (tmp->data == gflare)
	return n;
      tmp = tmp->next;
      n++;
    }
  return -1;
}

gint
gflares_list_remove (GFlare *gflare)
{
  GList		*tmp;
  gint		n;

  DEBUG_PRINT (("gflares_list_remove %s\n", gflare->name));

  n = 0;
  tmp = gflares_list;
  while (tmp)
    {
      if (tmp->data == gflare)
	{
	  /* Found! */
	  if (tmp->next == NULL)
	  num_gflares--;
	  gflares_list = g_list_remove (gflares_list, gflare);
	  return n;
	}
      tmp = tmp->next;
      n++;
    }
  return -1;
}

/*
  Load all gflares, which are founded in gflare-path-list, into gflares_list.

  gflares-path-list must be initialized first. (plug_in_parse_gflare_path ())
 */
void
gflares_list_load_all ()
{
  GFlare	*gflare;
  GList		*list;
  gchar		*path;
  gchar		*filename;
  DIR		*dir;
  struct dirent *dir_ent;
  struct stat	filestat;
  gint		err;

#if 0	/* @@@ */
  printf("Waiting... (pid %d)\n", getpid());
  kill(getpid(), 19); /* SIGSTOP */
#endif

  /*  Make sure to clear any existing gflares  */
  gflares_list_free_all ();

  list = gflare_path_list;
  while (list)
    {
      path = list->data;
      list = list->next;

      /* Open directory */
      dir = opendir (path);

      if (!dir)
	g_warning(_("error reading GFlare directory \"%s\""), path);
      else
	{
	  while ((dir_ent = readdir (dir)))
	    {
	      filename = g_malloc (strlen(path) + strlen (dir_ent->d_name) + 1);

	      sprintf (filename, "%s%s", path, dir_ent->d_name);

	      /* Check the file and see that it is not a sub-directory */
	      err = stat (filename, &filestat);

	      if (!err && S_ISREG (filestat.st_mode))
		{
		  gflare = gflare_load (filename, dir_ent->d_name);
		  if (gflare)
		    gflares_list_insert (gflare);
		}

	      g_free (filename);
	    } /* while */

	  closedir (dir);
	} /* else */
    }
}

void
gflares_list_free_all ()
{
  GList *list;
  GFlare *gflare;

  list = gflares_list;
  while (list)
    {
      gflare = (GFlare *) list->data;
      gflare_free (gflare);
      list = list->next;
    }

  g_list_free (gflares_list);
  gflares_list = NULL;
}



/*************************************************************************/
/**									**/
/**		+++ Calculator						**/
/**									**/
/*************************************************************************/


/*
 * These routines calculates pixel values of particular gflare, at
 * specified point.  The client which wants to get benefit from these
 * calculation routines must call calc_init_params() first, and
 * iterate calling calc_init_progress() until it returns FALSE. and
 * must call calc_deinit() when job is done.
 */

void
calc_init_params (GFlare *gflare, gint calc_type,
		  gdouble xcenter, gdouble ycenter,
		  gdouble radius, gdouble rotation, gdouble hue,
		  gdouble vangle, gdouble vlength)
{
  DEBUG_PRINT (("//// calc_init_params ////\n"));
  calc.type	   = calc_type;
  calc.gflare	   = gflare;
  calc.xcenter	   = xcenter;
  calc.ycenter	   = ycenter;
  calc.radius	   = radius;
  calc.rotation	   = rotation * M_PI / 180.0;
  calc.hue	   = hue;
  calc.vangle	   = vangle * M_PI / 180.0;
  calc.vlength	   = radius * vlength / 100.0;
  calc.glow_radius   = radius * gflare->glow_size / 100.0;
  calc.rays_radius   = radius * gflare->rays_size / 100.0;
  calc.sflare_radius = radius * gflare->sflare_size / 100.0;
  calc.glow_rotation = (rotation + gflare->glow_rotation) * M_PI / 180.0;
  calc.rays_rotation = (rotation + gflare->rays_rotation) * M_PI / 180.0;
  calc.sflare_rotation = (rotation + gflare->sflare_rotation) * M_PI / 180.0;
  calc.glow_opacity  = gflare->glow_opacity * 255 / 100.0;
  calc.rays_opacity  = gflare->rays_opacity * 255 / 100.0;
  calc.sflare_opacity = gflare->sflare_opacity * 255 / 100.0;

  calc.glow_bounds.x0 = calc.xcenter - calc.glow_radius - 0.1;
  calc.glow_bounds.x1 = calc.xcenter + calc.glow_radius + 0.1;
  calc.glow_bounds.y0 = calc.ycenter - calc.glow_radius - 0.1;
  calc.glow_bounds.y1 = calc.ycenter + calc.glow_radius + 0.1;
  calc.rays_bounds.x0 = calc.xcenter - calc.rays_radius - 0.1;
  calc.rays_bounds.x1 = calc.xcenter + calc.rays_radius + 0.1;
  calc.rays_bounds.y0 = calc.ycenter - calc.rays_radius - 0.1;
  calc.rays_bounds.y1 = calc.ycenter + calc.rays_radius + 0.1;

  /* Thanks to Marcelo Malheiros for this algorithm */
  calc.rays_thinness = log (gflare->rays_thickness / 100.0) / log(0.8);

  calc.rays_spike_mod	= 1.0 / (2 * gflare->rays_nspikes);

  /*
    Initialize part of sflare
    The rest will be initialized in calc_sflare()
   */
  calc.sflare_list = NULL;
  calc.sflare_shape = gflare->sflare_shape;
  if (calc.sflare_shape == GF_POLYGON)
    {
      calc.sflare_angle = 2 * M_PI / (2 * gflare->sflare_nverts);
      calc.sflare_factor = 1.0 / cos (calc.sflare_angle);
    }

  calc.glow_radial	 = NULL;
  calc.glow_angular	 = NULL;
  calc.glow_angular_size = NULL;
  calc.rays_radial	 = NULL;
  calc.rays_angular	 = NULL;
  calc.rays_angular_size = NULL;
  calc.sflare_radial	 = NULL;
  calc.sflare_sizefac	 = NULL;
  calc.sflare_probability = NULL;

  calc.init = TRUE;
}

int
calc_init_progress ()
{
  if (calc_sample_one_gradient ())
    return TRUE;
  calc_place_sflare ();
  return FALSE;
}

/*
   Store samples of gradient into an array
   this routine is called during Calc initialization
   this code is very messy... :( */
static int
calc_sample_one_gradient ()
{
  static struct {
    guchar	**values;
    gint	name_offset;
    gint	hue_offset;
    gint	gray;
  } table[] = {
    { &calc.glow_radial, OFFSETOF (GFlare, glow_radial), OFFSETOF (GFlare, glow_hue), FALSE },
    { &calc.glow_angular, OFFSETOF (GFlare, glow_angular), 0, FALSE },
    { &calc.glow_angular_size, OFFSETOF (GFlare, glow_angular_size), 0, TRUE },
    { &calc.rays_radial, OFFSETOF (GFlare, rays_radial), OFFSETOF (GFlare, rays_hue), FALSE },
    { &calc.rays_angular, OFFSETOF (GFlare, rays_angular), 0, FALSE },
    { &calc.rays_angular_size, OFFSETOF (GFlare, rays_angular_size), 0, TRUE },
    { &calc.sflare_radial, OFFSETOF (GFlare, sflare_radial), OFFSETOF (GFlare, sflare_hue), FALSE },
    { &calc.sflare_sizefac, OFFSETOF (GFlare, sflare_sizefac), 0, TRUE },
    { &calc.sflare_probability, OFFSETOF (GFlare, sflare_probability), 0, TRUE },
  };
  GFlare	*gflare = calc.gflare;
  GradientName	*grad_name;
  guchar	*gradient;
  gdouble	hue_deg;
  gint		i, j, hue;

  for (i = 0; i < sizeof (table) / sizeof (table[0]); i++)
    {
      if (*(table[i].values) == NULL)
	{
	  /* @GRADIENT_NAME */
	  grad_name = (GradientName *) ((char*) gflare + table[i].name_offset);
	  gradient = *(table[i].values) = g_new (guchar, 4 * GRADIENT_RESOLUTION);
	  gradient_get_values (*grad_name, gradient, GRADIENT_RESOLUTION);

	  /*
	   * Do hue rotation, if needed
	   */

	  if (table[i].hue_offset != 0)
	    {
	      hue_deg = calc.hue + *(gdouble *) ((char*) gflare + table[i].hue_offset);
	      hue = (gint) (hue_deg / 360.0 * 256.0) % 256;
	      if (hue < 0)
		hue += 256;
	      g_assert (0 <= hue && hue < 256);

	      if (hue > 0)
		{
		  for (j = 0; j < GRADIENT_RESOLUTION; j++)
		    {
		      gint	r, g, b;

		      r = gradient[j*4];
		      g = gradient[j*4+1];
		      b = gradient[j*4+2];

		      gimp_rgb_to_hsv (&r, &g, &b);
		      r = (r + hue) % 256;
		      gimp_hsv_to_rgb (&r, &g, &b);

		      gradient[j*4] = r;
		      gradient[j*4+1] = g;
		      gradient[j*4+2] = b;
		    }
		}
	    }

	  /*
	   *	Grayfy gradient, if needed
	   */

	  if (table[i].gray)
	    {
	      for (j = 0; j < GRADIENT_RESOLUTION; j++)
		/* the first byte is enough */
		gradient[j*4] = LUMINOSITY ((gradient + j*4));

	    }

	  /* sampling of one gradient is done */
	  return TRUE;
	}
    }
  return FALSE;
}

static void
calc_place_sflare ()
{
  GFlare	*gflare;
  CalcSFlare	*sflare;
  gdouble	prob[GRADIENT_RESOLUTION];
  gdouble	sum, sum2;
  gdouble	pos;
  gdouble	rnd, sizefac;
  int		n;
  int		i;

  if ((calc.type & CALC_SFLARE) == 0)
    return;

  DEBUG_PRINT (("calc_place_sflare\n"));

  gflare = calc.gflare;

  /*
    Calc cumulative probability
    */

  sum = 0.0;
  for (i = 0; i < GRADIENT_RESOLUTION; i++)
    {
      /* probability gradient was grayfied already */
      prob[i] = calc.sflare_probability[i*4];
      sum += prob[i];
    }

  if (sum == 0.0)
    sum = 1.0;

  sum2 = 0;
  for (i = 0; i < GRADIENT_RESOLUTION; i++)
    {
      sum2 += prob[i];		/* cumulation */
      prob[i] = sum2 / sum;
    }

  if (gflare->sflare_seed == -1)
    srand (time (NULL));
  else
    srand (gflare->sflare_seed);

  for (n = 0; n < SFLARE_NUM; n++)
    {
      sflare = g_new (CalcSFlare, 1);
      rnd = (double) rand () / G_MAXRAND;
      for (i = 0; i < GRADIENT_RESOLUTION; i++)
	if (prob[i] >= rnd)
	  break;
      if (i >= GRADIENT_RESOLUTION)
	i = GRADIENT_RESOLUTION - 1;

      /* sizefac gradient was grayfied already */
      sizefac = calc.sflare_sizefac[i*4] / 255.0;
      sizefac = pow (sizefac, 5.0);

      pos = (double) (i - GRADIENT_RESOLUTION / 2) / GRADIENT_RESOLUTION;
      sflare->xcenter = calc.xcenter + cos (calc.vangle) * calc.vlength * pos;
      sflare->ycenter = calc.ycenter - sin (calc.vangle) * calc.vlength * pos;
      sflare->radius = sizefac * calc.sflare_radius; /* FIXME */
      sflare->bounds.x0 = sflare->xcenter - sflare->radius - 1;
      sflare->bounds.x1 = sflare->xcenter + sflare->radius + 1;
      sflare->bounds.y0 = sflare->ycenter - sflare->radius - 1;
      sflare->bounds.y1 = sflare->ycenter + sflare->radius + 1;
      calc.sflare_list = g_list_append (calc.sflare_list, sflare);
    }
}


void
calc_deinit ()
{
  GList		*list;

  DEBUG_PRINT (("\\\\\\\\ calc_deinit \\\\\\\\ \n"));

  if (!calc.init)
    {
      g_warning("calc_deinit: not initialized");
      return;
    }

  list = calc.sflare_list;
  while (list)
    {
      g_free (list->data);
      list = list->next;
    }
  g_list_free (calc.sflare_list);

  g_free (calc.glow_radial);
  g_free (calc.glow_angular);
  g_free (calc.glow_angular_size);
  g_free (calc.rays_radial);
  g_free (calc.rays_angular);
  g_free (calc.rays_angular_size);
  g_free (calc.sflare_radial);
  g_free (calc.sflare_sizefac);
  g_free (calc.sflare_probability);

  calc.init = FALSE;
}

/*
 *  Get sample value at specified position of a gradient
 *
 *  gradient samples are stored into array at the time of
 *  calc_sample_one_gradients (), and it is now linear interpolated.
 *
 *  INPUT:
 *	guchar	gradient[4*GRADIENT_RESOLUTION]		gradient array(RGBA)
 *	gdouble pos					position (0<=pos<=1)
 *  OUTPUT:
 *	guchar	pix[4]
 */
static void
calc_get_gradient(guchar *pix, guchar *gradient, gdouble pos)
{
  gint		ipos;
  gdouble	frac;
  gint		i;

  if (pos < 0 || pos > 1)
    {
      pix[0] = pix[1] = pix[2] = pix[3] = 0;
      return;
    }
  pos *= GRADIENT_RESOLUTION - 1.0001;
  ipos = (gint) pos;	frac = pos - ipos;
  gradient += ipos * 4;

  for (i = 0; i < 4; i++)
    {
      pix[i] = gradient[i] * (1 - frac) + gradient[i+4] * frac;
    }
}

/* I need fmod to return always positive value */
static gdouble
fmod_positive (gdouble x, gdouble m)
{
  return x - floor (x/m) * m;
}

/*
 *  Calc glow's pixel (RGBA) value
 *  INPUT:
 *	gdouble x, y			image coordinates
 *  OUTPUT:
 *	guchar	pix[4]
 */
void
calc_glow_pix (guchar *dest_pix, gdouble x, gdouble y)
{
  gdouble radius, angle;
  gdouble angular_size;
  guchar  radial_pix[4], angular_pix[4], size_pix[4];
  gint	  i;

  if ((calc.type & CALC_GLOW) == 0
      || x < calc.glow_bounds.x0 || x > calc.glow_bounds.x1
      || y < calc.glow_bounds.y0 || y > calc.glow_bounds.y1)
    {
      memset (dest_pix, 0, 4);
      return;
    }

  x -= calc.xcenter;
  y -= calc.ycenter;
  radius = sqrt (x*x + y*y) / calc.glow_radius;
  angle = (atan2 (-y, x) + calc.glow_rotation ) / (2*M_PI);
  angle = fmod_positive (angle, 1.0);

  calc_get_gradient (size_pix, calc.glow_angular_size, angle);
  /* angular_size gradient was grayfied already */
  angular_size = size_pix[0] / 255.0;
  radius /= (angular_size+0.0001);	/* in case angular_size == 0.0 */
  if( radius < 0 || radius > 1 )
    {
      memset (dest_pix, 0, 4);
      return;
    }

  calc_get_gradient (radial_pix, calc.glow_radial, radius);
  calc_get_gradient (angular_pix, calc.glow_angular, angle);

  for (i = 0; i < 4; i++)
    dest_pix[i] = radial_pix[i] * angular_pix[i] / 255;
}

/*
 *  Calc rays's pixel (RGBA) value
 *
 */
void
calc_rays_pix (guchar *dest_pix, gdouble x, gdouble y)
{
  gdouble radius, angle;
  gdouble angular_size;
  gdouble spike_frac, spike_inten, spike_angle;
  guchar  radial_pix[4], angular_pix[4], size_pix[4];
  gint	  i;

  if ((calc.type & CALC_RAYS) == 0
      || x < calc.rays_bounds.x0 || x > calc.rays_bounds.x1
      || y < calc.rays_bounds.y0 || y > calc.rays_bounds.y1)
    {
      memset (dest_pix, 0, 4);
      return;
    }

  x -= calc.xcenter;
  y -= calc.ycenter;
  radius = sqrt (x*x + y*y) / calc.rays_radius;
  angle = (atan2 (-y, x) + calc.rays_rotation ) / (2*M_PI);
  angle = fmod_positive (angle, 1.0);	/* make sure 0 <= angle < 1.0 */
  spike_frac = fmod (angle, calc.rays_spike_mod * 2);
  spike_angle = angle - spike_frac + calc.rays_spike_mod;
  spike_frac = (angle - spike_angle) / calc.rays_spike_mod;
  /* spike_frac is between -1.0 and 1.0 here (except round error...) */

  spike_inten = pow (1.0 - fabs (spike_frac), calc.rays_thinness);

  calc_get_gradient (size_pix, calc.rays_angular_size, spike_angle);
  /* angular_size gradient was grayfied already */
  angular_size = size_pix[0] / 255.0;
  radius /= (angular_size+0.0001);	/* in case angular_size == 0.0 */
  if( radius < 0 || radius > 1 )
    {
      memset (dest_pix, 0, 4);
      return;
    }

  calc_get_gradient (radial_pix, calc.rays_radial, radius);
  calc_get_gradient (angular_pix, calc.rays_angular, spike_angle);

  for (i = 0; i < 3; i++)
    dest_pix[i] =  radial_pix[i] * angular_pix[i] / 255;
  dest_pix[3] = spike_inten * radial_pix[3] * angular_pix[3] / 255;

}


/*
 *  Calc sflare's pixel (RGBA) value
 *
 *  the sflare (second flares) are needed to be rendered one each
 *  sequencially, onto the source image, such as like usual layer
 *  operations. So the function takes src_pix as argment.  glow, rays
 *  routines don't have src_pix as argment, because of convienience.
 *
 *  @JAPANESE
 *  sflare は複数のフレアを順に(レイヤ的に)かぶせながら描画する必要が
 *  あるので、これだけ src_pix を引数にとって paint_func を適用する。
 *  glow, rays は簡易化のためになし。
 */
void
calc_sflare_pix (guchar *dest_pix, gdouble x, gdouble y, guchar *src_pix)
{
  GList		*list;
  CalcSFlare	*sflare;
  gdouble	sx, sy, th;
  gdouble	radius, angle;
  guchar	radial_pix[4], tmp_pix[4];

  memcpy (dest_pix, src_pix, 4);

  if ((calc.type & CALC_SFLARE) == 0)
    return;

  list = calc.sflare_list;
  while (list)
    {
      sflare = list->data;
      list = list->next;

      if (x < sflare->bounds.x0 || x > sflare->bounds.x1
	  || y < sflare->bounds.y0 || y > sflare->bounds.y1)
	continue;
      sx = x - sflare->xcenter;
      sy = y - sflare->ycenter;
      radius = sqrt (sx * sx + sy * sy) / sflare->radius;
      if (calc.sflare_shape == GF_POLYGON)
	{
	  angle = atan2 (-sy, sx) - calc.vangle + calc.sflare_rotation;
	  th = fmod_positive (angle, calc.sflare_angle * 2) - calc.sflare_angle;
	  radius *= cos (th) * calc.sflare_factor;
	}
      if (radius < 0 || radius > 1)
	continue;

      calc_get_gradient (radial_pix, calc.sflare_radial, radius);
      memcpy (tmp_pix, dest_pix, 4);
      calc_paint_func (dest_pix, tmp_pix, radial_pix,
		       calc.sflare_opacity, calc.gflare->sflare_mode);
    }
}



void
calc_gflare_pix (guchar *dest_pix, gdouble x, gdouble y, guchar *src_pix)
{
  GFlare	*gflare = calc.gflare;
  guchar	glow_pix[4], rays_pix[4];
  guchar	tmp_pix[4];

  memcpy (dest_pix, src_pix, 4);

  if (calc.type & CALC_GLOW)
    {
      memcpy (tmp_pix, dest_pix, 4);
      calc_glow_pix (glow_pix, x, y);
      calc_paint_func (dest_pix, tmp_pix, glow_pix,
		       calc.glow_opacity, gflare->glow_mode);
    }
  if (calc.type & CALC_RAYS)
    {
      memcpy (tmp_pix, dest_pix, 4);
      calc_rays_pix (rays_pix, x, y);
      calc_paint_func (dest_pix, tmp_pix, rays_pix,
		       calc.rays_opacity, gflare->rays_mode);
    }
  if (calc.type & CALC_SFLARE)
    {
      memcpy (tmp_pix, dest_pix, 4);
      calc_sflare_pix (dest_pix, x, y, tmp_pix);
    }
}

/*
	Paint func routines, such as Normal, Addition, ...
 */
static void
calc_paint_func (guchar *dest, guchar *src1, guchar *src2, gint opacity,
		 GFlareMode mode)
{
  guchar	buf[4], *s=buf;

  if (src2[3] == 0 || opacity <= 0)
    {
      memcpy (dest, src1, 4);
      return;
    }

  switch (mode)
    {
    case GF_NORMAL:
      s = src2;
      break;
    case GF_ADDITION:
      calc_addition (s, src1, src2);
      break;
    case GF_OVERLAY:
      calc_overlay (s, src1, src2);
      break;
    case GF_SCREEN:
      calc_screen (s, src1, src2);
      break;
    default:
      s = src2;
      break;
    }
  calc_combine (dest, src1, s, opacity);
}

static void
calc_combine (guchar *dest, guchar *src1, guchar *src2, gint opacity)
{
  gdouble	s1_a, s2_a, new_a;
  gdouble	ratio, compl_ratio;
  gint		i;

  s1_a = src1[3] / 255.0;
  s2_a = src2[3] * opacity / 65025.0;
  new_a	 = s1_a + (1.0 - s1_a) * s2_a;

  if (new_a != 0.0)
    ratio = s2_a / new_a;
  else
    ratio = 0.0;

  compl_ratio = 1.0 - ratio;

  for (i = 0; i < 3; i++)
    dest[i] = src1[i] * compl_ratio + src2[i] * ratio;

  dest[3] = new_a * 255.0;
}

static void
calc_addition (guchar *dest, guchar *src1, guchar *src2)
{
  gint		tmp, i;

  for (i = 0; i < 3; i++)
    {
      tmp = src1[i] + src2[i];
      dest[i] = tmp <= 255 ? tmp: 255;
    }
  dest[3] = MIN (src1[3], src2[3]);
}

static void
calc_screen (guchar *dest, guchar *src1, guchar *src2)
{
  gint		i;

  for (i = 0; i < 3; i++)
    {
      dest[i] = 255 - ((255 - src1[i]) * (255 - src2[i])) / 255;
    }
  dest[3] = MIN (src1[3], src2[3]);
}

static void
calc_overlay (guchar *dest, guchar *src1, guchar *src2)
{
  gint		screen, mult, i;

  for (i = 0; i < 3; i++)
    {
      screen = 255 - ((255 - src1[i]) * (255 - src2[i])) / 255;
      mult = (src1[i] * src2[i]) / 255;
      dest[i] = (screen * src1[i] + mult * (255 - src1[i])) / 255;
    }
  dest[3] = MIN (src1[3], src2[3]);
}

/*************************************************************************/
/**									**/
/**			Main Dialog					**/
/**			+++ dlg						**/
/**									**/
/*************************************************************************/

/*
	This is gflare main dialog, one which opens in first.
 */

int
dlg_run ()
{
  GtkWidget	*shell;
  GtkWidget	*button;
  GtkWidget	*table;
  GtkWidget	*inner_prv_frame;
  GtkWidget     *outer_prv_frame; 
  GtkWidget	*notebook;
  guchar	*color_cube;
  gchar		**argv;
  gint		argc;

  /*
   *	Init Gdk & Gtk stuff
   */
  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("gflare");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  /* gdk_set_show_events (TRUE);
     gdk_set_debug_level (3); */

  /*
   *	Init Main Dialog
   */

#if 0		/* @@@ debug stuff */
  printf("Waiting... (pid %d)\n", getpid());
  kill(getpid(), 19); /* SIGSTOP */
#endif

  pint.run = FALSE;
  dlg = g_new (GFlareDialog, 1);
  dlg->init = TRUE;
  dlg->update_preview = TRUE;

  gradient_menu_init(); /* FIXME: this should go elsewhere  */
  dlg_setup_gflare ();

  g_assert (gflares_list != NULL);
  g_assert (dlg->gflare != NULL);
  g_assert (dlg->gflare->name != NULL);

  /*
   *	Dialog Shell
   */

  shell = dlg->shell = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (shell), _("GFlare"));
  gtk_window_set_position (GTK_WINDOW (shell), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (shell), "destroy",
		      (GtkSignalFunc) dlg_close_callback,
		      NULL);

  /*
   *	Action area
   */

  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) dlg_ok_callback,
		      shell);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  /*
   *	Preview
   */

  dlg->preview = preview_new (DLG_PREVIEW_WIDTH, DLG_PREVIEW_HEIGHT,
			      dlg_preview_init_func, NULL,
			      dlg_preview_render_func, NULL,
			      dlg_preview_deinit_func, NULL);

  gtk_widget_set_events (GTK_WIDGET (dlg->preview->widget), DLG_PREVIEW_MASK);
  gtk_signal_connect (GTK_OBJECT(dlg->preview->widget), "event",
		      (GtkSignalFunc) dlg_preview_handle_event,
		      NULL);
  dlg_preview_calc_window ();

  inner_prv_frame = gtk_frame_new (NULL); 
  gtk_frame_set_shadow_type (GTK_FRAME (inner_prv_frame), GTK_SHADOW_IN); 
  gtk_container_add (GTK_CONTAINER (inner_prv_frame), dlg->preview->widget); 
  gtk_container_set_border_width (GTK_CONTAINER (inner_prv_frame), 4); 
  gtk_widget_show (inner_prv_frame); 
 
  outer_prv_frame = gtk_frame_new (_("Preview")); 
  gtk_frame_set_shadow_type (GTK_FRAME (outer_prv_frame), GTK_SHADOW_ETCHED_IN); 
  gtk_container_add (GTK_CONTAINER (outer_prv_frame), inner_prv_frame); 
  gtk_widget_show (outer_prv_frame); 

  /*
   *	Notebook
   */

  notebook = dlg->notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);

  dlg_make_page_settings (dlg, notebook);
  dlg_make_page_selector (dlg, notebook);
  gtk_widget_show (notebook);

  /*
   *	pack them
   */

  table = gtk_table_new (1, 2, TRUE); 
  gtk_container_set_border_width (GTK_CONTAINER (table), 8); 
  gtk_table_attach (GTK_TABLE (table), outer_prv_frame, 0, 1, 0, 1, 
                   GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0); 
  gtk_table_attach (GTK_TABLE (table), notebook, 1, 2, 0, 1, 
                   GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0); 
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 8); 
  gtk_widget_show (table); 

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->vbox), table, TRUE, TRUE, 0);
  DEBUG_PRINT (("shell is shown\n"));

  gtk_widget_show (shell);

  /*
   *	Make sure the selector page is realized
   *	This idea is from app/layers_dialog.c
   */
  DEBUG_PRINT (("pages are shown\n"));

  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 1);
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 0);


  /*
   *	Initialization done
   */
  dlg->init = FALSE;
  dlg_preview_update ();

  DEBUG_PRINT (("dlg init done\n"));

  gtk_main ();
  gdk_flush ();

  return pint.run;
}

static void
dlg_ok_callback (GtkWidget *widget,
		 gpointer   data)
{
  /* @GFLARE_NAME */
  gflare_name_copy(pvals.gflare_name, dlg->gflare->name);

  pint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}


static void
dlg_close_callback (GtkWidget *widget,
		    gpointer   data)
{
  gtk_main_quit ();
}

static void
dlg_setup_gflare ()
{
  dlg->gflare = gflares_list_lookup (pvals.gflare_name);

  if (dlg->gflare == NULL)
    {
      dlg->gflare = gflares_list_lookup ("Default");
      if (dlg->gflare == NULL)
	{
	  g_warning (_("`Default' is created."));
	  dlg->gflare = gflare_new_with_default (_("Default"));
	  gflares_list_insert (dlg->gflare);
	}
    }
}

static void
dlg_page_map_callback (GtkWidget *widget, gpointer data)
{
  /* dlg_preview_update (); */
}

/***********************************/
/**	Main Dialog / Preview	  **/
/***********************************/

/*
 *	Calculate preview's window, ie. translation of preview widget and
 *	drawable.
 *
 *	x0, x1, y0, y1 are drawable coord, corresponding with top left
 *	corner of preview widget, etc.
 */
void
dlg_preview_calc_window ()
{
  int			is_wide;
  gdouble		offx, offy;

  is_wide = ((double) DLG_PREVIEW_HEIGHT * drawable->width
	     >= (double) DLG_PREVIEW_WIDTH * drawable->height);
  if (is_wide)
    {
      offy = ((double) drawable->width * DLG_PREVIEW_HEIGHT / DLG_PREVIEW_WIDTH) / 2.0;

      dlg->pwin.x0 = 0;
      dlg->pwin.x1 = drawable->width;
      dlg->pwin.y0 = drawable->height / 2.0 - offy;
      dlg->pwin.y1 = drawable->height / 2.0 + offy;
    }
  else
    {
      offx = ((double) drawable->height * DLG_PREVIEW_WIDTH / DLG_PREVIEW_HEIGHT) / 2.0;

      dlg->pwin.x0 = drawable->width / 2.0 - offx;
      dlg->pwin.x1 = drawable->width / 2.0 + offx;
      dlg->pwin.y0 = 0;
      dlg->pwin.y1 = drawable->height;
    }
}

void
ed_preview_calc_window ()
{
  int			is_wide;
  gdouble		offx, offy;

  is_wide = ((double) DLG_PREVIEW_HEIGHT * drawable->width
	     >= (double) DLG_PREVIEW_WIDTH * drawable->height);
  if (is_wide)
    {
      offy = ((double) drawable->width * DLG_PREVIEW_HEIGHT / DLG_PREVIEW_WIDTH) / 2.0;

      dlg->pwin.x0 = 0;
      dlg->pwin.x1 = drawable->width;
      dlg->pwin.y0 = drawable->height / 2.0 - offy;
      dlg->pwin.y1 = drawable->height / 2.0 + offy;
    }
  else
    {
      offx = ((double) drawable->height * DLG_PREVIEW_WIDTH / DLG_PREVIEW_HEIGHT) / 2.0;

      dlg->pwin.x0 = drawable->width / 2.0 - offx;
      dlg->pwin.x1 = drawable->width / 2.0 + offx;
      dlg->pwin.y0 = 0;
      dlg->pwin.y1 = drawable->height;
    }
}

gint
dlg_preview_handle_event (GtkWidget *widget, GdkEvent *event)
{
  GdkEventButton *bevent;
  gint		 bx, by, x, y;
  gchar		 buf[256];

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      bx = bevent->x;
      by = bevent->y;

      /* convert widget coord to drawable coord */
      x = dlg->pwin.x0 + (double) (dlg->pwin.x1 - dlg->pwin.x0)
					* bx / DLG_PREVIEW_WIDTH;
      y = dlg->pwin.y0 + (double) (dlg->pwin.y1 - dlg->pwin.y0)
					* by / DLG_PREVIEW_HEIGHT;
      DEBUG_PRINT (("dlg_preview_handle_event: bxy [%d,%d] xy [%d,%d]\n",
		    bx, by, x, y));

      if ((x != pvals.xcenter || y != pvals.ycenter))
	{
	  if (x != pvals.xcenter)
	    {
	      pvals.xcenter = x;
	      gtk_signal_handler_block (GTK_OBJECT (dlg->xentry), dlg->xentry_id);
	      sprintf (buf, "%d", x);
	      gtk_entry_set_text (GTK_ENTRY (dlg->xentry), buf);
	      gtk_signal_handler_unblock (GTK_OBJECT (dlg->xentry), dlg->xentry_id);
	    }
	  if (y != pvals.ycenter)
	    {
	      pvals.ycenter = y;
	      gtk_signal_handler_block (GTK_OBJECT (dlg->yentry), dlg->yentry_id);
	      sprintf (buf, "%d", y);
	      gtk_entry_set_text (GTK_ENTRY (dlg->yentry), buf);
	      gtk_signal_handler_unblock (GTK_OBJECT (dlg->yentry), dlg->yentry_id);
	    }
	  dlg_preview_update ();
	}
      return TRUE;
    default:
      break;
    }
  return FALSE;
}

gint
ed_preview_handle_event (GtkWidget *widget, GdkEvent *event)
{
  GdkEventButton *bevent;
  gint		 bx, by, x, y;
  gchar		 buf[256];

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      bx = bevent->x;
      by = bevent->y;

      /* convert widget coord to drawable coord */
      x = dlg->pwin.x0 + (double) (dlg->pwin.x1 - dlg->pwin.x0)
					* bx / DLG_PREVIEW_WIDTH;
      y = dlg->pwin.y0 + (double) (dlg->pwin.y1 - dlg->pwin.y0)
					* by / DLG_PREVIEW_HEIGHT;
      DEBUG_PRINT (("dlg_preview_handle_event: bxy [%d,%d] xy [%d,%d]\n",
		    bx, by, x, y));

      if ((x != pvals.xcenter || y != pvals.ycenter))
	{
	  if (x != pvals.xcenter)
	    {
	      pvals.xcenter = x;
	      gtk_signal_handler_block (GTK_OBJECT (dlg->xentry), dlg->xentry_id);
	      sprintf (buf, "%d", x);
	      gtk_entry_set_text (GTK_ENTRY (dlg->xentry), buf);
	      gtk_signal_handler_unblock (GTK_OBJECT (dlg->xentry), dlg->xentry_id);
	    }
	  if (y != pvals.ycenter)
	    {
	      pvals.ycenter = y;
	      gtk_signal_handler_block (GTK_OBJECT (dlg->yentry), dlg->yentry_id);
	      sprintf (buf, "%d", y);
	      gtk_entry_set_text (GTK_ENTRY (dlg->yentry), buf);
	      gtk_signal_handler_unblock (GTK_OBJECT (dlg->yentry), dlg->yentry_id);
	    }
	  dlg_preview_update ();
	}
      return TRUE;
    default:
      break;
    }
  return FALSE;
}

static void
dlg_preview_update ()
{
  if (dlg->init)
    return;

  if (dlg->update_preview)
    {
      dlg->init_params_done = FALSE;
      preview_render_start (dlg->preview);
    }
}

/*	preview callbacks	*/
static gint
dlg_preview_init_func (Preview *preview, gpointer data)
{
  /* call init_params first, and iterate init_progress while
     it returns true */
  if (dlg->init_params_done == FALSE)
    {
      calc_init_params (dlg->gflare,
			CALC_GLOW | CALC_RAYS | CALC_SFLARE,
			pvals.xcenter, pvals.ycenter,
			pvals.radius, pvals.rotation, pvals.hue,
			pvals.vangle, pvals.vlength);
      dlg->init_params_done = TRUE;
      return TRUE;
    }
  return calc_init_progress ();
}

/* render preview
   do what "preview" means, ie. render lense flare effect onto drawable */
static void
dlg_preview_render_func (Preview *preview, guchar *dest, gint y, gpointer data)
{
  GPixelRgn	srcPR;
  gint		x;
  gint		dx, dy;		/* drawable x, y */
  guchar	*src_row, *src;
  guchar	src_pix[4], dest_pix[4];
  gint		b;

  dy = dlg->pwin.y0 + (double) (dlg->pwin.y1 - dlg->pwin.y0) * y / DLG_PREVIEW_HEIGHT;
  if (dy < 0 || dy >= drawable->height)
    {
      memset (dest, GRAY50, 3 * DLG_PREVIEW_WIDTH);
      return;
    }

  src_row = g_new (guchar, drawable->bpp * drawable->width);
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);
  gimp_pixel_rgn_get_row (&srcPR, src_row, 0, dy, drawable->width);

  for (x = 0; x < DLG_PREVIEW_HEIGHT; x++)
    {
      dx = dlg->pwin.x0 + (double) (dlg->pwin.x1 - dlg->pwin.x0) * x / DLG_PREVIEW_WIDTH;
      if (dx < 0 || dx >= drawable->width)
	{
	  for (b = 0; b < 3; b++)
	    *dest++ = GRAY50;
	  continue;
	}

      /* Get drawable pix value */
      src = &src_row[dx * drawable->bpp];

      for (b = 0; b < 3; b++)
	src_pix[b] = dinfo.is_color ? src[b] : src[0];
      src_pix[3] = dinfo.has_alpha ? src[drawable->bpp-1] : OPAQUE;

      /* Get GFlare pix value */

      calc_gflare_pix (dest_pix, dx, dy, src_pix);

      /* Draw gray check if needed */
      preview_rgba_to_rgb (dest, x, y, dest_pix);
      dest += 3;
    }

  g_free (src_row);
}

static void
dlg_preview_deinit_func (Preview *preview, gpointer data)
{
  if (dlg->init_params_done)
    {
      calc_deinit ();
      dlg->init_params_done = TRUE;
    }
}

/*****************************************/
/**	Main Dialog / Settings Page	**/
/*****************************************/

static void
dlg_make_page_settings (GFlareDialog *dlg, GtkWidget *notebook)
{
  GtkWidget	*table;
  GtkWidget	*note_label;
  GtkWidget	*button;
  GtkWidget	*asup_frame;
  GtkWidget	*asup_table;
  GtkWidget	*label;
  GtkWidget	*scale;
  GtkObject	*scale_data;
  GtkWidget	*entry;
  gchar		buffer[256];

  table = gtk_table_new (10, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  label = gtk_label_new (_("Center X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  entry = dlg->xentry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf( buffer, "%d", pvals.xcenter );
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  dlg->xentry_id =
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
			(GtkSignalFunc) &dlg_position_entry_callback,
			&pvals.xcenter );
  gtk_table_attach (GTK_TABLE(table), entry, 1, 2, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (entry);

  label = gtk_label_new (_("Center Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE(table), label, 0, 1, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  entry = dlg->yentry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf( buffer, "%d", pvals.ycenter );
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  dlg->yentry_id =
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
			(GtkSignalFunc) &dlg_position_entry_callback,
			&pvals.ycenter );
  gtk_table_attach (GTK_TABLE(table), entry, 1, 2, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (entry);

  entscale_new (table, 0, 2, _("Radius:"),
		ENTSCALE_DOUBLE, &pvals.radius,
		0.0, drawable->width/2, 1.0, FALSE,
		&dlg_entscale_callback, (gpointer) PAGE_SETTINGS);
  entscale_new (table, 0, 3, _("Rotation:"),
		ENTSCALE_DOUBLE, &pvals.rotation,
		-180.0, 180.0, 1.0, FALSE,
		&dlg_entscale_callback, (gpointer) PAGE_SETTINGS);
  entscale_new (table, 0, 4, _("Hue Rotation:"),
		ENTSCALE_DOUBLE, &pvals.hue,
		-180.0, 180.0, 1.0, FALSE,
		&dlg_entscale_callback, (gpointer) PAGE_SETTINGS);
  entscale_new (table, 0, 5, _("Vector Angle:"),
		ENTSCALE_DOUBLE, &pvals.vangle,
		0.0, 359.0, 1.0, FALSE,
		&dlg_entscale_callback, (gpointer) PAGE_SETTINGS);
  entscale_new (table, 0, 6, _("Vector Length:"),
		ENTSCALE_DOUBLE, &pvals.vlength,
		1, 1000, 1.0, FALSE,
		&dlg_entscale_callback, (gpointer) PAGE_SETTINGS);
  /**
  ***	Asupsample settings
  ***	This code is stolen from gimp-0.99.x/app/blend.c
  **/

  button = gtk_check_button_new_with_label (_("Adaptive Supersampling"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), pvals.use_asupsample );
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) dlg_use_asupsample_callback,
		      &pvals.use_asupsample );
  gtk_table_attach (GTK_TABLE(table), button, 0, 2, 7, 8,
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (button);

  /*  asupsample frame */
  asup_frame = dlg->asupsample_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (asup_frame), GTK_SHADOW_ETCHED_IN);
  gtk_widget_set_sensitive (asup_frame, pvals.use_asupsample);

  asup_table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (asup_table), 5);
  gtk_container_add (GTK_CONTAINER (asup_frame), asup_table);

  label = gtk_label_new (_("Max depth:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (asup_table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  gtk_widget_show (label);

  scale_data = gtk_adjustment_new (pvals.asupsample_max_depth,
				   1.0, 10.0, 1.0, 1.0, 1.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) dlg_iscale_callback,
		      &pvals.asupsample_max_depth);
  gtk_table_attach (GTK_TABLE (asup_table), scale, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 4, 2);
  gtk_widget_show (scale);

  label = gtk_label_new (_("Threshold:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (asup_table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  gtk_widget_show (label);

  scale_data = gtk_adjustment_new (pvals.asupsample_threshold,
				   0.0, 4.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) dlg_dscale_callback,
		      &pvals.asupsample_threshold);
  gtk_table_attach (GTK_TABLE (asup_table), scale, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 4, 2);
  gtk_widget_show (scale);

  gtk_widget_show (asup_table);
  gtk_table_attach (GTK_TABLE(table), asup_frame, 0, 2, 8, 9,
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (asup_frame);

  button = gtk_check_button_new_with_label (_("Auto update preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), dlg->update_preview );
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) dlg_update_preview_callback,
		      &dlg->update_preview );
  gtk_table_attach (GTK_TABLE(table), button, 0, 2, 9, 10,
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (button);

  /*
   *	Create Page
   */
  note_label = gtk_label_new (_("Settings"));
  gtk_misc_set_alignment (GTK_MISC (note_label), 0.5, 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, note_label);
  gtk_signal_connect (GTK_OBJECT (table), "map",
		      (GtkSignalFunc) dlg_page_map_callback,
		      (gpointer) PAGE_SETTINGS);
  gtk_widget_show (note_label);
  gtk_widget_show (table);
}


static void
dlg_position_entry_callback (GtkWidget *widget, gpointer data)
{
  gint *var = data;
  gint new_val;

  DEBUG_PRINT (("dlg_position_entry_callback\n"));

  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  if (*var != new_val)
    {
      *var = new_val;
      dlg_preview_update ();
    }
}

static void
dlg_entscale_callback (gdouble new_val, gpointer data)
{
  DEBUG_PRINT (("dlg_entscale_callback\n"));

  dlg_preview_update ();
}


static void
dlg_use_asupsample_callback (GtkWidget *widget, gpointer data)
{
  gint			*toggle_val = (gint *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;

  if (!dlg->init)
    gtk_widget_set_sensitive (dlg->asupsample_frame, *toggle_val);
}

static void
dlg_iscale_callback (GtkAdjustment *adjustment, gpointer data)
{
  *(gint*) data = (gint) adjustment->value;

  /* dlg_preview_update (); */
}

static void
dlg_dscale_callback (GtkAdjustment *adjustment, gpointer data)
{
  *(gdouble*) data = adjustment->value;

  /* dlg_preview_update (); */
}

static void
dlg_update_preview_callback (GtkWidget *widget, gpointer data)
{
  gint	*toggle_val = (gint *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;

  dlg_preview_update ();
}

/*****************************************/
/**	Main Dialog / Selector Page	**/
/*****************************************/

static void
dlg_make_page_selector (GFlareDialog *dlg, GtkWidget *notebook)
{
  GtkWidget	*vbox;
  GtkWidget	*hbox;
  GtkWidget	*listbox;
  GtkWidget	*list;
  GtkWidget	*button;
  GtkWidget	*note_label;
  int		i;
  static struct {
    char		*label;
    GtkSignalFunc	callback;
  } buttons[] = {
    { "New",	(GtkSignalFunc) &dlg_selector_new_callback},
    { "Edit",	(GtkSignalFunc) &dlg_selector_edit_callback},
    { "Copy",	(GtkSignalFunc) &dlg_selector_copy_callback},
    { "Delete", (GtkSignalFunc) &dlg_selector_delete_callback}
  };

  DEBUG_PRINT (("dlg_make_page_selector\n"));

  vbox = gtk_vbox_new (FALSE, 0);

  /*
   *	List Box
   */

  listbox = gtk_scrolled_window_new (NULL, NULL);

  gtk_widget_set_usize (listbox, DLG_LISTBOX_WIDTH, DLG_LISTBOX_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 0);

  list = dlg->selector_list = gtk_list_new ();
  gtk_scrolled_window_add_with_viewport (
             GTK_SCROLLED_WINDOW (listbox), list);
  gtk_widget_show (listbox);
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);
  gtk_widget_show (list);

  dlg_selector_setup_listbox ();

  /*
   *	The buttons for the possible listbox operations
   */

  hbox = gtk_hbox_new (FALSE, 0);
  for (i = 0; i < sizeof (buttons) / sizeof (buttons[0]); i++)
    {
      button = gtk_button_new_with_label (gettext(buttons[i].label));
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  buttons[i].callback, button);
      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 2);
      gtk_widget_show (button);
    }
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 3);
  gtk_widget_show (hbox);

  gtk_widget_show (vbox);

  /*
   *	Create Page
   */
  note_label = gtk_label_new (_("Selector"));
  gtk_widget_show (note_label);
  gtk_misc_set_alignment (GTK_MISC (note_label), 0.5, 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, note_label);
  gtk_signal_connect (GTK_OBJECT (vbox), "map",
		      (GtkSignalFunc) dlg_page_map_callback,
		      (gpointer) PAGE_SELECTOR);
  gtk_widget_show (note_label);
  gtk_widget_show (vbox);
}


/*
 *	Set up selector's listbox, according to gflares_list
 */
static void
dlg_selector_setup_listbox ()
{
  GList		*list;
  GFlare	*gflare;
  int		n;

  DEBUG_PRINT (("dlg_selector_setup_listbox\n"));

  list = gflares_list;
  n = 0;

  while (list)
    {
      gflare = list->data;

      /*
	dlg->gflare should be valid (ie. not NULL) here.
	*/
      if (gflare == dlg->gflare)
	dlg_selector_insert (gflare, n, 1);
      else
	dlg_selector_insert (gflare, n, 0);

      list = list->next;
      n++;
    }
}

/*
 *	Insert new list_item to selector's listbox
 */
static void
dlg_selector_insert (GFlare *gflare, int pos, int select)
{
  GtkWidget	*list_item;
  GList		*list;

  DEBUG_PRINT (("dlg_selector_insert %s %d\n", gflare->name, pos));

  list_item = gtk_list_item_new_with_label (gflare->name);
  /* gflare->list_item = list_item; */
  gtk_signal_connect (GTK_OBJECT (list_item), "select",
		      (GtkSignalFunc) dlg_selector_list_item_callback,
		      (gpointer) gflare);
  gtk_widget_show (list_item);

  list = g_list_append (NULL, list_item);
  gtk_list_insert_items (GTK_LIST (dlg->selector_list), list, pos);

  if (select)
    gtk_list_select_item (GTK_LIST (dlg->selector_list), pos);
}

static void
dlg_selector_list_item_callback (GtkWidget *widget, gpointer data)
{
  if (widget->state != GTK_STATE_SELECTED)
    return;

  dlg->gflare = data;

  dlg_preview_update ();
}


/*
 *	"New" button in Selector page
 */
static void
dlg_selector_new_callback (GtkWidget *widget,
			   gpointer data)
{
  query_string_box ( _("New GFlare"),
		     _("Enter a name for the new GFlare"),
		     _("untitled"),
		     dlg_selector_new_ok_callback, dlg);
}

static void
dlg_selector_new_ok_callback (GtkWidget *widget, gpointer client_data, gpointer call_data)
{
  char		*new_name = call_data;
  GFlare	*gflare;
  gint		pos;

  g_assert (new_name != NULL);

  if (gflares_list_lookup (new_name))
    {
      /* message_box (); */
      g_warning (_("The name `%s' is used already!"), new_name);
      return;
    }

  gflare = gflare_new_with_default (new_name);

  pos = gflares_list_insert (gflare);
  dlg_selector_insert (gflare, pos, 1);

  dlg->gflare = gflare;
  dlg_preview_update ();
}

/*
 *	"Edit" button in Selector page
 */
static void
dlg_selector_edit_callback (GtkWidget *widget,
			   gpointer data)
{
  preview_render_end (dlg->preview);
  gtk_widget_set_sensitive (dlg->shell, FALSE);
  ed_run (dlg->gflare, dlg_selector_edit_done_callback, NULL);
}

static void
dlg_selector_edit_done_callback (gint updated, gpointer data)
{
  gtk_widget_set_sensitive (dlg->shell, TRUE);
  if (updated)
    {
      gflare_save (dlg->gflare);
    }
  dlg_preview_update ();
}


/*
 *	"Copy" button in Selector page
 */
static void
dlg_selector_copy_callback (GtkWidget *widget,
			    gpointer data)
{
  char *name;

  name = g_new (gchar, strlen (dlg->gflare->name) + 6);
  sprintf (name, "%s copy", dlg->gflare->name);

  query_string_box ( _("Copy GFlare"),
		     _("Enter a name for the copied GFlare"),
		     name,
		     dlg_selector_copy_ok_callback, dlg);
}


static void
dlg_selector_copy_ok_callback (GtkWidget *widget, gpointer client_data, gpointer call_data)
{
  char		*copy_name = call_data;
  GFlare	*gflare;
  gint		pos;

  g_assert (copy_name != NULL);

  if (gflares_list_lookup (copy_name))
    {
      /* message_box (); */
      g_warning (_("The name `%s' is used already!"), copy_name);
      return;
    }

  gflare = gflare_dup (dlg->gflare, copy_name);

  pos = gflares_list_insert (gflare);
  dlg_selector_insert (gflare, pos, 1);

  dlg->gflare = gflare;
  gflare_save (dlg->gflare);
  dlg_preview_update ();
}

/*
 *	"Delete" button in Selector page
 */
static void
dlg_selector_delete_callback (GtkWidget *widget,
			      gpointer data)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *button;
  gchar	    *str;

  if (num_gflares <= 1)
    {
      /* message_box () */
      g_warning (_("Cannot delete!! There must be at least one GFlare."));
      return;
    }

  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), _("Delete GFlare"));
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 0);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox,
		     FALSE, FALSE, 0);
  gtk_widget_show(vbox);

  /* Question */

  str = g_strdup_printf(_("Are you sure you want to delete "
    "\"%s\" from the list and from disk?"), dlg->gflare->name);
  label = gtk_label_new(str);
  g_free(str);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  /* Buttons */

  button = gtk_button_new_with_label (_("Delete"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) dlg_selector_delete_ok_callback,
		      (gpointer) dialog);
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT); 
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) dlg_selector_delete_cancel_callback,
		      (gpointer) dialog);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  /* Show! */

  gtk_widget_show(dialog);
  gtk_widget_set_sensitive(dlg->shell, FALSE);
}

static void
dlg_selector_delete_ok_callback(GtkWidget *widget, gpointer client_data)
{
  GFlare	*old_gflare;
  GList		*tmp;
  int		i, new_i;

  gtk_widget_destroy(GTK_WIDGET(client_data));
  gtk_widget_set_sensitive(dlg->shell, TRUE);

  i = gflares_list_index (dlg->gflare);

  if (i >= 0)
    {
      /* Remove current gflare from gflares_list and free it */
      old_gflare = dlg->gflare;
      gflares_list_remove (dlg->gflare);
      dlg->gflare = NULL;

      /* Remove from listbox */
      gtk_list_clear_items (GTK_LIST (dlg->selector_list), i, i + 1);

      /* Calculate new position of gflare and select it */
      new_i = (i < num_gflares) ? i : num_gflares - 1;
      if ((tmp = g_list_nth (gflares_list, new_i)))
	  dlg->gflare = tmp->data;
      gtk_list_select_item (GTK_LIST (dlg->selector_list), new_i);

      /* Delete old one from disk and memory */
      if (old_gflare->filename)
	unlink (old_gflare->filename);
      gflare_free (old_gflare);

      /* Update */
      dlg_preview_update ();
    }
  else
    g_warning (_("not found %s in gflares_list"), dlg->gflare->name);
}

static void
dlg_selector_delete_cancel_callback (GtkWidget *widget, gpointer client_data)
{
  gtk_widget_destroy(GTK_WIDGET(client_data));
  gtk_widget_set_sensitive(dlg->shell, TRUE);
}

/*************************************************************************/
/**									**/
/**			GFlare Editor					**/
/**			+++ ed						**/
/**									**/
/*************************************************************************/

/*
	This is gflare editor dilaog, one which opens by clicking
	"Edit" button on the selector page in the main dialog.
 */

static void
ed_run (GFlare		     *target_gflare,
	GFlareEditorCallback callback,
	gpointer	     calldata)
{
  GtkWidget	*shell;
  GtkWidget	*table;
  GtkWidget     *inner_prv_frame; 
  GtkWidget     *outer_prv_frame; 
  GtkWidget	*notebook;
  GtkWidget	*button;

  if (!ed)
    ed = g_new0 (GFlareEditor, 1);
  ed->init = TRUE;
  ed->run  = FALSE;
  ed->target_gflare = target_gflare;
  ed->gflare = gflare_dup (target_gflare, target_gflare->name);
  ed->callback = callback;
  ed->calldata = calldata;

  /*
   *	Dialog Shell
   */
  shell = ed->shell = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (shell), _("GFlare Editor"));
  gtk_window_set_position (GTK_WINDOW (shell), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (shell), "destroy",
		      (GtkSignalFunc) ed_close_callback,
		      NULL);

  /*
   *	Action area
   */

  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ed_ok_callback,
		      shell);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (shell));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Rescan Gradients"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ed_rescan_callback,
		      NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*
   *	Preview
   */

  ed->preview = preview_new (ED_PREVIEW_WIDTH, ED_PREVIEW_HEIGHT,
			     ed_preview_init_func, NULL,
			     ed_preview_render_func, NULL,
			     ed_preview_deinit_func, NULL);

  gtk_widget_set_events (GTK_WIDGET (ed->preview->widget), DLG_PREVIEW_MASK);
  gtk_signal_connect (GTK_OBJECT(ed->preview->widget), "event",
		      (GtkSignalFunc) ed_preview_handle_event,
		      NULL);
  ed_preview_calc_window ();

  inner_prv_frame = gtk_frame_new (NULL); 
  gtk_frame_set_shadow_type (GTK_FRAME (inner_prv_frame), GTK_SHADOW_IN); 
  gtk_container_add (GTK_CONTAINER (inner_prv_frame), ed->preview->widget); 
  gtk_container_set_border_width (GTK_CONTAINER (inner_prv_frame), 4); 
  gtk_widget_show (inner_prv_frame); 
 
  outer_prv_frame = gtk_frame_new (_("Preview")); 
  gtk_frame_set_shadow_type (GTK_FRAME (outer_prv_frame), GTK_SHADOW_ETCHED_IN); 
  gtk_container_add (GTK_CONTAINER (outer_prv_frame), inner_prv_frame); 
  gtk_widget_show (outer_prv_frame); 


  /*
   *	Notebook
   */
  notebook = ed->notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);

  ed_make_page_general (ed, notebook);
  ed_make_page_glow (ed, notebook);
  ed_make_page_rays (ed, notebook);
  ed_make_page_sflare (ed, notebook);
  gtk_widget_show (notebook);

  /*
   *	pack them
   */

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 8); 
  gtk_table_attach (GTK_TABLE (table), outer_prv_frame, 0, 1, 0, 1, 
                   GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0); 
  gtk_table_attach (GTK_TABLE (table), notebook, 1, 2, 0, 1, 
                   GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL, 0, 0); 
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 8); 
 
  gtk_widget_show (table);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (shell);

  ed->init = FALSE;
  ed_preview_update ();
}

static void
ed_close_callback (GtkWidget *widget,
		   gpointer   data)
{
  preview_free (ed->preview);
  gflare_free (ed->gflare);
  if (ed->callback)
    (*ed->callback) (ed->run, ed->calldata);
}

static void
ed_ok_callback (GtkWidget *widget,
		 gpointer   data)
{
  ed->run = TRUE;
  gflare_copy (ed->target_gflare, ed->gflare);
  gtk_widget_destroy (ed->shell);
}

static void
ed_rescan_callback (GtkWidget *widget, gpointer data)
{
  ed->init = TRUE;
  gradient_menu_rescan ();
  ed->init = FALSE;
  ed_preview_update ();
  gtk_widget_draw (ed->notebook, NULL);
}


static void
ed_make_page_general (GFlareEditor *ed, GtkWidget *notebook)
{
  GFlare	*gflare = ed->gflare;
  GtkWidget	*vbox;
  GtkWidget	*table;
  GtkWidget	*note_label;
  GtkWidget	*option_menu;

  vbox = gtk_vbox_new (FALSE, 0);

  /*
   *	Scales
   */

  table = gtk_table_new (6, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  entscale_new (table, 0, 0, _("Glow Opacity (%):"),
		ENTSCALE_DOUBLE, &gflare->glow_opacity,
		0.0, 100.0, 1.0, TRUE,
		&ed_entscale_callback, (gpointer) PAGE_GENERAL);

  option_menu = ed_mode_menu_new (&gflare->glow_mode);
  ed_put_mode_menu (table, 0, 1, _("Glow Mode:"), option_menu);

  entscale_new (table, 0, 2, _("Rays Opacity (%):"),
		ENTSCALE_DOUBLE, &gflare->rays_opacity,
		0.0, 100.0, 1.0, TRUE,
		&ed_entscale_callback, (gpointer) PAGE_GENERAL);

  option_menu = ed_mode_menu_new (&gflare->rays_mode);
  ed_put_mode_menu (table, 0, 3, _("Rays Mode:"), option_menu);

  entscale_new (table, 0, 4, _("Second Flares Opacity (%):"),
		ENTSCALE_DOUBLE, &gflare->sflare_opacity,
		0.0, 100.0, 1.0, TRUE,
		&ed_entscale_callback, (gpointer) PAGE_GENERAL);

  option_menu = ed_mode_menu_new (&gflare->sflare_mode);
  ed_put_mode_menu (table, 0, 5, _("Second Flares Mode:"), option_menu);

  gtk_widget_show (table);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*
   *	Create Page
   */
  note_label = gtk_label_new (_("General"));
  gtk_misc_set_alignment (GTK_MISC (note_label), 0.5, 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, note_label);
  gtk_signal_connect (GTK_OBJECT (vbox), "map",
		      (GtkSignalFunc) ed_page_map_callback,
		      (gpointer) PAGE_GENERAL);
  gtk_widget_show (note_label);
  gtk_widget_show (vbox);
}

static void
ed_make_page_glow (GFlareEditor *ed, GtkWidget *notebook)
{
  GFlare	*gflare = ed->gflare;
  GradientMenu	*gm;
  GtkWidget	*vbox;
  GtkWidget	*table;
  GtkWidget	*note_label;

  vbox = gtk_vbox_new (FALSE, 0);

  /*
   *	Gradient Menus
   */

  table = gtk_table_new (3, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  gm = gradient_menu_new ((GradientMenuCallback) &ed_gradient_menu_callback,
			  gflare->glow_radial, gflare->glow_radial );
  ed_put_gradient_menu (table, 0, 0, _("Radial Gradient:"), gm);

  gm = gradient_menu_new ((GradientMenuCallback) &ed_gradient_menu_callback,
			  gflare->glow_angular, gflare->glow_angular );
  ed_put_gradient_menu (table, 0, 1, _("Angular Gradient:"), gm);

  gm = gradient_menu_new ((GradientMenuCallback) &ed_gradient_menu_callback,
			  gflare->glow_angular_size, gflare->glow_angular_size );
  ed_put_gradient_menu (table, 0, 2, _("Angular Size Gradient:"), gm);

  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   *	Scales
   */

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);
  entscale_new (table, 0, 0, _("Size (%):"),
		ENTSCALE_DOUBLE, &gflare->glow_size,
		0.0, 200.0, 1.0, FALSE,
		&ed_entscale_callback, (gpointer) PAGE_GLOW);

  entscale_new (table, 0, 1, _("Rotation:"),
		ENTSCALE_DOUBLE, &gflare->glow_rotation,
		-180.0, 180.0, 1.0, FALSE,
		&ed_entscale_callback, (gpointer) PAGE_GLOW);

  entscale_new (table, 0, 2, _("Hue Rotation:"),
		ENTSCALE_DOUBLE, &gflare->glow_hue,
		-180.0, 180.0, 1.0, FALSE,
		&ed_entscale_callback, (gpointer) PAGE_GLOW);

  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   *	Create Page
   */
  note_label = gtk_label_new (_("Glow"));
  gtk_misc_set_alignment (GTK_MISC (note_label), 0.5, 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, note_label );
  gtk_signal_connect (GTK_OBJECT (vbox), "map",
		      (GtkSignalFunc) ed_page_map_callback,
		      (gpointer) PAGE_GLOW);
  gtk_widget_show (note_label);
  gtk_widget_show (vbox);
}

static void
ed_make_page_rays (GFlareEditor *ed, GtkWidget *notebook)
{
  GFlare	*gflare = ed->gflare;
  GradientMenu	*gm;
  GtkWidget	*vbox;
  GtkWidget	*table;
  GtkWidget	*note_label;

  vbox = gtk_vbox_new (FALSE, 0);

  /*
   *	Gradient Menus
   */

  table = gtk_table_new (3, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  gm = gradient_menu_new ((GradientMenuCallback) &ed_gradient_menu_callback,
			  gflare->rays_radial, gflare->rays_radial );
  ed_put_gradient_menu (table, 0, 0, _("Radial Gradient:"), gm);

  gm = gradient_menu_new ((GradientMenuCallback) &ed_gradient_menu_callback,
			  gflare->rays_angular, gflare->rays_angular );
  ed_put_gradient_menu (table, 0, 1, _("Angular Gradient:"), gm);

  gm = gradient_menu_new ((GradientMenuCallback) &ed_gradient_menu_callback,
			  gflare->rays_angular_size, gflare->rays_angular_size );
  ed_put_gradient_menu (table, 0, 2, _("Angular Size Gradient:"), gm);

  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   *	Scales
   */

  table = gtk_table_new (5, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  entscale_new (table, 0, 0, _("Size (%):"),
		ENTSCALE_DOUBLE, &gflare->rays_size,
		0.0, 200.0, 1.0, FALSE,
		&ed_entscale_callback, (gpointer) PAGE_RAYS);
  entscale_new (table, 0, 1, _("Rotation:"),
		ENTSCALE_DOUBLE, &gflare->rays_rotation,
		-180.0, 180.0, 1.0, FALSE,
		&ed_entscale_callback, (gpointer) PAGE_RAYS);
  entscale_new (table, 0, 2, _("Hue Rotation:"),
		ENTSCALE_DOUBLE, &gflare->rays_hue,
		-180.0, 180.0, 1.0, FALSE,
		&ed_entscale_callback, (gpointer) PAGE_RAYS);
  entscale_new (table, 0, 3, _("# of Spikes:"),
		ENTSCALE_INT, &gflare->rays_nspikes,
		1, 300, 1, FALSE,
		&ed_entscale_callback, (gpointer) PAGE_RAYS);
  entscale_new (table, 0, 4, _("Spike Thickness:"),
		ENTSCALE_DOUBLE, &gflare->rays_thickness,
		1.0, 100.0, 1.0, FALSE,
		&ed_entscale_callback, (gpointer) PAGE_RAYS);

  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   *	Create Pages
   */

  note_label = gtk_label_new (_("Rays"));
  gtk_misc_set_alignment (GTK_MISC (note_label), 0.5, 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, note_label );
  gtk_signal_connect (GTK_OBJECT (vbox), "map",
		      (GtkSignalFunc) ed_page_map_callback,
		      (gpointer) PAGE_RAYS);
  gtk_widget_show (note_label);
  gtk_widget_show (vbox);
}

static void
ed_make_page_sflare (GFlareEditor *ed, GtkWidget *notebook)
{
  GFlare	*gflare = ed->gflare;
  GradientMenu	*gm;
  GtkWidget	*note_label;
  GtkWidget	*vbox;
  GtkWidget	*table;
  GtkWidget	*shape_frame;
  GtkWidget	*shape_vbox;
  GSList	*shape_group = NULL;
  GtkWidget	*polygon_hbox;
  GtkWidget	*seed_hbox;
  GtkWidget	*toggle;
  GtkWidget	*label;
  GtkWidget	*entry;
  char		buf[256];


  vbox = gtk_vbox_new (FALSE, 0);

  /*
   *	Gradient Menus
   */

  table = gtk_table_new (3, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  gm = gradient_menu_new ((GradientMenuCallback) &ed_gradient_menu_callback,
			  gflare->sflare_radial, gflare->sflare_radial );
  ed_put_gradient_menu (table, 0, 0, _("Radial Gradient:"), gm);

  gm = gradient_menu_new ((GradientMenuCallback) &ed_gradient_menu_callback,
			  gflare->sflare_sizefac, gflare->sflare_sizefac );
  ed_put_gradient_menu (table, 0, 1, _("Size Factor Gradient:"), gm);

  gm = gradient_menu_new ((GradientMenuCallback) &ed_gradient_menu_callback,
			  gflare->sflare_probability, gflare->sflare_probability );
  ed_put_gradient_menu (table, 0, 2, _("Probability Gradient:"), gm);

  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   *	Scales
   */

  table = gtk_table_new (4, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  entscale_new (table, 0, 0, _("Size (%):"),
		ENTSCALE_DOUBLE, &gflare->sflare_size,
		0.0, 200.0, 1.0, FALSE,
		&ed_entscale_callback, (gpointer) PAGE_SFLARE);
  entscale_new (table, 0, 1, _("Rotation:"),
		ENTSCALE_DOUBLE, &gflare->sflare_rotation,
		-180.0, 180.0, 1.0, FALSE,
		&ed_entscale_callback, (gpointer) PAGE_SFLARE);
  entscale_new (table, 0, 2, _("Hue Rotation:"),
		ENTSCALE_DOUBLE, &gflare->sflare_hue,
		-180.0, 180.0, 1.0, FALSE,
		&ed_entscale_callback, (gpointer) PAGE_SFLARE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   *	Shape Radio Button Frame
   */

  shape_vbox = gtk_vbox_new (FALSE, 5);

  toggle = gtk_radio_button_new_with_label (shape_group, _("Circle"));
  shape_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) ed_shape_radio_callback,
		      (gpointer) GF_CIRCLE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
			       gflare->sflare_shape == GF_CIRCLE);
  gtk_widget_show (toggle);
  gtk_box_pack_start (GTK_BOX (shape_vbox), toggle, FALSE, FALSE, 0);

  toggle = ed->polygon_toggle =
    gtk_radio_button_new_with_label (shape_group, _("Polygon"));
  shape_group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) ed_shape_radio_callback,
		      (gpointer) GF_POLYGON);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
			       gflare->sflare_shape == GF_POLYGON);
  gtk_widget_show (toggle);

  entry = ed->polygon_entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buf, "%d", gflare->sflare_nverts);
  gtk_entry_set_text (GTK_ENTRY (entry), buf);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) ed_ientry_callback,
		      &gflare->sflare_nverts);
  gtk_widget_show (entry);

  polygon_hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (polygon_hbox), toggle, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (polygon_hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (polygon_hbox);

  gtk_box_pack_start (GTK_BOX (shape_vbox), polygon_hbox, FALSE, FALSE, 0);
  gtk_widget_show (shape_vbox);

  shape_frame = gtk_frame_new (_("Shape of Second Flares"));
  gtk_frame_set_shadow_type (GTK_FRAME (shape_frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (shape_frame), shape_vbox);
  gtk_widget_show (shape_frame);
  gtk_box_pack_start (GTK_BOX (vbox), shape_frame, FALSE, FALSE, 0);

  /*
   *	Random Seed Entry
   */

  seed_hbox = gtk_hbox_new (FALSE, 5);
  label = gtk_label_new (_("Random Seed (-1 means current time):"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (seed_hbox), label, FALSE, FALSE, 0);
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buf, "%d", gflare->sflare_seed);
  gtk_entry_set_text (GTK_ENTRY (entry), buf);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) ed_ientry_callback,
		      &gflare->sflare_seed);
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (seed_hbox), entry, FALSE, TRUE, 0);
  gtk_widget_show (seed_hbox);

  gtk_box_pack_start (GTK_BOX (vbox), seed_hbox, FALSE, FALSE, 0);

  /*
   *	Create Pages
   */
  note_label = gtk_label_new (_("Second Flares"));
  gtk_misc_set_alignment (GTK_MISC (note_label), 0.5, 0.5);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, note_label );
  gtk_signal_connect (GTK_OBJECT (vbox), "map",
		      (GtkSignalFunc) ed_page_map_callback,
		      (gpointer) PAGE_SFLARE);
  gtk_widget_show (note_label);
  gtk_widget_show (vbox);
}

GtkWidget *
ed_mode_menu_new (GFlareMode *mode_var)
{
  GtkWidget	*option_menu;
  GtkWidget	*menu;
  GtkWidget	*menuitem;
  gint		i;
  GFlareMode	mode;

  option_menu = gtk_option_menu_new ();
  menu = gtk_menu_new ();

  for (i = 0; i < GF_NUM_MODES; i++)
    {
      menuitem = gtk_menu_item_new_with_label (gettext(gflare_menu_modes[i]));

      gtk_object_set_user_data (GTK_OBJECT (menuitem), (gpointer) i);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  (GtkSignalFunc) ed_mode_menu_callback,
			  mode_var);
      gtk_widget_show (menuitem);
      gtk_menu_append (GTK_MENU (menu), menuitem);
    }
  mode = *mode_var;
  if (mode < 0 || mode >= GF_NUM_MODES)
    mode = GF_NORMAL;
  gtk_menu_set_active (GTK_MENU (menu), mode);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
  gtk_widget_show (option_menu);

  return option_menu;
}

static void
ed_put_mode_menu (GtkWidget *table, gint x, gint y,
		  gchar *caption, GtkWidget *option_menu)
{
  GtkWidget	*label;

  label = gtk_label_new (caption);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);

  gtk_table_attach (GTK_TABLE (table), label,
		    x	 , x + 1, y, y + 1,
		    GTK_FILL, 0, 0, 0);
  gtk_table_attach (GTK_TABLE (table), option_menu,
		    x + 1, x + 2, y, y + 1,
		    0, 0, 0, 0);
}

/*
  puts gradient menu with caption into table
  occupies 1 row and 3 cols in table
 */
static void
ed_put_gradient_menu (GtkWidget *table, gint x, gint y,
		      gchar *caption, GradientMenu *gm)
{
  GtkWidget	*label;

  label = gtk_label_new (caption);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);

  gtk_object_set_user_data (GTK_OBJECT (gm->option_menu), ed);
  gtk_table_attach (GTK_TABLE (table), label,
		    x	 , x + 1, y, y + 1,
		    GTK_FILL, 0, 0, 0);
  gtk_table_attach (GTK_TABLE (table), gm->preview,
		    x + 1, x + 2, y, y + 1,
		    0, 0, 0, 0);
  gtk_table_attach (GTK_TABLE (table), gm->option_menu,
		    x + 2, x + 3, y, y + 1,
		    0, 0, 0, 0);
}

static void
ed_entscale_callback (gdouble new_val, gpointer data)
{
  DEBUG_PRINT (("ed_entscale_callback\n"));
  ed_preview_update ();
}

static void
ed_mode_menu_callback (GtkWidget *widget, gpointer data)
{
  GFlareMode	*mode_var;

  mode_var = data;
  *mode_var = (GFlareMode) gtk_object_get_user_data (GTK_OBJECT (widget));

  ed_preview_update ();
}

static void
ed_gradient_menu_callback (gchar *gradient_name, gpointer data)
{
  gchar			*dest_string = data;

  /* @GRADIENT_NAME */
  gradient_name_copy (dest_string, gradient_name);
  ed_preview_update ();
}

static void
ed_shape_radio_callback (GtkWidget *widget, gpointer data)
{
  gint		state;

  state = GTK_TOGGLE_BUTTON (widget)->active;
  if (state)
    {
      ed->gflare->sflare_shape = (GFlareShape) data;
    }
  if (!ed->init && widget == ed->polygon_toggle)
    gtk_widget_set_sensitive (ed->polygon_entry, state);

  ed_preview_update ();
}

static void
ed_ientry_callback (GtkWidget *widget, gpointer data)
{
  gint		new_val;

  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  *(gint *)data = new_val;

  ed_preview_update ();
}


/*
  NOTE: This is hack, because this code depends on internal "map"
  signal of changing pages of gtknotebook.
 */
static void
ed_page_map_callback (GtkWidget *widget, gpointer data)
{
  gint		page_num = (gint) data;

  DEBUG_PRINT(("ed_page_map_callback\n"));

  ed->cur_page = page_num;
  ed_preview_update ();
}


static void
ed_preview_update ()
{
  if (ed->init)
    return;

  ed->init_params_done = FALSE;
  preview_render_start (ed->preview);
}

static gint
ed_preview_init_func (Preview *preview, gpointer data)
{
  int	type = 0;

  if (ed->init_params_done == FALSE)
    {
      switch (ed->cur_page)
	{
	case PAGE_GENERAL:
	  type = (CALC_GLOW | CALC_RAYS | CALC_SFLARE);
	  break;
	case PAGE_GLOW:
	  type = CALC_GLOW;
	  break;
	case PAGE_RAYS:
	  type = CALC_RAYS;
	  break;
	case PAGE_SFLARE:
	  type = CALC_SFLARE;
	  break;
	default:
	  g_warning ("ed_preview_edit_func: bad page");
	  break;
	}
      calc_init_params (ed->gflare, type,
			ED_PREVIEW_WIDTH/2, ED_PREVIEW_HEIGHT/2,
			ED_PREVIEW_WIDTH/2, 0.0, 0.0,
			pvals.vangle, pvals.vlength);

      ed->init_params_done = TRUE;
      return TRUE;
    }
  return calc_init_progress ();
}

static void
ed_preview_deinit_func (Preview *preview, gpointer data)
{
  if (ed->init_params_done)
    {
      calc_deinit ();
      ed->init_params_done = FALSE;
    }
}

static void
ed_preview_render_func (Preview *preview, guchar *buffer, gint y, gpointer data)
{
  switch (ed->cur_page)
    {
    case PAGE_GENERAL:
      ed_preview_render_general (buffer, y);
      break;
    case PAGE_GLOW:
      ed_preview_render_glow (buffer, y);
      break;
    case PAGE_RAYS:
      ed_preview_render_rays (buffer, y);
      break;
    case PAGE_SFLARE:
      ed_preview_render_sflare (buffer, y);
      break;
    default:
      g_warning ("hmm, bad page in ed_preview_render_func ()");
      break;
    }
}

static void
ed_preview_render_general (guchar *buffer, gint y)
{
  int		x, i;
  guchar	gflare_pix[4];
  static guchar src_pix[4] = {0, 0, 0, OPAQUE};
  int		gflare_a;

  for (x = 0; x < ED_PREVIEW_WIDTH; x++)
    {
      calc_gflare_pix (gflare_pix, x, y, src_pix);
      gflare_a = gflare_pix[3];

      for (i = 0; i < 3; i++)
	{
	  *buffer++ = gflare_pix[i] * gflare_a / 255;
	}
    }
}


static void
ed_preview_render_glow (guchar *buffer, gint y)
{
  int		x, i;
  guchar	pix[4];

  for (x = 0; x < ED_PREVIEW_WIDTH; x++)
    {
      calc_glow_pix (pix, x, y);
      for (i = 0; i < 3; i++)
	*buffer++ = pix[i] * pix[3] / 255;
    }
}

static void
ed_preview_render_rays (guchar *buffer, gint y)
{
  int		x, i;
  guchar	pix[4];

  for (x = 0; x < ED_PREVIEW_WIDTH; x++)
    {
      calc_rays_pix (pix, x, y);
      for (i = 0; i < 3; i++)
	*buffer++ = pix[i] * pix[3] / 255;
    }
}

static void
ed_preview_render_sflare (guchar *buffer, gint y)
{
  int		x, i;
  guchar	pix[4];
  static guchar src_pix[4] = {0, 0, 0, OPAQUE};

  for (x = 0; x < ED_PREVIEW_WIDTH; x++)
    {
      calc_sflare_pix (pix, x, y, src_pix);
      for (i = 0; i < 3; i++)
	*buffer++ = pix[i] * pix[3] / 255;
    }
}

/*************************************************************************/
/**									**/
/**			+++ Preview					**/
/**									**/
/*************************************************************************/

/*
	this is generic preview routines.
 */


/*
    Routines to render the preview in background
 */
Preview *
preview_new (gint		   width,
	     gint		   height,
	     PreviewInitFunc	   init_func,
	     gpointer		   init_data,
	     PreviewRenderFunc	   render_func,
	     gpointer		   render_data,
	     PreviewDeinitFunc	   deinit_func,
	     gpointer		   deinit_data)
{
  Preview	*preview;

  preview = g_new0 (Preview, 1);

  preview->widget = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_object_set_user_data (GTK_OBJECT (preview->widget), preview);
  gtk_preview_size (GTK_PREVIEW (preview->widget), width, height);
  gtk_widget_show (preview->widget);

  preview->width	   = width;
  preview->height	   = height;
  preview->init_func	   = init_func;
  preview->init_data	   = init_data;
  preview->render_func	   = render_func;
  preview->render_data	   = render_data;
  preview->deinit_func	   = deinit_func;
  preview->deinit_data	   = deinit_data;
  preview->idle_tag	   = 0;
  preview->buffer	   = g_new (guchar, width * 3);

  return preview;
}

void
preview_free (Preview *preview)
{
  preview_render_end (preview);
  /* not destroy preview->widget */
  g_free (preview->buffer);
  g_free (preview);
}


/*
  Start rendering of the preview in background using an idle event.
  If already started and not yet finished, stop it first.
 */
void
preview_render_start (Preview *preview)
{
  DEBUG_PRINT(("preview_render_start\n"));

  preview_render_end (preview);

  preview->init_done = FALSE;
  preview->current_y = 0;
  preview->drawn_y = 0;
  preview->timeout_tag = gtk_timeout_add (100, (GtkFunction) preview_render_start_2, preview);
}

static gint
preview_render_start_2 (Preview *preview)
{
  preview->timeout_tag = 0;
  preview->idle_tag = gtk_idle_add ((GtkFunction) preview_handle_idle, preview);
  return FALSE;
}


void
preview_render_end (Preview *preview)
{
  if (preview->timeout_tag > 0)
    {
      gtk_timeout_remove (preview->timeout_tag);
      preview->timeout_tag = 0;
    }
  if (preview->idle_tag > 0)
    {
      if (preview->deinit_func)
	(*preview->deinit_func) (preview, preview->deinit_data);

      gtk_idle_remove (preview->idle_tag);
      preview->idle_tag = 0;
      DEBUG_PRINT(("preview_render_end\n\n"));
    }
}

/*
  Handle an idle event.
  Return FALSE if done, TRUE otherwise.
 */
static gint
preview_handle_idle (Preview *preview)
{
  gint			done = FALSE;
  GdkRectangle		draw_rect;

  if (preview->init_done == FALSE)
    {
      if (preview->init_func &&
	  (*preview->init_func) (preview, preview->init_data))
	return TRUE;
      preview->init_done = TRUE;
    }

  if (preview->render_func)
    (*preview->render_func) (preview, preview->buffer, preview->current_y,
			     preview->render_data);
  else
    memset (preview->buffer, 0, preview->width * 3);

  gtk_preview_draw_row (GTK_PREVIEW (preview->widget), preview->buffer,
			0, preview->current_y, preview->width);

  if (++preview->current_y >= preview->height)
    done = TRUE;

  if (done || preview->current_y % 20 == 0)
    {
      draw_rect.x      = 0;
      draw_rect.y      = preview->drawn_y;
      draw_rect.width  = preview->width;
      draw_rect.height = preview->current_y - preview->drawn_y;
      preview->drawn_y = preview->current_y;
      gtk_widget_draw (preview->widget, &draw_rect);
    }

  if (done)
    {
      preview_render_end (preview);
      return FALSE;
    }

  return TRUE;
}

/*
  Convert RGBA to RGB with rendering gray check if needed.
	(from nova.c)
  input:  guchar src[4]		RGBA pixel
  output: guchar dest[3]	RGB pixel
 */

void
preview_rgba_to_rgb (guchar *dest, gint x, gint y, guchar *src)
{
  gint src_a;
  gint check;
  gint b;

  src_a = src[3];

  if (src_a == OPAQUE)	/* full opaque */
    {
      for (b = 0; b < 3; b++)
	dest[b] = src[b];
    }
  else
    {
      if ((x % (GIMP_CHECK_SIZE) < GIMP_CHECK_SIZE_SM) ^
	  (y % (GIMP_CHECK_SIZE) < GIMP_CHECK_SIZE_SM))
	check = GIMP_CHECK_LIGHT * 255;
      else
	check = GIMP_CHECK_DARK * 255;

      if (src_a == 0)	/* full transparent */
	{
	  for (b = 0; b < 3; b++)
	    dest[b] = check;
	}
      else
	{
	  for (b = 0; b < 3; b++)
	    dest[b] = (src[b] * src_a + check * (OPAQUE-src_a)) / OPAQUE;
	}
    }
}

/*************************************************************************/
/**									**/
/**			+++ Gradient Menu				**/
/**			+++ gm						**/
/**									**/
/*************************************************************************/

void
gradient_menu_init ()
{
  gm_gradient_get_list ();
  gradient_menus = NULL;
}

void
gradient_menu_rescan ()
{
  GList		*tmp;
  GradientMenu	*gm;
  GtkWidget	*menu;

  /* Detach and destroy menus first */
  tmp = gradient_menus;
  while (tmp)
    {
      gm  = tmp->data;
      tmp = tmp->next;
      menu = GTK_MULTI_OPTION_MENU (gm->option_menu)->menu;
      if (menu)
	{
	  gtk_multi_option_menu_remove_menu (GTK_MULTI_OPTION_MENU (gm->option_menu));
	}
    }

  /* reget list of gradient names */
  gm_gradient_get_list ();

  /* Create menus and attach them again */
  tmp = gradient_menus;
  while (tmp)
    {
      GtkWidget *parent;

      gm  = tmp->data;
      tmp = tmp->next;
      /* @GRADIENT_NAME */
      menu = gm_menu_new (gm, gm->gradient_name);

      /*
	FIXME:
	This is a kind of hack so that it doesn't mess up gtknotebook
	to set menu into an option menu which is not shown (but
	GTK_WIDGET_VISIBLE)
       */
      parent = gm->option_menu->parent;
      if (0 && (parent != NULL) && GTK_CHECK_TYPE (parent, gtk_container_get_type ()))
	{
	    /*gtk_container_block_resize (GTK_CONTAINER (parent));*/
	    gtk_multi_option_menu_set_menu (GTK_MULTI_OPTION_MENU (gm->option_menu), menu);
	    /*gtk_container_unblock_resize (GTK_CONTAINER (parent));*/
	}
      else
	    gtk_multi_option_menu_set_menu (GTK_MULTI_OPTION_MENU (gm->option_menu), menu);
    }
}

GradientMenu *
gradient_menu_new (GradientMenuCallback callback,
		   gpointer			callback_data,
		   gchar			*default_gradient_name)
{
  GtkWidget	*menu;
  GradientMenu	*gm;

  gm = g_new (GradientMenu, 1);

  gm->callback = callback;
  gm->callback_data = callback_data;

  gm->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size( GTK_PREVIEW (gm->preview),
		    GM_PREVIEW_WIDTH,
		    GM_PREVIEW_HEIGHT );

  gm->option_menu = gtk_multi_option_menu_new ();

  /* @GRADIENT_NAME */
  menu = gm_menu_new (gm, default_gradient_name);
  gtk_multi_option_menu_set_menu (GTK_MULTI_OPTION_MENU (gm->option_menu), menu);

  gtk_widget_show (gm->preview);
  gtk_widget_show (gm->option_menu);

  gradient_menus = g_list_append (gradient_menus, gm);
  gtk_signal_connect (GTK_OBJECT (gm->option_menu), "destroy",
		      (GtkSignalFunc) &gm_option_menu_destroy_callback, gm);

  return gm;
}

void
gradient_menu_destroy (GradientMenu *gm)
{
  gtk_widget_destroy (gm->preview);
  gtk_widget_destroy (gm->option_menu);	  /* gm is removed from gradient_menus too */

  g_free (gm);
}

/* Local Functions */

static void
gm_gradient_get_list ()
{
  int	i;

  if (gradient_names)
    {
      for (i = 0; i < num_gradient_names; i++)
	g_free (gradient_names[i]);
      g_free (gradient_names);
    }

  gradient_cache_flush ();	/* to make sure */
  gradient_names = gradient_get_list (&num_gradient_names);
}

/*
 *  Create GtkMenu and arrange GtkMenuItem's of gradient names into it
 */

static GtkWidget *
gm_menu_new (GradientMenu *gm, gchar *default_gradient_name )
{
  GtkWidget	*menu;
  GtkWidget	*menuitem;
  gchar		*active_name;

  menu = gtk_menu_new ();

  if (num_gradient_names == 0)
    {
      menuitem = gtk_menu_item_new_with_label (_("none"));
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_widget_show (menuitem);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_menu_set_active (GTK_MENU (menu), 0);
    }
  else /* num_gradient_names == 0 */
    {
      menu = gm_menu_create_sub_menus (gm, 0, &active_name,
				       default_gradient_name);
      if (active_name == NULL)
	{
	  active_name = gradient_names[0];
	  g_warning (_("Not found \"%s\": used \"%s\" instead"),
		     default_gradient_name, active_name);
	}

      gradient_name_copy (gm->gradient_name, active_name);
      gm_preview_draw (gm->preview, active_name);
      if ( GTK_WIDGET_VISIBLE (gm->preview) && GTK_WIDGET_MAPPED(gm->preview))
	{
	  DEBUG_PRINT (("gm_menu_new: preview is visible and mapped\n"));
	  gtk_widget_draw (gm->preview, NULL);
	}
      if (gm->callback)
	(* gm->callback) ( active_name, gm->callback_data );

    } /* num_gradient_names == 0 */

  return menu;
}

static GtkWidget *
gm_menu_create_sub_menus (GradientMenu *gm,
			  gint start_n,
			  gchar **active_name_ptr,
			  gchar *default_gradient_name)
{
  GtkWidget	*menu, *sub_menu;
  gchar		*sub_active_name;
  GtkWidget	*menuitem;
  gchar		*name;
  gint		active_i = 0;
  gint		i, n;

  *active_name_ptr = NULL;
  if (start_n >= num_gradient_names)
    {
      return NULL;
    }

  /* gradient_names[] are malloced strings and alive during
     this menuitem lives */

  menu = gtk_menu_new ();
  for (i = 0, n = start_n; i < GM_MENU_MAX && n < num_gradient_names; i++, n++)
    {
      name = gradient_names[n];
      if (strcmp (name, default_gradient_name) == 0)
	{
	  active_i = i;
	  *active_name_ptr = name;
	}
      menuitem = gtk_menu_item_new_with_label (name);
      gtk_object_set_user_data (GTK_OBJECT (menuitem), gm);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  (GtkSignalFunc) gm_menu_item_callback,
			  name);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
    } /* for */

  sub_menu = gm_menu_create_sub_menus (gm, n, &sub_active_name,
				       default_gradient_name);
  if (sub_menu)
    {
      active_i += 2;

      /* hline */
      menuitem = gtk_menu_item_new ();
      gtk_widget_show (menuitem);
      gtk_menu_prepend (GTK_MENU (menu), menuitem);

      menuitem = gtk_menu_item_new_with_label (_("More..."));
      gtk_widget_show (menuitem);
      gtk_menu_prepend (GTK_MENU (menu), menuitem);
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), sub_menu);

      if (sub_active_name)
	{
	  *active_name_ptr = sub_active_name;
	  active_i = 0; /* "More ..." */
	}
    }

  gtk_menu_set_active (GTK_MENU (menu), active_i);
  return menu;
}

static void
gm_menu_item_callback (GtkWidget *w, gpointer data)
{
  GradientMenu		*gm;
  gchar			*gradient_name = (gchar *) data;

  DEBUG_PRINT(("gm_menu_item_callback\n"));

  gm =	(GradientMenu *) gtk_object_get_user_data (GTK_OBJECT (w));
  gradient_name_copy (gm->gradient_name, gradient_name);

  gm_preview_draw (gm->preview, gradient_name);
  if (GTK_WIDGET_VISIBLE (gm->preview) && GTK_WIDGET_MAPPED(gm->preview))
    {
      DEBUG_PRINT(("gm_menu_item_callback: preview is visible and mapped\n"));
      gtk_widget_draw (gm->preview, NULL);
    }

  if (gm->callback)
    (* gm->callback) ( gradient_name, gm->callback_data );
}

static void
gm_preview_draw (GtkWidget *preview, gchar *gradient_name)
{
  guchar	values[GM_PREVIEW_WIDTH][4];
  gint		nvalues = GM_PREVIEW_WIDTH;
  int		row, irow, col;
  guchar	dest_row[GM_PREVIEW_WIDTH][3];
  guchar	*dest, *src;
  int		check, b;
  const int	alpha = 3;

  gradient_get_values (gradient_name, (guchar *)values, nvalues);

  for( row = 0; row < GM_PREVIEW_HEIGHT; row += GIMP_CHECK_SIZE_SM )
    {
      for( col = 0; col < GM_PREVIEW_WIDTH; col++ )
	{
	  dest = dest_row[col];
	  src = values[col];

	  if( src[alpha] == OPAQUE )
	    {
	      /* no alpha channel or opaque -- simple way */
	      for ( b = 0; b < alpha; b++ )
		dest[b] = src[b];
	    }
	  else
	    {
	      /* more or less transparent */
	      if( ( col % (GIMP_CHECK_SIZE) < GIMP_CHECK_SIZE_SM ) ^
		  ( row % (GIMP_CHECK_SIZE) < GIMP_CHECK_SIZE_SM ) )
		check = GIMP_CHECK_LIGHT * 255;
	      else
		check = GIMP_CHECK_DARK * 255;

	      if ( src[alpha] == 0 )
		{
		  /* full transparent -- check */
		  for ( b = 0; b < alpha; b++ )
		    dest[b] = check;
		}
	      else
		{
		  /* middlemost transparent -- mix check and src */
		  for ( b = 0; b < alpha; b++ )
		    dest[b] = ( src[b]*src[alpha] + check*(OPAQUE-src[alpha]) ) / OPAQUE;
		}
	    }
	}
      for( irow = 0; irow < GIMP_CHECK_SIZE_SM && row + irow < GM_PREVIEW_HEIGHT; irow++ )
	{
	  gtk_preview_draw_row( GTK_PREVIEW (preview), (guchar*) dest_row,
				0, row + irow, GM_PREVIEW_WIDTH );
	}
    }

}

static void
gm_option_menu_destroy_callback (GtkWidget *w, gpointer data)
{
  GradientMenu *gm = data;
  gradient_menus = g_list_remove (gradient_menus, gm);
}

/*************************************************************************/
/**									**/
/**			+++ Gradients					**/
/**									**/
/*************************************************************************/

/*
    Manage both internal and external gradients: list up, cache,
    sampling, etc.

    External gradients are cached.
 */


void
gradient_name_copy (gchar *dest, gchar *src)
{
  strncpy (dest, src, GRADIENT_NAME_MAX);
  dest[GRADIENT_NAME_MAX-1] = '\0';
}

/*
  Translate SPACE to "\\040", etc.
 */
void
gradient_name_encode (guchar *dest, guchar *src)
{
  int	cnt = GRADIENT_NAME_MAX - 1;

  while (*src && cnt--)
    {
      if (iscntrl (*src) || isspace (*src) || *src == '\\')
	{
	  sprintf (dest, "\\%03o", *src++);
	  dest += 4;
	}
      else
	*dest++ = *src++;
    }
  *dest = '\0';
}

/*
  Translate "\\040" to SPACE, etc.
 */
void
gradient_name_decode (guchar *dest, guchar *src)
{
  int	cnt = GRADIENT_NAME_MAX - 1;
  int	tmp;

  while (*src && cnt--)
    {
      if (*src == '\\' && *(src+1) && *(src+2) && *(src+3))
	{
	  sscanf (src+1, "%3o", &tmp);
	  *dest++ = tmp;
	  src += 4;
	}
      else
	*dest++ = *src++;
    }
  *dest = '\0';
}


void
gradient_init ()
{
  gradient_cache_head = NULL;
  gradient_cache_count = 0;
}

void
gradient_free ()
{
  gradient_cache_flush ();
}

char **
gradient_get_list (gint *num_gradients)
{
  gchar	      **gradients;
  gchar	      **external_gradients = NULL;
  gint	      external_ngradients = 0;
  gint	      i, n;


  gradient_cache_flush ();
  external_gradients = gimp_gradients_get_list (&external_ngradients);

  *num_gradients = internal_ngradients + external_ngradients;
  gradients = g_new (gchar *, *num_gradients);

  n = 0;
  for (i = 0; i < internal_ngradients; i++)
    {
      gradients[n++] = g_strdup (internal_gradients[i]);
    }
  for (i = 0; i < external_ngradients; i++)
    {
      gradients[n++] = g_strdup (external_gradients[i]);
    }

  return gradients;
}

void
gradient_get_values (gchar *gradient_name, guchar *values, gint nvalues)
{
  /* DEBUG_PRINT (("gradient_get_values: %s %d\n", gradient_name, nvalues)); */

  /*
    Criteria to distinguish internal and external is rather simple here.
    It should be fixed later.
   */
  if (gradient_name[0] == '%')
    gradient_get_values_internal (gradient_name, values, nvalues);
  else
    gradient_get_values_external (gradient_name, values, nvalues);
}

static void
gradient_get_values_internal (gchar *gradient_name, guchar *values, gint nvalues)
{
  static guchar white[4] = {255,255,255,255};
  static guchar white_trans[4] = {255,255,255,0};
  static guchar red_trans[4] = {255,0,0,0};
  static guchar blue_trans[4] = {0,0,255,0};
  static guchar yellow_trans[4] = {255,255,0,0};

  /*
    The internal gradients here are example --
    What kind of internals would be useful ?
   */
  if( !strcmp(gradient_name, "%white"))
    {
      gradient_get_blend (white, white, values, nvalues);
    }
  else if( !strcmp(gradient_name, "%white_grad"))
    {
      gradient_get_blend (white, white_trans, values, nvalues);
    }
  else if (!strcmp (gradient_name, "%red_grad" ))
    {
      gradient_get_blend (white, red_trans, values, nvalues);
    }
  else if (!strcmp (gradient_name, "%blue_grad" ))
    {
      gradient_get_blend (white, blue_trans, values, nvalues);
    }
  else if (!strcmp (gradient_name, "%yellow_grad" ))
    {
      gradient_get_blend (white, yellow_trans, values, nvalues);
    }
  else if (!strcmp (gradient_name, "%random" ))
    {
      gradient_get_random (1, values, nvalues);
    }
  else
    {
      gradient_get_default (gradient_name, values, nvalues);
    }
}

static void
gradient_get_blend (guchar *fg, guchar *bg, guchar *values, gint nvalues)
{
  gdouble	x;
  int		i, j;
  guchar	*v = values;

  for (i=0; i<nvalues; i++)
    {
      x = (double) i / nvalues;
      for (j = 0; j < 4; j++)
	*v++ = fg[j] * (1 - x) + bg[j] * x;
    }
}

static void
gradient_get_random (gint seed, guchar *values, gint nvalues)
{
  int		i, j;
  int		inten;
  guchar	*v = values;

  /*
    This is really simple  -- gaussian noise might be better
   */
  srand (seed);
  for (i = 0; i < nvalues; i++)
    {
      inten = rand () % 256;
      for (j = 0; j < 3; j++)
	*v++ = inten;
      *v++ = 255;
    }
}

static void
gradient_get_default (gchar *name, guchar *values, gint nvalues)
{
  double	e[3];
  double	x;
  int		i, j;
  guchar	*v = values;

  /*
    Create gradient by name
   */
  name++;
  for (j = 0; j < 3; j++)
    e[j] = name[j] / 255.0;

  for (i = 0; i < nvalues; i++)
    {
      x = (double) i / nvalues;
      for (j = 0; j < 3; j++)
	*v++ = 255 * pow (x, e[j]);
      *v++ = 255;
    }
}

/*
  Caching gradients is really needed. It really takes 0.2 seconds each
  time to resample an external gradient. (And this plug-in has
  currently 6 gradient menus.)

  However, this caching routine is not too good. It picks up just
  GRADIENT_RESOLUTION samples everytime, and rescales it later.	 And
  cached values are stored in guchar array. No accuracy.
 */
static void
gradient_get_values_external (gchar *gradient_name, guchar *values, gint nvalues)
{
  GradientCacheItem *ci;
  gint		    found;
#ifdef DEBUG
  clock_t	clk = clock ();
#endif

  g_return_if_fail (nvalues >= 2);

  ci = gradient_cache_lookup (gradient_name, &found);
  if (!found)
    {
      gradient_get_values_real_external (gradient_name, ci->values, GRADIENT_RESOLUTION);
    }
  if (nvalues == GRADIENT_RESOLUTION)
    {
      memcpy (values, ci->values, 4 * GRADIENT_RESOLUTION);
    }
  else
    {
      double	pos, frac;
      int	ipos;
      int	i, j;

      for (i = 0; i < nvalues; i++)
	{
	  pos = ((double) i / (nvalues - 1)) * (GRADIENT_RESOLUTION - 1);
	  g_assert (0 <= pos && pos <= GRADIENT_RESOLUTION - 1);
	  ipos = (int) pos; frac = pos - ipos;
	  if (frac == 0.0)
	    {
	      memcpy (&values[4 * i], &ci->values[4 * ipos], 4);
	    }
	  else
	    for (j = 0; j < 4; j++)
	      values[4 * i + j] = ci->values[4 * ipos + j] * (1 - frac)
				+ ci->values[4 * (ipos + 1) + j] * frac;
	}
    }

#ifdef DEBUG
  get_values_external_clock += clock () - clk;
  get_values_external_count ++;
#endif

}

static void
gradient_get_values_real_external (gchar *gradient_name, guchar *values, gint nvalues)
{
  gchar		*old_name;
  gdouble	*tmp_values;
  int		i, j;

  old_name = gimp_gradients_get_active ();

  gimp_gradients_set_active (gradient_name);

  tmp_values = gimp_gradients_sample_uniform (nvalues);
  for (i = 0; i < nvalues; i++)
    for (j = 0; j < 4; j++)
      values[4*i+j] = (guchar) (tmp_values[4*i+j] * 255);

  gimp_gradients_set_active (old_name);

  g_free (tmp_values);
  g_free (old_name);
}

void
gradient_cache_flush ()
{
  GradientCacheItem	*ci, *tmp;

  ci = gradient_cache_head;
  while (ci)
    {
      tmp = ci->next;
      g_free (ci);
      ci = tmp;
    }
  gradient_cache_head = NULL;
  gradient_cache_count = 0;
}

static GradientCacheItem *
gradient_cache_lookup (gchar *name, gint *found)
{
  GradientCacheItem	*ci;

  ci = gradient_cache_head;
  while (ci)
    {
      if (!strcmp (ci->name, name))
	break;
      ci = ci->next;
    }
  if (ci)
    {
      *found = TRUE;
      if (!ci->prev)
	{
	  g_assert (ci == gradient_cache_head);
	  return ci;
	}
      ci->prev->next = ci->next;
      if (ci->next)
	ci->next->prev = ci->prev;
      ci->next = gradient_cache_head;
      gradient_cache_head->prev = ci;
      gradient_cache_head = ci;
      ci->prev = NULL;
      return ci;
    }
  else
    {
      *found = FALSE;
      while (gradient_cache_count >= GRADIENT_CACHE_SIZE)
	gradient_cache_zorch();
      ci = g_new (GradientCacheItem, 1);
      strncpy (ci->name, name, GRADIENT_NAME_MAX - 1);
      ci->next = gradient_cache_head;
      ci->prev = NULL;
      if (gradient_cache_head)
	gradient_cache_head->prev = ci;
      gradient_cache_head = ci;
      ++gradient_cache_count;
      return ci;
    }
}

static void
gradient_cache_zorch ()
{
  GradientCacheItem	*ci = gradient_cache_head;

  while (ci && ci->next)
    {
      ci = ci->next;
    }
  if (ci)
    {
      g_assert (ci->next == NULL);
      if (ci->prev)
	ci->prev->next = NULL;
      else
	gradient_cache_head = NULL;
      g_free (ci);
      --gradient_cache_count;
    }
}

#ifdef DEBUG
void
gradient_report ()
{
  double total = (double) get_values_external_clock / CLOCKS_PER_SEC;

  printf( "gradient_get_values_external %.2f sec. / %d times (ave %.2f sec.)\n",
	  total,
	  get_values_external_count,
	  total / get_values_external_count );
}
#endif

/*************************************************************************/
/**									**/
/**			+++ Entscale					**/
/**									**/
/*************************************************************************/

/* these routines are taken from my private subroutine collection. */

/* -*- mode:c -*- */
/*
  Entry and Scale pair (int and double integrated)

  This is an attempt to combine entscale_int and double.
  Never compiled yet.

  TODO:
  - Do the proper thing when the user changes value in entry,
  so that callback should not be called when value is actually not changed.
  - Update delay
 */

/* $Id$ */


static void   entscale_destroy_callback (GtkWidget *widget,
					 gpointer data);
static void   entscale_scale_update (GtkAdjustment *adjustment,
				     gpointer	   data);
static void   entscale_entry_update (GtkWidget *widget,
				     gpointer	data);
/*
 *  Create an entry, a scale and a label, then attach them to
 *  specified table.
 *  1 row and 2 cols of table are needed.
 *
 *  Input:
 *    table:	  table which entscale is attached to
 *    x, y:	  starting row and col in table
 *    caption:	  label string
 *    type:	  type of variable (ENTSCALE_INT or ENTSCALE_DOUBLE)
 *    variable:	  pointer to variable
 *    min, max:	  boundary of scale
 *    step:	  step of scale (ignored when (type == ENTSCALE_INT))
 *    constraint: (bool) true iff the value of *variable should be
 *		  constraint by min and max
 *    callback:	  called when the value is actually changed
 *		  (*variable is automatically changed, so there's no
 *		  need of callback func usually.)
 *    call_data:  data for callback func
 */

void
entscale_new (GtkWidget *table, gint x, gint y, gchar *caption,
	      EntscaleType type, gpointer variable,
	      gdouble min, gdouble max, gdouble step,
	      gint constraint,
	      EntscaleCallbackFunc callback,
	      gpointer call_data)
{
  Entscale	*entscale;
  GtkWidget	*hbox;
  GtkWidget	*label;
  GtkWidget	*entry;
  GtkWidget	*scale;
  GtkObject	*adjustment;
  gchar		buffer[256];
  gdouble	val;
  gdouble	constraint_val;

  entscale = g_new0 (Entscale, 1 );
  entscale->type	= type;
  entscale->constraint	= constraint;
  entscale->callback	= callback;
  entscale->call_data	= call_data;

  switch (type)
    {
    case ENTSCALE_INT:
      step = 1.0;
      strcpy (entscale->fmt_string, "%.0f");
      val = *(gint *)variable;
      break;
    case ENTSCALE_DOUBLE:
      sprintf (entscale->fmt_string, "%%.%df", entscale_get_precision (step) + 1);
      val = *(gdouble *)variable;
      break;
    default:
      g_error ("TYPE must be either ENTSCALE_INT or ENTSCALE_DOUBLE");
      val = 0;
      break;
    }

  label = gtk_label_new (caption);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  /*
    If the first arg of gtk_adjustment_new() isn't between min and
    max, it is automatically corrected by gtk later with
    "value_changed" signal. I don't like this, since I want to leave
    *variable untouched when `constraint' is false.
    The lines below might look oppositely, but this is OK.
   */
  if (constraint)
    constraint_val = val;
  else
    constraint_val = BOUNDS (val, min, max);

  adjustment = entscale->adjustment =
    gtk_adjustment_new (constraint_val, min, max, step, step, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_widget_set_usize (scale, ENTSCALE_SCALE_WIDTH, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  entry = entscale->entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTSCALE_ENTRY_WIDTH, 0);
  sprintf (buffer, entscale->fmt_string, val);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);

  /* entscale is done */
  gtk_object_set_user_data (GTK_OBJECT(adjustment), entscale);
  gtk_object_set_user_data (GTK_OBJECT(entry), entscale);

  /* now ready for signals */
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entscale_entry_update,
		      variable);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) entscale_scale_update,
		      variable);
  gtk_signal_connect (GTK_OBJECT (entry), "destroy",
		      (GtkSignalFunc) entscale_destroy_callback,
		      NULL );

  /* start packing */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), label, x, x+1, y, y+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (label);
  gtk_widget_show (entry);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);
}

static int
entscale_get_precision (gdouble step)
{
  int precision;
  if (step <= 0)
    return 0;
  for (precision = 0; pow (0.1, precision) > step; precision++) ;
  return precision;
}

static void
entscale_destroy_callback (GtkWidget *widget,
			   gpointer data)
{
  Entscale *entscale;

  entscale = gtk_object_get_user_data (GTK_OBJECT (widget));
  g_free (entscale);
}
static void
entscale_scale_update (GtkAdjustment *adjustment,
		       gpointer	     data)
{
  Entscale	*entscale;
  GtkEntry	*entry;
  gchar		buffer[256];
  gdouble	old_val, new_val;

  entscale = gtk_object_get_user_data (GTK_OBJECT (adjustment));

  new_val = adjustment->value;
  /* adjustmet->value is always constrainted */

  switch (entscale->type)
    {
    case ENTSCALE_INT:
      old_val = *(gint*) data;
      *(gint*) data = (gint) new_val;
      DEBUG_PRINT (("entscale_scale_update(int): fmt=\"%s\" old=%g new=%g\n",
		    entscale->fmt_string, old_val, new_val));
      break;
    case ENTSCALE_DOUBLE:
      old_val = *(gdouble*) data;
      *(gdouble*) data = new_val;
      DEBUG_PRINT (("entscale_scale_update(double): fmt=\"%s\" old=%g new=%g\n",
		    entscale->fmt_string, old_val, new_val));
      break;
    default:
      g_warning ("entscale_scale_update: invalid type");
      return;
    }

  entry = GTK_ENTRY (entscale->entry);
  sprintf (buffer, entscale->fmt_string, new_val);

  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data (GTK_OBJECT(entry), data);
  gtk_entry_set_text (entry, buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT(entry), data);

  if (entscale->callback && (old_val != new_val))
    (*entscale->callback) (new_val, entscale->call_data);
}

static void
entscale_entry_update (GtkWidget *widget,
		       gpointer	  data)
{
  Entscale	*entscale;
  GtkAdjustment *adjustment;
  gdouble	old_val, val, new_val, constraint_val;

  entscale = gtk_object_get_user_data (GTK_OBJECT (widget));
  adjustment = GTK_ADJUSTMENT (entscale->adjustment);

  val = atof (gtk_entry_get_text (GTK_ENTRY (widget)));

  constraint_val = BOUNDS (val, adjustment->lower, adjustment->upper);

  if (entscale->constraint)
    new_val = constraint_val;
  else
    new_val = val;

  switch (entscale->type)
    {
    case ENTSCALE_INT:
      old_val = *(gint*) data;
      *(gint*) data = (gint) new_val;
      DEBUG_PRINT (("entscale_entry_update(int): old=%g new=%g const=%g\n",
		    old_val, new_val, constraint_val));
      break;
    case ENTSCALE_DOUBLE:
      old_val = *(gdouble*) data;
      *(gdouble*) data = new_val;
      DEBUG_PRINT (("entscale_entry_update(double): old=%g new=%g const=%g\n",
		    old_val, new_val, constraint_val));
      break;
    default:
      g_warning ("entscale_entry_update: invalid type");
      return;
    }

  adjustment->value = constraint_val;
  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data (GTK_OBJECT(adjustment), data );
  gtk_signal_emit_by_name (GTK_OBJECT(adjustment), "value_changed");
  gtk_signal_handler_unblock_by_data (GTK_OBJECT(adjustment), data );

  if (entscale->callback && (old_val != new_val))
    (*entscale->callback) (new_val, entscale->call_data);
}

/*************************************************************************/
/**									**/
/**			+++ Miscellaneous				**/
/**									**/
/*************************************************************************/

/*
  These query box functions are yanked from app/interface.c.	taka
 */

/*
 *  A text string query box
 */

typedef struct _QueryBox QueryBox;

struct _QueryBox
{
  GtkWidget *qbox;
  GtkWidget *entry;
  QueryFunc callback;
  gpointer data;
};

static void query_box_cancel_callback (GtkWidget *, gpointer);
static void query_box_ok_callback (GtkWidget *, gpointer);

GtkWidget *
query_string_box (char	      *title,
		  char	      *message,
		  char	      *initial,
		  QueryFunc    callback,
		  gpointer     data)
{
  QueryBox  *query_box;
  GtkWidget *qbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;

  query_box = (QueryBox *) g_malloc (sizeof (QueryBox));

  qbox = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (qbox), title);
  gtk_window_set_position (GTK_WINDOW (qbox), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (qbox)->action_area), 2);

  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) query_box_ok_callback,
		      query_box);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (qbox)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) query_box_cancel_callback,
		      query_box);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (qbox)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (qbox)->vbox), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
  if (initial)
    gtk_entry_set_text (GTK_ENTRY (entry), initial);
  gtk_widget_show (entry);

  query_box->qbox = qbox;
  query_box->entry = entry;
  query_box->callback = callback;
  query_box->data = data;

  gtk_widget_show (qbox);

  return qbox;
}

static void
query_box_cancel_callback (GtkWidget *w,
			   gpointer   client_data)
{
  QueryBox *query_box;

  query_box = (QueryBox *) client_data;

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
query_box_ok_callback (GtkWidget *w,
		       gpointer	  client_data)
{
  QueryBox *query_box;
  char *string;

  query_box = (QueryBox *) client_data;

  /*  Get the entry data  */
  string = g_strdup (gtk_entry_get_text (GTK_ENTRY (query_box->entry)));

  /*  Call the user defined callback  */
  (* query_box->callback) (w, query_box->data, (gpointer) string);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}
