/*
 * This is the Glass Tile plug-in for the GIMP 1.2
 * Version 1.02
 *
 * Copyright (C) 1997 Karl-Johan Andersson (t96kja@student.tdb.uu.se)
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
 * This filter divide the image into square "glass"-blocks in which
 * the image is refracted. 
 * 
 * The alpha-channel is left unchanged.
 * 
 * Please send any comments or suggestions to
 * Karl-Johan Andersson (t96kja@student.tdb.uu.se)
 *
 * May 2000 - tim copperfield [timecop@japan.co.jp]
 * Added preview mode.
 * Noticed there is an issue with the algorithm if odd number of rows or
 * columns is requested.  Dunno why.  I am not a graphics expert :(
 *  
 * May 2000 alt@gimp.org Made preview work and removed some boundary 
 * conditions that caused "streaks" to appear when using some tile spaces.
 */ 

#include "config.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PREVIEW_SIZE  128 

/* --- Typedefs --- */
typedef struct
{
  gint xblock;
  gint yblock;
} GlassValues;

typedef struct
{
    gint run;
} GlassInterface;

/* --- Declare local functions --- */
static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GimpParam  *param,
		   gint    *nreturn_vals,
		   GimpParam **return_vals);

static gint glass_dialog               (GimpDrawable     *drawable);
static void glass_ok_callback          (GtkWidget     *widget,
					gpointer       data);

static void fill_preview_with_thumb    (GtkWidget     *preview_widget, 
					gint32         drawable_ID);
static GtkWidget *preview_widget       (GimpDrawable     *drawable);

static void glasstile                  (GimpDrawable     *drawable, 
					gboolean       preview_mode);

/* --- Variables --- */
GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};
static GlassValues gtvals =
{
    20,    /* tile width  */
    20     /* tile height */
};
static GlassInterface gt_int =
{
  FALSE    /* run */
};

/* preview */
static guchar    *preview_bits;
static GtkWidget *preview;
static gdouble    preview_scale_x;
static gdouble    preview_scale_y;
static guchar    *preview_cache;
static gint       preview_cache_rowstride;
static gint       preview_cache_bpp;


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
    { GIMP_PDB_INT32, "tilex", "Tile width (10 - 50)" },
    { GIMP_PDB_INT32, "tiley", "Tile height (10 - 50)" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_glasstile",
			  "Divide the image into square glassblocks",
			  "Divide the image into square glassblocks in which the image is refracted.",
			  "Karl-Johan Andersson", /* Author */
			  "Karl-Johan Andersson", /* Copyright */
			  "May 2000",
			  N_("<Image>/Filters/Glass Effects/Glass Tile..."),
			  "RGB*, GRAY*",
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[1];
  GimpDrawable *drawable;
  GimpRunModeType run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  
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
      INIT_I18N_UI();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_glasstile", &gtvals);
      
      /*  First acquire information with a dialog  */
      if (! glass_dialog (drawable))
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;
      
    case GIMP_RUN_NONINTERACTIVE:
      INIT_I18N();
      /*  Make sure all the arguments are there!  */
      if (nparams != 5)
	status = GIMP_PDB_CALLING_ERROR;
      if (status == GIMP_PDB_SUCCESS)
	{
	  gtvals.xblock = (gint) param[3].data.d_int32;
	  gtvals.yblock = (gint) param[4].data.d_int32;
	}
      if (gtvals.xblock < 10 || gtvals.xblock > 50) 
	status = GIMP_PDB_CALLING_ERROR;
      if (gtvals.yblock < 10 || gtvals.yblock > 50) 
	status = GIMP_PDB_CALLING_ERROR;
      break;
      
    case GIMP_RUN_WITH_LAST_VALS:
      INIT_I18N();
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_glasstile", &gtvals);
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
	  gimp_progress_init ( _("Glass Tile..."));
	  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
	  
	  glasstile (drawable, 0);
	  
	  if (run_mode != GIMP_RUN_NONINTERACTIVE)
	    gimp_displays_flush (); 
	  /*  Store data  */
	  if (run_mode == GIMP_RUN_INTERACTIVE) 
	    {
	      gimp_set_data ("plug_in_glasstile", &gtvals, 
			     sizeof (GlassValues));
	      g_free (preview_bits);
	      g_free (preview_cache);
	  }
	}
      else
	{
	  /* gimp_message ("glasstile: cannot operate on indexed color images"); */
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }
    
  values[0].data.d_status = status;
  
  gimp_drawable_detach (drawable);
}

