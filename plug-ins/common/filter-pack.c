/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Copyright (C) Pavel Grinfeld (pavel@ml.com)
 *
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC       "plug-in-filter-pack"
#define PLUG_IN_BINARY     "filter-pack"
#define PLUG_IN_ROLE       "gimp-filter-pack"

#define MAX_PREVIEW_SIZE   125
#define MAX_ROUGHNESS      128
#define RANGE_HEIGHT       15
#define PR_BX_BRDR         4
#define MARGIN             4

#define RANGE_ADJUST_MASK (GDK_EXPOSURE_MASK       | \
                           GDK_ENTER_NOTIFY_MASK   | \
                           GDK_BUTTON_PRESS_MASK   | \
                           GDK_BUTTON_RELEASE_MASK | \
                           GDK_BUTTON1_MOTION_MASK | \
                           GDK_POINTER_MOTION_HINT_MASK)


typedef struct
{
  gboolean run;
} fpInterface;

typedef struct
{
  gint       width;
  gint       height;
  guchar    *rgb;
  gdouble   *hsv;
  guchar    *mask;
} ReducedImage;

typedef enum
{
  SHADOWS,
  MIDTONES,
  HIGHLIGHTS,
  INTENSITIES
} FPIntensity;

enum
{
  NONEATALL  = 0,
  CURRENT    = 1,
  HUE        = 2,
  SATURATION = 4,
  VALUE      = 8
};

enum
{
  BY_HUE,
  BY_SAT,
  BY_VAL,
  JUDGE_BY
};

enum
{
  RED,
  GREEN,
  BLUE,
  CYAN,
  YELLOW,
  MAGENTA,
  ALL_PRIMARY
};

enum
{
  DOWN = -1,
  UP   =  1
};

typedef struct
{
  GtkWidget *window;
  GtkWidget *range_preview;
  GtkWidget *aliasing_preview;
  GtkWidget *aliasing_graph;
} AdvancedWindow;


typedef struct
{
  gdouble       roughness;
  gdouble       aliasing;
  gdouble       preview_size;
  FPIntensity   intensity_range;
  gint          value_by;
  gint          selection_only;
  guchar        offset;
  guchar        visible_frames;
  guchar        cutoff[INTENSITIES];
  gboolean      touched[JUDGE_BY];
  gint          red_adjust[JUDGE_BY][256];
  gint          blue_adjust[JUDGE_BY][256];
  gint          green_adjust[JUDGE_BY][256];
  gint          sat_adjust[JUDGE_BY][256];
} FPValues;

typedef struct
{
  GtkWidget    *roughness_scale;
  GtkWidget    *aliasing_scale;
  GtkWidget    *preview_size_scale;
  GtkWidget    *range_label[12];
} FPWidgets;

static void           fp_show_hide_frame          (GtkWidget     *button,
                                                   GtkWidget     *frame);

static ReducedImage * fp_reduce_image             (GimpDrawable  *drawable,
                                                   GimpDrawable  *mask,
                                                   gint           longer_size,
                                                   gint           selection);

static void           fp_render_preview           (GtkWidget     *preview,
                                                   gint           change_what,
                                                   gint           change_which);

static void           update_current_fp           (gint           change_what,
                                                   gint           change_which);

static void           fp_create_nudge             (gint          *adj_array);

static gboolean       fp_dialog                   (void);
static void           fp_advanced_dialog          (GtkWidget     *parent);

static void           fp_selection_made           (GtkWidget     *widget,
                                                   gpointer       data);
static void           fp_scale_update             (GtkAdjustment *adjustment,
                                                   gdouble        *scale_val);
static void           fp_reset_filter_packs       (void);

static void           fp_create_smoothness_graph  (GtkWidget     *preview);

static void           fp_range_preview_spill      (GtkWidget     *preview,
                                                   gint           type);
static void           fp_adjust_preview_sizes     (gint           width,
                                                   gint           height);
static void           fp_redraw_all_windows       (void);
static void           fp_refresh_previews         (gint           which);
static void           fp_init_filter_packs        (void);

static void           fp_preview_scale_update     (GtkAdjustment *adjustment,
                                                   gdouble       *scale_val);

static void           fp                          (GimpDrawable  *drawable);
static GtkWidget *    fp_create_bna               (void);
static GtkWidget *    fp_create_rough             (void);
static GtkWidget *    fp_create_range             (void);
static GtkWidget *    fp_create_circle_palette    (GtkWidget     *parent);
static GtkWidget *    fp_create_lnd               (GtkWidget     *parent);
static GtkWidget *    fp_create_show              (void);
static GtkWidget *    fp_create_msnls             (GtkWidget     *parent);
static GtkWidget *    fp_create_pixels_select_by  (void);
static void           update_range_labels         (void);
static gboolean       fp_range_draw               (GtkWidget     *widget,
                                                   cairo_t       *cr,
                                                   FPValues      *current);
static gboolean       fp_range_change_events      (GtkWidget     *widget,
                                                   GdkEvent      *event,
                                                   FPValues      *current);

static void           fp_create_preview           (GtkWidget    **preview,
                                                   GtkWidget    **frame,
                                                   gint           preview_width,
                                                   gint           preview_height);

static void           fp_create_table_entry      (GtkWidget     **box,
                                                  GtkWidget      *smaller_frame,
                                                  const gchar    *description);

static void         fp_frames_checkbutton_in_box (GtkWidget      *vbox,
                                                  const gchar    *label,
                                                  GCallback       func,
                                                  GtkWidget      *frame,
                                                  gboolean        clicked);

static void             fp_preview_size_allocate (GtkWidget      *widget,
                                                  GtkAllocation  *allocation);

#define RESPONSE_RESET 1

/* These values are translated for the GUI but also used internally
   to figure out which button the user pushed, etc.
   Not my design, please don't blame me -- njl */

static const gchar *hue_red     = N_("Red:");
static const gchar *hue_green   = N_("Green:");
static const gchar *hue_blue    = N_("Blue:");
static const gchar *hue_cyan    = N_("Cyan:");
static const gchar *hue_yellow  = N_("Yellow:");
static const gchar *hue_magenta = N_("Magenta:");

static const gchar *val_darker  = N_("Darker:");
static const gchar *val_lighter = N_("Lighter:");

static const gchar *sat_more    = N_("More Sat:");
static const gchar *sat_less    = N_("Less Sat:");

static const gchar *current_val = N_("Current:");

static const gint colorSign[3][ALL_PRIMARY]=
{{1,-1,-1,-1,1,1},{-1,1,-1,1,1,-1},{-1,-1,1,1,-1,1}};

static AdvancedWindow AW = { NULL, NULL, NULL, NULL };

static FPWidgets fp_widgets = { NULL, NULL, NULL };

static gint nudgeArray[256];

static GtkWidget *origPreview, *curPreview;
static GtkWidget *rPreview, *gPreview, *bPreview;
static GtkWidget *cPreview, *yPreview, *mPreview;
static GtkWidget *centerPreview;
static GtkWidget *darkerPreview, *lighterPreview, *middlePreview;
static GtkWidget *dlg;
static GtkWidget *plusSatPreview, *SatPreview, *minusSatPreview;

static struct
{
  GtkWidget *bna;
  GtkWidget *palette;
  GtkWidget *rough;
  GtkWidget *range;
  GtkWidget *show;
  GtkWidget *lnd;
  GtkWidget *pixelsBy;
  GtkWidget *frameSelect;
  GtkWidget *satur;
} fp_frames;

static fpInterface FPint =
{
  FALSE   /*  run  */
};

static ReducedImage *reduced;

