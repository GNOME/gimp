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
#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "appenv.h"
#include "colormaps.h"
#include "cursorutil.h"
#include "disp_callbacks.h"
#include "drawable.h"
#include "draw_core.h"
#include "gdisplay.h"
#include "gdisplayP.h"
#include "gdisplay_color.h"
#include "gdisplay_ops.h"
#include "gimage_mask.h"
#include "gimpcontext.h"
#include "gimprc.h"
#include "gximage.h"
#include "image_render.h"
#include "info_window.h"
#include "interface.h"
#include "lc_dialog.h"
#include "menus.h"
#include "nav_window.h"
#include "plug_in.h"
#include "qmask.h"
#include "scale.h"
#include "scroll.h"
#include "tools.h"
#include "undo.h"

#include "bezier_selectP.h"
#include "layer_pvt.h"			/* ick. (not alone either) */

#include "libgimp/gimpmath.h"

#include "libgimp/gimpintl.h"


#define OVERHEAD          25  /*  in units of pixel area  */
#define EPSILON           5

#define MAX_TITLE_BUF 256

/* variable declarations */
GSList *               display_list = NULL;
static gint            display_num  = 1;
static GdkCursorType   default_gdisplay_cursor = GDK_TOP_LEFT_ARROW;

/*  Local functions  */
static void       gdisplay_format_title       (GDisplay *, char *, int);
static void       gdisplay_delete             (GDisplay *);
static GSList *   gdisplay_free_area_list     (GSList *);
static GSList *   gdisplay_process_area_list  (GSList *, GimpArea *);
static void       gdisplay_add_update_area    (GDisplay *, int, int, int, int);
static void       gdisplay_add_display_area   (GDisplay *, int, int, int, int);
static void       gdisplay_paint_area         (GDisplay *, int, int, int, int);
static void	  gdisplay_draw_cursor	      (GDisplay *);
static void       gdisplay_display_area       (GDisplay *, int, int, int, int);
static guint      gdisplay_hash               (GDisplay *);
static void       gdisplay_cleandirty_handler (GimpImage *, void *);


static GHashTable *display_ht = NULL;

/* FIXME: GDisplays really need to be GtkObjects */

GDisplay*
gdisplay_new (GimpImage *gimage,
	      guint      scale)
{
  GDisplay *gdisp;
  gchar title [MAX_TITLE_BUF];

  /*  If there isn't an interface, never create a gdisplay  */
  if (no_interface)
    return NULL;

  /*
   *  Set all GDisplay parameters...
   */
  gdisp = g_new (GDisplay, 1);

  gdisp->offset_x           = 0;
  gdisp->offset_y           = 0;
  gdisp->scale              = scale;
  gdisp->dot_for_dot        = default_dot_for_dot;
  gdisp->gimage             = gimage;
  gdisp->window_info_dialog = NULL;
  gdisp->window_nav_dialog  = NULL;
  gdisp->nav_popup          = NULL;
  gdisp->depth              = g_visual->depth;
  gdisp->select             = NULL;
  gdisp->ID                 = display_num++;
  gdisp->instance           = gimage->instance_count;
  gdisp->update_areas       = NULL;
  gdisp->display_areas      = NULL;
  gdisp->disp_xoffset       = 0;
  gdisp->disp_yoffset       = 0;
  gdisp->current_cursor     = (GdkCursorType) -1;
  gdisp->cursor_tool        = TOOL_TYPE_NONE;
  gdisp->cursor_modifier    = CURSOR_MODIFIER_NONE;
  gdisp->toggle_cursor      = FALSE;
  gdisp->draw_guides        = TRUE;
  gdisp->snap_to_guides     = TRUE;

  gdisp->draw_cursor           = FALSE;
  gdisp->proximity             = FALSE;
  gdisp->have_cursor           = FALSE;
  gdisp->using_override_cursor = FALSE;

  gdisp->progressid = FALSE;

  gdisp->idle_render.idleid       = -1;
  /*gdisp->idle_render.handlerid = -1;*/
  gdisp->idle_render.update_areas = NULL;
  gdisp->idle_render.active       = FALSE;

  gdisp->cd_list = NULL;
  gdisp->cd_ui   = NULL;

  gdisp->warning_dialog = NULL;

  /* format the title */
  gdisplay_format_title (gdisp, title, MAX_TITLE_BUF);

  /*  add the new display to the list so that it isn't lost  */
  display_list = g_slist_append (display_list, (void *) gdisp);

  /*  create the shell for the image  */
  create_display_shell (gdisp, gimage->width, gimage->height,
			title, gimage_base_type (gimage));

  /* update the title to correct the initially displayed scale */
  gdisplay_update_title (gdisp);

  /*  set the gdisplay colormap type and install the appropriate colormap  */
  gdisp->color_type = (gimage_base_type (gimage) == GRAY) ? GRAY : RGB;

  /* set the qmask buttons */
  qmask_buttons_update(gdisp);

  /*  set the user data  */
  if (!display_ht)
    display_ht = g_hash_table_new ((GHashFunc) gdisplay_hash, NULL);

  g_hash_table_insert (display_ht, gdisp->shell, gdisp);
  g_hash_table_insert (display_ht, gdisp->canvas, gdisp);

  /*  set the current tool cursor  */
  gdisplay_install_tool_cursor (gdisp, default_gdisplay_cursor,
				TOOL_TYPE_NONE,
				CURSOR_MODIFIER_NONE,
				FALSE);

  gimage->instance_count++;   /* this is obsolete */
  gimage->disp_count++;

  lc_dialog_preview_update(gimage);

  /* We're interested in clean and dirty signals so we can update the
   * title if need be. */
  gtk_signal_connect (GTK_OBJECT (gimage), "dirty",
		      GTK_SIGNAL_FUNC (gdisplay_cleandirty_handler), gdisp);
  gtk_signal_connect (GTK_OBJECT (gimage), "clean",
		      GTK_SIGNAL_FUNC (gdisplay_cleandirty_handler), gdisp);

  return gdisp;
}


static int print (char *, int, int, const char *, ...) G_GNUC_PRINTF (4, 5);

static int
print (char *buf, int len, int start, const char *fmt, ...)
{
  va_list args;
  int printed;

  va_start (args, fmt);

  printed = g_vsnprintf (buf + start, len - start, fmt, args);
  if (printed < 0)
    printed = len - start;

  va_end (args);

  return printed;
}

static void
gdisplay_format_title (GDisplay *gdisp,
		       gchar    *title,
		       gint      title_len)
{
  GimpImage *gimage;
  char *image_type_str;
  int empty;
  int i;
  char *format;

  gimage = gdisp->gimage;

  empty = gimage_is_empty (gimage);

  switch (gimage_base_type (gimage))
    {
    case RGB:
      image_type_str = (empty) ? _("RGB-empty") : _("RGB");
      break;
    case GRAY:
      image_type_str = (empty) ? _("grayscale-empty") : _("grayscale");
      break;
    case INDEXED:
      image_type_str = (empty) ? _("indexed-empty") : _("indexed");
      break;
    default:
      image_type_str = NULL;
    }

  i = 0;
  format = image_title_format;

  while (i < title_len && *format)
  {
    switch (*format) {
    case '%':
      format++;
      switch (*format) {
      case 0:
	  g_warning ("image-title-format string ended within %%-sequence");
	  break;

      case '%':
	  title[i++] = '%';
	  break;

      case 'f': /* pruned filename */
	  i += print (title, title_len, i,
		      "%s", g_basename (gimage_filename (gimage)));
	  break;

      case 'F': /* full filename */
	  i += print (title, title_len, i, "%s", gimage_filename (gimage));
	  break;

      case 'p': /* PDB id */
	  i += print (title, title_len, i, "%d", pdb_image_to_id (gimage));
	  break;

      case 'i': /* instance */
	  i += print (title, title_len, i, "%d", gdisp->instance);
	  break;

      case 't': /* type */
	  i += print (title, title_len, i, "%s", image_type_str);
	  break;

      case 's': /* user source zoom factor */
	  i += print (title, title_len, i, "%d", SCALESRC (gdisp));
	  break;

      case 'd': /* user destination zoom factor */
	  i += print (title, title_len, i, "%d", SCALEDEST (gdisp));
	  break;

      case 'z': /* user zoom factor (percentage) */
	  i += print (title, title_len, i,
		      "%d", 100 * SCALEDEST (gdisp) / SCALESRC (gdisp));
	  break;

      case 'D': /* dirty flag */
	  if (format[1] == 0)
	  {
	      g_warning("image-title-format string ended within %%D-sequence");
	      break;
	  }
	  if (gimage->dirty)
	      title[i++] = format[1];
	  format++;
	  break;

	  /* Other cool things to be added:
	   * %m = memory used by picture
	   * some kind of resolution / image size thing
	   * people seem to want to know the active layer name
	   */

      default:
	  g_warning ("image-title-format contains unknown format sequence '%%%c'", *format);
	  break;
      }
      break;

    default:
      title[i++] = *format;
      break;
    }

    format++;
  }

  title[MIN(i, title_len-1)] = 0;
}


