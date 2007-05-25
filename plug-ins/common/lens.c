/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Lens plug-in - adjust for lens distortion
 * Copyright (C) 2001-2005 David Hodson hodsond@acm.org
 * Many thanks for Lars Clausen for the original inspiration,
 *   useful discussion, optimisation and improvements.
 * Framework borrowed from many similar plugins...
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* non-zero value compiles with statistics gathering */
#define GATHERING_STATS  0

#define PLUG_IN_PROC     "plug-in-lens-distortion"
#define PLUG_IN_BINARY   "lens"

#define RESPONSE_RESET   1

#define LENS_PIXEL_ACCESS_REGIONS  20
#define LENS_PIXEL_ACCESS_WIDTH    40
#define LENS_PIXEL_ACCESS_HEIGHT   20
#define LENS_PIXEL_ACCESS_XOFFSET   3
#define LENS_PIXEL_ACCESS_YOFFSET   3

#define LENS_MAX_PIXEL_DEPTH        4


typedef struct
{
  gint          preview_drawable; /* preview pixels */
  gdouble       preview_scale;
  GimpPixelRgn  src_rgn; /* image pixels */
  guchar       *buffer[LENS_PIXEL_ACCESS_REGIONS];
  gint          width;
  gint          height;
  gint          depth;
  gint          image_width;
  gint          image_height;
  gint          tile_xmin[LENS_PIXEL_ACCESS_REGIONS];
  gint          tile_xmax[LENS_PIXEL_ACCESS_REGIONS];
  gint          tile_ymin[LENS_PIXEL_ACCESS_REGIONS];
  gint          tile_ymax[LENS_PIXEL_ACCESS_REGIONS];
#if GATHERING_STATS
  gint          pixels_found;
  gint          pixels_found_in_buffer[LENS_PIXEL_ACCESS_REGIONS];
  gint          pixels_loaded_from_image;
#endif
} LensPixelAccess;

typedef struct
{
  gdouble  centre_x;
  gdouble  centre_y;
  gdouble  square_a;
  gdouble  quad_a;
  gdouble  scale_a;
  gdouble  brighten;
} LensValues;

typedef struct
{
  gdouble  normallise_radius_sq;
  gdouble  centre_x;
  gdouble  centre_y;
  gdouble  mult_sq;
  gdouble  mult_qd;
  gdouble  rescale;
  gdouble  brighten;
} LensCalcValues;


/* Declare local functions. */

static void     query (void);
static void     run   (const gchar      *name,
                       gint              nparams,
                       const GimpParam  *param,
                       gint             *nreturn_vals,
                       GimpParam       **return_vals);

static void     lens_correction    (GimpDrawable *drawable);
static gboolean lens_dialog        (GimpDrawable *drawable);


static LensValues         vals = { 0.0, 0.0, 0.0, 0.0 };
static LensCalcValues     calc_vals;


const GimpPlugInInfo PLUG_IN_INFO =
  {
    NULL,  /* init_proc  */
    NULL,  /* quit_proc  */
    query, /* query_proc */
    run,   /* run_proc   */
  };


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
    {
      { GIMP_PDB_INT32,      "run-mode",    "Interactive, non-interactive" },
      { GIMP_PDB_IMAGE,      "image",       "Input image (unused)" },
      { GIMP_PDB_DRAWABLE,   "drawable",    "Input drawable" },
      { GIMP_PDB_FLOAT,      "offset-x",    "Effect centre offset in X" },
      { GIMP_PDB_FLOAT,      "offset-y",    "Effect centre offset in Y" },
      { GIMP_PDB_FLOAT,      "main-adjust",
        "Amount of second-order distortion" },
      { GIMP_PDB_FLOAT,      "edge-adjust",
        "Amount of fourth-order distortion" },
      { GIMP_PDB_FLOAT,      "rescale",     "Rescale overall image size" },
      { GIMP_PDB_FLOAT,      "brighten",    "Adjust brightness in corners" }
    };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Corrects lens distortion"),
                          "Corrects barrel or pincushion lens distortion.",
                          "David Hodson, ported by Aurimas Juska",
                          "David Hodson",
                          "Version 1.0.10",
                          N_("Lens Distortion..."),
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
  GimpDrawable      *drawable;
  gint32             image_ID;
  GimpPDBStatusType  status   = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;

  run_mode = param[0].data.d_int32;
  image_ID = param[1].data.d_int32;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  INIT_I18N ();

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  *nreturn_vals = 1;
  *return_vals = values;

  switch (run_mode) {
  case GIMP_RUN_INTERACTIVE:
    gimp_get_data (PLUG_IN_PROC, &vals);
    if (! lens_dialog (drawable))
      return;
    break;

  case GIMP_RUN_NONINTERACTIVE:
    if (nparams != 9)
      status = GIMP_PDB_CALLING_ERROR;

    if (status == GIMP_PDB_SUCCESS)
      {
        vals.centre_x = param[3].data.d_float;
        vals.centre_y = param[4].data.d_float;
        vals.square_a = param[5].data.d_float;
        vals.quad_a = param[6].data.d_float;
        vals.scale_a = param[7].data.d_float;
        vals.brighten = param[8].data.d_float;
      }

    break;

  case GIMP_RUN_WITH_LAST_VALS:
    gimp_get_data (PLUG_IN_PROC, &vals);
    break;

  default:
    break;
  }

  if ( status == GIMP_PDB_SUCCESS )
    {
      lens_correction (drawable);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &vals, sizeof (LensValues));

      gimp_drawable_detach (drawable);
    }

  values[0].data.d_status = status;
}

