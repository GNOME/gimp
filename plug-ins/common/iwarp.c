/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* IWarp  a plug-in for GIMP
   Version 0.1

   IWarp is a gimp plug-in for interactive image warping. To apply the
   selected deformation to the image, press the left mouse button and
   move the mouse pointer in the preview image.

   Copyright (C) 1997 Norbert Schmitz
   nobert.schmitz@student.uni-tuebingen.de

   Most of the gimp and gtk specific code is taken from other plug-ins

   v0.11a
    animation of non-alpha layers (background) creates now layers with
    alpha channel. (thanks to Adrian Likins for reporting this bug)

  v0.12
    fixes a very bad bug.
     (thanks to Arthur Hagen for reporting it)
  v0.13
    2005 - changed to scale preview with the window;
           João S. O. Bueno Calligaris

*/

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC           "plug-in-iwarp"
#define PLUG_IN_BINARY         "iwarp"
#define RESPONSE_RESET         1

#define MAX_DEFORM_AREA_RADIUS 250

#define SCALE_WIDTH            100
#define MAX_NUM_FRAMES         100

typedef enum
{
  GROW,
  SHRINK,
  MOVE,
  REMOVE,
  SWIRL_CCW,
  SWIRL_CW
} DeformMode;

typedef struct
{
  gboolean  run;
} iwarp_interface;

typedef struct
{
  gint        deform_area_radius;
  gdouble     deform_amount;
  DeformMode  deform_mode;
  gboolean    do_bilinear;
  gboolean    do_supersample;
  gdouble     supersample_threshold;
  gint        max_supersample_depth;
} iwarp_vals_t;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void      iwarp                    (void);
static void      iwarp_frame              (void);

static gboolean  iwarp_dialog             (void);
static void      iwarp_animate_dialog     (GtkWidget *dlg,
                                           GtkWidget *notebook);

static void      iwarp_settings_dialog    (GtkWidget *dlg,
                                           GtkWidget *notebook);

static void      iwarp_response           (GtkWidget *widget,
                                           gint       response_id,
                                           gpointer   data);

static void      iwarp_realize_callback   (GtkWidget *widget);
static gboolean  iwarp_motion_callback    (GtkWidget *widget,
                                           GdkEvent  *event);
static void      iwarp_resize_callback    (GtkWidget *widget);

static void      iwarp_update_preview     (gint       x0,
                                           gint       y0,
                                           gint       x1,
                                           gint       y1);

static void      iwarp_get_pixel          (gint       x,
                                           gint       y,
                                           guchar    *pixel);

static void      iwarp_get_deform_vector  (gdouble    x,
                                           gdouble    y,
                                           gdouble   *xv,
                                           gdouble   *yv);

static void      iwarp_get_point          (gdouble    x,
                                           gdouble    y,
                                           guchar    *color);

static gint      iwarp_supersample_test   (GimpVector2 *v0,
                                           GimpVector2 *v1,
                                           GimpVector2 *v2,
                                           GimpVector2 *v3);

static void      iwarp_getsample          (GimpVector2  v0,
                                           GimpVector2  v1,
                                           GimpVector2  v2,
                                           GimpVector2  v3,
                                           gdouble      x,
                                           gdouble      y,
                                           gint        *sample,
                                           gint        *cc,
                                           gint         depth,
                                           gdouble      scale);

static void      iwarp_supersample        (gint       sxl,
                                           gint       syl,
                                           gint       sxr,
                                           gint       syr,
                                           guchar    *dest_data,
                                           gint       stride,
                                           gint      *progress,
                                           gint       max_progress);

static void      iwarp_cpy_images         (void);

static void      iwarp_preview_get_pixel  (gint       x,
                                           gint       y,
                                           guchar   **color);

static void      iwarp_preview_get_point  (gdouble    x,
                                           gdouble    y,
                                           guchar    *color);

static void      iwarp_deform             (gint       x,
                                           gint       y,
                                           gdouble    vx,
                                           gdouble    vy);

static void      iwarp_move               (gint       x,
                                           gint       y,
                                           gint       xx,
                                           gint       yy);

static void     iwarp_scale_preview       (gint       new_width,
                                           gint       new_height,
                                           gint       old_width,
                                           gint       old_height);
static void     iwarp_preview_build       (GtkWidget *vbox);

static void     iwarp_init                (void);
static void     iwarp_preview_init        (void);



const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static iwarp_interface wint =
{
  FALSE
};

static iwarp_vals_t iwarp_vals =
{
  20,
  0.3,
  MOVE,
  TRUE,
  FALSE,
  2.0,
  2
};


static GimpDrawable *drawable = NULL;
static GimpDrawable *destdrawable = NULL;
static GtkWidget    *preview = NULL;
static guchar       *srcimage = NULL;
static guchar       *dstimage = NULL;
static gint          preview_width, preview_height;
static gint          sel_width, sel_height;
static gint          image_bpp;
static gint          lock_alpha;
static GimpVector2  *deform_vectors = NULL;
static GimpVector2  *deform_area_vectors = NULL;
static gint          lastx, lasty;
static gdouble       filter[MAX_DEFORM_AREA_RADIUS];
static gboolean      do_animate = FALSE;
static gboolean      do_animate_reverse = FALSE;
static gboolean      do_animate_ping_pong = FALSE;
static gdouble       supersample_threshold_2;
static gint          xl, yl, xh, yh;
static gint          tile_width, tile_height;
static GimpTile     *tile = NULL;
static gdouble       pre2img, img2pre;
static gint          preview_bpp;
static gdouble       animate_deform_value = 1.0;
static gint32        imageID;
static gint          animate_num_frames = 2;
static gint          frame_number;
static gboolean      layer_alpha;
static gint          max_current_preview_width  = 320;
static gint          max_current_preview_height = 320;
static gint          resize_idle = 0;


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"               }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Use mouse control to warp image areas"),
                          "Interactive warping of the specified drawable",
                          "Norbert Schmitz",
                          "Norbert Schmitz",
                          "1997",
                          N_("_IWarp..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Distorts");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  /*  Get the specified drawable  */
  destdrawable = drawable = gimp_drawable_get (param[2].data.d_drawable);
  imageID = param[1].data.d_int32;

  /*  Make sure that the drawable is grayscale or RGB color  */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_get_data (PLUG_IN_PROC, &iwarp_vals);
          gimp_tile_cache_ntiles (2 * (drawable->width + gimp_tile_width ()-1) /
                                  gimp_tile_width ());
          if (iwarp_dialog())
            iwarp();
          gimp_set_data (PLUG_IN_PROC, &iwarp_vals, sizeof (iwarp_vals_t));
          gimp_displays_flush ();
          break;

        case GIMP_RUN_NONINTERACTIVE:
          status = GIMP_PDB_CALLING_ERROR;
        break;

        default:
          break;
        }
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);

  g_free (srcimage);
  g_free (dstimage);
  g_free (deform_vectors);
  g_free (deform_area_vectors);
}