static FPValues fpvals =
{
  .25,                    /* Initial Roughness */
  .6,                     /* Initial Degree of Aliasing */
  80,                     /* Initial preview size */
  MIDTONES,               /* Initial Range */
  BY_VAL,                 /* Initial God knows what */
  TRUE,                   /* Selection Only */
  0,                      /* Offset */
  0,                      /* Visible frames */
  { 32, 224, 255 },       /* cutoffs */
  { FALSE, FALSE, FALSE } /* touched */
};

static GimpDrawable *drawable = NULL;
static GimpDrawable *mask     = NULL;

static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN()

static void
query (void)
{
  GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"          },
    { GIMP_PDB_IMAGE,    "image",    "Input image (used for indexed images)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"                        }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Interactively modify the image colors"),
                          "Interactively modify the image colors.",
                          "Pavel Grinfeld (pavel@ml.com)",
                          "Pavel Grinfeld (pavel@ml.com)",
                          "27th March 1997",
                          N_("_Filter Pack..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

/********************************STANDARD RUN*************************/

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunMode       run_mode;

  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  fp_init_filter_packs();

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  if (gimp_selection_is_empty (param[1].data.d_image))
    mask = NULL;
  else
    mask = gimp_drawable_get (gimp_image_get_selection (param[1].data.d_image));

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &fpvals);

      if (gimp_drawable_is_indexed (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id) )
        {
          gimp_message (_("FP can only be used on RGB images."));
          status = GIMP_PDB_EXECUTION_ERROR;
        }
      else if (! fp_dialog())
        {
          status = GIMP_PDB_CANCEL;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      gimp_message (_("FP can only be run interactively."));
      status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PLUG_IN_PROC, &fpvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id))
        {
          gimp_progress_init (_("Applying filter pack"));
          gimp_tile_cache_ntiles (2 * (drawable->width /
                                       gimp_tile_width () + 1));
          fp (drawable);

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &fpvals, sizeof (FPValues));

          gimp_displays_flush ();
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  gimp_drawable_detach (drawable);
  if (mask)
    gimp_drawable_detach (mask);

  values[0].data.d_status = status;
}

static void
fp_func (const guchar *src,
         guchar       *dest,
         gint          bpp,
         gpointer      data)
{
  gint    bytenum, k;
  gint    JudgeBy, Intensity = 0, P[3];
  GimpRGB rgb;
  GimpHSV hsv;
  gint    M, m, middle;

  P[0] = src[0];
  P[1] = src[1];
  P[2] = src[2];

  gimp_rgb_set_uchar (&rgb, (guchar) P[0], (guchar) P[1], (guchar) P[2]);
  gimp_rgb_to_hsv (&rgb, &hsv);

  for (JudgeBy = BY_HUE; JudgeBy < JUDGE_BY; JudgeBy++)
    {
      if (!fpvals.touched[JudgeBy])
        continue;

      switch (JudgeBy)
        {
        case BY_HUE:
          Intensity = 255 * hsv.h;
          break;

        case BY_SAT:
          Intensity = 255 * hsv.s;
          break;

        case BY_VAL:
          Intensity = 255 * hsv.v;
          break;
        }


      /* It's important to take care of Saturation first!!! */

      m = MIN (MIN (P[0], P[1]), P[2]);
      M = MAX (MAX (P[0], P[1]), P[2]);
      middle = (M + m) / 2;

      for (k = 0; k < 3; k++)
        if (P[k] != m && P[k] != M)
          middle = P[k];

      for (k = 0; k < 3; k++)
        if (M != m)
          {
            if (P[k] == M)
              P[k] = MAX (P[k] + fpvals.sat_adjust[JudgeBy][Intensity], middle);
            else if (P[k] == m)
              P[k] = MIN (P[k] - fpvals.sat_adjust[JudgeBy][Intensity], middle);
          }

      P[0] += fpvals.red_adjust[JudgeBy][Intensity];
      P[1] += fpvals.green_adjust[JudgeBy][Intensity];
      P[2] += fpvals.blue_adjust[JudgeBy][Intensity];

      P[0]  = CLAMP0255(P[0]);
      P[1]  = CLAMP0255(P[1]);
      P[2]  = CLAMP0255(P[2]);
    }

  dest[0] = P[0];
  dest[1] = P[1];
  dest[2] = P[2];

  for (bytenum = 3; bytenum < bpp; bytenum++)
    dest[bytenum] = src[bytenum];
}

static void
fp (GimpDrawable *drawable)
{
  gimp_rgn_iterate2 (drawable, 0 /* unused */, fp_func, NULL);
}

/***********************************************************/
/************   Main Dialog Window   ******************/
/***********************************************************/

static GtkWidget *
fp_create_bna (void)
{
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *bframe, *aframe;

  fp_create_preview (&origPreview, &bframe, reduced->width, reduced->height);
  fp_create_preview (&curPreview, &aframe, reduced->width, reduced->height);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);

  label = gtk_label_new (_("Original:"));
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  gtk_table_attach (GTK_TABLE (table), bframe, 0, 1, 1, 2,
                    GTK_EXPAND, 0, 0, 0);

  label = gtk_label_new (_("Current:"));
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  gtk_table_attach (GTK_TABLE (table), aframe, 1, 2, 1, 2,
                    GTK_EXPAND, 0, 0, 0);

  gtk_widget_show (table);

  return table;
}

/* close a sub dialog (from window manager) by simulating toggle click */
static gboolean
sub_dialog_destroy (GtkWidget *dialog,
                    GdkEvent  *ev,
                    gpointer   dummy)
{
  GtkWidget *button =
    GTK_WIDGET (g_object_get_data (G_OBJECT (dialog), "ctrlButton"));

  gtk_button_clicked (GTK_BUTTON (button));

  return TRUE;
}