static void
lens_get_source_coords (gdouble  i,
                        gdouble  j,
                        gdouble *x,
                        gdouble *y,
                        gdouble *mag)
{
  gdouble radius_sq;

  gdouble off_x;
  gdouble off_y;

  gdouble radius_mult;

  off_x = i - calc_vals.centre_x;
  off_y = j - calc_vals.centre_y;
  radius_sq = (off_x * off_x) + (off_y * off_y);

  radius_sq *= calc_vals.normallise_radius_sq;

  radius_mult = radius_sq * calc_vals.mult_sq + radius_sq * radius_sq *
    calc_vals.mult_qd;
  *mag = radius_mult;
  radius_mult = calc_vals.rescale * (1.0 + radius_mult);

  *x = calc_vals.centre_x + radius_mult * off_x;
  *y = calc_vals.centre_y + radius_mult * off_y;
}

static void
lens_setup_calc (gint width, gint height)
{
  calc_vals.normallise_radius_sq =
    4.0 / (width * width + height * height);

  calc_vals.centre_x = width * (100.0 + vals.centre_x) / 200.0;
  calc_vals.centre_y = height * (100.0 + vals.centre_y) / 200.0;
  calc_vals.mult_sq = vals.square_a / 200.0;
  calc_vals.mult_qd = vals.quad_a / 200.0;
  calc_vals.rescale = pow(2.0, - vals.scale_a / 100.0);
  calc_vals.brighten = - vals.brighten / 10.0;
}

/*
 * Catmull-Rom cubic interpolation
 *
 * equally spaced points p0, p1, p2, p3
 * interpolate 0 <= u < 1 between p1 and p2
 *
 * (1 u u^2 u^3) (  0.0  1.0  0.0  0.0 ) (p0)
 *               ( -0.5  0.0  0.5  0.0 ) (p1)
 *               (  1.0 -2.5  2.0 -0.5 ) (p2)
 *               ( -0.5  1.5 -1.5  0.5 ) (p3)
 *
 */

static void
lens_cubic_interpolate (const guchar *src,
                        gint          row_stride,
                        gint          src_depth,
                        guchar       *dst,
                        gint          dst_depth,
                        gdouble       dx,
                        gdouble       dy,
                        gdouble       brighten)
{
  gfloat um1, u, up1, up2;
  gfloat vm1, v, vp1, vp2;
  gint   c;
  gfloat verts[4 * LENS_MAX_PIXEL_DEPTH];

  um1 = ((-0.5 * dx + 1.0) * dx - 0.5) * dx;
  u = (1.5 * dx - 2.5) * dx * dx + 1.0;
  up1 = ((-1.5 * dx + 2.0) * dx + 0.5) * dx;
  up2 = (0.5 * dx - 0.5) * dx * dx;

  vm1 = ((-0.5 * dy + 1.0) * dy - 0.5) * dy;
  v = (1.5 * dy - 2.5) * dy * dy + 1.0;
  vp1 = ((-1.5 * dy + 2.0) * dy + 0.5) * dy;
  vp2 = (0.5 * dy - 0.5) * dy * dy;

  /* Note: if dst_depth < src_depth, we calculate unneeded pixels here */
  /* later - select or create index array */
  for (c = 0; c < 4 * src_depth; ++c)
    {
      verts[c] = vm1 * src[c] + v * src[c+row_stride] +
        vp1 * src[c+row_stride*2] + vp2 * src[c+row_stride*3];
    }

  for (c = 0; c < dst_depth; ++c)
    {
      gfloat result;

      result = um1 * verts[c] + u * verts[c+src_depth] +
        up1 * verts[c+src_depth*2] + up2 * verts[c+src_depth*3];

      result *= brighten;

      if (result < 0.0)
        {
          dst[c] = 0;
        }
      else if (result > 255.0)
        {
          dst[c] = 255;
        }
      else
        {
          dst[c] = result;
        }
    }
}

