/*
 * GFLI 1.0
 *
 * A gimp plug-in to read FLI and FLC movies.
 *
 * Copyright (C) 1997 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * This is a first loader for FLI and FLC movies. It uses as the same method as
 * the gif plug-in to store the animation (i.e. 1 layer/frame).
 * 
 * Current disadvantages:
 * - All frames must share the same colormap. Maybe I add an option for 
 *   generating RGB images in the next version !
 * - Generates A LOT OF warnings.
 * - Consumes a lot of memory (See wish-list: use the original data or 
 *   compression).
 * - doesn't save movies (will be added to next version, depends on
 *   feedback). You can save as animated GIF, of course...
 *
 * Wish-List:
 * - I'd like to have a different format for storing animations, so I can use
 *   Layers and Alpha-Channels for effects. I'm working on another movie-loader
 *   that _requires_ layers (Gold Disk moviesetter). An older version of 
 *   this plug-in created one image per frame, and went real soon out of
 *   memory.
 * - I'd like a method that requests unmodified frames from the original
 *   image, and stores modified frames compressed (suggestion: libpng).
 * - I'd like a way to store additional information about a image to it, for
 *   example copyright stuff or a timecode.
 *
 * Ideas:
 * - I could put all the functions for loading / saving fli into a
 *   gimp-independant library. Anybody interested to save fli from his
 *   application ?
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
gint32 load_image (char *filename);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN ()

static void
query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name entered" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  gimp_install_procedure ("file_fli_load",
                          "load AA-movies",
                          "This is a experimantal plug-in to handle FLI movies",
                          "Jens Ch. Restemeier",
                          "Jens Ch. Restemeier",
                          "1997",
                          "<Load>/FLI",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_register_magic_load_handler ("file_fli_load", "fli,flc", "", "4,short,0x11af,4,short,0x12af");
}

static void
run (char    *name,
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

  if (strcmp (name, "file_fli_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[0].data.d_status = STATUS_SUCCESS;
          values[1].type = PARAM_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          values[0].data.d_status = STATUS_EXECUTION_ERROR;
        }
    }
}

typedef struct _fli_header {
	unsigned long filesize;
	unsigned short magic;
	unsigned short frames;
	unsigned short width;
	unsigned short height;
	unsigned short depth;
	unsigned short flags;
	unsigned long speed;
} s_fli_header;

typedef struct _fli_frame {
	unsigned long framesize;
	unsigned short magic;
	unsigned short chunks;
} s_fli_frame;

/* read byte/word/long */
inline unsigned char fli_read_char(FILE *f)
{
	unsigned char b;
	fread(&b,1,1,f);
	return b;
}

inline unsigned short fli_read_short(FILE *f)
{
	unsigned char b[2];
	fread(&b,1,2,f);
	return (unsigned short)(b[1]<<8) | b[0];
}

inline unsigned long fli_read_long(FILE *f)
{
	unsigned char b[4];
	fread(&b,1,4,f);
	return (unsigned long)(b[3]<<24) | (b[2]<<16) | (b[1]<<8) | b[0];
}

void fli_read_header(FILE *f, s_fli_header *fli_header)
{
	fli_header->filesize=fli_read_long(f);	/* 0 */
	fli_header->magic=fli_read_short(f);	/* 4 */
	fli_header->frames=fli_read_short(f);	/* 6 */
	fli_header->width=fli_read_short(f);	/* 8 */
	fli_header->height=fli_read_short(f);	/* 10 */
	fli_header->depth=fli_read_short(f);	/* 12 */
	fli_header->flags=fli_read_short(f);	/* 14 */
	if (fli_header->magic == 0xAF11) {			/* FLI */
		/* FLI saves speed in 1/70s */
		fli_header->speed=fli_read_short(f)*14;		/* 16 */
	} else {
		if (fli_header->magic == 0xAF12) {		/* FLC */
			/* FLC saves speed in 1/1000s */
			fli_header->speed=fli_read_long(f);	/* 16 */
		} else {
			fprintf(stderr, "error: magic number is wrong !\n");
		}
	}
}

