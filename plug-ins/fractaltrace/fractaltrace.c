/*******************************************************************************

  fractaltrace.c  -- This is a plug-in for the GIMP 1.0

  Copyright (C) 1997  Hirotsuna Mizuno
                      s1041150@u-aizu.ac.jp

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*******************************************************************************/

#define PLUG_IN_NAME     "plug_in_fractal_trace"
#define PLUG_IN_TITLE    "Fractal Trace"
#define PLUG_IN_VERSION  "v0.4 test version (Dec. 25 1997)"
#define PLUG_IN_CATEGORY "<Image>/Filters/Map/Fractal Trace"

/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <math.h>
#include <libgimp/gimp.h>

#ifndef PI_2
#define PI_2 (3.14159265358979323*2.0)
#endif

/******************************************************************************/

static void query( void );
static void run( char*, int, GParam*, int*, GParam** );
static void filter( GDrawable* );

static void pixels_init( GDrawable* );
static void pixels_free( void );

static int  dialog_show( void );
static void dialog_preview_draw( void );

/******************************************************************************/

GPlugInInfo PLUG_IN_INFO = {
  NULL,  /*  init_proc */
  NULL,  /*  quit_proc */
  query, /* query_proc */
  run    /*   run_proc */
};

MAIN()

/******************************************************************************/

typedef struct {
  gint32 wrap;
  gint32 transparent;
  gint32 black;
  gint32 white;
} outside_type_t;

#define OUTSIDE_TYPE_WRAP        0
#define OUTSIDE_TYPE_TRANSPARENT 1
#define OUTSIDE_TYPE_BLACK       2
#define OUTSIDE_TYPE_WHITE       3

static outside_type_t outside_type = {
  OUTSIDE_TYPE_WRAP,
  OUTSIDE_TYPE_TRANSPARENT,
  OUTSIDE_TYPE_BLACK,
  OUTSIDE_TYPE_WHITE
};

typedef struct {
  gdouble x1;
  gdouble x2;
  gdouble y1;
  gdouble y2;
  gint32  depth;
  gint32  outside_type;
} parameter_t;

static parameter_t parameters = {
  -1.0,
  +0.5,
  -1.0,
  +1.0,
  3,
  OUTSIDE_TYPE_WRAP
};

/******************************************************************************/

static void query( void )
{
  static GParamDef args[] = {
    { PARAM_INT32,    "run_mode",     "interactive / non-interactive"    },
    { PARAM_IMAGE,    "image",        "input image (not used)"           },
    { PARAM_DRAWABLE, "drawable",     "input drawable"                   },
    { PARAM_FLOAT,    "xmin",         "xmin fractal image delimiter"     },
    { PARAM_FLOAT,    "xmax",         "xmax fractal image delimiter"     },
    { PARAM_FLOAT,    "ymin",         "ymin fractal image delimiter"     },
    { PARAM_FLOAT,    "ymax",         "ymax fractal image delimiter"     },
    { PARAM_INT32,    "depth",        "trace depth"                      },
    { PARAM_INT32,    "outside_type", "outside type"
                                      "(0=WRAP/1=TRANS/2=BLACK/3=WHITE)" },
  };
  static int nargs = sizeof( args ) / sizeof( args[0] );

  static GParamDef *rets = NULL;
  static int nrets = 0;
  
  gimp_install_procedure(
			 PLUG_IN_NAME,
			 "transform image with the Mandelbrot Fractal",
			 "transform image with the Mandelbrot Fractal",
			 "Hirotsuna Mizuno <s1041150@u-aizu.ac.jp>",
			 "Copyright (C) 1997 Hirotsuna Mizuno",
			 PLUG_IN_VERSION,
			 PLUG_IN_CATEGORY,
			 "RGB*, GRAY*",
			 PROC_PLUG_IN,
			 nargs,
			 nrets,
			 args,
			 rets
			 );
}

/******************************************************************************/