/* Solving the eternal problem: random, cubic-interpolated,
 * sub-pixel coordinate access to a tiled image.
 * Assuming that accesses are at least slightly coherent,
 * LensPixelAccess keeps LENS_PIXEL_ACCESS_REGIONS buffers, each containing a
 * LENS_PIXEL_ACCESS_WIDTH x LENS_PIXEL_ACCESS_HEIGHT region of pixels.
 * Buffer[0] is always checked first, so move the last accessed
 * region into that position.
 * When a request arrives which is outside all the regions,
 * get a new region (using GimpPixelRgn - good idea? bad idea?).
 * The new region is placed so that the requested pixel is positioned
 * at [LENS_PIXEL_ACCESS_XOFFSET, LENS_PIXEL_ACCESS_YOFFSET] in the region.
 */

#if GATHERING_STATS
static void
lens_pixel_access_dump_region (LensPixelAccess* pa,
                               gint             n)
{
  g_print ("Region %d: buffer %p\n", n, pa->buffer[n]);
  g_print ("  X min %d max %d, Y min %d max %d\n",
           pa->tile_xmin[n], pa->tile_xmax[n],
           pa->tile_ymin[n], pa->tile_ymax[n]);
}

static void
lens_pixel_access_dump_stats (LensPixelAccess* pa)
{
  g_print ("Pixels found %d\n", pa->pixels_found);

  if (pa->pixels_found)
    {
      gint i;

      for (i = 0; i < LENS_PIXEL_ACCESS_REGIONS; ++i)
        {
          g_print ("  In buffer[%d]: %d ratio %f\n", i,
                   pa->pixels_found_in_buffer[i],
                   (gfloat) pa->pixels_found_in_buffer[i] /
                   (gfloat) pa->pixels_found );
          pa->pixels_found_in_buffer[i] = 0;
        }

      g_print ("  Loaded: %d ratio %f\n", pa->pixels_loaded_from_image,
               (gfloat) pa->pixels_loaded_from_image /
               (gfloat) pa->pixels_found );
      pa->pixels_loaded_from_image = 0;
    }

  pa->pixels_found = 0;
}
#endif

/* get rect of pixels from preview data */
static void
lens_pixel_access_get_rect (LensPixelAccess *pa,
                            guchar          *buffer,
                            gint             x,
                            gint             y,
                            gint             width,
                            gint             height)
{
  gint          i;
  guchar       *row;
  guchar       *data;
  gint          rowstride;
  gint          rowbytes;
  gint          src_x, src_y;
  gint          src_width, src_height;
  gint          data_width;
  gint          data_height;
  gint          data_bpp;

  g_assert (x + width <= pa->image_width);
  g_assert (y + height <= pa->image_height);

  memset (buffer, 0, width * height * pa->depth);

  src_x = x / pa->preview_scale;
  src_y = y / pa->preview_scale;
  src_width = width / pa->preview_scale;
  if (src_width <= 0)
    src_width = 1;
  src_height = height / pa->preview_scale;
  if (src_height <= 0)
    src_height = 1;
  data_width = width;
  data_height = height;
  data_bpp = pa->depth;
  data = gimp_drawable_get_sub_thumbnail_data (pa->preview_drawable,
                                               src_x, src_y,
                                               src_width, src_height,
                                               &data_width,
                                               &data_height,
                                               &data_bpp);

  rowstride = data_width * data_bpp;
  rowbytes = width * data_bpp;
  row = data;

  for ( i = 0; i < height; i++ )
    {
      memcpy (buffer, row, rowbytes);
      buffer += rowbytes;
      row += rowstride;
    }

  g_free (data);
}

