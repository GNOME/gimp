/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * SuperNova plug-in
 * Copyright (C) 1997 Eiichi Takamori <taka@ma1.seikyou.ne.jp>,
 *		      Spencer Kimball, Federico Mena Quintero
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * version 1.1115
 * This plug-in requires GIMP v0.99.10 or above.
 *
 * This plug-in produces an effect like a supernova burst.
 *
 *	Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 *	http://ha1.seikyou.ne.jp/home/taka/gimp/
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#ifdef RCSID
static char rcsid[] = "$Id$";
#endif



/* Some useful macros */

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif /* RAND_MAX */

#if 0
#define DEBUG1 printf
#else
#define DEBUG1 dummy_printf
static void dummy_printf( char *fmt, ... ) {}
#endif

#define ENTRY_WIDTH 40
#define SCALE_WIDTH 125
#define PREVIEW_SIZE 100
#define TILE_CACHE_SIZE 16

#define PREVIEW	  0x1
#define CURSOR	  0x2
#define ALL	  0xf

#define PREVIEW_MASK   GDK_EXPOSURE_MASK | \
		       GDK_BUTTON_PRESS_MASK | \
		       GDK_BUTTON1_MOTION_MASK

typedef struct {
  gint	  xcenter, ycenter;
  gint	  color[3];
  gint	  radius;
  gint	  nspoke;
} NovaValues;

typedef struct {
  gint run;
} NovaInterface;

typedef struct {
  GtkObject	*adjustment;
  GtkWidget	*entry;
  gint		constraint;
} NovaEntryScaleData;

typedef struct
{
  GDrawable	*drawable;
  gint		dwidth, dheight;
  gint		bpp;
  GtkWidget	*xentry, *yentry;
  GtkWidget	*preview;
  gint		pwidth, pheight;
  gint		cursor;
  gint		curx, cury;		 /* x,y of cursor in preview */
  gint		oldx, oldy;
  gint		in_call;
} NovaCenter;


/* Declare a local function.
 */
static void	query	(void);
static void	run	(gchar	 *name,
			 gint	 nparams,
			 GParam	 *param,
			 gint	 *nreturn_vals,
			 GParam	 **return_vals);

static gint	nova_dialog ( GDrawable *drawable );

static GtkWidget * nova_center_create ( GDrawable *drawable );
static void	   nova_center_destroy ( GtkWidget *widget,
					 gpointer data );
static void	   nova_center_preview_init ( NovaCenter *center );
static void	   nova_center_draw ( NovaCenter *center, gint update );
static void	   nova_center_entry_update ( GtkWidget *widget,
					      gpointer data );
static void	   nova_center_cursor_update ( NovaCenter *center );
static gint	   nova_center_preview_expose ( GtkWidget *widget,
						GdkEvent *event );
static gint	   nova_center_preview_events ( GtkWidget *widget,
						GdkEvent *event );

static void	nova_int_entryscale_new ( GtkTable *table, gint x, gint y,
					 gchar *caption, gint *intvar,
					 gint min, gint max, gint constraint);

static void	nova_close_callback	    (GtkWidget *widget,
					     gpointer	data);
static void	nova_ok_callback	    (GtkWidget *widget,
					     gpointer	data);

static void	nova_paired_entry_destroy_callback (GtkWidget *widget,
						    gpointer data);

static void	nova_paired_int_scale_update (GtkAdjustment *adjustment,
					      gpointer	 data);
static void	nova_paired_int_entry_update (GtkWidget *widget,
					     gpointer data );

static void	nova	(GDrawable *drawable);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,	   /* init_proc */
  NULL,	   /* quit_proc */
  query,   /* query_proc */
  run,	   /* run_proc */
};

static NovaValues pvals =
{
  128, 128,		/* xcenter, ycenter */
  { 90, 100, 256 },	/* color */
  20,			/* radius */
  100			/* nspoke */
};

static NovaInterface pint =
{
  FALSE	    /* run */
};


MAIN ()