static gint
glass_dialog (GimpDrawable *drawable)
{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *abox;
  GtkObject *adj;

  gimp_ui_init ("glasstile", TRUE);

  dlg = gimp_dialog_new (_("Glass Tile"), "glasstile",
			 gimp_standard_help_func, "filters/glasstile.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), glass_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  main_vbox = gtk_vbox_new (FALSE, 3);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  frame = gtk_frame_new (_("Preview"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);
  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);
  preview = preview_widget (drawable); /* we are here */
  gtk_container_add (GTK_CONTAINER (frame), preview);
  glasstile (drawable, TRUE); /* filter routine, initial pass */
  gtk_widget_show (preview);
  
  /*  Parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* Horizontal scale - Width */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Tile Width:"), 150, 0,
			      gtvals.xblock, 10, 50, 2, 10, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &gtvals.xblock);
  gtk_signal_connect_object (GTK_OBJECT (adj), "value_changed",
			     GTK_SIGNAL_FUNC (glasstile),
			     (gpointer)drawable);

  /* Horizontal scale - Height */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Tile Height:"), 150, 0,
			      gtvals.yblock, 10, 50, 2, 10, 0,
			      TRUE, 0, 0,
			      NULL, NULL);
  gtk_object_set_data (GTK_OBJECT (adj), "drawable", drawable);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &gtvals.yblock);
  gtk_signal_connect_object (GTK_OBJECT (adj), "value_changed",
			     GTK_SIGNAL_FUNC (glasstile),
			     (gpointer)drawable);

  gtk_widget_show (frame);
  gtk_widget_show (table);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return gt_int.run;
}

static void
glass_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  gt_int.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
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

  preview_cache = drawable_data;
  preview_cache_rowstride = width * bpp;
  preview_cache_bpp = bpp;

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
}

