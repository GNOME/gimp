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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include "appenv.h"
#include "buildmenu.h"
#include "channels_dialog.h"
#include "colormaps.h"
#include "cursorutil.h"
#include "disp_callbacks.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gdisplayP.h"
#include "gdisplay_ops.h"
#include "general.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "gximage.h"
#include "image_render.h"
#include "info_window.h"
#include "interface.h"
#include "layers_dialog.h"
#include "menus.h"
#include "plug_in.h"
#include "scale.h"
#include "scroll.h"
#include "tools.h"
#include "undo.h"

#include "layer_pvt.h"			/* ick. */

#define OVERHEAD          25  /*  in units of pixel area  */
#define EPSILON           5

/* variable declarations */
GSList *               display_list = NULL;
static int             display_num  = 1;
static GdkCursorType   default_gdisplay_cursor = GDK_TOP_LEFT_ARROW;

#define ROUND(x) ((int) (x + 0.5))

#define MAX_TITLE_BUF 4096

static char *image_type_strs[] =
{
  "RGB",
  "RGB-alpha",
  "grayscale",
  "grayscale-alpha",
  "indexed",
  "indexed-alpha"
};


/*  Local functions  */
static void       gdisplay_format_title     (GImage *, char *);
static void       gdisplay_delete           (GDisplay *);
static GSList *   gdisplay_free_area_list   (GSList *);
static GSList *   gdisplay_process_area_list(GSList *, GArea *);
static void       gdisplay_add_update_area  (GDisplay *, int, int, int, int);
static void       gdisplay_add_display_area (GDisplay *, int, int, int, int);
static void       gdisplay_paint_area       (GDisplay *, int, int, int, int);
static void       gdisplay_display_area     (GDisplay *, int, int, int, int);
static guint      gdisplay_hash             (GDisplay *);

static GHashTable *display_ht = NULL;


GDisplay*
gdisplay_new (GImage       *gimage,
	      unsigned int  scale)
{
  GDisplay *gdisp;
  char title [MAX_TITLE_BUF];
  int instance;

  /*  If there isn't an interface, never create a gdisplay  */
  if (no_interface)
    return NULL;

  /* format the title */
  gdisplay_format_title (gimage, title);

  instance = gimage->instance_count;
  gimage->instance_count++;
  gimage->ref_count++;

  /*
   *  Set all GDisplay parameters...
   */
  gdisp = (GDisplay *) g_malloc (sizeof (GDisplay));

  gdisp->offset_x = gdisp->offset_y = 0;
  gdisp->scale = scale;
  gdisp->gimage = gimage;
  gdisp->window_info_dialog = NULL;
  gdisp->depth = g_visual->depth;
  gdisp->select = NULL;
  gdisp->ID = display_num++;
  gdisp->instance = instance;
  gdisp->update_areas = NULL;
  gdisp->display_areas = NULL;
  gdisp->disp_xoffset = 0;
  gdisp->disp_yoffset = 0;
  gdisp->current_cursor = -1;
  gdisp->draw_guides = TRUE;
  gdisp->snap_to_guides = TRUE;

  /*  add the new display to the list so that it isn't lost  */
  display_list = g_slist_append (display_list, (void *) gdisp);

  /*  create the shell for the image  */
  create_display_shell (gdisp->ID, gimage->width, gimage->height,
			title, gimage_base_type (gimage));

  /*  set the gdisplay colormap type and install the appropriate colormap  */
  gdisp->color_type = (gimage_base_type (gimage) == GRAY) ? GRAY : RGB;

  /*  set the user data  */
  if (!display_ht)
    display_ht = g_hash_table_new ((GHashFunc) gdisplay_hash, NULL);

  g_hash_table_insert (display_ht, gdisp->shell, gdisp);
  g_hash_table_insert (display_ht, gdisp->canvas, gdisp);

  /*  set the current tool cursor  */
  gdisplay_install_tool_cursor (gdisp, default_gdisplay_cursor);

  return gdisp;
}


static void
gdisplay_format_title (GImage *gimage,
		       char   *title)
{
  char *image_type_str;
  int empty;
  Tag t = gimage_tag (gimage);
  
  empty = gimage_is_empty (gimage);

  if (gimage_is_flat (gimage))
    image_type_str = image_type_strs[drawable_type (gimage_active_drawable (gimage))];
  else
    switch (gimage_base_type (gimage))
      {
      case RGB:
	image_type_str = (empty) ? "RGB-empty" : "RGB-layered";
	break;
      case GRAY:
	image_type_str = (empty) ? "grayscale-empty" : "grayscale-layered";
	break;
      case INDEXED:
	image_type_str = (empty) ? "indexed-empty" : "indexed-layered";
	break;
      default:
	image_type_str = NULL;
      }

  sprintf (title, "%s-%d.%d (%s - %s - %s)%s",
	   prune_filename (gimage_filename (gimage)),
	   gimage->ID, gimage->instance_count,
	   tag_string_precision (tag_precision (t)),
	   tag_string_format (tag_format (t)),
	   tag_string_alpha (tag_alpha (t)),
           empty ? " (empty)" : "");
}