static void
query()
{
  static GParamDef args[]=
    {
      { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
      { PARAM_IMAGE, "image", "Input image (unused)" },
      { PARAM_DRAWABLE, "drawable", "Input drawable" },
      { PARAM_INT32, "xcenter", "X coordinates of the center of supernova" },
      { PARAM_INT32, "ycenter", "Y coordinates of the center of supernova" },
      { PARAM_COLOR, "color",	"Color of supernova" },
      { PARAM_INT32, "radius",	"Radius of supernova" },
      { PARAM_INT32, "nspoke",	"Number of spokes" }
   };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;
  gchar *help_string =
    " This plug-in produces an effect like a supernova burst. The"
    " amount of the light effect is approximately in proportion to 1/r,"
    " where r is the distance from the center of the star. It works with"
    " RGB*, GRAY* image.";

  gimp_install_procedure ("plug_in_nova",
			  "Produce Supernova effect to the specified drawable",
			  help_string,
			  "Eiichi Takamori",
			  "Eiichi Takamori",
			  "1997",
			  "<Image>/Filters/Light Effects/SuperNova",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (gchar   *name,
     gint    nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam  **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

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
      if (! nova_dialog ( drawable ))
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 8)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  pvals.xcenter = param[3].data.d_int32;
	  pvals.ycenter = param[4].data.d_int32;
	  pvals.color[0] = param[5].data.d_color.red;
	  pvals.color[1] = param[5].data.d_color.green;
	  pvals.color[2] = param[5].data.d_color.blue;
	  pvals.radius = param[6].data.d_int32;
	  pvals.nspoke = param[7].data.d_int32;
	}
      if ((status == STATUS_SUCCESS) &&
	   pvals.radius <= 0 )
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
      if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id))
	{
	  gimp_progress_init ("Rendering...");
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
/*		   */
/*   Main Dialog   */
/*		   */
/*******************/

static gint
nova_dialog ( GDrawable *drawable )
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *center_frame;
  guchar *color_cube;
  gchar **argv;
  gint	argc;

  argc = 1;
  argv = g_new (gchar *, 1);
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
  printf("Waiting... (pid %d)\n", getpid());
  kill(getpid(), 19); /* SIGSTOP */
#endif

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "SuperNova");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) nova_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) nova_ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ("Parameter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (6, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  center_frame = nova_center_create ( drawable );
  gtk_table_attach( GTK_TABLE(table), center_frame, 0, 2, 0, 1,
		    0, 0, 0, 0 );
  nova_int_entryscale_new( GTK_TABLE (table), 0, 1,
			  "R value:", &pvals.color[0],
			  0, 255, TRUE );
  nova_int_entryscale_new( GTK_TABLE (table), 0, 2,
			  "G value:", &pvals.color[1],
			  0, 255, TRUE );
  nova_int_entryscale_new( GTK_TABLE (table), 0, 3,
			  "B value:", &pvals.color[2],
			  0, 255, TRUE );
  nova_int_entryscale_new( GTK_TABLE (table), 0, 4,
			  "Radius:", &pvals.radius,
			  1, 100, FALSE );
  nova_int_entryscale_new( GTK_TABLE (table), 0, 5,
			  "Spokes:", &pvals.nspoke,
			  1, 1024, TRUE );

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return pint.run;
}

/***  Dialog interface ***/

static void
nova_close_callback (GtkWidget *widget,
			 gpointer   data)
{
  gtk_main_quit ();
}