static void
gdisplay_delete (GDisplay *gdisp)
{
  g_hash_table_remove (display_ht, gdisp->shell);
  g_hash_table_remove (display_ht, gdisp->canvas);

  /*  stop any active tool  */
  active_tool_control (HALT, (void *) gdisp);

  /*  clear out the pointer to this gdisp from the active tool  */
  if (active_tool &&
      active_tool->gdisp_ptr == gdisp)
    {
      active_tool->drawable = NULL;
      active_tool->gdisp_ptr = NULL;
    }

  /*  free the selection structure  */
  selection_free (gdisp->select);

  /* If this gdisplay was idlerendering at the time when it was deleted,
     deactivate the idlerendering thread before deletion! */
  if (gdisp->idle_render.active)
    {
      gtk_idle_remove (gdisp->idle_render.idleid);
      gdisp->idle_render.active = FALSE;
    }

  /* detach any color displays */
  gdisplay_color_detach_all (gdisp);

  /* get rid of signals handled by this display */
  gtk_signal_disconnect_by_data (GTK_OBJECT (gdisp->gimage), gdisp);

  if (gdisp->scroll_gc)
    gdk_gc_destroy (gdisp->scroll_gc);

  /*  free the area lists  */
  gdisplay_free_area_list (gdisp->update_areas);
  gdisplay_free_area_list (gdisp->display_areas);

  gdisplay_free_area_list (gdisp->idle_render.update_areas);

  /*  remove dialogs before removing the image because they may want to
   *  disconnect from image signals
   */

  /*  insure that if a window information dialog exists, it is removed  */
  info_window_free (gdisp->window_info_dialog);

  /* Remove navigation dialog */
  nav_window_free (gdisp, gdisp->window_nav_dialog);

  /*  free the gimage  */
  gdisp->gimage->disp_count--;
  gimage_delete (gdisp->gimage);

  if (gdisp->nav_popup)
    nav_popup_free (gdisp->nav_popup);

  gtk_widget_unref (gdisp->shell);

  g_free (gdisp);
}


static GSList *
gdisplay_free_area_list (GSList *list)
{
  GSList *l = list;
  GimpArea *ga;

  while (l)
    {
      /*  free the data  */
      ga = (GimpArea *) l->data;
      g_free (ga);

      l = g_slist_next (l);
    }

  if (list)
    g_slist_free (list);

  return NULL;
}


/*
 *  As far as I can tell, this function takes a GimpArea and unifies it with
 *  an existing list of GimpAreas, trying to avoid overdraw.  [adam]
 */
static GSList *
gdisplay_process_area_list (GSList *list,
			    GimpArea  *ga1)
{
  GSList *new_list;
  GSList *l = list;
  gint area1, area2, area3;
  GimpArea *ga2;

  /*  start new list off  */
  new_list = g_slist_prepend (NULL, ga1);
  while (l)
    {
      /*  process the data  */
      ga2 = (GimpArea *) l->data;
      area1 = (ga1->x2 - ga1->x1) * (ga1->y2 - ga1->y1) + OVERHEAD;
      area2 = (ga2->x2 - ga2->x1) * (ga2->y2 - ga2->y1) + OVERHEAD;
      area3 = (MAX (ga2->x2, ga1->x2) - MIN (ga2->x1, ga1->x1)) *
	(MAX (ga2->y2, ga1->y2) - MIN (ga2->y1, ga1->y1)) + OVERHEAD;

      if ((area1 + area2) < area3)
	new_list = g_slist_prepend (new_list, ga2);
      else
	{
	  ga1->x1 = MIN (ga1->x1, ga2->x1);
	  ga1->y1 = MIN (ga1->y1, ga2->y1);
	  ga1->x2 = MAX (ga1->x2, ga2->x2);
	  ga1->y2 = MAX (ga1->y2, ga2->y2);

	  g_free (ga2);
	}

      l = g_slist_next (l);
    }

  if (list)
    g_slist_free (list);

  return new_list;
}


static int
idle_render_next_area (GDisplay *gdisp)
{
  GimpArea  *ga;
  GSList *list;
  
  list = gdisp->idle_render.update_areas;

  if (list == NULL)
    {
      return (-1);
    }

  ga = (GimpArea*) list->data;

  gdisp->idle_render.update_areas =
    g_slist_remove (gdisp->idle_render.update_areas, ga);

  gdisp->idle_render.x = gdisp->idle_render.basex = ga->x1;
  gdisp->idle_render.y = gdisp->idle_render.basey = ga->y1;
  gdisp->idle_render.width = ga->x2 - ga->x1;
  gdisp->idle_render.height = ga->y2 - ga->y1;

  g_free (ga);

  return (0);
}


/* Unless specified otherwise, display re-rendering is organised
 by IdleRender, which amalgamates areas to be re-rendered and
 breaks them into bite-sized chunks which are chewed on in a low-
 priority idle thread.  This greatly improves responsiveness for
 many GIMP operations.  -- Adam */
static int
idlerender_callback (gpointer data)
{
  const gint CHUNK_WIDTH = 256;
  const gint CHUNK_HEIGHT = 128;
  gint workx, worky, workw, workh;
  GDisplay* gdisp = data;

  workw = CHUNK_WIDTH;
  workh = CHUNK_HEIGHT;
  workx = gdisp->idle_render.x;
  worky = gdisp->idle_render.y;

  if (workx+workw > gdisp->idle_render.basex+gdisp->idle_render.width)
    {
      workw = gdisp->idle_render.basex+gdisp->idle_render.width-workx;
    }

  if (worky+workh > gdisp->idle_render.basey+gdisp->idle_render.height)
    {
      workh = gdisp->idle_render.basey+gdisp->idle_render.height-worky;
    }  

  gdisplay_paint_area (gdisp, workx, worky,
		       workw, workh);
  gdisplay_flush_displays_only (gdisp);

  gdisp->idle_render.x += CHUNK_WIDTH;
  if (gdisp->idle_render.x >= gdisp->idle_render.basex+gdisp->idle_render.width)
    {
      gdisp->idle_render.x = gdisp->idle_render.basex;
      gdisp->idle_render.y += CHUNK_HEIGHT;
      if (gdisp->idle_render.y >=
	  gdisp->idle_render.basey + gdisp->idle_render.height)
	{
	  if (idle_render_next_area(gdisp) != 0)
	    {
	      /* FINISHED */
	      gdisp->idle_render.active = FALSE;

	      return 0;
	    }
	}
    }

  /* Still work to do. */
  return 1;
}


