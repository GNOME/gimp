/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Copyright (C) Pavel Grinfeld (pavel@ml.com)
 *
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
 */

#include "config.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define MAX_PREVIEW_SIZE   125
#define MAX_ROUGHNESS      128
#define RANGE_HEIGHT       15
#define PR_BX_BRDR         4
#define ALL                255
#define MARGIN             4

#define HELP_ID            "plug-in-filter-pack"

#define RANGE_ADJUST_MASK GDK_EXPOSURE_MASK | \
                        GDK_ENTER_NOTIFY_MASK | \
                        GDK_BUTTON_PRESS_MASK | \
                        GDK_BUTTON_RELEASE_MASK | \
                        GDK_BUTTON1_MOTION_MASK | \
                        GDK_POINTER_MOTION_HINT_MASK


typedef struct {
  gint run;
} fpInterface;

typedef struct {
  gint      width;
  gint      height;
  guchar    *rgb;
  gdouble   *hsv;
  guchar    *mask;
} ReducedImage;

typedef enum {
  SHADOWS,
  MIDTONES,
  HIGHLIGHTS,
  INTENSITIES
}FP_Intensity;

enum {
  NONEATALL  =0,
  CURRENT    =1,
  HUE        =2,
  SATURATION =4,
  VALUE      =8
};

enum {
  BY_HUE,
  BY_SAT,
  BY_VAL,
  JUDGE_BY
};

enum {
  RED,
  GREEN,
  BLUE,
  CYAN,
  YELLOW,
  MAGENTA,
  ALL_PRIMARY
};

enum {
  DOWN = -1,
  UP   =  1
};

typedef struct {
  GtkWidget *window;
  GtkWidget *shadowsEntry;
  GtkWidget *midtonesEntry;
  GtkWidget *rangePreview;
  GtkWidget *aliasingPreview;
  GtkObject *aliasingData;
  GtkWidget *aliasingGraph;
} AdvancedWindow;


typedef struct {
  int Color;
  float Rough;
  GtkWidget *roughnessScale;
  float Alias;
  GtkWidget *aliasingScale;
  float PreviewSize;
  GtkWidget *previewSizeScale;
  FP_Intensity Range;
  gint ValueBy;
  gint SlctnOnly;
  gint  RealTime;
  guchar Offset;
  guchar VisibleFrames;
  guchar Cutoffs[INTENSITIES];
  gint Touched[JUDGE_BY];
  gint redAdj[JUDGE_BY][256];
  gint blueAdj[JUDGE_BY][256];
  gint greenAdj[JUDGE_BY][256];
  gint satAdj[JUDGE_BY][256];
  GtkWidget *rangeLabels[12];
} FP_Params;

static void  fp_show_hide_frame(GtkWidget *button,
                                GtkWidget *frame);

static ReducedImage  *Reduce_The_Image   (GimpDrawable *,
                                          GimpDrawable *,
                                          gint,
                                          gint);

static void      fp_render_preview  (GtkWidget *,
                                     gint,
                                     gint);

static void      Update_Current_FP  (gint,
                                     gint);

static void      fp_Create_Nudge    (gint*);

static gboolean  fp_dialog          (void);
static void      fp_advanced_dialog (void);

static void      selectionMade                  (GtkWidget *,
                                                 gpointer     );
static void      fp_scale_update                (GtkAdjustment *,
                                                 float*       );
static void      resetFilterPacks               (void);

static void      fp_create_smoothness_graph     (GtkWidget*    );

static void      fp_range_preview_spill         (GtkWidget*,
                                                 gint           );
static void      Adjust_Preview_Sizes     (gint width,
                                           gint height   );
static void     fp_redraw_all_windows (void);
static void      refreshPreviews          (int);
static void      initializeFilterPacks    (void);

static void     As_You_Drag               (GtkWidget *button);
static void     preview_size_scale_update (GtkAdjustment *adjustment,
                                           float         *scale_val);

static void             fp                      (GimpDrawable  *drawable);
static GtkWidget       *fp_create_bna           (void);
static GtkWidget       *fp_create_rough         (void);
static GtkWidget       *fp_create_range         (void);
static GtkWidget       *fp_create_circle_palette(void);
static GtkWidget       *fp_create_lnd           (void);
static GtkWidget       *fp_create_show          (void);
static GtkWidget       *fp_create_msnls         (void);
static GtkWidget       *fp_create_pixels_select_by(void);
static void             update_range_labels    (void);
static gboolean         FP_Range_Change_Events (GtkWidget *widget,
                                                GdkEvent  *event,
                                                FP_Params *current);

static void      Create_A_Preview        (GtkWidget  **,
                                          GtkWidget  **,
                                          int,
                                          int           );

static void      Create_A_Table_Entry    (GtkWidget  **,
                                          GtkWidget  *,
                                          const gchar *);

static void Check_Button_In_A_Box       (GtkWidget     *,
                                         const gchar   *label,
                                         GtkSignalFunc  func,
                                         gpointer       data,
                                         int            clicked);

static void Frames_Check_Button_In_A_Box (GtkWidget     *,
                                          const gchar   *label,
                                          GtkSignalFunc  func,
                                          GtkWidget     *frame,
                                          int            clicked);


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

static gint colorSign[3][ALL_PRIMARY]=
{{1,-1,-1,-1,1,1},{-1,1,-1,1,1,-1},{-1,-1,1,1,-1,1}};

static AdvancedWindow AW = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };

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
} fpFrames;

static fpInterface FPint =
{
  FALSE   /*  run  */
};

static ReducedImage *reduced;