static void
gdisplay_delete (GDisplay *gdisp)
{
  GDisplay *tool_gdisp;

  g_hash_table_remove (display_ht, gdisp->shell);
  g_hash_table_remove (display_ht, gdisp->canvas);

  /*  stop any active tool  */
  active_tool_control (HALT, (void *) gdisp);

  /*  clear out the pointer to this gdisp from the active tool */

  if (active_tool && active_tool->gdisp_ptr) {
    tool_gdisp = active_tool->gdisp_ptr;
    if (gdisp->ID == tool_gdisp->ID) {
      active_tool->drawable = NULL;
      active_tool->gdisp_ptr = NULL;
    }
  }

  /*  free the selection structure  */
  selection_free (gdisp->select);
  
  if (gdisp->scroll_gc)
    gdk_gc_destroy (gdisp->scroll_gc);

  /*  free the area lists  */
  gdisplay_free_area_list (gdisp->update_areas);
  gdisplay_free_area_list (gdisp->display_areas);

  /*  free the gimage  */
  gimage_delete (gdisp->gimage);

  /*  insure that if a window information dialog exists, it is removed  */
  if (gdisp->window_info_dialog)
    info_window_free (gdisp->window_info_dialog);

  /*  set popup_shell to NULL if appropriate */
  if (popup_shell == gdisp->shell)
    popup_shell= NULL;

  gtk_widget_unref (gdisp->shell);

  g_free (gdisp);
}


static GSList *
gdisplay_free_area_list (GSList *list)
{
  GSList *l = list;
  GArea *ga;

  while (l)
    {
      /*  free the data  */
      ga = (GArea *) l->data;
      g_free (ga);

      l = g_slist_next (l);
    }

  if (list)
    g_slist_free (list);

  return NULL;
}


static GSList *
gdisplay_process_area_list (GSList *list,
			    GArea  *ga1)
{
  GSList *new_list;
  GSList *l = list;
  int area1, area2, area3;
  GArea *ga2;

  /*  start new list off  */
  new_list = g_slist_prepend (NULL, ga1);
  while (l)
    {
      /*  process the data  */
      ga2 = (GArea *) l->data;
      area1 = (ga1->x2 - ga1->x1) * (ga1->y2 - ga1->y1) + OVERHEAD;
      area2 = (ga2->x2 - ga2->x1) * (ga2->y2 - ga2->y1) + OVERHEAD;
      area3 = (MAXIMUM (ga2->x2, ga1->x2) - MINIMUM (ga2->x1, ga1->x1)) *
	(MAXIMUM (ga2->y2, ga1->y2) - MINIMUM (ga2->y1, ga1->y1)) + OVERHEAD;

      if ((area1 + area2) < area3)
	new_list = g_slist_prepend (new_list, ga2);
      else
	{
	  ga1->x1 = MINIMUM (ga1->x1, ga2->x1);
	  ga1->y1 = MINIMUM (ga1->y1, ga2->y1);
	  ga1->x2 = MAXIMUM (ga1->x2, ga2->x2);
	  ga1->y2 = MAXIMUM (ga1->y2, ga2->y2);

	  g_free (ga2);
	}

      l = g_slist_next (l);
    }

  if (list)
    g_slist_free (list);

  return new_list;
}


void
gdisplay_flush (GDisplay *gdisp)
{
  GArea  *ga;
  GSList *list;

  /*  Flush the items in the displays and updates lists--
   *  but only if gdisplay has been mapped and exposed
   */
  if (!gdisp->select)
    return;

  /*  First the updates...  */
  list = gdisp->update_areas;
  while (list)
    {
      /*  Paint the area specified by the GArea  */
      ga = (GArea *) list->data;
      gdisplay_paint_area (gdisp, ga->x1, ga->y1,
			   (ga->x2 - ga->x1), (ga->y2 - ga->y1));

      list = g_slist_next (list);
    }
  /*  Free the update lists  */
  gdisp->update_areas = gdisplay_free_area_list (gdisp->update_areas);

  /*  Next the displays...  */
  list = gdisp->display_areas;

  if (list)
    {
      /*  stop the currently active tool  */
      active_tool_control (PAUSE, (void *) gdisp);

      while (list)
	{
	  /*  Paint the area specified by the GArea  */
	  ga = (GArea *) list->data;
	  gdisplay_display_area (gdisp, ga->x1, ga->y1,
				 (ga->x2 - ga->x1), (ga->y2 - ga->y1));

	  list = g_slist_next (list);
	}
      /*  Free the update lists  */
      gdisp->display_areas = gdisplay_free_area_list (gdisp->display_areas);

      /* draw the guides */
      gdisplay_draw_guides (gdisp);

      /* restart (and recalculate) the selection boundaries */
      selection_start (gdisp->select, TRUE);

      /* start the currently active tool */
      active_tool_control (RESUME, (void *) gdisp);
    }

  /*  update the gdisplay's info dialog  */
  if (gdisp->window_info_dialog)
    info_window_update (gdisp->window_info_dialog,
			(void *) gdisp);
}


void
gdisplay_draw_guides (GDisplay *gdisp)
{
  GList *tmp_list;
  Guide *guide;

  if (gdisp->draw_guides)
    {
      tmp_list = gdisp->gimage->guides;
      while (tmp_list)
	{
	  guide = tmp_list->data;
	  tmp_list = tmp_list->next;

	  gdisplay_draw_guide (gdisp, guide, FALSE);
	}
    }
}