static void
gdisplay_idlerender_init (GDisplay *gdisp)
{
  GSList *list;
  GimpArea  *ga, *new_ga;

/*  gdisplay_install_override_cursor(gdisp, GDK_CIRCLE); */

  /* We need to merge the IdleRender's and the GDisplay's update_areas list
     to keep track of which of the updates have been flushed and hence need
     to be drawn.   */
  list = gdisp->update_areas;
  while (list)
    {
      ga = (GimpArea *) list->data;
      new_ga = g_malloc (sizeof(GimpArea));
      memcpy (new_ga, ga, sizeof(GimpArea));

      gdisp->idle_render.update_areas =
	gdisplay_process_area_list (gdisp->idle_render.update_areas, new_ga);

      list = g_slist_next (list);
    }

  /* If an idlerender was already running, merge the remainder of its
     unrendered area with the update_areas list, and make it start work
     on the next unrendered area in the list. */
  if (gdisp->idle_render.active)
    {
      new_ga = g_malloc (sizeof(GimpArea));
      new_ga->x1 = gdisp->idle_render.basex;
      new_ga->y1 = gdisp->idle_render.y;
      new_ga->x2 = gdisp->idle_render.basex + gdisp->idle_render.width;
      new_ga->y2 = gdisp->idle_render.y +
	(gdisp->idle_render.height -
	 (gdisp->idle_render.y - gdisp->idle_render.basey)
	 );
      
      gdisp->idle_render.update_areas =
	gdisplay_process_area_list (gdisp->idle_render.update_areas, new_ga);

      idle_render_next_area(gdisp);
    }
  else
    {
      if (gdisp->idle_render.update_areas == NULL)
	{
	  g_warning ("Wanted to start idlerender thread with no update_areas. (+memleak)");
	  return;
	}
      
      idle_render_next_area(gdisp);
      
      gdisp->idle_render.active = TRUE;
      
      gdisp->idle_render.idleid =
	gtk_idle_add_priority (GTK_PRIORITY_LOW,
			       idlerender_callback, gdisp);
    }

  /* Caller frees gdisp->update_areas */
}


void
gdisplay_flush_displays_only (GDisplay *gdisp)
{
  GSList *list;
  GimpArea  *ga;

  list = gdisp->display_areas;

  if (list)
    {
      /*  stop the currently active tool  */
      active_tool_control (PAUSE, (void *) gdisp);

      while (list)
	{
	  /*  Paint the area specified by the GimpArea  */
	  ga = (GimpArea *) list->data;
	  gdisplay_display_area (gdisp, ga->x1, ga->y1,
				 (ga->x2 - ga->x1), (ga->y2 - ga->y1));

	  list = g_slist_next (list);
	}
      /*  Free the update lists  */
      gdisp->display_areas = gdisplay_free_area_list (gdisp->display_areas);

      /* draw the guides */
      gdisplay_draw_guides (gdisp);

      /* and the cursor (if we have a software cursor */
      if (gdisp->have_cursor)
	gdisplay_draw_cursor (gdisp);

      /* restart (and recalculate) the selection boundaries */
      selection_start (gdisp->select, TRUE);

      /* start the currently active tool */
      active_tool_control (RESUME, (void *) gdisp);
    }  
}


static void
gdisplay_flush_whenever (GDisplay *gdisp, 
			 gboolean  now)
{
  GSList *list;
  GimpArea  *ga;

  /*  Flush the items in the displays and updates lists -
   *  but only if gdisplay has been mapped and exposed
   */
  if (!gdisp->select)
    return;

  /*  First the updates...  */
  if (now)
    { /* Synchronous */
      list = gdisp->update_areas;
      while (list)
	{
	  /*  Paint the area specified by the GimpArea  */
	  ga = (GimpArea *) list->data;
	  
	  if ((ga->x1 != ga->x2) && (ga->y1 != ga->y2))
	    {
	      gdisplay_paint_area (gdisp, ga->x1, ga->y1,
				   (ga->x2 - ga->x1), (ga->y2 - ga->y1));
	    }
	  
	  list = g_slist_next (list);
	}
    }
  else
    { /* Asynchronous */
      if (gdisp->update_areas)
	gdisplay_idlerender_init (gdisp);
    }
  /*  Free the update lists  */
  gdisp->update_areas = gdisplay_free_area_list (gdisp->update_areas);

  /*  Next the displays...  */
  gdisplay_flush_displays_only (gdisp);

  /*  update the gdisplay's info dialog  */
  info_window_update (gdisp);
 
  /* update the gdisplay's qmask buttons */
  qmask_buttons_update (gdisp);

  /*  ensure the consistency of the tear-off menus  */
  if (!now && gimp_context_get_display (gimp_context_get_user ()) == gdisp)
    gdisplay_set_menu_sensitivity (gdisp);
}

void
gdisplay_flush (GDisplay *gdisp)
{
  /* Redraw on idle time */
  gdisplay_flush_whenever (gdisp, FALSE);
}

void
gdisplay_flush_now (GDisplay *gdisp)
{
  /* Redraw NOW */
  gdisplay_flush_whenever (gdisp, TRUE);
}

