/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * SuperNova plug-in
 * Copyright (C) 1997 Eiichi Takamori <taka@ma1.seikyou.ne.jp>,
 *                    Spencer Kimball, Federico Mena Quintero
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

/*
 * version 1.122
 * This plug-in requires GIMP v0.99.10 or above.
 *
 * This plug-in produces an effect like a supernova burst.
 *
 *      Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 *      http://ha1.seikyou.ne.jp/home/taka/gimp/
 *
 *
 * Changes from version 1.1115 to version 1.122 by Martin Weber:
 * - Little bug fixes
 * - Added random hue
 * - Freeing memory
 *
 * Changes from version 1.1114 to version 1.1115:
 * - Added gtk_rc_parse
 * - Fixed bug that drawing preview of small height image
 *   (maybe image height < PREVIEW_SIZE) caused core dump.
 * - Changed default value.
 * - Moved to <Image>/Filters/Effects.  right?
 * - etc...
 *
 * Changes from version 1.1112 to version 1.1114:
 * - Modified proc args to PARAM_COLOR, also included nspoke.
 * - nova_int_entryscale_new(): Fixed caption was guchar -> gchar, etc.
 * - Now nova renders properly with alpha channel.
 *   (Very thanks to Spencer Kimball and Federico Mena !)
 *
 * TODO:
 * - clean up the code more
 * - fix preview
 * - add notebook interface and so on
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/gimpmath.h>
#include <libgimp/gimplimits.h>

#include "libgimp/stdplugins-intl.h"

#ifdef RCSID
static char rcsid[] = "$Id$";
#endif

/* Some useful macros */

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif /* RAND_MAX */

#if 0
#define DEBUG1 printf
#else
#define DEBUG1 dummy_printf
static void dummy_printf(char *fmt, ...) {}
#endif

#define ENTRY_WIDTH      50
#define SCALE_WIDTH     125
#define PREVIEW_SIZE    100
#define TILE_CACHE_SIZE  16

#define PREVIEW   0x1
#define CURSOR    0x2
#define ALL       0xf

#define PREVIEW_MASK (GDK_EXPOSURE_MASK | \
                      GDK_BUTTON_PRESS_MASK | \
                      GDK_BUTTON1_MOTION_MASK)

typedef struct
{
  gint    xcenter, ycenter;
  guchar  color[3];
  gint    radius;
  gint    nspoke;
  gint    randomhue;
} NovaValues;

typedef struct
{
  gint run;
} NovaInterface;

typedef struct
{
  GDrawable *drawable;
  gint       dwidth;
  gint       dheight;
  gint       bpp;
  GtkObject *xadj;
  GtkObject *yadj;
  GtkWidget *preview;
  gint       pwidth;
  gint       pheight;
  gint       cursor;
  gint       curx, cury;              /* x,y of cursor in preview */
  gint       oldx, oldy;
  gint       in_call;
} NovaCenter;


/* Declare a local function.
 */
static void        query (void);
static void        run   (gchar    *name,
			  gint      nparams,
			  GParam   *param,
			  gint     *nreturn_vals,
			  GParam  **return_vals);

static void        nova  (GDrawable *drawable);

static gint        nova_dialog      (GDrawable *drawable);
static void        nova_ok_callback (GtkWidget *widget,
				     gpointer   data);

static GtkWidget * nova_center_create            (GDrawable     *drawable);
static void        nova_center_destroy           (GtkWidget     *widget,
						  gpointer       data);
static void        nova_center_preview_init      (NovaCenter    *center);
static void        nova_center_draw              (NovaCenter    *center,
						  gint           update);
static void        nova_center_adjustment_update (GtkAdjustment *widget,
						  gpointer       data);
static void        nova_center_cursor_update     (NovaCenter    *center);
static gint        nova_center_preview_expose    (GtkWidget     *widget,
						  GdkEvent      *event,
						  gpointer       data);
static gint        nova_center_preview_events    (GtkWidget     *widget,
						  GdkEvent      *event,
						  gpointer       data);

static void        rgb_to_hsl (gdouble  r, gdouble  g, gdouble  b,
			       gdouble *h, gdouble *s, gdouble *l);
