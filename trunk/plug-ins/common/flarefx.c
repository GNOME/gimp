/*
 * This is the FlareFX plug-in for GIMP 0.99
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-flarefx"
#define PLUG_IN_BINARY "flarefx"

/* --- Typedefs --- */
typedef struct
{
  gint     posx;
  gint     posy;
} FlareValues;

typedef struct REFLECT
{
  GimpRGB ccol;
  gfloat  size;
  gint    xp;
  gint    yp;
  gint    type;
} Reflect;

typedef struct
{
  GimpDrawable *drawable;
  GimpPreview  *preview;
  GtkWidget    *coords;
  gboolean      cursor_drawn;
  gint          curx, cury;                 /* x,y of cursor in preview */
  gint          oldx, oldy;
} FlareCenter;


/* --- Declare local functions --- */
static void        query                          (void);
static void        run                            (const gchar      *name,
                                                   gint              nparams,
                                                   const GimpParam  *param,
                                                   gint             *nreturn_vals,
                                                   GimpParam       **return_vals);

static void        FlareFX                        (GimpDrawable     *drawable,
                                                   GimpPreview      *preview);
static gboolean    flare_dialog                   (GimpDrawable     *drawable);

static GtkWidget * flare_center_create            (GimpDrawable     *drawable,
                                                   GimpPreview      *preview);
static void        flare_center_cursor_draw       (FlareCenter      *center);
static void        flare_center_coords_update     (GimpSizeEntry    *coords,
                                                   FlareCenter      *center);
static void        flare_center_cursor_update     (FlareCenter      *center);
static void        flare_center_preview_realize   (GtkWidget        *widget,
                                                   FlareCenter      *center);
static gboolean    flare_center_preview_expose    (GtkWidget        *widget,
                                                   GdkEvent         *event,
                                                   FlareCenter      *center);
static gboolean    flare_center_preview_events    (GtkWidget        *widget,
                                                   GdkEvent         *event,
                                                   FlareCenter      *center);

static void mcolor  (guchar  *s,
                     gfloat   h);
static void mglow   (guchar  *s,
                     gfloat   h);
static void minner  (guchar  *s,
                     gfloat   h);
static void mouter  (guchar  *s,
                     gfloat   h);
static void mhalo   (guchar  *s,
                     gfloat   h);
static void initref (gint     sx,
                     gint     sy,
                     gint     width,
                     gint     height,
                     gint     matt);
static void fixpix  (guchar  *data,
                     float    procent,
                     GimpRGB *colpro);
static void mrt1    (guchar  *s,
                     Reflect *ref,
                     gint     col,
                     gint     row);
static void mrt2    (guchar  *s,
                     Reflect *ref,
                     gint     col,
                     gint     row);
static void mrt3    (guchar  *s,
                     Reflect *ref,
                     gint     col,
                     gint     row);
static void mrt4    (guchar  *s,
                     Reflect *ref,
                     gint     col,
                     gint     row);


/* --- Variables --- */
const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static FlareValues fvals =
{
  128, 128   /* posx, posy */
};

static gfloat     scolor, sglow, sinner, souter; /* size     */
static gfloat     shalo;
static gint       xs, ys;
static gint       numref;
static GimpRGB    color, glow, inner, outer, halo;
static Reflect    ref1[19];
static gboolean   show_cursor = TRUE;


/* --- Functions --- */
MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"               },
    { GIMP_PDB_INT32,    "pos-x",    "X-position"                   },
    { GIMP_PDB_INT32,    "pos-y",    "Y-position"                   }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Add a lens flare effect"),
                          "Adds a lens flare effects.  Makes your image look "
                          "like it was snapped with a cheap camera with a lot "
                          "of lens :)",
                          "Karl-Johan Andersson", /* Author */
                          "Karl-Johan Andersson", /* Copyright */
                          "May 2000",
                          N_("Lens _Flare..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC,
                             "<Image>/Filters/Light and Shadow/Light");
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
      gimp_get_data (PLUG_IN_PROC, &fvals);

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
      gimp_get_data (PLUG_IN_PROC, &fvals);
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
          gimp_progress_init (_("Render lens flare"));
          gimp_tile_cache_ntiles (2 *
                                  (drawable->width / gimp_tile_width () + 1));

          FlareFX (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC, &fvals, sizeof (FlareValues));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static gboolean