void
gdisplay_draw_guide (GDisplay *gdisp,
		     Guide    *guide,
		     int       active)
{
  static GdkGC *normal_hgc = NULL;
  static GdkGC *active_hgc = NULL;
  static GdkGC *normal_vgc = NULL;
  static GdkGC *active_vgc = NULL;
  static int initialize = TRUE;
  int x1, x2;
  int y1, y2;
  int w, h;
  int x, y;

  if (guide->position < 0)
    return;

  if (initialize)
    {
      GdkGCValues values;
      char stipple[8] =
      {
	0xF0,    /*  ####----  */
	0xE1,    /*  ###----#  */
	0xC3,    /*  ##----##  */
	0x87,    /*  #----###  */
	0x0F,    /*  ----####  */
	0x1E,    /*  ---####-  */
	0x3C,    /*  --####--  */
	0x78,    /*  -####---  */
      };

      initialize = FALSE;

      values.foreground.pixel = gdisplay_black_pixel (gdisp);
      values.background.pixel = g_normal_guide_pixel;
      values.fill = GDK_OPAQUE_STIPPLED;
      values.stipple = gdk_bitmap_create_from_data (gdisp->canvas->window, (char*) stipple, 8, 1);
      normal_hgc = gdk_gc_new_with_values (gdisp->canvas->window, &values,
					  GDK_GC_FOREGROUND |
					  GDK_GC_BACKGROUND |
					  GDK_GC_FILL |
					  GDK_GC_STIPPLE);

      values.background.pixel = g_active_guide_pixel;
      active_hgc = gdk_gc_new_with_values (gdisp->canvas->window, &values,
					   GDK_GC_FOREGROUND |
					   GDK_GC_BACKGROUND |
					   GDK_GC_FILL |
					   GDK_GC_STIPPLE);

      values.foreground.pixel = gdisplay_black_pixel (gdisp);
      values.background.pixel = g_normal_guide_pixel;
      values.fill = GDK_OPAQUE_STIPPLED;
      values.stipple = gdk_bitmap_create_from_data (gdisp->canvas->window, (char*) stipple, 1, 8);
      normal_vgc = gdk_gc_new_with_values (gdisp->canvas->window, &values,
					  GDK_GC_FOREGROUND |
					  GDK_GC_BACKGROUND |
					  GDK_GC_FILL |
					  GDK_GC_STIPPLE);

      values.background.pixel = g_active_guide_pixel;
      active_vgc = gdk_gc_new_with_values (gdisp->canvas->window, &values,
					   GDK_GC_FOREGROUND |
					   GDK_GC_BACKGROUND |
					   GDK_GC_FILL |
					   GDK_GC_STIPPLE);
    }

  gdisplay_transform_coords (gdisp, 0, 0, &x1, &y1, FALSE);
  gdisplay_transform_coords (gdisp, gdisp->gimage->width, gdisp->gimage->height, &x2, &y2, FALSE);
  gdk_window_get_size (gdisp->canvas->window, &w, &h);

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 > w) x2 = w;
  if (y2 > h) y2 = h;

  if (guide->orientation == HORIZONTAL_GUIDE)
    {
      gdisplay_transform_coords (gdisp, 0, guide->position, &x, &y, FALSE);

      if (active)
	gdk_draw_line (gdisp->canvas->window, active_hgc, x1, y, x2, y);
      else
	gdk_draw_line (gdisp->canvas->window, normal_hgc, x1, y, x2, y);
    }
  else if (guide->orientation == VERTICAL_GUIDE)
    {
      gdisplay_transform_coords (gdisp, guide->position, 0, &x, &y, FALSE);

      if (active)
	gdk_draw_line (gdisp->canvas->window, active_vgc, x, y1, x, y2);
      else
	gdk_draw_line (gdisp->canvas->window, normal_vgc, x, y1, x, y2);
    }
}

Guide*
gdisplay_find_guide (GDisplay *gdisp,
		     int       x,
		     int       y)
{
  GList *tmp_list;
  Guide *guide;
  double scale;
  int offset_x, offset_y;
  int pos;

  if (gdisp->draw_guides)
    {
      offset_x = gdisp->offset_x - gdisp->disp_xoffset;
      offset_y = gdisp->offset_y - gdisp->disp_yoffset;
      scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) : 1.0 / SCALESRC (gdisp);

      tmp_list = gdisp->gimage->guides;
      while (tmp_list)
	{
	  guide = tmp_list->data;
	  tmp_list = tmp_list->next;

	  switch (guide->orientation)
	    {
	    case HORIZONTAL_GUIDE:
	      pos = (int) (scale * guide->position - offset_y);
	      if ((guide->position != -1) &&
		  (pos > (y - EPSILON)) &&
		  (pos < (y + EPSILON)))
		return guide;
	      break;
	    case VERTICAL_GUIDE:
	      pos = (int) (scale * guide->position - offset_x);
	      if ((guide->position != -1) &&
		  (pos > (x - EPSILON)) &&
		  (pos < (x + EPSILON)))
		return guide;
	      break;
	    }
	}
    }

  return NULL;
}

