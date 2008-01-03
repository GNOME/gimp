/*
 * This is a plugin for the GIMP.
 *
 * Copyright (C) 1996 Stephen Norris
 * Copyright (C) 2001-2002,2004 Yeti (David Necas)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/*
 * This plug-in produces plasma fractal images. The algorithm is losely
 * based on a description of the fractint algorithm, but completely
 * re-implemented because the fractint code was too ugly to read :)
 *
 * Please send any patches or suggestions to me: srn@flibble.cs.su.oz.au.
 */

/* Version 1.01 */

/*
 * Ported to GIMP Plug-in API 1.0
 *    by Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 *
 * $Id: plasma2.c,v 1.8 2004/01/05 22:26:12 yeti Exp $
 *
 * A few functions names and their order are changed :)
 * Plasma implementation almost hasn't been changed.
 *
 * Feel free to correct my WRONG English, or to modify Plug-in Path,
 * and so on. ;-)
 *
 * Version 1.02
 *
 * May 2000
 * tim copperfield [timecop@japan.co.jp]
 * Added dynamic preview mode.
 *
 */

/*
   Version 2.0

   Mar 2001
   Yeti (David Necas) <yeti@physics.muni.cz>
   Branched from plasma.c (not much code reused).
   Note: The number of parameters changed thus it is not possible to simply
   replace the old Plasma with this one.
   Improvements:
   - Rewrote the engine: staightforward algorithm which appeared in Fractint 19
     is used (more precisely, my implemetation of this approach from 1997).
   - Added horizontal/vertical tileability options.
   - Added selfsimilar (exponential, fractal, etc.) algorithm option (so it
     can really generate the thing known as `the plasma fractal').
   - Some effort put into invariant behaviour (compare e.g. tileable and
     non-tileable (or selfsimilar and non-selfsimilar) images generated from the
     same seed).
   - Speed optimizations.
   Bugfixes:
   - Fixed preview not showing final image when seed is time.
   - Fixed using two-parametric plasma() as one-parametric signal callback.
   - Only really needed channels are generated (3x faster on grayscale).
   - Progress bar really shows progress (the old one was cheating).
   - Preview correctly shows grayscale image in grayscale mode.
   Bugs:
   - This plugin now uses  xsize x ysize  bytes of memory and no tile cache.
     Ban me, kill me, let me administrate MS-Windows(TM), but I will still
     maintain the point it does need random access to the whole selection, so
     I don't pretend it doesn't, and even if in some very unfortunate cases
     the extra memory allocation can really be a problem, it normally makes
     the plug-in run considerably faster (note in usual RGBA case, we allocate
     only 1/4 of layer size).

*/
/*
   Version 2.1

   Sep 2001
   Yeti <yeti@physics.muni.cz>
   Improvements:
   - Generator and scaling methods
   - Buttons to reset/revert settings
   - Possibility to directly map to a gradient (much more smoothly since it's
     done when values are still floats)
   - Grayscale can be generated even in RGB drawable
   - New dialog layout
   - It's faster
   Bug fixes:
   - More standard widget spacing
   - Escape exits the dialog
   Bugs:
   - Generator uses gfloat for intermediate results (eating even more
     memory...)
   - It shows -1 as random seed when seed is time (FIXME: how to fix it
     keeping the otherwise nice and consistent seed behaviour?)
   - Cannot handle gradients whith names longer than GRADIENT_NAME_SIZE
     (this is a GIMP problem)
   - Cannot reset/revert gradient (gimp exports no function for this and doing
     it by hand is obscure and error prone)
     (this is a GIMP problem)
*/

/*
   Version 2.2

   Sep 2001
   Yeti <yeti@physics.muni.cz>
   Internal changes:
   - Simplified comitting, now uses gimp_pixel_rgns_...() functions instead of
     handling the regions by hand (still needs some cleanup)
   - Uses it's own accelerator group instead of default
   Bug fixes:
   - Fixed checker array overflow in plasma2_add_transparency()
*/

/*
   Version 2.3

   Oct 2002
   Yeti <yeti@physics.muni.cz>
   Ported to Gimp-1.3/Gtk-2.0:
   - Renamed and retyped stuff as necessary
   - Created a brand new preview widget using GtkImage & GdkPixbuf
   - Stockized stock items and reordered buttons to Gtk-2.0 order.
     Can't use GIMP_STOCK_RESET, becuase its label associates `Defaults' while
     its image `Revert'
   - Added accels to labels.  FIXME: Ugly Gimp random seed doesn't work
   - Removed Esc-exits accel, since Gtk-2.0 does it itself
*/

/*
   Version 2.4

   Jan 2003
   Yeti <yeti@physics.muni.cz>
   Ported to use the new Gimp random seed widget (great!).
   Bug fixes:
   - tilable -> tileable
*/

/*
   Version 2.5

   Oct 2003
   Yeti <yeti@physics.muni.cz>
   - works with gimp-1.3.21 and newer
*/

/* Version 2.6

   Jan 2004
   Yeti <yeti@physics.muni.cz>
   - changed dialog construction and main event loop to work with gimp-1.3.23
   - some code cleanup
*/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>  /* For random seeding */

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/* Some debugging macros */
#ifdef __GNUC__
# define PVAR(f, v) fprintf(stderr, __FILE__ ":%u %s(): " \
                            #v " == %" #f "\n", __LINE__, __FUNCTION__, v)