static void
nova_ok_callback (GtkWidget *widget,
		      gpointer	 data)
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
nova_center_create ( GDrawable *drawable )
{
  NovaCenter	 *center;
  GtkWidget	 *frame;
  GtkWidget	 *table;
  GtkWidget	 *label;
  GtkWidget	 *entry;
  GtkWidget	 *pframe;
  GtkWidget	 *preview;
  gchar		 buf[256];

  center = g_new( NovaCenter, 1 );
  center->drawable = drawable;
  center->dwidth   = gimp_drawable_width(drawable->id );
  center->dheight  = gimp_drawable_height(drawable->id );
  center->bpp	   = gimp_drawable_bpp(drawable->id);
  if ( gimp_drawable_has_alpha(drawable->id) )
    center->bpp--;
  center->cursor = FALSE;
  center->curx = 0;
  center->cury = 0;
  center->oldx = 0;
  center->oldy = 0;
  center->in_call = TRUE;  /* to avoid side effects while initialization */

  frame = gtk_frame_new ( "Center of SuperNova" );
  gtk_signal_connect( GTK_OBJECT( frame ), "destroy",
		      (GtkSignalFunc) nova_center_destroy,
		      center );
  gtk_frame_set_shadow_type( GTK_FRAME( frame ) ,GTK_SHADOW_ETCHED_IN );
  gtk_container_border_width( GTK_CONTAINER( frame ), 10 );

  table = gtk_table_new ( 2, 4, FALSE );
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);

  label = gtk_label_new ( "X: " );
  gtk_misc_set_alignment( GTK_MISC(label), 0.0, 0.5 );
  gtk_table_attach( GTK_TABLE(table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0 );
  gtk_widget_show(label);

  center->xentry = entry = gtk_entry_new ();
  gtk_object_set_user_data( GTK_OBJECT(entry), center );
  gtk_signal_connect( GTK_OBJECT(entry), "changed",
		      (GtkSignalFunc) nova_center_entry_update,
		      &pvals.xcenter );
  gtk_widget_set_usize( GTK_WIDGET(entry), ENTRY_WIDTH,0 );
  gtk_table_attach( GTK_TABLE(table), entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0 );
  gtk_widget_show(entry);

  label = gtk_label_new ( "Y: " );
  gtk_misc_set_alignment( GTK_MISC(label), 0.0, 0.5 );
  gtk_table_attach( GTK_TABLE(table), label, 2, 3, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0 );
  gtk_widget_show(label);

  center->yentry = entry = gtk_entry_new ();
  gtk_object_set_user_data( GTK_OBJECT(entry), center );
  gtk_signal_connect( GTK_OBJECT(entry), "changed",
		      (GtkSignalFunc) nova_center_entry_update,
		      &pvals.ycenter );
  gtk_widget_set_usize( GTK_WIDGET(entry), ENTRY_WIDTH, 0 );
  gtk_table_attach( GTK_TABLE(table), entry, 3, 4, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0 );
  gtk_widget_show(entry);

  /* frame (shadow_in) that contains preview */
  pframe = gtk_frame_new ( NULL );
  gtk_frame_set_shadow_type( GTK_FRAME( pframe ), GTK_SHADOW_IN );
  gtk_table_attach( GTK_TABLE(table), pframe, 0, 4, 1, 2, 0, 0, 0, 0 );

  /* PREVIEW */
  center->preview = preview = gtk_preview_new( center->bpp==3 ? GTK_PREVIEW_COLOR : GTK_PREVIEW_GRAYSCALE );
  gtk_object_set_user_data( GTK_OBJECT(preview), center );
  gtk_widget_set_events( GTK_WIDGET(preview), PREVIEW_MASK );
  gtk_signal_connect_after( GTK_OBJECT(preview), "expose_event",
		      (GtkSignalFunc) nova_center_preview_expose,
		      center );
  gtk_signal_connect( GTK_OBJECT(preview), "event",
		      (GtkSignalFunc) nova_center_preview_events,
		      center );
  gtk_container_add( GTK_CONTAINER( pframe ), center->preview );

  /*
   * Resize the greater one of dwidth and dheight to PREVIEW_SIZE
   */
  if ( center->dwidth > center->dheight ) {
    center->pheight = center->dheight * PREVIEW_SIZE / center->dwidth;
    center->pwidth = PREVIEW_SIZE;
  } else {
    center->pwidth = center->dwidth * PREVIEW_SIZE / center->dheight;
    center->pheight = PREVIEW_SIZE;
  }
  gtk_preview_size( GTK_PREVIEW( preview ), center->pwidth, center->pheight );

  /* Draw the contents of preview, that is saved in the preview widget */
  nova_center_preview_init( center );
  gtk_widget_show(preview);

  gtk_widget_show( pframe );
  gtk_widget_show( table );
  gtk_widget_show( frame );

  sprintf( buf, "%d", pvals.xcenter );
  gtk_entry_set_text( GTK_ENTRY(center->xentry), buf );
  sprintf( buf, "%d", pvals.ycenter );
  gtk_entry_set_text( GTK_ENTRY(center->yentry), buf );

  nova_center_cursor_update( center );

  center->cursor = FALSE;    /* Make sure that the cursor has not been drawn */
  center->in_call = FALSE;   /* End of initialization */
  DEBUG1("pvals center=%d,%d\n", pvals.xcenter, pvals.ycenter );
  DEBUG1("center cur=%d,%d\n", center->curx, center->cury );
  return frame;
}

static void
nova_center_destroy ( GtkWidget *widget,
		      gpointer data )
{
  NovaCenter *center = data;
  g_free( center );
}

static void render_preview ( GtkWidget *preview, GPixelRgn *srcrgn );

/*
 *  Initialize preview
 *  Draw the contents into the internal buffer of the preview widget
 */