static LensPixelAccess *
lens_pixel_access_new (GimpDrawable *drawable)
{
  LensPixelAccess *pa = g_new (LensPixelAccess, 1);
  gint             i;
  gint             buffer_size;

  pa->preview_drawable = -1;
  pa->width = LENS_PIXEL_ACCESS_WIDTH;
  pa->height = LENS_PIXEL_ACCESS_HEIGHT;
  pa->depth = drawable->bpp;
  pa->image_width = drawable->width;
  pa->image_height = drawable->height;

  gimp_pixel_rgn_init (&pa->src_rgn, drawable, 0, 0, drawable->width,
                       drawable->height, FALSE, FALSE);

  buffer_size = pa->height * pa->width * pa->depth;
  pa->buffer[0] = (guchar*) g_malloc (buffer_size);
  /* better: lens_pixel_access_reposition(), pixelAccessSelect()! */
  gimp_pixel_rgn_get_rect (&pa->src_rgn, pa->buffer[0], 0, 0,
                           pa->width, pa->height);
  pa->tile_xmin[0] = 1;
  pa->tile_xmax[0] = pa->width - 2;
  pa->tile_ymin[0] = 1;
  pa->tile_ymax[0] = pa->height - 2;

  for (i = 1; i < LENS_PIXEL_ACCESS_REGIONS; ++i)
    {
      pa->buffer[i] = (guchar*) g_malloc (buffer_size);
      memcpy (pa->buffer[i], pa->buffer[0], buffer_size);
      pa->tile_xmin[i] = 1;
      pa->tile_xmax[i] = pa->width - 2;
      pa->tile_ymin[i] = 1;
      pa->tile_ymax[i] = pa->height - 2;
    }
#if GATHERING_STATS
  for ( i = 0; i < LENS_PIXEL_ACCESS_REGIONS; i++ )
    pa->pixels_found_in_buffer[i] = 0;
  pa->pixels_found = 0;
  pa->pixels_loaded_from_image = 0;
#endif

  return pa;
}

static LensPixelAccess *
lens_pixel_access_new_for_preview (gint         drawable,
                                   gint         width,
                                   gint         height,
                                   gdouble      factor)
{
  LensPixelAccess *pa = g_new (LensPixelAccess, 1);
  gint             i;
  gint             buffer_size;

  pa->preview_drawable = drawable;
  pa->preview_scale = (gdouble) (width * factor) /
      gimp_drawable_width (drawable);
  pa->width = MIN (LENS_PIXEL_ACCESS_WIDTH,
                   gimp_drawable_width (drawable) * pa->preview_scale);
  pa->height = MIN (LENS_PIXEL_ACCESS_HEIGHT,
                    gimp_drawable_height (drawable) * pa->preview_scale);
  pa->depth = gimp_drawable_bpp (drawable);
  pa->image_width = gimp_drawable_width (drawable) * pa->preview_scale;
  pa->image_height = gimp_drawable_height (drawable) * pa->preview_scale;

  buffer_size = pa->height * pa->width * pa->depth;
  pa->buffer[0] = (guchar*) g_malloc (buffer_size);
  lens_pixel_access_get_rect (pa, pa->buffer[0], 0, 0, pa->width, pa->height);
  pa->tile_xmin[0] = 1;
  pa->tile_xmax[0] = pa->width - 2;
  pa->tile_ymin[0] = 1;
  pa->tile_ymax[0] = pa->height - 2;

  for (i = 1; i < LENS_PIXEL_ACCESS_REGIONS; ++i)
    {
      pa->buffer[i] = (guchar*) g_malloc (buffer_size);
      memcpy (pa->buffer[i], pa->buffer[0], buffer_size);
      pa->tile_xmin[i] = 1;
      pa->tile_xmax[i] = pa->width - 2;
      pa->tile_ymin[i] = 1;
      pa->tile_ymax[i] = pa->height - 2;
    }
#if GATHERING_STATS
  for ( i = 0; i < LENS_PIXEL_ACCESS_REGIONS; i++ )
    pa->pixels_found_in_buffer[i] = 0;
  pa->pixels_found = 0;
  pa->pixels_loaded_from_image = 0;
#endif

  return pa;
}