static FP_Params Current =
{
  1,
  .25,                /* Initial Roughness */
  NULL,
  .6,                 /* Initial Degree of Aliasing */
  NULL,
  80,
  NULL,
  MIDTONES,           /* Initial Range */
  BY_VAL,             /* Initial God knows what */
  TRUE,               /* Selection Only */
  TRUE,               /* Real Time */
  0,                  /* Offset */
  0,
  {32,224,255},
  {0,0,0}
};

static GimpDrawable *drawable, *mask;

static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

GimpPlugInInfo PLUG_IN_INFO =
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
    { GIMP_PDB_INT32,    "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (used for indexed images)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
  };

  gimp_install_procedure ("plug_in_filter_pack",
                          "Allows the user to change H, S, or C with many previews",
                          "No help available",
                          "Pavel Grinfeld (pavel@ml.com)",
                          "Pavel Grinfeld (pavel@ml.com)",
                          "27th March 1997",
                          N_("_Filter Pack..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register ("plug_in_filter_pack", "<Image>/Filters/Colors");
}

/********************************STANDARD RUN*************************/

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpParam         values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunMode       run_mode;

  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  initializeFilterPacks();

  drawable = gimp_drawable_get (param[2].data.d_drawable);
  mask = gimp_drawable_get (gimp_image_get_selection (param[1].data.d_image));

  if (gimp_drawable_is_indexed (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id) )
    {
      gimp_message (_("Convert the image to RGB first!"));
      status = GIMP_PDB_EXECUTION_ERROR;
    }
  else if (gimp_drawable_is_rgb (drawable->drawable_id) && fp_dialog())
    {
      gimp_progress_init (_("Applying the Filter Pack..."));
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      fp (drawable);
      gimp_displays_flush ();
    }
  else status = GIMP_PDB_EXECUTION_ERROR;


  values[0].data.d_status = status;

  if (status == GIMP_PDB_SUCCESS)
    gimp_drawable_detach (drawable);
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
      if (!Current.Touched[JudgeBy])
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
              P[k] = MAX (P[k] + Current.satAdj[JudgeBy][Intensity], middle);
            else if (P[k] == m)
              P[k] = MIN (P[k] - Current.satAdj[JudgeBy][Intensity], middle);
          }

      P[0] += Current.redAdj[JudgeBy][Intensity];
      P[1] += Current.greenAdj[JudgeBy][Intensity];
      P[2] += Current.blueAdj[JudgeBy][Intensity];

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

  Create_A_Preview (&origPreview, &bframe, reduced->width, reduced->height);
  Create_A_Preview (&curPreview, &aframe, reduced->width, reduced->height);

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
fp_create_circle_palette (void)
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

  gimp_help_connect (win, gimp_standard_help_func, HELP_ID, NULL);

  gtk_window_set_title (GTK_WINDOW (win), _("Hue Variations"));

  g_signal_connect (win, "delete_event",
                    G_CALLBACK (sub_dialog_destroy),
                    NULL);

  table = gtk_table_new (11, 11, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_container_add (GTK_CONTAINER (win), table);
  gtk_widget_show (table);

  Create_A_Preview (&rPreview, &rFrame, reduced->width, reduced->height);
  Create_A_Preview (&gPreview, &gFrame, reduced->width, reduced->height);
  Create_A_Preview (&bPreview, &bFrame, reduced->width, reduced->height);
  Create_A_Preview (&cPreview, &cFrame, reduced->width, reduced->height);
  Create_A_Preview (&yPreview, &yFrame, reduced->width, reduced->height);
  Create_A_Preview (&mPreview, &mFrame, reduced->width, reduced->height);
  Create_A_Preview (&centerPreview, &centerFrame,
                    reduced->width, reduced->height);

  Create_A_Table_Entry (&rVbox, rFrame, hue_red);
  Create_A_Table_Entry (&gVbox, gFrame, hue_green);
  Create_A_Table_Entry (&bVbox, bFrame, hue_blue);
  Create_A_Table_Entry (&cVbox, cFrame, hue_cyan);
  Create_A_Table_Entry (&yVbox, yFrame, hue_yellow);
  Create_A_Table_Entry (&mVbox, mFrame, hue_magenta);
  Create_A_Table_Entry (&centerVbox, centerFrame, current_val);

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
  GtkWidget *frame, *vbox, *scale;
  GtkObject *data;

  frame = gimp_frame_new (_("Roughness"));
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  data = gtk_adjustment_new (Current.Rough, 0, 1.0, 0.05, 0.01, 0.0);
  Current.roughnessScale = scale = gtk_hscale_new (GTK_ADJUSTMENT (data));

  gtk_widget_set_size_request (scale, 60, -1);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_widget_show (scale);

  g_signal_connect (data, "value_changed",
                    G_CALLBACK (fp_scale_update),
                    &Current.Rough);

  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return frame;
}

static void
fp_change_current_range (GtkWidget *widget,
                         gpointer   data)
{
  gimp_radio_button_update (widget, data);

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      refreshPreviews (Current.VisibleFrames);
      if (AW.window && GTK_WIDGET_VISIBLE (AW.window))
        fp_create_smoothness_graph (AW.aliasingPreview);
    }
}