static void
nova_center_preview_init ( NovaCenter *center )
{
  GtkWidget	 *preview;
  GPixelRgn	 src_rgn;
  gint		 dwidth, dheight, pwidth, pheight, bpp;

  preview = center->preview;
  dwidth = center->dwidth;
  dheight = center->dheight;
  pwidth = center->pwidth;
  pheight = center->pheight;
  bpp = center->bpp;

  gimp_pixel_rgn_init ( &src_rgn, center->drawable, 0, 0,
			center->dwidth, center->dheight, FALSE, FALSE );
  render_preview( center->preview, &src_rgn );
}


/*======================================================================
		Preview Rendering Util routine
=======================================================================*/

#define CHECKWIDTH 4
#define LIGHTCHECK 192
#define DARKCHECK  128
#ifndef OPAQUE
#define OPAQUE	   255
#endif

static void
render_preview ( GtkWidget *preview, GPixelRgn *srcrgn )
{
  guchar	 *src_row, *dest_row, *src, *dest;
  gint		 row, col;
  gint		 dwidth, dheight, pwidth, pheight;
  gint		 *src_col;
  gint		 bpp, alpha, has_alpha, b;
  guchar	 check;

  dwidth  = srcrgn->w;
  dheight = srcrgn->h;
  if( GTK_PREVIEW(preview)->buffer )
    {
      pwidth  = GTK_PREVIEW(preview)->buffer_width;
      pheight = GTK_PREVIEW(preview)->buffer_height;
    }
  else
    {
      pwidth  = preview->requisition.width;
      pheight = preview->requisition.height;
    }

  bpp = srcrgn->bpp;
  alpha = bpp;
  has_alpha = gimp_drawable_has_alpha( srcrgn->drawable->id );
  if( has_alpha ) alpha--;
  /*  printf("render_preview: %d %d %d", bpp, alpha, has_alpha);
      printf(" (%d %d %d %d)\n", dwidth, dheight, pwidth, pheight); */

  src_row = g_new ( guchar, dwidth * bpp );
  dest_row = g_new ( guchar, pwidth * bpp );
  src_col = g_new ( gint, pwidth );

  for ( col = 0; col < pwidth; col++ )
    src_col[ col ] = ( col * dwidth / pwidth ) * bpp;

  for ( row = 0; row < pheight; row++ )
    {
      gimp_pixel_rgn_get_row ( srcrgn, src_row,
			       0, row * dheight / pheight, dwidth );
      dest = dest_row;
      for ( col = 0; col < pwidth; col++ )
	{
	  src = &src_row[ src_col[col] ];
	  if( !has_alpha || src[alpha] == OPAQUE )
	    {
	      /* no alpha channel or opaque -- simple way */
	      for ( b = 0; b < alpha; b++ )
		dest[b] = src[b];
	    }
	  else
	    {
	      /* more or less transparent */
	      if( ( col % (CHECKWIDTH*2) < CHECKWIDTH ) ^
		  ( row % (CHECKWIDTH*2) < CHECKWIDTH ) )
		check = LIGHTCHECK;
	      else
		check = DARKCHECK;

	      if ( src[alpha] == 0 )
		{
		  /* full transparent -- check */
		  for ( b = 0; b < alpha; b++ )
		    dest[b] = check;
		}
	      else
		{
		  /* middlemost transparent -- mix check and src */
		  for ( b = 0; b < alpha; b++ )
		    dest[b] = ( src[b]*src[alpha] + check*(OPAQUE-src[alpha]) ) / OPAQUE;
		}
	    }
	  dest += alpha;
	}
      gtk_preview_draw_row( GTK_PREVIEW( preview ), dest_row,
			    0, row, pwidth );
    }

  g_free ( src_col );
  g_free ( src_row );
  g_free ( dest_row );
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
nova_center_draw ( NovaCenter *center, gint update )
{
  if( update & PREVIEW )
    {
      center->cursor = FALSE;
      DEBUG1("draw-preview\n");
    }

  if( update & CURSOR )
    {
      DEBUG1("draw-cursor %d old=%d,%d cur=%d,%d\n",
	     center->cursor, center->oldx, center->oldy, center->curx, center->cury);
      gdk_gc_set_function ( center->preview->style->black_gc, GDK_INVERT);
      if( center->cursor )
	{
	  gdk_draw_line ( center->preview->window,
			  center->preview->style->black_gc,
			  center->oldx, 1, center->oldx, center->pheight-1 );
	  gdk_draw_line ( center->preview->window,
			  center->preview->style->black_gc,
			  1, center->oldy, center->pwidth-1, center->oldy );
	}
      gdk_draw_line ( center->preview->window,
		      center->preview->style->black_gc,
		      center->curx, 1, center->curx, center->pheight-1 );
      gdk_draw_line ( center->preview->window,
		      center->preview->style->black_gc,
		      1, center->cury, center->pwidth-1, center->cury );
      /* current position of cursor is updated */
      center->oldx = center->curx;
      center->oldy = center->cury;
      center->cursor = TRUE;
      gdk_gc_set_function ( center->preview->style->black_gc, GDK_COPY);
    }
}


/*
 *  CenterFrame entry callback
 */

static void
nova_center_entry_update ( GtkWidget *widget,
			     gpointer data )
{
  NovaCenter *center;
  gint *val, new_val;

  DEBUG1("entry\n");
  val = data;
  new_val = atoi ( gtk_entry_get_text( GTK_ENTRY(widget) ) );

  if( *val != new_val )
    {
      *val = new_val;
      center = gtk_object_get_user_data( GTK_OBJECT(widget) );
      DEBUG1("entry:newval in_call=%d\n", center->in_call );
      if( !center->in_call )
	{
	  nova_center_cursor_update( center );
	  nova_center_draw ( center, CURSOR );
	}
    }
}

/*
 *  Update the cross cursor's  coordinates accoding to pvals.[xy]center
 *  but not redraw it
 */

static void
nova_center_cursor_update ( NovaCenter *center )
{
  center->curx = pvals.xcenter * center->pwidth / center->dwidth;
  center->cury = pvals.ycenter * center->pheight / center->dheight;

  if( center->curx < 0 )		     center->curx = 0;
  else if( center->curx >= center->pwidth )  center->curx = center->pwidth-1;
  if( center->cury < 0 )		     center->cury = 0;
  else if( center->cury >= center->pheight)  center->cury = center->pheight-1;

}

/*
 *    Handle the expose event on the preview
 */
static gint
nova_center_preview_expose( GtkWidget *widget,
			    GdkEvent *event )
{
  NovaCenter *center;

  center = gtk_object_get_user_data( GTK_OBJECT(widget) );
  nova_center_draw( center, ALL );
  return FALSE;
}


/*
 *    Handle other events on the preview
 */

static gint
nova_center_preview_events ( GtkWidget *widget,
			     GdkEvent *event )
{
  NovaCenter *center;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gchar buf[256];

  center = gtk_object_get_user_data ( GTK_OBJECT(widget) );

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
      if ( !mevent->state ) break;
      center->curx = mevent->x;
      center->cury = mevent->y;
    mouse:
      nova_center_draw( center, CURSOR );
      center->in_call = TRUE;
      sprintf(buf, "%d", center->curx * center->dwidth / center->pwidth );
      gtk_entry_set_text( GTK_ENTRY(center->xentry), buf );
      sprintf(buf, "%d", center->cury * center->dheight / center->pheight );
      gtk_entry_set_text( GTK_ENTRY(center->yentry), buf );
      center->in_call = FALSE;
      break;

    default:
      break;
    }

  return FALSE;
}

