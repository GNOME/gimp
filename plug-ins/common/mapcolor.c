/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * mapcolor plugin
 * Copyright (C) 1998 Peter Kirchgessner
 * email: peter@kirchgessner.net, WWW: http://www.kirchgessner.net)
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

/* Event history:
 * V 1.00, PK, 26-Oct-98: Creation.
 * V 1.01, PK, 21-Nov-99: Fix problem with working on layered images
 *                        Internationalization
 * V 1.02, PK, 19-Mar-00: Better explaining text/dialogue
 *                        Preview added
 *                        Fix problem with divide by zero
 * V 1.03,neo, 22-May-00: Fixed divide by zero in preview code.
 */
#define VERSIO                                     1.03
static char dversio[] =                          "v1.03  22-May-00";
static char ident[] = "@(#) GIMP mapcolor plug-in v1.03  22-May-00";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PRV_WIDTH  50
#define PRV_HEIGHT 20

typedef struct
{
  guchar colors[4][3];
  gint32 map_mode;
} PluginValues;

PluginValues plvals =
{
  {
    { 0, 0, 0},
    { 255, 255, 255 },
    { 0, 0, 0 },
    { 255, 255, 255 }
  },
  0
};


/* Preview handling stuff */

#define IMG_PRV_SIZE 128

typedef struct
{
  guint   width, height;
  guchar *img;
} IMG_PREVIEW;


typedef struct
{
 GtkWidget   *preview;
 IMG_PREVIEW *img_preview;
 IMG_PREVIEW *map_preview;
} PLInterface;


static gboolean run_flag = FALSE;

/* Declare some local functions.
 */