static GtkWidget *
fp_create_range (void)
{
  GtkWidget *frame;

  frame = gimp_int_radio_group_new (TRUE, _("Affected Range"),
                                    G_CALLBACK (fp_change_current_range),
                                    &Current.Range, Current.Range,

                                    _("Sha_dows"),  SHADOWS, NULL,
                                    _("_Midtones"), MIDTONES, NULL,
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

  box = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  Frames_Check_Button_In_A_Box (box, _("_Hue"),
                                GTK_SIGNAL_FUNC (fp_show_hide_frame),
                                fpFrames.palette,
                                Current.VisibleFrames & HUE);
  Frames_Check_Button_In_A_Box (box, _("_Saturation"),
                                GTK_SIGNAL_FUNC (fp_show_hide_frame),
                                fpFrames.satur,
                                Current.VisibleFrames & SATURATION);
  Frames_Check_Button_In_A_Box (box, _("_Value"),
                                GTK_SIGNAL_FUNC (fp_show_hide_frame),
                                fpFrames.lnd,
                                Current.VisibleFrames & VALUE);
  Frames_Check_Button_In_A_Box (box, _("A_dvanced"),
                                GTK_SIGNAL_FUNC (fp_show_hide_frame),
                                AW.window,
                                FALSE);
  gtk_widget_show (frame);

  return frame;
}

static GtkWidget *
fp_create_lnd (void)
{
  GtkWidget *table, *lighterFrame, *middleFrame, *darkerFrame;
  GtkWidget *lighterVbox, *middleVbox, *darkerVbox;
  GtkWidget *win;

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gimp_help_connect (win, gimp_standard_help_func, HELP_ID, NULL);

  gtk_window_set_title (GTK_WINDOW (win), _("Value Variations"));

  g_signal_connect (win, "delete_event",
                    G_CALLBACK (sub_dialog_destroy),
                    NULL);

  Create_A_Preview (&lighterPreview, &lighterFrame,
                    reduced->width, reduced->height);
  Create_A_Preview (&middlePreview, &middleFrame,
                    reduced->width, reduced->height);
  Create_A_Preview (&darkerPreview, &darkerFrame,
                    reduced->width, reduced->height);

  Create_A_Table_Entry (&lighterVbox, lighterFrame, val_lighter);
  Create_A_Table_Entry (&middleVbox, middleFrame, current_val);
  Create_A_Table_Entry (&darkerVbox, darkerFrame, val_darker);

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
fp_create_msnls (void)
{
  GtkWidget *table, *lessFrame, *middleFrame, *moreFrame;
  GtkWidget *lessVbox, *middleVbox, *moreVbox;
  GtkWidget *win;

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gimp_help_connect (win, gimp_standard_help_func, HELP_ID, NULL);

  gtk_window_set_title (GTK_WINDOW (win), _("Saturation Variations"));

  g_signal_connect (win, "delete_event",
                    G_CALLBACK (sub_dialog_destroy),
                    NULL);

  Create_A_Preview (&minusSatPreview, &lessFrame,
                    reduced->width, reduced->height);
  Create_A_Preview (&SatPreview, &middleFrame,
                    reduced->width, reduced->height);
  Create_A_Preview (&plusSatPreview, &moreFrame,
                    reduced->width, reduced->height);

  Create_A_Table_Entry (&moreVbox, moreFrame, sat_more);
  Create_A_Table_Entry (&middleVbox, middleFrame, current_val);
  Create_A_Table_Entry (&lessVbox, lessFrame, sat_less);

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

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      refreshPreviews (Current.VisibleFrames);
      if (AW.window && GTK_WIDGET_VISIBLE (AW.window) && AW.rangePreview)
        fp_range_preview_spill (AW.rangePreview,Current.ValueBy);
    }
}

static GtkWidget *
fp_create_pixels_select_by (void)
{
  GtkWidget *frame;

  frame = gimp_int_radio_group_new (TRUE, _("Select Pixels by"),
                                    G_CALLBACK (fp_change_current_pixels_by),
                                    &Current.ValueBy,
                                    Current.ValueBy,

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

  if (GTK_TOGGLE_BUTTON (widget)->active)
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
                                    &Current.SlctnOnly,
                                    Current.SlctnOnly,

                                    _("_Entire Image"),  0, NULL,
                                    _("Se_lection Only"), 1, NULL,
                                    _("Selec_tion In Context"), 2, NULL,

                                    NULL);

  gtk_widget_show (frame);

  return frame;
}

static void
Create_A_Preview (GtkWidget **preview,
                  GtkWidget **frame,
                  gint        previewWidth,
                  gint        previewHeight)
{
  *frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (*frame), GTK_SHADOW_IN);
  gtk_widget_show (*frame);

  *preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (*preview, previewWidth, previewHeight);
  gtk_widget_show (*preview);
  gtk_container_add (GTK_CONTAINER (*frame), *preview);
}

static void
Check_Button_In_A_Box (GtkWidget     *vbox,
                       const gchar   *label,
                       GtkSignalFunc  function,
                       gpointer       data,
                       gboolean       clicked)
{
  GtkWidget *button;

  button = gtk_check_button_new_with_mnemonic (label);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (function),
                    data);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clicked);
}

static void
Frames_Check_Button_In_A_Box (GtkWidget     *vbox,
                              const gchar   *label,
                              GtkSignalFunc  function,
                              GtkWidget     *frame,
                              gboolean       clicked)
{
  GtkWidget *button;

  button = gtk_check_button_new_with_mnemonic (label);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_object_set_data (G_OBJECT (frame), "ctrlButton", (gpointer) button);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (function),
                    frame);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), clicked);
}

static void
Create_A_Table_Entry (GtkWidget   **box,
                      GtkWidget    *SmallerFrame,
                      const gchar  *description)
{
  GtkWidget *label, *button, *table;

  *box = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (*box), PR_BX_BRDR);
  gtk_widget_show (*box);

  /* Delayed translation applied here */
  label = gtk_label_new (gettext (description));

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
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

      gtk_container_add (GTK_CONTAINER (button), SmallerFrame);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (selectionMade),
                        (gchar *) description);
    }
  else
    {
      gtk_table_attach (GTK_TABLE (table), SmallerFrame, 0, 1, 1, 2,
                        0, 0, 0, 4);
    }
}