flare_dialog (GimpDrawable *drawable)
{
  GtkWidget   *dialog;
  GtkWidget   *main_vbox;
  GtkWidget   *preview;
  GtkWidget   *frame;
  gboolean     run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Lens Flare"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

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

  preview = gimp_zoom_preview_new (drawable);
  gtk_widget_add_events (GIMP_PREVIEW (preview)->area,
                         GDK_POINTER_MOTION_MASK);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (FlareFX),
                            drawable);

  frame = flare_center_create (drawable, GIMP_PREVIEW (preview));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

/* --- Filter functions --- */
static void
FlareFX (GimpDrawable *drawable,
         GimpPreview  *preview)
{
  GimpPixelRgn  srcPR, destPR;
  gint          width, height;
  gint          bytes;
  guchar       *cur_row, *s;
  guchar       *src  = NULL;
  guchar       *dest = NULL;
  gint          row, col, i;
  gint          x1, y1, x2, y2;
  gint          matt;
  gfloat        hyp;
  gdouble       zoom = 0.0;

  bytes  = drawable->bpp;
  if (preview)
    {
      src = gimp_zoom_preview_get_source (GIMP_ZOOM_PREVIEW (preview),
                                          &width, &height, &bytes);

      zoom = gimp_zoom_preview_get_factor (GIMP_ZOOM_PREVIEW (preview));

      gimp_preview_transform (preview,
                              fvals.posx, fvals.posy, &xs, &ys);

      x1 = 0;
      y1 = 0;
      x2 = width;
      y2 = height;
      dest = g_new (guchar, bytes * width * height);
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
      width  = drawable->width;
      height = drawable->height;

      xs = fvals.posx; /* set x,y of flare center */
      ys = fvals.posy;
      /*  initialize the pixel regions  */
      gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
      gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);
    }

  if (preview)
    matt = width * zoom;
  else
    matt = width;

  cur_row = g_new (guchar, (x2 - x1) * bytes);

  scolor = (gfloat) matt * 0.0375;
  sglow  = (gfloat) matt * 0.078125;
  sinner = (gfloat) matt * 0.1796875;
  souter = (gfloat) matt * 0.3359375;
  shalo  = (gfloat) matt * 0.084375;

  color.r = 239.0/255.0; color.g = 239.0/255.0; color.b = 239.0/255.0;
  glow.r  = 245.0/255.0; glow.g  = 245.0/255.0; glow.b  = 245.0/255.0;
  inner.r = 255.0/255.0; inner.g = 38.0/255.0;  inner.b = 43.0/255.0;
  outer.r = 69.0/255.0;  outer.g = 59.0/255.0;  outer.b = 64.0/255.0;
  halo.r  = 80.0/255.0;  halo.g  = 15.0/255.0;  halo.b  = 4.0/255.0;

  initref (xs, ys, width, height, matt);

  /*  Loop through the rows */
  for (row = y1; row < y2; row++) /* y-coord */
    {
      if (preview)
        memcpy (cur_row, src + row * width * bytes, width * bytes);
      else
        gimp_pixel_rgn_get_row (&srcPR, cur_row, x1, row, x2-x1);

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
                  mrt1 (s, ref1 + i, col, row);
                  break;
                case 2:
                  mrt2 (s, ref1 + i, col, row);
                  break;
                case 3:
                  mrt3 (s, ref1 + i, col, row);
                  break;
                case 4:
                  mrt4 (s, ref1 + i, col, row);
                  break;
                }
            }
          s += bytes;
        }
      if (preview)
        {
          memcpy (dest + row * width * bytes, cur_row, width * bytes);
        }
      else
        {
          /*  store the dest  */
          gimp_pixel_rgn_set_row (&destPR, cur_row, x1, row, (x2 - x1));
        }

      if ((row % 5) == 0 && !preview)
        gimp_progress_update ((double) row / (double) (y2 - y1));
    }

  if (preview)
    {
      gimp_preview_draw_buffer (preview, dest, width * bytes);
      g_free (src);
      g_free (dest);
    }
  else
    {
      /*  update the textured region  */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
    }

  g_free (cur_row);
}

static void
mcolor (guchar *s,
        gfloat  h)
{
  gfloat procent;

  procent  = scolor - h;
  procent /= scolor;

  if (procent > 0.0)
    {
      procent *= procent;
      fixpix (s, procent, &color);
    }
}

static void
mglow (guchar *s,
       gfloat  h)
{
  gfloat procent;

  procent  = sglow - h;
  procent /= sglow;

  if (procent > 0.0)
    {
      procent *= procent;
      fixpix (s, procent, &glow);
    }
}

static void
minner (guchar *s,
        gfloat  h)
{
  gfloat procent;

  procent  = sinner - h;
  procent /= sinner;

  if (procent > 0.0)
    {
      procent *= procent;
      fixpix (s, procent, &inner);
    }
}