typedef struct {
  gint     x1;
  gint     x2;
  gint     y1;
  gint     y2;
  gint     width;
  gint     height;
  gdouble  center_x;
  gdouble  center_y;
} selection_t;

typedef struct {
  gint         width;
  gint         height;
  gint         bpp;
  gint         alpha;
} image_t;

static selection_t selection;
static image_t     image;
  
/******************************************************************************/

static void run( char *name, int argc, GParam *args, int *retc, GParam **rets )
{
  GDrawable     *drawable;
  GRunModeType   run_mode;
  GStatusType    status;
  static GParam  returns[1];

  run_mode = args[0].data.d_int32;
  status   = STATUS_SUCCESS;

  drawable     = gimp_drawable_get( args[2].data.d_drawable );
  image.width  = gimp_drawable_width( drawable->id );
  image.height = gimp_drawable_height( drawable->id );
  image.bpp    = gimp_drawable_bpp( drawable->id );
  image.alpha  = gimp_drawable_has_alpha( drawable->id );
  gimp_drawable_mask_bounds( drawable->id, &selection.x1, &selection.y1,
			     &selection.x2, &selection.y2 );
  selection.width    = selection.x2 - selection.y1;
  selection.height   = selection.y2 - selection.y1;
  selection.center_x = selection.x1 + (gdouble)selection.width / 2.0;
  selection.center_y = selection.y1 + (gdouble)selection.height / 2.0;

  pixels_init( drawable );

  if( !gimp_drawable_color( drawable->id ) && 
      !gimp_drawable_gray( drawable->id ) ){
    status = STATUS_EXECUTION_ERROR;
  }

  switch( run_mode ){
  case RUN_WITH_LAST_VALS:
    gimp_get_data( PLUG_IN_NAME, &parameters );
    break;
  case RUN_INTERACTIVE:
    gimp_get_data( PLUG_IN_NAME, &parameters );
    if( !dialog_show() ){
      status = STATUS_EXECUTION_ERROR;
      break;
    }
    gimp_set_data( PLUG_IN_NAME, &parameters, sizeof( parameter_t ) );
    break;
  case RUN_NONINTERACTIVE:
    if( argc != 9 ){
      status = STATUS_CALLING_ERROR;
    } else {
      parameters.x1           = args[3].data.d_float;
      parameters.x2           = args[4].data.d_float;
      parameters.y1           = args[5].data.d_float;
      parameters.y2           = args[6].data.d_float;
      parameters.depth        = args[7].data.d_int32;
      parameters.outside_type = args[8].data.d_int32;
    }
    break;
  }

  if( status == STATUS_SUCCESS ){
    gimp_tile_cache_ntiles( 2 * ( drawable->width / gimp_tile_width() + 1 ) );
    filter( drawable );
    if( run_mode != RUN_NONINTERACTIVE ) gimp_displays_flush();
  }

  gimp_drawable_detach( drawable );

  pixels_free();

  returns[0].type          = PARAM_STATUS;
  returns[0].data.d_status = status;
  *retc = 1;
  *rets = returns;
}

/******************************************************************************/

static guchar    **spixels;
static guchar    **dpixels;
static GPixelRgn   sPR;
static GPixelRgn   dPR;

typedef struct {
  guchar r;
  guchar g;
  guchar b;
  guchar a;
} pixel_t;

static void pixels_init( GDrawable *drawable )
{
  gint y;
  gimp_pixel_rgn_init( &sPR, drawable,
		       0, 0, image.width, image.height, FALSE, FALSE );
  gimp_pixel_rgn_init( &dPR, drawable,
		       0, 0, image.width, image.height, TRUE, TRUE );
  spixels = (guchar**)g_malloc( image.height * sizeof( guchar* ) );
  dpixels = (guchar**)g_malloc( image.height * sizeof( guchar* ) );
  for( y = 0; y < image.height; y++ ){
    spixels[y] = (guchar*)g_malloc( image.width * image.bpp * sizeof( guchar ) );
    dpixels[y] = (guchar*)g_malloc( image.width * image.bpp * sizeof( guchar ) );
    gimp_pixel_rgn_get_row( &sPR, spixels[y], 0, y, image.width );
  }
}