/*===================================================================

	 Entry - Scale Pair

====================================================================*/


/***********************************************************************/
/*								       */
/*    Create new entry-scale pair with label. (int)		       */
/*    1 row and 2 cols of table are needed.			       */
/*								       */
/*    `x' and `y' means starting row and col in `table'.	       */
/*								       */
/*    `caption' is label string.				       */
/*								       */
/*    `min', `max' are boundary of scale.			       */
/*								       */
/*    `constraint' means whether value of *intvar should be constraint */
/*    by scale adjustment, e.g. between `min' and `max'.	       */
/*								       */
/***********************************************************************/

static void
nova_int_entryscale_new ( GtkTable *table, gint x, gint y,
			 gchar *caption, gint *intvar,
			 gint min, gint max, gint constraint)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *scale;
  GtkObject *adjustment;
  NovaEntryScaleData *userdata;
  gchar	   buffer[256];


  label = gtk_label_new (caption);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  adjustment = gtk_adjustment_new ( *intvar, min, max, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new ( GTK_ADJUSTMENT(adjustment) );
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf( buffer, "%d", *intvar );
  gtk_entry_set_text( GTK_ENTRY (entry), buffer );


  userdata = g_new ( NovaEntryScaleData, 1 );
  userdata->entry = entry;
  userdata->adjustment = adjustment;
  userdata->constraint = constraint;
  gtk_object_set_user_data (GTK_OBJECT(entry), userdata);
  gtk_object_set_user_data (GTK_OBJECT(adjustment), userdata);

  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) nova_paired_int_entry_update,
		      intvar);
  gtk_signal_connect ( adjustment, "value_changed",
		      (GtkSignalFunc) nova_paired_int_scale_update,
		      intvar);
  gtk_signal_connect ( GTK_OBJECT( entry ), "destroy",
		      (GtkSignalFunc) nova_paired_entry_destroy_callback,
		      userdata );


  hbox = gtk_hbox_new ( FALSE, 5 );
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), label, x, x+1, y, y+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_widget_show (label);
  gtk_widget_show (entry);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);
}