void fli_read_frame(FILE *f, s_fli_header *fli_header, guchar *framebuf, gchar *cmap)
{
	s_fli_frame fli_frame;
	unsigned long framepos;
	int c;
	unsigned short col_pos;
	col_pos=0;
	framepos=ftell(f);

	fli_frame.framesize=fli_read_long(f);
	fli_frame.magic=fli_read_short(f);
	fli_frame.chunks=fli_read_short(f);

	if (fli_frame.magic == 0xF1FA) {
		fseek(f, framepos+16, SEEK_SET);
		for (c=0;c<fli_frame.chunks;c++) {
			unsigned long chunkpos, chunksize;
			unsigned short chunktype;
			chunkpos = ftell(f);
			chunksize=fli_read_long(f);
			chunktype=fli_read_short(f);
			switch (chunktype) {
				case 11 : { /* fli_color */
					unsigned short num_packets;
					int cnt_packets;
					num_packets=fli_read_short(f);
					for (cnt_packets=num_packets; cnt_packets>0; cnt_packets--) {
						unsigned short skip_col, num_col, col_cnt;
						skip_col=fli_read_char(f);
						num_col=fli_read_char(f);
						if (num_col == 0) {
							for (col_pos=0; col_pos<768; col_pos++) {
								cmap[col_pos]=fli_read_char(f)<<2;
							}
						} else {
							col_pos+=(skip_col*3);
							for (col_cnt=num_col; (col_cnt>0) && (col_pos<768); col_cnt--) {
								cmap[col_pos++]=fli_read_char(f)<<2;
								cmap[col_pos++]=fli_read_char(f)<<2;
								cmap[col_pos++]=fli_read_char(f)<<2;
							}
						}
					}
					break;
				}
				case 13 : /* fli_black */
					memset(framebuf, 0, fli_header->width * fli_header->height);
					break;
				case 15 : { /* fli_brun */
					unsigned short yc;
					guchar *pos;
					for (yc=0; yc< fli_header->height; yc++) {
						unsigned short xc, pc, pcnt;
						pc=fli_read_char(f);
						xc=0;
						pos=framebuf+(fli_header->width * yc);
						for (pcnt=pc; pcnt>0; pcnt --) {
							unsigned short ps;
							ps=fli_read_char(f);

							if (ps & 0x80) {
								unsigned short len; 
								ps^=0xFF;
								ps+=1;
								for (len=ps; len>0; len--) {
									pos[xc++]=fli_read_char(f);
								} 
							} else {
								unsigned char val;
								val=fli_read_char(f);
								memset(&(pos[xc]), val, ps);
								xc+=ps;
							}
						}
					} 
					break;
				}
				case 16 : { /* fli_copy */
					unsigned long cc;
					guchar *pos;
					pos=framebuf;
					for (cc=fli_header->width * fli_header->height; cc>0; cc--) {
						*(pos++)=fli_read_char(f);
					}
					break;
				}
				case 12 : { /* fli_lc */ 
					unsigned short yc, firstline, numline;
					guchar *pos;
					firstline = fli_read_short(f);
					numline = fli_read_short(f);
					for (yc=0; yc < numline; yc++) {
						unsigned short xc, pc, pcnt;
						pc=fli_read_char(f);
						xc=0;
						pos=framebuf+(fli_header->width * (firstline+yc));
						for (pcnt=pc; pcnt>0; pcnt --) {
							unsigned short ps,skip;
							skip=fli_read_char(f);
							ps=fli_read_char(f);
							xc+=skip;

							if (ps & 0x80) {
								unsigned char val;
								ps^=0xFF;
								ps+=1;
								val=fli_read_char(f);
								memset(&(pos[xc]), val, ps);
								xc+=ps;
							} else {
								unsigned short len; 
								for (len=ps; len>0; len--) {
									pos[xc++]=fli_read_char(f);
								} 
							}
						}
					} 
					break;
				}
				case 7  : { /* fli_lc2 */ 
					unsigned short yc, lc, numline;
					guchar *pos;
					yc=0;
					numline = fli_read_short(f);
					for (lc=0; lc < numline; lc++) {
						unsigned short xc, pc, pcnt, lpf, lpn;
						pc=fli_read_short(f);
						lpf=0; lpn=0;
						while (pc & 0x8000) {
							if (pc & 0x4000) {
								yc+=pc & 0x3FFF;
							} else {
								lpf=1;lpn=pc&0xFF;
							}
							pc=fli_read_short(f);
						}
						xc=0;
						pos=framebuf+(fli_header->width * yc);
						for (pcnt=pc; pcnt>0; pcnt --) {
							unsigned short ps,skip;
							skip=fli_read_char(f);
							ps=fli_read_char(f);
							xc+=skip;

							if (ps & 0x80) {
								unsigned char v1,v2;
								ps^=0xFF;
								ps++;
								v1=fli_read_char(f);
								v2=fli_read_char(f);
								while (ps>0) {
									pos[xc++]=v1;
									pos[xc++]=v2;
									ps--;
								}
							} else {
								unsigned short len; 
								for (len=ps; len>0; len--) {
									pos[xc++]=fli_read_char(f);
									pos[xc++]=fli_read_char(f);
								} 
							}
						}
						if (lpf) pos[xc]=lpn;
						yc++;
					} 
					break;
				}
				case 4  : { /* fli_color_2 */
					unsigned short num_packets;
					int cnt_packets;
					num_packets=fli_read_short(f);
					col_pos=0;
					for (cnt_packets=num_packets; cnt_packets>0; cnt_packets--) {
						unsigned short skip_col, num_col, col_cnt;
						skip_col=fli_read_char(f);
						num_col=fli_read_char(f);
						if (num_col == 0) {
							for (col_pos=0; col_pos<768; col_pos++) {
								cmap[col_pos]=fli_read_char(f);
							}
						} else {
							col_pos+=(skip_col+=3);
							for (col_cnt=num_col; (col_cnt>0) && (col_pos<768); col_cnt--) {
								cmap[col_pos++]=fli_read_char(f);
								cmap[col_pos++]=fli_read_char(f);
								cmap[col_pos++]=fli_read_char(f);
							}
						}
					}
					break;
				}
				case 18 : /* fli_mini (ignored) */ break;
			}
			if (chunksize & 1) chunksize++;
			fseek(f,chunkpos+chunksize,SEEK_SET);
		}
	}
	fseek(f, framepos+fli_frame.framesize, SEEK_SET);
}