# define PARR(f, v, n) ( { int _i; \
  fprintf(stderr, __FILE__ ":%u %s(): " #v " == { ", __LINE__, __FUNCTION__); \
  for (_i = 0; _i < n; _i++) fprintf(stderr, "%" #f ", ", (v)[_i]); \
  fputs("}\n", stderr); \
} )
#else /* __GNUC__ */
/* FIXME */
#endif /* __GNUC__ */

#define PLUG_IN_PROC     "plug-in-plasma2"
#define PLUG_IN_BINARY   "plasma2"

#define DATA_KEY_VALS    "plug-in-plasma2"
#define DATA_KEY_UI_VALS "plug-in-plasma2-ui"

#define PARASITE_KEY     "plug-in-plasma2-options"

/* we are not 100% 16-bit-ready, but partially yes (almost everything is done
   on floats anyway) */
#define CHANNEL_MAX_VALUE     255
#define BITS_PER_SAMPLE         8

#define ENTRY_WIDTH            75
#define SCALE_WIDTH           128
#define PREVIEW_SIZE          128
#define CELL_SIZE_WIDTH        84

/* XXX: we cannot store true strings as parameters, hope this is enough */
#define GRADIENT_NAME_SIZE    256

/* generally, we need much more than 256
   note we do NOT do our own subsampling, just rounding */
#define GRADIENT_SAMPLE_SIZE  8192

/* just some small but definitely nonzero number (it should be several orders
   larger than machine epsilon, at least) */
#define EPS 0.0000001

/* FIXME: this is OK with glibc, but are other generators good enough? */
#define RND() ((double)rand()/RAND_MAX)

enum {
  PLASMA2_RESPONSE_RESET = 1,
  PLASMA2_RESPONSE_DEFAULTS
};

typedef struct {
  guint32  seed;
  gint     htil;
  gint     vtil;
  gint     rdt;
  gint     fsf;
  gdouble  lfwhm;
  gdouble  a;
  gint     cm;
  gchar    grad[GRADIENT_NAME_SIZE];
} PlasmaValues;

/* local prototypes */
static void query                    (void);
static void run                      (const gchar              *name,
                                      gint                      nparams,
                                      const GimpParam          *param,
                                      gint                     *nreturn_vals,
                                      GimpParam               **return_vals);

static gint plasma2_dialog           (void);
static void plasma2_refresh_controls (const PlasmaValues       *new_pvals);
static void plasma2_rdt_changed      (GimpIntComboBox          *combo,
                                      gpointer                  data);
static void plasma2_fsf_changed      (GimpIntComboBox          *combo,
                                      gpointer                  data);
static void plasma2_cm_changed       (GimpIntComboBox          *combo,
                                      gpointer                  data);
static void plasma2_gradient_changed (GimpGradientSelectButton *button,
                                      gchar                    *name,
                                      gint                      width,
                                      gpointer                  grad_data,
                                      gboolean                  dialog_closing,
                                      gpointer                  user_data);

static void plasma2                  (GimpDrawable             *drawable);
static void init_plasma2             (GimpDrawable             *drawable);
static void generate_plasma_channel  (gint                      xs,
                                      gint                      ys,
                                      const gint                channel);
static void commit                   (GimpDrawable             *drawable,
                                      const gint                channel);
static void commit_one_tile          (GimpPixelRgn             *rgn,
                                      const gint                channel,
                                      const gint                gradwidth);
static void plasma2_add_transparency (const gint                channel);
static void end_plasma2              (GimpDrawable             *drawable);

/***** Local vars *****/

typedef struct
{
  GtkWidget *seed;
  GtkWidget *htil;
  GtkWidget *vtil;
  GtkWidget *rdt;
  GtkWidget *fsf;
  GtkObject *lfwhm;
  GtkObject *a;
  GtkWidget *cm;
  GtkWidget *grad;
  GtkWidget *grad_label; /* for `inactivating' the label */
} PlasmaControls;

static PlasmaControls pctrl;

typedef enum
{
  PLASMA2_RDT_UNIFORM = 0,
  PLASMA2_RDT_CAUCHY  = 1,
  PLASMA2_RDT_EXP     = 2,
} PlasmaRandomDistributionType;

typedef enum
{
  PLASMA2_FSF_EXP   = 0,
  PLASMA2_FSF_POWER = 1,
} PlasmaFWHMScaleFunction;

typedef enum
{
  PLASMA2_CM_INDRGB   = 0,
  PLASMA2_CM_GRAY     = 1,
  PLASMA2_CM_GRADIENT = 2,
} PlasmaColoringMethod;

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static const PlasmaValues pvals_defaults =
{
  11,                  /* seed */
  FALSE,               /* htil */
  FALSE,               /* vtil */
  PLASMA2_RDT_UNIFORM, /* rdt */
  PLASMA2_FSF_EXP,     /* fsf */
  0.0,                 /* lfwhm */
  1.0,                 /* a */
  PLASMA2_CM_INDRGB,   /* cm */
  "",                  /* grad */
};

static PlasmaValues pvals;     /* = pvals_defaults; */
static PlasmaValues pvals_old; /* for revert */

static GtkWidget    *preview_image    = NULL;
static GdkPixbuf    *preview          = NULL;
static gboolean      preview_mode;
static gboolean      is_rgb;
static GimpDrawable *drawable;
static gdouble      *gradient         = NULL;
static gint          gradient_p_width = -1;

/*
 * Some globals to save passing too many paramaters that don't change.
 */

static gint       ix1, iy1, ix2, iy2;     /* Selected image size. */
static gint       bpp, has_alpha, alpha;
static glong      max_progress, progress;
static gfloat   **plasma; /* plasma channel buffer */
static gdouble    fwhm, randomness; /* true and recomputed fwhm */
static gdouble    xscale, yscale; /* scale (for noise amplitude) */