static void
iwarp_get_pixel (gint    x,
                 gint    y,
                 guchar *pixel)
{
 static gint  old_col = -1;
 static gint  old_row = -1;
 guchar      *data;
 gint         col, row;
 gint         i;

 if (x >= xl && x < xh && y  >= yl && y < yh)
   {
     col = x / tile_width;
     row = y / tile_height;

     if (col != old_col || row != old_row)
       {
         if (tile)
           gimp_tile_unref (tile, FALSE);

         tile = gimp_drawable_get_tile (drawable, FALSE, row, col);
         gimp_tile_ref (tile);

         old_col = col;
         old_row = row;
       }

     data = tile->data + (tile->ewidth * (y % tile_height) +
                          x % tile_width) * image_bpp;

     for (i = 0; i < image_bpp; i++)
       *pixel++ = *data++;
   }
 else
   {
     pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
   }
}

static void
iwarp_get_deform_vector (gdouble  x,
                         gdouble  y,
                         gdouble *xv,
                         gdouble *yv)
{
  gint    i, xi, yi;
  gdouble dx, dy, my0, my1, mx0, mx1;

  if (x >= 0 && x < (preview_width - 1) && y >= 0  && y < (preview_height - 1))
    {
      xi = (gint) x;
      yi = (gint) y;
      dx = x-xi;
      dy = y-yi;
      i = (yi * preview_width + xi);
      mx0 =
        deform_vectors[i].x +
        (deform_vectors[i+1].x -
         deform_vectors[i].x) * dx;
      mx1 =
        deform_vectors[i+preview_width].x +
        (deform_vectors[i+preview_width+1].x -
         deform_vectors[i+preview_width].x) * dx;
      my0 =
        deform_vectors[i].y +
        dx * (deform_vectors[i+1].y -
              deform_vectors[i].y);
      my1 =
        deform_vectors[i+preview_width].y +
        dx * (deform_vectors[i+preview_width+1].y -
              deform_vectors[i+preview_width].y);
      *xv = mx0 + dy * (mx1 - mx0);
      *yv = my0 + dy * (my1 - my0);
    }
  else
    {
      *xv = *yv = 0.0;
    }
}

static void
iwarp_get_point (gdouble  x,
                 gdouble  y,
                 guchar  *color)
{
  gdouble dx, dy, m0, m1;
  guchar  p0[4], p1[4], p2[4], p3[4];
  gint    xi, yi, i;

  xi = (gint) x;
  yi = (gint) y;
  dx = x - xi;
  dy = y - yi;
  iwarp_get_pixel (xi, yi, p0);
  iwarp_get_pixel (xi + 1, yi, p1);
  iwarp_get_pixel (xi, yi + 1, p2);
  iwarp_get_pixel (xi + 1, yi + 1, p3);
  if (layer_alpha)
    {
      gdouble a0 = p0[image_bpp-1];
      gdouble a1 = p1[image_bpp-1];
      gdouble a2 = p2[image_bpp-1];
      gdouble a3 = p3[image_bpp-1];
      gdouble alpha;

      m0 = a0 + dx * (a1 - a0);
      m1 = a2 + dx * (a3 - a2);
      alpha = (m0 + dy * (m1 - m0));
      color[image_bpp-1] = alpha;
      if (color[image_bpp-1])
        {
          for (i = 0; i < image_bpp-1; i++)
            {
              m0 = a0*p0[i] + dx * (a1*p1[i] - a0*p0[i]);
              m1 = a2*p2[i] + dx * (a3*p3[i] - a2*p2[i]);
              color[i] = (m0 + dy * (m1 - m0))/alpha;
            }
        }
    }
  else
    {
      for (i = 0; i < image_bpp; i++)
        {
          m0 = p0[i] + dx * (p1[i] - p0[i]);
          m1 = p2[i] + dx * (p3[i] - p2[i]);
          color[i] = m0 + dy * (m1 - m0);
        }
    }
}

static gboolean
iwarp_supersample_test (GimpVector2 *v0,
                        GimpVector2 *v1,
                        GimpVector2 *v2,
                        GimpVector2 *v3)
{
  gdouble dx, dy;

  dx = 1.0 + v1->x - v0->x;
  dy = v1->y - v0->y;
  if (SQR(dx) + SQR(dy) > supersample_threshold_2)
    return TRUE;

  dx = 1.0 + v2->x - v3->x;
  dy = v2->y - v3->y;
  if (SQR(dx) + SQR(dy) > supersample_threshold_2)
    return TRUE;

  dx = v2->x - v0->x;
  dy = 1.0 + v2->y - v0->y;
  if (SQR(dx) + SQR(dy) > supersample_threshold_2)
    return TRUE;

  dx = v3->x - v1->x;
  dy = 1.0 + v3->y - v1->y;
  if (SQR(dx) + SQR(dy) > supersample_threshold_2)
    return TRUE;

  return FALSE;
}