static GtkWidget *
fp_create_circle_palette (GtkWidget *parent)
{
  GtkWidget *table;
  GtkWidget *rVbox, *rFrame;
  GtkWidget *gVbox, *gFrame;
  GtkWidget *bVbox, *bFrame;
  GtkWidget *cVbox, *cFrame;
  GtkWidget *yVbox, *yFrame;
  GtkWidget *mVbox, *mFrame;
  GtkWidget *centerVbox, *centerFrame;
  GtkWidget *win;

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gimp_help_connect (win, gimp_standard_help_func, PLUG_IN_PROC, NULL);

  gtk_window_set_title (GTK_WINDOW (win), _("Hue Variations"));
  gtk_window_set_transient_for (GTK_WINDOW (win), GTK_WINDOW (parent));

  g_signal_connect (win, "delete-event",
                    G_CALLBACK (sub_dialog_destroy),
                    NULL);

  table = gtk_table_new (11, 11, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_container_add (GTK_CONTAINER (win), table);
  gtk_widget_show (table);

  fp_create_preview (&rPreview, &rFrame, reduced->width, reduced->height);
  fp_create_preview (&gPreview, &gFrame, reduced->width, reduced->height);
  fp_create_preview (&bPreview, &bFrame, reduced->width, reduced->height);
  fp_create_preview (&cPreview, &cFrame, reduced->width, reduced->height);
  fp_create_preview (&yPreview, &yFrame, reduced->width, reduced->height);
  fp_create_preview (&mPreview, &mFrame, reduced->width, reduced->height);
  fp_create_preview (&centerPreview, &centerFrame,
                     reduced->width, reduced->height);

  fp_create_table_entry (&rVbox, rFrame, hue_red);
  fp_create_table_entry (&gVbox, gFrame, hue_green);
  fp_create_table_entry (&bVbox, bFrame, hue_blue);
  fp_create_table_entry (&cVbox, cFrame, hue_cyan);
  fp_create_table_entry (&yVbox, yFrame, hue_yellow);
  fp_create_table_entry (&mVbox, mFrame, hue_magenta);
  fp_create_table_entry (&centerVbox, centerFrame, current_val);

  gtk_table_attach (GTK_TABLE (table), rVbox, 8, 11 ,4 , 7,
                    GTK_EXPAND , GTK_EXPAND, 0 ,0);
  gtk_table_attach (GTK_TABLE (table), gVbox, 2, 5, 0, 3,
                    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (table), bVbox, 2, 5, 8, 11,
                    GTK_EXPAND, GTK_EXPAND,0,0);
  gtk_table_attach (GTK_TABLE (table), cVbox, 0, 3, 4, 7,
                    GTK_EXPAND, GTK_EXPAND, 0 ,0);
  gtk_table_attach (GTK_TABLE (table), yVbox, 6, 9, 0, 3,
                    GTK_EXPAND, GTK_EXPAND, 0 ,0);
  gtk_table_attach (GTK_TABLE (table), mVbox, 6, 9, 8, 11,
                    GTK_EXPAND, GTK_EXPAND, 0 ,0);
  gtk_table_attach (GTK_TABLE (table), centerVbox, 4, 7, 4, 7,
                    GTK_EXPAND, GTK_EXPAND, 0 ,0);

  return win;
}

static GtkWidget *
fp_create_rough (void)
{
  GtkWidget     *frame, *vbox, *scale;
  GtkAdjustment *data;

  frame = gimp_frame_new (_("Roughness"));
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  data = gtk_adjustment_new (fpvals.roughness, 0, 1.0, 0.05, 0.01, 0.0);
  fp_widgets.roughness_scale = scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                                      data);

  gtk_widget_set_size_request (scale, 60, -1);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_widget_show (scale);

  g_signal_connect (data, "value-changed",
                    G_CALLBACK (fp_scale_update),
                    &fpvals.roughness);

  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return frame;
}

static void
fp_change_current_range (GtkWidget *widget,
                         gpointer   data)
{
  gimp_radio_button_update (widget, data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      fp_refresh_previews (fpvals.visible_frames);
      if (AW.window && gtk_widget_get_visible (AW.window))
        fp_create_smoothness_graph (AW.aliasing_preview);
    }
}

static GtkWidget *
fp_create_range (void)
{
  GtkWidget *frame;

  frame = gimp_int_radio_group_new (TRUE, _("Affected Range"),
                                    G_CALLBACK (fp_change_current_range),
                                    &fpvals.intensity_range, fpvals.intensity_range,

                                    _("Sha_dows"),    SHADOWS,    NULL,
                                    _("_Midtones"),   MIDTONES,   NULL,
                                    _("H_ighlights"), HIGHLIGHTS, NULL,

                                    NULL);

  gtk_widget_show (frame);

  return frame;
}

static GtkWidget *
fp_create_control (void)
{
  GtkWidget *frame, *box;

  frame = gimp_frame_new (_("Windows"));

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  fp_frames_checkbutton_in_box (box, _("_Hue"),
                                G_CALLBACK (fp_show_hide_frame),
                                fp_frames.palette,
                                fpvals.visible_frames & HUE);
  fp_frames_checkbutton_in_box (box, _("_Saturation"),
                                G_CALLBACK (fp_show_hide_frame),
                                fp_frames.satur,
                                fpvals.visible_frames & SATURATION);
  fp_frames_checkbutton_in_box (box, _("_Value"),
                                G_CALLBACK (fp_show_hide_frame),
                                fp_frames.lnd,
                                fpvals.visible_frames & VALUE);
  fp_frames_checkbutton_in_box (box, _("A_dvanced"),
                                G_CALLBACK (fp_show_hide_frame),
                                AW.window,
                                FALSE);
  gtk_widget_show (frame);

  return frame;
}

static GtkWidget *
fp_create_lnd (GtkWidget *parent)
{
  GtkWidget *table, *lighterFrame, *middleFrame, *darkerFrame;
  GtkWidget *lighterVbox, *middleVbox, *darkerVbox;
  GtkWidget *win;

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gimp_help_connect (win, gimp_standard_help_func, PLUG_IN_PROC, NULL);

  gtk_window_set_title (GTK_WINDOW (win), _("Value Variations"));
  gtk_window_set_transient_for (GTK_WINDOW (win), GTK_WINDOW (parent));

  g_signal_connect (win, "delete-event",
                    G_CALLBACK (sub_dialog_destroy),
                    NULL);

  fp_create_preview (&lighterPreview, &lighterFrame,
                    reduced->width, reduced->height);
  fp_create_preview (&middlePreview, &middleFrame,
                    reduced->width, reduced->height);
  fp_create_preview (&darkerPreview, &darkerFrame,
                    reduced->width, reduced->height);

  fp_create_table_entry (&lighterVbox, lighterFrame, val_lighter);
  fp_create_table_entry (&middleVbox, middleFrame, current_val);
  fp_create_table_entry (&darkerVbox, darkerFrame, val_darker);

  table = gtk_table_new (1, 11, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_container_add (GTK_CONTAINER (win), table);
  gtk_widget_show (table);

  gtk_table_attach (GTK_TABLE (table), lighterVbox, 0, 3, 0, 1,
                    GTK_EXPAND , GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (table), middleVbox, 4, 7, 0, 1,
                    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (table), darkerVbox, 8, 11, 0, 1,
                    GTK_EXPAND, GTK_EXPAND, 0, 0);

  return win;
}

static GtkWidget *
fp_create_msnls (GtkWidget *parent)
{
  GtkWidget *table, *lessFrame, *middleFrame, *moreFrame;
  GtkWidget *lessVbox, *middleVbox, *moreVbox;
  GtkWidget *win;

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gimp_help_connect (win, gimp_standard_help_func, PLUG_IN_PROC, NULL);

  gtk_window_set_title (GTK_WINDOW (win), _("Saturation Variations"));
  gtk_window_set_transient_for (GTK_WINDOW (win), GTK_WINDOW (parent));

  g_signal_connect (win, "delete-event",
                    G_CALLBACK (sub_dialog_destroy),
                    NULL);

  fp_create_preview (&minusSatPreview, &lessFrame,
                    reduced->width, reduced->height);
  fp_create_preview (&SatPreview, &middleFrame,
                    reduced->width, reduced->height);
  fp_create_preview (&plusSatPreview, &moreFrame,
                    reduced->width, reduced->height);

  fp_create_table_entry (&moreVbox, moreFrame, sat_more);
  fp_create_table_entry (&middleVbox, middleFrame, current_val);
  fp_create_table_entry (&lessVbox, lessFrame, sat_less);

  table = gtk_table_new (1, 11, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_container_add (GTK_CONTAINER (win), table);
  gtk_widget_show (table);

  gtk_table_attach (GTK_TABLE (table), moreVbox, 0, 3, 0, 1,
                    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (table), middleVbox, 4, 7, 0, 1,
                    GTK_EXPAND, GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (table), lessVbox, 8, 11, 0, 1,
                    GTK_EXPAND, GTK_EXPAND, 0, 0);

  return win;
}

static void
fp_change_current_pixels_by (GtkWidget *widget,
                             gpointer   data)
{
  gimp_radio_button_update (widget, data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      fp_refresh_previews (fpvals.visible_frames);
      if (AW.window && gtk_widget_get_visible (AW.window) && AW.range_preview)
        fp_range_preview_spill (AW.range_preview,fpvals.value_by);
    }
}

static GtkWidget *
fp_create_pixels_select_by (void)
{
  GtkWidget *frame;

  frame = gimp_int_radio_group_new (TRUE, _("Select Pixels By"),
                                    G_CALLBACK (fp_change_current_pixels_by),
                                    &fpvals.value_by,
                                    fpvals.value_by,

                                    _("H_ue"),  0, NULL,
                                    _("Satu_ration"), 1, NULL,
                                    _("V_alue"), 2, NULL,

                                    NULL);

  gtk_widget_show (frame);

  return frame;
}

static void
fp_change_selection (GtkWidget *widget,
                     gpointer   data)
{
  gimp_radio_button_update (widget, data);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      fp_redraw_all_windows ();
    }
}

static GtkWidget *
fp_create_show (void)
{
  GtkWidget *frame;

  frame = gimp_int_radio_group_new (TRUE, _("Show"),
                                    G_CALLBACK (fp_change_selection),
                                    &fpvals.selection_only,
                                    fpvals.selection_only,

                                    _("_Entire image"),  0, NULL,
                                    _("Se_lection only"), 1, NULL,
                                    _("Selec_tion in context"), 2, NULL,

                                    NULL);

  gtk_widget_show (frame);

  return frame;
}

static void
fp_create_preview (GtkWidget **preview,
                  GtkWidget  **frame,
                  gint         preview_width,
                  gint         preview_height)
{
  *frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (*frame), GTK_SHADOW_IN);
  gtk_widget_show (*frame);

  *preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (*preview, preview_width, preview_height);
  g_signal_connect (*preview, "size-allocate",
                    G_CALLBACK (fp_preview_size_allocate), NULL);
  gtk_widget_show (*preview);
  gtk_container_add (GTK_CONTAINER (*frame), *preview);
}

