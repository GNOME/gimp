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
 * version 1.200
 * This plug-in requires GIMP v0.99.10 or above.
 *
 * This plug-in produces an effect like a supernova burst.
 *
 *      Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 *      http://ha1.seikyou.ne.jp/home/taka/gimp/
 *
 *      Preview render mode by timecop@japan.co.jp
 *      http://www.ne.jp/asahi/linux/timecop
 *
 * Changes from version 1.122 to version 1.200 by tim copperfield:
 * - preview mode now previews the nova with scale;
 * - toggle for cursor show/hide during preview
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
 * - Modified proc args to GIMP_PDB_COLOR, also included nspoke.
 * - nova_int_entryscale_new(): Fixed caption was guchar -> gchar, etc.
 * - Now nova renders properly with alpha channel.
 *   (Very thanks to Spencer Kimball and Federico Mena !)
 *
 * TODO:
 * - clean up the code more
 * - add notebook interface and so on
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#ifdef RCSID
static char rcsid[] = "$Id$";
#endif

/* Some useful macros */

#if 0
#define DEBUG1 printf
#else
#define DEBUG1 dummy_printf
static void dummy_printf(char *fmt, ...) {}
#endif

#define ENTRY_WIDTH      50
#define SCALE_WIDTH     125
#define TILE_CACHE_SIZE  16
#define PREVIEW_SIZE    128 


#define PREVIEW   0x1
#define CURSOR    0x2
#define ALL       0xf

#define PREVIEW_MASK (GDK_EXPOSURE_MASK | \
                      GDK_BUTTON_PRESS_MASK | \
                      GDK_BUTTON1_MOTION_MASK)

static  guchar    *preview_bits;
static  GtkWidget *preview;
static  gdouble    preview_scale_x;
static  gdouble    preview_scale_y;
static  gboolean   show_cursor = FALSE;

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
  GimpDrawable *drawable;
  gint       dwidth;
  gint       dheight;
  gint       bpp;
  GtkObject *xadj;
  GtkObject *yadj;
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
			  GimpParam   *param,
			  gint     *nreturn_vals,
			  GimpParam  **return_vals);

static void        fill_preview_with_thumb (GtkWidget *preview_widget, 
					    gint32     drawable_ID);
static GtkWidget  *preview_widget          (GimpDrawable *drawable);

static void        nova                    (GimpDrawable *drawable, 
					    gboolean   preview_mode);

static gint        nova_dialog             (GimpDrawable *drawable);
static void        nova_ok_callback        (GtkWidget *widget,
				            gpointer   data);

static GtkWidget * nova_center_create            (GimpDrawable     *drawable);
static void        nova_center_destroy           (GtkWidget     *widget,
						  gpointer       data);
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

GimpPlugInInfo PLUG_IN_INFO =
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


MAIN ()

