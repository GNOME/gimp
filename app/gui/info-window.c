/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "appenv.h"
#include "actionarea.h"
#include "canvas.h"
#include "colormaps.h"
#include "info_dialog.h"
#include "info_window.h"
#include "gdisplay.h"
#include "general.h"
#include "gximage.h"
#include "interface.h"
#include "tag.h"

#define MAX_BUF 256

typedef struct _InfoWinData InfoWinData;
struct _InfoWinData
{
  char dimensions_str[MAX_BUF];
  char scale_str[MAX_BUF];
  char color_type_str[MAX_BUF];
  char visual_class_str[MAX_BUF];
  char visual_depth_str[MAX_BUF];
  char shades_str[MAX_BUF];
  char xy_str[MAX_BUF];
  char color_str[MAX_BUF];
  char color_str2[MAX_BUF];
  char color_str3[MAX_BUF];
};

/*  The different classes of visuals  */
static char *visual_classes[] =
{
  "Static Gray",
  "Grayscale",
  "Static Color",
  "Pseudo Color",
  "True Color",
  "Direct Color",
};


static void
get_shades (GDisplay *gdisp,
	    char     *buf)
{
  GtkPreviewInfo *info;

  info = gtk_preview_get_info ();

  switch (gimage_base_type (gdisp->gimage))
    {
    case GRAY:
      sprintf (buf, "%d", info->ngray_shades);
      break;
    case RGB:
      switch (gdisp->depth)
	{
	case 8 :
	  sprintf (buf, "%d / %d / %d",
		   info->nred_shades,
		   info->ngreen_shades,
		   info->nblue_shades);
	  break;
	case 15 : case 16 :
	  sprintf (buf, "%d / %d / %d",
		   (1 << (8 - info->visual->red_prec)),
		   (1 << (8 - info->visual->green_prec)),
		   (1 << (8 - info->visual->blue_prec)));
	  break;
	case 24 :
	  sprintf (buf, "256 / 256 / 256");
	  break;
	}
      break;

    case INDEXED:
      sprintf (buf, "%d", gdisp->gimage->num_cols);
      break;
    }
}

static void
info_window_close_callback (GtkWidget *w,
			    gpointer   client_data)
{
  info_dialog_popdown ((InfoDialog *) client_data);
}

  /*  displays information:
   *    image name
   *    image width, height
   *    zoom ratio
   *    image color type
   *    Display info:
   *      visual class
   *      visual depth
   *      shades of color/gray
   */

static ActionAreaItem action_items[] =
{
  { "Close", info_window_close_callback, NULL, NULL },
};

InfoDialog *
info_window_create (void *gdisp_ptr)
{
  InfoDialog *info_win;
  GDisplay *gdisp;
  InfoWinData *iwd;
  char * title, * title_buf;
  int type;

  gdisp = (GDisplay *) gdisp_ptr;

  title = prune_filename (gimage_filename (gdisp->gimage));
  type = gimage_base_type (gdisp->gimage);

  /*  allocate the title buffer  */
  title_buf = (char *) g_malloc (sizeof (char) * (strlen (title) + 15));
  sprintf (title_buf, "%s: Window Info", title);

  /*  create the info dialog  */
  info_win = info_dialog_new (title_buf);
  g_free (title_buf);

  iwd = (InfoWinData *) g_malloc (sizeof (InfoWinData));
  info_win->user_data = iwd;
  iwd->dimensions_str[0] = '\0';
  iwd->scale_str[0] = '\0';
  iwd->color_type_str[0] = '\0';
  iwd->visual_class_str[0] = '\0';
  iwd->visual_depth_str[0] = '\0';
  iwd->shades_str[0] = '\0';
  iwd->xy_str[0] = '\0';
  iwd->color_str[0] = '\0';
  iwd->color_str2[0] = '\0';
  iwd->color_str2[0] = '\0';

  /*  add the information fields  */
  info_dialog_add_field (info_win, "Dimensions (w x h): ", iwd->dimensions_str);
  info_dialog_add_field (info_win, "Scale Ratio: ", iwd->scale_str);
  info_dialog_add_field (info_win, "Display Type: ", iwd->color_type_str);
  info_dialog_add_field (info_win, "Visual Class: ", iwd->visual_class_str);
  info_dialog_add_field (info_win, "Visual Depth: ", iwd->visual_depth_str);
  if (type == RGB)
    info_dialog_add_field (info_win, "Shades of Color: ", iwd->shades_str);
  else if (type == INDEXED)
    info_dialog_add_field (info_win, "Shades: ", iwd->shades_str);
  else if (type == GRAY)
    info_dialog_add_field (info_win, "Shades of Gray: ", iwd->shades_str);

  /* color and position fields */ 
  info_dialog_add_field (info_win, "XY: ", iwd->xy_str);
  info_dialog_add_field (info_win, "Color: ", iwd->color_str);
  info_dialog_add_field (info_win, "Color: ", iwd->color_str2);
  info_dialog_add_field (info_win, "Color: ", iwd->color_str3);
  
  /*  update the fields  */
  info_window_update (info_win, gdisp_ptr);

  /* Create the action area  */
  action_items[0].user_data = info_win;
  build_action_area (GTK_DIALOG (info_win->shell), action_items, 1, 0);

  return info_win;
}

