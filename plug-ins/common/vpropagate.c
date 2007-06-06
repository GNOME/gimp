/* vpropagate.c -- This is a plug-in for GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <2000-01-09 15:50:46 yasuhiro>
 * Version: 0.89a
 *
 * Copyright (C) 1996-1997 Shuji Narazaki <narazaki@InetQ.or.jp>
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

/* memo
   the initial value of each pixel is the value of the pixel itself.
   To determine whether it is an isolated local peak point, use:
   (self == min && (! modified_flag))   ; modified_flag holds history of update
   In other word, pixel itself is not a neighbor of it.
*/
/*
   in response to bug #156545, after lengthy discussion, the meanings of "dilate"
   and "erode" are being swapped -- 19 May 2006.
*/

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define VPROPAGATE_PROC      "plug-in-vpropagate"
#define ERODE_PROC           "plug-in-erode"
#define DILATE_PROC          "plug-in-dilate"
#define PLUG_IN_BINARY       "vpropagate"
#define PLUG_IN_IMAGE_TYPES  "RGB*, GRAY*"

#define VP_RGB          (1 << 0)
#define VP_GRAY         (1 << 1)
#define VP_WITH_ALPHA   (1 << 2)
#define VP_WO_ALPHA     (1 << 3)
#define num_direction   4
#define Right2Left      0
#define Bottom2Top      1
#define Left2Right      2
#define Top2Bottom      3

static void         query                      (void);
static void         run                        (const gchar       *name,
                                                gint               nparams,
                                                const GimpParam   *param,
                                                gint              *nreturn_vals,
                                                GimpParam        **return_vals);

static GimpPDBStatusType  value_propagate      (GimpDrawable  *drawable);
static void         value_propagate_body       (GimpDrawable  *drawable,
                                                GimpPreview   *preview);
static gboolean     vpropagate_dialog          (GimpDrawable  *drawable);
static void         prepare_row                (GimpPixelRgn  *pixel_rgn,
                                                guchar        *data,
                                                gint           x,
                                                gint           y,
                                                gint           w);

static void    vpropagate_toggle_button_update (GtkWidget     *widget,
                                                gpointer       data);
static GtkWidget *  gtk_table_add_toggle       (GtkWidget     *table,
                                                gchar         *name,
                                                gint           x1,
                                                gint           x2,
                                                gint           y,
                                                GCallback      update,
                                                gint          *value);

static int          value_difference_check  (guchar *, guchar *, int);
static void         set_value               (GimpImageBaseType,
                                             int, guchar *, guchar *, guchar *,
                                             void *);
static void         initialize_white        (GimpImageBaseType,
                                             int, guchar *, guchar *,
                                             void **);
static void         propagate_white         (GimpImageBaseType,
                                             int, guchar *, guchar *, guchar *,
                                             void *);
static void         initialize_black        (GimpImageBaseType,
                                             int, guchar *, guchar *,
                                             void **);
static void         propagate_black         (GimpImageBaseType,
                                             int, guchar *, guchar *, guchar *,
                                             void *);
static void         initialize_middle       (GimpImageBaseType,
                                             int, guchar *, guchar *,
                                             void **);
static void         propagate_middle        (GimpImageBaseType,
                                             int, guchar *, guchar *, guchar *,
                                             void *);
static void         set_middle_to_peak      (GimpImageBaseType,
                                             int, guchar *, guchar *, guchar *,
                                             void *);
static void         set_foreground_to_peak  (GimpImageBaseType,
                                             int, guchar *, guchar *, guchar *,
                                             void *);
static void         initialize_foreground   (GimpImageBaseType,
                                             int, guchar *, guchar *,
                                             void **);
static void         initialize_background   (GimpImageBaseType,
                                             int, guchar *, guchar *,
                                             void **);
static void         propagate_a_color       (GimpImageBaseType,
                                             int, guchar *, guchar *, guchar *,
                                             void *);
static void         propagate_opaque        (GimpImageBaseType,
                                             int, guchar *, guchar *, guchar *,
                                             void *);
static void         propagate_transparent   (GimpImageBaseType,
                                             int, guchar *, guchar *, guchar *,
                                             void *);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

#define SCALE_WIDTH        100
#define PROPAGATING_VALUE  (1 << 0)
#define PROPAGATING_ALPHA  (1 << 1)