static void pixels_free( void )
{
  gint y;
  for( y = 0; y < image.height; y++ ){
    free( spixels[y] );
    free( dpixels[y] );
  }
  free( spixels );
  free( dpixels );
}

static void pixels_get( gint x, gint y, pixel_t *pixel )
{
  if( x < 0 ) x = 0; else if( image.width  <= x ) x = image.width  - 1;
  if( y < 0 ) y = 0; else if( image.height <= y ) y = image.height - 1;

  switch( image.bpp ){
  case 1: /* GRAY */
    pixel->r = spixels[y][x*image.bpp];
    pixel->g = spixels[y][x*image.bpp];
    pixel->b = spixels[y][x*image.bpp];
    pixel->a = 255;
    break;
  case 2: /* GRAY+A */
    pixel->r = spixels[y][x*image.bpp];
    pixel->g = spixels[y][x*image.bpp];
    pixel->b = spixels[y][x*image.bpp];
    pixel->a = spixels[y][x*image.bpp+1];
    break;
  case 3: /* RGB */
    pixel->r = spixels[y][x*image.bpp];
    pixel->g = spixels[y][x*image.bpp+1];
    pixel->b = spixels[y][x*image.bpp+2];
    pixel->a = 255;
    break;
  case 4: /* RGB+A */
    pixel->r = spixels[y][x*image.bpp];
    pixel->g = spixels[y][x*image.bpp+1];
    pixel->b = spixels[y][x*image.bpp+2];
    pixel->a = spixels[y][x*image.bpp+3];
    break;
  }
}

static void pixels_get_biliner( gdouble x, gdouble y, pixel_t *pixel )
{
  pixel_t A, B, C, D;
  gdouble a, b, c, d;
  gint    x1, y1, x2, y2;
  gdouble dx, dy;

  x1 = (gint)floor( x );
  x2 = x1 + 1;
  y1 = (gint)floor( y );
  y2 = y1 + 1;

  dx = x - (gdouble)x1;
  dy = y - (gdouble)y1;
  a  = ( 1.0 - dx ) * ( 1.0 - dy );
  b  = dx * ( 1.0 - dy );
  c  = ( 1.0 - dx ) * dy;
  d  = dx * dy;
  
  pixels_get( x1, y1, &A );
  pixels_get( x2, y1, &B );
  pixels_get( x1, y2, &C );
  pixels_get( x2, y2, &D );

  pixel->r = (guchar)( a * (gdouble)A.r + b * (gdouble)B.r +
		       c * (gdouble)C.r + d * (gdouble)D.r );
  pixel->g = (guchar)( a * (gdouble)A.g + b * (gdouble)B.g +
		       c * (gdouble)C.g + d * (gdouble)D.g );
  pixel->b = (guchar)( a * (gdouble)A.b + b * (gdouble)B.b +
		       c * (gdouble)C.b + d * (gdouble)D.b );
  pixel->a = (guchar)( a * (gdouble)A.a + b * (gdouble)B.a +
		       c * (gdouble)C.a + d * (gdouble)D.a );
}

static void pixels_set( gint x, gint y, pixel_t *pixel )
{
  switch( image.bpp ){
  case 1: /* GRAY */
    dpixels[y][x*image.bpp]   = pixel->r;
    break;
  case 2: /* GRAY+A */
    dpixels[y][x*image.bpp]   = pixel->r;
    dpixels[y][x*image.bpp+1] = pixel->a;
    break;
  case 3: /* RGB */
    dpixels[y][x*image.bpp]   = pixel->r;
    dpixels[y][x*image.bpp+1] = pixel->g;
    dpixels[y][x*image.bpp+2] = pixel->b;
    break;
  case 4: /* RGB+A */
    dpixels[y][x*image.bpp]   = pixel->r;
    dpixels[y][x*image.bpp+1] = pixel->g;
    dpixels[y][x*image.bpp+2] = pixel->b;
    dpixels[y][x*image.bpp+3] = pixel->a;
    break;
  }
}

