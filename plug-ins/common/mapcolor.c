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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PRV_WIDTH  50
#define PRV_HEIGHT 20

typedef struct
{
  GimpRGB colors[4];
  gint32  map_mode;
} PluginValues;

PluginValues plvals =
{
  {
    { 0.0, 0.0, 0.0, 1.0 },
    { 1.0, 1.0, 1.0, 1.0 },
    { 0.0, 0.0, 0.0, 1.0 },
    { 1.0, 1.0, 1.0, 1.0 },
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

static guchar redmap[256], greenmap[256], bluemap[256];

/* Declare some local functions.
 */
static void   query (void);
static void   run   (const gchar      *name,
		     gint              nparams,
		     const GimpParam  *param,
		     gint             *nreturn_vals,
		     GimpParam       **return_vals);

GimpPlugInInfo PLUG_IN_INFO =
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

static gboolean        dialog         (gint32     drawable_ID);
static void   get_mapping             (GimpRGB   *src_col1,
				       GimpRGB   *src_col2,
				       GimpRGB   *dst_col1,
				       GimpRGB   *dst_col2,
				       gint32     map_mode,
				       guchar    *redmap,
				       guchar    *greenmap,
				       guchar    *bluemap);
static void   add_color_button        (gint       csel_index,
				       gint       left,
				       gint       top,
				       GtkWidget *table);

static void   color_mapping           (GimpDrawable *drawable);


/* The run mode */
static GimpRunMode l_run_mode;

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
img_preview_alloc (guint width,
		   guint height)
{
  IMG_PREVIEW *ip;

  ip = g_new (IMG_PREVIEW, 1);
  ip->img = g_new (guchar, width * height*3);
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
      g_free (ip->img);
      g_free (ip);
    }
}

/* Copy image preview. Create/modify destination preview */
static void
img_preview_copy (IMG_PREVIEW  *src,
		  IMG_PREVIEW **dst)

{
  gint numbytes;
  IMG_PREVIEW *dst_p;

  if ((src == NULL) || (src->img == NULL) || (dst == NULL)) return;

  numbytes = src->width * src->height * 3; /* 1 byte spare */
  if (numbytes <= 0) return;

  if (*dst == NULL)  /* Create new preview ? */
    {
      *dst = img_preview_alloc (src->width, src->height);
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
    }
  dst_p->width = src->width;
  dst_p->height = src->height;
  memcpy (dst_p->img, src->img, numbytes);
}

static IMG_PREVIEW *
img_preview_create_from_drawable (guint  maxsize,
				  gint32 drawable_ID)
{
 GimpDrawable *drw;
 GimpPixelRgn pixel_rgn;
 guint drw_width, drw_height;
 guint prv_width, prv_height;
 gint  src_x, src_y, x, y;
 guchar *prv_data, *img_data, *cu_row;
 gdouble xfactor, yfactor;
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
  static GimpParamDef adjust_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (not used)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable to adjust" }
  };

  static GimpParamDef map_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image (not used)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable where colors are to map" },
    { GIMP_PDB_COLOR, "srccolor_1", "First source color" },
    { GIMP_PDB_COLOR, "srccolor_2", "Second source color" },
    { GIMP_PDB_COLOR, "dstcolor_1", "First destination color" },
    { GIMP_PDB_COLOR, "dstcolor_2", "Second destination color" },
    { GIMP_PDB_INT32, "map_mode", "Mapping mode (0: linear, others reserved)" }
  };

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
                          N_("Adjust _FG-BG"),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (adjust_args), 0,
                          adjust_args, NULL);

  gimp_plugin_menu_register ("plug_in_color_adjust",
                             N_("<Image>/Filters/Colors/Map"));

  gimp_install_procedure ("plug_in_color_map",
                          "Map color range specified by two colors"
			  "to color range specified by two other color.",
                          "Map color range specified by two colors"
			  "to color range specified by two other color."
                          "Intermediate colors are interpolated.",
                          "Peter Kirchgessner",
                          "Peter Kirchgessner",
                          dversio,
                          N_("Color Range _Mapping..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (map_args), 0,
                          map_args, NULL);

  gimp_plugin_menu_register ("plug_in_color_map",
                             N_("<Image>/Filters/Colors/Map"));
}