/* parameters */
typedef struct
{
  gint     propagate_mode;
  gint     propagating_channel;
  gdouble  propagating_rate;
  gint     direction_mask;
  gint     lower_limit;
  gint     upper_limit;
  gboolean preview;
} VPValueType;

static VPValueType vpvals =
{
  0,        /* propagate_mode                        */
  3,        /* PROPAGATING_VALUE + PROPAGATING_ALPHA */
  1.0,      /* [0.0:1.0]                             */
  15,       /* propagate to all 4 directions         */
  0,        /* lower_limit                           */
  255,      /* upper_limit                           */
  TRUE      /* preview                               */
};

/* dialog variables */
static gint       propagate_alpha;
static gint       propagate_value;
static gint       direction_mask_vec[4];
static gint       channel_mask[] = { 1, 1, 1 };
static gint       peak_max = 1;
static gint       peak_min = 1;
static gint       peak_includes_equals = 1;
static guchar     fore[3];
static GtkWidget *preview;

typedef struct
{
  gint    applicable_image_type;
  gchar  *name;
  void  (*initializer) (GimpImageBaseType, gint, guchar *, guchar *, gpointer *);
  void  (*updater) (GimpImageBaseType, gint, guchar *, guchar *, guchar *, gpointer);
  void  (*finalizer) (GimpImageBaseType, gint, guchar *, guchar *, guchar *, gpointer);
} ModeParam;

