/*
 * This is a plugin for the GIMP.
 *
 * Copyright (C) 1997 Jochen Friedrich
 * Parts Copyright (C) 1995 Gert Doering
 * Parts Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#define VERSION "0.6"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <libgimp/gimp.h>
#include "g3.h"

/* Declare local functions.
 */
gint32        emitgimp   (int, int, char *, int, char *);
void   query      (void);
static void   run        (char    *name,
			  int      nparams,
			  GParam  *param,
			  int     *nreturn_vals,
			  GParam **return_vals);
static gint32 load_image (char *);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN ()

void query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name of the file to load" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  gimp_install_procedure ("file_faxg3_load",
                          "loads g3 fax files",
			  "This plug-in loads Fax G3 Image files.",
                          "Jochen Friedrich",
                          "Jochen Friedrich, Gert Doering, Spencer Kimball & Peter Mattis",
                          VERSION,
			  "<Load>/Fax G3",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_register_magic_load_handler ("file_faxg3_load", "g3", "", "0,short,0x0001,0,short,0x0014");
}

static void run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType run_mode;
  gint32 image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;
  if (strcmp (name, "file_faxg3_load") == 0)
    {
	  *nreturn_vals = 2;
      image_ID = load_image (param[1].data.d_string);
	  values[1].type = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;

      if (image_ID != -1)
		{
		  values[0].data.d_status = STATUS_SUCCESS;
		}
      else
		{
		  values[0].data.d_status = STATUS_EXECUTION_ERROR;
		}
    }
}

#ifdef DEBUG
void putbin (unsigned long d )
{
unsigned long i = 0x80000000;

    while ( i!=0 )
    {
	putc( ( d & i ) ? '1' : '0', stderr );
	i >>= 1;
    }
    putc( '\n', stderr );
}
#endif

static int byte_tab[ 256 ];
/* static int o_stretch; */		/* -stretch: double each line */
/* static int o_stretch_force=-1; */	/* -1: guess from filename */
/* static int o_lj; */			/* -l: LJ output */
/* static int o_turn; */		/* -t: turn 90 degrees right */

struct g3_tree * black, * white;

#define CHUNK 2048;
static	char rbuf[2048];	/* read buffer */
static	int  rp;		/* read pointer */
static	int  rs;		/* read buffer size */

#define MAX_ROWS 4300
#define MAX_COLS 1728		/* !! FIXME - command line parameter */