static void
iwarp_getsample (GimpVector2  v0,
                 GimpVector2  v1,
                 GimpVector2  v2,
                 GimpVector2  v3,
                 gdouble      x,
                 gdouble      y,
                 gint        *sample,
                 gint        *cc,
                 gint         depth,
                 gdouble      scale)
{
  gint        i;
  gdouble     xv, yv;
  GimpVector2 v01, v13, v23, v02, vm;
  guchar      c[4];

  if ((depth >= iwarp_vals.max_supersample_depth) ||
      (!iwarp_supersample_test (&v0, &v1, &v2, &v3)))
    {
      iwarp_get_deform_vector (img2pre * (x - xl),
                               img2pre * (y - yl),
                               &xv, &yv);
      xv *= animate_deform_value;
      yv *= animate_deform_value;
      iwarp_get_point (pre2img * xv + x, pre2img * yv + y, c);
      for (i = 0;  i < image_bpp; i++)
        sample[i] += c[i];
      (*cc)++;
    }
 else
   {
     scale *= 0.5;
     iwarp_get_deform_vector (img2pre * (x - xl),
                              img2pre * (y - yl),
                              &xv, &yv);
     xv *= animate_deform_value;
     yv *= animate_deform_value;
     iwarp_get_point (pre2img * xv + x, pre2img * yv + y, c);
     for (i = 0;  i < image_bpp; i++)
       sample[i] += c[i];
     (*cc)++;
     vm.x = xv;
     vm.y = yv;

     iwarp_get_deform_vector (img2pre * (x - xl),
                              img2pre * (y - yl - scale),
                              &xv, &yv);
     xv *= animate_deform_value;
     yv *= animate_deform_value;
     v01.x = xv;
     v01.y = yv;

     iwarp_get_deform_vector (img2pre * (x - xl + scale),
                              img2pre * (y - yl),
                              &xv, &yv);
     xv *= animate_deform_value;
     yv *= animate_deform_value;
     v13.x = xv;
     v13.y = yv;

     iwarp_get_deform_vector (img2pre * (x - xl),
                              img2pre * (y - yl + scale),
                              &xv, &yv);
     xv *= animate_deform_value;
     yv *= animate_deform_value;
     v23.x = xv;
     v23.y = yv;

     iwarp_get_deform_vector (img2pre * (x - xl - scale),
                              img2pre * (y - yl),
                              &xv, &yv);
     xv *= animate_deform_value;
     yv *= animate_deform_value;
     v02.x = xv;
     v02.y = yv;

     iwarp_getsample (v0, v01, vm, v02,
                      x-scale, y-scale,
                      sample, cc, depth + 1,
                      scale);
     iwarp_getsample (v01, v1, v13, vm,
                      x + scale, y - scale,
                      sample, cc, depth + 1,
                      scale);
     iwarp_getsample (v02, vm, v23, v2,
                      x - scale, y + scale,
                      sample, cc, depth + 1,
                      scale);
     iwarp_getsample (vm, v13, v3, v23,
                      x + scale, y + scale,
                      sample, cc, depth + 1,
                      scale);
   }
}

static void
iwarp_supersample (gint    sxl,
                   gint    syl,
                   gint    sxr,
                   gint    syr,
                   guchar *dest_data,
                   gint    stride,
                   gint   *progress,
                   gint    max_progress)
{
  gint         i, wx, wy, col, row, cc;
  GimpVector2 *srow, *srow_old, *vh;
  gdouble      xv, yv;
  gint         color[4];
  guchar      *dest;

  wx = sxr - sxl + 1;
  wy = syr - syl + 1;
  srow     = g_new (GimpVector2, sxr - sxl + 1);
  srow_old = g_new (GimpVector2, sxr - sxl + 1);

  for (i = sxl; i < (sxr + 1); i++)
    {
      iwarp_get_deform_vector (img2pre * (-0.5 + i - xl),
                               img2pre * (-0.5 + syl - yl),
                               &xv, &yv);
      xv *= animate_deform_value;
      yv *= animate_deform_value;
      srow_old[i-sxl].x = xv;
      srow_old[i-sxl].y = yv;
    }

  for (col = syl; col < syr; col++)
    {
      iwarp_get_deform_vector (img2pre * (-0.5 + sxl - xl),
                               img2pre * (0.5 + col - yl),
                               &xv, &yv);
      xv *= animate_deform_value;
      yv *= animate_deform_value;
      srow[0].x = xv;
      srow[0].y = yv;
      for (row = sxl; row <sxr; row++)
        {
          iwarp_get_deform_vector (img2pre * (0.5 + row - xl),
                                   img2pre * (0.5 + col - yl),
                                   &xv, &yv);
          xv *= animate_deform_value;
          yv *= animate_deform_value;
          srow[row-sxl+1].x = xv;
          srow[row-sxl+1].y = yv;
          cc = 0;
          color[0] = color[1] = color[2] = color[3] = 0;
          iwarp_getsample (srow_old[row-sxl], srow_old[row-sxl+1],
                           srow[row-sxl], srow[row-sxl+1],
                           row, col, color, &cc, 0, 1.0);

          dest = dest_data + (col - syl) * stride + (row - sxl) * image_bpp;

          for (i = 0; i < image_bpp; i++)
            *dest++ = color[i] / cc;

          (*progress)++;
        }

      vh = srow_old;
      srow_old = srow;
      srow = vh;
    }

  gimp_progress_update ((gdouble) (*progress) / max_progress);

  g_free (srow);
  g_free (srow_old);
}