static void   query (void);
static void   run   (gchar   *name,
		     gint     nparams,
		     GParam  *param,
		     gint    *nreturn_vals,
		     GParam **return_vals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static IMG_PREVIEW   *img_preview_alloc (guint width, guint height);
static void           img_preview_free (IMG_PREVIEW *ip);
static void           img_preview_copy (IMG_PREVIEW *src, IMG_PREVIEW **dst);
static IMG_PREVIEW   *img_preview_create_from_drawable (guint maxsize,
							gint32 drawable_ID);

static void            update_img_preview (void);

static gboolean        dialog         (gint32 drawable_ID);
static void   mapcolor_ok_callback    (GtkWidget *widget,
				       gpointer   data);
static void   get_mapping (guchar *src_col1, guchar *src_col2,
			   guchar *dst_col1, guchar *dst_col2, gint32  map_mode,
			   guchar *redmap, guchar *greenmap, guchar *bluemap);
static void   color_button_color_changed_callback (GtkWidget *widget,
						   gpointer   data);
static void   add_color_button        (gint       csel_index,
				       gint       left,
				       gint       top,
				       GtkWidget *table);

static void   color_mapping           (GDrawable *drawable);


/* The run mode */
static GRunModeType l_run_mode;

static gchar *csel_title[4] =
{
  N_("First Source Color"),
  N_("Second Source Color"),
  N_("First Destination Color"),
  N_("Second Destination Color")
};

static PLInterface plinterface;


/* Allocate image preview structure and preview memory */
static IMG_PREVIEW *
img_preview_alloc (guint width, guint height)

{IMG_PREVIEW *ip;

 ip = (IMG_PREVIEW *)g_malloc (sizeof (IMG_PREVIEW));
 ip->img = (guchar *)g_malloc (width*height*3);
 if (ip->img == NULL)
 {
   g_free (ip);
   return NULL;
 }
 ip->width = width;
 ip->height = height;

 return ip;
}


/* Free image preview */
static void
img_preview_free (IMG_PREVIEW *ip)

{
 if (ip)
 {
   if (ip->img)
   {
     g_free (ip->img);
     ip->img = NULL;
   }
   ip->width = ip->height = 0;
   g_free (ip);
 }
}


/* Copy image preview. Create/modify destinataion preview */
static void
img_preview_copy (IMG_PREVIEW *src, IMG_PREVIEW **dst)

{int numbytes;
 IMG_PREVIEW *dst_p;

 if ((src == NULL) || (src->img == NULL) || (dst == NULL)) return;

 numbytes = src->width * src->height * 3; /* 1 byte spare */
 if (numbytes <= 0) return;

 if (*dst == NULL)  /* Create new preview ? */
 {
   *dst = img_preview_alloc (src->width, src->height);
   if (*dst == NULL) return;
   memcpy ((*dst)->img, src->img, numbytes);
   return;
 }

 /* destination preview already exists */
 dst_p = *dst;

 /* Did not already allocate enough memory ? */
 if ((dst_p->img != NULL) && (dst_p->width*dst_p->height*3 < numbytes))
 {
   g_free (dst_p->img);
   dst_p->width = dst_p->height = 0;
   dst_p->img = NULL;
 }
 if (dst_p->img == NULL)
 {
   dst_p->img = (guchar *)g_malloc (numbytes);
   if (dst_p->img == NULL) return;
 }
 dst_p->width = src->width;
 dst_p->height = src->height;
 memcpy (dst_p->img, src->img, numbytes);
}


static IMG_PREVIEW *
img_preview_create_from_drawable (guint  maxsize, 
				  gint32 drawable_ID)
{
 GDrawable *drw;
 GPixelRgn pixel_rgn;
 guint drw_width, drw_height;
 guint prv_width, prv_height;
 gint  src_x, src_y, x, y;
 guchar *prv_data, *img_data, *cu_row;
 double xfactor, yfactor;
 gint tile_height, row_start, row_end;
 gint bpp;
 IMG_PREVIEW *ip;

 drw_width = gimp_drawable_width (drawable_ID);
 drw_height = gimp_drawable_height (drawable_ID);
 tile_height = (gint)gimp_tile_height();
 bpp = gimp_drawable_bpp(drawable_ID);

 img_data = g_malloc (drw_width * tile_height * bpp);
 if (img_data == NULL) 
   return NULL;

 /* Calculate preview size */
 if ((drw_width <= maxsize) && (drw_height <= maxsize))
 {
   prv_width = drw_width;
   prv_height = drw_height;
 }
 else
 {
   xfactor = ((double)maxsize) / ((double)drw_width);
   yfactor = ((double)maxsize) / ((double)drw_height);
   if (xfactor < yfactor)
   {
     prv_width = maxsize;
     prv_height = (guint)(drw_height * xfactor);
   }
   else
   {
     prv_width = (guint)(drw_width * yfactor);
     prv_height = maxsize;
   }
 }
 ip = img_preview_alloc (prv_width, prv_height);
 if (ip == NULL) 
   return NULL;

 drw = gimp_drawable_get (drawable_ID);
 prv_data = ip->img;
 gimp_pixel_rgn_init (&pixel_rgn, drw, 0, 0, drw_width, drw_height,
                      FALSE, FALSE);
 row_start = row_end = -1;

 /* Get the pixels for the preview from the drawable */
 for (y = 0; y < prv_height; y++)
 {
   src_y = (drw_height * y) / prv_height;
   if (src_y > row_end)         /* Need new row ? */
   {
     row_start = (src_y / tile_height) * tile_height;
     row_end = row_start+tile_height-1;
     if (row_end > drw_height-1) row_end = drw_height-1;
     gimp_pixel_rgn_get_rect (&pixel_rgn, img_data, 0, row_start, drw_width,
                              row_end-row_start+1);
   }
   cu_row = img_data + (src_y-row_start)*drw_width*bpp;
   for (x = 0; x < prv_width; x++)
   {
     src_x = (drw_width * x) / prv_width;
     memcpy (prv_data, cu_row+bpp*src_x, 3);
     prv_data += 3;
   }
 }

 gimp_drawable_detach (drw);
 g_free (img_data);
 return ip;
}


MAIN ()

static void
query (void)

{
  static GParamDef adjust_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (not used)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable to adjust" }
  };
  static gint nadjust_args = sizeof (adjust_args) / sizeof (adjust_args[0]);

  static GParamDef map_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (not used)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable where colors are to map" },
    { PARAM_COLOR, "srccolor_1", "First source color" },
    { PARAM_COLOR, "srccolor_2", "Second source color" },
    { PARAM_COLOR, "dstcolor_1", "First destination color" },
    { PARAM_COLOR, "dstcolor_2", "Second destination color" },
    { PARAM_INT32, "map_mode", "Mapping mode (0: linear, others reserved)" }
  };
  static gint nmap_args = sizeof (map_args) / sizeof (map_args[0]);

  gimp_install_procedure ("plug_in_color_adjust",
                          "Adjust color range given by foreground/background "
                          "color to black/white",
                          "The current foreground color is mapped to black "
                          "(black point), the current background color is "
                          "mapped to white (white point). Intermediate "
                          "colors are interpolated",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner",
                          dversio,
                          N_("<Image>/Filters/Colors/Map/Adjust FG-BG"),
                          "RGB*",
                          PROC_PLUG_IN,
                          nadjust_args, 0,
                          adjust_args, NULL);

  gimp_install_procedure ("plug_in_color_map",
                          "Map color range specified by two colors"
			  "to color range specified by two other color.",
                          "Map color range specified by two colors"
			  "to color range specified by two other color."
                          "Intermediate colors are interpolated.",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner",
                          dversio,
                          N_("<Image>/Filters/Colors/Map/Color Range Mapping..."),
                          "RGB*",
                          PROC_PLUG_IN,
                          nmap_args, 0,
                          map_args, NULL);
}