#define num_mode 8
static ModeParam modes[num_mode] =
{
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("More _white (larger value)"),
    initialize_white,      propagate_white,       set_value },
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("More blac_k (smaller value)"),
    initialize_black,      propagate_black,       set_value },
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("_Middle value to peaks"),
    initialize_middle,     propagate_middle,      set_middle_to_peak },
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("_Foreground to peaks"),
    initialize_middle,     propagate_middle,      set_foreground_to_peak },
  { VP_RGB | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("O_nly foreground"),
    initialize_foreground, propagate_a_color,     set_value },
  { VP_RGB | VP_WITH_ALPHA | VP_WO_ALPHA,
    N_("Only b_ackground"),
    initialize_background, propagate_a_color,     set_value },
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA,
    N_("Mor_e opaque"),
    NULL,                  propagate_opaque,      set_value },
  { VP_RGB | VP_GRAY | VP_WITH_ALPHA,
    N_("More t_ransparent"),
    NULL,                  propagate_transparent, set_value },
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",            "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",               "Input image (not used)" },
    { GIMP_PDB_DRAWABLE, "drawable",            "Input drawable" },
    { GIMP_PDB_INT32,    "propagate-mode",      "propagate 0:white, 1:black, 2:middle value 3:foreground to peak, 4:foreground, 5:background, 6:opaque, 7:transparent" },
    { GIMP_PDB_INT32,    "propagating-channel", "channels which values are propagated" },
    { GIMP_PDB_FLOAT,    "propagating-rate",    "0.0 <= propagatating_rate <= 1.0" },
    { GIMP_PDB_INT32,    "direction-mask",      "0 <= direction-mask <= 15" },
    { GIMP_PDB_INT32,    "lower-limit",         "0 <= lower-limit <= 255" },
    { GIMP_PDB_INT32,    "upper-limit",         "0 <= upper-limit <= 255" }
  };

  gimp_install_procedure (VPROPAGATE_PROC,
                          N_("Propagate certain colors to neighboring pixels"),
                          "Propagate values of the layer",
                          "Shuji Narazaki (narazaki@InetQ.or.jp)",
                          "Shuji Narazaki",
                          "1996-1997",
                          N_("_Value Propagate..."),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_install_procedure (ERODE_PROC,
                          N_("Shrink darker areas of the image"),
                          "Erode image",
                          "Shuji Narazaki (narazaki@InetQ.or.jp)",
                          "Shuji Narazaki",
                          "1996-1997",
                          N_("E_rode"),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_install_procedure (DILATE_PROC,
                          N_("Grow darker areas of the image"),
                          "Dilate image",
                          "Shuji Narazaki (narazaki@InetQ.or.jp)",
                          "Shuji Narazaki",
                          "1996-1997",
                          N_("_Dilate"),
                          PLUG_IN_IMAGE_TYPES,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (VPROPAGATE_PROC, "<Image>/Filters/Distorts");
  gimp_plugin_menu_register (ERODE_PROC,      "<Image>/Filters/Generic");
  gimp_plugin_menu_register (DILATE_PROC,     "<Image>/Filters/Generic");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  GimpDrawable      *drawable;

  run_mode = param[0].data.d_int32;
  drawable = gimp_drawable_get (param[2].data.d_int32);

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      if (strcmp (name, VPROPAGATE_PROC) == 0)
        {
          gimp_get_data (VPROPAGATE_PROC, &vpvals);
          /* building the values of dialog variables from vpvals. */
          propagate_alpha =
            (vpvals.propagating_channel & PROPAGATING_ALPHA) ? TRUE : FALSE;
          propagate_value =
            (vpvals.propagating_channel & PROPAGATING_VALUE) ? TRUE : FALSE;

          {
            gint i;
            for (i = 0; i < 4; i++)
              direction_mask_vec[i] =
                (vpvals.direction_mask & (1 << i)) ? TRUE : FALSE;
          }

          if (! vpropagate_dialog (drawable))
            return;
        }
      else if (strcmp (name, ERODE_PROC) == 0 ||
               strcmp (name, DILATE_PROC) == 0)
        {
          vpvals.propagating_channel = PROPAGATING_VALUE;
          vpvals.propagating_rate    = 1.0;
          vpvals.direction_mask      = 15;
          vpvals.lower_limit         = 0;
          vpvals.upper_limit         = 255;

          if (strcmp (name, ERODE_PROC) == 0)
            vpvals.propagate_mode = 1;
          else if (strcmp (name, DILATE_PROC) == 0)
            vpvals.propagate_mode = 0;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      if (strcmp (name, VPROPAGATE_PROC) == 0)
        {
          vpvals.propagate_mode      = param[3].data.d_int32;
          vpvals.propagating_channel = param[4].data.d_int32;
          vpvals.propagating_rate    = param[5].data.d_float;
          vpvals.direction_mask      = param[6].data.d_int32;
          vpvals.lower_limit         = param[7].data.d_int32;
          vpvals.upper_limit         = param[8].data.d_int32;
        }
      else if (strcmp (name, ERODE_PROC) == 0 ||
               strcmp (name, DILATE_PROC) == 0)
        {
          vpvals.propagating_channel = PROPAGATING_VALUE;
          vpvals.propagating_rate    = 1.0;
          vpvals.direction_mask      = 15;
          vpvals.lower_limit         = 0;
          vpvals.upper_limit         = 255;

          if (strcmp (name, ERODE_PROC) == 0)
            vpvals.propagate_mode = 1;
          else if (strcmp (name, DILATE_PROC) == 0)
            vpvals.propagate_mode = 0;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (name, &vpvals);
      break;
    }

  status = value_propagate (drawable);

  if (status == GIMP_PDB_SUCCESS)
    {
      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (name, &vpvals, sizeof (VPValueType));
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

/* registered function entry */
static GimpPDBStatusType
value_propagate (GimpDrawable *drawable)
{
  /* check the validness of parameters */
  if (!(vpvals.propagating_channel & (PROPAGATING_VALUE | PROPAGATING_ALPHA)))
    {
      /* gimp_message ("No channel selected."); */
      return GIMP_PDB_EXECUTION_ERROR;
    }
  if (vpvals.direction_mask == 0)
    {
      /* gimp_message ("No direction selected."); */
      return GIMP_PDB_EXECUTION_ERROR;
    }
  if ((vpvals.lower_limit < 0) || (255 < vpvals.lower_limit) ||
       (vpvals.upper_limit < 0) || (255 < vpvals.lower_limit) ||
       (vpvals.upper_limit < vpvals.lower_limit))
    {
      /* gimp_message ("Limit values are not valid."); */
      return GIMP_PDB_EXECUTION_ERROR;
    }
  value_propagate_body (drawable, NULL);
  return GIMP_PDB_SUCCESS;
}

static void
value_propagate_body (GimpDrawable *drawable,
                      GimpPreview  *preview)
{
  GimpImageBaseType  dtype;
  ModeParam          operation;
  GimpPixelRgn       srcRgn, destRgn;
  guchar            *here, *best, *dest;
  guchar            *dest_row, *prev_row, *cur_row, *next_row;
  guchar            *pr, *cr, *nr, *swap;
  gint               width, height, bytes, index;
  gint               begx, begy, endx, endy, x, y, dx;
  gint               left_index, right_index, up_index, down_index;
  gpointer           tmp;
  GimpRGB            foreground;

  /* calculate neighbors' indexes */
  left_index  = (vpvals.direction_mask & (1 << Left2Right)) ? -1 : 0;
  right_index = (vpvals.direction_mask & (1 << Right2Left)) ?  1 : 0;
  up_index    = (vpvals.direction_mask & (1 << Top2Bottom)) ? -1 : 0;
  down_index  = (vpvals.direction_mask & (1 << Bottom2Top)) ?  1 : 0;
  operation   = modes[vpvals.propagate_mode];
  tmp         = NULL;

  dtype = gimp_drawable_type (drawable->drawable_id);
  bytes = drawable->bpp;

  /* Here I use the algorithm of blur.c . */
  if (preview)
    {
       gimp_preview_get_position (preview, &begx, &begy);
       gimp_preview_get_size (preview, &width, &height);
       endx = begx + width;
       endy = begy + height;
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id,
                                 &begx, &begy, &endx, &endy);
      width  = endx - begx;
      height = endy - begy;
    }

  gimp_tile_cache_ntiles (2 * ((width) / gimp_tile_width () + 1));

  prev_row = g_new (guchar, (width + 2) * bytes);
  cur_row  = g_new (guchar, (width + 2) * bytes);
  next_row = g_new (guchar, (width + 2) * bytes);
  dest_row = g_new (guchar, width * bytes);

  gimp_pixel_rgn_init (&srcRgn, drawable,
                       begx, begy, width, height,
                       FALSE, FALSE);
  gimp_pixel_rgn_init (&destRgn, drawable,
                       begx, begy, width, height,
                       (preview == NULL), TRUE);

  pr = prev_row + bytes;
  cr = cur_row + bytes;
  nr = next_row + bytes;

  prepare_row (&srcRgn, pr, begx, (0 < begy) ? begy : begy - 1, endx-begx);
  prepare_row (&srcRgn, cr, begx, begy, endx-begx);

  best = g_new (guchar, bytes);

  if (!preview)
    gimp_progress_init (_("Value Propagate"));

  gimp_context_get_foreground (&foreground);
  gimp_rgb_get_uchar (&foreground, fore+0, fore+1, fore+2);

  /* start real job */
  for (y = begy ; y < endy ; y++)
    {
      prepare_row (&srcRgn, nr, begx, ((y+1) < endy) ? y+1 : endy, endx-begx);

      for (index = 0; index < (endx - begx) * bytes; index++)
        dest_row[index] = cr[index];

      for (x = 0 ; x < endx - begx; x++)
        {
          dest = dest_row + (x * bytes);
          here = cr + (x * bytes);

          /* *** copy source value to best value holder *** */
          memcpy (best, here, bytes);

          if (operation.initializer)
            (* operation.initializer)(dtype, bytes, best, here, &tmp);

          /* *** gather neighbors' values: loop-unfolded version *** */
          if (up_index == -1)
            for (dx = left_index ; dx <= right_index ; dx++)
              (* operation.updater)(dtype, bytes, here, pr+((x+dx)*bytes), best, tmp);
          for (dx = left_index ; dx <= right_index ; dx++)
            if (dx != 0)
              (* operation.updater)(dtype, bytes, here, cr+((x+dx)*bytes), best, tmp);
          if (down_index == 1)
            for (dx = left_index ; dx <= right_index ; dx++)
              (* operation.updater)(dtype, bytes, here, nr+((x+dx)*bytes), best, tmp);
          /* *** store it to dest_row*** */
          (* operation.finalizer)(dtype, bytes, best, here, dest, tmp);
        }

      /* now store destline to destRgn */
      gimp_pixel_rgn_set_row (&destRgn, dest_row, begx, y, endx - begx);

      /* shift the row pointers  */
      swap = pr;
      pr = cr;
      cr = nr;
      nr = swap;


      if (((y % 5) == 0) && !preview)
        gimp_progress_update ((gdouble) y / (gdouble) (endy - begy));
    }

  if (preview)
    {
      gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                         &destRgn);
    }
  else
    {
      /*  update the region  */
      gimp_progress_update(1.0);
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, begx, begy, endx-begx, endy-begy);
    }
}

static void
prepare_row (GimpPixelRgn *pixel_rgn,
             guchar       *data,
             gint          x,
             gint          y,
             gint          w)
{
  gint b;

  if (y <= 0)
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, (y + 1), w);
  else if (y >= pixel_rgn->h)
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, (y - 1), w);
  else
    gimp_pixel_rgn_get_row (pixel_rgn, data, x, y, w);

  /*  Fill in edge pixels  */
  for (b = 0; b < pixel_rgn->bpp; b++)
    {
      data[-(gint)pixel_rgn->bpp + b] = data[b];
      data[w * pixel_rgn->bpp + b] = data[(w - 1) * pixel_rgn->bpp + b];
    }
}