static void
lens_pixel_access_delete (LensPixelAccess *pa)
{
  gint i;

  for (i = 0; i < LENS_PIXEL_ACCESS_REGIONS; ++i)
    g_free (pa->buffer[i]);

  g_free (pa);
}

static guchar *
lens_pixel_access_address (LensPixelAccess *pa,
                           gint             i,
                           gint             j)
{
  return pa->buffer[0] + pa->depth *
    (pa->width * (j + 1 - pa->tile_ymin[0]) + (i + 1 - pa->tile_xmin[0]));
}

/* swap region[n] with region[0] */
static void
lens_pixel_access_select_region (LensPixelAccess *pa,
                                 gint             n)
{
  guchar *temp;
  gint    a, b, c, d;
  gint    i;

  temp = pa->buffer[n];
  a = pa->tile_xmin[n];
  b = pa->tile_xmax[n];
  c = pa->tile_ymin[n];
  d = pa->tile_ymax[n];

  for (i = n; i > 0; --i)
    {
      pa->buffer[i] = pa->buffer[i-1];
      pa->tile_xmin[i] = pa->tile_xmin[i-1];
      pa->tile_xmax[i] = pa->tile_xmax[i-1];
      pa->tile_ymin[i] = pa->tile_ymin[i-1];
      pa->tile_ymax[i] = pa->tile_ymax[i-1];
    }

  pa->buffer[0] = temp;
  pa->tile_xmin[0] = a;
  pa->tile_xmax[0] = b;
  pa->tile_ymin[0] = c;
  pa->tile_ymax[0] = d;

}

/* buffer[0] is cleared, should start at [i, j],
 * fill rows that overlap image */
static void
lens_pixel_access_do_edge (LensPixelAccess *pa,
                           gint             i,
                           gint             j)
{
  guchar *line;
  gint    y;
  gint    line_start, line_end;
  gint    row_start, row_end;
  gint    line_width;

  line_start = MAX (i, 0);
  line_end   = MIN (i + pa->width, pa->image_width);
  line_width = line_end - line_start;

  if (line_start >= line_end)
    return;

  row_start = MAX (j, 0);
  row_end   = MIN (j + pa->height, pa->image_height);

  for (y = row_start; y < row_end; ++y)
    {
      line = lens_pixel_access_address (pa, line_start, y);
#if GATHERING_STATS
      pa->pixels_loaded_from_image += line_width;
#endif
      if (pa->preview_drawable != -1)
        lens_pixel_access_get_rect (pa, line, line_start, y, line_width, 1);
      else
        gimp_pixel_rgn_get_row (&pa->src_rgn, line, line_start, y, line_width);
    }
}

/* moves buffer[0] so that [x, y] is inside it */
static void
lens_pixel_access_reposition (LensPixelAccess *pa,
                              gint             x_int,
                              gint             y_int)
{
  gint new_xstart, new_ystart;

  /* experiment! */
  /* could look at current position of region, for example... */
  /* this assumes random direction */
  /*
   *  new_xstart = x_int - pa->width / 2;
   *  new_ystart = y_int - pa->height / 2;
   */
  /* this assumes mostly stepping min->max in x and y */
  new_xstart = x_int - LENS_PIXEL_ACCESS_XOFFSET;
  new_ystart = y_int - LENS_PIXEL_ACCESS_YOFFSET;

  pa->tile_xmin[0] = new_xstart + 1;
  pa->tile_xmax[0] = new_xstart + pa->width - 2;
  pa->tile_ymin[0] = new_ystart + 1;
  pa->tile_ymax[0] = new_ystart + pa->height - 2;

  if ( (new_xstart < 0) || ((new_xstart + pa->width) >= pa->image_width) ||
       (new_ystart < 0) || ((new_ystart + pa->height) >= pa->image_height) )
    {
      /* some data is off edge of image */

      memset (pa->buffer[0], 0, pa->width * pa->height * pa->depth);

      if ( ((new_xstart + pa->width) < 0) || (new_xstart >= pa->image_width) ||
           ((new_ystart + pa->height) < 0) || (new_ystart >= pa->image_height))
        {
          /* totally outside, just leave it */
        }
      else
        {
          lens_pixel_access_do_edge (pa, new_xstart, new_ystart);
        }

    }
  else
    {
#if GATHERING_STATS
      pa->pixels_loaded_from_image += pa->width * pa->height;
#endif
      if ( pa->preview_drawable != -1 )
        lens_pixel_access_get_rect (pa, pa->buffer[0],
                                    new_xstart, new_ystart,
                                    pa->width, pa->height);
      else
        gimp_pixel_rgn_get_rect (&pa->src_rgn, pa->buffer[0],
                                 new_xstart, new_ystart,
                                 pa->width, pa->height);
    }
}

