/* scatter_hsv.c -- This is a plug-in for the GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <2000-01-08 02:49:39 yasuhiro>
 * Version: 0.42
 *
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_NAME "plug_in_scatter_hsv"
#define SHORT_NAME   "scatter_hsv"
#define HELP_ID      "plug-in-scatter-hsv"

static void   query (void);
static void   run   (const gchar      *name,
                     gint              nparams,
                     const GimpParam  *param,
                     gint             *nreturn_vals,
                     GimpParam       **return_vals);

static GimpPDBStatusType scatter_hsv   (gint32  drawable_id);
static void        scatter_hsv_scatter (guchar *r,
                                        guchar *g,
                                        guchar *b);
static gint        randomize_value     (gint    now,
                                        gint    min,
                                        gint    max,
                                        gint    mod_p,
                                        gint    rand_max);

static gboolean scatter_hsv_dialog         (void);
static gboolean preview_event_handler      (GtkWidget     *widget,
                                            GdkEvent      *event);
static void     scatter_hsv_preview_update (void);
static void     scatter_hsv_iscale_update  (GtkAdjustment *adjustment,
                                            gpointer       data);

#define PROGRESS_UPDATE_NUM 100
#define PREVIEW_WIDTH       128
#define PREVIEW_HEIGHT      128
#define SCALE_WIDTH         100
#define ENTRY_WIDTH           3

static gint preview_width  = PREVIEW_WIDTH;
static gint preview_height = PREVIEW_HEIGHT;


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

typedef struct
{                               /* gint, gdouble, and so on */
  gint  holdness;
  gint  hue_distance;
  gint  saturation_distance;
  gint  value_distance;
} ValueType;

static ValueType VALS =
{
  2,
  3,
  10,
  10
};

static gint      drawable_id;

static GtkWidget *preview;
static gint       preview_start_x = 0;
static gint       preview_start_y = 0;
static guchar    *preview_buffer = NULL;
static gint       preview_offset_x = 0;
static gint       preview_offset_y = 0;
static gint       preview_dragging = FALSE;
static gint       preview_drag_start_x = 0;
static gint       preview_drag_start_y = 0;

MAIN ()

static void
query (void)
{
  static GimpParamDef args [] =
  {
    { GIMP_PDB_INT32,    "run_mode",            "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",               "Input image (not used)" },
    { GIMP_PDB_DRAWABLE, "drawable",            "Input drawable" },
    { GIMP_PDB_INT32,    "holdness",            "convolution strength" },
    { GIMP_PDB_INT32,    "hue_distance",        "distribution distance on hue axis [0,255]" },
    { GIMP_PDB_INT32,    "saturation_distance", "distribution distance on saturation axis [0,255]" },
    { GIMP_PDB_INT32,    "value_distance",      "distribution distance on value axis [0,255]" }
  };

  gimp_install_procedure (PLUG_IN_NAME,
                          "Scattering pixel values in HSV space",
                          "Scattering pixel values in HSV space",
                          "Shuji Narazaki (narazaki@InetQ.or.jp)",
                          "Shuji Narazaki",
                          "1997",
                          N_("S_catter HSV..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/Filters/Noise");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_EXECUTION_ERROR;
  GimpRunMode       run_mode;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;
  drawable_id = param[2].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_NAME, &VALS);
      if (!gimp_drawable_is_rgb (drawable_id))
        {
          g_message ("Cannot operate on non-RGB drawables.");
          return;
        }
      if (! scatter_hsv_dialog ())
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      VALS.holdness = param[3].data.d_int32;
      VALS.hue_distance = param[4].data.d_int32;
      VALS.saturation_distance = param[5].data.d_int32;
      VALS.value_distance = param[6].data.d_int32;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_NAME, &VALS);
      break;
    }

  status = scatter_hsv (drawable_id);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush();
  if (run_mode == GIMP_RUN_INTERACTIVE && status == GIMP_PDB_SUCCESS )
    gimp_set_data (PLUG_IN_NAME, &VALS, sizeof (ValueType));

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
scatter_hsv_func (const guchar *src,
                  guchar       *dest,
                  gint          bpp,
                  gpointer      data)
{
  guchar h, s, v;

  h = src[0];
  s = src[1];
  v = src[2];

  scatter_hsv_scatter (&h, &s, &v);

  dest[0] = h;
  dest[1] = s;
  dest[2] = v;

  if (bpp == 4)
    dest[3] = src[3];
}

static GimpPDBStatusType
scatter_hsv (gint32 drawable_id)
{
  GimpDrawable *drawable;

  drawable = gimp_drawable_get (drawable_id);

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

  gimp_progress_init (_("Scattering HSV..."));

  gimp_rgn_iterate2 (drawable, 0 /* unused */, scatter_hsv_func, NULL);

  gimp_drawable_detach (drawable);

  return GIMP_PDB_SUCCESS;
}