static void
set_value (GimpImageBaseType  dtype,
           gint               bytes,
           guchar            *best,
           guchar            *here,
           guchar            *dest,
           void              *tmp)
{
  gint  value_chs = 0;
  gint  alpha = 0;
  gint  ch;

  switch (dtype)
    {
    case GIMP_RGB_IMAGE:
      value_chs = 3;
      break;
    case GIMP_RGBA_IMAGE:
      value_chs = 3;
      alpha = 3;
      break;
    case GIMP_GRAY_IMAGE:
      value_chs = 1;
      break;
    case GIMP_GRAYA_IMAGE:
      value_chs = 1;
      alpha = 1;
      break;
    default:
      break;
    }
  for (ch = 0; ch < value_chs; ch++)
    {
      if (vpvals.propagating_channel & PROPAGATING_VALUE) /* value channel */
        *dest++ = (guchar)(vpvals.propagating_rate * best[ch]
                       + (1.0 - vpvals.propagating_rate) * here[ch]);
      else
        *dest++ = here[ch];
    }
  if (alpha)
    {
      if (vpvals.propagating_channel & PROPAGATING_ALPHA) /* alpha channel */
        *dest++ = (guchar)(vpvals.propagating_rate * best[alpha]
                       + (1.0 - vpvals.propagating_rate) * here[alpha]);
      else
        *dest++ = here[alpha];
    }
}