static void
query (void)
{
  static GimpParamDef args[]=
  {
    { GIMP_PDB_INT32,    "run_mode",  "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "Input image (unused)" },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable" },
    { GIMP_PDB_INT32,    "xcenter",   "X coordinates of the center of supernova" },
    { GIMP_PDB_INT32,    "ycenter",   "Y coordinates of the center of supernova" },
    { GIMP_PDB_COLOR,    "color",     "Color of supernova" },
    { GIMP_PDB_INT32,    "radius",    "Radius of supernova" },
    { GIMP_PDB_INT32,    "nspoke",    "Number of spokes" },
    { GIMP_PDB_INT32,    "randomhue", "Random hue" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_nova",
                          "Produce Supernova effect to the specified drawable",
                          "This plug-in produces an effect like a supernova "
			  "burst. The amount of the light effect is "
			  "approximately in proportion to 1/r, where r is the "
			  "distance from the center of the star. It works with "
			  "RGB*, GRAY* image.",
                          "Eiichi Takamori",
                          "Eiichi Takamori",
                          "May 2000",
                          N_("<Image>/Filters/Light Effects/SuperNova..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          nargs, 0,
                          args, NULL);
}

static void
run (gchar    *name,
     gint      nparams,
     GimpParam   *param,
     gint     *nreturn_vals,
     GimpParam  **return_vals)
{
  static GimpParam values[1];
  GimpDrawable *drawable;
  GimpRunModeType run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  INIT_I18N_UI();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_nova", &pvals);

      /*  First acquire information with a dialog  */
      if (! nova_dialog (drawable))
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 9)
        status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
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
      if ((status == GIMP_PDB_SUCCESS) &&
	  pvals.radius <= 0)
        status = GIMP_PDB_CALLING_ERROR;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_nova", &pvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->id) ||
	  gimp_drawable_is_gray (drawable->id))
        {
          gimp_progress_init (_("Rendering SuperNova..."));
          gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

          nova (drawable, 0);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data ("plug_in_nova", &pvals, sizeof (NovaValues));
	    g_free(preview_bits); /* this is allocated during preview render */
        }
      else
        {
          /* gimp_message ("nova: cannot operate on indexed color images"); */
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static GtkWidget *
preview_widget (GimpDrawable *drawable)
{
  gint       size;
  GtkWidget *preview;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  fill_preview_with_thumb (preview, drawable->id);
  size = GTK_PREVIEW (preview)->rowstride * GTK_PREVIEW (preview)->buffer_height;
  preview_bits = g_malloc (size);
  memcpy (preview_bits, GTK_PREVIEW (preview)->buffer, size);

  return preview;
}

static void
fill_preview_with_thumb (GtkWidget *widget, 
			 gint32     drawable_ID)
{
  guchar  *drawable_data;
  gint     bpp;
  gint     x,y;
  gint     width  = PREVIEW_SIZE;
  gint     height = PREVIEW_SIZE;
  guchar  *src;
  gdouble  r, g, b, a;
  gdouble  c0, c1;
  guchar  *p0, *p1;
  guchar  *even, *odd;

  bpp = 0; /* Only returned */
  
  drawable_data = 
    gimp_drawable_get_thumbnail_data (drawable_ID, &width, &height, &bpp);

  if (width < 1 || height < 1)
    return;

  gtk_preview_size (GTK_PREVIEW (widget), width, height);
  preview_scale_x = (gdouble)width  / (gdouble)gimp_drawable_width  (drawable_ID);
  preview_scale_y = (gdouble)height / (gdouble)gimp_drawable_height (drawable_ID);

  even = g_malloc (width * 3);
  odd  = g_malloc (width * 3);
  src = drawable_data;

  for (y = 0; y < height; y++)
    {
      p0 = even;
      p1 = odd;
      
      for (x = 0; x < width; x++) 
	{
	  if(bpp == 4)
	    {
	      r =  ((gdouble)src[x*4+0])/255.0;
	      g = ((gdouble)src[x*4+1])/255.0;
	      b = ((gdouble)src[x*4+2])/255.0;
	      a = ((gdouble)src[x*4+3])/255.0;
	    }
	  else if(bpp == 3)
	    {
	      r =  ((gdouble)src[x*3+0])/255.0;
	      g = ((gdouble)src[x*3+1])/255.0;
	      b = ((gdouble)src[x*3+2])/255.0;
	      a = 1.0;
	    }
	  else
	    {
	      r = ((gdouble)src[x*bpp+0])/255.0;
	      g = b = r;
	      if(bpp == 2)
		a = ((gdouble)src[x*bpp+1])/255.0;
	      else
		a = 1.0;
	    }
	  
	  if ((x / GIMP_CHECK_SIZE_SM) & 1) 
	    {
	      c0 = GIMP_CHECK_LIGHT;
	      c1 = GIMP_CHECK_DARK;
	    } 
	  else 
	    {
	      c0 = GIMP_CHECK_DARK;
	      c1 = GIMP_CHECK_LIGHT;
	    }
	  
	*p0++ = (c0 + (r - c0) * a) * 255.0;
	*p0++ = (c0 + (g - c0) * a) * 255.0;
	*p0++ = (c0 + (b - c0) * a) * 255.0;
	
	*p1++ = (c1 + (r - c1) * a) * 255.0;
	*p1++ = (c1 + (g - c1) * a) * 255.0;
	*p1++ = (c1 + (b - c1) * a) * 255.0;
	
      } /* for */
      
      if ((y / GIMP_CHECK_SIZE_SM) & 1)
	gtk_preview_draw_row (GTK_PREVIEW (widget), (guchar *)odd,  0, y, width);
      else
	gtk_preview_draw_row (GTK_PREVIEW (widget), (guchar *)even, 0, y, width);

      src += width * bpp;
    }

  g_free (even);
  g_free (odd);
  g_free (drawable_data);
}


/*******************/
/*   Main Dialog   */
/*******************/

static gint
nova_dialog (GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *center_frame;
  GtkObject *adj;

  gimp_ui_init ("nova", TRUE);

  dlg = gimp_dialog_new (_("SuperNova"), "nova",
			 gimp_standard_help_func, "filters/nova.html",
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
  gtk_signal_connect_object (GTK_OBJECT (button), "color_changed",
			     GTK_SIGNAL_FUNC (nova),
			     (gpointer)drawable);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Color:"), 1.0, 0.5,
			     button, 1, TRUE);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
			      _("Radius:"), SCALE_WIDTH, 0,
			      pvals.radius, 1, 100, 1, 10, 0,
			      FALSE, 1, GIMP_MAX_IMAGE_SIZE,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &pvals.radius);
  gtk_signal_connect_object (GTK_OBJECT (adj), "value_changed",
			     GTK_SIGNAL_FUNC (nova),
			     (gpointer)drawable);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
			      _("Spokes:"), SCALE_WIDTH, 0,
			      pvals.nspoke, 1, 1024, 1, 16, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &pvals.nspoke);
  gtk_signal_connect_object (GTK_OBJECT (adj), "value_changed",
			     GTK_SIGNAL_FUNC (nova),
			     (gpointer)drawable);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 5,
			      _("Random Hue:"), SCALE_WIDTH, 0,
			      pvals.randomhue, 0, 360, 1, 15, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &pvals.randomhue);
  gtk_signal_connect_object (GTK_OBJECT (adj), "value_changed",
			     GTK_SIGNAL_FUNC (nova),
			     (gpointer)drawable);

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
nova_center_create (GimpDrawable *drawable)
{
  NovaCenter *center;
  GtkWidget  *frame;
  GtkWidget  *table;
  GtkWidget  *label;
  GtkWidget  *pframe;
  GtkWidget  *spinbutton;
  GtkWidget  *check;

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

  table = gtk_table_new (3, 4, FALSE);
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
  preview = preview_widget (center->drawable);
  gtk_widget_set_events (GTK_WIDGET (preview), PREVIEW_MASK);
  gtk_signal_connect_after (GTK_OBJECT (preview), "expose_event",
			    GTK_SIGNAL_FUNC (nova_center_preview_expose),
			    center);
  gtk_signal_connect (GTK_OBJECT (preview), "event",
                      GTK_SIGNAL_FUNC (nova_center_preview_events),
                      center);
  gtk_container_add (GTK_CONTAINER (pframe), preview);
  gtk_widget_show (preview);

  gtk_widget_show (pframe);
  gtk_widget_show (table);
  gtk_object_set_user_data (GTK_OBJECT (frame), center);
  gtk_widget_show (frame);

  check = gtk_check_button_new_with_label (_("Show Cursor"));
  gtk_table_attach (GTK_TABLE (table), check, 0, 4, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), show_cursor);
  gtk_signal_connect (GTK_OBJECT (check), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &show_cursor);
  gtk_signal_connect_object (GTK_OBJECT (check), "toggled",
			     GTK_SIGNAL_FUNC (nova),
			     (gpointer)drawable);
  gtk_widget_show (check);
  
  nova_center_cursor_update (center);

  center->cursor  = FALSE;   /* Make sure that the cursor has not been drawn */
  center->in_call = FALSE;   /* End of initialization */
  nova (drawable, TRUE); 

  DEBUG1("pvals center=%d,%d\n", pvals.xcenter, pvals.ycenter);
  DEBUG1("center cur=%d,%d\n", center->curx, center->cury);

  return frame;
}