static void
preview_do_row(gint    row,
	       gint    width,
	       guchar *even,
	       guchar *odd,
	       guchar *src)
{
  gint    x;
  
  guchar *p0 = even;
  guchar *p1 = odd;
  
  gdouble    r, g, b, a;
  gdouble    c0, c1;
  
  for (x = 0; x < width; x++) 
    {
      if (preview_cache_bpp == 4)
	{
	  r = ((gdouble)src[x*4+0]) / 255.0;
	  g = ((gdouble)src[x*4+1]) / 255.0;
	  b = ((gdouble)src[x*4+2]) / 255.0;
	  a = ((gdouble)src[x*4+3]) / 255.0;
	}
      else if (preview_cache_bpp == 3)
	{
	  r = ((gdouble)src[x*3+0]) / 255.0;
	  g = ((gdouble)src[x*3+1]) / 255.0;
	  b = ((gdouble)src[x*3+2]) / 255.0;
	  a = 1.0;
	}
      else
	{
	  r = ((gdouble)src[x*preview_cache_bpp+0]) / 255.0;
	  g = b = r;
	  if (preview_cache_bpp == 2)
		    a = ((gdouble)src[x*preview_cache_bpp+1]) / 255.0;
	  else
	    a = 1.0;
	}
      
      if ((x / GIMP_CHECK_SIZE) & 1) 
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
  
  if ((row / GIMP_CHECK_SIZE) & 1)
    {
      gtk_preview_draw_row (GTK_PREVIEW (preview), (guchar *)odd,  0, row, width); 
    }
  else
    {
      gtk_preview_draw_row (GTK_PREVIEW (preview), (guchar *)even, 0, row, width); 
    }
}

/*  -  Filter function  -  I wish all filter functions had a pmode :) */
static void
glasstile (GimpDrawable *drawable, 
	   gboolean   preview_mode)
{
  GimpPixelRgn srcPR, destPR;
  gint width, height;
  gint bytes;
  guchar *dest, *d;
  guchar *cur_row;
  gint row, col, i, iwidth;
  gint x1, y1, x2, y2;

  guchar *odd = NULL;
  guchar *even = NULL;

  gint rutbredd, xpixel1, xpixel2;
  gint ruthojd , ypixel2;
  gint xhalv, xoffs, xmitt, xplus;
  gint yhalv, yoffs, ymitt, yplus;
  gint cbytes;
      
  if (preview_mode) 
    {
      width  = GTK_PREVIEW (preview)->buffer_width;
      height = GTK_PREVIEW (preview)->buffer_height;
      bytes  = preview_cache_bpp;

      x1 = y1 = 0;
      x2 = width;
      y2 = height;

      even = g_malloc (width * 3);
      odd  = g_malloc (width * 3);
    } 
  else 
    {
      gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
      width  = drawable->width;
      height = drawable->height;
      bytes  = drawable->bpp;
    }
  
  cur_row = g_new (guchar, width * bytes);
  dest    = g_new (guchar, width * bytes);

  /*  initialize the pixel regions  */
  if (preview_mode) 
    {
      rutbredd = gtvals.xblock * preview_scale_x;
      ruthojd  = gtvals.yblock * preview_scale_y;
    }
  else
    {
      gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
      gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

      rutbredd = gtvals.xblock;
      ruthojd  = gtvals.yblock;
    }

  xhalv = rutbredd / 2;
  yhalv = ruthojd  / 2;
  cbytes = bytes;

  if (cbytes % 2 == 0) 
    cbytes--; 

  iwidth = width - x1;
  xplus = rutbredd % 2;
  yplus = ruthojd  % 2;

  ymitt = y1;
  yoffs = 0;

  /*  Loop through the rows */
  for (row = y1; row < y2; row++)
    {
      d = dest;
      ypixel2 = ymitt + yoffs * 2;

      if(ypixel2 < 0)
	ypixel2 = 0;

      if (preview_mode)
	{
          if (ypixel2 < height)
            memcpy (cur_row, 
		    preview_cache + (ypixel2 * preview_cache_rowstride), 
		    preview_cache_rowstride);
          else 
	    memcpy (cur_row, 
		    preview_cache + ((y2 - 1) * preview_cache_rowstride), 
		    preview_cache_rowstride);
	}
      else
	{
	  if (ypixel2 < height) 
            gimp_pixel_rgn_get_row (&srcPR, cur_row, x1, ypixel2, iwidth);
          else 
            gimp_pixel_rgn_get_row (&srcPR, cur_row, x1, y2 - 1, iwidth);
	}

      yoffs++;

      if (yoffs == yhalv) 
	{
	  ymitt += ruthojd;
	  yoffs =- yhalv;
	  yoffs -= yplus;
	}
      
      xmitt = 0;
      xoffs = 0;

      for (col = 0; col < (x2 - x1); col++) /* one pixel */
	{
	  xpixel1 = (xmitt + xoffs) * bytes;
	  xpixel2 = (xmitt + xoffs * 2) * bytes;

	  if (xpixel2 < ((x2 - x1) * bytes)) 
	    {
	      if(xpixel2 < 0)
		xpixel2 = 0;
	      for (i = 0; i < bytes; i++) 
		d[xpixel1 + i] = cur_row[xpixel2 + i];
	    }
	  else 
	    {
	      for (i = 0; i < bytes; i++)
		d[xpixel1 + i]=cur_row[xpixel1 + i];
	    }

	  xoffs++;

	  if (xoffs == xhalv) 
	    {
	      xmitt += rutbredd;
	      xoffs =- xhalv;
	      xoffs -= xplus;
	    }
	}

      /*  Store the dest  */
      if (preview_mode)
	preview_do_row(row,width,even,odd,dest);
      else
	gimp_pixel_rgn_set_row (&destPR, dest, x1, row, iwidth);
      
      if ((row % 5) == 0 && !preview_mode)
        gimp_progress_update ((double) row / (double) (y2 - y1));
    }


  if(even)
    g_free(even);

  if(odd)
    g_free(odd);


  /*  Update region  */
  if (preview_mode) 
    {
      gtk_widget_queue_draw (preview);
    }
  else
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->id, TRUE);
      gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
    }

  g_free (cur_row);
  g_free (dest);
}