static void
iwarp_frame (void)
{
  GimpPixelRgn  dest_rgn;
  gpointer   pr;
  guchar    *dest_row, *dest;
  gint       row, col;
  gint       i;
  gint       progress, max_progress;
  guchar     color[4];
  gdouble    xv, yv;
  gboolean   padding;

  progress = 0;
  max_progress = (yh-yl)*(xh-xl);

  gimp_pixel_rgn_init (&dest_rgn, destdrawable,
                       xl, yl, xh-xl, yh-yl, TRUE, TRUE);

  /* If the source drawable doesn't have an alpha channel but the
     destination drawable has (happens with animations), we need
     to pad the alpha channel of the destination drawable.
   */
  padding = (!layer_alpha &&
             gimp_drawable_has_alpha (destdrawable->drawable_id));

  if (!do_animate)
    gimp_progress_init (_("Warping"));

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      dest_row = dest_rgn.data;
      if (!iwarp_vals.do_supersample)
        {
          for (row = dest_rgn.y; row < (dest_rgn.y + dest_rgn.h); row++)
            {
              dest = dest_row;
              for (col = dest_rgn.x; col < (dest_rgn.x + dest_rgn.w); col++)
                {
                  progress++;
                  iwarp_get_deform_vector (img2pre * (col -xl),
                                           img2pre * (row -yl),
                                           &xv, &yv);
                  xv *= animate_deform_value;
                  yv *= animate_deform_value;
                  if (fabs(xv) > 0.0 || fabs(yv) > 0.0)
                    {
                      iwarp_get_point (pre2img * xv + col,
                                       pre2img * yv + row,
                                       color);

                      for (i = 0; i < image_bpp; i++)
                        *dest++ = color[i];
                    }
                  else
                    {
                      iwarp_get_pixel (col, row, color);

                      for (i = 0; i < image_bpp; i++)
                        *dest++ = color[i];
                    }

                  if (padding)
                    *dest++ = 255;
                }

              dest_row += dest_rgn.rowstride;
            }
	  gimp_progress_update ((gdouble) (progress) / max_progress);
        }
      else
        {
          supersample_threshold_2 =
            iwarp_vals.supersample_threshold * iwarp_vals.supersample_threshold;
          iwarp_supersample (dest_rgn.x, dest_rgn.y,
                             dest_rgn.x + dest_rgn.w, dest_rgn.y + dest_rgn.h,
                             dest_rgn.data,
                             dest_rgn.rowstride,
                             &progress, max_progress);
        }
    }

  gimp_drawable_flush (destdrawable);
  gimp_drawable_merge_shadow (destdrawable->drawable_id, TRUE);
  gimp_drawable_update (destdrawable->drawable_id, xl, yl, (xh - xl), (yh - yl));
}

static void
iwarp (void)
{
  gint     i;
  gint32   layerID;
  gint32  *animlayers;
  gdouble  delta;

  if (image_bpp == 1 || image_bpp == 3)
    layer_alpha = FALSE;
  else
    layer_alpha = TRUE;

  if (animate_num_frames > 1 && do_animate)
    {
      animlayers = g_new (gint32, animate_num_frames);
      if (do_animate_reverse)
        {
          animate_deform_value = 1.0;
          delta = -1.0 / (animate_num_frames - 1);

          gimp_image_undo_group_start (imageID);
        }
      else
        {
          animate_deform_value = 0.0;
          delta = 1.0 / (animate_num_frames - 1);
        }
      layerID = gimp_image_get_active_layer (imageID);
      frame_number = 0;
      for (i = 0; i < animate_num_frames; i++)
        {
          gchar *st = g_strdup_printf (_("Frame %d"), i);

          animlayers[i] = gimp_layer_copy (layerID);
          gimp_layer_add_alpha (animlayers[i]);
          gimp_drawable_set_name (animlayers[i], st);
          g_free (st);

          gimp_image_add_layer (imageID, animlayers[i], 0);

          destdrawable = gimp_drawable_get (animlayers[i]);

          gimp_progress_init_printf (_("Warping Frame %d"),
                                     frame_number);

          if (animate_deform_value > 0.0)
            iwarp_frame ();

          animate_deform_value = animate_deform_value + delta;
          frame_number++;
        }

      if (do_animate_ping_pong)
        {
          gimp_progress_init (_("Ping pong"));

          for (i = 0; i < animate_num_frames; i++)
            {
              gchar *st;

              gimp_progress_update ((gdouble) i / (animate_num_frames - 1));
              layerID = gimp_layer_copy (animlayers[animate_num_frames-i-1]);

              gimp_image_undo_group_end (imageID);

              gimp_layer_add_alpha (layerID);
              st = g_strdup_printf (_("Frame %d"), i + animate_num_frames);
              gimp_drawable_set_name (layerID, st);
              g_free (st);

              gimp_image_add_layer (imageID, layerID, 0);
            }
        }
      g_free (animlayers);
    }
  else
    {
      animate_deform_value = 1.0;
      iwarp_frame ();
    }

  if (tile)
    {
      gimp_tile_unref (tile, FALSE);
      tile = NULL;
    }
}

static void
iwarp_cpy_images (void)
{
  memcpy (dstimage, srcimage, preview_width * preview_height * preview_bpp);
}