void
gdisplay_snap_point (GDisplay *gdisp,
		     int       x ,
		     int       y,
		     int      *tx,
		     int      *ty)
{
  GList *tmp_list;
  Guide *guide;
  double scale;
  int offset_x, offset_y;
  int minhdist, minvdist;
  int pos, dist;

  *tx = x;
  *ty = y;

  if (gdisp->draw_guides &&
      gdisp->snap_to_guides &&
      gdisp->gimage->guides)
    {
      offset_x = gdisp->offset_x - gdisp->disp_xoffset;
      offset_y = gdisp->offset_y - gdisp->disp_yoffset;
      scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) : 1.0 / SCALESRC (gdisp);

      minhdist = G_MAXINT;
      minvdist = G_MAXINT;

      tmp_list = gdisp->gimage->guides;
      while (tmp_list)
	{
	  guide = tmp_list->data;
	  tmp_list = tmp_list->next;

	  switch (guide->orientation)
	    {
	    case HORIZONTAL_GUIDE:
	      pos = (int) (scale * guide->position - offset_y);
	      if ((pos > (y - EPSILON)) &&
		  (pos < (y + EPSILON)))
		{
		  dist = pos - y;
		  dist = ABS (dist);

		  if (dist < minhdist)
		    {
		      minhdist = dist;
		      *ty = pos;
		    }
		}
	      break;
	    case VERTICAL_GUIDE:
	      pos = (int) (scale * guide->position - offset_x);
	      if ((pos > (x - EPSILON)) &&
		  (pos < (x + EPSILON)))
		{
		  dist = pos - x;
		  dist = ABS (dist);

		  if (dist < minvdist)
		    {
		      minvdist = dist;
		      *tx = pos;
		    }
		}
	      break;
	    }
	}
    }
}

void
gdisplay_snap_rectangle (GDisplay *gdisp,
			 int       x1,
			 int       y1,
			 int       x2,
			 int       y2,
			 int      *tx1,
			 int      *ty1)
{
  int nx1, ny1;
  int nx2, ny2;

  *tx1 = x1;
  *ty1 = y1;

  if (gdisp->draw_guides &&
      gdisp->snap_to_guides &&
      gdisp->gimage->guides)
    {
      gdisplay_snap_point (gdisp, x1, y1, &nx1, &ny1);
      gdisplay_snap_point (gdisp, x2, y2, &nx2, &ny2);

      if (x1 != nx1)
	*tx1 = nx1;
      else if (x2 != nx2)
	*tx1 = x1 + (nx2 - x2);

      if (y1 != ny1)
	*ty1 = ny1;
      else if (y2 != ny2)
	*ty1 = y1 + (ny2 - y2);
    }
}


void
gdisplay_remove_and_delete (GDisplay *gdisp)
{
  /* remove the display from the list */
  display_list = g_slist_remove (display_list, (void *) gdisp);
  gdisplay_delete (gdisp);
}


static void
gdisplay_add_update_area (GDisplay *gdisp,
			  int       x,
			  int       y,
			  int       w,
			  int       h)
{
  GArea * ga;

  ga = (GArea *) g_malloc (sizeof (GArea));
  ga->x1 = BOUNDS (x, 0, gdisp->gimage->width);
  ga->y1 = BOUNDS (y, 0, gdisp->gimage->height);
  ga->x2 = BOUNDS (x + w, 0, gdisp->gimage->width);
  ga->y2 = BOUNDS (y + h, 0, gdisp->gimage->height);

  gdisp->update_areas = gdisplay_process_area_list (gdisp->update_areas, ga);
}


static void
gdisplay_add_display_area (GDisplay *gdisp,
			   int       x,
			   int       y,
			   int       w,
			   int       h)
{
  GArea * ga;

  ga = (GArea *) g_malloc (sizeof (GArea));

  ga->x1 = BOUNDS (x, 0, gdisp->disp_width);
  ga->y1 = BOUNDS (y, 0, gdisp->disp_height);
  ga->x2 = BOUNDS (x + w, 0, gdisp->disp_width);
  ga->y2 = BOUNDS (y + h, 0, gdisp->disp_height);

  gdisp->display_areas = gdisplay_process_area_list (gdisp->display_areas, ga);
}


static void
gdisplay_paint_area (GDisplay *gdisp,
		     int       x,
		     int       y,
		     int       w,
		     int       h)
{
  int x1, y1, x2, y2;

  /*  Bounds check  */
  x1 = BOUNDS (x, 0, gdisp->gimage->width);
  y1 = BOUNDS (y, 0, gdisp->gimage->height);
  x2 = BOUNDS (x + w, 0, gdisp->gimage->width);
  y2 = BOUNDS (y + h, 0, gdisp->gimage->height);
  x = x1;
  y = y1;
  w = (x2 - x1);
  h = (y2 - y1);

  /*  calculate the extents of the update as limited by what's visible  */
  gdisplay_untransform_coords (gdisp, 0, 0, &x1, &y1, FALSE, FALSE);
  gdisplay_untransform_coords (gdisp, gdisp->disp_width, gdisp->disp_height, &x2, &y2, FALSE, FALSE);

  gimage_invalidate (gdisp->gimage, x, y, w, h, x1, y1, x2, y2);

    /*  display the area  */
  gdisplay_transform_coords (gdisp, x, y, &x1, &y1, FALSE);
  gdisplay_transform_coords (gdisp, x + w, y + h, &x2, &y2, FALSE);
  gdisplay_expose_area (gdisp, x1, y1, (x2 - x1), (y2 - y1));
}