static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)

{
  static GParam values[1];
  GRunModeType run_mode;
  GDrawable *drawable = NULL;
  GStatusType status = STATUS_SUCCESS;
  guchar *c = (guchar *)ident;
  int j;

  INIT_I18N_UI();

  l_run_mode = run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  while (status == STATUS_SUCCESS)
    {
      if (nparams < 3)
	{
	  status = STATUS_CALLING_ERROR;
	  break;
	}

      /* Make sure the drawable is RGB color */
      drawable = gimp_drawable_get (param[2].data.d_drawable);
      if (!gimp_drawable_is_rgb (drawable->id))
	{
	  g_message (_("Color Mapping / Adjust FG/BG:\nCannot operate on gray/indexed images"));
	  status = STATUS_EXECUTION_ERROR;
	  break;
	}

      if (strcmp (name, "plug_in_color_adjust") == 0)
	{
	  if (nparams != 3)  /* Make sure all the arguments are there */
	    {
	      status = STATUS_CALLING_ERROR;
	      break;
	    }

	  c = &(plvals.colors[0][0]);      /* First source color */
	  gimp_palette_get_foreground (c, c+1, c+2);

	  c = &(plvals.colors[1][0]);      /* Second source color */
	  gimp_palette_get_background (c, c+1, c+2);

	  c = &(plvals.colors[2][0]);      /* First destination color */
	  c[0] = c[1] = c[2] = 0;          /* Foreground mapped to black */

	  c = &(plvals.colors[3][0]);      /* Second destination color */
	  c[0] = c[1] = c[2] = 255;        /* Background mapped to white */

	  plvals.map_mode = 0;

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_progress_init (_("Adjusting Foreground/Background"));

	  color_mapping (drawable);
	  break;
	}

      if (strcmp (name, "plug_in_color_map") == 0)
	{
	  if (run_mode == RUN_NONINTERACTIVE)
	    {
	      if (nparams != 8)  /* Make sure all the arguments are there */
		{
		  status = STATUS_CALLING_ERROR;
		  break;
		}

	      for (j = 0; j < 4; j++)
		{
		  plvals.colors[j][0] = param[3+j].data.d_color.red;
		  plvals.colors[j][1] = param[3+j].data.d_color.green;
		  plvals.colors[j][2] = param[3+j].data.d_color.blue;
		}
	      plvals.map_mode = param[7].data.d_int32;
	    }
	  else if (run_mode == RUN_INTERACTIVE)
	    {
	      gimp_get_data (name, &plvals);

	      c = &(plvals.colors[0][0]);      /* First source color */
	      gimp_palette_get_foreground (c, c+1, c+2);

	      c = &(plvals.colors[1][0]);      /* Second source color */
	      gimp_palette_get_background (c, c+1, c+2);

	      if (!dialog (param[2].data.d_drawable))
		break;
	    }
	  else if (run_mode == RUN_WITH_LAST_VALS)
	    {
	      gimp_get_data (name, &plvals);
	    }
	  else
	    {
	      status = STATUS_CALLING_ERROR;
	      break;
	    }

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_progress_init (_("Mapping colors"));

	  color_mapping (drawable);

	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data (name, &plvals, sizeof (plvals));

	  break;
	}

      status = STATUS_EXECUTION_ERROR;
    }

  if ((status == STATUS_SUCCESS) && (run_mode != RUN_NONINTERACTIVE))
    gimp_displays_flush ();

  if (drawable != NULL) gimp_drawable_detach (drawable);
  values[0].data.d_status = status;
}