static void
fp_redraw_all_windows (void)
{
  reduced = Reduce_The_Image (drawable,mask,
                              Current.PreviewSize,
                              Current.SlctnOnly);
  Adjust_Preview_Sizes (reduced->width, reduced->height);

  gtk_widget_queue_draw (fpFrames.palette);
  gtk_widget_queue_draw (fpFrames.satur);
  gtk_widget_queue_draw (fpFrames.lnd);
  gtk_widget_queue_draw (dlg);

  refreshPreviews (Current.VisibleFrames);
}

static void
fp_show_hide_frame (GtkWidget *button,
                    GtkWidget *frame)
{
  gint prev = Current.VisibleFrames;

  if (frame == NULL)
    return;

  if (GTK_TOGGLE_BUTTON (button)->active)
    {
      if (!GTK_WIDGET_VISIBLE (frame))
        {
          gtk_widget_show (frame);

          if (frame==fpFrames.palette)
            Current.VisibleFrames |= HUE;
          else if (frame==fpFrames.satur)
            Current.VisibleFrames |= SATURATION;
          else if (frame==fpFrames.lnd)
            Current.VisibleFrames |= VALUE;

          refreshPreviews (Current.VisibleFrames & ~prev);
          fp_create_smoothness_graph (AW.aliasingPreview);
          fp_range_preview_spill (AW.rangePreview,Current.ValueBy);
        }
    }
  else
    {
      if (GTK_WIDGET_VISIBLE (frame))
        {
          gtk_widget_hide (frame);

          if (frame==fpFrames.palette)
            Current.VisibleFrames &= ~HUE;
          else if (frame==fpFrames.satur)
            Current.VisibleFrames &= ~SATURATION;
          else if (frame==fpFrames.lnd)
            Current.VisibleFrames &= ~VALUE;
        }
    }
}

static void
Adjust_Preview_Sizes (gint width,
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
selectionMade (GtkWidget *widget,
               gpointer   data)
{
  Current.Touched[Current.ValueBy] = 1;

  if (data == (gpointer) hue_red) {
    Update_Current_FP (HUE, RED);
  } else if (data == (gpointer) hue_green) {
    Update_Current_FP (HUE, GREEN);
  } else if (data == (gpointer) hue_blue) {
    Update_Current_FP (HUE, BLUE);
  } else if (data == (gpointer) hue_cyan) {
    Update_Current_FP (HUE, CYAN);
  } else if (data == (gpointer) hue_yellow) {
    Update_Current_FP (HUE, YELLOW);
  } else if (data == (gpointer) hue_magenta) {
    Update_Current_FP (HUE, MAGENTA);
  } else if (data == (gpointer) val_darker) {
    Update_Current_FP (VALUE, DOWN);
  } else if (data == (gpointer) val_lighter) {
    Update_Current_FP (VALUE, UP);
  } else if (data == (gpointer) sat_more) {
    Update_Current_FP (SATURATION, UP);
  } else if (data == (gpointer) sat_less) {
    Update_Current_FP (SATURATION, DOWN);
  }

  refreshPreviews (Current.VisibleFrames);
}

static void
refreshPreviews (gint which)
{
  fp_Create_Nudge (nudgeArray);
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
      resetFilterPacks ();
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
                 float         *scale_val)
{
  static gfloat prevValue = 0.25;

  *scale_val = adjustment->value;

  if (prevValue != adjustment->value)
    {
      fp_Create_Nudge (nudgeArray);
      refreshPreviews (Current.VisibleFrames);
      if (AW.window != NULL && GTK_WIDGET_VISIBLE (AW.window))
        fp_create_smoothness_graph (AW.aliasingPreview);
      prevValue = adjustment->value;
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

  reduced = Reduce_The_Image (drawable,mask,
                              Current.PreviewSize,
                              Current.SlctnOnly);

  gimp_ui_init ("fp", TRUE);

  dlg = gimp_dialog_new (_("Filter Pack Simulation"), "fp",
                         NULL, 0,
                         gimp_standard_help_func, HELP_ID,

                         GIMP_STOCK_RESET, RESPONSE_RESET,
                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  g_signal_connect (dlg, "response",
                    G_CALLBACK (fp_response),
                    dlg);

  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  fp_advanced_dialog ();

  fpFrames.bna          = bna          = fp_create_bna();
  fpFrames.rough        = rough        = fp_create_rough();
  fpFrames.range        = range        = fp_create_range();
  fpFrames.palette      = palette      = fp_create_circle_palette();
  fpFrames.lnd          = lnd          = fp_create_lnd();
  fpFrames.show         = show         = fp_create_show();
  fpFrames.satur        = satur        = fp_create_msnls();
  fpFrames.pixelsBy     = pixelsBy     = fp_create_pixels_select_by();
                          control      = fp_create_control();
  /********************************************************************/
  /********************   PUT EVERYTHING TOGETHER    ******************/

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  gtk_table_attach (GTK_TABLE (table), bna, 0, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), control, 1, 2, 1, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), rough, 1, 2, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), show, 0, 1, 1, 2,
                    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), range, 0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), pixelsBy, 0, 1, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_widget_show (dlg);

  refreshPreviews (Current.VisibleFrames);

  gtk_main ();

  return FPint.run;
}

/***********************************************************/
/************   Advanced Options Window   ******************/
/***********************************************************/