/* Force all gdisplays to finish their idlerender projection */
void 
gdisplays_finish_draw (void)
{
  GSList *list = display_list;
  GDisplay* gdisp;

  while (list)
    {
      gdisp = (GDisplay*) list->data;
      
      if (gdisp->idle_render.active)
	{
	  gtk_idle_remove (gdisp->idle_render.idleid);
	  while (idlerender_callback(gdisp));
	}
      list = g_slist_next (list);
    }
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
		     gboolean  active)
{
  static GdkGC *normal_hgc = NULL;
  static GdkGC *active_hgc = NULL;
  static GdkGC *normal_vgc = NULL;
  static GdkGC *active_vgc = NULL;
  static int initialize = TRUE;
  gint x1, x2;
  gint y1, y2;
  gint w, h;
  gint x, y;

  if (guide->position < 0)
    return;

  if (initialize)
    {
      GdkGCValues values;
      const char stipple[] =
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
      values.stipple = gdk_bitmap_create_from_data (gdisp->canvas->window,
						    (char*) stipple, 8, 1);
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
      values.stipple = gdk_bitmap_create_from_data (gdisp->canvas->window,
						    (char*) stipple, 1, 8);
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
  gdisplay_transform_coords (gdisp,
			     gdisp->gimage->width, gdisp->gimage->height,
			     &x2, &y2, FALSE);
  gdk_window_get_size (gdisp->canvas->window, &w, &h);

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 > w) x2 = w;
  if (y2 > h) y2 = h;

  if (guide->orientation == ORIENTATION_HORIZONTAL)
    {
      gdisplay_transform_coords (gdisp, 0, guide->position, &x, &y, FALSE);

      if (active)
	gdk_draw_line (gdisp->canvas->window, active_hgc, x1, y, x2, y);
      else
	gdk_draw_line (gdisp->canvas->window, normal_hgc, x1, y, x2, y);
    }
  else if (guide->orientation == ORIENTATION_VERTICAL)
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
		     gdouble   x,
		     gdouble   y)
{
  GList *tmp_list;
  Guide *guide;
  gint offset_x, offset_y;
  gdouble scalex, scaley;
  gdouble pos;

  if (gdisp->draw_guides)
    {
      offset_x = gdisp->offset_x - gdisp->disp_xoffset;
      offset_y = gdisp->offset_y - gdisp->disp_yoffset;
      scalex = SCALEFACTOR_X (gdisp);
      scaley = SCALEFACTOR_Y (gdisp);

      tmp_list = gdisp->gimage->guides;
      while (tmp_list)
	{
	  guide = tmp_list->data;
	  tmp_list = tmp_list->next;

	  switch (guide->orientation)
	    {
	    case ORIENTATION_HORIZONTAL:
	      pos = scaley * guide->position - offset_y;
	      if ((guide->position != -1) &&
		  (pos > (y - EPSILON)) &&
		  (pos < (y + EPSILON)))
		return guide;
	      break;
	    case ORIENTATION_VERTICAL:
	      pos = scalex * guide->position - offset_x;
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

gboolean
gdisplay_snap_point (GDisplay *gdisp,
		     gdouble   x,
		     gdouble   y,
		     gdouble  *tx,
		     gdouble  *ty)
{
  GList *tmp_list;
  Guide *guide;
  gdouble scalex, scaley;
  gdouble pos;
  gint offset_x, offset_y;
  gint minhdist, minvdist;
  gint dist;
  gboolean snapped = FALSE;

  *tx = x;
  *ty = y;

  if (gdisp->draw_guides &&
      gdisp->snap_to_guides &&
      gdisp->gimage->guides)
    {
      offset_x = gdisp->offset_x - gdisp->disp_xoffset;
      offset_y = gdisp->offset_y - gdisp->disp_yoffset;
      scalex = SCALEFACTOR_X (gdisp);
      scaley = SCALEFACTOR_Y (gdisp);

      minhdist = G_MAXINT;
      minvdist = G_MAXINT;

      tmp_list = gdisp->gimage->guides;
      while (tmp_list)
	{
	  guide = tmp_list->data;
	  tmp_list = tmp_list->next;

	  switch (guide->orientation)
	    {
	    case ORIENTATION_HORIZONTAL:
	      pos = scaley * guide->position - offset_y;

	      if ((pos > (y - EPSILON)) &&
		  (pos < (y + EPSILON)))
		{
		  dist = (int) pos - y;
		  dist = ABS (dist);

		  if (dist < minhdist)
		    {
		      minhdist = dist;
		      *ty = pos;
		      snapped = TRUE;
		    }
		}
	      break;
	    case ORIENTATION_VERTICAL:
	      pos = scalex * guide->position - offset_x;

	      if ((pos > (x - EPSILON)) &&
		  (pos < (x + EPSILON)))
		{
		  dist = (int) pos - x;
		  dist = ABS (dist);

		  if (dist < minvdist)
		    {
		      minvdist = dist;
		      *tx = pos; 
		      snapped = TRUE;
		    }
		}
	      break;
	    }
	}
    }
  return snapped;
}

void
gdisplay_snap_rectangle (GDisplay *gdisp,
			 gdouble   x1,
			 gdouble   y1,
			 gdouble   x2,
			 gdouble   y2,
			 gdouble  *tx1,
			 gdouble  *ty1)
{
  gdouble nx1, ny1;
  gdouble nx2, ny2;
  gboolean snap1, snap2;

  *tx1 = x1;
  *ty1 = y1;

  snap1 = gdisplay_snap_point (gdisp, x1, y1, &nx1, &ny1);
  snap2 = gdisplay_snap_point (gdisp, x2, y2, &nx2, &ny2);
  
  if (snap1 || snap2)
    {
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
gdisplay_draw_cursor (GDisplay *gdisp)
{
  gint x = gdisp->cursor_x;
  gint y = gdisp->cursor_y;
  
  gdk_draw_line (gdisp->canvas->window,
		 gdisp->canvas->style->white_gc,
		 x - 7, y-1, x + 7, y-1);
  gdk_draw_line (gdisp->canvas->window,
		 gdisp->canvas->style->black_gc,
		 x - 7, y, x + 7, y);
  gdk_draw_line (gdisp->canvas->window,
		 gdisp->canvas->style->white_gc,
		 x - 7, y+1, x + 7, y+1);
  gdk_draw_line (gdisp->canvas->window,
		 gdisp->canvas->style->white_gc,
		 x-1, y - 7, x-1, y + 7);
  gdk_draw_line (gdisp->canvas->window,
		 gdisp->canvas->style->black_gc,
		 x, y - 7, x, y + 7);
  gdk_draw_line (gdisp->canvas->window,
		 gdisp->canvas->style->white_gc,
		 x+1, y - 7, x+1, y + 7);
}

void
gdisplay_update_cursor (GDisplay *gdisp, 
			gint      x, 
			gint      y)
{
  gint new_cursor;
  gchar buffer[CURSOR_STR_LENGTH];
  gint t_x;
  gint t_y;

  new_cursor = gdisp->draw_cursor && gdisp->proximity;
  
  /* Erase old cursor, if necessary */

  if (gdisp->have_cursor && (!new_cursor || x != gdisp->cursor_x ||
			     y != gdisp->cursor_y))
    {
      gdisplay_expose_area (gdisp, gdisp->cursor_x - 7,
			    gdisp->cursor_y - 7,
			    15, 15);
      if (!new_cursor)
	{
	  gdisp->have_cursor = FALSE;
	  gdisplay_flush (gdisp);
	}
    }

  gdisplay_untransform_coords (gdisp, x, y, &t_x, &t_y, FALSE, FALSE);

  if (t_x < 0 ||
      t_y < 0 ||
      t_x >= gdisp->gimage->width ||
      t_y >= gdisp->gimage->height)
    {
      gtk_label_set (GTK_LABEL (gdisp->cursor_label), "");
      info_window_update_RGB (gdisp, -1, -1);
    } 
  else 
    {
      if (gdisp->dot_for_dot)
	{
	  g_snprintf (buffer, CURSOR_STR_LENGTH, 
		      gdisp->cursor_format_str, "", t_x, ", ", t_y);
	}
      else /* show real world units */
	{
	  gdouble unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);
	  
	  g_snprintf
	    (buffer, CURSOR_STR_LENGTH, gdisp->cursor_format_str,
	     "",
	     (gdouble) t_x * unit_factor / gdisp->gimage->xresolution,
	     ", ",
	     (gdouble) t_y * unit_factor / gdisp->gimage->yresolution);
	}
      gtk_label_set (GTK_LABEL (gdisp->cursor_label), buffer);
      info_window_update_RGB (gdisp, t_x, t_y);
    }

  gdisp->have_cursor = new_cursor;
  gdisp->cursor_x = x;
  gdisp->cursor_y = y;
  
  if (new_cursor)
    gdisplay_flush (gdisp);
}


void
gdisplay_set_dot_for_dot (GDisplay *gdisp, 
			  gboolean  value)
{
  if (value != gdisp->dot_for_dot)
    {
      gdisp->dot_for_dot = value;

      gdisplay_resize_cursor_label (gdisp);
      resize_display (gdisp, allow_resize_windows, TRUE);
    }
}


void
gdisplay_resize_cursor_label (GDisplay *gdisp)
{
  /* Set a proper size for the coordinates display in the statusbar. */
  gchar buffer[CURSOR_STR_LENGTH];
  gint cursor_label_width;
  gint label_frame_size_difference;

  if (gdisp->dot_for_dot)
    {
      g_snprintf (gdisp->cursor_format_str, sizeof (gdisp->cursor_format_str),
		  "%%s%%d%%s%%d");
      g_snprintf (buffer, sizeof (buffer), gdisp->cursor_format_str,
		  "", gdisp->gimage->width, ", ", gdisp->gimage->height);
    }
  else /* show real world units */
    {
      gdouble unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);

      g_snprintf (gdisp->cursor_format_str, sizeof (gdisp->cursor_format_str),
		  "%%s%%.%df%%s%%.%df %s",
		  gimp_unit_get_digits (gdisp->gimage->unit),
		  gimp_unit_get_digits (gdisp->gimage->unit),
		  gimp_unit_get_symbol (gdisp->gimage->unit));

      g_snprintf (buffer, sizeof (buffer), gdisp->cursor_format_str,
		  "",
		  (gdouble) gdisp->gimage->width * unit_factor /
		  gdisp->gimage->xresolution,
		  ", ",
		  (gdouble) gdisp->gimage->height * unit_factor /
		  gdisp->gimage->yresolution);
    }
  cursor_label_width = 
    gdk_string_width (gtk_widget_get_style (gdisp->cursor_label)->font, buffer);
  
  /*  find out how many pixels the label's parent frame is bigger than
   *  the label itself
   */
  label_frame_size_difference =
    gdisp->cursor_label->parent->allocation.width -
    gdisp->cursor_label->allocation.width;

  gtk_widget_set_usize (gdisp->cursor_label, cursor_label_width, -1);
  if (label_frame_size_difference) /* don't resize if this is a new display */
    gtk_widget_set_usize (gdisp->cursor_label->parent,
			  cursor_label_width + label_frame_size_difference, -1);

  gdisplay_update_cursor (gdisp, gdisp->cursor_x, gdisp->cursor_y);
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
			  gint      x,
			  gint      y,
			  gint      w,
			  gint      h)
{
  GimpArea * ga;

  ga = (GimpArea *) g_malloc (sizeof (GimpArea));
  ga->x1 = CLAMP (x, 0, gdisp->gimage->width);
  ga->y1 = CLAMP (y, 0, gdisp->gimage->height);
  ga->x2 = CLAMP (x + w, 0, gdisp->gimage->width);
  ga->y2 = CLAMP (y + h, 0, gdisp->gimage->height);

  gdisp->update_areas = gdisplay_process_area_list (gdisp->update_areas, ga);
}


static void
gdisplay_add_display_area (GDisplay *gdisp,
			   gint      x,
			   gint      y,
			   gint      w,
			   gint      h)
{
  GimpArea * ga;

  ga = g_new (GimpArea, 1);

  ga->x1 = CLAMP (x, 0, gdisp->disp_width);
  ga->y1 = CLAMP (y, 0, gdisp->disp_height);
  ga->x2 = CLAMP (x + w, 0, gdisp->disp_width);
  ga->y2 = CLAMP (y + h, 0, gdisp->disp_height);

  gdisp->display_areas = gdisplay_process_area_list (gdisp->display_areas, ga);
}


static void
gdisplay_paint_area (GDisplay *gdisp,
		     gint      x,
		     gint      y,
		     gint      w,
		     gint      h)
{
  gint x1, y1, x2, y2;

  /*  Bounds check  */
  x1 = CLAMP (x, 0, gdisp->gimage->width);
  y1 = CLAMP (y, 0, gdisp->gimage->height);
  x2 = CLAMP (x + w, 0, gdisp->gimage->width);
  y2 = CLAMP (y + h, 0, gdisp->gimage->height);
  x = x1;
  y = y1;
  w = (x2 - x1);
  h = (y2 - y1);

  /*  calculate the extents of the update as limited by what's visible  */
  gdisplay_untransform_coords (gdisp, 0, 0, &x1, &y1, FALSE, FALSE);
  gdisplay_untransform_coords (gdisp, gdisp->disp_width, gdisp->disp_height,
			       &x2, &y2, FALSE, FALSE);

  gimage_invalidate (gdisp->gimage, x, y, w, h, x1, y1, x2, y2);

    /*  display the area  */
  gdisplay_transform_coords (gdisp, x, y, &x1, &y1, FALSE);
  gdisplay_transform_coords (gdisp, x + w, y + h, &x2, &y2, FALSE);

  gdisplay_expose_area (gdisp, x1, y1, (x2 - x1), (y2 - y1));
}


static void
gdisplay_display_area (GDisplay *gdisp,
		       gint      x,
		       gint      y,
		       gint      w,
		       gint      h)
{
  gint sx, sy;
  gint x1, y1;
  gint x2, y2;
  gint dx, dy;
  gint i, j;
  GList *list;
  guchar *buf;
  gint bpp, bpl;

  buf = gximage_get_data ();
  bpp = gximage_get_bpp ();
  bpl = gximage_get_bpl ();

  sx = SCALEX (gdisp, gdisp->gimage->width);
  sy = SCALEY (gdisp, gdisp->gimage->height);

  /*  Bounds check  */
  x1 = CLAMP (x, 0, gdisp->disp_width);
  y1 = CLAMP (y, 0, gdisp->disp_height);
  x2 = CLAMP (x + w, 0, gdisp->disp_width);
  y2 = CLAMP (y + h, 0, gdisp->disp_height);

  if (y1 < gdisp->disp_yoffset)
    {
      gdk_draw_rectangle (gdisp->canvas->window,
			  gdisp->canvas->style->bg_gc[GTK_STATE_NORMAL], 1,
			  x, y, w, gdisp->disp_yoffset - y);
      /* X X X
         . # .
         . . . */

      y1 = gdisp->disp_yoffset;
    }

  if (x1 < gdisp->disp_xoffset)
    {
      gdk_draw_rectangle (gdisp->canvas->window,
			  gdisp->canvas->style->bg_gc[GTK_STATE_NORMAL], 1,
			  x, y1, gdisp->disp_xoffset - x, h);
      /* . . .
         X # .
         X . . */

      x1 = gdisp->disp_xoffset;
    }

  if (x2 > (gdisp->disp_xoffset + sx))
    {
      gdk_draw_rectangle (gdisp->canvas->window,
			  gdisp->canvas->style->bg_gc[GTK_STATE_NORMAL], 1,
			  gdisp->disp_xoffset + sx, y1,
			  x2 - (gdisp->disp_xoffset + sx), h - (y1-y));
      /* . . .
         . # X
         . . X */

      x2 = gdisp->disp_xoffset + sx;
    }

  if (y2 > (gdisp->disp_yoffset + sy))
    {
      gdk_draw_rectangle (gdisp->canvas->window,
			  gdisp->canvas->style->bg_gc[GTK_STATE_NORMAL], 1,
			  x1, gdisp->disp_yoffset + sy,
			  x2-x1,
			  y2 - (gdisp->disp_yoffset + sy));
      /* . . .
         . # .
         . X . */

      y2 = gdisp->disp_yoffset + sy;
    }

  /*  display the image in GXIMAGE_WIDTH x GXIMAGE_HEIGHT sized chunks */
  for (i = y1; i < y2; i += GXIMAGE_HEIGHT)
    for (j = x1; j < x2; j += GXIMAGE_WIDTH)
      {
	dx = MIN (x2 - j, GXIMAGE_WIDTH);
	dy = MIN (y2 - i, GXIMAGE_HEIGHT);
	render_image (gdisp, j - gdisp->disp_xoffset, i - gdisp->disp_yoffset,
		      dx, dy);
#if 0
	    /* Invalidate the projection just after we render it! */
	    gimage_invalidate_without_render (gdisp->gimage,
					      j - gdisp->disp_xoffset,
					      i - gdisp->disp_yoffset,
					      dx, dy,
					      0, 0, 0, 0);
#endif

	list = gdisp->cd_list;
	while (list)
	  {
	    ColorDisplayNode *node = (ColorDisplayNode *) list->data;
	    node->cd_convert (node->cd_ID, buf, dx, dy, bpp, bpl);
	    list = list->next;
	  }

	gximage_put (gdisp->canvas->window,
		     j, i, dx, dy,
		     gdisp->offset_x,
		     gdisp->offset_y);
      }
}


gint
gdisplay_mask_value (GDisplay *gdisp,
		     gint      x,
		     gint      y)
{
  /*  move the coordinates from screen space to image space  */
  gdisplay_untransform_coords (gdisp, x, y, &x, &y, FALSE, 0);

  return gimage_mask_value (gdisp->gimage, x, y);
}


gint
gdisplay_mask_bounds (GDisplay *gdisp,
		      gint     *x1,
		      gint     *y1,
		      gint     *x2,
		      gint     *y2)
{
  Layer *layer;
  gint off_x;
  gint off_y;

  /*  If there is a floating selection, handle things differently  */
  if ((layer = gimage_floating_sel (gdisp->gimage)))
    {
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      if (! channel_bounds (gimage_get_mask (gdisp->gimage), x1, y1, x2, y2))
	{
	  *x1 = off_x;
	  *y1 = off_y;
	  *x2 = off_x + drawable_width (GIMP_DRAWABLE (layer));
	  *y2 = off_y + drawable_height (GIMP_DRAWABLE (layer));
	}
      else
	{
	  *x1 = MIN (off_x, *x1);
	  *y1 = MIN (off_y, *y1);
	  *x2 = MAX (off_x + drawable_width (GIMP_DRAWABLE (layer)), *x2);
	  *y2 = MAX (off_y + drawable_height (GIMP_DRAWABLE (layer)), *y2);
	}
    }
  else if (! channel_bounds (gimage_get_mask (gdisp->gimage), x1, y1, x2, y2))
    return FALSE;

  gdisplay_transform_coords (gdisp, *x1, *y1, x1, y1, 0);
  gdisplay_transform_coords (gdisp, *x2, *y2, x2, y2, 0);

  /*  Make sure the extents are within bounds  */
  *x1 = CLAMP (*x1, 0, gdisp->disp_width);
  *y1 = CLAMP (*y1, 0, gdisp->disp_height);
  *x2 = CLAMP (*x2, 0, gdisp->disp_width);
  *y2 = CLAMP (*y2, 0, gdisp->disp_height);

  return TRUE;
}


void
gdisplay_transform_coords (GDisplay *gdisp,
			   gint      x,
			   gint      y,
			   gint     *nx,
			   gint     *ny,
			   gboolean  use_offsets)
{
  gdouble scalex;
  gdouble scaley;
  gint offset_x;
  gint offset_y;

  /*  transform from image coordinates to screen coordinates  */
  scalex = SCALEFACTOR_X (gdisp);
  scaley = SCALEFACTOR_Y (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = (int) (scalex * (x + offset_x) - gdisp->offset_x);
  *ny = (int) (scaley * (y + offset_y) - gdisp->offset_y);

  *nx += gdisp->disp_xoffset;
  *ny += gdisp->disp_yoffset;
}


void
gdisplay_untransform_coords (GDisplay *gdisp,
			     gint       x,
			     gint       y,
			     gint      *nx,
			     gint      *ny,
			     gboolean   round,
			     gboolean   use_offsets)
{
  gdouble scalex;
  gdouble scaley;
  gint offset_x;
  gint offset_y;

  x -= gdisp->disp_xoffset;
  y -= gdisp->disp_yoffset;

  /*  transform from screen coordinates to image coordinates  */
  scalex = SCALEFACTOR_X (gdisp);
  scaley = SCALEFACTOR_Y (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  if (round)
    {
      *nx = ROUND ((x + gdisp->offset_x) / scalex - offset_x);
      *ny = ROUND ((y + gdisp->offset_y) / scaley - offset_y);
    }
  else
    {
      *nx = (int) ((x + gdisp->offset_x) / scalex - offset_x);
      *ny = (int) ((y + gdisp->offset_y) / scaley - offset_y);
    }
}


void
gdisplay_transform_coords_f (GDisplay *gdisp,
			     gdouble   x,
			     gdouble   y,
			     gdouble  *nx,
			     gdouble  *ny,
			     gboolean  use_offsets)
{
  gdouble scalex;
  gdouble scaley;
  gint offset_x;
  gint offset_y;

  /*  transform from gimp coordinates to screen coordinates  */
  scalex = SCALEFACTOR_X (gdisp);
  scaley = SCALEFACTOR_Y (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = scalex * (x + offset_x) - gdisp->offset_x;
  *ny = scaley * (y + offset_y) - gdisp->offset_y;

  *nx += gdisp->disp_xoffset;
  *ny += gdisp->disp_yoffset;
}


void
gdisplay_untransform_coords_f (GDisplay *gdisp,
			       gdouble    x,
			       gdouble    y,
			       gdouble   *nx,
			       gdouble   *ny,
			       gboolean   use_offsets)
{
  gdouble scalex;
  gdouble scaley;
  gint offset_x;
  gint offset_y;

  x -= gdisp->disp_xoffset;
  y -= gdisp->disp_yoffset;

  /*  transform from screen coordinates to gimp coordinates  */
  scalex = SCALEFACTOR_X (gdisp);
  scaley = SCALEFACTOR_Y (gdisp);

  if (use_offsets)
    drawable_offsets (gimage_active_drawable (gdisp->gimage), &offset_x, &offset_y);
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = (x + gdisp->offset_x) / scalex - offset_x;
  *ny = (y + gdisp->offset_y) / scaley - offset_y;
}


/*  install and remove tool cursor from gdisplay...  */
void
gdisplay_real_install_tool_cursor (GDisplay      *gdisp,
				   GdkCursorType  cursor_type,
				   ToolType       tool_type,
				   CursorModifier modifier,
				   gboolean       toggle_cursor,
				   gboolean       always_install)
{
  if (cursor_type != GIMP_BAD_CURSOR)
    {
      switch (cursor_mode)
	{
	case CURSOR_MODE_TOOL_ICON:
	  break;

	case CURSOR_MODE_TOOL_CROSSHAIR:
	  cursor_type = GIMP_CROSSHAIR_SMALL_CURSOR;
	  break;

	case CURSOR_MODE_CROSSHAIR:
	  cursor_type = GIMP_CROSSHAIR_CURSOR;
	  tool_type   = TOOL_TYPE_NONE;
	  modifier    = CURSOR_MODIFIER_NONE;
	  break;
	}
    }

  if (gdisp->current_cursor  != cursor_type   ||
      gdisp->cursor_tool     != tool_type     ||
      gdisp->cursor_modifier != modifier      ||
      gdisp->toggle_cursor   != toggle_cursor ||
      always_install)
    {
      gdisp->current_cursor  = cursor_type;
      gdisp->cursor_tool     = tool_type;
      gdisp->cursor_modifier = modifier;
      gdisp->toggle_cursor   = toggle_cursor;

      change_win_cursor (gdisp->canvas->window,
			 cursor_type,
			 tool_type,
			 modifier,
			 toggle_cursor);
    }
}

void
gdisplay_install_tool_cursor (GDisplay       *gdisp,
			      GdkCursorType   cursor_type,
			      ToolType        tool_type,
			      CursorModifier  modifier,
			      gboolean        toggle_cursor)
{
  if (!gdisp->using_override_cursor)
    gdisplay_real_install_tool_cursor (gdisp,
				       cursor_type,
				       tool_type,
				       modifier,
				       toggle_cursor,
				       FALSE);
}


/*  install an override-cursor on gdisplay...  */
void
gdisplay_install_override_cursor (GDisplay      *gdisp,
				  GdkCursorType  cursor_type)
{
  if (!gdisp->using_override_cursor ||
      (gdisp->using_override_cursor &&
       (gdisp->override_cursor != cursor_type)))
    {
      gdisp->override_cursor       = cursor_type;
      gdisp->using_override_cursor = TRUE;
      change_win_cursor (gdisp->canvas->window,
			 cursor_type,
			 TOOL_TYPE_NONE,
			 CURSOR_MODIFIER_NONE,
			 FALSE);
    }
}


/*  remove an override-cursor from gdisplay...  */
void
gdisplay_remove_override_cursor (GDisplay *gdisp)
{
  if (gdisp->using_override_cursor)
    {
      gdisp->using_override_cursor = FALSE;
      gdisplay_real_install_tool_cursor (gdisp,
					 gdisp->current_cursor,
					 gdisp->cursor_tool,
					 gdisp->cursor_modifier,
					 gdisp->toggle_cursor,
					 TRUE);
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
  GimpImageBaseType  base_type = 0;
  GimpImageType      type      = -1;
  GimpDrawable      *drawable  = NULL;
  Layer             *layer     = NULL;
  gboolean fs    = FALSE;
  gboolean aux   = FALSE;
  gboolean lm    = FALSE;
  gboolean lp    = FALSE;
  gboolean alpha = FALSE;
  gint     lind  = -1;
  gint     lnum  = -1;

  if (gdisp)
    {
      base_type = gimage_base_type (gdisp->gimage);

      fs  = (gimage_floating_sel (gdisp->gimage) != NULL);
      aux = (gimage_get_active_channel (gdisp->gimage) != NULL);
      lp  = (gdisp->gimage->layers != NULL);

      drawable = gimage_active_drawable (gdisp->gimage);
      if (drawable)
	type = drawable_type (drawable);

      if (lp)
	{
	  layer = gimage_get_active_layer (gdisp->gimage);
	  if (layer)
	    {
	      lm    = layer_get_mask (layer) ? TRUE : FALSE;
	      alpha = layer_has_alpha (layer);
	    }

	  lind = gimage_get_layer_index (gdisp->gimage,
					 gdisp->gimage->active_layer);
	  lnum = g_slist_length (gdisp->gimage->layers);
	}
    }

#define SET_SENSITIVE(menu,condition) \
        menus_set_sensitive ("<Image>/" menu, (condition) != 0)
#define SET_STATE(menu,condition) \
        menus_set_state ("<Image>/" menu, (condition) != 0)

  SET_SENSITIVE ("File/Save", gdisp && drawable);
  SET_SENSITIVE ("File/Save As...", gdisp && drawable);
  SET_SENSITIVE ("File/Revert", gdisp && gdisp->gimage->filename);
  SET_SENSITIVE ("File/Close", gdisp);

  SET_SENSITIVE ("Edit", gdisp);
  SET_SENSITIVE ("Edit/Buffer", gdisp);
  if (gdisp)
    {
      /* Interactive tools such as CURVES, COLOR_BALANCE, LEVELS disable */
      /* undo to fake some kind of atomic behaviour. G. R. Osgood #14072  */

      if (gimp_image_undo_is_enabled (gdisp->gimage))
	{
	  /* If undo/redo stacks are empty, disable respective menu */

	  SET_SENSITIVE ("Edit/Undo", undo_get_undo_name (gdisp->gimage));
	  SET_SENSITIVE ("Edit/Redo", undo_get_redo_name (gdisp->gimage));
	}
      else
	{
	  SET_SENSITIVE ("Edit/Undo", FALSE);
	  SET_SENSITIVE ("Edit/Redo", FALSE);
	}
      SET_SENSITIVE ("Edit/Cut", lp);
      SET_SENSITIVE ("Edit/Copy", lp);
      SET_SENSITIVE ("Edit/Buffer/Cut Named...", lp);
      SET_SENSITIVE ("Edit/Buffer/Copy Named...", lp);
      SET_SENSITIVE ("Edit/Clear", lp);
      SET_SENSITIVE ("Edit/Fill with FG Color", lp);
      SET_SENSITIVE ("Edit/Fill with BG Color", lp);
      SET_SENSITIVE ("Edit/Stroke", lp);
    }

  SET_SENSITIVE ("Select", gdisp && lp);
  SET_SENSITIVE ("Select/Save to Channel", !fs);

  SET_SENSITIVE ("View", gdisp);
  SET_SENSITIVE ("View/Zoom", gdisp);
  if (gdisp)
    {
      SET_STATE ("View/Toggle Selection", !gdisp->select->hidden);
      SET_STATE ("View/Toggle Rulers",
		 GTK_WIDGET_VISIBLE (gdisp->origin) ? 1 : 0);
      SET_STATE ("View/Toggle Guides", gdisp->draw_guides);
      SET_STATE ("View/Snap to Guides", gdisp->snap_to_guides);
      SET_STATE ("View/Toggle Statusbar",
		 GTK_WIDGET_VISIBLE (gdisp->statusarea) ? 1 : 0);
      SET_STATE ("View/Dot for Dot", gdisp->dot_for_dot);
    }

  SET_SENSITIVE ("Image", gdisp);
  SET_SENSITIVE ("Image/Mode", gdisp);
  SET_SENSITIVE ("Image/Colors", gdisp);
  SET_SENSITIVE ("Image/Colors/Auto", gdisp);
  SET_SENSITIVE ("Image/Alpha", gdisp);
  SET_SENSITIVE ("Image/Transforms", gdisp);
  SET_SENSITIVE ("Image/Transforms/Rotate", gdisp);
  if (gdisp)
    {
      SET_SENSITIVE ("Image/Mode/RGB", (base_type != RGB));
      SET_SENSITIVE ("Image/Mode/Grayscale", (base_type != GRAY));
      SET_SENSITIVE ("Image/Mode/Indexed...", (base_type != INDEXED));
      SET_SENSITIVE ("Image/Histogram...", lp);

      SET_SENSITIVE ("Image/Colors", lp);
      SET_SENSITIVE ("Image/Colors/Color Balance...", (base_type == RGB));
      SET_SENSITIVE ("Image/Colors/Hue-Saturation...", (base_type == RGB));
      SET_SENSITIVE ("Image/Colors/Brightness-Contrast...", (base_type != INDEXED));
      SET_SENSITIVE ("Image/Colors/Threshold...", (base_type != INDEXED));
      SET_SENSITIVE ("Image/Colors/Levels...", (base_type != INDEXED));
      SET_SENSITIVE ("Image/Colors/Curves...", (base_type != INDEXED));
      SET_SENSITIVE ("Image/Colors/Desaturate", (base_type == RGB));
      SET_SENSITIVE ("Image/Colors/Posterize...", (base_type != INDEXED));
      SET_SENSITIVE ("Image/Colors/Invert", (base_type != INDEXED));
      SET_SENSITIVE ("Image/Colors/Auto/Equalize", (base_type != INDEXED));

      SET_SENSITIVE ("Image/Alpha/Add Alpha Channel",
		     !fs && !aux && lp && !lm && !alpha);

      SET_SENSITIVE ("Image/Transforms/Offset...", lp);
    }

  SET_SENSITIVE ("Layers/Stack", gdisp);
  if (gdisp)
    {
      SET_SENSITIVE ("Layers/Stack/Previous Layer",
		     !fs && !aux && lp && lind > 0);
      SET_SENSITIVE ("Layers/Stack/Next Layer",
		     !fs && !aux && lp && lind < (lnum - 1));
      SET_SENSITIVE ("Layers/Stack/Raise Layer",
		     !fs && !aux && lp && alpha && lind > 0);
      SET_SENSITIVE ("Layers/Stack/Lower Layer",
		     !fs && !aux && lp && alpha && lind < (lnum - 1));
      SET_SENSITIVE ("Layers/Stack/Layer to Top",
		     !fs && !aux && lp && alpha && lind > 0);
      SET_SENSITIVE ("Layers/Stack/Layer to Bottom",
		     !fs && !aux && lp && alpha && lind < (lnum - 1));
    }
  SET_SENSITIVE ("Layers/Rotate", gdisp && !aux && !lm & lp);
  SET_SENSITIVE ("Layers/Layer to Imagesize", gdisp && !aux && lp);

  SET_SENSITIVE ("Layers/Anchor Layer", gdisp && fs && !aux && lp);
  SET_SENSITIVE ("Layers/Merge Visible Layers...", gdisp && !fs && !aux && lp);
  SET_SENSITIVE ("Layers/Flatten Image", gdisp && !fs && !aux && lp);
  SET_SENSITIVE ("Layers/Alpha to Selection", gdisp && !aux && lp && alpha);
  SET_SENSITIVE ("Layers/Mask to Selection", gdisp && !aux && lm && lp);
  SET_SENSITIVE ("Layers/Add Alpha Channel",
		 gdisp && !fs && !aux && lp && !lm && !alpha);

  SET_SENSITIVE ("Filters", gdisp && lp);

  SET_SENSITIVE ("Script-Fu", gdisp && lp);

#undef SET_STATE
#undef SET_SENSITIVE

  plug_in_set_menu_sensitivity (type);
}

void
gdisplay_expose_area (GDisplay *gdisp,
		      gint      x,
		      gint      y,
		      gint      w,
		      gint      h)
{
  gdisplay_add_display_area (gdisp, x, y, w, h);
}

void
gdisplay_expose_guide (GDisplay *gdisp,
		       Guide    *guide)
{
  gint x;
  gint y;

  if (guide->position < 0)
    return;

  gdisplay_transform_coords (gdisp, guide->position,
			     guide->position, &x, &y, FALSE);

  switch (guide->orientation)
    {
    case ORIENTATION_HORIZONTAL:
      gdisplay_expose_area (gdisp, 0, y, gdisp->disp_width, 1);
      break;
    case ORIENTATION_VERTICAL:
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
gdisplay_active (void)
{
  GdkEvent *event;

  /*  Whoever finds out why we do this (see below) gets a free beer.
   *  Report back the reason to Sven and Mitch ... 
   */
  event = gtk_get_current_event ();
  if (event != NULL)
    {
      gdk_event_free (event);
    }

  return gimp_context_get_display (gimp_context_get_user ());
}


GDisplay *
gdisplay_get_ID (gint ID)
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
gdisplay_update_title (GDisplay *gdisp)
{
  gchar title [MAX_TITLE_BUF];
  guint context_id;

  /* format the title */
  gdisplay_format_title (gdisp, title, MAX_TITLE_BUF);
  gdk_window_set_title (gdisp->shell->window, title);

  /* update the statusbar */
  context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "title");
  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), context_id);
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), context_id, title);
}


void
gdisplays_update_title (GimpImage *gimage)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage == gimage)
	   gdisplay_update_title (gdisp);

      list = g_slist_next (list);
    }
}

void
gdisplays_resize_cursor_label (GimpImage *gimage)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage == gimage)
	gdisplay_resize_cursor_label (gdisp);

      list = g_slist_next (list);
    }
}