static gint
randomize_value (gint now,
                 gint min,
                 gint max,
                 gint mod_p,
                 gint rand_max)
{
  gint    flag, new, steps, index;
  gdouble rand_val;

  steps = max - min + 1;
  rand_val = g_random_double ();
  for (index = 1; index < VALS.holdness; index++)
    {
      double tmp = g_random_double ();
      if (tmp < rand_val)
        rand_val = tmp;
    }

  if (g_random_double () < 0.5)
    flag = -1;
  else
    flag = 1;

  new = now + flag * ((int) (rand_max * rand_val) % steps);

  if (new < min)
    {
      if (mod_p == 1)
        new += steps;
      else
        new = min;
    }
  if (max < new)
    {
      if (mod_p == 1)
        new -= steps;
      else
        new = max;
    }
  return new;
}

static void scatter_hsv_scatter (guchar *r,
                                 guchar *g,
                                 guchar *b)
{
  gint h, s, v;
  gint h1, s1, v1;
  gint h2, s2, v2;

  h = *r; s = *g; v = *b;

  gimp_rgb_to_hsv_int (&h, &s, &v);

  if (0 < VALS.hue_distance)
    h = randomize_value (h, 0, 255, 1, VALS.hue_distance);
  if ((0 < VALS.saturation_distance))
    s = randomize_value (s, 0, 255, 0, VALS.saturation_distance);
  if ((0 < VALS.value_distance))
    v = randomize_value (v, 0, 255, 0, VALS.value_distance);

  h1 = h; s1 = s; v1 = v;

  gimp_hsv_to_rgb_int (&h, &s, &v); /* don't believe ! */

  h2 = h; s2 = s; v2 = v;

  gimp_rgb_to_hsv_int (&h2, &s2, &v2); /* h2 should be h1. But... */

  if ((abs (h1 - h2) <= VALS.hue_distance)
      && (abs (s1 - s2) <= VALS.saturation_distance)
      && (abs (v1 - v2) <= VALS.value_distance))
    {
      *r = h;
      *g = s;
      *b = v;
    }
}

/* dialog stuff */
static gboolean
scatter_hsv_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *pframe;
  GtkWidget *abox;
  GtkWidget *table;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (SHORT_NAME, TRUE);

  dlg = gimp_dialog_new (_("Scatter HSV"), SHORT_NAME,
                         NULL, 0,
                         gimp_standard_help_func, HELP_ID,

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), vbox);
  gtk_widget_show (vbox);

  frame = gimp_frame_new (_("Right-Click Preview to Jump"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  abox = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), pframe);
  gtk_widget_show (pframe);

  preview = gimp_preview_area_new ();
  {
    gint width  = gimp_drawable_width (drawable_id);
    gint height = gimp_drawable_height (drawable_id);

    preview_width  = (PREVIEW_WIDTH  < width)  ? PREVIEW_WIDTH  : width;
    preview_height = (PREVIEW_HEIGHT < height) ? PREVIEW_HEIGHT : height;
  }
  gtk_widget_set_size_request (preview, preview_width * 2, preview_height);
  gtk_container_add (GTK_CONTAINER (pframe), preview);
  gtk_widget_set_events (preview,
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON_MOTION_MASK |
                         GDK_POINTER_MOTION_HINT_MASK);
  gtk_widget_show (preview);

  g_signal_connect (preview, "event",
                    G_CALLBACK (preview_event_handler),
                    NULL);

  gtk_widget_show (frame);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("_Holdness:"), SCALE_WIDTH, ENTRY_WIDTH,
                              VALS.holdness, 1, 8, 1, 2, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (scatter_hsv_iscale_update),
                    &VALS.holdness);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("H_ue:"), SCALE_WIDTH, ENTRY_WIDTH,
                              VALS.hue_distance, 0, 255, 1, 8, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (scatter_hsv_iscale_update),
                    &VALS.hue_distance);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                              _("_Saturation:"), SCALE_WIDTH, ENTRY_WIDTH,
                              VALS.saturation_distance, 0, 255, 1, 8, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (scatter_hsv_iscale_update),
                    &VALS.saturation_distance);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                              _("_Value:"), SCALE_WIDTH, ENTRY_WIDTH,
                              VALS.value_distance, 0, 255, 1, 8, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (scatter_hsv_iscale_update),
                    &VALS.value_distance);

  gtk_widget_show (dlg);

  scatter_hsv_preview_update ();

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