static void
nova_center_destroy (GtkWidget *widget,
		     gpointer   data)
{
  NovaCenter *center = data;
  g_free (center);
}


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
      gdk_gc_set_function (preview->style->black_gc, GDK_INVERT);
      if (show_cursor) 
	{
	  if (center->cursor)
	    {
	      gdk_draw_line (preview->window,
			     preview->style->black_gc,
			     center->oldx, 1, center->oldx, 
			     (GTK_PREVIEW (preview)->buffer_height) - 1);
	      gdk_draw_line (preview->window,
			     preview->style->black_gc,
			     1, center->oldy, 
			     (GTK_PREVIEW (preview)->buffer_width)-1, center->oldy);
	    }

	  gdk_draw_line (preview->window,
			 preview->style->black_gc,
			 center->curx, 1, center->curx, 
			 (GTK_PREVIEW (preview)->buffer_height) - 1);
	  gdk_draw_line (preview->window,
			 preview->style->black_gc,
			 1, center->cury, 
			 (GTK_PREVIEW (preview)->buffer_width)-1, center->cury);
	}
      /* current position of cursor is updated */
      center->oldx = center->curx;
      center->oldy = center->cury;
      center->cursor = TRUE;
      gdk_gc_set_function (preview->style->black_gc, GDK_COPY);
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
      nova (center->drawable, TRUE);
    }
}

/*
 *  Update the cross cursor's  coordinates accoding to pvals.[xy]center
 *  but not redraw it
 */