/***** Functions *****/

MAIN()

static void
query(void)
{
  static GimpParamDef args[]=
    {
      { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
      { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
      { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
      { GIMP_PDB_INT32, "seed", "Random seed" },
      { GIMP_PDB_INT32, "htil", "Horizontal tileability" },
      { GIMP_PDB_INT32, "vtil", "Vertical tileability" },
      { GIMP_PDB_INT32, "rdt",
        "Noise distribution type: { UNIFORM (0), CAUCHY (2), EXPONENTIAL (2) }" },
      { GIMP_PDB_INT32, "fsf",
        "Noise scaling function: { FRACTINT_LIKE (0), NORRIS_LIKE (1) }" },
      { GIMP_PDB_FLOAT, "lfwhm", "Noise FWHM logarithm" },
      { GIMP_PDB_FLOAT, "a", "Scaling parameter a" },
      { GIMP_PDB_INT32, "cm",
        "Coloring method: { INDEPENDENT_RGB (0), GRAYSCALE (1), GRADIENT (2) }" },
      { GIMP_PDB_STRING, "grad", "Gradient used for coloring (when cm is GRADIENT)" },
    };

  gimp_install_procedure("plug_in_plasma2",
                         "Create a plasma cloud like image to the specified drawable",
                         "This plugin creates several types of cloud like images, "
                         "mostly variations on the famous `plasma fractal'.",
                         "Yeti & authors of original Plasma: "
                         "Stephen Norris & (ported to 1.0 by) Eiichi Takamori",
                         "Stephen Norris & Yeti",
                         "Nov 2002",
                         N_("Plasma..."),
                         "RGB*, GRAY*",
                         GIMP_PLUGIN,
                         G_N_ELEMENTS(args), 0,
                         args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Render/Clouds");
}

static void
run (const gchar     *name,
     gint             nparams,
     const GimpParam *param,
     gint            *nreturn_vals,
     GimpParam      **return_vals)
{
  static GimpParam  values[1];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  is_rgb = gimp_drawable_is_rgb (drawable->drawable_id);

  pvals = pvals_defaults;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (DATA_KEY_VALS, &pvals);

      /*  First acquire information with a dialog  */
      pvals_old = pvals;

      if (! plasma2_dialog ())
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 12)
        status = GIMP_PDB_CALLING_ERROR;
      else
        {
          pvals.seed  = (gint) param[3].data.d_int32;
          pvals.htil  = (gint) param[4].data.d_int32;
          pvals.vtil  = (gint) param[5].data.d_int32;
          pvals.rdt   = (PlasmaRandomDistributionType) param[6].data.d_int32;
          pvals.fsf   = (PlasmaFWHMScaleFunction) param[7].data.d_int32;
          pvals.lfwhm = (gdouble) param[8].data.d_float;
          pvals.a     = (gdouble) param[9].data.d_float;
          pvals.cm    = (PlasmaColoringMethod) param[10].data.d_int32;
          strncpy (pvals.grad, (gchar*) param[11].data.d_string,
                   GRADIENT_NAME_SIZE - 1);
          pvals.grad[GRADIENT_NAME_SIZE - 1] = '\0';
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (DATA_KEY_VALS, &pvals);
      break;

    default:
      break;
  }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (is_rgb || gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_progress_init (_("Plasma2..."));

          preview_mode = FALSE;
          plasma2 (drawable);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE
              || run_mode == GIMP_RUN_WITH_LAST_VALS)
            gimp_set_data (DATA_KEY_VALS, &pvals, sizeof (PlasmaValues));
        }
      else status = GIMP_PDB_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;
  gimp_drawable_detach (drawable);
}

/* remove some source code redundancies {{{ */
static inline void
yeti_table_attach_to_row (GtkWidget  *table,
                          GtkWidget  *child,
                          const gint  row)
{
  gtk_table_attach (GTK_TABLE (table), child, 0, GTK_TABLE (table)->ncols,
                   row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
}

static inline void
yeti_table_attach_single_cell (GtkWidget  *table,
                               GtkWidget  *child,
                               const gint  column,
                               const gint  row)
{
  gtk_table_attach (GTK_TABLE (table), child, column, column + 1, row, row + 1,
                   GTK_EXPAND | GTK_FILL, 0, 0, 0);
}

static inline GtkWidget*
yeti_label_new_in_table (const gchar    *name,
                         GtkWidget      *table,
                         const gint      row,
                         const gboolean  sensitive)
{
  GtkWidget *label;

  label = gtk_label_new_with_mnemonic (name);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  yeti_table_attach_single_cell (table, label, 0, row);
  gtk_widget_set_sensitive (label, sensitive);
  gtk_widget_show (label);

  return label;
}

static inline GtkWidget*
yeti_frame_new_in_box (const gchar    *name,
                       GtkWidget      *box,
                       const gboolean  expand,
                       const gboolean  fill)
{
  GtkWidget *frame;

  frame = gtk_frame_new (name);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (box), frame, expand, fill, 0);
  gtk_widget_show (frame);

  return frame;
}

static inline GtkWidget*
yeti_table_new_in_frame (const gint  cols,
                         const gint  rows,
                         GtkWidget  *frame)
{
  GtkWidget *table;
  GtkWidget *align;

  align = gtk_alignment_new (0.5, 0.0, 1.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (align), 0);
  gtk_container_add (GTK_CONTAINER (frame), align);
  gtk_widget_show (align);

  table = gtk_table_new (cols, rows, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (align), table);
  gtk_widget_show (table);

  return table;
}

static inline GtkWidget*
yeti_check_button_new_with_label (const gchar *name,
                                  gint        *value,
                                  GCallback    cb,
                                  gpointer     data)
{
  GtkWidget *check;

  check = gtk_check_button_new_with_mnemonic (name);
  g_signal_connect (check, "toggled",
                   G_CALLBACK (gimp_toggle_button_update), value);
  g_signal_connect_swapped (check, "toggled", cb, data);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), *value);
  gtk_widget_show (check);

  return check;
}