static int
value_difference_check (guchar *pos1,
                        guchar *pos2,
                        gint   ch)
{
  gint  index;
  int   diff;

  for (index = 0 ; index < ch; index++)
    if (channel_mask[index] != 0)
      {
        diff = abs(pos1[index] - pos2[index]);
        if (! ((vpvals.lower_limit <= diff) && (diff <= vpvals.upper_limit)))
          return 0;
      }
  return 1;
}

/* mothods for each mode */
static void
initialize_white (GimpImageBaseType   dtype,
                  gint                bytes,
                  guchar             *best,
                  guchar             *here,
                  void              **tmp)
{
  if (*tmp == NULL)
    *tmp = (void *) g_new (gfloat, 1);
  *(float *)*tmp = sqrt (channel_mask[0] * here[0] * here[0]
                         + channel_mask[1] * here[1] * here[1]
                         + channel_mask[2] * here[2] * here[2]);
}

static void
propagate_white (GimpImageBaseType  dtype,
                 gint               bytes,
                 guchar            *orig,
                 guchar            *here,
                 guchar            *best,
                 void              *tmp)
{
  float v_here;

  switch (dtype)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      v_here = sqrt (channel_mask[0] * here[0] * here[0]
                     + channel_mask[1] * here[1] * here[1]
                     + channel_mask[2] * here[2] * here[2]);
     if (*(float *)tmp < v_here && value_difference_check(orig, here, 3))
        {
          *(float *)tmp = v_here;
          memcpy(best, here, 3 * sizeof(guchar)); /* alpha channel holds old value */
        }
      break;
    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      if (*best < *here && value_difference_check(orig, here, 1))
        *best = *here;
      break;
    default:
      break;
    }
}

static void
initialize_black (GimpImageBaseType   dtype,
                  gint                channels,
                  guchar             *best,
                  guchar             *here,
                  void              **tmp)
{
  if (*tmp == NULL)
    *tmp = (void *) g_new (gfloat, 1);
  *(float *)*tmp = sqrt (channel_mask[0] * here[0] * here[0]
                         + channel_mask[1] * here[1] * here[1]
                         + channel_mask[2] * here[2] * here[2]);
}

static void
propagate_black (GimpImageBaseType  image_type,
                 gint               channels,
                 guchar            *orig,
                 guchar            *here,
                 guchar            *best,
                 void              *tmp)
{
  float v_here;

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      v_here = sqrt (channel_mask[0] * here[0] * here[0]
                     + channel_mask[1] * here[1] * here[1]
                     + channel_mask[2] * here[2] * here[2]);
      if (v_here < *(float *)tmp && value_difference_check(orig, here, 3))
        {
          *(float *)tmp = v_here;
          memcpy (best, here, 3 * sizeof(guchar)); /* alpha channel holds old value */
        }
      break;
    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      if (*here < *best && value_difference_check(orig, here, 1))
        *best = *here;
      break;
    default:
      break;
    }
}