static void
lens_pixel_access_get_cubic (LensPixelAccess *pa,
                             gdouble          src_x,
                             gdouble          src_y,
                             gdouble          brighten,
                             guchar          *dst,
                             gint             dst_depth)
{
  guchar  *corner;
  gdouble  dx, dy;
  gint     i;
  gint     x_int, y_int;

  x_int = floor (src_x);
  dx = src_x - x_int;

  y_int = floor (src_y);
  dy = src_y - y_int;

#if GATHERING_STATS
  ++pa->pixels_found;
#endif

  /* we need 4x4 pixels, x_int-1 to x_int+2 horz, y_int-1 to y_int+2 vert */
  /* they're probably in the last place we looked... */
  if ((x_int >= pa->tile_xmin[0]) && (x_int < pa->tile_xmax[0]) &&
      (y_int >= pa->tile_ymin[0]) && (y_int < pa->tile_ymax[0]) )
    {
#if GATHERING_STATS
      ++pa->pixels_found_in_buffer[0];
#endif
      corner = lens_pixel_access_address (pa, x_int - 1, y_int - 1);
      lens_cubic_interpolate (corner, pa->depth * pa->width, pa->depth, dst,
                              dst_depth, dx, dy, brighten);
      return;
    }

  /* or maybe it was a while back... */
  for (i = 1; i < LENS_PIXEL_ACCESS_REGIONS; ++i)
    {
      if ((x_int >= pa->tile_xmin[i]) && (x_int < pa->tile_xmax[i]) &&
          (y_int >= pa->tile_ymin[i]) && (y_int < pa->tile_ymax[i]) )
        {
#if GATHERING_STATS
          ++pa->pixels_found_in_buffer[i];
#endif
          /* check here first next time */
          lens_pixel_access_select_region (pa, i);
          corner = lens_pixel_access_address (pa, x_int - 1, y_int - 1);
          lens_cubic_interpolate (corner, pa->depth * pa->width, pa->depth,
                                  dst, dst_depth, dx, dy, brighten);
          return;
        }
    }

  /* nope, recycle an old region */
  lens_pixel_access_select_region (pa, LENS_PIXEL_ACCESS_REGIONS - 1);
  lens_pixel_access_reposition (pa, x_int, y_int);

  corner = lens_pixel_access_address (pa, x_int - 1, y_int - 1);
  lens_cubic_interpolate (corner, pa->depth * pa->width, pa->depth, dst,
                          dst_depth, dx, dy, brighten);
}

/*
 * start at image (i, j), increment by (step, step)
 * output goes to dst, which is w x h x d in size
 * NB: d <= image.bpp
 */
static void
lens_calc_rect (LensPixelAccess *pa,
                guchar          *dst,
                gint             x,
                gint             y,
                gint             width,
                gint             height,
                gint             depth,
                gint             rowstride,
                gint             step)
{
  gdouble src_x, src_y, mag, brighten;
  gint    i, j;
  gint    xlimit, ylimit;
  gint    rowpad;

  xlimit = x + width * step;
  ylimit = y + height * step;
  rowpad = rowstride - width * depth;

  for (i = y; i < ylimit; i += step)
    {
      for ( j = x; j < xlimit; j += step)
        {
          lens_get_source_coords (j, i, &src_x, &src_y, &mag);
          brighten = 1.0 + mag * calc_vals.brighten;
          lens_pixel_access_get_cubic (pa, src_x, src_y, brighten,
                                       dst, depth);
          dst += depth;
        }

      dst += rowpad;
    }
}