static inline GtkWidget*
yeti_preview_frame_new_in_box (GtkWidget *hbox)
{
  GtkWidget *frame;
  GtkWidget *align;
  GtkWidget *image;
  GdkPixbuf *pixbuf;

  frame = yeti_frame_new_in_box (_("Preview"), hbox, FALSE, FALSE);

  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (align), 4);
  gtk_container_add (GTK_CONTAINER (frame), align);
  gtk_widget_show (align);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (align), frame);
  gtk_widget_show (frame);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                          has_alpha,
                          BITS_PER_SAMPLE,
                          PREVIEW_SIZE,
                          PREVIEW_SIZE);
  gdk_pixbuf_fill (pixbuf, 0);

  image = gtk_image_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);
  gtk_container_add (GTK_CONTAINER (frame), image);
  gtk_widget_show (image);

  return frame;
}

static inline GtkObject*
yeti_scale_entry_new_double (const gchar   *name,
                             GtkWidget     *table,
                             const gint     row,
                             gdouble       *value,
                             const gdouble  min,
                             const gdouble  max,
                             const gdouble  step,
                             const gdouble  page,
                             const gint     digits,
                             GCallback      cb,
                             gpointer       data)
{
  GtkObject *adj;

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row, name, SCALE_WIDTH, 0,
                             *value, min, max, step, page, digits,
                             TRUE, 0, 0, NULL, NULL);
  g_signal_connect (adj, "value_changed",
                   G_CALLBACK (gimp_double_adjustment_update),
                   value);
  g_signal_connect_swapped (adj, "value_changed", cb, data);

  return adj;
}

static inline void
yeti_progress_update (gint p,
                      gint max)
{
  gdouble r;

  r = (gdouble) p / max;
  gimp_progress_update (r);
}
/* }}} */

static gint
plasma2_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *table;
  gboolean   run;
  gboolean   init;

  preview_mode = TRUE;
  gimp_ui_init ("plasma2", TRUE);

  dlg = gimp_dialog_new (_("Plasma2"), "plasma2",
                        NULL, 0,
                        gimp_standard_help_func, "filters/plasma2.html",

                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                        _("Defaults"), PLASMA2_RESPONSE_DEFAULTS,
                        _("Revert"), PLASMA2_RESPONSE_RESET,
                        GTK_STOCK_OK, GTK_RESPONSE_OK,
                        NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /* top part with preview and common options */
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* make a nice preview frame */
  frame = yeti_preview_frame_new_in_box (hbox);
  preview_image = GTK_BIN (frame)->child;
  preview = gtk_image_get_pixbuf (GTK_IMAGE (preview_image));

  /* graphic options */
  frame = yeti_frame_new_in_box (_("Graphic options"), hbox, TRUE, TRUE);
  table = yeti_table_new_in_frame (3, 4, frame);

  /* horizontal tileability */
  pctrl.htil = yeti_check_button_new_with_label (_("_Horizontally tileable"),
                                                &pvals.htil,
                                                G_CALLBACK (plasma2),
                                                (gpointer)drawable);
  yeti_table_attach_to_row (table, pctrl.htil, 0);

  /* vertical tileability */
  pctrl.vtil = yeti_check_button_new_with_label (_("_Vertically tileable"),
                                                &pvals.vtil,
                                                G_CALLBACK (plasma2),
                                                (gpointer)drawable);
  yeti_table_attach_to_row (table, pctrl.vtil, 1);

  /* gradient, only if rgb */
  if (!is_rgb)
    pvals.cm = PLASMA2_CM_GRAY;
  /* synchronize pvals.grad with current gradient */
  if (!strlen (pvals.grad) || !gimp_context_set_gradient (pvals.grad))
    {
      strncpy (pvals.grad, gimp_context_get_gradient (), GRADIENT_NAME_SIZE-1);
      pvals.grad[GRADIENT_NAME_SIZE-1] = '\0';
    }

  pctrl.grad = gimp_gradient_select_button_new ("Plasma2 Gradient",
                                                pvals.grad);
  g_signal_connect (pctrl.grad, "gradient-set",
                    (GCallback) plasma2_gradient_changed, NULL);

  yeti_table_attach_single_cell (table, pctrl.grad, 1, 3);
  gtk_widget_set_sensitive (pctrl.grad, pvals.cm == PLASMA2_CM_GRADIENT);

  pctrl.grad_label = yeti_label_new_in_table (_("_Gradient:"), table, 3,
                                             pvals.cm == PLASMA2_CM_GRADIENT);
  gtk_label_set_mnemonic_widget (GTK_LABEL (pctrl.grad_label), pctrl.grad);

  /* coloring function */
  pctrl.cm = gimp_int_combo_box_new (_("Independent RGB"), PLASMA2_CM_INDRGB,
                                     _("Grayscale"),       PLASMA2_CM_GRAY,
                                     _("Gradient"),        PLASMA2_CM_GRADIENT,
                                     NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (pctrl.cm),
                              pvals.cm,
                              (GCallback) plasma2_cm_changed,
                              NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                            _("Coloring _method:"), 1.0, 0.5,
                            pctrl.cm, 2, TRUE);
  if (! is_rgb)
    gtk_widget_set_sensitive (pctrl.cm, FALSE);

  /* generator settings  */
  frame = yeti_frame_new_in_box (_("Generator settings"), main_vbox, TRUE, TRUE);
  table = yeti_table_new_in_frame (3, 5, frame);

  /* seed/time */
  init = TRUE;
  pctrl.seed = gimp_random_seed_new (&pvals.seed, &init);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                            _("Random Seed:"), 1.0, 0.5,
                            pctrl.seed, 1, TRUE);
  g_signal_connect_swapped (GIMP_RANDOM_SEED_SPINBUTTON_ADJ (pctrl.seed),
                           "value_changed",
                           G_CALLBACK (plasma2),
                           (gpointer)drawable);

  /* random distribution type */
  pctrl.rdt = gimp_int_combo_box_new (_("Uniform"),     PLASMA2_RDT_UNIFORM,
                                      _("Cauchy"),      PLASMA2_RDT_CAUCHY,
                                      _("Exponential"), PLASMA2_RDT_EXP,
                                      NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (pctrl.rdt),
                              pvals.rdt,
                              (GCallback) plasma2_rdt_changed,
                              NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                            _("Noise _distribution:"), 1.0, 0.5,
                            pctrl.rdt, 2, TRUE);

  /* fwhm */
  pctrl.lfwhm = yeti_scale_entry_new_double (_("Noise _amplitude (log):"),
                                            table, 2, &pvals.lfwhm,
                                            -7.0, 5.0, 0.01, 0.1, 2,
                                            G_CALLBACK (plasma2),
                                            (gpointer)drawable);

  /* fwhm scale function */
  pctrl.fsf = gimp_int_combo_box_new (_("Fractint-like,  2^(-a*depth)"),
                                      PLASMA2_FSF_EXP,
                                      _("Norris-like,  1/depth^a"),
                                      PLASMA2_FSF_POWER,
                                      NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (pctrl.fsf),
                              pvals.fsf,
                              (GCallback) plasma2_fsf_changed,
                              NULL);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
                            _("_Scaling function:"), 1.0, 0.5,
                            pctrl.fsf, 2, TRUE);

  /* parameter a */
  pctrl.a = yeti_scale_entry_new_double (_("Scaling _parameter a:"),
                                        table, 4, &pvals.a,
                                        0.0, 3.0, 0.01, 0.1, 2,
                                        G_CALLBACK (plasma2),
                                        (gpointer)drawable);

  gtk_widget_show_all (dlg);
  plasma2 (drawable); /* preview image */

  do {
    run = gimp_dialog_run (GIMP_DIALOG (dlg));
    if (run == PLASMA2_RESPONSE_RESET)
      plasma2_refresh_controls (&pvals_old);
    else if (run == PLASMA2_RESPONSE_DEFAULTS)
      plasma2_refresh_controls (&pvals_defaults);
    else
      break;
  } while (TRUE);

  gtk_widget_destroy (dlg);

  return run == GTK_RESPONSE_OK;
}