static void        hsl_to_rgb (gdouble  h, gdouble  s, gdouble  l,
			       gdouble *r, gdouble *g, gdouble *b);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static NovaValues pvals =
{
  128, 128,             /* xcenter, ycenter */
  { 90, 100, 255 },     /* color */
  20,                   /* radius */
  100,                  /* nspoke */
  0                     /* random hue */
};

static NovaInterface pint =
{
  FALSE     /* run */
};


MAIN()

static void
query (void)
{
  static GParamDef args[]=
    {
      { PARAM_INT32,    "run_mode",  "Interactive, non-interactive" },
      { PARAM_IMAGE,    "image",     "Input image (unused)" },
      { PARAM_DRAWABLE, "drawable",  "Input drawable" },
      { PARAM_INT32,    "xcenter",   "X coordinates of the center of supernova" },
      { PARAM_INT32,    "ycenter",   "Y coordinates of the center of supernova" },
      { PARAM_COLOR,    "color",     "Color of supernova" },
      { PARAM_INT32,    "radius",    "Radius of supernova" },
      { PARAM_INT32,    "nspoke",    "Number of spokes" },
      { PARAM_INT32,    "randomhue", "Random hue" }
   };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_nova",
                          _("Produce Supernova effect to the specified drawable"),
                          _("This plug-in produces an effect like a supernova burst. The "
			    "amount of the light effect is approximately in proportion to 1/r, "
			    "where r is the distance from the center of the star. It works with "
			    "RGB*, GRAY* image."),
                          "Eiichi Takamori",
                          "Eiichi Takamori",
                          "1997",
                          N_("<Image>/Filters/Light Effects/SuperNova..."),
                          "RGB*, GRAY*",
                          PROC_PLUG_IN,
                          nargs, nreturn_vals,
                          args, return_vals);
}

static void
run (gchar    *name,
     gint      nparams,
     GParam   *param,
     gint     *nreturn_vals,
     GParam  **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  INIT_I18N_UI();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_nova", &pvals);

      /*  First acquire information with a dialog  */
      if (! nova_dialog (drawable))
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 9)
        status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
        {
          pvals.xcenter   = param[3].data.d_int32;
          pvals.ycenter   = param[4].data.d_int32;
          pvals.color[0]  = param[5].data.d_color.red;
          pvals.color[1]  = param[5].data.d_color.green;
          pvals.color[2]  = param[5].data.d_color.blue;
          pvals.radius    = param[6].data.d_int32;
          pvals.nspoke    = param[7].data.d_int32;
	  pvals.randomhue = param[8].data.d_int32;
        }
      if ((status == STATUS_SUCCESS) &&
	  pvals.radius <= 0)
        status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_nova", &pvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) ||
	  gimp_drawable_is_gray (drawable->id))
        {
          gimp_progress_init (_("Rendering SuperNova..."));
          gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

          nova (drawable);

          if (run_mode != RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == RUN_INTERACTIVE)
            gimp_set_data ("plug_in_nova", &pvals, sizeof (NovaValues));
        }
      else
        {
          /* gimp_message ("nova: cannot operate on indexed color images"); */
          status = STATUS_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

/*******************/
/*   Main Dialog   */
/*******************/

static gint
nova_dialog (GDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *center_frame;
  GtkObject *adj;
  guchar  *color_cube;
  gchar  **argv;
  gint     argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("nova");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
                              color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

#if 0
  g_print ("Waiting... (pid %d)\n", getpid ());
  kill (getpid (), 19); /* SIGSTOP */
#endif

  dlg = gimp_dialog_new (_("SuperNova"), "nova",
			 gimp_plugin_help_func, "filters/nova.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), nova_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  center_frame = nova_center_create (drawable);
  gtk_table_attach (GTK_TABLE (table), center_frame, 0, 3, 0, 1,
                    0, 0, 0, 0);

  button = gimp_color_button_new (_("SuperNova Color Picker"), 
				  SCALE_WIDTH - 8, 16, 
				  pvals.color, 3);
  gimp_table_attach_aligned (GTK_TABLE (table), 1,
			     _("Color:"), 1.0, 0.5,
			     button, TRUE);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Radius:"), SCALE_WIDTH, 0,
			      pvals.radius, 1, 100, 1, 10, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &pvals.radius);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
			      _("Spokes:"), SCALE_WIDTH, 0,
			      pvals.nspoke, 1, 1024, 1, 16, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &pvals.nspoke);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
			      _("Random Hue:"), SCALE_WIDTH, 0,
			      pvals.randomhue, 0, 360, 1, 15, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &pvals.randomhue);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return pint.run;
}