static void
fp_frames_checkbutton_in_box (GtkWidget     *vbox,
                              const gchar   *label,
                              GCallback      function,
                              GtkWidget     *frame,
                              gboolean       clicked)
{
  GtkWidget *button;

  button = gtk_check_button_new_with_mnemonic (label);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_object_set_data (G_OBJECT (frame), "ctrlButton", (gpointer) button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    function,
                    frame);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clicked);
}

static void
fp_create_table_entry (GtkWidget   **box,
                      GtkWidget    *smaller_frame,
                      const gchar  *description)
{
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *table;

  *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
  gtk_container_set_border_width (GTK_CONTAINER (*box), PR_BX_BRDR);
  gtk_widget_show (*box);

  /* Delayed translation applied here */
  label = gtk_label_new (gettext (description));

  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_widget_show (label);

  table = gtk_table_new (2, 1, FALSE);
  gtk_widget_show (table);

  gtk_box_pack_start (GTK_BOX (*box), table, TRUE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    0, 0, 0, 0);

  if (description != current_val)
    {
      button = gtk_button_new ();
      gtk_table_attach (GTK_TABLE (table), button, 0, 1, 1, 2,
                        0, 0, 0, 4);
      gtk_widget_show (button);

      gtk_container_add (GTK_CONTAINER (button), smaller_frame);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (fp_selection_made),
                        (gchar *) description);
    }
  else
    {
      gtk_table_attach (GTK_TABLE (table), smaller_frame, 0, 1, 1, 2,
                        0, 0, 0, 4);
    }
}

static void
fp_redraw_all_windows (void)
{
  if (reduced)
    {
      g_free (reduced->rgb);
      g_free (reduced->hsv);
      g_free (reduced->mask);

      g_free (reduced);
    }

  reduced = fp_reduce_image (drawable, mask,
                             fpvals.preview_size,
                             fpvals.selection_only);

  fp_adjust_preview_sizes (reduced->width, reduced->height);

  gtk_widget_queue_draw (fp_frames.palette);
  gtk_widget_queue_draw (fp_frames.satur);
  gtk_widget_queue_draw (fp_frames.lnd);
  gtk_widget_queue_draw (dlg);

  fp_refresh_previews (fpvals.visible_frames);
}

static void
fp_show_hide_frame (GtkWidget *button,
                    GtkWidget *frame)
{
  gint prev = fpvals.visible_frames;

  if (frame == NULL)
    return;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      if (! gtk_widget_get_visible (frame))
        {
          gtk_widget_show (frame);

          if (frame==fp_frames.palette)
            fpvals.visible_frames |= HUE;
          else if (frame==fp_frames.satur)
            fpvals.visible_frames |= SATURATION;
          else if (frame==fp_frames.lnd)
            fpvals.visible_frames |= VALUE;

          fp_refresh_previews (fpvals.visible_frames & ~prev);
          fp_create_smoothness_graph (AW.aliasing_preview);
          fp_range_preview_spill (AW.range_preview,fpvals.value_by);
        }
    }
  else
    {
      if (gtk_widget_get_visible (frame))
        {
          gtk_widget_hide (frame);

          if (frame==fp_frames.palette)
            fpvals.visible_frames &= ~HUE;
          else if (frame==fp_frames.satur)
            fpvals.visible_frames &= ~SATURATION;
          else if (frame==fp_frames.lnd)
            fpvals.visible_frames &= ~VALUE;
        }
    }
}

static void
fp_adjust_preview_sizes (gint width,
                         gint height)
{
  gtk_widget_set_size_request (origPreview,     width, height);
  gtk_widget_set_size_request (curPreview,      width, height);
  gtk_widget_set_size_request (rPreview,        width, height);
  gtk_widget_set_size_request (gPreview,        width, height);
  gtk_widget_set_size_request (bPreview,        width, height);
  gtk_widget_set_size_request (cPreview,        width, height);
  gtk_widget_set_size_request (yPreview,        width, height);
  gtk_widget_set_size_request (mPreview,        width, height);
  gtk_widget_set_size_request (centerPreview,   width, height);
  gtk_widget_set_size_request (lighterPreview,  width, height);
  gtk_widget_set_size_request (darkerPreview,   width, height);
  gtk_widget_set_size_request (middlePreview,   width, height);
  gtk_widget_set_size_request (minusSatPreview, width, height);
  gtk_widget_set_size_request (SatPreview,      width, height);
  gtk_widget_set_size_request (plusSatPreview,  width, height);

}

static void
fp_selection_made (GtkWidget *widget,
                   gpointer   data)
{
  fpvals.touched[fpvals.value_by] = TRUE;

  if (data == (gpointer) hue_red)
    {
      update_current_fp (HUE, RED);
    }
  else if (data == (gpointer) hue_green)
    {
      update_current_fp (HUE, GREEN);
    }
  else if (data == (gpointer) hue_blue)
    {
      update_current_fp (HUE, BLUE);
    }
  else if (data == (gpointer) hue_cyan)
    {
      update_current_fp (HUE, CYAN);
    }
  else if (data == (gpointer) hue_yellow)
    {
      update_current_fp (HUE, YELLOW);
    }
  else if (data == (gpointer) hue_magenta)
    {
      update_current_fp (HUE, MAGENTA);
    }
  else if (data == (gpointer) val_darker)
    {
      update_current_fp (VALUE, DOWN);
    }
  else if (data == (gpointer) val_lighter)
    {
      update_current_fp (VALUE, UP);
    }
  else if (data == (gpointer) sat_more)
    {
      update_current_fp (SATURATION, UP);
    }
  else if (data == (gpointer) sat_less)
    {
      update_current_fp (SATURATION, DOWN);
    }

  fp_refresh_previews (fpvals.visible_frames);
}