/*
 * The setup function.
 */

static void
plasma2 (GimpDrawable *drawable)
{
  gint i, chmax;

  init_plasma2 (drawable);
  chmax = bpp;

  for (i = 0; i < chmax; i++)
    {
      /* yet more voodoo.  to throw away the duplicate pixels so they don't
         appear on both edges in tileable images, we always generate one pixel
         larger image but pretend it has the right size */
      if (i == 0 || pvals.cm == PLASMA2_CM_INDRGB)
        generate_plasma_channel (ix2-ix1+1, iy2-iy1+1, i);

      commit (drawable, i);
    }

  if (has_alpha && preview_mode && pvals.cm == PLASMA2_CM_GRADIENT)
    {
      for (i = 0; i < chmax; i++)
        plasma2_add_transparency (i);
    }

  end_plasma2 (drawable);
}

static void
init_plasma2 (GimpDrawable *drawable)
{
  gint x;

  /* don't use time for seed when rendering preview in full size to get the
     same image as shown in preview */

  /* map lfwhm to true FWHM */
  fwhm = exp (pvals.lfwhm);

  /* compute sizes and other stuff of this kind */
  alpha = -1000; /* to cause a segfault ;-> */
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  if (preview_mode)
    {
      xscale = 1.0 / PREVIEW_SIZE;
      yscale = 1.0 / PREVIEW_SIZE;
      ix1 = iy1 = 0;
      ix2 = gdk_pixbuf_get_width (preview);
      iy2 = gdk_pixbuf_get_height (preview);
      bpp = gdk_pixbuf_get_n_channels (preview);
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &ix1, &iy1, &ix2, &iy2);
      xscale = 1.0 / (ix2 - ix1);
      yscale = 1.0 / (iy2 - iy1);
      bpp = gimp_drawable_bpp (drawable->drawable_id);
      if (has_alpha)
        alpha = bpp - 1;
  }

  plasma = g_new (gfloat*, ix2-ix1+1);
  for (x = 0; x < ix2 - ix1 + 1; x++)
    plasma[x] = g_new (gfloat, iy2 - iy1 + 1);

  max_progress = (ix2 - ix1 + 1) * (iy2 - iy1 + 1);
  max_progress *= bpp + (pvals.cm == PLASMA2_CM_INDRGB ? bpp : 1);
  max_progress += (has_alpha ? (ix2 - ix1) * (iy2 - iy1) : 0);
  progress = 0;

  /* generate gradient, this is the right place to do it */
  if (pvals.cm == PLASMA2_CM_GRADIENT)
    {
      if (preview_mode)
        {
          /* preview, done many times, don't regenerate what we already have */
          if (gradient == NULL)
            {
              gint num_color_samples;

              gradient_p_width = CELL_SIZE_WIDTH;
              gimp_gradient_get_uniform_samples   (gimp_context_get_gradient (),
                                                   gradient_p_width,
                                                   FALSE,
                                                   &num_color_samples,
                                                   &gradient);
              /*
               * above call replaces:

              gradient = gimp_gradients_sample_uniform (gradient_p_width, FALSE);

              *
              */
            }
        }
      else
        {
          gint num_color_samples;

          /* final rendering, done only once at the end */
          gimp_context_set_gradient (pvals.grad);
          g_free (gradient);
          gimp_gradient_get_uniform_samples   (gimp_context_get_gradient (),
                                               GRADIENT_SAMPLE_SIZE,
                                               FALSE,
                                               &num_color_samples,
                                               &gradient);
        }
    }
}