void
info_window_free (InfoDialog *info_win)
{
  g_free (info_win->user_data);
  info_dialog_free (info_win);
}

void
info_window_update (InfoDialog *info_win,
		    void       *gdisp_ptr)
{
  GDisplay *gdisp;
  InfoWinData *iwd;
  int type;
  int flat;

  gdisp = (GDisplay *) gdisp_ptr;
  iwd = (InfoWinData *) info_win->user_data;

  /*  width and height  */
  sprintf (iwd->dimensions_str, "%d x %d",
	   (int) gdisp->gimage->width, (int) gdisp->gimage->height);

  /*  zoom ratio  */
  sprintf (iwd->scale_str, "%d:%d",
	   SCALEDEST (gdisp), SCALESRC (gdisp));

  type = gimage_base_type (gdisp->gimage);
  flat = gimage_is_flat (gdisp->gimage);

  /*  color type  */
  if (type == RGB && flat)
    sprintf (iwd->color_type_str, "%s", "RGB Color");
  else if (type == GRAY && flat)
    sprintf (iwd->color_type_str, "%s", "Grayscale");
  else if (type == INDEXED && flat)
    sprintf (iwd->color_type_str, "%s", "Indexed Color");
  if (type == RGB && !flat)
    sprintf (iwd->color_type_str, "%s", "RGB-alpha Color");
  else if (type == GRAY && !flat)
    sprintf (iwd->color_type_str, "%s", "Grayscale-alpha");
  else if (type == INDEXED && !flat)
    sprintf (iwd->color_type_str, "%s", "Indexed-alpha Color");

  /*  visual class  */
  if (type == RGB ||
      type == INDEXED)
    sprintf (iwd->visual_class_str, "%s", visual_classes[g_visual->type]);
  else if (type == GRAY)
    sprintf (iwd->visual_class_str, "%s", visual_classes[g_visual->type]);

  /*  visual depth  */
  sprintf (iwd->visual_depth_str, "%d", gdisp->depth);

  /*  pure color shades  */
  get_shades (gdisp, iwd->shades_str);

  info_dialog_update (info_win);
}

static gint u8_to_u16( guint8 code )
{
  return ( (code/255.0) * 65535 + .5 );
}

static gfloat u8_to_float( guint8 code )
{
  return code/255.0;
}

static gint u16_to_u8( guint16 code )
{
  return ( (code/65535.0) * 255 + .5 );
}

static gfloat u16_to_float( guint16 code )
{
  return code/65535.0;
}

static gint float_to_u8( gfloat code )
{
  return (255 * code + .5);
}

static gint float_to_u16( gfloat code )
{
  return (65535 * code + .5);
}

static void setnull( char *c)
{
  int i;
  for( i = 0; i <= 30; i++)
	c[i] = '\0';
} 