static void
fp_refresh_previews (gint which)
{
  fp_create_nudge (nudgeArray);
  fp_render_preview (origPreview, NONEATALL, 0);
  fp_render_preview (curPreview, CURRENT, 0);

  if (which & HUE)
    {
      fp_render_preview (rPreview,        HUE,        RED);
      fp_render_preview (gPreview,        HUE,        GREEN);
      fp_render_preview (bPreview,        HUE,        BLUE);
      fp_render_preview (cPreview,        HUE,        CYAN);
      fp_render_preview (yPreview,        HUE,        YELLOW);
      fp_render_preview (mPreview,        HUE,        MAGENTA);
      fp_render_preview (centerPreview,   CURRENT,    0);
    }

  if (which & VALUE)
    {
      fp_render_preview (lighterPreview,  VALUE,      UP);
      fp_render_preview (middlePreview,   CURRENT,    0);
      fp_render_preview (darkerPreview,   VALUE,      DOWN);
    }

  if (which & SATURATION)
    {
      fp_render_preview (plusSatPreview,  SATURATION, UP);
      fp_render_preview (SatPreview,      CURRENT,    0);
      fp_render_preview (minusSatPreview, SATURATION, DOWN);
    }
}

static void
fp_response (GtkWidget *widget,
             gint       response_id,
             gpointer   data)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      fp_reset_filter_packs ();
      break;

    case GTK_RESPONSE_OK:
      FPint.run = TRUE;
      gtk_widget_destroy (widget);
      break;

    default:
      gtk_widget_destroy (widget);
      break;
    }
}

static void
fp_scale_update (GtkAdjustment *adjustment,
                 gdouble       *scale_val)
{
  static gdouble prevValue = 0.25;

  *scale_val = gtk_adjustment_get_value (adjustment);

  if (prevValue != gtk_adjustment_get_value (adjustment))
    {
      fp_create_nudge (nudgeArray);
      fp_refresh_previews (fpvals.visible_frames);

      if (AW.window != NULL && gtk_widget_get_visible (AW.window))
        fp_create_smoothness_graph (AW.aliasing_preview);

      prevValue = gtk_adjustment_get_value (adjustment);
    }
}

static gboolean
fp_dialog (void)
{
  GtkWidget *bna;
  GtkWidget *palette;
  GtkWidget *lnd;
  GtkWidget *show;
  GtkWidget *rough;
  GtkWidget *range;
  GtkWidget *pixelsBy;
  GtkWidget *satur;
  GtkWidget *control;
  GtkWidget *table;

  reduced = fp_reduce_image (drawable, mask,
                             fpvals.preview_size,
                             fpvals.selection_only);

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dlg = gimp_dialog_new (_("Filter Pack Simulation"), PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Reset"),  RESPONSE_RESET,
                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  g_signal_connect (dlg, "response",
                    G_CALLBACK (fp_response),
                    dlg);

  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  fp_advanced_dialog (dlg);

  fp_frames.bna          = bna          = fp_create_bna ();
  fp_frames.rough        = rough        = fp_create_rough ();
  fp_frames.range        = range        = fp_create_range ();
  fp_frames.palette      = palette      = fp_create_circle_palette (dlg);
  fp_frames.lnd          = lnd          = fp_create_lnd (dlg);
  fp_frames.show         = show         = fp_create_show ();
  fp_frames.satur        = satur        = fp_create_msnls (dlg);
  fp_frames.pixelsBy     = pixelsBy     = fp_create_pixels_select_by ();
                           control      = fp_create_control ();
  /********************************************************************/
  /********************   PUT EVERYTHING TOGETHER    ******************/

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  gtk_table_attach (GTK_TABLE (table), bna, 0, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), control, 1, 2, 1, 3,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);

  gtk_table_attach (GTK_TABLE (table), rough, 1, 2, 3, 4,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);

  gtk_table_attach (GTK_TABLE (table), show, 0, 1, 1, 2,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);

  gtk_table_attach (GTK_TABLE (table), range, 0, 1, 2, 3,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);

  gtk_table_attach (GTK_TABLE (table), pixelsBy, 0, 1, 3, 4,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);

  gtk_widget_show (dlg);

  fp_refresh_previews (fpvals.visible_frames);

  gtk_main ();

  return FPint.run;
}

/***********************************************************/
/************   Advanced Options Window   ******************/
/***********************************************************/

static void
fp_preview_scale_update (GtkAdjustment *adjustment,
                         gdouble        *scale_val)
{
  fpvals.preview_size = gtk_adjustment_get_value (adjustment);
  fp_redraw_all_windows();
}

static void
fp_advanced_dialog (GtkWidget *parent)
{
  const gchar *rangeNames[] = { N_("Shadows:"),
                                N_("Midtones:"),
                                N_("Highlights:") };
  GtkWidget     *frame, *hbox;
  GtkAdjustment *smoothnessData;
  GtkWidget     *graphFrame, *scale;
  GtkWidget     *vbox, *label, *labelTable, *alignment;
  GtkWidget     *inner_vbox, *innermost_vbox;
  gint           i;

  AW.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gimp_help_connect (AW.window, gimp_standard_help_func, PLUG_IN_PROC, NULL);

  gtk_window_set_title (GTK_WINDOW (AW.window),
                        _("Advanced Filter Pack Options"));
  gtk_window_set_transient_for (GTK_WINDOW (AW.window), GTK_WINDOW (parent));

  g_signal_connect (AW.window, "delete-event",
                    G_CALLBACK (sub_dialog_destroy),
                    NULL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_container_add (GTK_CONTAINER (AW.window), hbox);
  gtk_widget_show (hbox);

  frame = gimp_frame_new (_("Affected Range"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  graphFrame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1, TRUE);
  gtk_frame_set_shadow_type (GTK_FRAME (graphFrame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (graphFrame), 0);
  gtk_box_pack_start (GTK_BOX (vbox), graphFrame, FALSE, FALSE, 0);
  gtk_widget_show (graphFrame);

  inner_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (graphFrame), inner_vbox);
  gtk_widget_show (inner_vbox);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (inner_vbox), alignment, TRUE, TRUE, 0);
  gtk_widget_show (alignment);

  innermost_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (alignment), innermost_vbox);
  gtk_widget_show (innermost_vbox);

  AW.aliasing_preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (AW.aliasing_preview, 256, MAX_ROUGHNESS);
  gtk_box_pack_start (GTK_BOX (innermost_vbox),
                      AW.aliasing_preview, TRUE, TRUE, 0);
  gtk_widget_show (AW.aliasing_preview);

  fp_create_smoothness_graph (AW.aliasing_preview);

  AW.range_preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (AW.range_preview, 256, RANGE_HEIGHT);
  gtk_box_pack_start(GTK_BOX (innermost_vbox),
                     AW.range_preview, TRUE, TRUE, 0);
  gtk_widget_show (AW.range_preview);

  fp_range_preview_spill (AW.range_preview, fpvals.value_by);

  labelTable = gtk_table_new (3, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (labelTable), 6);
  gtk_table_set_row_spacings (GTK_TABLE (labelTable), 6);
  gtk_box_pack_start (GTK_BOX (vbox), labelTable, FALSE, FALSE, 0);
  gtk_widget_show (labelTable);

  /************************************************************/

  AW.aliasing_graph = gtk_drawing_area_new ();
  gtk_widget_set_size_request (AW.aliasing_graph,
                               2 * MARGIN + 256,
                               RANGE_HEIGHT);
  gtk_box_pack_start (GTK_BOX (inner_vbox), AW.aliasing_graph, TRUE, TRUE, 0);
  gtk_widget_show (AW.aliasing_graph);
  gtk_widget_set_events (AW.aliasing_graph, RANGE_ADJUST_MASK);

  g_signal_connect (AW.aliasing_graph, "draw",
                    G_CALLBACK (fp_range_draw),
                    &fpvals);
  g_signal_connect (AW.aliasing_graph, "event",
                    G_CALLBACK (fp_range_change_events),
                    &fpvals);

  /************************************************************/

  for (i = 0; i < 12; i++)
    {
      label = fp_widgets.range_label[i] = gtk_label_new ("-");

      if (!(i % 4))
        {
          gtk_label_set_text (GTK_LABEL(label), gettext (rangeNames[i/4]));
          gimp_label_set_attributes (GTK_LABEL (label),
                                     PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                     -1);
          gtk_label_set_xalign (GTK_LABEL (label), 1.0);
          gtk_label_set_yalign (GTK_LABEL (label), 1.0);
        }

      gtk_widget_show (label);
      gtk_table_attach (GTK_TABLE (labelTable), label, i%4, i%4+1, i/4, i/4+1,
                        GTK_EXPAND | GTK_FILL, 0, 0, 0);
    }

  smoothnessData = gtk_adjustment_new (fpvals.aliasing,
                                       0, 1.0, 0.05, 0.01, 0.0);

  fp_widgets.aliasing_scale = scale =
    gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, smoothnessData);
  gtk_widget_set_size_request (scale, 200, -1);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect (smoothnessData, "value-changed",
                    G_CALLBACK (fp_scale_update),
                    &fpvals.aliasing);

  /******************* MISC OPTIONS ***************************/

  frame = gimp_frame_new (_("Preview Size"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  smoothnessData = gtk_adjustment_new (fpvals.preview_size,
                                       50, MAX_PREVIEW_SIZE,
                                       5, 5, 0.0);

  fp_widgets.preview_size_scale = scale =
    gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, smoothnessData);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_set_size_request (scale, 100, -1);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_widget_show (scale);

  g_signal_connect (smoothnessData, "value-changed",
                    G_CALLBACK (fp_preview_scale_update),
                    &fpvals.preview_size);

  update_range_labels ();
}