static void
gdisplay_display_area (GDisplay *gdisp,
		       int       x,
		       int       y,
		       int       w,
		       int       h)
{
  int sx, sy;
  int x1, y1;
  int x2, y2;
  int dx, dy;
  int i, j;

  sx = SCALE (gdisp, gdisp->gimage->width);
  sy = SCALE (gdisp, gdisp->gimage->height);

  /*  Bounds check  */
  x1 = BOUNDS (x, 0, gdisp->disp_width);
  y1 = BOUNDS (y, 0, gdisp->disp_height);
  x2 = BOUNDS (x + w, 0, gdisp->disp_width);
  y2 = BOUNDS (y + h, 0, gdisp->disp_height);

  if (x1 < gdisp->disp_xoffset)
    {
      gdk_window_clear_area (gdisp->canvas->window,
			     x, y, gdisp->disp_xoffset - x, h);

      x1 = gdisp->disp_xoffset;
    }

  if (y1 < gdisp->disp_yoffset)
    {
      gdk_window_clear_area (gdisp->canvas->window,
			     x, y, w, gdisp->disp_yoffset - y);

      y1 = gdisp->disp_yoffset;
    }

  if (x2 > (gdisp->disp_xoffset + sx))
    {
      gdk_window_clear_area (gdisp->canvas->window,
			     gdisp->disp_xoffset + sx, y,
			     x2 - (gdisp->disp_xoffset + sx), h);

      x2 = gdisp->disp_xoffset + sx;
    }

  if (y2 > (gdisp->disp_yoffset + sy))
    {
      gdk_window_clear_area (gdisp->canvas->window,
			     x, gdisp->disp_yoffset + sy,
			     w, y2 - (gdisp->disp_yoffset + sy));

      y2 = gdisp->disp_yoffset + sy;
    }

  /*  display the image in GXIMAGE_WIDTH x GXIMAGE_HEIGHT sized chunks */
  for (i = y1; i < y2; i += GXIMAGE_HEIGHT)
    for (j = x1; j < x2; j += GXIMAGE_WIDTH)
      {
	dx = (x2 - j < GXIMAGE_WIDTH) ? x2 - j : GXIMAGE_WIDTH;
	dy = (y2 - i < GXIMAGE_HEIGHT) ? y2 - i : GXIMAGE_HEIGHT;
	render_image (gdisp, j - gdisp->disp_xoffset, i - gdisp->disp_yoffset, dx, dy);
	gximage_put (gdisp->canvas->window,
		     j, i, dx, dy);
      }
}


int
gdisplay_mask_value (GDisplay *gdisp,
		     int       x,
		     int       y)
{
  /*  move the coordinates from screen space to image space  */
  gdisplay_untransform_coords (gdisp, x, y, &x, &y, FALSE, 0);

  return gimage_mask_value (gdisp->gimage, x, y);
}


int
gdisplay_mask_bounds (GDisplay *gdisp,
		      int      *x1,
		      int      *y1,
		      int      *x2,
		      int      *y2)
{
  Layer *layer;
  int off_x, off_y;

  /*  If there is a floating selection, handle things differently  */
  if ((layer = gimage_floating_sel (gdisp->gimage)))
    {
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      if (! channel_bounds (gimage_get_mask (gdisp->gimage), x1, y1, x2, y2))
	{
	  *x1 = off_x;
	  *y1 = off_y;
	  *x2 = off_x + drawable_width (GIMP_DRAWABLE(layer));
	  *y2 = off_y + drawable_height (GIMP_DRAWABLE(layer));
	}
      else
	{
	  *x1 = MINIMUM (off_x, *x1);
	  *y1 = MINIMUM (off_y, *y1);
	  *x2 = MAXIMUM (off_x + drawable_width (GIMP_DRAWABLE(layer)), *x2);
	  *y2 = MAXIMUM (off_y + drawable_height (GIMP_DRAWABLE(layer)), *y2);
	}
    }
  else if (! channel_bounds (gimage_get_mask (gdisp->gimage), x1, y1, x2, y2))
    return FALSE;

  gdisplay_transform_coords (gdisp, *x1, *y1, x1, y1, 0);
  gdisplay_transform_coords (gdisp, *x2, *y2, x2, y2, 0);

  /*  Make sure the extents are within bounds  */
  *x1 = BOUNDS (*x1, 0, gdisp->disp_width);
  *y1 = BOUNDS (*y1, 0, gdisp->disp_height);
  *x2 = BOUNDS (*x2, 0, gdisp->disp_width);
  *y2 = BOUNDS (*y2, 0, gdisp->disp_height);

  return TRUE;
}