void
gdisplays_setup_scale (GimpImage *gimage)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage == gimage)
	setup_scale (gdisp);

      list = g_slist_next (list);
    }
}

void
gdisplays_update_area (GimpImage *gimage,
		       gint       x,
		       gint       y,
		       gint       w,
		       gint       h)
{
  GDisplay *gdisp;
  GSList *list = display_list;
  /* int x1, y1, x2, y2; */
  /*  int count = 0; */

  /*  traverse the linked list of displays  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage == gimage)
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
	  /* Seems like this isn't needed, it's done in
	     gdisplay_flush. -la
	  gdisplay_transform_coords (gdisp, x, y, &x1, &y1, 0);
	  gdisplay_transform_coords (gdisp, x + w, y + h, &x2, &y2, 0);
	  gdisplay_add_display_area (gdisp, x1, y1, (x2 - x1), (y2 - y1));
	  */
	}
      list = g_slist_next (list);
    }
}


void
gdisplays_expose_guides (GimpImage *gimage)
{
  GDisplay *gdisp;
  GList *tmp_list;
  GSList *list;

  /*  traverse the linked list of displays, handling each one  */
  list = display_list;
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage == gimage)
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
gdisplays_expose_guide (GimpImage *gimage,
			Guide     *guide)
{
  GDisplay *gdisp;
  GSList *list;

  /*  traverse the linked list of displays, handling each one  */
  list = display_list;
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage == gimage)
	{
	  gdisplay_expose_guide (gdisp, guide);
	}
	  
      list = g_slist_next (list);
    }
}