typedef struct
{
  gshort min_modified;
  gshort max_modified;
  glong  original_value;
  glong  minv;
  guchar min[3];
  glong  maxv;
  guchar max[3];
} MiddlePacket;

static void
initialize_middle (GimpImageBaseType   image_type,
                   gint                channels,
                   guchar             *best,
                   guchar             *here,
                   void              **tmp)
{
  int index;
  MiddlePacket *data;

  if (*tmp == NULL)
    *tmp = (void *) g_new (MiddlePacket, 1);
  data = (MiddlePacket *)*tmp;
  for (index = 0; index < channels; index++)
    data->min[index] = data->max[index] = here[index];
  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      data->original_value = sqrt (channel_mask[0] * here[0] * here[0]
                                   + channel_mask[1] * here[1] * here[1]
                                   + channel_mask[2] * here[2] * here[2]);
      break;
    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      data->original_value = *here;
      break;
    default:
      break;
    }
  data->minv = data->maxv = data->original_value;
  data->min_modified = data->max_modified = 0;
}

static void
propagate_middle (GimpImageBaseType  image_type,
                  gint               channels,
                  guchar            *orig,
                  guchar            *here,
                  guchar            *best,
                  void              *tmp)
{
  float v_here;
  MiddlePacket *data;

  data = (MiddlePacket *)tmp;

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      v_here = sqrt (channel_mask[0] * here[0] * here[0]
                     + channel_mask[1] * here[1] * here[1]
                     + channel_mask[2] * here[2] * here[2]);
      if ((v_here <= data->minv) && value_difference_check(orig, here, 3))
        {
          data->minv = v_here;
          memcpy (data->min, here, 3*sizeof(guchar));
          data->min_modified = 1;
        }
      if (data->maxv <= v_here && value_difference_check(orig, here, 3))
        {
          data->maxv = v_here;
          memcpy (data->max, here, 3*sizeof(guchar));
          data->max_modified = 1;
        }
      break;
    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      if ((*here <= data->min[0]) && value_difference_check(orig, here, 1))
        {
          data->min[0] = *here;
          data->min_modified = 1;
        }
      if ((data->max[0] <= *here) && value_difference_check(orig, here, 1))
        {
          data->max[0] = *here;
          data->max_modified = 1;
        }
      break;
    default:
      break;
    }
}

static void
set_middle_to_peak (GimpImageBaseType  image_type,
                    gint               channels,
                    guchar            *here,
                    guchar            *best,
                    guchar            *dest,
                    void              *tmp)
{
  gint  value_chs = 0;
  gint  alpha = 0;
  gint  ch;
  MiddlePacket  *data;

  data = (MiddlePacket *)tmp;
  if (! ((peak_min & (data->minv == data->original_value))
         || (peak_max & (data->maxv == data->original_value))))
    return;
  if ((! peak_includes_equals)
      && ((peak_min & (! data->min_modified))
          || (peak_max & (! data->max_modified))))
      return;

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
      value_chs = 3;
      break;
    case GIMP_RGBA_IMAGE:
      value_chs = 3;
      alpha = 3;
      break;
    case GIMP_GRAY_IMAGE:
      value_chs = 1;
      break;
    case GIMP_GRAYA_IMAGE:
      value_chs = 1;
      alpha = 1;
      break;
    default:
      break;
    }
  for (ch = 0; ch < value_chs; ch++)
    {
      if (vpvals.propagating_channel & PROPAGATING_VALUE) /* value channel */
        *dest++ = (guchar)(vpvals.propagating_rate * 0.5 * (data->min[ch] + data->max[ch])
                       + (1.0 - vpvals.propagating_rate) * here[ch]);
      else
        *dest++ = here[ch];
    }
  if (alpha)
    {
      if (vpvals.propagating_channel & PROPAGATING_ALPHA) /* alpha channel */
        *dest++ = (guchar)(vpvals.propagating_rate * best[alpha]
                       + (1.0 - vpvals.propagating_rate) * here[alpha]);
      else
        *dest++ = here[alpha];
    }
}

