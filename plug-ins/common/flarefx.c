/*
 * This is the FlareFX plug-in for the GIMP 0.99
 * Version 1.05
 *
 * Copyright (C) 1997-1998 Karl-Johan Andersson (t96kja@student.tdb.uu.se)
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
 * Please send any comments or suggestions to me,
 * Karl-Johan Andersson (t96kja@student.tdb.uu.se)
 *
 * TODO:
 * - add "streaks" from lightsource
 * - improve the user interface
 * - speed it up
 * - more flare types, more control (color, size, intensity...)
 *
 * Missing something? - please contact me!
 *
 * May 2000 - tim copperfield [timecop@japan.co.jp]
 * preview window now draws a "mini flarefx" to show approximate
 * positioning after final render.
 *
 * Note, the algorithm does not render into an alpha channel.
 * Therefore, changed RGB* to RGB in the capabilities.
 * Someone who actually knows something about graphics should
 * take a look to see why this doesnt render on alpha channel :)
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "gimpoldpreview.h"

/* --- Defines --- */
#define ENTRY_WIDTH  75
#define PREVIEW_MASK (GDK_EXPOSURE_MASK | \
                      GDK_BUTTON_PRESS_MASK | \
                      GDK_BUTTON1_MOTION_MASK)

#define PREVIEW   0x1
#define CURSOR    0x2
#define ALL       0xf

/* --- Typedefs --- */
typedef struct
{
  gint posx;
  gint posy;
} FlareValues;

typedef struct RGBFLOAT
{
  gfloat r;
  gfloat g;
  gfloat b;
} RGBfloat;

typedef struct REFLECT
{
  RGBfloat ccol;
  gfloat size;
  gint   xp;
  gint   yp;
  gint   type;
} Reflect;

typedef struct
{
  GimpDrawable *drawable;
  gint       dwidth;
  gint       dheight;
  gint       bpp;
  GtkObject *xadj;
  GtkObject *yadj;
  gint       cursor;
  gint       curx, cury;                 /* x,y of cursor in preview */
  gint       oldx, oldy;
  gint       in_call;
} FlareCenter;

/* --- Declare local functions --- */
static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

static void FlareFX                    (GimpDrawable *drawable,
                                        gint          preview_mode);
static gint flare_dialog               (GimpDrawable *drawable);

static GtkWidget * flare_center_create            (GimpDrawable  *drawable);
static void        flare_center_destroy           (GtkWidget     *widget,
                                                   gpointer       data);
static void        flare_center_draw              (FlareCenter   *center,
                                                   gint           update);
static void        flare_center_adjustment_update (GtkAdjustment *adjustment,
                                                   gpointer       data);
static void        flare_center_cursor_update     (FlareCenter   *center);
static gint        flare_center_preview_expose    (GtkWidget     *widget,
                                                   GdkEvent      *event);
static gint        flare_center_preview_events    (GtkWidget     *widget,
                                                   GdkEvent      *event);

static void mcolor  (guchar *s, gfloat h);
static void mglow   (guchar *s, gfloat h);
static void minner  (guchar *s, gfloat h);
static void mouter  (guchar *s, gfloat h);
static void mhalo   (guchar *s, gfloat h);
static void initref (gint sx, gint sy, gint width, gint height, gint matt);
static void fixpix  (guchar *data, float procent, RGBfloat colpro);
static void mrt1    (guchar *s, gint i, gint col, gint row);
static void mrt2    (guchar *s, gint i, gint col, gint row);
static void mrt3    (guchar *s, gint i, gint col, gint row);
static void mrt4    (guchar *s, gint i, gint col, gint row);

/* --- Variables --- */
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static FlareValues fvals =
{
  128, 128              /* posx, posy */
};

static gfloat          scolor, sglow, sinner, souter; /* size     */
static gfloat          shalo;
static gint            xs, ys;
static gint            numref;
static RGBfloat        color, glow, inner, outer, halo;
static Reflect         ref1[19];
static GimpOldPreview *preview;
static gboolean        show_cursor = FALSE;