static void pixels_store( void )
{
  gint y;
  for( y = selection.y1; y < selection.y2; y++ ){
    gimp_pixel_rgn_set_row( &dPR, dpixels[y], 0, y, image.width );
  }
}

/******************************************************************************/

static void mandelbrot( gdouble x, gdouble y, gdouble *u, gdouble *v )
{
  gint    iter = 0;
  gdouble xx   = x;
  gdouble yy   = y;
  gdouble x2   = xx * xx;
  gdouble y2   = yy * yy;
  gdouble tmp;
  while( iter < parameters.depth ){
    tmp = x2 - y2 + x;
    yy  = 2 * xx * yy + y;
    xx  = tmp;
    x2  = xx * xx;
    y2  = yy * yy;
    iter++;
  }
  *u = xx;
  *v = yy;
}


/******************************************************************************/

static void filter( GDrawable *drawable )
{
  gint    x, y;
  pixel_t pixel;
  gdouble scale_x, scale_y;
  gdouble cx, cy;
  gdouble px, py;

  gimp_progress_init( PLUG_IN_TITLE );

  scale_x = ( parameters.x2 - parameters.x1 ) / selection.width;
  scale_y = ( parameters.y2 - parameters.y1 ) / selection.height;

  for( y = selection.y1; y < selection.y2; y++ ){
    cy = parameters.y1 + ( y - selection.y1 ) * scale_y;
    for( x = selection.x1; x < selection.x2; x++ ){
      cx = parameters.x1 + ( x - selection.x1 ) * scale_x;
      mandelbrot( cx, cy, &px, &py );
      px = ( px - parameters.x1 ) / scale_x + selection.x1;
      py = ( py - parameters.y1 ) / scale_y + selection.y1;
      if( 0 <= px && px < image.width && 0 <= py && py < image.height ){
	pixels_get_biliner( px, py, &pixel );
      } else {
	switch( parameters.outside_type ){
	case OUTSIDE_TYPE_WRAP:
	  px = fmod( px, image.width );
	  py = fmod( py, image.height );
	  if( px < 0.0 ) px += image.width;
	  if( py < 0.0 ) py += image.height;
	  pixels_get_biliner( px, py, &pixel );
	  break;
	case OUTSIDE_TYPE_TRANSPARENT:
	  pixel.r = pixel.g = pixel.b = 0;
	  pixel.a = 0;
	  break;
	case OUTSIDE_TYPE_BLACK:
	  pixel.r = pixel.g = pixel.b = 0;
	  pixel.a = 255;
	  break;
	case OUTSIDE_TYPE_WHITE:
	  pixel.r = pixel.g = pixel.b = 255;
	  pixel.a = 255;
	  break;
	}
      }
      pixels_set( x, y, &pixel );
    }
    gimp_progress_update( (gdouble)(y-selection.y1)/selection.height );
  }

  pixels_store();

  gimp_drawable_flush( drawable );
  gimp_drawable_merge_shadow( drawable->id, TRUE );
  gimp_drawable_update( drawable->id,
			selection.x1, selection.y1, selection.width, selection.height );
}

/*
    GUI 
*/

/******************************************************************************/

static int dialog_status;

/******************************************************************************/

static void dialog_entry_gint32_callback( GtkWidget *widget, gpointer *data )
{
  gint32 value = (gint32)atof( gtk_entry_get_text( GTK_ENTRY( widget ) ) );
  if( *(gint32*)data != value ){
    *(gint32*)data = value;
    dialog_preview_draw();
  }
}