static void
iwarp_scale_preview (gint new_width,
                     gint new_height,
                     gint old_width,
                     gint old_height)
{
  gint     x, y, z;
  gdouble  ox, oy, dx, dy;
  gint     src1, src2, ix, iy;
  gdouble  in0, in1, in2;
  guchar  *new_data;

  new_data = g_new (guchar, new_width * new_height * preview_bpp);

  for (y = 0; y < new_height; y++)
    for (x = 0; x < new_width; x++)
      {
        ox = ((gdouble) x / new_width) * old_width;
        oy = ((gdouble) y / new_height) * old_height;

        ix = (gint) ox;
        iy = (gint) oy;

        dx = ox - ix;
        dy = oy - iy;

        if (ix == old_width - 1)
          dx = 0.0;

        for (z = 0; z < preview_bpp; z++)
          {
            src1 = (iy * old_width + ix) * preview_bpp + z;

            if (iy != old_height - 1)
              src2 = src1 + old_width * preview_bpp;
            else
              src2 = src1;

            in0 = dstimage [src1] + (dstimage [src1 + preview_bpp] -
                                     dstimage [src1]) * dx;
            in1 = dstimage [src2] + (dstimage [src2 + preview_bpp] -
                                     dstimage [src2]) * dx;
            in2 = in0 + (in1 - in0) * dy;

            new_data[(y * new_width +  x) * preview_bpp + z] = (guchar) in2;
          }
      }

  g_free (dstimage);
  dstimage = new_data;
}

static void
iwarp_preview_init (void)
{
  gint       y, x, xi, i;
  GimpPixelRgn  srcrgn;
  guchar    *pts;
  guchar    *linebuffer = NULL;
  gdouble    dx, dy;


  dx = (gdouble) sel_width / max_current_preview_width;
  dy = (gdouble) sel_height / max_current_preview_height;

  if (dx > dy)
    pre2img = dx;
  else
    pre2img = dy;

  if (dx <= 1.0 && dy <= 1.0)
    pre2img = 1.0;

  img2pre = 1.0 / pre2img;

  preview_width  = (gint) (sel_width  / pre2img);
  preview_height = (gint) (sel_height / pre2img);


  if (srcimage)
    {
      srcimage = g_renew (guchar,
                          srcimage, preview_width * preview_height * image_bpp);
    }
  else
    {
      srcimage = g_new (guchar, preview_width * preview_height * image_bpp);
      dstimage = g_new (guchar, preview_width * preview_height * preview_bpp);
    }

  linebuffer = g_new (guchar, sel_width * image_bpp);

  gimp_pixel_rgn_init (&srcrgn, drawable,
                       xl, yl, sel_width, sel_height, FALSE, FALSE);

  for (y = 0; y < preview_height; y++)
    {
      gimp_pixel_rgn_get_row (&srcrgn, linebuffer,
                              xl, (gint) (pre2img * y) + yl, sel_width);
      for (x = 0; x < preview_width; x++)
        {
          pts = srcimage + (y * preview_width + x) * image_bpp;
          xi = (gint) (pre2img * x);

          for (i = 0; i < image_bpp; i++)
            *pts++ = linebuffer[xi * image_bpp + i];
        }
    }

  g_free (linebuffer);
}

static void
iwarp_init (void)
{
  gint  i;

  gimp_drawable_mask_bounds (drawable->drawable_id, &xl, &yl, &xh, &yh);
  sel_width = xh - xl;
  sel_height = yh - yl;

  image_bpp = gimp_drawable_bpp (drawable->drawable_id);

  if (gimp_drawable_is_layer (drawable->drawable_id))
    lock_alpha = gimp_layer_get_lock_alpha (drawable->drawable_id);
  else
    lock_alpha = FALSE;

  preview_bpp = image_bpp;

  tile_width  = gimp_tile_width ();
  tile_height = gimp_tile_height ();

  iwarp_preview_init ();
  iwarp_cpy_images ();

  deform_vectors = g_new0 (GimpVector2, preview_width * preview_height);
  deform_area_vectors = g_new (GimpVector2,
                               (MAX_DEFORM_AREA_RADIUS * 2 + 1) *
                               (MAX_DEFORM_AREA_RADIUS * 2 + 1));

  for (i = 0; i < MAX_DEFORM_AREA_RADIUS; i++)
    {
      filter[i] =
        pow ((cos (sqrt((gdouble) i / MAX_DEFORM_AREA_RADIUS) * G_PI) + 1) *
             0.5, 0.7); /*0.7*/
    }
}

static void
iwarp_animate_dialog (GtkWidget *dlg,
                      GtkWidget *notebook)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *button;
  GtkObject *scale_data;

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button = gtk_check_button_new_with_mnemonic (_("A_nimate"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), do_animate);
  gtk_frame_set_label_widget (GTK_FRAME (frame), button);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &do_animate);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  g_object_set_data (G_OBJECT (button), "set_sensitive", table);
  gtk_widget_set_sensitive (table, do_animate);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                     _("Number of _frames:"), SCALE_WIDTH, 0,
                                     animate_num_frames,
                                     2, MAX_NUM_FRAMES, 1, 10, 0,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &animate_num_frames);

  button = gtk_check_button_new_with_mnemonic (_("R_everse"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 3, 1, 2,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_toggle_button_update),
                    &do_animate_reverse);

  button = gtk_check_button_new_with_mnemonic (_("_Ping pong"));
  gtk_table_attach (GTK_TABLE (table), button, 0, 3, 2, 3,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_toggle_button_update),
                    &do_animate_ping_pong);

  gtk_widget_show (vbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            vbox,
                            gtk_label_new_with_mnemonic (_("_Animate")));
}