void
gdisplay_transform_coords (GDisplay *gdisp,
			   int       x,
			   int       y,
			   int      *nx,
			   int      *ny,
			   int       use_offsets)
{
  double scale;
  int offset_x, offset_y;

  /*  transform from image coordinates to screen coordinates  */
  scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) :
    1.0 / SCALESRC (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = (int) (scale * (x + offset_x) - gdisp->offset_x);
  *ny = (int) (scale * (y + offset_y) - gdisp->offset_y);

  *nx += gdisp->disp_xoffset;
  *ny += gdisp->disp_yoffset;
}


void
gdisplay_untransform_coords (GDisplay *gdisp,
			     int       x,
			     int       y,
			     int      *nx,
			     int      *ny,
			     int       round,
			     int       use_offsets)
{
  double scale;
  int offset_x, offset_y;

  x -= gdisp->disp_xoffset;
  y -= gdisp->disp_yoffset;

  /*  transform from screen coordinates to image coordinates  */
  scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) :
    1.0 / SCALESRC (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  if (round)
    {
      *nx = ROUND ((x + gdisp->offset_x) / scale - offset_x);
      *ny = ROUND ((y + gdisp->offset_y) / scale - offset_y);
    }
  else
    {
      *nx = (int) ((x + gdisp->offset_x) / scale - offset_x);
      *ny = (int) ((y + gdisp->offset_y) / scale - offset_y);
    }
}


void
gdisplay_transform_coords_f (GDisplay *gdisp,
			     double    x,
			     double    y,
			     double   *nx,
			     double   *ny,
			     int       use_offsets)
{
  double scale;
  int offset_x, offset_y;

  /*  transform from gimp coordinates to screen coordinates  */
  scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) :
    1.0 / SCALESRC (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = scale * (x + offset_x) - gdisp->offset_x;
  *ny = scale * (y + offset_y) - gdisp->offset_y;

  *nx += gdisp->disp_xoffset;
  *ny += gdisp->disp_yoffset;
}