static void dialog_entry_gint32_new( char *caption, gint32 *value,
					    GtkWidget *table, gint row )
{
  GtkWidget *label;
  GtkWidget *entry;
  char       buffer[256];

  label = gtk_label_new( caption );
  gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1, row, row + 1 );
  gtk_widget_show( label );
  
  entry = gtk_entry_new();
  sprintf( buffer, "%d", *value );
  gtk_entry_set_text( GTK_ENTRY( entry ), buffer );
  gtk_signal_connect( GTK_OBJECT( entry ), "changed",
		      GTK_SIGNAL_FUNC( dialog_entry_gint32_callback ), value );
  gtk_table_attach_defaults( GTK_TABLE( table ), entry, 1, 2, row, row + 1 );
  gtk_widget_show( entry );
}

/******************************************************************************/

static void dialog_entry_gdouble_callback( GtkWidget *widget, gpointer *data )
{
  gdouble value = (gdouble)atof( gtk_entry_get_text( GTK_ENTRY( widget ) ) );
  if( *(gdouble*)data != value ){
    *(gdouble*)data = value;
    dialog_preview_draw();
  }
}

static void dialog_entry_gdouble_new( char *caption, gdouble *value,
					    GtkWidget *table, gint row )
{
  GtkWidget *label;
  GtkWidget *entry;
  char       buffer[256];

  label = gtk_label_new( caption );
  gtk_table_attach_defaults( GTK_TABLE( table ), label, 0, 1, row, row + 1 );
  gtk_widget_show( label );
  
  entry = gtk_entry_new();
  sprintf( buffer, "%f", *value );
  gtk_entry_set_text( GTK_ENTRY( entry ), buffer );
  gtk_signal_connect( GTK_OBJECT( entry ), "changed",
		      GTK_SIGNAL_FUNC( dialog_entry_gdouble_callback ), value );
  gtk_table_attach_defaults( GTK_TABLE( table ), entry, 1, 2, row, row + 1 );
  gtk_widget_show( entry );
}

/******************************************************************************/

static GtkWidget* dialog_entry_table( void )
{
  GtkWidget *table;
  
  table = gtk_table_new( 5, 2, FALSE );
  gtk_table_set_row_spacings( GTK_TABLE( table ), 2 );
  gtk_table_set_col_spacings( GTK_TABLE( table ), 10 );
  dialog_entry_gdouble_new( "X1",   &parameters.x1,    table, 0 );
  dialog_entry_gdouble_new( "X2",   &parameters.x2,    table, 1 );
  dialog_entry_gdouble_new( "Y1",   &parameters.y1,    table, 2 );
  dialog_entry_gdouble_new( "Y2",   &parameters.y2,    table, 3 );
  dialog_entry_gint32_new( "DEPTH", &parameters.depth, table, 4 );

  return table;
}

/******************************************************************************/

static void dialog_destroy_callback( GtkWidget *widget, gpointer data )
{
  gtk_main_quit();
  gdk_flush();
}

static void dialog_ok_callback( GtkWidget *widget, gpointer data )
{
  dialog_status = TRUE;
  gtk_widget_destroy( GTK_WIDGET( data ) );
}

static void dialog_cancel_callback( GtkWidget *widget, gpointer data )
{
  dialog_status = FALSE;
  gtk_widget_destroy( GTK_WIDGET( data ) );
}

static void dialog_help_callback( GtkWidget *widget, gpointer data )
{
}

/******************************************************************************/

static void dialog_outside_type_callback( GtkWidget *widget, gpointer *data )
{
  gint32 value = *(gint32*)data;

  if( parameters.outside_type != value ){
    parameters.outside_type = value;
    dialog_preview_draw();
  }
}

/******************************************************************************/

#define PREVIEW_SIZE 200

typedef struct {
  GtkWidget  *preview;
  guchar    **source;
  guchar    **pixels;
  gdouble     scale;
  gint        width;
  gint        height;
  gint        bpp;
} preview_t;

static preview_t preview;