static gboolean
preview_event_handler (GtkWidget *widget,
                       GdkEvent  *event)
{
  gint            x, y;
  gint            dx, dy;
  GdkEventButton *bevent;

  gtk_widget_get_pointer (widget, &x, &y);

  bevent = (GdkEventButton *) event;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (x < preview_width)
        {
          if (bevent->button == 3)
            {
              preview_offset_x = - x;
              preview_offset_y = - y;
              scatter_hsv_preview_update ();
            }
          else
            {
              preview_dragging = TRUE;
              preview_drag_start_x = x;
              preview_drag_start_y = y;
              gtk_grab_add (widget);
            }
        }
      break;
    case GDK_BUTTON_RELEASE:
      if (preview_dragging)
        {
          gtk_grab_remove (widget);
          preview_dragging = FALSE;
          scatter_hsv_preview_update ();
        }
      break;
    case GDK_MOTION_NOTIFY:
      if (preview_dragging)
        {
          dx = x - preview_drag_start_x;
          dy = y - preview_drag_start_y;

          preview_drag_start_x = x;
          preview_drag_start_y = y;

          if ((dx == 0) && (dy == 0))
            break;

          preview_offset_x = MAX (preview_offset_x - dx, 0);
          preview_offset_y = MAX (preview_offset_y - dy, 0);
          scatter_hsv_preview_update ();
        }
      break;
    default:
      break;
    }
  return FALSE;
}

static void
scatter_hsv_preview_update (void)
{
  GimpDrawable *drawable;
  GimpPixelRgn  src_rgn;
  gint          scale;
  gint          x, y, dx, dy;
  gint          bound_start_x, bound_start_y, bound_end_x, bound_end_y;
  gint          src_has_alpha = FALSE;
  gint          src_is_gray = FALSE;
  gint          src_bpp, src_bpl;
  guchar        data[3];
  gdouble       shift_rate;
  guchar       *buffer;

  drawable = gimp_drawable_get (drawable_id);
  gimp_drawable_mask_bounds (drawable_id,
                             &bound_start_x, &bound_start_y,
                             &bound_end_x, &bound_end_y);
  src_has_alpha  = gimp_drawable_has_alpha (drawable_id);
  src_is_gray =  gimp_drawable_is_gray (drawable_id);
  src_bpp = (src_is_gray ? 1 : 3) + (src_has_alpha ? 1 : 0);
  src_bpl = preview_width * src_bpp;

  buffer = g_new (guchar, 2 * preview_width * preview_height * 3);
  
  if (! preview_buffer)
    preview_buffer = g_new (guchar, src_bpl * preview_height);

  if (preview_offset_x < 0)
    preview_offset_x = (bound_end_x - bound_start_x) * (- preview_offset_x) /  preview_width;
  if (preview_offset_y < 0)
    preview_offset_y = (bound_end_y - bound_start_y) * (- preview_offset_y) /  preview_height;
  preview_start_x = CLAMP (bound_start_x + preview_offset_x,
                           bound_start_x, MAX (bound_end_x - preview_width, 0));
  preview_start_y = CLAMP (bound_start_y + preview_offset_y,
                           bound_start_y, MAX (bound_end_y - preview_height, 0));
  if (preview_start_x == bound_start_x)
    preview_offset_x = 0;
  if (preview_start_y == bound_start_y)
    preview_offset_y =0;

  gimp_pixel_rgn_init (&src_rgn, drawable, preview_start_x, preview_start_y,
                       preview_width, preview_height,
                       FALSE, FALSE);

  /* Since it's small, get whole data before processing. */
  gimp_pixel_rgn_get_rect (&src_rgn, preview_buffer,
                           preview_start_x, preview_start_y,
                           preview_width, preview_height);

  scale = 4;
  shift_rate = (gdouble) (scale - 1) / ( 2 * scale);
  for (y = 0; y < preview_height/4; y++)
    {
      for (x = 0; x < preview_width/4; x++)
        {
          gint pos;
          gint  i;

          pos = (gint)(y + preview_height * shift_rate) * src_bpl
                + (gint)(x + preview_width * shift_rate) * src_bpp;

          for (i = 0; i < src_bpp; i++)
            data[i] = preview_buffer[pos + i];

          scatter_hsv_scatter (data+0, data+1, data+2);
          for (dy = 0; dy < scale; dy++)
            for (dx = 0; dx < scale; dx++)
              memcpy ( buffer+((y*scale+dy)*2*preview_width+preview_width+x*scale+dx)*3,
                       data, 3);
        }
    }
  for (y = 0; y < preview_height; y ++)
    for (x = 0; x < preview_width; x++)
      {
        gint    i;

        for (i = 0; i < src_bpp; i++)
          data[i] = preview_buffer[y * src_bpl + x * src_bpp + i];

        scatter_hsv_scatter (data+0, data+1, data+2);
        memcpy (buffer+(y*2*preview_width+x)*3, data, 3);
      }
  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                          0, 0, preview_width * 2, preview_height,
                          GIMP_RGB_IMAGE,
                          buffer,
                          preview_width * 2 * 3);
  g_free (buffer);
}

static void
scatter_hsv_iscale_update (GtkAdjustment *adjustment,
                           gpointer       data)
{
  gimp_int_adjustment_update (adjustment, data);

  scatter_hsv_preview_update ();
}