void
gdisplays_update_full (GimpImage* gimage)
{
  GDisplay *gdisp;
  GSList *list = display_list;
  gint count = 0;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage == gimage)
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
gdisplays_shrink_wrap (GimpImage *gimage)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage == gimage)
	shrink_wrap_display (gdisp);

      list = g_slist_next (list);
    }
}


void
gdisplays_expose_full (void)
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
gdisplays_nav_preview_resized (void)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;

      if (gdisp->window_nav_dialog)
	nav_window_preview_resized (gdisp->window_nav_dialog);
      
      if (gdisp->nav_popup)
	nav_window_popup_preview_resized (&gdisp->nav_popup);

      list = g_slist_next (list);
    }
}

void
gdisplays_selection_visibility (GimpImage        *gimage,
				SelectionControl  function)
{
  GDisplay *gdisp;
  GSList *list = display_list;
  gint count = 0;

  /*  traverse the linked list of displays, handling each one  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp->gimage == gimage && gdisp->select)
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


gboolean
gdisplays_dirty (void)
{
  gboolean dirty = FALSE;
  GSList *list = display_list;

  /*  traverse the linked list of displays  */
  while (list)
    {
      if (((GDisplay *) list->data)->gimage->dirty != 0)
	dirty = TRUE;
      list = g_slist_next (list);
    }

  return dirty;
}