static void
As_You_Drag (GtkWidget *button)
{
  static gboolean notFirstTime = FALSE;

  if (! notFirstTime)
    return;

  notFirstTime = TRUE;

  if (GTK_TOGGLE_BUTTON (button)->active)
    {
      Current.RealTime=TRUE;
      gtk_range_set_update_policy (GTK_RANGE (Current.roughnessScale),0);
      gtk_range_set_update_policy (GTK_RANGE (Current.aliasingScale),0);
      gtk_range_set_update_policy (GTK_RANGE (Current.previewSizeScale),0);
    }
  else
    {
      Current.RealTime=FALSE;
      gtk_range_set_update_policy (GTK_RANGE (Current.roughnessScale),
                                   GTK_UPDATE_DELAYED);
      gtk_range_set_update_policy (GTK_RANGE (Current.aliasingScale),
                                   GTK_UPDATE_DELAYED);
      gtk_range_set_update_policy (GTK_RANGE (Current.previewSizeScale),
                                   GTK_UPDATE_DELAYED);
    }
}

static void
preview_size_scale_update (GtkAdjustment *adjustment,
                           float         *scale_val)
{
  Current.PreviewSize = adjustment->value;
  fp_redraw_all_windows();
}

static void
fp_advanced_dialog (void)
{
  gchar     *rangeNames[] = { N_("Shadows:"),
                              N_("Midtones:"),
                              N_("Highlights:") };
  GtkWidget *frame, *mainvbox;
  GtkObject *smoothnessData;
  GtkWidget *graphFrame, *table, *scale;
  GtkWidget *vbox, *label, *labelTable, *alignment, *inner_vbox;
  gint i;

  AW.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gimp_help_connect (AW.window, gimp_standard_help_func, HELP_ID, NULL);

  gtk_window_set_title (GTK_WINDOW (AW.window),
                        _("Advanced Filter Pack Options"));

  g_signal_connect (AW.window, "delete_event",
                    G_CALLBACK (sub_dialog_destroy),
                    NULL);

  mainvbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (mainvbox), 12);
  gtk_container_add (GTK_CONTAINER (AW.window), mainvbox);
  gtk_widget_show (mainvbox);

  frame = gimp_frame_new (_("Smoothness of Aliasing"));
  gtk_box_pack_start (GTK_BOX (mainvbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 1, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  graphFrame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1, TRUE);
  gtk_frame_set_shadow_type (GTK_FRAME (graphFrame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (graphFrame),0);
  gtk_widget_show (graphFrame);
  gtk_table_attach (GTK_TABLE (table), graphFrame, 0, 1, 0, 1,
                    GTK_EXPAND, 0, 0, 0);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (graphFrame), vbox);
  gtk_widget_show (vbox);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start_defaults (GTK_BOX (vbox), alignment);
  gtk_widget_show (alignment);

  inner_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (alignment), inner_vbox);
  gtk_widget_show (inner_vbox);

  AW.aliasingPreview = gimp_preview_area_new ();
  gtk_widget_set_size_request (AW.aliasingPreview, 256, MAX_ROUGHNESS);
  gtk_box_pack_start (GTK_BOX (inner_vbox), AW.aliasingPreview, TRUE, TRUE, 0);
  gtk_widget_show (AW.aliasingPreview);

  fp_create_smoothness_graph (AW.aliasingPreview);

  AW.rangePreview = gimp_preview_area_new ();
  gtk_widget_set_size_request (AW.rangePreview, 256, RANGE_HEIGHT);
  gtk_box_pack_start(GTK_BOX (inner_vbox), AW.rangePreview, TRUE, TRUE, 0);
  gtk_widget_show (AW.rangePreview);

  fp_range_preview_spill (AW.rangePreview, Current.ValueBy);

  labelTable = gtk_table_new (3, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (labelTable), 6);
  gtk_table_set_row_spacings (GTK_TABLE (labelTable), 6);
  gtk_widget_show (labelTable);
  gtk_table_attach (GTK_TABLE (table), labelTable, 0, 1, 1, 2,
                    GTK_EXPAND, 0, 0, 0);

  for (i = 0; i < 12; i++)
    {
      label = Current.rangeLabels[i] = gtk_label_new ("-");
      if (!(i % 4))
        {
          gtk_label_set_text (GTK_LABEL(label), gettext (rangeNames[i/4]));
          gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
        }
      gtk_widget_show (label);
      gtk_table_attach (GTK_TABLE (labelTable), label, i%4, i%4+1, i/4, i/4+1,
                        GTK_EXPAND | GTK_FILL, 0, 0, 0);
    }

  /************************************************************/

  AW.aliasingGraph = gtk_drawing_area_new ();
  gtk_widget_set_size_request (AW.aliasingGraph,
                               2 * MARGIN + 256,
                               RANGE_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), AW.aliasingGraph, TRUE, TRUE, 0);
  gtk_widget_show (AW.aliasingGraph);
  gtk_widget_set_events (AW.aliasingGraph, RANGE_ADJUST_MASK);

  g_signal_connect (AW.aliasingGraph, "event",
                    G_CALLBACK (FP_Range_Change_Events),
                    &Current);

  /************************************************************/

  smoothnessData = gtk_adjustment_new (Current.Alias, 0, 1.0, 0.05, 0.01, 0.0);

  Current.aliasingScale = scale =
    gtk_hscale_new (GTK_ADJUSTMENT (smoothnessData));
  gtk_widget_set_size_request (scale, 200, -1);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), 0);
  gtk_table_attach (GTK_TABLE (table), scale, 0, 1, 2, 3,
                    0, 0, 0, 0);
  gtk_widget_show (scale);

  g_signal_connect (smoothnessData, "value_changed",
                    G_CALLBACK (fp_scale_update),
                    &Current.Alias);

  /******************* MISC OPTIONS ***************************/

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (mainvbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  Check_Button_In_A_Box (vbox, _("Preview as You Drag"),
                         GTK_SIGNAL_FUNC (As_You_Drag),
                         NULL, TRUE);

  frame = gimp_frame_new (_("Preview Size"));
  gtk_widget_show (frame);

  smoothnessData = gtk_adjustment_new (Current.PreviewSize,
                                       50, MAX_PREVIEW_SIZE,
                                       5, 5, 0.0);

  Current.previewSizeScale = scale =
    gtk_hscale_new (GTK_ADJUSTMENT (smoothnessData));
  gtk_container_add (GTK_CONTAINER (frame), scale);
  gtk_widget_set_size_request (scale, 100, -1);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), 0);
  gtk_widget_show (scale);

  g_signal_connect (smoothnessData, "value_changed",
                    G_CALLBACK (preview_size_scale_update),
                    &Current.PreviewSize);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
}

