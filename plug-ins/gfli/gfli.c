/*
 * GFLI 1.0
 *
 * A gimp plug-in to read and write FLI and FLC movies.
 *
 * Copyright (C) 1998 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
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
 * - Generates A LOT OF warnings.
 * - Consumes a lot of memory (See wish-list: use the original data or 
 *   compression).
 * - doesn't support palette changes between two frames.
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
 * - I've thought about a small utility to mix MIDI events as custom chunks
 *   between the FLI chunks. Anyone interested in implementing this ?
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#include "fli.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static void query (void);
static void run (gchar *name, gint nparams, GParam *param, gint *nreturn_vals, GParam **return_vals);

static gint32 load_image (gchar *filename);
static void save_image (gchar *filename, gint32 image);

GPlugInInfo PLUG_IN_INFO = {
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

GParamDef load_args[] = {
	{ PARAM_INT32, "run_mode", "Interactive, non-interactive" },
	{ PARAM_STRING, "filename", "The name of the file to load" },
	{ PARAM_STRING, "raw_filename", "The name entered" },
};
GParamDef load_return_vals[] = {
	{ PARAM_IMAGE, "image", "Output image" },
};
gint nload_args = sizeof (load_args) / sizeof (load_args[0]);
gint nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

GParamDef save_args[] = {
	{ PARAM_INT32, "run_mode", "Interactive, non-interactive" },
	{ PARAM_IMAGE, "image", "Input image (unused)" },
	{ PARAM_DRAWABLE, "drawable", "Input drawable" },
	{ PARAM_STRING, "filename", "The name of the file to save" },
	{ PARAM_STRING, "raw_filename", "The name entered" },
};
gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

MAIN ()

static void query ()
{
	gimp_install_procedure (
		"file_fli_load",
                "load FLI-movies",
                "This is a experimantal plug-in to handle FLI movies",
                "Jens Ch. Restemeier",
                "Jens Ch. Restemeier",
                "1997",
                "<Load>/FLI",
		NULL,
                PROC_PLUG_IN,
                nload_args, nload_return_vals,
                load_args, load_return_vals);
	gimp_register_magic_load_handler ("file_fli_load", "fli", "", "4,byte,0x11,4,byte,0x12,5,byte,0xAF");

	gimp_install_procedure (
		"file_fli_save",
                "save FLI-movies",
                "This is a experimantal plug-in to handle FLI movies",
                "Jens Ch. Restemeier",
                "Jens Ch. Restemeier",
                "1997",
                "<Save>/FLI",
		"INDEXED,GRAY",
                PROC_PLUG_IN,
                nsave_args, 0,
                save_args, NULL);
	gimp_register_save_handler ("file_fli_save", "fli", "");
}

GParam values[2];

void run (gchar *name, gint nparams, GParam *param, gint *nreturn_vals, GParam **return_vals)
{
	GRunModeType run_mode;
	gint32 image_ID;

	run_mode = param[0].data.d_int32;

	*nreturn_vals = 1;
	*return_vals = values;
	values[0].type = PARAM_STATUS;
	values[0].data.d_status = STATUS_CALLING_ERROR;

	if (strcmp (name, "file_fli_load") == 0) {
		image_ID = load_image (param[1].data.d_string);

		if (image_ID != -1) {
			*nreturn_vals = 2;
			values[0].data.d_status = STATUS_SUCCESS;
			values[1].type = PARAM_IMAGE;
			values[1].data.d_image = image_ID;
		} else {
			values[0].data.d_status = STATUS_EXECUTION_ERROR;
		}
		return;
	}
	if (strcmp (name, "file_fli_save") == 0) {
		if (param[1].type == PARAM_IMAGE) {
			save_image (param[3].data.d_string, param[1].data.d_image);
			*nreturn_vals = 1;
			values[0].data.d_status = STATUS_SUCCESS;
		} else {
			values[0].data.d_status = STATUS_EXECUTION_ERROR;
		}
		return;
	}
}

/*
 * load fli animation and store as framestack
 */
gint32 load_image (gchar *filename)
{
	FILE *f;
	gchar *name_buf;
	GDrawable *drawable;
	gint32 image_ID, layer_ID;
    	
	gchar *fb, *ofb, *fb_x;
	gchar cm[768], ocm[768];
	GPixelRgn pixel_rgn;
	s_fli_header fli_header;

	gint cnt;

	name_buf = g_malloc (64);

	sprintf (name_buf, "Loading %s:", filename);
	gimp_progress_init (name_buf);

	f=fopen(filename ,"r");
	if (!f) {
		fprintf (stderr, "FLI: can't open \"%s\"\n", filename);
		return -1;
	}
	fli_read_header(f, &fli_header);
	fseek(f,128,SEEK_SET);

	image_ID = gimp_image_new (fli_header.width, fli_header.height, INDEXED);
	gimp_image_set_filename (image_ID, filename);

	fb=g_malloc(fli_header.width * fli_header.height);
	ofb=g_malloc(fli_header.width * fli_header.height);
	for (cnt=0; cnt < fli_header.frames; cnt++) {
		sprintf(name_buf, "Frame (%i)",cnt+1); 
		layer_ID = gimp_layer_new (
			image_ID, name_buf,
			fli_header.width, fli_header.height,
			INDEXED_IMAGE, 100, NORMAL_MODE);
		gimp_image_add_layer (image_ID, layer_ID, cnt);

		drawable = gimp_drawable_get (layer_ID);
		gimp_progress_update ((double) cnt / (double)fli_header.frames);

		fli_read_frame(f, &fli_header, ofb, ocm, fb, cm);

		gimp_pixel_rgn_init     (&pixel_rgn, drawable,     0, 0, fli_header.width, fli_header.height, TRUE, FALSE);
		gimp_pixel_rgn_set_rect (&pixel_rgn, fb, 0, 0, fli_header.width, fli_header.height);

		gimp_drawable_flush (drawable);
		gimp_drawable_detach (drawable);
		memcpy(ocm, cm, 768);
		fb_x=fb; fb=ofb; ofb=fb_x;
	}
	gimp_image_set_cmap (image_ID, cm, 256);
	fclose(f);

	g_free(name_buf);
	g_free(fb);
	g_free(ofb);
	return image_ID;
}