static void
end_plasma2 (GimpDrawable *drawable)
{
  gint x;

  if (preview_mode)
    gtk_widget_queue_draw (preview_image);
  else
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id,
                            ix1, iy1, (ix2 - ix1), (iy2 - iy1));
    }

  for (x = 0; x < ix2 - ix1 + 1; x++)
    g_free (plasma[x]);
  g_free (plasma);
}

/* move one-channel plasma data into preview or drawable */
static void
commit (GimpDrawable *drawable,
        const gint    channel)
{
  gpointer     pr;
  GimpPixelRgn dest_rgn;

  if (preview_mode)
    {
      dest_rgn.x = dest_rgn.y = 0;
      dest_rgn.w = dest_rgn.h = PREVIEW_SIZE;
      dest_rgn.bpp = gdk_pixbuf_get_n_channels (preview);
      dest_rgn.rowstride = gdk_pixbuf_get_rowstride (preview);
      dest_rgn.data = gdk_pixbuf_get_pixels (preview);
      commit_one_tile (&dest_rgn, channel, gradient_p_width);
    }
  else
    {
      gint k = 0;

      gimp_pixel_rgn_init (&dest_rgn, drawable,
                           ix1, iy1, (ix2 - ix1), (iy2 - iy1),
                           TRUE, TRUE);
      /* iterate by tiles */
      for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
           pr != NULL;
           pr = gimp_pixel_rgns_process (pr))
        {
          commit_one_tile (&dest_rgn, channel, GRADIENT_SAMPLE_SIZE);
          progress += dest_rgn.w * dest_rgn.h;
          if (k % 10 == 0)
            yeti_progress_update (progress, max_progress);
          k++;
        }

      gimp_progress_end ();
    }
}

/* move one-channel plasma data into one tile of drawable
   when channel == alpha, just fill it with CHANNEL_MAX_VALUE */
static void
commit_one_tile (GimpPixelRgn *rgn,
                 const gint    channel,
                 const gint    gradwidth)
{
  gint    x, y;
  gint    index;
  guchar *p;
  gfloat  r;

  /* XXX: hope compiler does its work good and moves invariant code out of the
     loops, otherwise this is slow */
  for (y = rgn->y; y < rgn->y + rgn->h; y++)
    {
      for (x = rgn->x; x < rgn->x + rgn->w; x++)
        {
          p = rgn->data + (y - rgn->y) * rgn->rowstride + (x - rgn->x) * rgn->bpp;
          if (pvals.cm == PLASMA2_CM_GRADIENT)
            {
              index = plasma[x - ix1][y - iy1] * (gradwidth - 1);
              r = gradient[4 * index + channel];
            }
          else
            {
              if (channel == alpha)
                r = 1.0;
              else
                r = plasma[x - ix1][y - iy1];
            }
          p[channel] = CHANNEL_MAX_VALUE*r;
        }
    }
}

static void
plasma2_add_transparency (const gint channel)
{
  static gint check_colors[] =
    {
      CHANNEL_MAX_VALUE*GIMP_CHECK_DARK,
      CHANNEL_MAX_VALUE*GIMP_CHECK_LIGHT,
    };

  guchar  *p;
  gint     x, y, i, chc;
  gdouble  f;

  for (x = ix1; x < ix2; x++)
    {
      for (y = iy1; y < iy2; y++)
        {
          i = plasma[x - ix1][y - iy1] * (gradient_p_width - 1);
          f = gradient[4 * i + 3];

          p = gdk_pixbuf_get_pixels (preview)
            + y * gdk_pixbuf_get_rowstride (preview) + x * bpp + channel;

          chc = (x - ix1) % (2 * GIMP_CHECK_SIZE) / GIMP_CHECK_SIZE
            + (y - iy1) % (2  *GIMP_CHECK_SIZE) / GIMP_CHECK_SIZE;

          *p = (*p) * f + (1 - f) * check_colors[chc % 2];
        }
    }
}

/* find start and end of block of zeros in flagfield (of size size), starting
   search from i1, the start and end indices are stored in i1 and i2
   returns FALSE when there are no more zeroes in flagfield, TRUE otherwise */
static inline gboolean
find_next_range (gint       *i1,
                 gint       *i2,
                 const gint *flagfield,
                 const gint  size)
{
  while (*i1 < size && flagfield[*i1])
    (*i1)++;
  if (*i1 == size)
    return FALSE;
  *i2 = (*i1)--;
  while (*i2 < size && ! flagfield[*i2])
    (*i2)++;

  return (*i2) < size;
}