static void
slider_erase (GdkWindow *window,
              int        xpos)
{
  gdk_window_clear_area (window, MARGIN + xpos - (RANGE_HEIGHT - 1) / 2, 0,
                         RANGE_HEIGHT, RANGE_HEIGHT);
}

static void
draw_slider (GdkWindow *window,
             GdkGC     *border_gc,
             GdkGC     *fill_gc,
             gint       xpos)
{
  gint i;

  for (i = 0; i < RANGE_HEIGHT; i++)
    gdk_draw_line (window, fill_gc, MARGIN + xpos-i/2, i, MARGIN + xpos+i/2,i);

  gdk_draw_line (window, border_gc, MARGIN + xpos, 0,
                 MARGIN + xpos - (RANGE_HEIGHT - 1) / 2, RANGE_HEIGHT - 1);

  gdk_draw_line (window, border_gc, MARGIN + xpos, 0,
                 MARGIN + xpos + (RANGE_HEIGHT - 1) / 2, RANGE_HEIGHT - 1);

  gdk_draw_line (window, border_gc, MARGIN + xpos- (RANGE_HEIGHT - 1)/2,
                 RANGE_HEIGHT-1, MARGIN + xpos + (RANGE_HEIGHT-1)/2,
                 RANGE_HEIGHT - 1);
}

static void
draw_it (GtkWidget *widget)
{
  draw_slider (AW.aliasingGraph->window,
               AW.aliasingGraph->style->black_gc,
               AW.aliasingGraph->style->dark_gc[GTK_STATE_NORMAL],
               Current.Cutoffs[SHADOWS]);

  draw_slider (AW.aliasingGraph->window,
               AW.aliasingGraph->style->black_gc,
               AW.aliasingGraph->style->dark_gc[GTK_STATE_NORMAL],
               Current.Cutoffs[MIDTONES]);

  draw_slider (AW.aliasingGraph->window,
               AW.aliasingGraph->style->black_gc,
               AW.aliasingGraph->style->dark_gc[GTK_STATE_SELECTED],
               Current.Offset);
}

static gboolean
FP_Range_Change_Events (GtkWidget *widget,
                        GdkEvent  *event,
                        FP_Params *current)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint shad, mid, offset, min;
  static guchar  *new;
  gint  x;

  switch (event->type)
    {
    case GDK_EXPOSE:
      draw_it (NULL);
      break;

    case GDK_BUTTON_PRESS:
      bevent= (GdkEventButton *) event;

      shad   =  abs (bevent->x - Current.Cutoffs[SHADOWS]);
      mid    =  abs (bevent->x - Current.Cutoffs[MIDTONES]);
      offset =  abs (bevent->x - Current.Offset);

      min = MIN (MIN (shad, mid), offset);

      if (bevent->x >0 && bevent->x<256)
        {
          if (min == shad)
            new = &Current.Cutoffs[SHADOWS];
          else if (min == mid)
            new = &Current.Cutoffs[MIDTONES];
          else
            new = &Current.Offset;

          slider_erase (AW.aliasingGraph->window, *new);
          *new = bevent->x;
        }

      draw_it (NULL);

      if (Current.RealTime)
        {
          fp_range_preview_spill (AW.rangePreview, Current.ValueBy);
          update_range_labels ();
          fp_create_smoothness_graph (AW.aliasingPreview);
          refreshPreviews (Current.VisibleFrames);
        }
      break;

    case GDK_BUTTON_RELEASE:
      if (!Current.RealTime)
        {
          fp_range_preview_spill (AW.rangePreview, Current.ValueBy);
          update_range_labels ();
          fp_create_smoothness_graph (AW.aliasingPreview);
          refreshPreviews (Current.VisibleFrames);
        }
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      gdk_window_get_pointer (widget->window, &x, NULL, NULL);

      if (x >= 0 && x < 256)
        {
          slider_erase (AW.aliasingGraph->window, *new);
          *new = x;
          draw_it (NULL);
          if (Current.RealTime)
            {
              fp_range_preview_spill (AW.rangePreview, Current.ValueBy);
              update_range_labels ();
              fp_create_smoothness_graph (AW.aliasingPreview);
              refreshPreviews (Current.VisibleFrames);
            }
        }
      break;

    default:
      break;
    }

  return FALSE;
}

static void
update_range_labels (void)
{
  gchar buffer[3];

  gtk_label_set_text (GTK_LABEL(Current.rangeLabels[1]),"0");

  g_snprintf (buffer, sizeof (buffer), "%d", Current.Cutoffs[SHADOWS]);
  gtk_label_set_text (GTK_LABEL (Current.rangeLabels[3]), buffer);
  gtk_label_set_text (GTK_LABEL (Current.rangeLabels[5]), buffer);

  g_snprintf (buffer, sizeof (buffer), "%d", Current.Cutoffs[MIDTONES]);
  gtk_label_set_text (GTK_LABEL (Current.rangeLabels[7]), buffer);
  gtk_label_set_text (GTK_LABEL (Current.rangeLabels[9]), buffer);

  gtk_label_set_text (GTK_LABEL(Current.rangeLabels[11]), "255");
}