/*
 * get framestack and store as fli animation
 * (some code was taken from the GIF plugin.)
 */ 
void save_image(gchar *filename, gint32 image_ID)
{
	FILE *f;
	gchar *name_buf;
	GDrawable *drawable;
	GDrawableType drawable_type;
	gint32 *framelist;
	gint nframes;
    	
	gchar *fb, *ofb;
	gchar cm[768];
	GPixelRgn pixel_rgn;
	s_fli_header fli_header;

	gint cnt;

	/*
	 * prepare header and check information
	 */
	framelist = gimp_image_get_layers(image_ID, &nframes);
	drawable_type = gimp_drawable_type(framelist[0]);
	switch (drawable_type) {
		case INDEXEDA_IMAGE:
		case GRAYA_IMAGE:
			/* FIXME: should be a popup */
			fprintf (stderr, "FLI: Sorry, can't save images with Alpha.\n");
			return;
			break;
		case GRAY_IMAGE: {
			/* build grayscale palette */
			int i;
			for (i=0; i<256; i++) {
				cm[i*3+0]=cm[i*3+1]=cm[i*3+2]=i;
			}
			break;
		}
		case INDEXED_IMAGE: {
			gint colors, i;
			gchar *cmap;
			cmap=gimp_image_get_cmap(image_ID, &colors);
			for (i=0; i<colors; i++) {
				cm[i*3+0]=cmap[i*3+0];
				cm[i*3+1]=cmap[i*3+1];
				cm[i*3+2]=cmap[i*3+2];
			}
			for (i=colors; i<256; i++) {
				cm[i*3+0]=cm[i*3+1]=cm[i*3+2]=i;
			}
			break;
		}
		default:
			/* FIXME: should be a popup */
			fprintf (stderr, "FLI: Sorry, I can save only INDEXED and GRAY images.\n");
			return;
			break;
	}
	

	name_buf = g_malloc (64);

	sprintf (name_buf, "Saving %s:", filename);
	gimp_progress_init (name_buf);

	/*
	 * First build the fli header.
	 */
	fli_header.filesize=0;	/* will be fixed when writing the header */
	fli_header.frames=0; 	/* will be fixed during the write */
	fli_header.width=gimp_image_width(image_ID);
	fli_header.height=gimp_image_height(image_ID);

	if ((fli_header.width==320) && (fli_header.height=200)) {
		fli_header.magic=HEADER_FLI;
	} else {
		fli_header.magic=HEADER_FLC;
	}
	fli_header.depth=8;	/* I've never seen a depth != 8 */
	fli_header.flags=3;
	fli_header.speed=1000/25;
	fli_header.created=0;	/* program ID. not neccessary... */
	fli_header.updated=0;	/* date in MS-DOS format. ignore...*/
	fli_header.aspect_x=1;  /* aspect ratio. Will be added as soon.. */
	fli_header.aspect_y=1;  /* ... as GIMP supports it. */

	f=fopen(filename ,"w");
	if (!f) {
		fprintf (stderr, "FLI: can't open \"%s\"\n", filename);
	}
	fseek(f,128,SEEK_SET);

	fb=g_malloc(fli_header.width * fli_header.height);
	ofb=g_malloc(fli_header.width * fli_header.height);
	/*
	 * Now write all frames
	 */
	for (cnt=0; cnt<nframes; cnt++) {
		gint offset_x, offset_y, xc, yc;
		guint rows, cols, rowstride;
		gchar *tmp;

		gimp_progress_update ((double) cnt / (double)nframes);
		
		/* get layer data from GIMP */
		drawable = gimp_drawable_get (framelist[cnt]);
		gimp_drawable_offsets (framelist[cnt], &offset_x, &offset_y);
		cols = drawable->width;
		rows = drawable->height;
		rowstride = drawable->width;
		gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);
		tmp=malloc(cols * rows);
		gimp_pixel_rgn_get_rect (&pixel_rgn, tmp, 0, 0, drawable->width, drawable->height);

		/* now paste it into the framebuffer, with the neccessary offset */
		for (yc=0; yc<cols; yc++) {
			int yy;
			yy=yc+offset_y;
			if ((yy>0) && (yy<fli_header.height)) {
				for (xc=0; xc<cols; xc++) {
					int xx;
					xx=xc+offset_x;
					if ((xx>0) && (xx<fli_header.width)) {
						fb[yy*fli_header.width + xx]=tmp[yc*cols+xc];
					}
				}
			}
		}
		free(tmp);

		/* save the frame */
		if (cnt>0) {
			/* save frame, allow all codecs */
			fli_write_frame(f, &fli_header, ofb, cm, fb, cm, W_ALL);
		} else {
			/* save first frame, no delta information, allow all codecs */
			fli_write_frame(f, &fli_header, NULL, NULL, fb, cm, W_ALL);
		}
		memcpy(ofb, fb, fli_header.width * fli_header.height);
	}

	/*
	 * finish fli
	 */
	fli_write_header(f, &fli_header);
	fclose(f);

	g_free(name_buf);
	g_free(fb);
	g_free(ofb);
	g_free(framelist);
}