static void
update_img_preview (void)

{IMG_PREVIEW *dst_ip = plinterface.map_preview;
 IMG_PREVIEW *src_ip = plinterface.img_preview;
 guchar *src, *dst;
 GtkWidget *preview = plinterface.preview;
 int j;
 unsigned char redmap[256], greenmap[256], bluemap[256];
 unsigned char *src_col1 = plvals.colors[0];
 unsigned char *src_col2 = plvals.colors[1];
 unsigned char *dst_col1 = plvals.colors[2];
 unsigned char *dst_col2 = plvals.colors[3];

 if ((dst_ip == NULL) || (src_ip == NULL)) return;

 get_mapping (src_col1, src_col2, dst_col1, dst_col2, plvals.map_mode,
              redmap, greenmap, bluemap);

 j = dst_ip->width*dst_ip->height;
 src = src_ip->img;
 dst = dst_ip->img;
 while (j-- > 0)
 {
   *(dst++) = redmap[*(src++)];
   *(dst++) = greenmap[*(src++)];
   *(dst++) = bluemap[*(src++)];
 }
 for (j = 0; j < dst_ip->height; j++)
   gtk_preview_draw_row (GTK_PREVIEW (preview), dst_ip->img + dst_ip->width*3*j,
                         0, j, dst_ip->width);
 gtk_widget_draw (preview, NULL);
 gdk_flush ();
}


static gboolean
dialog (gint32 drawable_ID)
{
  GtkWidget *dlg;
  GtkWidget *frame, *pframe;
  GtkWidget *abox;
  GtkWidget *table;
  GtkWidget *preview;
  IMG_PREVIEW *ip;
  gint  j;

  gimp_ui_init ("mapcolor", TRUE);

  memset (&plinterface, 0, sizeof (plinterface));

  dlg = gimp_dialog_new (_("Map Color Range"), "mapcolor",
			 gimp_plugin_help_func, "filters/mapcolor.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), mapcolor_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);

  /* Preview */
  ip = img_preview_create_from_drawable (IMG_PRV_SIZE, drawable_ID);
  if (ip)
    {
      plinterface.img_preview = ip;
      img_preview_copy (plinterface.img_preview, &(plinterface.map_preview));

      frame = gtk_frame_new (_("Preview"));
      gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame,
			  FALSE, FALSE, 0);

      abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
      gtk_container_add (GTK_CONTAINER (frame), abox);

      pframe = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (pframe), 4);
      gtk_container_add (GTK_CONTAINER (abox), pframe);

      preview = gtk_preview_new (GTK_PREVIEW_COLOR);
      plinterface.preview = preview;
      gtk_preview_size (GTK_PREVIEW (preview), ip->width, ip->height);
      gtk_container_add (GTK_CONTAINER (pframe), preview);

      gtk_widget_show (preview);
      gtk_widget_show (pframe);
      gtk_widget_show (abox);
      gtk_widget_show (frame);
    }

  for (j = 0; j < 2; j++)
    {
      frame = gtk_frame_new ((j == 0) ?
			     _("Source color range") :
			     _("Destination color range"));
      gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame,
			  FALSE, FALSE, 0);
      gtk_widget_show (frame);

      /* The table keeps the color selections */
      table = gtk_table_new (1, 4, FALSE);
      gtk_table_set_row_spacings (GTK_TABLE (table), 4);
      gtk_table_set_col_spacings (GTK_TABLE (table), 4);
      gtk_table_set_col_spacing (GTK_TABLE (table), 1, 6);
      gtk_container_set_border_width (GTK_CONTAINER (table), 4);
      gtk_container_add (GTK_CONTAINER (frame), table);
      gtk_widget_show (table);

      add_color_button (j * 2, 0, 0, table);
      add_color_button (j * 2 + 1, 2, 0, table);
    }

  update_img_preview ();

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}

static void
color_button_color_changed_callback (GtkWidget *widget,
                                     gpointer   data)
{
 update_img_preview ();
}


static void
add_color_button (gint       csel_index,
                  gint       left,
                  gint       top,
                  GtkWidget *table)