static void
iwarp_settings_dialog (GtkWidget *dlg,
                       GtkWidget *notebook)
{
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *vbox3;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *table;
  GtkObject *scale_data;
  GtkWidget *widget[3];
  gint       i;

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  frame = gimp_frame_new (_("Deform Mode"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (TRUE, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  vbox2 = gimp_int_radio_group_new (FALSE, NULL,
                                    G_CALLBACK (gimp_radio_button_update),
                                    &iwarp_vals.deform_mode,
                                    iwarp_vals.deform_mode,

                                    _("_Move"),      MOVE,      NULL,
                                    _("_Grow"),      GROW,      NULL,
                                    _("S_wirl CCW"), SWIRL_CCW, NULL,
                                    _("Remo_ve"),    REMOVE,    &widget[0],
                                    _("S_hrink"),    SHRINK,    &widget[1],
                                    _("Sw_irl CW"),  SWIRL_CW,  &widget[2],

                                    NULL);

  gtk_container_add (GTK_CONTAINER (hbox), vbox2);
  gtk_widget_show (vbox2);

  vbox3 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (hbox), vbox3);
  gtk_widget_show (vbox3);

  for (i = 0; i < 3; i++)
    {
      g_object_ref (widget[i]);
      gtk_widget_hide (widget[i]);
      gtk_container_remove (GTK_CONTAINER (vbox2), widget[i]);
      gtk_box_pack_start (GTK_BOX (vbox3), widget[i],
                          FALSE, FALSE, 0);
      gtk_widget_show (widget[i]);
      g_object_unref (widget[i]);
    }

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                     _("_Deform radius:"), SCALE_WIDTH, 4,
                                     iwarp_vals.deform_area_radius,
                                     5.0, MAX_DEFORM_AREA_RADIUS, 1.0, 10.0, 0,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &iwarp_vals.deform_area_radius);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                     _("D_eform amount:"), SCALE_WIDTH, 4,
                                     iwarp_vals.deform_amount,
                                     0.0, 1.0, 0.01, 0.1, 2,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &iwarp_vals.deform_amount);

  button = gtk_check_button_new_with_mnemonic (_("_Bilinear"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                iwarp_vals.do_bilinear);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &iwarp_vals.do_bilinear);

  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button = gtk_check_button_new_with_mnemonic (_("Adaptive s_upersample"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                iwarp_vals.do_supersample);
  gtk_frame_set_label_widget (GTK_FRAME (frame), button);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &iwarp_vals.do_supersample);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  g_object_set_data (G_OBJECT (button), "set_sensitive", table);
  gtk_widget_set_sensitive (table, iwarp_vals.do_supersample);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                     _("Ma_x depth:"), SCALE_WIDTH, 5,
                                     iwarp_vals.max_supersample_depth,
                                     1.0, 5.0, 1.1, 1.0, 0,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &iwarp_vals.max_supersample_depth);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                     _("Thresho_ld:"), SCALE_WIDTH, 5,
                                     iwarp_vals.supersample_threshold,
                                     1.0, 10.0, 0.01, 0.1, 2,
                                     TRUE, 0, 0,
                                     NULL, NULL);
  g_signal_connect (scale_data, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &iwarp_vals.supersample_threshold);

  gtk_widget_show (vbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            vbox,
                            gtk_label_new_with_mnemonic (_("_Settings")));
}