static void
initializeFilterPacks (void)
{
  gint i, j;
  for (i = 0; i < 256; i++)
    for (j = BY_HUE; j < JUDGE_BY; j++)
      {
        Current.redAdj   [j][i] = 0;
        Current.greenAdj [j][i] = 0;
        Current.blueAdj  [j][i] = 0;
        Current.satAdj   [j][i] = 0;
      }
}

static void
resetFilterPacks (void)
{
  initializeFilterPacks ();
  refreshPreviews (Current.VisibleFrames);
}

static ReducedImage *
Reduce_The_Image (GimpDrawable *drawable,
                  GimpDrawable *mask,
                  gint LongerSize,
                  gint Slctn)
{
  gint          RH, RW, width, height, bytes=drawable->bpp;
  ReducedImage *temp = (ReducedImage *) malloc (sizeof (ReducedImage));
  guchar       *tempRGB, *src_row, *tempmask, *src_mask_row, R, G, B;
  gint          i, j, whichcol, whichrow, x1, x2, y1, y2;
  GimpPixelRgn  srcPR, srcMask;
  gboolean      NoSelectionMade = TRUE;
  gdouble      *tempHSV;
  GimpRGB       rgb;
  GimpHSV       hsv;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
  width  = x2 - x1;
  height = y2 - y1;

  if (width != drawable->width && height != drawable->height)
    NoSelectionMade = FALSE;

  if (Slctn == 0)
    {
      x1 = 0;
      x2 = drawable->width;
      y1 = 0;
      y2 = drawable->height;
    }
  else if (Slctn == 2)
    {
      x1 = MAX (0,                x1 - width  / 2.0);
      x2 = MIN (drawable->width,  x2 + width  / 2.0);
      y1 = MAX (0,                y1 - height / 2.0);
      y2 = MIN (drawable->height, y2 + height / 2.0);
    }

  width  = x2 - x1;
  height = y2 - y1;

  if (width > height)
    {
      RW = LongerSize;
      RH = (gfloat) height * (gfloat) LongerSize / (gfloat) width;
    }
  else
    {
      RH = LongerSize;
      RW = (gfloat) width * (gfloat) LongerSize / (gfloat) height;
    }

  tempRGB  = (guchar *)  malloc (RW * RH * bytes);
  tempHSV  = (gdouble *) malloc (RW * RH * bytes * sizeof (gdouble));
  tempmask = (guchar *)  malloc (RW * RH);

  gimp_pixel_rgn_init (&srcPR, drawable, x1, y1, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&srcMask, mask, x1, y1, width, height, FALSE, FALSE);

  src_row      = (guchar *) malloc (width * bytes);
  src_mask_row = (guchar *) malloc (width * bytes);

  for (i = 0; i < RH; i++)
    {
      whichrow = (gfloat) i * (gfloat) height / (gfloat) RH;

      gimp_pixel_rgn_get_row (&srcPR, src_row, x1, y1 + whichrow, width);
      gimp_pixel_rgn_get_row (&srcMask, src_mask_row, x1, y1 + whichrow, width);

      for (j = 0; j < RW; j++)
        {
          whichcol = (gfloat) j * (gfloat) width / (gfloat) RW;

          if (NoSelectionMade)
            tempmask[i * RW + j] = 255;
          else
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
            tempRGB[i * RW * bytes + j * bytes + 3] =
              src_row[whichcol * bytes + 3];
        }
    }

  temp->width  = RW;
  temp->height = RH;
  temp->rgb    = tempRGB;
  temp->hsv    = tempHSV;
  temp->mask   = tempmask;

  return temp;
}