static void
mouter (guchar *s,
        gfloat  h)
{
  gfloat procent;

  procent  = souter - h;
  procent /= souter;

  if (procent > 0.0)
    fixpix (s, procent, &outer);
}

static void
mhalo (guchar *s,
       gfloat  h)
{
  gfloat procent;

  procent  = h - shalo;
  procent /= (shalo * 0.07);
  procent  = fabs (procent);

  if (procent < 1.0)
    fixpix (s, 1.0 - procent, &halo);
}

static void
fixpix (guchar   *data,
        float     procent,
        GimpRGB  *colpro)
{
  data[0] += (255 - data[0]) * procent * colpro->r;
  data[1] += (255 - data[1]) * procent * colpro->g;
  data[2] += (255 - data[2]) * procent * colpro->b;
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
mrt1 (guchar  *s,
      Reflect *ref,
      gint     col,
      gint     row)
{
  gfloat procent;

  procent  = ref->size - hypot (ref->xp - col, ref->yp - row);
  procent /= ref->size;

  if (procent > 0.0)
    {
      procent *= procent;
      fixpix (s, procent, &ref->ccol);
    }
}

static void
mrt2 (guchar *s,
      Reflect *ref,
      gint    col,
      gint    row)
{
  gfloat procent;

  procent  = ref->size - hypot (ref->xp - col, ref->yp - row);
  procent /= (ref->size * 0.15);

  if (procent > 0.0)
    {
      if (procent > 1.0)
        procent = 1.0;

      fixpix (s, procent, &ref->ccol);
    }
}

static void
mrt3 (guchar *s,
      Reflect *ref,
      gint    col,
      gint    row)
{
  gfloat procent;

  procent  = ref->size - hypot (ref->xp - col, ref->yp - row);
  procent /= (ref->size * 0.12);

  if (procent > 0.0)
    {
      if (procent > 1.0)
        procent = 1.0 - (procent * 0.12);

      fixpix (s, procent, &ref->ccol);
    }
}

static void
mrt4 (guchar *s,
      Reflect *ref,
      gint    col,
      gint    row)
{
  gfloat procent;

  procent  = hypot (ref->xp - col, ref->yp - row) - ref->size;
  procent /= (ref->size*0.04);
  procent  = fabs (procent);

  if (procent < 1.0)
    fixpix (s, 1.0 - procent, &ref->ccol);
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
flare_center_create (GimpDrawable *drawable,
                     GimpPreview  *preview)
{
  FlareCenter *center;
  GtkWidget   *frame;
  GtkWidget   *hbox;
  GtkWidget   *check;
  gint32       image_ID;
  gdouble      res_x;
  gdouble      res_y;

  center = g_new0 (FlareCenter, 1);

  center->drawable     = drawable;
  center->preview      = preview;
  center->cursor_drawn = FALSE;
  center->curx         = 0;
  center->cury         = 0;
  center->oldx         = 0;
  center->oldy         = 0;

  frame = gimp_frame_new (_("Center of Flare Effect"));

  g_object_set_data (G_OBJECT (frame), "center", center);

  g_signal_connect_swapped (frame, "destroy",
                            G_CALLBACK (g_free),
                            center);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  image_ID = gimp_drawable_get_image (drawable->drawable_id);
  gimp_image_get_resolution (image_ID, &res_x, &res_y);

  center->coords = gimp_coordinates_new (GIMP_UNIT_PIXEL, "%p", TRUE, TRUE, -1,
                                         GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                         FALSE, FALSE,

                                         _("_X:"), fvals.posx, res_x,
                                         - (gdouble) drawable->width,
                                         2 * drawable->width,
                                         0, drawable->width,

                                         _("_Y:"), fvals.posy, res_y,
                                         - (gdouble) drawable->height,
                                         2 * drawable->height,
                                         0, drawable->height);

  gtk_table_set_row_spacing (GTK_TABLE (center->coords), 1, 12);
  gtk_box_pack_start (GTK_BOX (hbox), center->coords, FALSE, FALSE, 0);
  gtk_widget_show (center->coords);

  g_signal_connect (center->coords, "value-changed",
                    G_CALLBACK (flare_center_coords_update),
                    center);
  g_signal_connect (center->coords, "refval-changed",
                    G_CALLBACK (flare_center_coords_update),
                    center);

  check = gtk_check_button_new_with_mnemonic (_("Show _position"));
  gtk_table_attach (GTK_TABLE (center->coords), check, 0, 5, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), show_cursor);
  gtk_widget_show (check);

  g_signal_connect (check, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &show_cursor);
  g_signal_connect_swapped (check, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  g_signal_connect (preview->area, "realize",
                    G_CALLBACK (flare_center_preview_realize),
                    center);
  g_signal_connect_after (preview->area, "expose-event",
                          G_CALLBACK (flare_center_preview_expose),
                          center);
  g_signal_connect (preview->area, "event",
                    G_CALLBACK (flare_center_preview_events),
                    center);

  flare_center_cursor_update (center);

  return frame;
}

/*
 *   Drawing CenterFrame
 *   if update & PREVIEW, draw preview
 *   if update & CURSOR,  draw cross cursor
 */

static void
flare_center_cursor_draw (FlareCenter *center)
{
  GtkWidget *prvw = center->preview->area;
  gint       width, height;

  gimp_preview_get_size (center->preview, &width, &height);

  gdk_gc_set_function (prvw->style->black_gc, GDK_INVERT);

  if (show_cursor)
    {
      if (center->cursor_drawn)
        {
          gdk_draw_line (prvw->window,
                         prvw->style->black_gc,
                         center->oldx, 1,
                         center->oldx,
                         height - 1);
          gdk_draw_line (prvw->window,
                         prvw->style->black_gc,
                         1, center->oldy,
                         width - 1,
                         center->oldy);
        }

      gdk_draw_line (prvw->window,
                     prvw->style->black_gc,
                     center->curx, 1,
                     center->curx,
                     height - 1);
      gdk_draw_line (prvw->window,
                     prvw->style->black_gc,
                     1, center->cury,
                     width - 1,
                     center->cury);
    }

  /* current position of cursor is updated */
  center->oldx         = center->curx;
  center->oldy         = center->cury;
  center->cursor_drawn = TRUE;

  gdk_gc_set_function (prvw->style->black_gc, GDK_COPY);
}


/*
 *  CenterFrame entry callback
 */
static void
flare_center_coords_update (GimpSizeEntry *coords,
                            FlareCenter   *center)
{
  fvals.posx = gimp_size_entry_get_refval (coords, 0);
  fvals.posy = gimp_size_entry_get_refval (coords, 1);

  flare_center_cursor_update (center);
  flare_center_cursor_draw (center);

  gimp_preview_invalidate (center->preview);
}

/*
 *  Update the cross cursor's coordinates accoding to pvals.[xy]center
 *  but not redraw it
 */
static void
flare_center_cursor_update (FlareCenter *center)
{
  gimp_preview_transform (center->preview,
                          fvals.posx, fvals.posy,
                          &center->curx, &center->cury);
}

/*
 *  Set the preview area's cursor on realize
 */
static void
flare_center_preview_realize (GtkWidget   *widget,
                              FlareCenter *center)
{
  GdkDisplay *display = gtk_widget_get_display (widget);
  GdkCursor  *cursor  = gdk_cursor_new_for_display (display, GDK_CROSSHAIR);

  gimp_preview_set_default_cursor (center->preview, cursor);
  gdk_cursor_unref (cursor);
}

/*
 *    Handle the expose event on the preview
 */
static gboolean
flare_center_preview_expose (GtkWidget   *widget,
                             GdkEvent    *event,
                             FlareCenter *center)
{
  center->cursor_drawn = FALSE;

  flare_center_cursor_update (center);
  flare_center_cursor_draw (center);

  return FALSE;
}

/*
 *    Handle other events on the preview
 */
static gboolean
flare_center_preview_events (GtkWidget   *widget,
                             GdkEvent    *event,
                             FlareCenter *center)
{
  gint tx, ty;

  switch (event->type)
    {
    case GDK_MOTION_NOTIFY:
      if (! (((GdkEventMotion *) event)->state & GDK_BUTTON1_MASK))
        break;

    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;

        if (bevent->button == 1)
          {
            gimp_preview_untransform (center->preview,
                                      bevent->x, bevent->y,
                                      &tx, &ty);

            flare_center_cursor_draw (center);

            g_signal_handlers_block_by_func (center->coords,
                                             flare_center_coords_update,
                                             center);

            gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (center->coords),
                                        0, tx);
            gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (center->coords),
                                        1, ty);

            g_signal_handlers_unblock_by_func (center->coords,
                                               flare_center_coords_update,
                                               center);

            flare_center_coords_update (GIMP_SIZE_ENTRY (center->coords), center);
          }
      }
      break;

    default:
      break;
    }

  return FALSE;
}