static void
iwarp_preview_build (GtkWidget *vbox)
{
  GtkWidget *frame;

  frame = gtk_aspect_frame_new (NULL, 0.0, 0.0, 1.0, TRUE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview, preview_width, preview_height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  gtk_widget_add_events (preview,
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_POINTER_MOTION_HINT_MASK);

  g_signal_connect (preview, "realize",
                    G_CALLBACK (iwarp_realize_callback),
                    NULL);
  g_signal_connect (preview, "event",
                    G_CALLBACK (iwarp_motion_callback),
                    NULL);
  g_signal_connect (preview, "size-allocate",
                    G_CALLBACK (iwarp_resize_callback),
                    NULL);
}


static gboolean
iwarp_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *hint;
  GtkWidget *notebook;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  iwarp_init ();

  dlg = gimp_dialog_new (_("IWarp"), PLUG_IN_BINARY,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         GIMP_STOCK_RESET, RESPONSE_RESET,
                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  g_signal_connect (dlg, "response",
                    G_CALLBACK (iwarp_response),
                    NULL);
  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  main_hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_hbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (main_hbox);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  iwarp_preview_build (vbox);
  hint = gimp_hint_box_new (_("Click and drag in the preview to define "
                              "the distortions to apply to the image."));
  gtk_box_pack_end (GTK_BOX (vbox), hint, FALSE, FALSE, 0);
  gtk_widget_show (hint);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (main_hbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  iwarp_settings_dialog (dlg, notebook);
  iwarp_animate_dialog (dlg, notebook);

  gtk_widget_show (dlg);

  iwarp_update_preview (0, 0, preview_width, preview_height);

  gtk_main ();

  return wint.run;
}

static void
iwarp_update_preview (gint x0,
                      gint y0,
                      gint x1,
                      gint y1)
{
  x0 = CLAMP (x0, 0, preview_width);
  y0 = CLAMP (y0, 0, preview_height);
  x1 = CLAMP (x1, x0, preview_width);
  y1 = CLAMP (y1, y0, preview_height);

  if (x1 > x0 && y1 > y0)
    gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                            x0, y0,
                            x1 - x0, y1 - y0,
                            gimp_drawable_type(drawable->drawable_id),
                            dstimage + (y0 * preview_width + x0) * preview_bpp,
                            preview_width * preview_bpp);
}

static void
iwarp_preview_get_pixel (gint     x,
                         gint     y,
                         guchar **color)
{
  static guchar black[4] = { 0, 0, 0, 0 };

  if (x < 0 || x >= preview_width || y<0 || y >= preview_height)
    {
      *color = black;
      return;
    }

  *color = srcimage + (y * preview_width + x) * image_bpp;
}

static void
iwarp_preview_get_point (gdouble  x,
                         gdouble  y,
                         guchar  *color)
{
  gint     xi, yi, j;
  gdouble  dx, dy, m0, m1;
  guchar  *p0, *p1, *p2, *p3;

  xi = (gint) x;
  yi = (gint) y;

  if (iwarp_vals.do_bilinear)
    {
      dx = x-xi;
      dy = y-yi;

      iwarp_preview_get_pixel (xi, yi, &p0);
      iwarp_preview_get_pixel (xi + 1, yi, &p1);
      iwarp_preview_get_pixel (xi, yi + 1, &p2);
      iwarp_preview_get_pixel (xi + 1, yi + 1, &p3);

      for (j = 0; j < image_bpp; j++)
        {
          m0 = p0[j] + dx * (p1[j] - p0[j]);
          m1 = p2[j] + dx * (p3[j] - p2[j]);
          color[j] = (guchar) (m0 + dy * (m1 - m0));
        }
    }
  else
    {
      iwarp_preview_get_pixel (xi, yi, &p0);
      for (j = 0; j < image_bpp; j++)
        color[j] = p0[j];
    }
}

static void
iwarp_deform (gint    x,
              int     y,
              gdouble vx,
              gdouble vy)
{
  gint    xi, yi, ptr, fptr, x0, x1, y0, y1, radius2, length2;
  gdouble deform_value, xn, yn, nvx=0, nvy=0, emh, em, edge_width, xv, yv;
  guchar  color[4], alpha = 255;

  x0 = (x < iwarp_vals.deform_area_radius) ?
    -x : -iwarp_vals.deform_area_radius;

  x1 = (x + iwarp_vals.deform_area_radius >= preview_width) ?
    preview_width - x - 1 : iwarp_vals.deform_area_radius;

  y0 = (y < iwarp_vals.deform_area_radius) ?
    -y :  -iwarp_vals.deform_area_radius;

  y1 = (y + iwarp_vals.deform_area_radius >= preview_height) ?
    preview_height-y-1 : iwarp_vals.deform_area_radius;

  radius2 = SQR (iwarp_vals.deform_area_radius);

  for (yi = y0; yi <= y1; yi++)
    for (xi = x0; xi <= x1; xi++)
      {
        length2 = (xi * xi + yi * yi) * MAX_DEFORM_AREA_RADIUS / radius2;
        if (length2 < MAX_DEFORM_AREA_RADIUS)
          {
            ptr = (y + yi) * preview_width + x + xi;
            fptr =
              (yi + iwarp_vals.deform_area_radius) *
              (iwarp_vals.deform_area_radius * 2 + 1) +
              xi +
              iwarp_vals.deform_area_radius;

            switch (iwarp_vals.deform_mode)
              {
              case GROW:
                deform_value = filter[length2] *  0.1 * iwarp_vals.deform_amount;
                nvx = -deform_value * xi;
                nvy = -deform_value * yi;
                break;

              case SHRINK:
                deform_value = filter[length2] * 0.1 * iwarp_vals.deform_amount;
                nvx = deform_value * xi;
                nvy = deform_value * yi;
                break;

              case SWIRL_CW:
                deform_value = filter[length2] * iwarp_vals.deform_amount * 0.5;
                nvx =  deform_value * yi;
                nvy = -deform_value * xi;
                break;

              case SWIRL_CCW:
                deform_value = filter[length2] *iwarp_vals.deform_amount * 0.5;
                nvx = -deform_value * yi;
                nvy =  deform_value  * xi;
                break;

              case MOVE:
                deform_value = filter[length2] * iwarp_vals.deform_amount;
                nvx = deform_value * vx;
                nvy = deform_value * vy;
                break;

              default:
                break;
              }

            if (iwarp_vals.deform_mode == REMOVE)
              {
                deform_value =
                  1.0 - 0.5 * iwarp_vals.deform_amount * filter[length2];
                deform_area_vectors[fptr].x =
                  deform_value * deform_vectors[ptr].x ;
                deform_area_vectors[fptr].y =
                  deform_value * deform_vectors[ptr].y ;
              }
            else
              {
                edge_width = 0.2 *  iwarp_vals.deform_area_radius;
                emh = em = 1.0;
                if (x+xi < edge_width)
                  em = (gdouble) (x + xi) / edge_width;
                if (y + yi < edge_width)
                  emh = (gdouble) (y + yi) / edge_width;
                if (emh <em)
                  em = emh;
                if (preview_width - x - xi - 1 < edge_width)
                  emh = (gdouble) (preview_width - x - xi - 1) / edge_width;
                if (emh < em)
                  em = emh;
                if (preview_height-y-yi-1 < edge_width)
                  emh = (gdouble) (preview_height - y - yi - 1) / edge_width;
                if (emh <em)
                  em = emh;

                nvx = nvx * em;
                nvy = nvy * em;

                iwarp_get_deform_vector (nvx + x+ xi, nvy + y + yi, &xv, &yv);
                xv += nvx;
                if (xv +x+xi <0.0)
                  xv = -x - xi;
                else if (xv + x +xi > (preview_width-1))
                  xv = preview_width - x -xi-1;
                yv += nvy;
                if (yv + y + yi < 0.0)
                  yv = -y - yi;
                else if (yv + y + yi > (preview_height-1))
                  yv = preview_height - y -yi - 1;
                deform_area_vectors[fptr].x = xv;
                deform_area_vectors[fptr].y = yv;
              }

            xn = deform_area_vectors[fptr].x + x + xi;
            yn = deform_area_vectors[fptr].y + y + yi;

            /* Yeah, it is ugly but since color is a pointer into image-data
             * I must not change color[image_bpp - 1]  ...
             */
            if (lock_alpha && (image_bpp == 4 || image_bpp == 2))
              {
                iwarp_preview_get_point (x + xi, y + yi, color);
                alpha = color[image_bpp - 1];
              }

            iwarp_preview_get_point (xn, yn, color);

            if (!lock_alpha && (image_bpp == 4 || image_bpp == 2))
              {
                alpha = color[image_bpp - 1];
              }

            switch (preview_bpp)
              {
              case 4:
                dstimage[ptr*4 + 0] = color[0];
                dstimage[ptr*4 + 1] = color[1];
                dstimage[ptr*4 + 2] = color[2];
                dstimage[ptr*4 + 3] = alpha;
                break;

              case 3:
                dstimage[ptr*3 + 0] = color[0];
                dstimage[ptr*3 + 1] = color[1];
                dstimage[ptr*3 + 2] = color[2];
                break;

              case 2:
                dstimage[ptr*2 + 0] = color[0];
                dstimage[ptr*2 + 1] = alpha;
                break;

              case 1:
                dstimage[ptr] = color[0];
              }
          }
      }

  for (yi = y0; yi <= y1; yi++)
    for (xi = x0; xi <= x1; xi++)
      {
        length2 = (xi*xi+yi*yi) * MAX_DEFORM_AREA_RADIUS / radius2;
        if (length2 < MAX_DEFORM_AREA_RADIUS)
          {
            ptr = (yi +y) * preview_width + xi +x;
            fptr =
              (yi+iwarp_vals.deform_area_radius) *
              (iwarp_vals.deform_area_radius*2+1) +
              xi +
              iwarp_vals.deform_area_radius;
            deform_vectors[ptr] = deform_area_vectors[fptr];
          }
      }

  iwarp_update_preview (x + x0, y + y0, x + x1 + 1, y + y1 + 1);
}

static void
iwarp_move (gint x,
            gint y,
            gint xx,
            gint yy)
{
  gdouble l, dx, dy, xf, yf;
  gint    num, i, x0, y0;

  dx = x - xx;
  dy = y - yy;
  l= sqrt (dx * dx + dy * dy);
  num = (gint) (l * 2 / iwarp_vals.deform_area_radius) + 1;
  dx /= num;
  dy /= num;
  xf = xx + dx; yf = yy + dy;

  for (i=0; i< num; i++)
    {
      x0 = (gint) xf;
      y0 = (gint) yf;
      iwarp_deform (x0, y0, -dx, -dy);
      xf += dx;
      yf += dy;
    }
}

static void
iwarp_response (GtkWidget *widget,
                gint       response_id,
                gpointer   data)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      {
        gint i;

        iwarp_cpy_images ();

        for (i = 0; i < preview_width * preview_height; i++)
          deform_vectors[i].x = deform_vectors[i].y = 0.0;

        iwarp_update_preview (0, 0, preview_width, preview_height);
      }
      break;

    case GTK_RESPONSE_OK:
      wint.run = TRUE;

    default:
      gtk_widget_destroy (widget);
      break;
    }
}