static void
set_foreground_to_peak (GimpImageBaseType  image_type,
                        gint               channels,
                        guchar            *here,
                        guchar            *best,
                        guchar            *dest,
                        void              *tmp)
{
  gint  value_chs = 0;
  gint  alpha = 0;
  gint  ch;
  MiddlePacket  *data;

  data = (MiddlePacket *)tmp;
  if (! ((peak_min & (data->minv == data->original_value))
         || (peak_max & (data->maxv == data->original_value))))
    return;
  if (peak_includes_equals
      && ((peak_min & (! data->min_modified))
          || (peak_max & (! data->max_modified))))
      return;

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
      value_chs = 3;
      break;
    case GIMP_RGBA_IMAGE:
      value_chs = 3;
      alpha = 3;
      break;
    case GIMP_GRAY_IMAGE:
      value_chs = 1;
      break;
    case GIMP_GRAYA_IMAGE:
      value_chs = 1;
      alpha = 1;
      break;
    default:
      break;
    }
  for (ch = 0; ch < value_chs; ch++)
    {
      if (vpvals.propagating_channel & PROPAGATING_VALUE) /* value channel */
        *dest++ = (guchar)(vpvals.propagating_rate*fore[ch]
                       + (1.0 - vpvals.propagating_rate)*here[ch]);
      else
        *dest++ = here[ch];
    }
}

static void
initialize_foreground (GimpImageBaseType   image_type,
                       gint                channels,
                       guchar             *here,
                       guchar             *best,
                       void              **tmp)
{
  GimpRGB  foreground;
  guchar  *ch;

  if (*tmp == NULL)
    {
      *tmp = (void *) g_new (guchar, 3);
      ch = (guchar *)*tmp;
      gimp_context_get_foreground (&foreground);
      gimp_rgb_get_uchar (&foreground, &ch[0], &ch[1], &ch[2]);
    }
}

static void
initialize_background (GimpImageBaseType   image_type,
                       gint                channels,
                       guchar             *here,
                       guchar             *best,
                       void              **tmp)
{
  GimpRGB  background;
  guchar  *ch;

  if (*tmp == NULL)
    {
      *tmp = (void *) g_new (guchar, 3);
      ch = (guchar *)*tmp;
      gimp_context_get_background (&background);
      gimp_rgb_get_uchar (&background, &ch[0], &ch[1], &ch[2]);
    }
}

static void
propagate_a_color (GimpImageBaseType  image_type,
                   gint               channels,
                   guchar            *orig,
                   guchar            *here,
                   guchar            *best,
                   void              *tmp)
{
  guchar *fg = (guchar *)tmp;

  switch (image_type)
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      if (here[0] == fg[0] && here[1] == fg[1] && here[2] == fg[2] &&
          value_difference_check(orig, here, 3))
        {
          memcpy (best, here, 3 * sizeof(guchar)); /* alpha channel holds old value */
        }
      break;
    case GIMP_GRAYA_IMAGE:
    case GIMP_GRAY_IMAGE:
      break;
    default:
      break;
    }
}

static void
propagate_opaque (GimpImageBaseType  image_type,
                  gint               channels,
                  guchar            *orig,
                  guchar            *here,
                  guchar            *best,
                  void              *tmp)
{
  switch (image_type)
    {
    case GIMP_RGBA_IMAGE:
      if (best[3] < here[3] && value_difference_check(orig, here, 3))
        memcpy(best, here, channels * sizeof(guchar));
      break;
    case GIMP_GRAYA_IMAGE:
      if (best[1] < here[1] && value_difference_check(orig, here, 1))
        memcpy(best, here, channels * sizeof(guchar));
      break;
    default:
      break;
    }
}

static void
propagate_transparent (GimpImageBaseType  image_type,
                       gint               channels,
                       guchar            *orig,
                       guchar            *here,
                       guchar            *best,
                       void              *tmp)
{
  switch (image_type)
    {
    case GIMP_RGBA_IMAGE:
      if (here[3] < best[3] && value_difference_check(orig, here, 3))
        memcpy(best, here, channels * sizeof(guchar));
      break;
    case GIMP_GRAYA_IMAGE:
      if (here[1] < best[1] && value_difference_check(orig, here, 1))
        memcpy(best, here, channels * sizeof(guchar));
      break;
    default:
      break;
    }
}

/* dialog stuff */