static void dialog_preview_setpixel( gint x, gint y, pixel_t *pixel )
{
  switch( preview.bpp ){
  case 1:
    preview.pixels[y][x*preview.bpp] = pixel->r;
    break;
  case 3:
    preview.pixels[y][x*preview.bpp]   = pixel->r;
    preview.pixels[y][x*preview.bpp+1] = pixel->g;
    preview.pixels[y][x*preview.bpp+2] = pixel->b;
    break;
  }
}

static void dialog_preview_store( void )
{
  gint y;
  for( y = 0; y < preview.height; y++ ){
    gtk_preview_draw_row( GTK_PREVIEW( preview.preview ),
			  preview.pixels[y], 0, y, preview.width );
  }
  gtk_widget_draw( preview.preview, NULL );
  gdk_flush();
}

static void dialog_preview_init( void )
{
  {
    guchar *cube;
    gtk_preview_set_gamma( gimp_gamma() );
    gtk_preview_set_install_cmap( gimp_install_cmap() );
    cube = gimp_color_cube();
    gtk_preview_set_color_cube( cube[0], cube[1], cube[2], cube[3] );
    gtk_widget_set_default_visual( gtk_preview_get_visual() );
    gtk_widget_set_default_colormap( gtk_preview_get_cmap() );
  }

  if( image.width < image.height )
    preview.scale = (gdouble)selection.height / (gdouble)PREVIEW_SIZE;
  else
    preview.scale = (gdouble)selection.width / (gdouble)PREVIEW_SIZE;
  preview.width  = (gdouble)selection.width / preview.scale;
  preview.height = (gdouble)selection.height / preview.scale;

  if( image.bpp < 3 ){
    preview.bpp = 1;
    preview.preview = gtk_preview_new( GTK_PREVIEW_GRAYSCALE );
  } else {
    preview.bpp = 3;
    preview.preview = gtk_preview_new( GTK_PREVIEW_COLOR );
  }
  gtk_preview_size( GTK_PREVIEW( preview.preview ), preview.width, preview.height );
  
  {
    gint y;
    preview.source = (guchar**)g_malloc( preview.height * sizeof( guchar* ) );
    preview.pixels = (guchar**)g_malloc( preview.height * sizeof( guchar* ) );
    for( y = 0; y < preview.height; y++ ){
      preview.source[y] = (guchar*)g_malloc( preview.width * preview.bpp * sizeof( guchar ) );
      preview.pixels[y] = (guchar*)g_malloc( preview.width * preview.bpp * sizeof( guchar ) );
    }
  }
  
  {
    pixel_t pixel;
    gint    x, y;
    gdouble cx, cy;
    for( y = 0; y < preview.height; y++ ){
      cy = selection.y1 + (gdouble)y * preview.scale;
      for( x = 0; x < preview.width; x++ ){
	cx = selection.x1 + (gdouble)x * preview.scale;
	pixels_get_biliner( cx, cy, &pixel );
	dialog_preview_setpixel( x, y, &pixel );
      }
    }
    dialog_preview_store();
  }
}

static void dialog_preview_draw( void )
{
  gint    x, y;
  pixel_t pixel;
  gdouble scale_x, scale_y;
  gdouble cx, cy;
  gdouble px, py;

  scale_x = ( parameters.x2 - parameters.x1 ) / preview.width;
  scale_y = ( parameters.y2 - parameters.y1 ) / preview.height;

  for( y = 0; y < preview.height; y++ ){
    cy = parameters.y1 + y * scale_y;
    for( x = 0; x < preview.width; x++ ){
      cx = parameters.x1 + x * scale_x;
      mandelbrot( cx, cy, &px, &py );
      px = ( px - parameters.x1 ) / scale_x * preview.scale + selection.x1;
      py = ( py - parameters.y1 ) / scale_y * preview.scale + selection.y1;
      if( 0 <= px && px < image.width && 0 <= py && py < image.height ){
	pixels_get_biliner( px, py, &pixel );
      } else {
	switch( parameters.outside_type ){
	case OUTSIDE_TYPE_WRAP:
	  px = fmod( px, image.width );
	  py = fmod( py, image.height );
	  if( px < 0.0 ) px += image.width;
	  if( py < 0.0 ) py += image.height;
	  pixels_get_biliner( px, py, &pixel );
	  break;
	case OUTSIDE_TYPE_TRANSPARENT:
	case OUTSIDE_TYPE_BLACK:
	  pixel.r = pixel.g = pixel.b = 0;
	  break;
	case OUTSIDE_TYPE_WHITE:
	  pixel.r = pixel.g = pixel.b = 255;
	  break;
	}
      }
      dialog_preview_setpixel( x, y, &pixel );
    }
  }

  dialog_preview_store();
}