static void
iwarp_realize_callback (GtkWidget *widget)
{
  GdkDisplay *display = gtk_widget_get_display (widget);
  GdkCursor  *cursor  = gdk_cursor_new_for_display (display, GDK_CROSSHAIR);

  gdk_window_set_cursor (widget->window, cursor);
  gdk_cursor_unref (cursor);
}

static gboolean
iwarp_motion_callback (GtkWidget *widget,
                       GdkEvent  *event)
{
  GdkEventButton *mb = (GdkEventButton *) event;
  gint            x, y;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      lastx = mb->x;
      lasty = mb->y;
      break;

    case GDK_BUTTON_RELEASE:
      if (mb->state & GDK_BUTTON1_MASK)
        {
          x = mb->x;
          y = mb->y;

          if (iwarp_vals.deform_mode == MOVE)
            iwarp_move (x, y, lastx, lasty);
          else
            iwarp_deform (x, y, 0.0, 0.0);
        }
      break;

   case GDK_MOTION_NOTIFY:
     if (mb->state & GDK_BUTTON1_MASK)
       {
         gtk_widget_get_pointer (widget, &x, &y);

         if (iwarp_vals.deform_mode == MOVE)
           iwarp_move (x, y, lastx, lasty);
         else
           iwarp_deform (x, y, 0.0, 0.0);

         lastx = x;
         lasty = y;
       }
     break;

    default:
      break;
    }

  return FALSE;
}

static gboolean
iwarp_resize_idle (GtkWidget *widget)
{
  GimpVector2 *new_deform_vectors;
  gint         old_preview_width, old_preview_height;
  gint         new_preview_width, new_preview_height;
  gint         x, y;
  gdouble      new2old;

  resize_idle = 0;

  old_preview_width = preview_width;
  old_preview_height = preview_height;

  max_current_preview_width = widget->allocation.width;
  max_current_preview_height = widget->allocation.height;

  /* preview width and height get updated here: */
  iwarp_preview_init ();
  new_preview_width = preview_width;
  new_preview_height = preview_height;

  new_deform_vectors = g_new0 (GimpVector2, preview_width * preview_height);
  new2old = (gdouble) old_preview_width / preview_width;

  /* preview_width and height are used as global variables inside
   * iwarp_get_deform_factor().  In the following call to the function,
   * I need it to run with the old values. Adding a width and height
   * to these function parameters would be an option for cleaner code,
   * but that would also mean pushing 16 extra parameter bytes several
   * times over for each pixel processed.
   */

  preview_width = old_preview_width;
  preview_height = old_preview_height;

  for (y = 0; y < new_preview_height; y++)
    for (x = 0; x < new_preview_width; x++)
      iwarp_get_deform_vector (new2old * x,
                               new2old * y,
                               &new_deform_vectors[x + new_preview_width * y].x,
                               &new_deform_vectors[x + new_preview_width * y].y);

  preview_width = new_preview_width;
  preview_height = new_preview_height;

  g_free (deform_vectors);
  deform_vectors = new_deform_vectors;

  iwarp_scale_preview (new_preview_width, new_preview_height,
                       old_preview_width, old_preview_height);

  iwarp_update_preview (0, 0, preview_width, preview_height);

  return FALSE;
}

static void
iwarp_resize_callback (GtkWidget *widget)
{
  if (resize_idle)
    g_source_remove (resize_idle);

  resize_idle = g_idle_add_full (G_PRIORITY_LOW,
                                 (GSourceFunc) iwarp_resize_idle, widget,
                                 NULL);
}