void 
info_window_update_xy(
			InfoDialog *info_win,
		        void       *gdisp_ptr,
			int x,
			int y
			)
{
  GDisplay *gdisp;
  InfoWinData *iwd;
  int imagetype;
  int flat;
  int layertype;
  
  gdisp = (GDisplay *) gdisp_ptr;
  iwd = (InfoWinData *) info_win->user_data;

  /* color field */
  {
    Canvas *c = gimage_projection_canvas (gdisp->gimage) ;
    if (c)
    { 
      guchar * data;
      Tag t = canvas_tag (c);
      Alpha a = tag_alpha (t);
      Format f = tag_format (t);
      Precision p = tag_precision (t);

      canvas_portion_ref (c, x, y);
      data = canvas_portion_data (c, x, y); 
      canvas_portion_unref (c, x, y);
      setnull (iwd->color_str);
      setnull (iwd->color_str2);
      setnull (iwd->color_str3);
	 
      if (data) 
      {
      switch (p)
	{
	
	case PRECISION_U8:
	  {
	    guint8 *d = (guint8 *)data;
	    switch (f)
	      {
	      case FORMAT_RGB:
		  switch (a)
		    {
		    case ALPHA_YES:
		      sprintf (iwd->color_str, "%d, %d, %d, %d", 
				(int)d[0], 
				(int)d[1], 
				(int)d[2], 
				(int)d[3]);
		      sprintf (iwd->color_str2, "%d, %d, %d, %d", 
				u8_to_u16(d[0]), 
				u8_to_u16(d[1]),
				u8_to_u16(d[2]),
				u8_to_u16(d[3]));
		      sprintf (iwd->color_str3, " %f, %f, %f, %f", 
				u8_to_float(d[0]), 
				u8_to_float(d[1]),
				u8_to_float(d[2]),
				u8_to_float(d[3]));
		      break;
		    case ALPHA_NO:
		      sprintf (iwd->color_str, "%d, %d, %d", 
				(int)d[0], 
				(int)d[1], 
				(int)d[2]);
		      sprintf (iwd->color_str2, "%d, %d, %d", 
				u8_to_u16(d[0]), 
				u8_to_u16(d[1]),
				u8_to_u16(d[2]));
		      sprintf (iwd->color_str3, "%f, %f, %f", 
				u8_to_float(d[0]), 
				u8_to_float(d[1]),
				u8_to_float(d[2]));
		      break;
		    default:
		      break;
		    }
		  break;
	      case FORMAT_GRAY:
		  switch (a)
		    {
		    case ALPHA_YES:
	              sprintf (iwd->color_str, "%d, %d", 
				(int)d[0], 
				(int)d[1]); 
		      sprintf (iwd->color_str2, "%d, %d", 
				u8_to_u16(d[0]), 
				u8_to_u16(d[1]));
		      sprintf (iwd->color_str2, "%f, %f", 
				u8_to_float(d[0]), 
				u8_to_float(d[1]));
		      break;
		    case ALPHA_NO:
	              sprintf (iwd->color_str, "%d", 
				(int)d[0]); 
		      sprintf (iwd->color_str2, "%d", 
				u8_to_u16(d[0])); 
		      sprintf (iwd->color_str2, "%f", 
				u8_to_float(d[1]));
		      break;
		    default:
		      break;
		    }
		  break;
	      case FORMAT_INDEXED:      
		  switch (a)
		    {
		    case ALPHA_YES:
	              sprintf (iwd->color_str, "%d, %d", 
				(int)d[0], 
				(int)d[1]); 
		      break;
		    case ALPHA_NO:
	              sprintf (iwd->color_str, "%d", 
				(int)d[0]); 
		      break;
		    default:
		      break;
		    }
		  break;
	      default:
		break; 
	      } 
	  }
	  break;
	
	case PRECISION_U16:
	  {
	    guint16 *d = (guint16 *)data;
	    switch (f)
	      {
	      case FORMAT_RGB:
		  switch (a)
		    {
		    case ALPHA_YES:
		      sprintf (iwd->color_str, "%d, %d, %d, %d", 
				(int)d[0], 
				(int)d[1], 
				(int)d[2], 
				(int)d[3]);
		      sprintf (iwd->color_str2, "%d, %d, %d, %d", 
				u16_to_u8(d[0]), 
				u16_to_u8(d[1]),
				u16_to_u8(d[2]),
				u16_to_u8(d[3]));
		      sprintf (iwd->color_str3, "%f, %f, %f, %f", 
				u16_to_float(d[0]), 
				u16_to_float(d[1]),
				u16_to_float(d[2]),
				u16_to_float(d[3]));
		      break;
		    case ALPHA_NO:
		      sprintf (iwd->color_str, "%d, %d, %d", 
				(int)d[0], 
				(int)d[1], 
				(int)d[2]);
		      sprintf (iwd->color_str2, "%d, %d, %d", 
				u16_to_u8(d[0]), 
				u16_to_u8(d[1]),
				u16_to_u8(d[2]));
		      sprintf (iwd->color_str3, "%f, %f, %f", 
				u16_to_float(d[0]), 
				u16_to_float(d[1]),
				u16_to_float(d[2]));
		      break;
		    default:
		      break;
		    }
		  break;
	      case FORMAT_GRAY:
		  switch (a)
		    {
		    case ALPHA_YES:
	              sprintf (iwd->color_str, "%d, %d", 
				(int)d[0], 
				(int)d[1]); 
		      sprintf (iwd->color_str2, "%d, %d", 
				u16_to_u8(d[0]), 
				u16_to_u8(d[1]));
		      sprintf (iwd->color_str3, "%f, %f", 
				u16_to_float(d[0]), 
				u16_to_float(d[1]));
		      break;
		    case ALPHA_NO:
	              sprintf (iwd->color_str, "%d", 
				(int)d[0]); 
		      sprintf (iwd->color_str2, "%d", 
				u16_to_u8(d[0])); 
		      sprintf (iwd->color_str3, "%f", 
				u16_to_float(d[0])); 
		      break;
		    default:
		      break;
		    }
		  break;
	      case FORMAT_INDEXED:      
		  switch (a)
		    {
		    case ALPHA_YES:
	              sprintf (iwd->color_str, "%d, %d", 
				(int)d[0], 
				(int)d[1]); 
		      break;
		    case ALPHA_NO:
	              sprintf (iwd->color_str, "%d", 
				(int)d[0]); 
		      break;
		    default:
		      break;
		    }
		  break;
	      default:
		break; 
	      } 
	  }
	  break;
	
	case PRECISION_FLOAT:
	  {
	    gfloat *d = (gfloat *)data;
	    switch (f)
	      {
	      case FORMAT_RGB:
		  switch (a)
		    {
		    case ALPHA_YES:
		      sprintf (iwd->color_str, "%f, %f, %f, %f", 
				(gfloat)d[0], 
				(gfloat)d[1], 
				(gfloat)d[2], 
				(gfloat)d[3]);
		      sprintf (iwd->color_str2, "%d, %d, %d, %d", 
				float_to_u8(d[0]), 
				float_to_u8(d[1]),
				float_to_u8(d[2]),
				float_to_u8(d[3]));
		      sprintf (iwd->color_str3, "%d, %d, %d, %d", 
				float_to_u16(d[0]), 
				float_to_u16(d[1]),
				float_to_u16(d[2]),
				float_to_u16(d[3]));
		      break;
		    case ALPHA_NO:
		      sprintf (iwd->color_str, "%f, %f, %f", 
				(gfloat)d[0], 
				(gfloat)d[1], 
				(gfloat)d[2]);
		      sprintf (iwd->color_str2, "%d, %d, %d", 
				float_to_u8(d[0]), 
				float_to_u8(d[1]),
				float_to_u8(d[2]));
		      sprintf (iwd->color_str3, "%d, %d, %d", 
				float_to_u16(d[0]), 
				float_to_u16(d[1]),
				float_to_u16(d[2]));
		      break;
		    default:
		      break;
		    }
		  break;
	      case FORMAT_GRAY:
		  switch (a)
		    {
		    case ALPHA_YES:
	              sprintf (iwd->color_str, "%f, %f", 
				(gfloat)d[0], 
				(gfloat)d[1]); 
		      sprintf (iwd->color_str2, "%d, %d", 
				float_to_u8(d[0]), 
				float_to_u8(d[1]));
		      sprintf (iwd->color_str3, "%d, %d", 
				float_to_u16(d[0]), 
				float_to_u16(d[1]));
		      break;
		    case ALPHA_NO:
	              sprintf (iwd->color_str, "%f", 
				(gfloat)d[0]); 
		      sprintf (iwd->color_str2, "%d", 
				float_to_u8(d[0])); 
		      sprintf (iwd->color_str3, "%d", 
				float_to_u16(d[0])); 
		      break;
		    default:
		      break;
		    }
		  break;
	      default:
		break; 
	      } 
	  }
	  break;
	default:
	  break;
	}
      }
    }
  }    
 
  /* x, y field */
  sprintf( iwd->xy_str, "%d,%d", x, y);
  info_dialog_update (info_win);
}