void
gdisplays_delete (void)
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


GDisplay *
gdisplays_check_valid (GDisplay  *gtest, 
		       GimpImage *gimage)
{
  /* Give a gdisp check that it is still valid and points to the required
   * GimpImage. If not return the first gDisplay that does point to the 
   * gimage. If none found return NULL;
   */

  GSList *list = display_list;
  GDisplay *gdisp;
  GDisplay *gdisp_found = NULL;

  /*  traverse the linked list of displays  */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      if (gdisp == gtest)
	return (gtest);
      if (!gdisp_found && gdisp->gimage == gimage)
	gdisp_found = gdisp;
      list = g_slist_next (list);
    }

  return (gdisp_found);
}


static void
gdisplays_flush_whenever (gboolean now)
{
  static gboolean flushing = FALSE;
  GSList *list = display_list;

  /*  no flushing necessary without an interface  */
  if (no_interface)
    return;

  /*  this prevents multiple recursive calls to this procedure  */
  if (flushing == TRUE)
    {
      g_warning ("gdisplays_flush() called recursively.");
      return;
    }

  flushing = TRUE;

  /*  traverse the linked list of displays  */
  while (list)
    {
      gdisplay_flush_whenever ((GDisplay *) list->data, now);
      list = g_slist_next (list);
    }

  /*  for convenience, we call the L&C flush here  */
  lc_dialog_flush ();

  flushing = FALSE;
}