static void
nova_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  pint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}


/*=================================================================
    CenterFrame

    A frame that contains one preview and 2 entrys, used for positioning
    of the center of Nova.
==================================================================*/

/*
 * Create new CenterFrame, and return it (GtkFrame).
 */
static GtkWidget *
nova_center_create (GDrawable *drawable)
{
  NovaCenter *center;
  GtkWidget  *frame;
  GtkWidget  *table;
  GtkWidget  *label;
  GtkWidget  *pframe;
  GtkWidget  *preview;
  GtkWidget  *spinbutton;

  center = g_new (NovaCenter, 1);
  center->drawable = drawable;
  center->dwidth   = gimp_drawable_width (drawable->id);
  center->dheight  = gimp_drawable_height (drawable->id);
  center->bpp      = gimp_drawable_bpp (drawable->id);
  if (gimp_drawable_has_alpha (drawable->id))
    center->bpp--;
  center->cursor   = FALSE;
  center->curx     = 0;
  center->cury     = 0;
  center->oldx     = 0;
  center->oldy     = 0;
  center->in_call  = TRUE;  /* to avoid side effects while initialization */

  frame = gtk_frame_new (_("Center of SuperNova"));
  gtk_signal_connect (GTK_OBJECT (frame), "destroy",
                      GTK_SIGNAL_FUNC (nova_center_destroy),
                      center);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 4);

  table = gtk_table_new (2, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new (_("X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
		   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton =
    gimp_spin_button_new (&center->xadj,
			  pvals.xcenter, G_MININT, G_MAXINT,
			  1, 10, 10, 0, 0);
  gtk_object_set_user_data (GTK_OBJECT (center->xadj), center);
  gtk_signal_connect (GTK_OBJECT (center->xadj), "value_changed",
		      GTK_SIGNAL_FUNC (nova_center_adjustment_update),
		      &pvals.xcenter);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  label = gtk_label_new (_("Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton =
    gimp_spin_button_new (&center->yadj,
			  pvals.ycenter, G_MININT, G_MAXINT,
			  1, 10, 10, 0, 0);
  gtk_object_set_user_data (GTK_OBJECT (center->yadj), center);
  gtk_signal_connect (GTK_OBJECT (center->yadj), "value_changed",
		      GTK_SIGNAL_FUNC (nova_center_adjustment_update),
		      &pvals.ycenter);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 3, 4, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  /* frame that contains preview */
  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (table), pframe, 0, 4, 1, 2,
		    0, 0, 0, 0);

  /* PREVIEW */
  center->preview = preview =
    gtk_preview_new (center->bpp==3 ? GTK_PREVIEW_COLOR : GTK_PREVIEW_GRAYSCALE);
  gtk_widget_set_events (GTK_WIDGET (preview), PREVIEW_MASK);
  gtk_signal_connect_after (GTK_OBJECT (preview), "expose_event",
			    GTK_SIGNAL_FUNC (nova_center_preview_expose),
			    center);
  gtk_signal_connect (GTK_OBJECT (preview), "event",
                      GTK_SIGNAL_FUNC (nova_center_preview_events),
                      center);
  gtk_container_add (GTK_CONTAINER (pframe), center->preview);

  /*
   * Resize the greater one of dwidth and dheight to PREVIEW_SIZE
   */
  if (center->dwidth > center->dheight)
    {
      center->pheight = center->dheight * PREVIEW_SIZE / center->dwidth;
      center->pwidth = PREVIEW_SIZE;
    }
  else
    {
      center->pwidth = center->dwidth * PREVIEW_SIZE / center->dheight;
      center->pheight = PREVIEW_SIZE;
    }
  gtk_preview_size (GTK_PREVIEW (preview), center->pwidth, center->pheight);

  /* Draw the contents of preview, that is saved in the preview widget */
  nova_center_preview_init (center);
  gtk_widget_show (preview);

  gtk_widget_show (pframe);
  gtk_widget_show (table);
  gtk_widget_show (frame);

  nova_center_cursor_update (center);

  center->cursor = FALSE;    /* Make sure that the cursor has not been drawn */
  center->in_call = FALSE;   /* End of initialization */
  DEBUG1("pvals center=%d,%d\n", pvals.xcenter, pvals.ycenter);
  DEBUG1("center cur=%d,%d\n", center->curx, center->cury);
  return frame;
}

static void
nova_center_destroy (GtkWidget *widget,
		     gpointer   data)
{
  NovaCenter *center = data;
  g_free(center);
}

static void
render_preview (GtkWidget *preview,
		GPixelRgn *srcrgn);

/*
 *  Initialize preview
 *  Draw the contents into the internal buffer of the preview widget
 */
static void
nova_center_preview_init (NovaCenter *center)
{
  GtkWidget      *preview;
  GPixelRgn      src_rgn;
  gint           dwidth, dheight, pwidth, pheight, bpp;

  preview = center->preview;
  dwidth = center->dwidth;
  dheight = center->dheight;
  pwidth = center->pwidth;
  pheight = center->pheight;
  bpp = center->bpp;

  gimp_pixel_rgn_init (&src_rgn, center->drawable, 0, 0,
                        center->dwidth, center->dheight, FALSE, FALSE);
  render_preview(center->preview, &src_rgn);
}


/*======================================================================
                Preview Rendering Util routine
=======================================================================*/

#ifndef OPAQUE
#define OPAQUE     255
#endif

static void
render_preview (GtkWidget *preview, 
		GPixelRgn *srcrgn)
{
  guchar *src_row, *dest_row, *src, *dest;
  gint    row, col;
  gint    dwidth, dheight, pwidth, pheight;
  gint   *src_col;
  gint    bpp, alpha, has_alpha, b;
  guchar  check;

  dwidth  = srcrgn->w;
  dheight = srcrgn->h;

  if (GTK_PREVIEW (preview)->buffer)
    {
      pwidth  = GTK_PREVIEW (preview)->buffer_width;
      pheight = GTK_PREVIEW (preview)->buffer_height;
    }
  else
    {
      pwidth  = preview->requisition.width;
      pheight = preview->requisition.height;
    }

  bpp = srcrgn->bpp;
  alpha = bpp;
  has_alpha = gimp_drawable_has_alpha (srcrgn->drawable->id);
  if (has_alpha)
    alpha--;

  /*  printf("render_preview: %d %d %d", bpp, alpha, has_alpha);
      printf(" (%d %d %d %d)\n", dwidth, dheight, pwidth, pheight); */

  src_row  = g_new (guchar, dwidth * bpp);
  dest_row = g_new (guchar, pwidth * bpp);
  src_col  = g_new (gint, pwidth);

  for (col = 0; col < pwidth; col++)
    src_col[ col ] = (col * dwidth / pwidth) * bpp;

  for (row = 0; row < pheight; row++)
    {
      gimp_pixel_rgn_get_row (srcrgn, src_row,
                               0, row * dheight / pheight, dwidth);
      dest = dest_row;
      for (col = 0; col < pwidth; col++)
        {
          src = &src_row[ src_col[col] ];
          if(!has_alpha || src[alpha] == OPAQUE)
            {
              /* no alpha channel or opaque -- simple way */
              for (b = 0; b < alpha; b++)
                dest[b] = src[b];
            }
          else
            {
              /* more or less transparent */
              if ((col % (GIMP_CHECK_SIZE) < GIMP_CHECK_SIZE_SM) ^
                  (row % (GIMP_CHECK_SIZE) < GIMP_CHECK_SIZE_SM))
                check = GIMP_CHECK_LIGHT * 255;
              else
                check = GIMP_CHECK_DARK * 255;

              if (src[alpha] == 0)
                {
                  /* full transparent -- check */
                  for (b = 0; b < alpha; b++)
                    dest[b] = check;
                }
              else
                {
                  /* middlemost transparent -- mix check and src */
                  for (b = 0; b < alpha; b++)
                    dest[b] = (src[b] * src[alpha] +
			       check * (OPAQUE - src[alpha])) / OPAQUE;
                }
            }
          dest += alpha;
        }
      gtk_preview_draw_row (GTK_PREVIEW (preview), dest_row,
                            0, row, pwidth);
    }

  g_free (src_col);
  g_free (src_row);
  g_free (dest_row);
}

/*======================================================================
                Preview Rendering Util routine End
=======================================================================*/

/*
 *   Drawing CenterFrame
 *   if update & PREVIEW, draw preview
 *   if update & CURSOR,  draw cross cursor
 */

static void
nova_center_draw (NovaCenter *center, 
		  gint        update)
{
  if (update & PREVIEW)
    {
      center->cursor = FALSE;
      DEBUG1 ("draw-preview\n");
    }

  if (update & CURSOR)
    {
      DEBUG1 ("draw-cursor %d old=%d,%d cur=%d,%d\n",
	      center->cursor, center->oldx, center->oldy,
	      center->curx, center->cury);
      gdk_gc_set_function (center->preview->style->black_gc, GDK_INVERT);
      if (center->cursor)
        {
          gdk_draw_line (center->preview->window,
			 center->preview->style->black_gc,
			 center->oldx, 1, center->oldx, center->pheight - 1);
          gdk_draw_line (center->preview->window,
			 center->preview->style->black_gc,
			 1, center->oldy, center->pwidth-1, center->oldy);
        }
      gdk_draw_line (center->preview->window,
		     center->preview->style->black_gc,
		     center->curx, 1, center->curx, center->pheight - 1);
      gdk_draw_line (center->preview->window,
		     center->preview->style->black_gc,
		     1, center->cury, center->pwidth-1, center->cury);
      /* current position of cursor is updated */
      center->oldx = center->curx;
      center->oldy = center->cury;
      center->cursor = TRUE;
      gdk_gc_set_function (center->preview->style->black_gc, GDK_COPY);
    }
}

/*
 *  CenterFrame entry callback
 */

static void
nova_center_adjustment_update (GtkAdjustment *adjustment,
			       gpointer       data)
{
  NovaCenter *center;

  gimp_int_adjustment_update (adjustment, data);

  center = gtk_object_get_user_data (GTK_OBJECT (adjustment));

  if (!center->in_call)
    {
      nova_center_cursor_update (center);
      nova_center_draw (center, CURSOR);
    }
}

/*
 *  Update the cross cursor's  coordinates accoding to pvals.[xy]center
 *  but not redraw it
 */

static void
nova_center_cursor_update (NovaCenter *center)
{
  center->curx = pvals.xcenter * center->pwidth / center->dwidth;
  center->cury = pvals.ycenter * center->pheight / center->dheight;

  if (center->curx < 0)
    center->curx = 0;
  else if (center->curx >= center->pwidth)
    center->curx = center->pwidth - 1;

  if (center->cury < 0)
    center->cury = 0;
  else if (center->cury >= center->pheight)
    center->cury = center->pheight - 1;
}

/*
 *    Handle the expose event on the preview
 */
static gint
nova_center_preview_expose (GtkWidget *widget,
                            GdkEvent  *event,
			    gpointer   data)
{
  NovaCenter *center;

  center = (NovaCenter *) data;
  nova_center_draw (center, ALL);

  return FALSE;
}

/*
 *    Handle other events on the preview
 */

static gint
nova_center_preview_events (GtkWidget *widget,
			    GdkEvent  *event,
			    gpointer   data)
{
  NovaCenter *center;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;

  center = (NovaCenter *) data;

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
      nova_center_draw (center, CURSOR);
      center->in_call = TRUE;
      gtk_adjustment_set_value (GTK_ADJUSTMENT (center->xadj),
				center->curx * center->dwidth /
				center->pwidth);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (center->yadj),
				center->cury * center->dheight /
				center->pheight);
      center->in_call = FALSE;
      break;

    default:
      break;
    }

  return FALSE;
}

/*
  ################################################################
  ##                                                            ##
  ##                   Main Calculation                         ##
  ##                                                            ##
  ################################################################
*/

static double 
gauss (void)
{
  double sum=0;
  int i;
  for(i=0; i<6; i++)
    sum+=(double)rand()/RAND_MAX;
  return sum/6;
}

static void
nova (GDrawable *drawable)
{
   GPixelRgn src_rgn, dest_rgn;
   gpointer pr;
   guchar *src_row, *dest_row;
   guchar *src, *dest;
   gint x1, y1, x2, y2;
   gint row, col;
   gint x, y;
   gint alpha, has_alpha, bpp;
   gint progress, max_progress;
   /****************/
   gint xc, yc; /* center of image */
   gdouble u, v;
   gdouble l, l0;
   gdouble w, w1, c;
   gdouble *spoke;
   gdouble nova_alpha;
   gdouble src_alpha;
   gdouble new_alpha;
   gdouble compl_ratio;
   gdouble ratio;
   gdouble r, g, b, h, s, lu;
   gdouble spokecol;
   gint    i, j;
   gint    color[4];
   gint    *spokecolor;
 
   /* initialize */
 
   new_alpha = 0.0;
 
   srand (time (NULL));
   spoke = g_new (gdouble, pvals.nspoke);
   spokecolor = g_new (gint, 3 * pvals.nspoke);

   rgb_to_hsl (pvals.color[0] / 255.0,
	       pvals.color[1] / 255.0,
	       pvals.color[2]/255.0,
	       &h, &s, &lu);

   for (i = 0; i < pvals.nspoke; i++)
     {
       spoke[i] = gauss();
       h += ((gdouble) pvals.randomhue / 360.0 *
	     ((gdouble) rand() / (gdouble) RAND_MAX - 0.5));
       if (h < 0)
	 h += 1.0;
       else if (h >= 1.0)
	 h -= 1.0;
       hsl_to_rgb (h, s, lu, &r, &g, &b);	
       spokecolor[3*i+0] = 255.0 * r;
       spokecolor[3*i+1] = 255.0 * g;
       spokecolor[3*i+2] = 255.0 * b;
     }
 
   gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
 
   /*
   xc = (x1+x2)/2;
   yc = (y1+y2)/2; */
   xc = pvals.xcenter;
   yc = pvals.ycenter;
   l0 = (x2-xc)/4+1;   /* standard length */
 
   bpp = gimp_drawable_bpp (drawable->id);
   has_alpha = gimp_drawable_has_alpha (drawable->id);
   alpha = (has_alpha) ? bpp - 1 : bpp;
 
   gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, x2-x1, y2-y1, FALSE, FALSE);
   gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, x2-x1, y2-y1, TRUE, TRUE);
 
   /* Initialize progress */
   progress = 0;
   max_progress = (x2 - x1) * (y2 - y1);
 
   for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
        pr != NULL; pr = gimp_pixel_rgns_process (pr))
     {
       src_row = src_rgn.data;
       dest_row = dest_rgn.data;
 
       for (row = 0, y = src_rgn.y; row < src_rgn.h; row++, y++)
         {
           src = src_row;
           dest = dest_row;
 
           for (col = 0, x = src_rgn.x; col < src_rgn.w; col++, x++)
             {
               u = (gdouble) (x-xc) / pvals.radius;
               v = (gdouble) (y-yc) / pvals.radius;
               l = sqrt(u*u + v*v);
 
               /* This algorithm is still under construction. */
               c = (atan2 (u, v) / (2 * G_PI) + .51) * pvals.nspoke;
               i = (int) floor (c);
               c -= i;
               i %= pvals.nspoke;
               w1 = spoke[i] * (1 - c) + spoke[(i + 1) % pvals.nspoke] * c;
               w1 = w1 * w1;
 
               w = 1/(l+0.001)*0.9;
 
               nova_alpha = CLAMP (w, 0.0, 1.0);
 
               if (has_alpha)
                 {
                   src_alpha = (gdouble) src[alpha] / 255.0;
                   new_alpha = src_alpha + (1.0 - src_alpha) * nova_alpha;
 
                   if (new_alpha != 0.0)
                     ratio = nova_alpha / new_alpha;
                   else
                     ratio = 0.0;
                 }
               else
                 ratio = nova_alpha;
 
               compl_ratio = 1.0 - ratio;
 
               for (j = 0; j < alpha; j++)
                 {
		   spokecol = (gdouble)spokecolor[3*i+j]*(1.0-c) + (gdouble)spokecolor[3*((i + 1) % pvals.nspoke)+j]*c;
                   if (w > 1.0)
                     color[j] = CLAMP (spokecol * w, 0, 255);
                   else
                     color[j] = src[j] * compl_ratio + spokecol * ratio;
 
                   c = CLAMP (w1 * w, 0, 1);
                   color[j] = color[j] + 255 * c;
 
                   dest[j]= CLAMP (color[j], 0, 255);
                 }
 
               if (has_alpha)
                 dest[alpha] = new_alpha * 255.0;
 
               src += src_rgn.bpp;
               dest += dest_rgn.bpp;
             }
           src_row += src_rgn.rowstride;
           dest_row += dest_rgn.rowstride;
         }
       /* Update progress */
       progress += src_rgn.w * src_rgn.h;
       gimp_progress_update ((double) progress / (double) max_progress);
     }
 
   gimp_drawable_flush (drawable);
   gimp_drawable_merge_shadow (drawable->id, TRUE);
   gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
 
   g_free(spoke);
   g_free(spokecolor);
}
 
 /*
  * RGB-HSL transforms.
  * Ken Fishkin, Pixar Inc., January 1989.
  */

 /*
  * given r,g,b on [0 ... 1],
  * return (h,s,l) on [0 ... 1]
  */