static void
slider_erase (GtkWidget *widget,
              int        xpos)
{
  gtk_widget_queue_draw_area (widget,
                              MARGIN + xpos - (RANGE_HEIGHT - 1) / 2, 0,
                              RANGE_HEIGHT, RANGE_HEIGHT);
}

static void
draw_slider (cairo_t  *cr,
             GdkColor *border_color,
             GdkColor *fill_color,
             gint      xpos)
{
  cairo_move_to (cr, MARGIN + xpos, 0);
  cairo_line_to (cr, MARGIN + xpos - (RANGE_HEIGHT - 1) / 2, RANGE_HEIGHT - 1);
  cairo_line_to (cr, MARGIN + xpos + (RANGE_HEIGHT - 1) / 2, RANGE_HEIGHT - 1);
  cairo_line_to (cr, MARGIN + xpos, 0);

  gdk_cairo_set_source_color (cr, fill_color);
  cairo_fill_preserve (cr);

  gdk_cairo_set_source_color (cr, border_color);
  cairo_stroke (cr);
}

static gboolean
fp_range_draw (GtkWidget *widget,
               cairo_t   *cr,
               FPValues  *current)
{
  GtkStyle *style = gtk_widget_get_style (AW.aliasing_graph);

  cairo_translate (cr, 0.5, 0.5);
  cairo_set_line_width (cr, 1.0);

  draw_slider (cr,
               &style->black,
               &style->dark[GTK_STATE_NORMAL],
               fpvals.cutoff[SHADOWS]);

  draw_slider (cr,
               &style->black,
               &style->dark[GTK_STATE_NORMAL],
               fpvals.cutoff[MIDTONES]);

  draw_slider (cr,
               &style->black,
               &style->dark[GTK_STATE_SELECTED],
               fpvals.offset);

  return FALSE;
}

static gboolean
fp_range_change_events (GtkWidget *widget,
                        GdkEvent  *event,
                        FPValues  *current)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint shad, mid, offset, min;
  static guchar  *new;
  gint  x;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent= (GdkEventButton *) event;

      shad   =  abs (bevent->x - fpvals.cutoff[SHADOWS]);
      mid    =  abs (bevent->x - fpvals.cutoff[MIDTONES]);
      offset =  abs (bevent->x - fpvals.offset);

      min = MIN (MIN (shad, mid), offset);

      if (bevent->x >0 && bevent->x<256)
        {
          if (min == shad)
            new = &fpvals.cutoff[SHADOWS];
          else if (min == mid)
            new = &fpvals.cutoff[MIDTONES];
          else
            new = &fpvals.offset;

          slider_erase (AW.aliasing_graph, *new);
          *new = bevent->x;
        }

      gtk_widget_queue_draw (widget);

      fp_range_preview_spill (AW.range_preview, fpvals.value_by);
      update_range_labels ();
      fp_create_smoothness_graph (AW.aliasing_preview);
      break;

    case GDK_BUTTON_RELEASE:
      fp_refresh_previews (fpvals.visible_frames);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      x = mevent->x;

      if (x >= 0 && x < 256)
        {
          slider_erase (AW.aliasing_graph, *new);
          *new = x;
          gtk_widget_queue_draw (widget);
          fp_range_preview_spill (AW.range_preview, fpvals.value_by);
          update_range_labels ();
          fp_create_smoothness_graph (AW.aliasing_preview);
        }

      gdk_event_request_motions (mevent);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
update_range_labels (void)
{
  gchar buffer[4];

  gtk_label_set_text (GTK_LABEL(fp_widgets.range_label[1]), "0");

  g_snprintf (buffer, sizeof (buffer), "%d", fpvals.cutoff[SHADOWS]);
  gtk_label_set_text (GTK_LABEL (fp_widgets.range_label[3]), buffer);
  gtk_label_set_text (GTK_LABEL (fp_widgets.range_label[5]), buffer);

  g_snprintf (buffer, sizeof (buffer), "%d", fpvals.cutoff[MIDTONES]);
  gtk_label_set_text (GTK_LABEL (fp_widgets.range_label[7]), buffer);
  gtk_label_set_text (GTK_LABEL (fp_widgets.range_label[9]), buffer);

  gtk_label_set_text (GTK_LABEL(fp_widgets.range_label[11]), "255");
}

static void
fp_init_filter_packs (void)
{
  gint i, j;

  for (i = 0; i < 256; i++)
    for (j = BY_HUE; j < JUDGE_BY; j++)
      {
        fpvals.red_adjust   [j][i] = 0;
        fpvals.green_adjust [j][i] = 0;
        fpvals.blue_adjust  [j][i] = 0;
        fpvals.sat_adjust   [j][i] = 0;
      }
}

static void
fp_reset_filter_packs (void)
{
  fp_init_filter_packs ();
  fp_refresh_previews (fpvals.visible_frames);
}