void
gdisplays_flush (void)
{
  gdisplays_flush_whenever (FALSE);
}

void
gdisplays_flush_now (void)
{
  gdisplays_flush_whenever (TRUE);
}

static guint
gdisplay_hash (GDisplay *display)
{
  return (gulong) display;
}

void
gdisplay_reconnect (GDisplay  *gdisp, 
		    GimpImage *gimage)
{
  gint instance;

  g_return_if_fail (gdisp != NULL && gimage != NULL);

  if (gdisp->idle_render.active)
    {
      gtk_idle_remove (gdisp->idle_render.idleid);
      gdisp->idle_render.active = FALSE;
    }

  gtk_signal_disconnect_by_data (GTK_OBJECT (gdisp->gimage), gdisp);
  gdisp->gimage->disp_count--;
  gimage_delete (gdisp->gimage);

  instance = gimage->instance_count;
  gimage->instance_count++;
  gimage->disp_count++;

  gdisp->gimage = gimage;
  gdisp->instance = instance;

  /* reconnect our clean / dirty signals */
  gtk_signal_connect (GTK_OBJECT (gimage), "dirty",
		      GTK_SIGNAL_FUNC(gdisplay_cleandirty_handler), gdisp);
  gtk_signal_connect (GTK_OBJECT (gimage), "clean",
		      GTK_SIGNAL_FUNC(gdisplay_cleandirty_handler), gdisp);

  gdisplays_update_title (gimage);

  gdisplay_expose_full (gdisp);
  gdisplay_flush (gdisp);
}


void
gdisplays_reconnect (GimpImage *old,
		     GimpImage *new)
{
  GSList   *list;
  GDisplay *gdisp;

  g_return_if_fail (old != NULL && new != NULL);
  
  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = list->data;
      
      if (gdisp->gimage == old)
	gdisplay_reconnect (gdisp, new);
    }
}

/* Called whenever the underlying gimage is dirtied or cleaned */
static void
gdisplay_cleandirty_handler (GimpImage *gimage, 
			     void      *data)
{
  GDisplay *gdisp = data;

  gdisplay_update_title (gdisp);
}

void
gdisplays_foreach (GFunc    func, 
		   gpointer user_data)
{
  g_slist_foreach (display_list, func, user_data);
}