static void
rgb_to_hsl (gdouble  r,
	    gdouble  g,
	    gdouble  b,
	    gdouble *h,
	    gdouble *s,
	    gdouble *l)
{
   gdouble v;
   gdouble m;
   gdouble vm;
   gdouble r2, g2, b2;

   v = MAX(r,g);
   v = MAX(v,b);
   m = MIN(r,g);
   m = MIN(m,b);

   if ((*l = (m + v) / 2.0) <= 0.0)
     return;
   if ((*s = vm = v - m) > 0.0)
     {
       *s /= (*l <= 0.5) ? (v + m) : (2.0 - v - m) ;
     }
   else
     return;

   r2 = (v - r) / vm;
   g2 = (v - g) / vm;
   b2 = (v - b) / vm;

   if (r == v)
     *h = (g == m ? 5.0 + b2 : 1.0 - g2);
   else if (g == v)
     *h = (b == m ? 1.0 + r2 : 3.0 - b2);
   else
     *h = (r == m ? 3.0 + g2 : 5.0 - r2);

   *h /= 6;
}

 /*
  * given h,s,l on [0..1],
  * return r,g,b on [0..1]
  */

static void
hsl_to_rgb (gdouble  h,
	    gdouble  sl,
	    gdouble  l,
	    gdouble *r,
	    gdouble *g,
	    gdouble *b)
{
   gdouble v;

   v = (l <= 0.5) ? (l * (1.0 + sl)) : (l + sl - l * sl);
   if (v <= 0)
     {
       *r = *g = *b = 0.0;
     }
   else
     {
       gdouble m;
       gdouble sv;
       gint sextant;
       gdouble fract, vsf, mid1, mid2;

       m = l + l - v;
       sv = (v - m) / v;
       h *= 6.0;
       sextant = h;
       fract = h - sextant;
       vsf = v * sv * fract;
       mid1 = m + vsf;
       mid2 = v - vsf;
       switch (sextant)
	 {
	   case 0: *r = v; *g = mid1; *b = m; break;
	   case 1: *r = mid2; *g = v; *b = m; break;
	   case 2: *r = m; *g = v; *b = mid1; break;
	   case 3: *r = m; *g = mid2; *b = v; break;
	   case 4: *r = mid1; *g = m; *b = v; break;
	   case 5: *r = v; *g = m; *b = mid2; break;
	 }
     }
}
 