static ReducedImage *
fp_reduce_image (GimpDrawable *drawable,
                 GimpDrawable *mask,
                 gint          longer_size,
                 gint          selection)
{
  gint          RH, RW, bytes = drawable->bpp;
  gint          x, y, width, height;
  ReducedImage *temp = g_new0 (ReducedImage, 1);
  guchar       *tempRGB, *src_row, *tempmask, *src_mask_row, R, G, B;
  gint          i, j, whichcol, whichrow;
  GimpPixelRgn  srcPR, srcMask;
  gdouble      *tempHSV;
  GimpRGB       rgb;
  GimpHSV       hsv;

  switch (selection)
    {
    case 0:
      x      = 0;
      width  = drawable->width;
      y      = 0;
      height = drawable->height;
      break;

    case 1:
      if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                          &x, &y, &width, &height))
        return temp;
      break;

    case 2:
      if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                          &x, &y, &width, &height) ||
          ! gimp_rectangle_intersect (x - width / 2, y - height / 2,
                                      2 * width, 2 * height,
                                      0, 0, drawable->width, drawable->height,
                                      &x, &y, &width, &height))
        return temp;
      break;

    default:
      return temp;
    }

  if (width > height)
    {
      RW = longer_size;
      RH = (gdouble) height * (gdouble) longer_size / (gdouble) width;
    }
  else
    {
      RH = longer_size;
      RW = (gdouble) width * (gdouble) longer_size / (gdouble) height;
    }

  tempRGB  = g_new (guchar, RW * RH * bytes);
  tempHSV  = g_new (gdouble, RW * RH * bytes);
  tempmask = g_new (guchar, RW * RH);

  src_row      = g_new (guchar, width * bytes);
  src_mask_row = g_new (guchar, width);

  gimp_pixel_rgn_init (&srcPR, drawable, x, y, width, height, FALSE, FALSE);

  if (mask)
    {
      gimp_pixel_rgn_init (&srcMask, mask, x, y, width, height, FALSE, FALSE);
    }
  else
    {
      memset (src_mask_row, 255, width);
    }

  for (i = 0; i < RH; i++)
    {
      whichrow = (gdouble) i * (gdouble) height / (gdouble) RH;

      gimp_pixel_rgn_get_row (&srcPR, src_row, x, y + whichrow, width);

      if (mask)
        gimp_pixel_rgn_get_row (&srcMask, src_mask_row, x, y + whichrow, width);

      for (j = 0; j < RW; j++)
        {
          whichcol = (gdouble) j * (gdouble) width / (gdouble) RW;

          tempmask[i * RW + j] = src_mask_row[whichcol];

          R = src_row[whichcol * bytes + 0];
          G = src_row[whichcol * bytes + 1];
          B = src_row[whichcol * bytes + 2];

          gimp_rgb_set_uchar (&rgb, R, G, B);
          gimp_rgb_to_hsv (&rgb, &hsv);

          tempRGB[i * RW * bytes + j * bytes + 0] = R;
          tempRGB[i * RW * bytes + j * bytes + 1] = G;
          tempRGB[i * RW * bytes + j * bytes + 2] = B;

          tempHSV[i * RW * bytes + j * bytes + 0] = hsv.h;
          tempHSV[i * RW * bytes + j * bytes + 1] = hsv.s;
          tempHSV[i * RW * bytes + j * bytes + 2] = hsv.v;

          if (bytes == 4)
            {
              tempRGB[i * RW * bytes + j * bytes + 3] =
                src_row[whichcol * bytes + 3];
            }
        }
    }

  g_free (src_row);
  g_free (src_mask_row);

  temp->width  = RW;
  temp->height = RH;
  temp->rgb    = tempRGB;
  temp->hsv    = tempHSV;
  temp->mask   = tempmask;

  return temp;
}

static void
fp_render_preview (GtkWidget *preview,
                   gint       change_what,
                   gint       change_which)
{
  guchar *a;
  gint    Inten;
  gint    bytes = drawable->bpp;
  gint    i, j, k, nudge, M, m, middle, JudgeBy;
  gdouble partial;
  gint    RW = reduced->width;
  gint    RH = reduced->height;
  gint    backupP[3];
  gint    P[3];
  gint    tempSat[JUDGE_BY][256];

  a = g_new (guchar, 4 * RW * RH);

  if (change_what == SATURATION)
    for (k = 0; k < 256; k++)
      {
        for (JudgeBy = BY_HUE; JudgeBy < JUDGE_BY; JudgeBy++)
          tempSat[JudgeBy][k] = 0;

        tempSat[fpvals.value_by][k] +=
          change_which * nudgeArray[(k + fpvals.offset) % 256];
      }

  for (i = 0; i < RH; i++)
    {
      for (j = 0; j < RW; j++)
        {
          backupP[0] = P[0] = reduced->rgb[i * RW * bytes + j * bytes + 0];
          backupP[1] = P[1] = reduced->rgb[i * RW * bytes + j * bytes + 1];
          backupP[2] = P[2] = reduced->rgb[i * RW * bytes + j * bytes + 2];

          m = MIN (MIN (P[0], P[1]), P[2]);
          M = MAX (MAX (P[0], P[1]), P[2]);

          middle = (M + m) / 2;

          for (k = 0; k < 3; k++)
            if (P[k] != m && P[k] != M) middle = P[k];

          partial = reduced->mask[i * RW + j] / 255.0;

          for (JudgeBy = BY_HUE; JudgeBy < JUDGE_BY; JudgeBy++)
            {
              if (!fpvals.touched[JudgeBy])
                continue;

              Inten =
                reduced->hsv[i * RW * bytes + j * bytes + JudgeBy] * 255.0;

              /*DO SATURATION FIRST*/
              if (change_what != NONEATALL)
                {
                  gint adjust = partial * fpvals.sat_adjust[JudgeBy][Inten];

                  if (M != m)
                    {
                      for (k = 0; k < 3; k++)
                        if (backupP[k] == M)
                          {
                            P[k] = MAX (P[k] + adjust, middle);
                          }
                        else if (backupP[k] == m)
                          {
                            P[k] = MIN (P[k] - adjust, middle);
                          }
                    }

                  P[0] += partial * fpvals.red_adjust[JudgeBy][Inten];
                  P[1] += partial * fpvals.green_adjust[JudgeBy][Inten];
                  P[2] += partial * fpvals.blue_adjust[JudgeBy][Inten];
                }
            }

          Inten =
            reduced->hsv[i * RW * bytes + j * bytes + fpvals.value_by] * 255.0;
          nudge = partial * nudgeArray[(Inten + fpvals.offset) % 256];

          switch (change_what)
            {
            case HUE:
              P[0] += colorSign[RED][change_which]   * nudge;
              P[1] += colorSign[GREEN][change_which] * nudge;
              P[2] += colorSign[BLUE][change_which]  * nudge;
              break;

            case SATURATION:
              for (JudgeBy = BY_HUE; JudgeBy < JUDGE_BY; JudgeBy++)
                {
                  gint adjust = partial * tempSat[JudgeBy][Inten];

                  for (k = 0; k < 3; k++)
                    if (M != m)
                      {
                        if (backupP[k] == M)
                          {
                            P[k] = MAX (P[k] + adjust, middle);
                          }
                        else if (backupP[k] == m)
                          {
                            P[k] = MIN (P[k] - adjust, middle);
                          }
                      }
                }
              break;

            case VALUE:
              P[0] += change_which * nudge;
              P[1] += change_which * nudge;
              P[2] += change_which * nudge;
              break;

            default:
              break;
            }

          a[(i * RW + j) * 4 + 0] = CLAMP0255 (P[0]);
          a[(i * RW + j) * 4 + 1] = CLAMP0255 (P[1]);
          a[(i * RW + j) * 4 + 2] = CLAMP0255 (P[2]);

          if (bytes == 4)
            a[(i * RW + j) * 4 + 3] = reduced->rgb[i * RW * bytes + j * bytes + 3];
          else
            a[(i * RW + j) * 4 + 3] = 255;
        }
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                          0, 0, RW, RH,
                          GIMP_RGBA_IMAGE,
                          a,
                          RW * 4);
  g_free (a);
}