static void
lens_correction (GimpDrawable *drawable)
{
  GimpPixelRgn     dest_rgn;
  gpointer         pr;
  guchar          *dest;
  gint             x1, y1, x2, y2;
  gint             width, height;
  gint             progress, progress_max;
  LensPixelAccess *pa;

  lens_setup_calc (drawable->width, drawable->height);

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
  width  = x2 - x1;
  height = y2 - y1;

  gimp_progress_init (_("Lens distortion"));

  progress_max = ((width - 1) / gimp_tile_width() + 1) *
    ((height - 1) / gimp_tile_height() + 1);

  dest = g_malloc (gimp_tile_width() * gimp_tile_height() * drawable->bpp);

  pa = lens_pixel_access_new (drawable);
#if GATHERING_STATS
  lens_pixel_access_dump_stats (pa);
#endif
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1,
                       width, height, TRUE, TRUE);
  pr = gimp_pixel_rgns_register (1, &dest_rgn);

  progress = 0;
  for ( ; pr; pr = gimp_pixel_rgns_process (pr))
    {
      lens_calc_rect (pa, dest_rgn.data, dest_rgn.x, dest_rgn.y,
                      dest_rgn.w, dest_rgn.h, dest_rgn.bpp,
                      dest_rgn.rowstride, 1);

      gimp_progress_update ((gfloat) progress / (gfloat) progress_max);
      ++progress;
    }

  g_free(dest);

  gimp_progress_update (1.0);
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);

#if GATHERING_STATS
  lens_pixel_access_dump_stats (pa);
#endif

  lens_pixel_access_delete (pa);
}

static void
lens_correction_preview (GimpDrawable *drawable,
                         GimpPreview  *preview)
{
  guchar          *dest;
  gint             x, y;
  gint             width, height;
  gint             image_width, image_height;
  gint             bpp;
  LensPixelAccess *pa;
  gdouble          zoom_factor;
  gdouble          zoom_scale;

  zoom_factor = gimp_zoom_preview_get_factor (GIMP_ZOOM_PREVIEW (preview));
  dest = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                       &width, &height, &bpp);
  image_width = width * zoom_factor;
  image_height = height * zoom_factor;
  zoom_scale = (gdouble)(width * zoom_factor) / drawable->width;
  lens_setup_calc (image_width, image_height);
  pa = lens_pixel_access_new_for_preview (drawable->drawable_id,
                                          width, height,
                                          zoom_factor);
  gimp_preview_untransform (GIMP_PREVIEW (preview), 0, 0, &x, &y);
  lens_calc_rect (pa, dest, x * zoom_scale, y * zoom_scale,
                  width, height, bpp, width*bpp, 1);

  gimp_preview_draw_buffer (preview, dest, width * bpp);

  lens_pixel_access_delete (pa);
  g_free (dest);
}

/* UI callback functions */

static GSList *adjustments = NULL;

static void
lens_dialog_reset (void)
{
  GSList *list;

  for (list = adjustments; list; list = list->next)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (list->data), 0.0);
}

static void
lens_response (GtkWidget *widget,
               gint       response_id,
               gboolean  *run)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      lens_dialog_reset ();
      break;

    case GTK_RESPONSE_OK:
      *run = TRUE;
      /* fallthrough */

    default:
      gtk_widget_destroy (GTK_WIDGET (widget));
      break;
    }
}

static gboolean
lens_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *table;
  GtkWidget *preview;
  GtkObject *adj;
  gint       row = 0;
  gboolean   run = FALSE;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Lens Distortion"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GIMP_STOCK_RESET, RESPONSE_RESET,
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_zoom_preview_new (drawable);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (lens_correction_preview),
                            drawable);

  table = gtk_table_new (6, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Main:"), 120, 6,
                              vals.square_a, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.square_a);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Edge:"), 120, 6,
                              vals.quad_a, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.quad_a);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Zoom:"), 120, 6,
                              vals.scale_a, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.scale_a);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Brighten:"), 120, 6,
                              vals.brighten, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.brighten);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_X shift:"), 120, 6,
                              vals.centre_x, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.centre_x);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
                              _("_Y shift:"), 120, 6,
                              vals.centre_y, -100.0, 100.0, 0.1, 1.0, 3,
                              TRUE, 0, 0,
                              NULL, NULL);
  adjustments = g_slist_append (adjustments, adj);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &vals.centre_y);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (lens_response),
                    &run);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  gtk_widget_show (dialog);

  gtk_main ();

  g_slist_free (adjustments);
  adjustments = NULL;

  return run;
}