static gboolean
vpropagate_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *toggle_vbox;
  GtkWidget *button;
  GtkObject *adj;
  GSList    *group = NULL;
  gint       index = 0;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Value Propagate"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, VPROPAGATE_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, &vpvals.preview);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (value_propagate_body),
                            drawable);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Propagate Mode */
  frame = gimp_frame_new (_("Mode"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);
  gtk_widget_show (toggle_vbox);

  for (index = 0; index < num_mode; index++)
    {
      button =
        gtk_radio_button_new_with_mnemonic (group,
                                            gettext (modes[index].name));
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (toggle_vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_object_set_data (G_OBJECT (button), "gimp-item-data",
                         GINT_TO_POINTER (index));

      g_signal_connect (button, "toggled",
                        G_CALLBACK (gimp_radio_button_update),
                        &vpvals.propagate_mode);
      g_signal_connect_swapped (button, "toggled",
                                G_CALLBACK (gimp_preview_invalidate),
                                preview);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    index == vpvals.propagate_mode);
    }

  /* Parameter settings */
  frame = gimp_frame_new (_("Propagate"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (8, 3, FALSE); /* 4 raw, 2 columns(name and value) */
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 12);
  gtk_table_set_row_spacing (GTK_TABLE (table), 5, 12);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              _("Lower t_hreshold:"), SCALE_WIDTH, 4,
                              vpvals.lower_limit, 0, 255, 1, 8, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &vpvals.lower_limit);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                              _("_Upper threshold:"), SCALE_WIDTH, 4,
                              vpvals.upper_limit, 0, 255, 1, 8, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &vpvals.upper_limit);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                              _("_Propagating rate:"), SCALE_WIDTH, 4,
                              vpvals.propagating_rate, 0, 1, 0.01, 0.1, 2,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vpvals.propagating_rate);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gtk_table_add_toggle (table, _("To l_eft"), 0, 1, 4,
                        G_CALLBACK (vpropagate_toggle_button_update),
                        &direction_mask_vec[Right2Left]);
  gtk_table_add_toggle (table, _("To _right"), 2, 3, 4,
                        G_CALLBACK (vpropagate_toggle_button_update),
                        &direction_mask_vec[Left2Right]);
  gtk_table_add_toggle (table, _("To _top"), 1, 2, 3,
                        G_CALLBACK (vpropagate_toggle_button_update),
                        &direction_mask_vec[Bottom2Top]);
  gtk_table_add_toggle (table, _("To _bottom"), 1, 2, 5,
                        G_CALLBACK (vpropagate_toggle_button_update),
                        &direction_mask_vec[Top2Bottom]);

  if (gimp_drawable_has_alpha (drawable->drawable_id))
    {
      GtkWidget *toggle;

      toggle =
        gtk_table_add_toggle (table, _("Propagating _alpha channel"),
                              0, 3, 6,
                              G_CALLBACK (vpropagate_toggle_button_update),
                              &propagate_alpha);

      if (gimp_layer_get_lock_alpha (drawable->drawable_id))
        {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), 0);
          gtk_widget_set_sensitive (toggle, FALSE);
        }

      gtk_table_add_toggle (table, _("Propagating value channel"), 0, 3, 7,
                            G_CALLBACK (vpropagate_toggle_button_update),
                            &propagate_value);
    }

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      gint i, result;

      for (i = result = 0; i < 4; i++)
        result |= (direction_mask_vec[i] ? 1 : 0) << i;
      vpvals.direction_mask = result;

      vpvals.propagating_channel = ((propagate_alpha ? PROPAGATING_ALPHA : 0) |
                                    (propagate_value ? PROPAGATING_VALUE : 0));
    }

  gtk_widget_destroy (dialog);

  return run;
}

static void
vpropagate_toggle_button_update (GtkWidget *widget,
                                 gpointer   data)
{
  gint i, result;

  gimp_toggle_button_update (widget, data);

  for (i = result = 0; i < 4; i++)
    result |= (direction_mask_vec[i] ? 1 : 0) << i;
  vpvals.direction_mask = result;

  vpvals.propagating_channel = ((propagate_alpha ? PROPAGATING_ALPHA : 0) |
                                (propagate_value ? PROPAGATING_VALUE : 0));
  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static GtkWidget *
gtk_table_add_toggle (GtkWidget *table,
                      gchar     *name,
                      gint       x1,
                      gint       x2,
                      gint       y,
                      GCallback  update,
                      gint      *value)
{
  GtkWidget *toggle;

  toggle = gtk_check_button_new_with_mnemonic(name);
  gtk_table_attach (GTK_TABLE (table), toggle, x1, x2, y, y+1,
                    GTK_FILL|GTK_EXPAND, 0, 0, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (update),
                    value);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *value);

  return toggle;
}