void
gdisplay_untransform_coords_f (GDisplay *gdisp,
			       double    x,
			       double    y,
			       double   *nx,
			       double   *ny,
			       int       use_offsets)
{
  double scale;
  int offset_x, offset_y;

  x -= gdisp->disp_xoffset;
  y -= gdisp->disp_yoffset;

  /*  transform from screen coordinates to gimp coordinates  */
  scale = (SCALESRC (gdisp) == 1) ? SCALEDEST (gdisp) :
    1.0 / SCALESRC (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = (x + gdisp->offset_x) / scale - offset_x;
  *ny = (y + gdisp->offset_y) / scale - offset_y;
}


/*  install and remove tool cursor from gdisplay...  */
void
gdisplay_install_tool_cursor (GDisplay      *gdisp,
			      GdkCursorType  cursor_type)
{
  if (gdisp->current_cursor != cursor_type)
    {
      gdisp->current_cursor = cursor_type;
      change_win_cursor (gdisp->canvas->window, cursor_type);
    }
}


void
gdisplay_remove_tool_cursor (GDisplay *gdisp)
{
  unset_win_cursor (gdisp->canvas->window);
}


void
gdisplay_set_menu_sensitivity (GDisplay *gdisp)
{
  Layer *layer;
  gint fs;
  gint aux;
  gint lm;
  gint lp;
  gint alpha = FALSE;
  GimpDrawable *drawable;
  gint base_type;
  gint type;

  fs = (gimage_floating_sel (gdisp->gimage) != NULL);
  aux = (gimage_get_active_channel (gdisp->gimage) != NULL);
  if ((layer = gimage_get_active_layer (gdisp->gimage)) != NULL)
      lm = (layer->mask) ? TRUE : FALSE;
  else
    lm = FALSE;
  base_type = gimage_base_type (gdisp->gimage);
  lp = (gdisp->gimage->layers != NULL);
  alpha = layer && layer_has_alpha (layer);

  type = -1;
  if (lp)
    {
      drawable = gimage_active_drawable (gdisp->gimage);
      type = drawable_type (drawable);
    }

  menus_set_sensitive ("<Image>/Layers/Raise Layer", !fs && !aux && lp && alpha);
  menus_set_sensitive ("<Image>/Layers/Lower Layer", !fs && !aux && lp && alpha);
  menus_set_sensitive ("<Image>/Layers/Anchor Layer", fs && !aux && lp);
  menus_set_sensitive ("<Image>/Layers/Merge Visible Layers", !fs && !aux && lp);
  menus_set_sensitive ("<Image>/Layers/Flatten Image", !fs && !aux && lp);
  menus_set_sensitive ("<Image>/Layers/Alpha To Selection", !aux && lp && alpha);
  menus_set_sensitive ("<Image>/Layers/Mask To Selection", !aux && lm && lp);
  menus_set_sensitive ("<Image>/Layers/Add Alpha Channel", !fs && !aux && lp && !lm && !alpha);

  menus_set_sensitive ("<Image>/Image/RGB", (base_type != RGB));
  menus_set_sensitive ("<Image>/Image/Grayscale", (base_type != GRAY));
  menus_set_sensitive ("<Image>/Image/Indexed", (base_type != INDEXED));

  menus_set_sensitive ("<Image>/Image/Colors/Threshold", (base_type != INDEXED));
  menus_set_sensitive ("<Image>/Image/Colors/Posterize", (base_type != INDEXED));
  menus_set_sensitive ("<Image>/Image/Colors/Equalize", (base_type != INDEXED));
  menus_set_sensitive ("<Image>/Image/Colors/Invert", (base_type != INDEXED));

  menus_set_sensitive ("<Image>/Image/Colors/Color Balance", (base_type == RGB));
  menus_set_sensitive ("<Image>/Image/Colors/Brightness-Contrast", (base_type != INDEXED));
  menus_set_sensitive ("<Image>/Image/Colors/Hue-Saturation", (base_type == RGB));
  menus_set_sensitive ("<Image>/Image/Colors/Curves", (base_type != INDEXED));
  menus_set_sensitive ("<Image>/Image/Colors/Levels", (base_type != INDEXED));

  menus_set_sensitive ("<Image>/Image/Colors/Desaturate", (base_type == RGB));

  menus_set_sensitive ("<Image>/Image/Alpha/Add Alpha Channel", !fs && !aux && lp && !lm && !alpha);

  menus_set_sensitive ("<Image>/Select", lp);
  menus_set_sensitive ("<Image>/Edit/Cut", lp);
  menus_set_sensitive ("<Image>/Edit/Copy", lp);
  menus_set_sensitive ("<Image>/Edit/Paste", lp);
  menus_set_sensitive ("<Image>/Edit/Paste Into", lp);
  menus_set_sensitive ("<Image>/Edit/Clear", lp);
  menus_set_sensitive ("<Image>/Edit/Fill", lp);
  menus_set_sensitive ("<Image>/Edit/Stroke", lp);
  menus_set_sensitive ("<Image>/Edit/Cut Named", lp);
  menus_set_sensitive ("<Image>/Edit/Copy Named", lp);
  menus_set_sensitive ("<Image>/Edit/Paste Named", lp);
  menus_set_sensitive ("<Image>/Image/Colors", lp);
  menus_set_sensitive ("<Image>/Image/Channel Ops/Offset", lp);
  menus_set_sensitive ("<Image>/Image/Histogram", lp);
  menus_set_sensitive ("<Image>/Filters", lp);

  /* save selection to channel */
  menus_set_sensitive ("<Image>/Select/Save To Channel", !fs);

  menus_set_state ("<Image>/View/Toggle Rulers", GTK_WIDGET_VISIBLE (gdisp->origin) ? 1 : 0);
  menus_set_state ("<Image>/View/Toggle Guides", gdisp->draw_guides);
  menus_set_state ("<Image>/View/Snap To Guides", gdisp->snap_to_guides);

  plug_in_set_menu_sensitivity (type);
}

void
gdisplay_expose_area (GDisplay *gdisp,
		      int       x,
		      int       y,
		      int       w,
		      int       h)
{
  gdisplay_add_display_area (gdisp, x, y, w, h);
}

void
gdisplay_expose_guide (GDisplay *gdisp,
		       Guide    *guide)
{
  int x, y;

  if (guide->position < 0)
    return;

  gdisplay_transform_coords (gdisp, guide->position,
			     guide->position, &x, &y, FALSE);

  switch (guide->orientation)
    {
    case HORIZONTAL_GUIDE:
      gdisplay_expose_area (gdisp, 0, y, gdisp->disp_width, 1);
      break;
    case VERTICAL_GUIDE:
      gdisplay_expose_area (gdisp, x, 0, 1, gdisp->disp_height);
      break;
    }
}

void
gdisplay_expose_full (GDisplay *gdisp)
{
  gdisplay_add_display_area (gdisp, 0, 0,
			     gdisp->disp_width,
			     gdisp->disp_height);
}

/**************************************************/
/*  Functions independent of a specific gdisplay  */
/**************************************************/

GDisplay *
gdisplay_active ()
{
  GtkWidget *event_widget;
  GtkWidget *toplevel_widget;
  GdkEvent *event;
  GDisplay *gdisp;

  /*  If the popup shell is valid, then get the gdisplay associated with that shell  */
  event = gtk_get_current_event ();
  event_widget = gtk_get_event_widget (event);
  gdk_event_free (event);

  if (event_widget == NULL)
    return NULL;

  toplevel_widget = gtk_widget_get_toplevel (event_widget);
  gdisp = g_hash_table_lookup (display_ht, toplevel_widget);

  if (gdisp)
    return gdisp;

  if (popup_shell)
    {
      gdisp = gtk_object_get_user_data (GTK_OBJECT (popup_shell));
      return gdisp;
    }

  return NULL;
}


GDisplay *
gdisplay_get_ID (int ID)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  Traverse the list of displays, returning the one that matches the ID  */
  /*  If no display in the list is a match, return NULL.                    */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->ID == ID)
	return gdisp;

      list = g_slist_next (list);
    }

  return NULL;
}


void
gdisplays_update_title (int ID)
{
  GDisplay *gdisp;
  GSList *list = display_list;
  char title [MAX_TITLE_BUF];

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	{
	  /* format the title */
	  gdisplay_format_title (gdisp->gimage, title);
	  gdk_window_set_title (gdisp->shell->window, title);
	}

      list = g_slist_next (list);
    }
}