static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)

{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  GimpDrawable      *drawable = NULL;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  guchar            *c;
  gint               j;

  c = (guchar *) ident;

  INIT_I18N ();

  l_run_mode = run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  while (status == GIMP_PDB_SUCCESS)
    {
      if (nparams < 3)
	{
	  status = GIMP_PDB_CALLING_ERROR;
	  break;
	}

      /* Make sure the drawable is RGB color */
      drawable = gimp_drawable_get (param[2].data.d_drawable);
      if (!gimp_drawable_is_rgb (drawable->drawable_id))
	{
	  g_message (_("Cannot operate on gray or indexed color images."));
	  status = GIMP_PDB_EXECUTION_ERROR;
	  break;
	}

      if (strcmp (name, "plug_in_color_adjust") == 0)
	{
	  if (nparams != 3)  /* Make sure all the arguments are there */
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	      break;
	    }

	  gimp_palette_get_foreground (plvals.colors);
	  gimp_palette_get_background (plvals.colors + 1);

	  gimp_rgb_set (plvals.colors + 2, 0.0, 0.0, 0.0);
	  gimp_rgb_set (plvals.colors + 3, 1.0, 1.0, 1.0);

	  plvals.map_mode = 0;

          gimp_progress_init (_("Adjusting Foreground/Background..."));

	  color_mapping (drawable);
	  break;
	}

      if (strcmp (name, "plug_in_color_map") == 0)
	{
	  if (run_mode == GIMP_RUN_NONINTERACTIVE)
	    {
	      if (nparams != 8)  /* Make sure all the arguments are there */
		{
		  status = GIMP_PDB_CALLING_ERROR;
		  break;
		}

	      for (j = 0; j < 4; j++)
		{
		  plvals.colors[j] = param[3+j].data.d_color;
		}
	      plvals.map_mode = param[7].data.d_int32;
	    }
	  else if (run_mode == GIMP_RUN_INTERACTIVE)
	    {
	      gimp_get_data (name, &plvals);

	      gimp_palette_get_foreground (plvals.colors);
	      gimp_palette_get_background (plvals.colors + 1);

	      if (!dialog (param[2].data.d_drawable))
		break;
	    }
	  else if (run_mode == GIMP_RUN_WITH_LAST_VALS)
	    {
	      gimp_get_data (name, &plvals);
	    }
	  else
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	      break;
	    }

          gimp_progress_init (_("Mapping colors..."));

	  color_mapping (drawable);

	  if (run_mode == GIMP_RUN_INTERACTIVE)
	    gimp_set_data (name, &plvals, sizeof (plvals));

	  break;
	}

      status = GIMP_PDB_EXECUTION_ERROR;
    }

  if ((status == GIMP_PDB_SUCCESS) && (run_mode != GIMP_RUN_NONINTERACTIVE))
    gimp_displays_flush ();

  if (drawable != NULL) gimp_drawable_detach (drawable);
  values[0].data.d_status = status;
}

static void
update_img_preview (void)