/* --- Functions --- */
MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    { GIMP_PDB_INT32, "posx", "X-position" },
    { GIMP_PDB_INT32, "posy", "Y-position" }
  };

  gimp_install_procedure ("plug_in_flarefx",
                          "Add lens flare effects",
                          "Adds a lens flare effects.  Makes your image look like it was snapped with a cheap camera with a lot of lens :)",
                          "Karl-Johan Andersson", /* Author */
                          "Karl-Johan Andersson", /* Copyright */
                          "May 2000",
                          /* don't translate '<Image>' entry,
                           * it is keyword for the gtk toolkit */
                          N_("<Image>/Filters/Light Effects/_FlareFX..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
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
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_flarefx", &fvals);

      /*  First acquire information with a dialog  */
      if (! flare_dialog (drawable))
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
        {
          fvals.posx = (gint) param[3].data.d_int32;
          fvals.posy = (gint) param[4].data.d_int32;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_flarefx", &fvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_progress_init (_("Render Flare..."));
          gimp_tile_cache_ntiles (2 *
                                  (drawable->width / gimp_tile_width () + 1));

          FlareFX (drawable, 0);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data ("plug_in_flarefx", &fvals, sizeof (FlareValues));
        }
      else
        {
          /* gimp_message ("FlareFX: cannot operate on indexed color images"); */
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static gint
flare_dialog (GimpDrawable *drawable)
{
  GtkWidget   *dlg;
  GtkWidget   *main_vbox;
  GtkWidget   *frame;
  FlareCenter *center;
  gboolean     run;

  gimp_ui_init ("flarefx", TRUE);

  dlg = gimp_dialog_new (_("FlareFX"), "flarefx",
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-flarefx",

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_OK,     GTK_RESPONSE_OK,

                         NULL);

  /*  parameter settings  */
  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);


  frame = flare_center_create (drawable);
  center = g_object_get_data (G_OBJECT (frame), "center");
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

  gtk_widget_show (frame);
  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

/* --- Filter functions --- */
static void
FlareFX (GimpDrawable *drawable,
         gboolean   preview_mode)
{
  GimpPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  guchar *dest, *d;
  guchar *cur_row, *s;
  gint row, col, i;
  gint x1, y1, x2, y2;
  gint matt;
  gfloat hyp;

  if (preview_mode)
    {
      width  = preview->width;
      height = preview->height;
      bytes  = preview->bpp;

      xs = (gdouble)fvals.posx * preview->scale_x;
      ys = (gdouble)fvals.posy * preview->scale_y;

      x1 = y1 = 0;
      x2 = width;
      y2 = height;
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
      width  = drawable->width;
      height = drawable->height;
      bytes  = drawable->bpp;

      xs = fvals.posx; /* set x,y of flare center */
      ys = fvals.posy;
    }

  matt = width;

  if (preview_mode)
    {
      cur_row = g_new (guchar, preview->rowstride);
      dest    = g_new (guchar, preview->rowstride);
    }
  else
    {
      /*  initialize the pixel regions  */
      gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
      gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

      cur_row = g_new (guchar, (x2 - x1) * bytes);
      dest    = g_new (guchar, (x2 - x1) * bytes);
    }

  scolor = (gfloat)matt * 0.0375;
  sglow  = (gfloat)matt * 0.078125;
  sinner = (gfloat)matt * 0.1796875;
  souter = (gfloat)matt * 0.3359375;
  shalo  = (gfloat)matt * 0.084375;

  color.r = 239.0/255.0; color.g = 239.0/255.0; color.b = 239.0/255.0;
  glow.r  = 245.0/255.0; glow.g  = 245.0/255.0; glow.b  = 245.0/255.0;
  inner.r = 255.0/255.0; inner.g = 38.0/255.0;  inner.b = 43.0/255.0;
  outer.r = 69.0/255.0;  outer.g = 59.0/255.0;  outer.b = 64.0/255.0;
  halo.r  = 80.0/255.0;  halo.g  = 15.0/255.0;  halo.b  = 4.0/255.0;

  initref (xs, ys, width, height, matt);

  /*  Loop through the rows */
  for (row = y1; row < y2; row++) /* y-coord */
    {
      if (preview_mode)
        memcpy (cur_row,
                preview->cache + preview->rowstride * row,
                preview->rowstride);
      else
        gimp_pixel_rgn_get_row (&srcPR, cur_row, x1, row, x2-x1);

      d = dest;
      s = cur_row;
      for (col = x1; col < x2; col++) /* x-coord */
        {
          hyp = hypot (col-xs, row-ys);
          mcolor (s, hyp); /* make color */
          mglow (s, hyp);  /* make glow  */
          minner (s, hyp); /* make inner */
          mouter (s, hyp); /* make outer */
          mhalo (s, hyp);  /* make halo  */
          for (i = 0; i < numref; i++)
            {
              switch (ref1[i].type)
                {
                case 1:
                  mrt1 (s, i, col, row);
                  break;
                case 2:
                  mrt2 (s, i, col, row);
                  break;
                case 3:
                  mrt3 (s, i, col, row);
                  break;
                case 4:
                  mrt4 (s, i, col, row);
                  break;
                }
            }
          s+=bytes;
        }
      if (preview_mode)
        {
          gimp_old_preview_do_row (preview, row, preview->width, cur_row);
        }
      else
        {
          /*  store the dest  */
          gimp_pixel_rgn_set_row (&destPR, cur_row, x1, row, (x2 - x1));
        }

      if ((row % 5) == 0 && !preview_mode)
        gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  if (preview_mode)
    {
      gtk_widget_queue_draw (preview->widget);
    }
  else
    {
      /*  update the textured region  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
    }

  g_free (cur_row);
  g_free (dest);
}

static void
mcolor (guchar *s,
        gfloat  h)
{
  static gfloat procent;

  procent = scolor - h;
  procent/=scolor;
  if (procent > 0.0)
    {
      procent*=procent;
      fixpix (s, procent, color);
    }
}

static void
mglow (guchar *s,
       gfloat  h)
{
  static gfloat procent;

  procent = sglow - h;
  procent/=sglow;
  if (procent > 0.0)
    {
      procent*=procent;
      fixpix (s, procent, glow);
    }
}

static void
minner (guchar *s,
        gfloat  h)
{
  static gfloat procent;

  procent = sinner - h;
  procent/=sinner;
  if (procent > 0.0)
    {
      procent*=procent;
      fixpix (s, procent, inner);
    }
}

static void
mouter (guchar *s,
        gfloat  h)
{
  static gfloat procent;

  procent = souter - h;
  procent/=souter;
  if (procent > 0.0)
    fixpix (s, procent, outer);
}

static void
mhalo (guchar *s,
       gfloat  h)
{
  static gfloat procent;

  procent = h - shalo;
  procent/=(shalo*0.07);
  procent = fabs (procent);
  if (procent < 1.0)
    fixpix (s, 1.0 - procent, halo);
}

static void
fixpix (guchar   *data,
        float     procent,
        RGBfloat  colpro)
{
  data[0] = data[0] + (255 - data[0]) * procent * colpro.r;
  data[1] = data[1] + (255 - data[1]) * procent * colpro.g;
  data[2] = data[2] + (255 - data[2]) * procent * colpro.b;
}

static void
initref (gint sx,
         gint sy,
         gint width,
         gint height,
         gint matt)
{
  gint xh, yh, dx, dy;

  xh = width / 2; yh = height / 2;
  dx = xh - sx;   dy = yh - sy;
  numref = 19;
  ref1[0].type=1; ref1[0].size=(gfloat)matt*0.027;
  ref1[0].xp=0.6699*dx+xh; ref1[0].yp=0.6699*dy+yh;
  ref1[0].ccol.r=0.0; ref1[0].ccol.g=14.0/255.0; ref1[0].ccol.b=113.0/255.0;
  ref1[1].type=1; ref1[1].size=(gfloat)matt*0.01;
  ref1[1].xp=0.2692*dx+xh; ref1[1].yp=0.2692*dy+yh;
  ref1[1].ccol.r=90.0/255.0; ref1[1].ccol.g=181.0/255.0; ref1[1].ccol.b=142.0/255.0;
  ref1[2].type=1; ref1[2].size=(gfloat)matt*0.005;
  ref1[2].xp=-0.0112*dx+xh; ref1[2].yp=-0.0112*dy+yh;
  ref1[2].ccol.r=56.0/255.0; ref1[2].ccol.g=140.0/255.0; ref1[2].ccol.b=106.0/255.0;
  ref1[3].type=2; ref1[3].size=(gfloat)matt*0.031;
  ref1[3].xp=0.6490*dx+xh; ref1[3].yp=0.6490*dy+yh;
  ref1[3].ccol.r=9.0/255.0; ref1[3].ccol.g=29.0/255.0; ref1[3].ccol.b=19.0/255.0;
  ref1[4].type=2; ref1[4].size=(gfloat)matt*0.015;
  ref1[4].xp=0.4696*dx+xh; ref1[4].yp=0.4696*dy+yh;
  ref1[4].ccol.r=24.0/255.0; ref1[4].ccol.g=14.0/255.0; ref1[4].ccol.b=0.0;
  ref1[5].type=2; ref1[5].size=(gfloat)matt*0.037;
  ref1[5].xp=0.4087*dx+xh; ref1[5].yp=0.4087*dy+yh;
  ref1[5].ccol.r=24.0/255.0; ref1[5].ccol.g=14.0/255.0; ref1[5].ccol.b=0.0;
  ref1[6].type=2; ref1[6].size=(gfloat)matt*0.022;
  ref1[6].xp=-0.2003*dx+xh; ref1[6].yp=-0.2003*dy+yh;
  ref1[6].ccol.r=42.0/255.0; ref1[6].ccol.g=19.0/255.0; ref1[6].ccol.b=0.0;
  ref1[7].type=2; ref1[7].size=(gfloat)matt*0.025;
  ref1[7].xp=-0.4103*dx+xh; ref1[7].yp=-0.4103*dy+yh;
  ref1[7].ccol.b=17.0/255.0; ref1[7].ccol.g=9.0/255.0; ref1[7].ccol.r=0.0;
  ref1[8].type=2; ref1[8].size=(gfloat)matt*0.058;
  ref1[8].xp=-0.4503*dx+xh; ref1[8].yp=-0.4503*dy+yh;
  ref1[8].ccol.b=10.0/255.0; ref1[8].ccol.g=4.0/255.0; ref1[8].ccol.r=0.0;
  ref1[9].type=2; ref1[9].size=(gfloat)matt*0.017;
  ref1[9].xp=-0.5112*dx+xh; ref1[9].yp=-0.5112*dy+yh;
  ref1[9].ccol.r=5.0/255.0; ref1[9].ccol.g=5.0/255.0; ref1[9].ccol.b=14.0/255.0;
  ref1[10].type=2; ref1[10].size=(gfloat)matt*0.2;
  ref1[10].xp=-1.496*dx+xh; ref1[10].yp=-1.496*dy+yh;
  ref1[10].ccol.r=9.0/255.0; ref1[10].ccol.g=4.0/255.0; ref1[10].ccol.b=0.0;
  ref1[11].type=2; ref1[11].size=(gfloat)matt*0.5;
  ref1[11].xp=-1.496*dx+xh; ref1[11].yp=-1.496*dy+yh;
  ref1[11].ccol.r=9.0/255.0; ref1[11].ccol.g=4.0/255.0; ref1[11].ccol.b=0.0;
  ref1[12].type=3; ref1[12].size=(gfloat)matt*0.075;
  ref1[12].xp=0.4487*dx+xh; ref1[12].yp=0.4487*dy+yh;
  ref1[12].ccol.r=34.0/255.0; ref1[12].ccol.g=19.0/255.0; ref1[12].ccol.b=0.0;
  ref1[13].type=3; ref1[13].size=(gfloat)matt*0.1;
  ref1[13].xp=dx+xh; ref1[13].yp=dy+yh;
  ref1[13].ccol.r=14.0/255.0; ref1[13].ccol.g=26.0/255.0; ref1[13].ccol.b=0.0;
  ref1[14].type=3; ref1[14].size=(gfloat)matt*0.039;
  ref1[14].xp=-1.301*dx+xh; ref1[14].yp=-1.301*dy+yh;
  ref1[14].ccol.r=10.0/255.0; ref1[14].ccol.g=25.0/255.0; ref1[14].ccol.b=13.0/255.0;
  ref1[15].type=4; ref1[15].size=(gfloat)matt*0.19;
  ref1[15].xp=1.309*dx+xh; ref1[15].yp=1.309*dy+yh;
  ref1[15].ccol.r=9.0/255.0; ref1[15].ccol.g=0.0; ref1[15].ccol.b=17.0/255.0;
  ref1[16].type=4; ref1[16].size=(gfloat)matt*0.195;
  ref1[16].xp=1.309*dx+xh; ref1[16].yp=1.309*dy+yh;
  ref1[16].ccol.r=9.0/255.0; ref1[16].ccol.g=16.0/255.0; ref1[16].ccol.b=5.0/255.0;
  ref1[17].type=4; ref1[17].size=(gfloat)matt*0.20;
  ref1[17].xp=1.309*dx+xh; ref1[17].yp=1.309*dy+yh;
  ref1[17].ccol.r=17.0/255.0; ref1[17].ccol.g=4.0/255.0; ref1[17].ccol.b=0.0;
  ref1[18].type=4; ref1[18].size=(gfloat)matt*0.038;
  ref1[18].xp=-1.301*dx+xh; ref1[18].yp=-1.301*dy+yh;
  ref1[18].ccol.r=17.0/255.0; ref1[18].ccol.g=4.0/255.0; ref1[18].ccol.b=0.0;
}

static void
mrt1 (guchar *s,
      gint    i,
      gint    col,
      gint    row)
{
  static gfloat procent;

  procent = ref1[i].size - hypot (ref1[i].xp - col, ref1[i].yp - row);
  procent/=ref1[i].size;
  if (procent > 0.0)
    {
      procent*=procent;
      fixpix (s, procent, ref1[i].ccol);
    }
}

static void
mrt2 (guchar *s,
      gint    i,
      gint    col,
      gint    row)
{
  static gfloat procent;

  procent = ref1[i].size - hypot (ref1[i].xp - col, ref1[i].yp - row);
  procent/=(ref1[i].size * 0.15);
  if (procent > 0.0)
    {
      if (procent > 1.0) procent = 1.0;
      fixpix (s, procent, ref1[i].ccol);
    }
}

static void
mrt3 (guchar *s,
      gint    i,
      gint    col,
      gint    row)
{
  static gfloat procent;

  procent = ref1[i].size - hypot (ref1[i].xp - col, ref1[i].yp - row);
  procent/=(ref1[i].size * 0.12);
  if (procent > 0.0)
    {
      if (procent > 1.0) procent = 1.0 - (procent * 0.12);
      fixpix (s, procent, ref1[i].ccol);
    }
}

static void
mrt4 (guchar *s,
      gint    i,
      gint    col,
      gint    row)
{
  static gfloat procent;

  procent = hypot (ref1[i].xp - col, ref1[i].yp - row) - ref1[i].size;
  procent/=(ref1[i].size*0.04);
  procent = fabs (procent);
  if (procent < 1.0)
    fixpix(s, 1.0 - procent, ref1[i].ccol);
}

/*=================================================================
    CenterFrame

    A frame that contains one preview and 2 entrys, used for positioning
    of the center of Flare.
    This whole thing is just too ugly, but I don't want to dig into it
     - tim
==================================================================*/

/*
 * Create new CenterFrame, and return it (GtkFrame).
 */

static GtkWidget *
flare_center_create (GimpDrawable *drawable)
{
  FlareCenter *center;
  GtkWidget   *frame;
  GtkWidget   *table;
  GtkWidget   *label;
  GtkWidget   *pframe;
  GtkWidget   *spinbutton;
  GtkWidget   *check;

  center = g_new (FlareCenter, 1);
  center->drawable = drawable;
  center->dwidth   = gimp_drawable_width(drawable->drawable_id );
  center->dheight  = gimp_drawable_height(drawable->drawable_id );
  center->bpp      = gimp_drawable_bpp(drawable->drawable_id);
  if (gimp_drawable_has_alpha (drawable->drawable_id))
    center->bpp--;
  center->cursor   = FALSE;
  center->curx     = 0;
  center->cury     = 0;
  center->oldx     = 0;
  center->oldy     = 0;
  center->in_call  = TRUE;  /* to avoid side effects while initialization */

  frame = gtk_frame_new (_("Center of FlareFX"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);

  g_signal_connect (frame, "destroy",
                    G_CALLBACK (flare_center_destroy),
                    center);

  table = gtk_table_new (3, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new_with_mnemonic (_("_X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5 );
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton =
    gimp_spin_button_new (&center->xadj,
                          fvals.posx, G_MININT, G_MAXINT,
                          1, 10, 10, 0, 0);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_widget_show (spinbutton);

  g_object_set_data (G_OBJECT (center->xadj), "center", center);

  g_signal_connect (center->xadj, "value_changed",
                    G_CALLBACK (flare_center_adjustment_update),
                    &fvals.posx);

  label = gtk_label_new_with_mnemonic (_("_Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5 );
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton =
    gimp_spin_button_new (&center->yadj,
                          fvals.posy, G_MININT, G_MAXINT,
                          1, 10, 10, 0, 0);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 3, 4, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_widget_show (spinbutton);

  g_object_set_data (G_OBJECT (center->yadj), "center", center);

  g_signal_connect (center->yadj, "value_changed",
                    G_CALLBACK (flare_center_adjustment_update),
                    &fvals.posy);

  /* frame (shadow_in) that contains preview */
  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), pframe, 0, 4, 1, 2, 0, 0, 0, 0);

  /* PREVIEW */
  preview = gimp_old_preview_new (drawable, FALSE);
  gtk_widget_set_events (GTK_WIDGET (preview->widget), PREVIEW_MASK);
  gtk_container_add (GTK_CONTAINER (pframe), preview->widget);
  gtk_widget_show (preview->widget);

  g_object_set_data (G_OBJECT (preview->widget), "center", center);

  g_signal_connect_after (preview->widget, "expose_event",
                          G_CALLBACK (flare_center_preview_expose),
                          center);
  g_signal_connect (preview->widget, "event",
                    G_CALLBACK (flare_center_preview_events),
                    center);

  gtk_widget_show (pframe);
  gtk_widget_show (table);
  g_object_set_data (G_OBJECT (frame), "center", center);
  gtk_widget_show (frame);

  /* show / hide cursor */
  check = gtk_check_button_new_with_mnemonic (_("_Show Cursor"));
  gtk_table_attach (GTK_TABLE (table), check, 0, 4, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), show_cursor);
  gtk_widget_show (check);

  g_signal_connect (check, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &show_cursor);
  g_signal_connect_swapped (check, "toggled",
                            G_CALLBACK (FlareFX),
                            drawable);

  flare_center_cursor_update (center);

  center->cursor = FALSE;    /* Make sure that the cursor has not been drawn */
  center->in_call = FALSE;   /* End of initialization */

  FlareFX (drawable, TRUE);

  return frame;
}

static void
flare_center_destroy (GtkWidget *widget,
                      gpointer   data)
{
  FlareCenter *center = data;
  g_free (center);
}

/*
 *   Drawing CenterFrame
 *   if update & PREVIEW, draw preview
 *   if update & CURSOR,  draw cross cursor
 */

static void
flare_center_draw (FlareCenter *center,
                   gint         update)
{
  GtkWidget *prvw = preview->widget;

  if (update & PREVIEW)
    {
      center->cursor = FALSE;
    }

  if (update & CURSOR)
    {
      gdk_gc_set_function (prvw->style->black_gc, GDK_INVERT);

      if (show_cursor)
        {
          if (center->cursor)
            {
              gdk_draw_line (prvw->window,
                             prvw->style->black_gc,
                             center->oldx, 1,
                             center->oldx,
                             preview->height - 1);
              gdk_draw_line (prvw->window,
                             prvw->style->black_gc,
                             1, center->oldy,
                             preview->width - 1,
                             center->oldy);
            }

          gdk_draw_line (prvw->window,
                         prvw->style->black_gc,
                         center->curx, 1,
                         center->curx,
                         preview->height - 1);
          gdk_draw_line (prvw->window,
                         prvw->style->black_gc,
                         1, center->cury,
                         preview->width - 1,
                         center->cury);
        }

      /* current position of cursor is updated */
      center->oldx = center->curx;
      center->oldy = center->cury;
      center->cursor = TRUE;

      gdk_gc_set_function (prvw->style->black_gc, GDK_COPY);
    }
}


/*
 *  CenterFrame entry callback
 */

static void
flare_center_adjustment_update (GtkAdjustment *adjustment,
                                gpointer       data)
{
  FlareCenter *center;

  gimp_int_adjustment_update (adjustment, data);

  center = g_object_get_data (G_OBJECT (adjustment), "center");

  if (!center->in_call)
    {
      flare_center_cursor_update (center);
      flare_center_draw (center, CURSOR);
      FlareFX(center->drawable, 1);
    }
}

/*
 *  Update the cross cursor's coordinates accoding to pvals.[xy]center
 *  but not redraw it
 */

static void
flare_center_cursor_update (FlareCenter *center)
{
  center->curx = fvals.posx * preview->width / center->dwidth;
  center->cury = fvals.posy * preview->height / center->dheight;

  center->curx = CLAMP (center->curx, 0, preview->width - 1);
  center->cury = CLAMP (center->cury, 0, preview->height - 1);
}

/*
 *    Handle the expose event on the preview
 */
static gint
flare_center_preview_expose (GtkWidget *widget,
                             GdkEvent  *event)
{
  FlareCenter *center;

  center = g_object_get_data (G_OBJECT (widget), "center");
  flare_center_draw (center, ALL);
  return FALSE;
}

/*
 *    Handle other events on the preview
 */

static gint
flare_center_preview_events (GtkWidget *widget,
                             GdkEvent  *event)
{
  FlareCenter    *center;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;

  center = g_object_get_data (G_OBJECT (widget), "center");

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      center->curx = bevent->x;
      center->cury = bevent->y;
      goto mouse;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if (!mevent->state)
        break;
      center->curx = mevent->x;
      center->cury = mevent->y;
    mouse:
      flare_center_draw (center, CURSOR);
      center->in_call = TRUE;
      gtk_adjustment_set_value (GTK_ADJUSTMENT (center->xadj),
                                center->curx * center->dwidth /
                                preview->width);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (center->yadj),
                                center->cury * center->dheight /
                                preview->height);
      center->in_call = FALSE;
      FlareFX(center->drawable, 1);
      break;

    default:
      break;
    }

  return FALSE;
}