{
  GtkWidget *label;
  GtkWidget *button;

  label = gtk_label_new ((left == 0) ? _("From:") : _("To:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, left, left+1, top, top+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  button = gimp_color_button_new (gettext (csel_title[csel_index]),
				  PRV_WIDTH, PRV_HEIGHT,
				  plvals.colors[csel_index], 3);
  gtk_signal_connect (GTK_OBJECT (button), "color_changed",
                      GTK_SIGNAL_FUNC (color_button_color_changed_callback),
                      NULL);
  gtk_table_attach (GTK_TABLE (table), button, left+1, left+2, top, top+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (button);
}


static void
mapcolor_ok_callback (GtkWidget *widget,
                      gpointer   data)

{
  plvals.map_mode = 0;  /* Currently always linear mapping */

  run_flag = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));

  img_preview_free (plinterface.img_preview);
  img_preview_free (plinterface.map_preview);
}

static void
get_mapping (guchar *src_col1,
             guchar *src_col2,
             guchar *dst_col1,
             guchar *dst_col2,
             gint32  map_mode,
             guchar *redmap,
             guchar *greenmap,
             guchar *bluemap)

{
  int rgb, i, j, a, as, b, bs;
  guchar *colormap[3];

  /* Currently we always do a linear mapping */

  colormap[0] = redmap;
  colormap[1] = greenmap;
  colormap[2] = bluemap;

  switch (map_mode)
    {
    case 0:
    default:
      for (rgb = 0; rgb < 3; rgb++)
	{
	  a = src_col1[rgb];  as = dst_col1[rgb];
	  b = src_col2[rgb];  bs = dst_col2[rgb];
          if (b == a) b = a+1;
	  for (i = 0; i < 256; i++)
	    {
	      j = ((i - a) * (bs - as)) / (b - a) + as;
	      if (j > 255) j = 255; else if (j < 0) j = 0;
	      colormap[rgb][i] = j;
	    }
	}
      break;
    }
}

static void
color_mapping (GDrawable *drawable)

{
  int processed, total;
  gint x, y, xmin, xmax, ymin, ymax, bpp = (gint)drawable->bpp;
  unsigned char *src, *dest;
  GPixelRgn src_rgn, dest_rgn;
  gpointer pr;
  double progress;
  unsigned char redmap[256], greenmap[256], bluemap[256];
  unsigned char *src_col1 = plvals.colors[0];
  unsigned char *src_col2 = plvals.colors[1];
  unsigned char *dst_col1 = plvals.colors[2];
  unsigned char *dst_col2 = plvals.colors[3];

  if (   (src_col1[0] == src_col2[0])
      || (src_col1[1] == src_col2[1])
      || (src_col1[2] == src_col2[2])) return;

  if (!gimp_drawable_is_rgb (drawable->id))
    {
      g_message (_("Color Mapping / Adjust FG/BG:\nCannot operate on gray/indexed images"));
      return;
    }
  
  gimp_drawable_mask_bounds (drawable->id, &xmin, &ymin, &xmax, &ymax);
  if ((ymin == ymax) || (xmin == xmax)) return;
  total = (xmax - xmin) * (ymax - ymin);

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
  gimp_pixel_rgn_init (&src_rgn, drawable, xmin, ymin,
                       (xmax - xmin), (ymax - ymin), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, xmin, ymin,
                       (xmax - xmin), (ymax - ymin), TRUE, TRUE);

  pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);

  get_mapping (src_col1, src_col2, dst_col1, dst_col2, plvals.map_mode,
               redmap, greenmap, bluemap);

  processed = 0;
  progress = 0.0;
  for (; pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      for (y = 0; y < src_rgn.h; y++)
	{
	  src = src_rgn.data + y * src_rgn.rowstride;
	  dest = dest_rgn.data + y * dest_rgn.rowstride;
	  for (x = 0; x < src_rgn.w; x++)
	    {
	      dest[0] = redmap[src[0]];
	      dest[1] = greenmap[src[1]];
	      dest[2] = bluemap[src[2]];
	      if (bpp > 3) dest[3] = src[3];
	      src += bpp;
	      dest += bpp;
	      processed++;
	    }
	}
      if (l_run_mode != RUN_NONINTERACTIVE)
	{
	  if ((double)processed/(double)total - progress > 0.1)
	    {
	      progress = (double)processed/(double)total;
	      gimp_progress_update (progress);
	    }
	}
    }
  if (l_run_mode != RUN_NONINTERACTIVE)
    gimp_progress_update (1.0);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, xmin, ymin, (xmax - xmin), (ymax - ymin));
}