/*
   when destroyed, userdata is destroyed too
*/
static void
nova_paired_entry_destroy_callback (GtkWidget *widget,
				    gpointer data)
{
  NovaEntryScaleData *userdata;
  userdata = data;
  g_free ( userdata );
}

/* scale callback (int) */
/* ==================== */

static void
nova_paired_int_scale_update (GtkAdjustment *adjustment,
			      gpointer	    data)
{
  NovaEntryScaleData *userdata;
  GtkEntry *entry;
  gchar buffer[256];
  gint *val, new_val;

  val = data;
  new_val = (gint) adjustment->value;

  *val = new_val;

  userdata = gtk_object_get_user_data (GTK_OBJECT (adjustment));
  entry = GTK_ENTRY( userdata->entry );
  sprintf (buffer, "%d", (int) new_val );

  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data ( GTK_OBJECT(entry), data );
  gtk_entry_set_text ( entry, buffer);
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(entry), data );
}

/*
   entry callback (int)
*/

static void
nova_paired_int_entry_update (GtkWidget *widget,
			      gpointer	 data)
{
  NovaEntryScaleData *userdata;
  GtkAdjustment *adjustment;
  int new_val, constraint_val;
  int *val;

  val = data;
  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  *val = new_val;

  userdata = gtk_object_get_user_data (GTK_OBJECT (widget));
  adjustment = GTK_ADJUSTMENT( userdata->adjustment );

  constraint_val = new_val;
  if ( constraint_val < adjustment->lower )
    constraint_val = adjustment->lower;
  if ( constraint_val > adjustment->upper )
    constraint_val = adjustment->upper;

  if ( userdata->constraint )
    *val = constraint_val;
  else
    *val = new_val;

  adjustment->value = constraint_val;
  gtk_signal_handler_block_by_data ( GTK_OBJECT(adjustment), data );
  gtk_signal_emit_by_name ( GTK_OBJECT(adjustment), "value_changed");
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(adjustment), data );

}

/*
################################################################
##							      ##
##		     Main Calculation			      ##
##							      ##
################################################################
*/


static double gauss()
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
  gint	  i;
  gint	  color[4];

  /* initialize */

  new_alpha = 0.0;

  srand(time(NULL));
  spoke = g_new( gdouble, pvals.nspoke );
  for( i=0; i<pvals.nspoke; i++ )
    spoke[i] = gauss();

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

      for ( row = 0, y = src_rgn.y; row < src_rgn.h; row++, y++)
	{
	  src = src_row;
	  dest = dest_row;

	  for ( col = 0, x = src_rgn.x; col < src_rgn.w; col++, x++)
	    {
	      u = (gdouble) (x-xc) / pvals.radius;
	      v = (gdouble) (y-yc) / pvals.radius;
	      l = sqrt( u*u + v*v );

	      /* This algorithm is still under construction. */
	      c = (atan2 (u, v) / (2 * M_PI) + .51) * pvals.nspoke;
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

	      for (i = 0; i < alpha; i++)
		{
		  if (w > 1.0)
		    color[i] = CLAMP (pvals.color[i] * w, 0, 255);
		  else
		    color[i] = src[i] * compl_ratio + pvals.color[i] * ratio;

		  c = CLAMP (w1 * w, 0, 1);
		  color[i] = color[i] + 255 * c;

		  dest[i]= CLAMP (color[i], 0, 255);
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

  g_free( spoke );
}