static void
nova_center_cursor_update (NovaCenter *center)
{
  center->curx = pvals.xcenter * GTK_PREVIEW (preview)->buffer_width / center->dwidth;
  center->cury = pvals.ycenter * GTK_PREVIEW (preview)->buffer_height / center->dheight;

  if (center->curx < 0)
    center->curx = 0;
  else if (center->curx >= GTK_PREVIEW (preview)->buffer_width)
    center->curx = GTK_PREVIEW (preview)->buffer_width - 1;

  if (center->cury < 0)
    center->cury = 0;
  else if (center->cury >= GTK_PREVIEW (preview)->buffer_height)
    center->cury = GTK_PREVIEW (preview)->buffer_height - 1;
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
				GTK_PREVIEW (preview)->buffer_width);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (center->yadj),
				center->cury * center->dheight /
				GTK_PREVIEW (preview)->buffer_height);
      center->in_call = FALSE;
      nova (center->drawable, 1);
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

static gdouble
gauss (void)
{
  gdouble sum = 0;
  gint i;

  for (i = 0; i < 6; i++)
    sum += (gdouble) rand () / G_MAXRAND;

  return sum / 6;
}

static void
nova (GimpDrawable *drawable, 
      gboolean   preview_mode)
{
   GimpPixelRgn src_rgn, dest_rgn;
   gpointer pr;
   guchar *src_row, *dest_row;
   guchar *src, *dest, *save_dest;
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
   gdouble h, s;
   gdouble spokecol;
   gint    i, j;
   gint    color[4];
   guchar  *spokecolor;
 
   /* initialize */
 
   new_alpha = 0.0;
 
   srand (time (NULL));
   spoke = g_new (gdouble, pvals.nspoke);
   spokecolor = g_new (guchar, 3 * pvals.nspoke);

   gimp_rgb_to_hsv4 (pvals.color, &h, &s, &v);

   for (i = 0; i < pvals.nspoke; i++)
     {
       spoke[i] = gauss();
       h += ((gdouble) pvals.randomhue / 360.0 *
	     ((gdouble) rand() / (gdouble) G_MAXRAND - 0.5));
       if (h < 0)
	 h += 1.0;
       else if (h >= 1.0)
	 h -= 1.0;
       gimp_hsv_to_rgb4 (&spokecolor[3*i], h, s, v);       
     }

   has_alpha = gimp_drawable_has_alpha (drawable->id);
   if (preview_mode) 
     {
       xc = (gdouble)pvals.xcenter * preview_scale_x;
       yc = (gdouble)pvals.ycenter * preview_scale_y;

       x1 = y1 = 0;
       x2 = GTK_PREVIEW (preview)->buffer_width;
       y2 = GTK_PREVIEW (preview)->buffer_height;
       bpp = GTK_PREVIEW (preview)->bpp;
       has_alpha = 0;
       alpha = bpp;
     } 
   else 
     {
       gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
       bpp = gimp_drawable_bpp (drawable->id);
       alpha = (has_alpha) ? bpp - 1 : bpp;
       xc = pvals.xcenter;
       yc = pvals.ycenter;

       gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, x2-x1, y2-y1, FALSE, FALSE);
       gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, x2-x1, y2-y1, TRUE, TRUE);
     }
 
   l0 = (x2-xc)/4+1;   /* standard length */
 
   /* Initialize progress */
   progress = 0;
   max_progress = (x2 - x1) * (y2 - y1);

   if (preview_mode) 
     {
       /* thanks to this pixel rgn register
	  stuff, i have to duplicate the
	  entire loop.  why not just use
	  get_row??? */
       src_row   = g_malloc (y2 * GTK_PREVIEW (preview)->rowstride);
       memcpy (src_row, preview_bits, y2 * GTK_PREVIEW (preview)->rowstride);
       dest_row  = g_malloc (y2 * GTK_PREVIEW (preview)->rowstride);
       save_dest = dest_row;
 
       for (row = 0, y = 0; row < y2; row++, y++)
         {
           src = src_row;
           dest = dest_row;
 
           for (col = 0, x = 0; col < x2; col++, x++)
             {
               u = (gdouble) (x-xc) / (pvals.radius * preview_scale_x);
               v = (gdouble) (y-yc) / (pvals.radius * preview_scale_y);
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
		   spokecol = (gdouble)spokecolor[3*i+j]*(1.0-c) + 
		              (gdouble)spokecolor[3*((i + 1) % pvals.nspoke)+j]*c;
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
 
               src += bpp;
               dest += bpp;
             }
           src_row  += GTK_PREVIEW (preview)->rowstride;
           dest_row += GTK_PREVIEW (preview)->rowstride;
         }

       memcpy (GTK_PREVIEW(preview)->buffer, save_dest, y2 * GTK_PREVIEW (preview)->rowstride);
       gtk_widget_queue_draw (preview);
     } 
   else 
     { /* normal mode */
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
		       spokecol = (gdouble)spokecolor[3*i+j]*(1.0-c) + 
			          (gdouble)spokecolor[3*((i + 1) % pvals.nspoke)+j]*c;
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
     }
 
   g_free (spoke);
   g_free (spokecolor);
}