static void
fp_render_preview(GtkWidget *preview,
                  gint       changewhat,
                  gint       changewhich)
{
  guchar *a;
  gint Inten, bytes = drawable->bpp;
  gint i, j, k, nudge, M, m, middle,JudgeBy;
  float partial;
  gint RW = reduced->width;
  gint RH = reduced->height;
  gint backupP[3], P[3], tempSat[JUDGE_BY][256];

  a = g_new (guchar, 4*RW*RH);

  if (changewhat==SATURATION)
    for (k=0; k<256; k++) {
      for (JudgeBy=BY_HUE; JudgeBy<JUDGE_BY; JudgeBy++)
        tempSat[JudgeBy][k]=0;
        tempSat[Current.ValueBy][k] += changewhich*nudgeArray[(k+Current.Offset)%256];
    }

  for (i=0; i<RH; i++) {
    for (j=0; j<RW; j++) {

      backupP[0] = P[0]  = (int) reduced->rgb[i*RW*bytes + j*bytes + 0];
      backupP[1] = P[1]  = (int) reduced->rgb[i*RW*bytes + j*bytes + 1];
      backupP[2] = P[2]  = (int) reduced->rgb[i*RW*bytes + j*bytes + 2];

      m = MIN(MIN(P[0],P[1]),P[2]);
      M = MAX(MAX(P[0],P[1]),P[2]);
      middle=(M+m)/2;
      for (k=0; k<3; k++)
        if (P[k]!=m && P[k]!=M) middle=P[k];

      partial = reduced->mask[i*RW+j]/255.0;

      for (JudgeBy=BY_HUE; JudgeBy<JUDGE_BY; JudgeBy++) {
        if (!Current.Touched[JudgeBy]) continue;

        Inten   = reduced->hsv[i*RW*bytes + j*bytes + JudgeBy]*255.0;

        /*DO SATURATION FIRST*/
        if (changewhat != NONEATALL) {
          if (M!=m) {
            for (k=0; k<3; k++)
              if (backupP[k] == M)
                P[k] = MAX(P[k]+partial*Current.satAdj[JudgeBy][Inten],middle);
              else if (backupP[k] == m)
                P[k] = MIN(P[k]-partial*Current.satAdj[JudgeBy][Inten],middle);
          }
          P[0]  += partial*Current.redAdj[JudgeBy][Inten];
          P[1]  += partial*Current.greenAdj[JudgeBy][Inten];
          P[2]  += partial*Current.blueAdj[JudgeBy][Inten];
        }
      }

      Inten   = reduced->hsv[i*RW*bytes + j*bytes + Current.ValueBy]*255.0;
      nudge   = partial*nudgeArray[(Inten+Current.Offset)%256];

      switch (changewhat) {
      case HUE:
        P[0]  += colorSign[RED][changewhich]   * nudge;
        P[1]  += colorSign[GREEN][changewhich] * nudge;
        P[2]  += colorSign[BLUE][changewhich]  * nudge;
        break;

      case SATURATION:
        for (JudgeBy=BY_HUE; JudgeBy<JUDGE_BY; JudgeBy++)
          for (k=0; k<3; k++)
            if (M!=m) {
              if (backupP[k] == M)
                P[k] = MAX(P[k]+
                           partial*tempSat[JudgeBy][Inten],middle);
              else if (backupP[k] == m)
                P[k] = MIN(P[k]-
                           partial*tempSat[JudgeBy][Inten],middle);
            }
        break;

      case VALUE:
        P[0]  += changewhich * nudge;
        P[1]  += changewhich * nudge;
        P[2]  += changewhich * nudge;
        break;

      default:
        break;
      }

      a[(i * RW + j) * 4 + 0] = CLAMP0255(P[0]);
      a[(i * RW + j) * 4 + 1] = CLAMP0255(P[1]);
      a[(i * RW + j) * 4 + 2] = CLAMP0255(P[2]);

      if (bytes == 4)
        a[(i * RW + j) * 4 + 3] = reduced->rgb[i*RW*bytes+j*bytes+3];
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
Update_Current_FP (gint changewhat,
                   gint changewhich)
{
  gint i;

  for (i = 0; i < 256; i++)
    {
      gint nudge;

      fp_Create_Nudge (nudgeArray);
      nudge = nudgeArray[(i + Current.Offset) % 256];

      switch (changewhat) {
      case HUE:
        Current.redAdj[Current.ValueBy][i] +=
          colorSign[RED][changewhich] * nudge;

        Current.greenAdj[Current.ValueBy][i] +=
          colorSign[GREEN][changewhich] * nudge;

        Current.blueAdj[Current.ValueBy][i] +=
          colorSign[BLUE][changewhich]  * nudge;
        break;

      case SATURATION:
        Current.satAdj[Current.ValueBy][i] += changewhich * nudge;
        break;

      case VALUE:
        Current.redAdj[Current.ValueBy][i]   += changewhich * nudge;
        Current.greenAdj[Current.ValueBy][i] += changewhich * nudge;
        Current.blueAdj[Current.ValueBy][i]  += changewhich * nudge;
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

  fp_Create_Nudge(nArray);

  for (i = 0; i < MAX_ROUGHNESS; i++)
    {
      gint coor = MAX_ROUGHNESS - i;
      for (j = 0; j < 256; j++) {
        data[3 * (i * 256 + j) + 0] = 255;
        data[3 * (i * 256 + j) + 1] = 255;
        data[3 * (i * 256 + j) + 2] = 255;
        if (!(i % (MAX_ROUGHNESS / 4))) {
          data[3 * (i * 256 + j) + 0] = 255;
          data[3 * (i * 256 + j) + 1] = 128;
          data[3 * (i * 256 + j) + 2] = 128;
        }
        if (!((j + 1) % 32)) {
          data[3 * (i * 256 + j) + 0] = 255;
          data[3 * (i * 256 + j) + 1] = 128;
          data[3 * (i * 256 + j) + 2] = 128;
        }
        toBeBlack = FALSE;
        if (nArray[j] == coor)
          toBeBlack = TRUE;

        if (j < 255) {
          gint jump = abs (nArray[j] - nArray[j+1]);
          if (abs (coor - nArray[j]) < jump  &&
              abs (coor - nArray[j + 1]) < jump)
            toBeBlack = TRUE;
        }
        if (toBeBlack) {
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
                  data[3 * (i * 256 + j) + 0] = j - Current.Offset;
                  data[3 * (i * 256 + j) + 1] = j - Current.Offset;
                  data[3 * (i * 256 + j) + 2] = j - Current.Offset;
                  break;

                case BY_HUE:
                  gimp_hsv_set (&hsv,
                                ((j - Current.Offset + 256) % 256) / 255.0,
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
                                ((j-(gint)Current.Offset+256)%256) / 255.0,
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
fp_Create_Nudge(gint *adjArray)
{
  gint left, right, middle,i;
  /* The following function was determined by trial and error */
  gdouble Steepness = pow (1 - Current.Alias, 4) * .8;

  left = (Current.Range == SHADOWS) ? 0 : Current.Cutoffs[Current.Range - 1];
  right = Current.Cutoffs[Current.Range];
  middle = (left + right)/2;

  if (Current.Alias)
    for (i = 0; i < 256; i++)
      if (i <= middle)
        adjArray[i] = MAX_ROUGHNESS *
          Current.Rough * (1 + tanh (Steepness * (i - left))) / 2;
      else
        adjArray[i] = MAX_ROUGHNESS *
          Current.Rough * (1 + tanh (Steepness * (right - i))) / 2;
  else
    for (i = 0; i < 256; i++)
      adjArray[i] = (left <= i && i <= right)
        ? MAX_ROUGHNESS * Current.Rough : 0;
}