void
gdisplays_update_area (int ID,
		       int x,
		       int y,
		       int w,
		       int h)
{
  GDisplay *gdisp;
  GSList *list = display_list;
  int x1, y1, x2, y2;
  /*  int count = 0; */

  /*  traverse the linked list of displays  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	{
	  /*  We only need to update the first instance that
	      we find of this gimage ID.  Otherwise, we would
	      be reconverting the same region unnecessarily.   */

	  /* Um.. I don't think so. If you only do this to the first
	     instance, you don't update other gdisplays pointing to this
	     gimage.  I'm going to comment this out to show how it was in
	     case we need to change it back.  msw 4/15/1998
	  */
	  /*
	  if (! count)
	    gdisplay_add_update_area (gdisp, x, y, w, h);
	  else
	    {
	      gdisplay_transform_coords (gdisp, x, y, &x1, &y1, 0);
	      gdisplay_transform_coords (gdisp, x + w, y + h, &x2, &y2, 0);
	      gdisplay_add_display_area (gdisp, x1, y1, (x2 - x1), (y2 - y1));
	    }
	  */

	  gdisplay_add_update_area (gdisp, x, y, w, h);
	  gdisplay_transform_coords (gdisp, x, y, &x1, &y1, 0);
	  gdisplay_transform_coords (gdisp, x + w, y + h, &x2, &y2, 0);
	  gdisplay_add_display_area (gdisp, x1, y1, (x2 - x1), (y2 - y1));
	}
      list = g_slist_next (list);
    }
}


void
gdisplays_expose_guides (int ID)
{
  GDisplay *gdisp;
  GList *tmp_list;
  GSList *list;

  /*  traverse the linked list of displays, handling each one  */
  list = display_list;
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	{
	  tmp_list = gdisp->gimage->guides;
	  while (tmp_list)
	    {
	      gdisplay_expose_guide (gdisp, tmp_list->data);
	      tmp_list = tmp_list->next;
	    }
	}

      list = g_slist_next (list);
    }
}


void
gdisplays_expose_guide (int    ID,
			Guide *guide)
{
  GDisplay *gdisp;
  GSList *list;

  /*  traverse the linked list of displays, handling each one  */
  list = display_list;
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	gdisplay_expose_guide (gdisp, guide);

      list = g_slist_next (list);
    }
}


void
gdisplays_update_full (int ID)
{
  GDisplay *gdisp;
  GSList *list = display_list;
  int count = 0;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	{
	  if (! count)
	    gdisplay_add_update_area (gdisp, 0, 0,
				      gdisp->gimage->width,
				      gdisp->gimage->height);
	  else
	    gdisplay_add_display_area (gdisp, 0, 0,
				       gdisp->disp_width,
				       gdisp->disp_height);

	  count++;
	}

      list = g_slist_next (list);
    }
}


void
gdisplays_shrink_wrap (int ID)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == ID)
	shrink_wrap_display (gdisp);

      list = g_slist_next (list);
    }
}


void
gdisplays_expose_full ()
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      gdisplay_expose_full (gdisp);
      list = g_slist_next (list);
    }
}


void
gdisplays_selection_visibility (int              gimage_ID,
				SelectionControl function)
{
  GDisplay *gdisp;
  GSList *list = display_list;
  int count = 0;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage->ID == gimage_ID && gdisp->select)
	{
	  switch (function)
	    {
	    case SelectionOff:
	      selection_invis (gdisp->select);
	      break;
	    case SelectionLayerOff:
	      selection_layer_invis (gdisp->select);
	      break;
	    case SelectionOn:
	      selection_start (gdisp->select, TRUE);
	      break;
	    case SelectionPause:
	      selection_pause (gdisp->select);
	      break;
	    case SelectionResume:
	      selection_resume (gdisp->select);
	      break;
	    }
	  count++;
	}

      list = g_slist_next (list);
    }
}


int
gdisplays_dirty ()
{
  int dirty = 0;
  GSList *list = display_list;

  /*  traverse the linked list of displays  */
  while (list)
    {
      if (((GDisplay *) list->data)->gimage->dirty > 0)
	dirty = 1;
      list = g_slist_next (list);
    }

  return dirty;
}


void
gdisplays_delete ()
{
  GSList *list = display_list;
  GDisplay *gdisp;

  /*  traverse the linked list of displays  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      list = g_slist_next (list);
      gtk_widget_destroy (gdisp->shell);
    }

  /*  free up linked list data  */
  g_slist_free (display_list);
}


void
gdisplays_flush ()
{
  static int flushing = FALSE;
  GSList *list = display_list;

  /*  no flushing necessary without an interface  */
  if (no_interface)
    return;

  /*  this prevents multiple recursive calls to this procedure  */
  if (flushing == TRUE)
    return;

  flushing = TRUE;

  /*  traverse the linked list of displays  */
  while (list)
    {
      gdisplay_flush ((GDisplay *) list->data);
      list = g_slist_next (list);
    }

  /*  for convenience, we call the layers dialog flush here  */
  layers_dialog_flush ();
  /*  for convenience, we call the channels dialog flush here  */
  channels_dialog_flush ();

  flushing = FALSE;
}

static guint
gdisplay_hash (GDisplay *display)
{
  return (gulong) display;
}