static void
update_current_fp (gint change_what,
                   gint change_which)
{
  gint i;

  for (i = 0; i < 256; i++)
    {
      gint nudge;

      fp_create_nudge (nudgeArray);
      nudge = nudgeArray[(i + fpvals.offset) % 256];

      switch (change_what) {
      case HUE:
        fpvals.red_adjust[fpvals.value_by][i] +=
          colorSign[RED][change_which] * nudge;

        fpvals.green_adjust[fpvals.value_by][i] +=
          colorSign[GREEN][change_which] * nudge;

        fpvals.blue_adjust[fpvals.value_by][i] +=
          colorSign[BLUE][change_which]  * nudge;
        break;

      case SATURATION:
        fpvals.sat_adjust[fpvals.value_by][i] += change_which * nudge;
        break;

      case VALUE:
        fpvals.red_adjust[fpvals.value_by][i]   += change_which * nudge;
        fpvals.green_adjust[fpvals.value_by][i] += change_which * nudge;
        fpvals.blue_adjust[fpvals.value_by][i]  += change_which * nudge;
        break;

      default:
        break;
      }
    }
}

static void
fp_create_smoothness_graph (GtkWidget *preview)
{
  guchar data[256 * MAX_ROUGHNESS * 3];
  gint nArray[256];
  gint i, j;
  gboolean toBeBlack;

  fp_create_nudge(nArray);

  for (i = 0; i < MAX_ROUGHNESS; i++)
    {
      gint coor = MAX_ROUGHNESS - i;

      for (j = 0; j < 256; j++)
        {
          data[3 * (i * 256 + j) + 0] = 255;
          data[3 * (i * 256 + j) + 1] = 255;
          data[3 * (i * 256 + j) + 2] = 255;

          if (!(i % (MAX_ROUGHNESS / 4)))
            {
              data[3 * (i * 256 + j) + 0] = 255;
              data[3 * (i * 256 + j) + 1] = 128;
              data[3 * (i * 256 + j) + 2] = 128;
            }

          if (!((j + 1) % 32))
            {
              data[3 * (i * 256 + j) + 0] = 255;
              data[3 * (i * 256 + j) + 1] = 128;
              data[3 * (i * 256 + j) + 2] = 128;
            }

          toBeBlack = FALSE;

          if (nArray[j] == coor)
            toBeBlack = TRUE;

          if (j < 255)
            {
              gint jump = abs (nArray[j] - nArray[j+1]);

              if (abs (coor - nArray[j]) < jump  &&
                  abs (coor - nArray[j + 1]) < jump)
                toBeBlack = TRUE;
            }

          if (toBeBlack)
            {
              data[3 * (i * 256 + j) + 0] = 0;
              data[3 * (i * 256 + j) + 1] = 0;
              data[3 * (i * 256 + j) + 2] = 0;
            }
        }
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                          0, 0, 256, MAX_ROUGHNESS,
                          GIMP_RGB_IMAGE,
                          data,
                          256 * 3);
}

static void
fp_range_preview_spill (GtkWidget *preview,
                        gint       type)
{
  gint   i, j;
  guchar data[256 * RANGE_HEIGHT * 3];

  for (i = 0; i < RANGE_HEIGHT; i++)
    {
      for (j = 0; j < 256; j++)
        {
          GimpRGB rgb;
          GimpHSV hsv;

          if (! ((j + 1) % 32))
            {
              data[3 * (i * 256 + j) + 0] = 255;
              data[3 * (i * 256 + j) + 1] = 128;
              data[3 * (i * 256 + j) + 2] = 128;
            }
          else
            {
              switch (type)
                {
                case BY_VAL:
                  data[3 * (i * 256 + j) + 0] = j - fpvals.offset;
                  data[3 * (i * 256 + j) + 1] = j - fpvals.offset;
                  data[3 * (i * 256 + j) + 2] = j - fpvals.offset;
                  break;

                case BY_HUE:
                  gimp_hsv_set (&hsv,
                                ((j - fpvals.offset + 256) % 256) / 255.0,
                                1.0,
                                0.5);
                  gimp_hsv_to_rgb (&hsv, &rgb);
                  gimp_rgb_get_uchar (&rgb,
                                      &data[3 * (i * 256 + j) + 0],
                                      &data[3 * (i * 256 + j) + 1],
                                      &data[3 * (i * 256 + j) + 2]);
                  break;

                case BY_SAT:
                  gimp_hsv_set (&hsv,
                                0.5,
                                ((j - (gint) fpvals.offset + 256) % 256) / 255.0,
                                0.5);
                  gimp_hsv_to_rgb (&hsv, &rgb);
                  gimp_rgb_get_uchar (&rgb,
                                      &data[3 * (i * 256 + j) + 0],
                                      &data[3 * (i * 256 + j) + 1],
                                      &data[3 * (i * 256 + j) + 2]);
                  break;
                }
            }
        }
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                          0, 0, 256, RANGE_HEIGHT,
                          GIMP_RGB_IMAGE,
                          data,
                          256 * 3);
}

static void
fp_create_nudge (gint *adj_array)
{
  gint left, right, middle,i;
  /* The following function was determined by trial and error */
  gdouble Steepness = pow (1 - fpvals.aliasing, 4) * .8;

  left = (fpvals.intensity_range == SHADOWS) ? 0 : fpvals.cutoff[fpvals.intensity_range - 1];
  right = fpvals.cutoff[fpvals.intensity_range];
  middle = (left + right)/2;

  if (fpvals.aliasing)
    for (i = 0; i < 256; i++)
      if (i <= middle)
        adj_array[i] = MAX_ROUGHNESS *
          fpvals.roughness * (1 + tanh (Steepness * (i - left))) / 2;
      else
        adj_array[i] = MAX_ROUGHNESS *
          fpvals.roughness * (1 + tanh (Steepness * (right - i))) / 2;
  else
    for (i = 0; i < 256; i++)
      adj_array[i] = (left <= i && i <= right)
        ? MAX_ROUGHNESS * fpvals.roughness : 0;
}

static void
fp_preview_size_allocate (GtkWidget     *widget,
                          GtkAllocation *allocation)
{
  gint which = fpvals.visible_frames;

  if (widget == origPreview)
    fp_render_preview (origPreview, NONEATALL, 0);
  else if (widget == curPreview)
    fp_render_preview (curPreview, CURRENT, 0);

  if (which & HUE)
    {
      if (widget == rPreview)
        fp_render_preview (rPreview,        HUE,        RED);
      else if (widget == gPreview)
        fp_render_preview (gPreview,        HUE,        GREEN);
      else if (widget == bPreview)
        fp_render_preview (bPreview,        HUE,        BLUE);
      else if (widget == cPreview)
        fp_render_preview (cPreview,        HUE,        CYAN);
      else if (widget == yPreview)
        fp_render_preview (yPreview,        HUE,        YELLOW);
      else if (widget == mPreview)
        fp_render_preview (mPreview,        HUE,        MAGENTA);
      else if (widget == centerPreview)
        fp_render_preview (centerPreview,   CURRENT,    0);
    }

  if (which & VALUE)
    {
      if (widget == lighterPreview)
        fp_render_preview (lighterPreview,  VALUE,      UP);
      else if (widget == middlePreview)
        fp_render_preview (middlePreview,   CURRENT,    0);
      else if (widget == darkerPreview)
        fp_render_preview (darkerPreview,   VALUE,      DOWN);
    }

  if (which & SATURATION)
    {
      if (widget == plusSatPreview)
        fp_render_preview (plusSatPreview,  SATURATION, UP);
      else if (widget == SatPreview)
        fp_render_preview (SatPreview,      CURRENT,    0);
      else if (widget == minusSatPreview)
        fp_render_preview (minusSatPreview, SATURATION, DOWN);
    }
}