static gint32
load_image (char *filename)
{
  int data;
  int hibit;
  struct g3_tree * p;
  int nr_pels;
  int fd;
  int color;
  int i, rr, rsize;
  int cons_eol;

  int	bperrow = MAX_COLS/8;	/* bytes per bit row */
  char *bitmap;			/* MAX_ROWS by (bperrow) bytes */
  char *bp;			/* bitmap pointer */
  char *name;
  int	row;
  int	max_rows;		/* max. rows allocated */
  int	col, hcol;		/* column, highest column ever used */

  name = (char *) malloc (strlen (filename) + 12);
  sprintf (name, "Loading %s:", filename);
  gimp_progress_init (name);
  free (name);

  /* initialize lookup trees */
  build_tree( &white, t_white );
  build_tree( &white, m_white );
  build_tree( &black, t_black );
  build_tree( &black, m_black );

  init_byte_tab( 0, byte_tab );

  fd = open(filename, O_RDONLY );

  hibit = 0;
  data = 0;

  cons_eol = 0;	/* consecutive EOLs read - zero yet */

  color = 0;		/* start with white */
  rr = 0;

  rsize = lseek(fd, 0L, SEEK_END);
  lseek(fd, 0L, 0);

  rs = read( fd, rbuf, sizeof(rbuf) );
  if ( rs < 0 ) { perror( "read" ); close( rs ); exit(8); }
  rr += rs;
  gimp_progress_update ((float)rr/rsize/2.0);

			/* skip GhostScript header */
  rp = ( rs >= 64 && strcmp( rbuf+1, "PC Research, Inc" ) == 0 ) ? 64 : 0;

  /* initialize bitmap */

  row = col = hcol = 0;
  bitmap = (char *) malloc( ( max_rows = MAX_ROWS ) * MAX_COLS / 8 );
  if ( bitmap == NULL )
  {
    fprintf( stderr, "cannot allocate %d bytes for bitmap",
             max_rows * MAX_COLS/8 );
    close( fd );
    exit(9);
  }
  memset( bitmap, 0, max_rows * MAX_COLS/8 );
  bp = &bitmap[ row * MAX_COLS/8 ]; 

  while ( rs > 0 && cons_eol < 4 )	/* i.e., while (!EOF) */
  {
#ifdef DEBUG
    fprintf( stderr, "hibit=%2d, data=", hibit );
    putbin( data );
#endif
    while ( hibit < 20 )
    {
      data |= ( byte_tab[ (int) (unsigned char) rbuf[ rp++] ] << hibit );
      hibit += 8;

      if ( rp >= rs )
      {
        rs = read( fd, rbuf, sizeof( rbuf ) );
        if ( rs < 0 ) { perror( "read2"); break; }
        rr += rs;
        gimp_progress_update ((float)rr/rsize/2.0);
        rp = 0;
        if ( rs == 0 ) { goto do_write; }
      }
#ifdef DEBUG
      fprintf( stderr, "hibit=%2d, data=", hibit );
      putbin( data );
#endif
    }

    if ( color == 0 )		/* white */
      p = white->nextb[ data & BITM ];
    else			/* black */
      p = black->nextb[ data & BITM ];

    while ( p != NULL && ! ( p->nr_bits ) )
    {
	    data >>= FBITS;
	    hibit -= FBITS;
	    p = p->nextb[ data & BITM ];
	}

	if ( p == NULL )	/* invalid code */
	{ 
	    fprintf( stderr, "invalid code, row=%d, col=%d, file offset=%lx, skip to eol\n",
		     row, col, (unsigned long) lseek( fd, 0, 1 ) - rs + rp );
	    while ( ( data & 0x03f ) != 0 )
	    {
		data >>= 1; hibit--;
		if ( hibit < 20 )
		{
		    data |= ( byte_tab[ (int) (unsigned char) rbuf[ rp++] ] << hibit );
		    hibit += 8;

		    if ( rp >= rs )	/* buffer underrun */
		    {   rs = read( fd, rbuf, sizeof( rbuf ) );
			if ( rs < 0 ) { perror( "read4"); break; }
        		rr += rs;
      			gimp_progress_update ((float)rr/rsize/2.0);
			rp = 0;
			if ( rs == 0 ) goto do_write;
		    }
		}
	    }
	    nr_pels = -1;		/* handle as if eol */
	}
	else				/* p != NULL <-> valid code */
	{
	    data >>= p->nr_bits;
	    hibit -= p->nr_bits;

	    nr_pels = ( (struct g3_leaf *) p ) ->nr_pels;
#ifdef DEBUG
	    fprintf( stderr, "PELs: %d (%c)\n", nr_pels, '0'+color );
#endif
	}

	/* handle EOL (including fill bits) */
	if ( nr_pels == -1 )
	{
#ifdef DEBUG
	    fprintf( stderr, "hibit=%2d, data=", hibit );
	    putbin( data );
#endif
	    /* skip filler 0bits -> seek for "1"-bit */
	    while ( ( data & 0x01 ) != 1 )
	    {
		if ( ( data & 0xf ) == 0 )	/* nibble optimization */
		{
		    hibit-= 4; data >>= 4;
		}
		else
		{
		    hibit--; data >>= 1;
		}
		/* fill higher bits */
		if ( hibit < 20 )
		{
		    data |= ( byte_tab[ (int) (unsigned char) rbuf[ rp++] ] << hibit );
		    hibit += 8;

		    if ( rp >= rs )	/* buffer underrun */
		    {   rs = read( fd, rbuf, sizeof( rbuf ) );
			if ( rs < 0 ) { perror( "read3"); break; }
        		rr += rs;
      			gimp_progress_update ((float)rr/rsize/2.0);
			rp = 0;
			if ( rs == 0 ) goto do_write;
		    }
		}
#ifdef DEBUG
	    fprintf( stderr, "hibit=%2d, data=", hibit );
	    putbin( data );
#endif
	    }				/* end skip 0bits */
	    hibit--; data >>=1;
	    
	    color=0; 

	    if ( col == 0 )
		cons_eol++;		/* consecutive EOLs */
	    else
	    {
	        if ( col > hcol && col <= MAX_COLS ) hcol = col;
		row++;

		/* bitmap memory full? make it larger! */
		if ( row >= max_rows )
		{
		    char * p = (char *) realloc( bitmap,
				       ( max_rows += 500 ) * MAX_COLS/8 );
		    if ( p == NULL )
		    {
			perror( "realloc() failed, page truncated" );
			rs = 0;
		    }
		    else
		    {
			bitmap = p;
			memset( &bitmap[ row * MAX_COLS/8 ], 0,
			       ( max_rows - row ) * MAX_COLS/8 );
		    }
		}
			
		col=0; bp = &bitmap[ row * MAX_COLS/8 ]; 
		cons_eol = 0;
	    }
	}
	else		/* not eol */
	{
	    if ( col+nr_pels > MAX_COLS ) nr_pels = MAX_COLS - col;

	    if ( color == 0 )                  /* white */
		col += nr_pels;
	    else                               /* black */
	    {
            register int bit = ( 0x80 >> ( col & 07 ) );
	    register char *w = & bp[ col>>3 ];

		for ( i=nr_pels; i > 0; i-- )
		{
		    *w |= bit;
		    bit >>=1; if ( bit == 0 ) { bit = 0x80; w++; }
		    col++;
		}
	    }
	    if ( nr_pels < 64 ) color = !color;		/* terminating code */
	}
    }		/* end main loop */

do_write:      	/* write pbm (or whatever) file */

    if( fd != 0 ) close(fd);	/* close input file */

#ifdef DEBUG
    fprintf( stderr, "consecutive EOLs: %d, max columns: %d\n", cons_eol, hcol );
#endif

    return emitgimp(hcol, row, bitmap, bperrow, filename );

}

