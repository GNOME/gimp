/* Tiler v0.31
 * 22 May 1997
 * Tim Rowley <tor@cs.brown.edu>
 */

/* TODO:
 * + better basis function
 */

/* History:
 * v0.1: initial version
 * v0.2: fix edge conditions
 * v0.3: port to 0.99 API
 * v0.31: small bugfix
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


/* Declare local functions.
 */
static void query (void);
static void run   (gchar      *name,
		   gint        nparams,
		   GimpParam  *param,
		   gint       *nreturn_vals,
		   GimpParam **return_vals);

static void tile  (GimpDrawable *drawable);
static gint scale (gint          width,
		   gint          height,
		   gint          x,
		   gint          y,
		   gint          data);


GimpPlugInInfo PLUG_IN_INFO =
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
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
  };

  gimp_install_procedure ("plug_in_make_seamless",
			  "Seamless tile creation",
			  "This plugin creates a seamless tileable from the input image",
			  "Tim Rowley",
			  "Tim Rowley",
			  "1997",
			  N_("<Image>/Filters/Map/Make Seamless"),
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);
}


static void
run (gchar      *name,
     gint        nparams,
     GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N();

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id))
    {
      gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
      tile(drawable);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
	gimp_displays_flush ();
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
}

static gint
scale (gint width,
       gint height,
       gint x,
       gint y,
       gint data)
{
  gint A = width/2-1;
  gint B = height/2-1;
  gint a, b;

  if (x < width/2)
    a = width/2 - x - 1; 
  else
    a = x - width/2 - (width & 1);

  if (y < height/2)
    b = height/2 - y -1; 
  else
    b = y - height/2 - (height & 1);
  
  if ((B*a<A*b) || ((B*a==A*b) && (a&1)))
    {
      a = A-a;
      b = B-b;
      if (a==A)
	return data;
      else
	return data - data * (A*B-a*B) /(A * b + A * B - a * B);
    }
  else
    {
      if (a==A)
	return 0;
      else 
	return data * (A * B - a * B) / (A * b + A * B - a * B);
  }
}

static void
tile_region (GimpDrawable *drawable, gboolean left,
	     gint x1, gint y1, gint x2, gint y2)
{
  glong      width, height;
  gint       bpp;
  gint       wodd, hodd;
  gint	     w, h, x, y;
  gint	     rgn1_x, rgn2_x, off_x;
  static gint progress = 0;
  gint       max_progress;
  GimpPixelRgn  src1_rgn, src2_rgn, dest1_rgn, dest2_rgn;
  gpointer     pr;

  bpp = gimp_drawable_bpp (drawable->drawable_id);

  height = y2 - y1;
  width = x2 - x1;

  wodd = width & 1;
  hodd = height & 1;

  w = width / 2;
  h = height / 2;

  if (left) 
    {
      rgn1_x = x1;
      rgn2_x = x1 + w + wodd;
      off_x = w + wodd;
    } 
  else
    {
      rgn1_x = x1 + w + wodd;
      rgn2_x = x1;
      off_x = -w - wodd;
    }

  gimp_pixel_rgn_init (&src1_rgn, drawable, rgn1_x, y1, w, h, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest1_rgn, drawable, rgn1_x, y1, w, h, TRUE, TRUE);
  gimp_pixel_rgn_init (&src2_rgn, drawable, rgn2_x, y1 + h + hodd, 
		       w, h, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest2_rgn, drawable, rgn2_x, y1 + h + hodd, 
		       w, h, TRUE, TRUE);

  max_progress = width * height / 2;

  for (pr = gimp_pixel_rgns_register (4, &src1_rgn, &dest1_rgn, &src2_rgn,
				      &dest2_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      guchar *src1  = src1_rgn.data;
      guchar *dest1 = dest1_rgn.data;
      guchar *src2  = src2_rgn.data;
      guchar *dest2 = dest2_rgn.data;
      gint row = src1_rgn.y - y1;

      for (y = 0; y < src1_rgn.h; y++, row++)
	{
	  guchar *s1 = src1;
	  guchar *s2 = src2;
	  guchar *d1 = dest1;
	  guchar *d2 = dest2;
	  gint col = src1_rgn.x - x1;

	  for (x = 0; x < src1_rgn.w; x++, col++)
	    {
	      gint c;

	      for (c = 0; c < bpp; c++)
		{
		  gint val = scale (width, height, col, row, s1[c]) + 
		     scale (width, height, col + off_x, row + h + hodd, s2[c]);

		  d1[c] = d2[c] = (val > 255) ? 255 : val;
		}
	      s1 += bpp;
	      s2 += bpp;
	      d1 += bpp;
	      d2 += bpp;
	    }

	  src1 += src1_rgn.rowstride;
	  src2 += src2_rgn.rowstride;
	  dest1 += dest1_rgn.rowstride;
	  dest2 += dest2_rgn.rowstride;
	}
      progress += src1_rgn.w * src1_rgn.h;
      gimp_progress_update ((gdouble) progress / (gdouble) max_progress);
    }
}

static void
copy_region (GimpDrawable *drawable, gint x, gint y, gint w, gint h)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  gpointer     pr;

  gimp_pixel_rgn_init (&src_rgn, drawable, x, y, w, h, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x, y, w, h, TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      gint k;

      for (k = 0; k < src_rgn.h; k++)
	{
	  memcpy (dest_rgn.data + k * dest_rgn.rowstride,
		  src_rgn.data + k * src_rgn.rowstride,
		  src_rgn.w * src_rgn.bpp);
	}
    }
}

static void
tile (GimpDrawable *drawable)
{
  glong      width, height;
  gint       x1, y1, x2, y2;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
  gimp_progress_init (_("Tiler..."));
  
  height = y2 - y1;
  width = x2 - x1;

  if (width & 1) /* Copy middle column */
    {
       copy_region (drawable, x1 + width / 2, y1, 1, height);
    }

  if (height & 1) /* Copy middle row */
    {
      copy_region (drawable, x1, y1 + height / 2, width, 1);
    }

  tile_region (drawable, TRUE, x1, y1, x2, y2);
  tile_region (drawable, FALSE, x1, y1, x2, y2);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
}