/* math functions */

/* recompute fwhm to distribution-specific parameter */
static inline void
recompute_randomness (const gdouble scale,
                      const gint    w)
{
  static gdouble fwhm_to_rdparam_ratio[] =
    {
      1, 2, M_LN2,
    };

  switch (pvals.fsf)
    {
    case PLASMA2_FSF_EXP:
      randomness = pow (w * scale, pvals.a);
    break;

    case PLASMA2_FSF_POWER:
    randomness = pow (-log (w * scale) / M_LN2, -pvals.a);
    break;

    default:
    g_assert_not_reached ();
    break;
  }
  randomness *= fwhm / fwhm_to_rdparam_ratio[pvals.rdt];
}

/* uniform distribution */
static inline gdouble
rand_uniform (const gdouble a)
{
  return (-1.0 + 2.0*RND ()) * a;
}

/* random Cauchy numbers */
static inline gdouble
rand_cauchy (const gdouble a)
{
  gdouble r;

  do
    {
      r = RND ();
    }
  while (r == 0.5);

  return a * tan (M_PI * r);
}

/* random exponential numbers
   XXX: it returns obscure single-sided exponential distribution with mean
   value = 0 */
static inline gdouble
rand_exp (const gdouble mu)
{
  gdouble r;

  do
    {
      r = RND ();
    }
  while (r == 0.0);

  return -mu * log (r) - mu;
}

/* the generator */

/* compute and return mean value of v1 and v2 with some random shift, inverse
   proportional to depth */
static inline gdouble
average_with_noise (const gdouble v1,
                    const gdouble v2,
                    const gint    depth)
{
  double val;

  val = 0.5 * (v1 + v2);
  switch (pvals.rdt)
    {
    case PLASMA2_RDT_UNIFORM:
    val += rand_uniform (randomness);
    break;

    case PLASMA2_RDT_CAUCHY:
    val += rand_cauchy (randomness);
    break;

    case PLASMA2_RDT_EXP:
    val += rand_exp (randomness);
    break;

    default:
    g_assert_not_reached ();
    break;
  }

  if (val >= 1.0)
    return 1.0;
  if (val <= 0.0)
    return 0.0;
  return val;
}

/* one half of a recursion level, horizontal averaging */
static void
horizontal_pass (const gint  xs,
                 const gint  ys,
                 const gint  depth,
                 gint       *col_fill,
                 gint       *col_todo,
                 const gint *row_fill,
                 const gint  row_todo)
{
  gint x1, x2, xavg, y;

  for (x1 = 0; *col_todo > 0; (*col_todo)--)
    {
      if (!find_next_range (&x1, &x2, col_fill, xs))
        break;
      recompute_randomness (xscale, x2 - x1);

      xavg = (x1 + x2) / 2;
      col_fill[xavg] = 1;

      for (y = 0; y < ys; y++)
        {
          if (row_fill[y])
            {
              plasma[xavg][y] = average_with_noise (plasma[x1][y], plasma[x2][y],
                                                    depth);
            }
        }
      if (pvals.vtil)
        plasma[xavg][ys - 1] = plasma[xavg][0];
      x1 = x2;
      progress += ys - row_todo;
    }

  if (! preview_mode)
    yeti_progress_update (progress, max_progress);
}

/* one half of a recursion level, vertical averaging */
static void
vertical_pass (const gint  xs,
               const gint  ys,
               const gint  depth,
               gint       *row_fill,
               gint       *row_todo,
               const gint *col_fill,
               const gint  col_todo)
{
  gint y1, y2, yavg, x;

  for (y1 = 0; row_todo > 0; (*row_todo)--)
    {
      if (! find_next_range (&y1, &y2, row_fill, ys))
        break;
      recompute_randomness (yscale, y2-y1);
      yavg = (y1 + y2) / 2;
      row_fill[yavg] = 1;
      for (x = 0; x < xs; x++)
        {
          if (col_fill[x])
            {
              plasma[x][yavg] = average_with_noise (plasma[x][y1], plasma[x][y2],
                                                    depth);
            }
        }
      if (pvals.htil)
        plasma[xs - 1][yavg] = plasma[0][yavg];
      y1 = y2;
      progress += xs - col_todo;
    }
  if (!preview_mode)
    yeti_progress_update (progress, max_progress);
}

/* generate single-channel plasma of size (xs,ys), and put it into plasma
   channel is used to generate the right seed value for given channel */