/* hcol is the number of columns, row the number of rows
 * bperrow is the number of bytes actually used by hcol, which may
 * be greater than (hcol+7)/8 [in case of an unscaled g3 image less
 * than 1728 pixels wide]
 */

gint32 emitgimp ( int hcol, int row, char *bitmap, int bperrow, char *filename )
{
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  gint32 image_ID;
  gint32 layer_ID;
  guchar *buf;
  guchar tmp;
  int x,y,xx,yy,tile_height;

  /* initialize */

  tmp = 0;

#ifdef DEBUG
  fprintf( stderr, "emit gimp: %d x %d\n", hcol, row);
#endif

  image_ID = gimp_image_new (hcol, row, GRAY);
  gimp_image_set_filename (image_ID, filename);

  layer_ID = gimp_layer_new (image_ID, "Background",
			     hcol,
			     row,
			     GRAY_IMAGE, 100, NORMAL_MODE);
  gimp_image_add_layer (image_ID, layer_ID, 0);

  drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, FALSE);
  tile_height = gimp_tile_height ();
#ifdef DEBUG
  fprintf( stderr, "tile height: %d\n", tile_height);
#endif

  buf = g_new(guchar, hcol*tile_height);
  xx=0;
  yy=0;
  for (y=0; y<row; y++) {
    for (x=0; x<hcol; x++) {
      if ((x&7)==0) tmp = bitmap[y*bperrow+(x>>3)];
      buf[xx++]=tmp&(128>>(x&7))?0:255;
    }
    if ((y-yy) == tile_height-1) {
#ifdef DEBUG
      fprintf( stderr, "update tile height: %d\n", tile_height);
#endif
      gimp_pixel_rgn_set_rect (&pixel_rgn, buf, 0, yy, hcol, tile_height);
      gimp_progress_update (0.5+(float)y/row/2.0);
      xx=0;
      yy += tile_height;
    }
  }
  if (row-yy) {
#ifdef DEBUG
    fprintf( stderr, "update rest: %d\n", row-yy);
#endif
    gimp_pixel_rgn_set_rect (&pixel_rgn, buf, 0, yy, hcol, row-yy);
  }
  gimp_progress_update (1.0);

  g_free (buf);

  gimp_drawable_flush (drawable);

  return image_ID;

}