gint32 load_image (char *filename)
{
	FILE *f;
	char *name_buf;
	GDrawable *drawable;
	gint32 image_ID, layer_ID;
    	
	guchar *frame_buffer;
	guchar cmap[768];
	GPixelRgn pixel_rgn;
	s_fli_header fli_header;

	int cnt;

	name_buf = g_malloc (64);

	sprintf (name_buf, "Loading %s:", filename);
	gimp_progress_init (name_buf);

	f=fopen(filename ,"r");
	if (!f) {
		printf ("FLI: can't open \"%s\"\n", filename);
		return -1;
	}
	fli_read_header(f, &fli_header);
	fseek(f,128,SEEK_SET);

	image_ID = gimp_image_new (fli_header.width, fli_header.height, INDEXED);
	gimp_image_set_filename (image_ID, filename);

	frame_buffer=g_malloc(fli_header.width * fli_header.height);
	for (cnt=0; cnt< fli_header.frames; cnt++) {
		sprintf(name_buf, "Frame (%i)",cnt+1); 
		layer_ID = gimp_layer_new (
			image_ID, name_buf,
			fli_header.width, fli_header.height,
			INDEXED_IMAGE, 100, NORMAL_MODE);
		gimp_image_add_layer (image_ID, layer_ID, 0);

		drawable = gimp_drawable_get (layer_ID);
		gimp_progress_update ((double) cnt / fli_header.frames);

		fli_read_frame(f, &fli_header, frame_buffer, cmap);

		gimp_pixel_rgn_init     (&pixel_rgn, drawable,     0, 0, fli_header.width, fli_header.height, TRUE, FALSE);
		gimp_pixel_rgn_set_rect (&pixel_rgn, frame_buffer, 0, 0, fli_header.width, fli_header.height);

		gimp_drawable_flush (drawable);
		gimp_drawable_detach (drawable);
	}
	gimp_image_set_cmap (image_ID, cmap, 256);
	fclose(f);

	g_free (name_buf);
	g_free(frame_buffer);
	return image_ID;
}