{
  IMG_PREVIEW *dst_ip = plinterface.map_preview;
  IMG_PREVIEW *src_ip = plinterface.img_preview;
  GtkWidget *preview = plinterface.preview;
  guchar redmap[256], greenmap[256], bluemap[256];
  guchar *src, *dst;
  gint j;

  if ((dst_ip == NULL) || (src_ip == NULL)) return;

  get_mapping (plvals.colors,
	       plvals.colors + 1,
	       plvals.colors + 2,
	       plvals.colors + 3,
	       plvals.map_mode,
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
    gtk_preview_draw_row (GTK_PREVIEW (preview),
			  dst_ip->img + dst_ip->width*3*j,
			  0, j, dst_ip->width);

  gtk_widget_queue_draw (preview);
}

static gboolean
dialog (gint32 drawable_ID)
{
  GtkWidget   *dlg;
  GtkWidget   *frame, *pframe;
  GtkWidget   *abox;
  GtkWidget   *table;
  GtkWidget   *preview;
  IMG_PREVIEW *ip;
  gint         j;
  gboolean     run;

  gimp_ui_init ("mapcolor", TRUE);

  memset (&plinterface, 0, sizeof (plinterface));

  dlg = gimp_dialog_new (_("Map Color Range"), "mapcolor",
                         NULL, 0,
			 gimp_standard_help_func, "plug-in-color-map",

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

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

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  if (run)
    plvals.map_mode = 0;  /* Currently always linear mapping */

  gtk_widget_destroy (dlg);

  img_preview_free (plinterface.img_preview);
  img_preview_free (plinterface.map_preview);

  return run;
}

static void
add_color_button (gint       csel_index,
                  gint       left,
                  gint       top,
                  GtkWidget *table)
{
  GtkWidget *button;

  button = gimp_color_button_new (gettext (csel_title[csel_index]),
				  PRV_WIDTH, PRV_HEIGHT,
				  &plvals.colors[csel_index],
				  GIMP_COLOR_AREA_FLAT);
  gimp_table_attach_aligned (GTK_TABLE (table), left + 1, top,
			     (left == 0) ? _("From:") : _("To:"),
			     1.0, 0.5,
			     button, 1, TRUE);

  g_signal_connect (button, "color_changed",
                    G_CALLBACK (gimp_color_button_get_color),
                    &plvals.colors[csel_index]);
  g_signal_connect (button, "color_changed",
                    G_CALLBACK (update_img_preview),
                    NULL);
}

static void
get_mapping (GimpRGB *src_col1,
             GimpRGB *src_col2,
             GimpRGB *dst_col1,
             GimpRGB *dst_col2,
             gint32   map_mode,
             guchar  *redmap,
             guchar  *greenmap,
             guchar  *bluemap)
{
  guchar  src1[3];
  guchar  src2[3];
  guchar  dst1[3];
  guchar  dst2[3];
  gint    rgb, i, a, as, b, bs;
  guchar *colormap[3];

  /* Currently we always do a linear mapping */

  colormap[0] = redmap;
  colormap[1] = greenmap;
  colormap[2] = bluemap;

  gimp_rgb_get_uchar (src_col1, src1, src1 + 1, src1 + 2);
  gimp_rgb_get_uchar (src_col2, src2, src2 + 1, src2 + 2);
  gimp_rgb_get_uchar (dst_col1, dst1, dst1 + 1, dst1 + 2);
  gimp_rgb_get_uchar (dst_col2, dst2, dst2 + 1, dst2 + 2);

  switch (map_mode)
    {
    case 0:
    default:
      for (rgb = 0; rgb < 3; rgb++)
	{
	  a = src1[rgb];  as = dst1[rgb];
	  b = src2[rgb];  bs = dst2[rgb];

          if (b == a)
	    b = a + 1;
	  for (i = 0; i < 256; i++)
            {
	      gint j = ((i - a) * (bs - as)) / (b - a) + as;
              colormap[rgb][i] = CLAMP0255(j);
            }
	}
      break;
    }
}

static void
mapcolor_func (const guchar *src,
	       guchar       *dest,
	       gint          bpp,
	       gpointer      data)
{
  dest[0] = redmap[src[0]];
  dest[1] = greenmap[src[1]];
  dest[2] = bluemap[src[2]];
  if (bpp > 3)
    dest[3] = src[3];
}

static void
color_mapping (GimpDrawable *drawable)

{
  if (gimp_rgb_distance (&plvals.colors[0], &plvals.colors[1]) < 0.0001)
    return;

  if (!gimp_drawable_is_rgb (drawable->drawable_id))
    {
      g_message (_("Cannot operate on gray or indexed color images."));
      return;
    }

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));

  get_mapping (plvals.colors,
	       plvals.colors + 1,
	       plvals.colors + 2,
	       plvals.colors + 3,
	       plvals.map_mode,
               redmap, greenmap, bluemap);

  gimp_rgn_iterate2 (drawable, l_run_mode, mapcolor_func, NULL);
}