/******************************************************************************/

static gint dialog_show( void )
{
  GtkWidget *dialog;
  GtkWidget *mainbox;
  
  dialog_status = FALSE;
  
  {
    gint    argc = 1;
    gchar **argv = g_new( gchar *, 1 );
    argv[0]      = g_strdup( PLUG_IN_TITLE );
    gtk_init( &argc, &argv );
    gtk_rc_parse( gimp_gtkrc() );
  }

  dialog = gtk_dialog_new();
  gtk_signal_connect( GTK_OBJECT( dialog ), "destroy",
		      GTK_SIGNAL_FUNC( dialog_destroy_callback ), NULL );

  mainbox = gtk_hbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), mainbox );
  gtk_widget_show( mainbox );
  
  {
    GtkWidget *button;
    
    button = gtk_button_new_with_label( "OK" );
    gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			       GTK_SIGNAL_FUNC( dialog_ok_callback ),
			       GTK_OBJECT( dialog ) );
    gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->action_area ),
			button, TRUE, TRUE, 0 );
    GTK_WIDGET_SET_FLAGS( button, GTK_CAN_DEFAULT );
    gtk_widget_grab_default( button );
    gtk_widget_show( button );

    button = gtk_button_new_with_label( "Cancel" );
    gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			       GTK_SIGNAL_FUNC( dialog_cancel_callback ),
			       GTK_OBJECT( dialog ) );
    gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->action_area ),
			button, TRUE, TRUE, 0 );
    gtk_widget_show( button );

    button = gtk_button_new_with_label( "Help" );
    gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",
			       GTK_SIGNAL_FUNC( dialog_help_callback ),
			       GTK_OBJECT( dialog ) );
    gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->action_area ),
			button, TRUE, TRUE, 0 );
    gtk_widget_show( button );
  }
  
  {
    GtkWidget *vbox;
    GtkWidget *frame;
    
    vbox = gtk_vbox_new( TRUE, 0 );
    gtk_container_border_width( GTK_CONTAINER( vbox ), 10 );
    gtk_box_pack_start( GTK_BOX( mainbox ), vbox, FALSE, FALSE, 0 );
    gtk_widget_show( vbox );

    frame = gtk_frame_new( NULL );
    gtk_frame_set_shadow_type( GTK_FRAME( frame ), GTK_SHADOW_IN );
    gtk_box_pack_start( GTK_BOX( vbox ), frame, FALSE, FALSE, 0 );
    gtk_widget_show( frame );

    dialog_preview_init();
    gtk_container_add( GTK_CONTAINER( frame ), preview.preview );
    gtk_widget_show( preview.preview );
  }
  
  {
    GtkWidget *vbox;
    GtkWidget *entrytable;
    GtkWidget *separator;
    GtkWidget *frame;
    GtkWidget *framebox;
    GtkWidget *button;
    GSList    *group;
    
    vbox = gtk_vbox_new( FALSE, 0 );
    gtk_container_border_width( GTK_CONTAINER( vbox ), 10 );
    gtk_box_pack_start( GTK_BOX( mainbox ), vbox, FALSE, FALSE, 0 );
    gtk_widget_show( vbox );

    entrytable = dialog_entry_table();
    gtk_box_pack_start( GTK_BOX( vbox ), entrytable, FALSE, FALSE, 0 );
    gtk_widget_show( entrytable );
    
    separator = gtk_hseparator_new();
    gtk_box_pack_start( GTK_BOX( vbox ), separator, FALSE, FALSE, 0 );
    gtk_widget_show( entrytable );

    frame = gtk_frame_new( "Outside Type" );
    gtk_container_border_width( GTK_CONTAINER( frame ), 5 );
    gtk_box_pack_start( GTK_BOX( vbox ), frame, TRUE, TRUE, 0 );
    gtk_widget_show( frame );

    framebox = gtk_vbox_new( FALSE, 0 );
    gtk_container_border_width( GTK_CONTAINER( framebox ), 5 );
    gtk_container_add( GTK_CONTAINER( frame ), framebox );
    gtk_widget_show( framebox );

    group = NULL;
    
    button = gtk_radio_button_new_with_label( group, "Wrap" );
    gtk_box_pack_start( GTK_BOX( framebox ), button, FALSE, FALSE, 0 );
    gtk_signal_connect( GTK_OBJECT( button ), "toggled",
			GTK_SIGNAL_FUNC( dialog_outside_type_callback ),
			&outside_type.wrap );
    gtk_widget_show( button );
    if( parameters.outside_type == OUTSIDE_TYPE_WRAP ){
      gtk_toggle_button_toggled( GTK_TOGGLE_BUTTON( button ) );
      gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON( button ), TRUE );
    }
    group = gtk_radio_button_group( GTK_RADIO_BUTTON( button ) );

    button = gtk_radio_button_new_with_label( group, "Transparent" );
    gtk_box_pack_start( GTK_BOX( framebox ), button, FALSE, FALSE, 0 );
    gtk_signal_connect( GTK_OBJECT( button ), "toggled",
			GTK_SIGNAL_FUNC( dialog_outside_type_callback ),
			&outside_type.transparent );
    gtk_widget_show( button );
    if( parameters.outside_type == OUTSIDE_TYPE_TRANSPARENT ){
      gtk_toggle_button_toggled( GTK_TOGGLE_BUTTON( button ) );
      gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON( button ), TRUE );
    }
    if( !image.alpha ){
      gtk_widget_set_sensitive( button, FALSE );
    }
    group = gtk_radio_button_group( GTK_RADIO_BUTTON( button ) );
    
    button = gtk_radio_button_new_with_label( group, "Black" );
    gtk_box_pack_start( GTK_BOX( framebox ), button, FALSE, FALSE, 0 );
    gtk_signal_connect( GTK_OBJECT( button ), "toggled",
			GTK_SIGNAL_FUNC( dialog_outside_type_callback ),
			&outside_type.black );
    gtk_widget_show( button );
    if( parameters.outside_type == OUTSIDE_TYPE_BLACK ){
      gtk_toggle_button_toggled( GTK_TOGGLE_BUTTON( button ) );
      gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON( button ), TRUE );
    }
    group = gtk_radio_button_group( GTK_RADIO_BUTTON( button ) );
    
    button = gtk_radio_button_new_with_label( group, "White" );
    gtk_box_pack_start( GTK_BOX( framebox ), button, FALSE, FALSE, 0 );
    gtk_signal_connect( GTK_OBJECT( button ), "toggled",
			GTK_SIGNAL_FUNC( dialog_outside_type_callback ),
			&outside_type.white );
    gtk_widget_show( button );
    if( parameters.outside_type == OUTSIDE_TYPE_WHITE ){
      gtk_toggle_button_toggled( GTK_TOGGLE_BUTTON( button ) );
      gtk_toggle_button_set_state( GTK_TOGGLE_BUTTON( button ), TRUE );
    }
    group = gtk_radio_button_group( GTK_RADIO_BUTTON( button ) );
    
  }
  
  gtk_widget_show( dialog );
  dialog_preview_draw();

  gtk_main();

  return dialog_status;
}

/******************************************************************************/