static void
generate_plasma_channel (gint       xs,
                         gint       ys,
                         const gint channel)
{
  gint *row_fill, *col_fill; /* true when slot is filled already  */
  gint col_todo = 0;         /* number of empty cols in col_fill */
  gint row_todo = 0;         /* number of empty rows in row_fill */
  gint depth;                /* `recursion' depth */
  gint x, y;

  /* black magic.  we have to use consistent seed value for the preview and for
     the image for all channels and also consistent behaviour in noninteractive
     mode.  so we have to reset it for all channels, but obviously to different
     values. */
  srand ((pvals.seed + 83 * channel) % RAND_MAX);

  /* initialize corner pixels
     note we use the same amount of random numbers regardless of tileability */
  plasma[0][0] = RND ();

  plasma[xs - 1][0] = RND ();

  if (pvals.htil)
    plasma[xs - 1][0] = plasma[0][0];

  plasma[0][ys - 1] = RND ();

  if (pvals.vtil)
    plasma[0][ys - 1] = plasma[0][0];

  plasma[xs - 1][ys - 1] = RND ();
  if (pvals.htil)
    plasma[xs - 1][ys - 1] = plasma[0][ys - 1];
  else if (pvals.vtil)
    plasma[xs - 1][ys - 1] = plasma[xs - 1][0];

  /* initialize center and edge-center pixels */
  plasma[xs / 2][ys / 2] = RND ();
  plasma[xs / 2][0] = RND ();
  plasma[xs / 2][ys-1] = RND ();

  if (pvals.vtil)
    plasma[xs / 2][ys - 1] = plasma[xs / 2][0];

  plasma[0][ys / 2] = RND ();
  plasma[xs - 1][ys / 2] = RND ();
  if (pvals.htil)
    plasma[xs-1][ys / 2] = plasma[0][ys / 2];

  /* initialize used-slot table */
  col_fill = g_new0 (gint, xs);
  row_fill = g_new0 (gint, ys);
  col_fill[0] = col_fill[xs-1] = col_fill[xs/2] = 1;
  row_fill[0] = row_fill[ys-1] = row_fill[ys/2] = 1;
  for (x = 0; x < xs; x++)
    col_todo += 1 - col_fill[x];
  for (y = 0; y < ys; y++)
    row_todo += 1 - row_fill[y];

  /* iterate while there remains something to do */
  for (depth = 1; row_todo || col_todo; depth++)
    {
      /* running vertical and horizontal pass always in the same order would
         create image looking `tall' or `wide', so we alternate them */
      if (depth % 2)
        {
          vertical_pass (xs, ys, depth, row_fill, &row_todo, col_fill, col_todo);
          horizontal_pass (xs, ys, depth, col_fill, &col_todo, row_fill, row_todo);
        }
      else
        {
          horizontal_pass (xs, ys, depth, col_fill, &col_todo, row_fill, row_todo);
          vertical_pass (xs, ys, depth, row_fill, &row_todo, col_fill, col_todo);
        }
    }

  g_free (row_fill);
  g_free (col_fill);
}

/* option menu callbacks */
static void
plasma2_rdt_changed (GimpIntComboBox *combo,
                     gpointer         data)
{
  gint value;

  gimp_int_combo_box_get_active (combo, &value);

  if (pvals.rdt == value)
    return;

  pvals.rdt = value;

  plasma2 (drawable);
}

static void
plasma2_fsf_changed (GimpIntComboBox *combo,
                     gpointer         data)
{
  gint value;

  gimp_int_combo_box_get_active (combo, &value);

  if (pvals.fsf == value)
    return;

  pvals.fsf = value;

  plasma2 (drawable);
}

static void
plasma2_cm_changed (GimpIntComboBox *combo,
                    gpointer         data)
{
  gint value;

  gimp_int_combo_box_get_active (combo, &value);

  if (pvals.cm == value)
    return;

  if  (is_rgb)
    { /* this should be always true here, but... */
      gtk_widget_set_sensitive (pctrl.grad, value == PLASMA2_CM_GRADIENT);
      gtk_widget_set_sensitive (pctrl.grad_label, value == PLASMA2_CM_GRADIENT);
    }

  pvals.cm = value;

  plasma2 (drawable);
}

static void
plasma2_gradient_changed (GimpGradientSelectButton *button,
                          gchar                    *name,
                          gint                      width,
                          gpointer                  grad_data,
                          gboolean                  dialog_closing,
                          gpointer                  user_data)
{
  if (strncmp (pvals.grad, name, GRADIENT_NAME_SIZE - 1) == 0)
    return;

  gimp_context_set_gradient (name);

  strncpy (pvals.grad, name, GRADIENT_NAME_SIZE - 1);
  pvals.grad[GRADIENT_NAME_SIZE - 1] = '\0';

  gradient = g_realloc (gradient, width * sizeof (gdouble));
  memcpy (gradient, grad_data, width * sizeof (gdouble));
  gradient_p_width = width / 4;

  plasma2 (drawable);
}


/* though this is obscure, we use it to reset/revert all values _and_ refresh
   the dialog controls */
static void
plasma2_refresh_controls (const PlasmaValues *new_pvals)
{
  /* we cannot reset some field and don't want to reset some others, so copy
     them one by one  */
  pvals.htil = new_pvals->htil;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pctrl.htil), pvals.htil);

  pvals.vtil = new_pvals->vtil;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pctrl.vtil), pvals.vtil);

  pvals.rdt = new_pvals->rdt;
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (pctrl.rdt), pvals.rdt);

  pvals.fsf = new_pvals->fsf;
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (pctrl.fsf), pvals.fsf);

  pvals.cm = new_pvals->cm;
  if (!is_rgb)
    pvals.cm = PLASMA2_CM_GRAY;

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (pctrl.cm), pvals.cm);
  gtk_widget_set_sensitive (pctrl.grad, pvals.cm == PLASMA2_CM_GRADIENT);
  gtk_widget_set_sensitive (pctrl.grad_label, pvals.cm == PLASMA2_CM_GRADIENT);

  pvals.lfwhm = new_pvals->lfwhm;
  gtk_adjustment_set_value (GIMP_SCALE_ENTRY_SCALE_ADJ (pctrl.lfwhm),
                            pvals.lfwhm);

  pvals.a = new_pvals->a;
  gtk_adjustment_set_value (GIMP_SCALE_ENTRY_SCALE_ADJ (pctrl.a),
                            pvals.a);

  pvals.seed = new_pvals->seed;
  gtk_adjustment_set_value (GIMP_RANDOM_SEED_SPINBUTTON_ADJ (pctrl.seed),
                            pvals.seed);

  /* unable to update: grad -- FIXME */
  plasma2 (drawable);
}
