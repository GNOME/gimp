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

#include <gtk/gtk.h>

#include "display-types.h"
#include "gui/gui-types.h" /* FIXME */

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "tools/gimptool.h"
#include "tools/tool_manager.h"

#include "gui/info-window.h"

#include "gimpdisplay.h"
#include "gimpdisplay-area.h"
#include "gimpdisplay-handlers.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-handlers.h"

#include "gimprc.h"
#include "nav_window.h"
#include "plug_in.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void       gimp_display_class_init            (GimpDisplayClass *klass);
static void       gimp_display_init                  (GimpDisplay      *gdisp);

static void       gimp_display_finalize              (GObject          *object);

static void       gimp_display_flush_whenever        (GimpDisplay      *gdisp, 
                                                      gboolean          now);
static void       gimp_display_flush_whenever        (GimpDisplay      *gdisp, 
                                                      gboolean          now);
static void       gimp_display_idlerender_init       (GimpDisplay      *gdisp);
static gboolean   gimp_display_idlerender_callback   (gpointer          data);
static gboolean   gimp_display_idle_render_next_area (GimpDisplay      *gdisp);
static void       gimp_display_paint_area            (GimpDisplay      *gdisp,
                                                      gint              x,
                                                      gint              y,
                                                      gint              w,
                                                      gint              h);


static GimpObjectClass *parent_class = NULL;

GSList      *display_list = NULL;
static gint  display_num  = 1;


GType
gimp_display_get_type (void)
{
  static GType display_type = 0;

  if (! display_type)
    {
      static const GTypeInfo display_info =
      {
        sizeof (GimpDisplayClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_display_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpDisplay),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_display_init,
      };

      display_type = g_type_register_static (GIMP_TYPE_OBJECT,
                                             "GimpDisplay",
                                             &display_info, 0);
    }

  return display_type;
}

static void
gimp_display_class_init (GimpDisplayClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_display_finalize;
}

static void
gimp_display_init (GimpDisplay *gdisp)
{
  gdisp->ID                       = display_num++;

  gdisp->gimage                   = NULL;
  gdisp->instance                 = 0;

  gdisp->shell                    = NULL;

  gdisp->scale                    = 0;
  gdisp->dot_for_dot              = gimprc.default_dot_for_dot;
  gdisp->draw_guides              = TRUE;
  gdisp->snap_to_guides           = TRUE;

  gdisp->update_areas             = NULL;

  gdisp->idle_render.idle_id      = 0;
  gdisp->idle_render.update_areas = NULL;
}

static void
gimp_display_finalize (GObject *object)
{
  GimpDisplay *gdisp;

  g_return_if_fail (GIMP_IS_DISPLAY (object));

  gdisp = GIMP_DISPLAY (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpDisplay *
gimp_display_new (GimpImage *gimage,
                  guint      scale)
{
  GimpDisplay *gdisp;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  /*  If there isn't an interface, never create a gdisplay  */
  if (gimage->gimp->no_interface)
    return NULL;

  /*
   *  Set all GimpDisplay parameters...
   */
  gdisp = g_object_new (GIMP_TYPE_DISPLAY, NULL);

  /*  add the new display to the list so that it isn't lost  */
  display_list = g_slist_append (display_list, gdisp);

  gimp_display_connect (gdisp, gimage);

  gdisp->scale = scale;

  /*  create the shell for the image  */
  gdisp->shell = gimp_display_shell_new (gdisp);

  gtk_widget_show (gdisp->shell);

  return gdisp;
}

void
gimp_display_delete (GimpDisplay *gdisp)
{
  GimpTool *active_tool;

  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  /* remove the display from the list */
  display_list = g_slist_remove (display_list, gdisp);

  /*  stop any active tool  */
  tool_manager_control_active (gdisp->gimage->gimp, HALT, gdisp);

  active_tool = tool_manager_get_active (gdisp->gimage->gimp);

  /*  clear out the pointer to this gdisp from the active tool  */
  if (active_tool && active_tool->gdisp == gdisp)
    {
      active_tool->drawable = NULL;
      active_tool->gdisp    = NULL;
    }

  /* If this gdisplay was idlerendering at the time when it was deleted,
   * deactivate the idlerendering thread before deletion!
   */
  if (gdisp->idle_render.idle_id)
    {
      g_source_remove (gdisp->idle_render.idle_id);
      gdisp->idle_render.idle_id = 0;
    }

  /*  free the update area lists  */
  gimp_display_area_list_free (gdisp->update_areas);
  gimp_display_area_list_free (gdisp->idle_render.update_areas);

  if (gdisp->shell)
    {
      gtk_widget_destroy (gdisp->shell);
      gdisp->shell = NULL;
    }

  /*  free the gimage  */
  gimp_display_disconnect (gdisp);

  g_object_unref (G_OBJECT (gdisp));
}

gint
gimp_display_get_ID (GimpDisplay *gdisp)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), -1);

  return gdisp->ID;
}

GimpDisplay *
gimp_display_get_by_ID (Gimp *gimp,
                        gint  ID)
{
  GimpDisplay *gdisp;
  GSList      *list;

  /*  Traverse the list of displays, returning the one that matches the ID
   *  If no display in the list is a match, return NULL.
   */
  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GimpDisplay *) list->data;

      if (gdisp->ID == ID)
	return gdisp;
    }

  return NULL;
}

void
gimp_display_reconnect (GimpDisplay *gdisp, 
                        GimpImage   *gimage)
{
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (gdisp->idle_render.idle_id)
    {
      g_source_remove (gdisp->idle_render.idle_id);
      gdisp->idle_render.idle_id = 0;
    }

  gimp_display_shell_disconnect (GIMP_DISPLAY_SHELL (gdisp->shell));

  gimp_display_disconnect (gdisp);

  gimp_display_connect (gdisp, gimage);

  gimp_display_add_update_area (gdisp,
                                0, 0,
                                gdisp->gimage->width,
                                gdisp->gimage->height);

  gimp_display_shell_reconnect (GIMP_DISPLAY_SHELL (gdisp->shell));
}

void
gimp_display_add_update_area (GimpDisplay *gdisp,
                              gint         x,
                              gint         y,
                              gint         w,
                              gint         h)
{
  GimpArea *ga;

  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  ga = g_new (GimpArea, 1);

  ga->x1 = CLAMP (x, 0, gdisp->gimage->width);
  ga->y1 = CLAMP (y, 0, gdisp->gimage->height);
  ga->x2 = CLAMP (x + w, 0, gdisp->gimage->width);
  ga->y2 = CLAMP (y + h, 0, gdisp->gimage->height);

  gdisp->update_areas = gimp_display_area_list_process (gdisp->update_areas, ga);
}

void
gimp_display_flush (GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  /* Redraw on idle time */
  gimp_display_flush_whenever (gdisp, FALSE);
}

void
gimp_display_flush_now (GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  /* Redraw NOW */
  gimp_display_flush_whenever (gdisp, TRUE);
}

/* Force all gdisplays to finish their idlerender projection */
void
gimp_display_finish_draw (GimpDisplay *gdisp)
{
  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));

  if (gdisp->idle_render.idle_id)
    {
      g_source_remove (gdisp->idle_render.idle_id);
      gdisp->idle_render.idle_id = 0;

      while (gimp_display_idlerender_callback (gdisp));
    }
}


/*  stuff that will go to GimpDisplayShell  */

void
gdisplay_transform_coords (GimpDisplay *gdisp,
			   gint         x,
			   gint         y,
			   gint        *nx,
			   gint        *ny,
			   gboolean     use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           scalex;
  gdouble           scaley;
  gint              offset_x;
  gint              offset_y;

  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  transform from image coordinates to screen coordinates  */
  scalex = SCALEFACTOR_X (gdisp);
  scaley = SCALEFACTOR_Y (gdisp);

  if (use_offsets)
    {
      gimp_drawable_offsets (gimp_image_active_drawable (gdisp->gimage),
                             &offset_x, &offset_y);
    }
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = (gint) (scalex * (x + offset_x) - shell->offset_x);
  *ny = (gint) (scaley * (y + offset_y) - shell->offset_y);

  *nx += shell->disp_xoffset;
  *ny += shell->disp_yoffset;
}

void
gdisplay_untransform_coords (GimpDisplay *gdisp,
			     gint         x,
			     gint         y,
			     gint        *nx,
			     gint        *ny,
			     gboolean     round,
			     gboolean     use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           scalex;
  gdouble           scaley;
  gint              offset_x;
  gint              offset_y;

  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  x -= shell->disp_xoffset;
  y -= shell->disp_yoffset;

  /*  transform from screen coordinates to image coordinates  */
  scalex = SCALEFACTOR_X (gdisp);
  scaley = SCALEFACTOR_Y (gdisp);

  if (use_offsets)
    {
      gimp_drawable_offsets (gimp_image_active_drawable (gdisp->gimage),
                             &offset_x, &offset_y);
    }
  else
    {
      offset_x = offset_y = 0;
    }

  if (round)
    {
      *nx = ROUND ((x + shell->offset_x) / scalex - offset_x);
      *ny = ROUND ((y + shell->offset_y) / scaley - offset_y);
    }
  else
    {
      *nx = (int) ((x + shell->offset_x) / scalex - offset_x);
      *ny = (int) ((y + shell->offset_y) / scaley - offset_y);
    }
}

void
gdisplay_transform_coords_f (GimpDisplay *gdisp,
			     gdouble      x,
			     gdouble      y,
			     gdouble     *nx,
			     gdouble     *ny,
			     gboolean     use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           scalex;
  gdouble           scaley;
  gint              offset_x;
  gint              offset_y;

  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  transform from gimp coordinates to screen coordinates  */
  scalex = SCALEFACTOR_X (gdisp);
  scaley = SCALEFACTOR_Y (gdisp);

  if (use_offsets)
    {
      gimp_drawable_offsets (gimp_image_active_drawable (gdisp->gimage),
                             &offset_x, &offset_y);
    }
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = scalex * (x + offset_x) - shell->offset_x;
  *ny = scaley * (y + offset_y) - shell->offset_y;

  *nx += shell->disp_xoffset;
  *ny += shell->disp_yoffset;
}

void
gdisplay_untransform_coords_f (GimpDisplay *gdisp,
			       gdouble      x,
			       gdouble      y,
			       gdouble     *nx,
			       gdouble     *ny,
			       gboolean     use_offsets)
{
  GimpDisplayShell *shell;
  gdouble           scalex;
  gdouble           scaley;
  gint              offset_x;
  gint              offset_y;

  g_return_if_fail (GIMP_IS_DISPLAY (gdisp));
  g_return_if_fail (nx != NULL);
  g_return_if_fail (ny != NULL);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  x -= shell->disp_xoffset;
  y -= shell->disp_yoffset;

  /*  transform from screen coordinates to gimp coordinates  */
  scalex = SCALEFACTOR_X (gdisp);
  scaley = SCALEFACTOR_Y (gdisp);

  if (use_offsets)
    {
      gimp_drawable_offsets (gimp_image_active_drawable (gdisp->gimage),
                             &offset_x, &offset_y);
    }
  else
    {
      offset_x = offset_y = 0;
    }

  *nx = (x + shell->offset_x) / scalex - offset_x;
  *ny = (y + shell->offset_y) / scaley - offset_y;
}


/*  private functions  */

static void
gimp_display_flush_whenever (GimpDisplay *gdisp, 
                             gboolean     now)
{
  GSList   *list;
  GimpArea *ga;

  /*  Flush the items in the displays and updates lists -
   *  but only if gdisplay has been mapped and exposed
   */
  if (! GIMP_DISPLAY_SHELL (gdisp->shell)->select)
    {
      g_warning ("gimp_display_flush_whenever(): called unrealized");
      return;
    }

  /*  First the updates...  */
  if (now)
    {
      /* Synchronous */

      list = gdisp->update_areas;
      while (list)
	{
	  /*  Paint the area specified by the GimpArea  */
	  ga = (GimpArea *) list->data;
	  
	  if ((ga->x1 != ga->x2) && (ga->y1 != ga->y2))
	    {
	      gimp_display_paint_area (gdisp,
                                       ga->x1,
                                       ga->y1,
                                       (ga->x2 - ga->x1),
                                       (ga->y2 - ga->y1));
	    }
	  
	  list = g_slist_next (list);
	}
    }
  else
    {
      /* Asynchronous */

      if (gdisp->update_areas)
	gimp_display_idlerender_init (gdisp);
    }

  /*  Free the update lists  */
  gdisp->update_areas = gimp_display_area_list_free (gdisp->update_areas);

  /*  Next the displays...  */
  gimp_display_shell_flush (GIMP_DISPLAY_SHELL (gdisp->shell));

  /*  update the gdisplay's info dialog  */
  info_window_update (gdisp);

  /*  ensure the consistency of the tear-off menus  */
  if (! now && gimp_context_get_display (gimp_get_user_context
                                         (gdisp->gimage->gimp)) == gdisp)
    {
      gimp_display_shell_set_menu_sensitivity (GIMP_DISPLAY_SHELL (gdisp->shell));
    }
}

static void
gimp_display_idlerender_init (GimpDisplay *gdisp)
{
  GSList    *list;
  GimpArea  *ga, *new_ga;

  /* We need to merge the IdleRender's and the GimpDisplay's update_areas list
   * to keep track of which of the updates have been flushed and hence need
   * to be drawn. 
   */
  list = gdisp->update_areas;

  while (list)
    {
      ga = (GimpArea *) list->data;

      new_ga = g_new (GimpArea, 1);

      memcpy (new_ga, ga, sizeof (GimpArea));

      gdisp->idle_render.update_areas =
	gimp_display_area_list_process (gdisp->idle_render.update_areas, new_ga);

      list = g_slist_next (list);
    }

  /* If an idlerender was already running, merge the remainder of its
   * unrendered area with the update_areas list, and make it start work
   * on the next unrendered area in the list.
   */
  if (gdisp->idle_render.idle_id)
    {
      new_ga = g_new (GimpArea, 1);

      new_ga->x1 = gdisp->idle_render.basex;
      new_ga->y1 = gdisp->idle_render.y;
      new_ga->x2 = gdisp->idle_render.basex + gdisp->idle_render.width;
      new_ga->y2 = gdisp->idle_render.y + (gdisp->idle_render.height -
                                           (gdisp->idle_render.y -
                                            gdisp->idle_render.basey));

      gdisp->idle_render.update_areas =
	gimp_display_area_list_process (gdisp->idle_render.update_areas, new_ga);

      gimp_display_idle_render_next_area (gdisp);
    }
  else
    {
      if (gdisp->idle_render.update_areas == NULL)
	{
	  g_warning ("Wanted to start idlerender thread with no update_areas. (+memleak)");
	  return;
	}

      gimp_display_idle_render_next_area (gdisp);

      gdisp->idle_render.idle_id = g_idle_add_full (G_PRIORITY_LOW,
                                                    gimp_display_idlerender_callback,
                                                    gdisp,
                                                    NULL);
    }

  /* Caller frees gdisp->update_areas */
}

/* Unless specified otherwise, display re-rendering is organised by
 * IdleRender, which amalgamates areas to be re-rendered and breaks
 * them into bite-sized chunks which are chewed on in a low- priority
 * idle thread.  This greatly improves responsiveness for many GIMP
 * operations.  -- Adam
 */
static gboolean
gimp_display_idlerender_callback (gpointer data)
{
  const gint   CHUNK_WIDTH  = 256;
  const gint   CHUNK_HEIGHT = 128;
  gint         workx, worky, workw, workh;
  GimpDisplay *gdisp = data;

  workw = CHUNK_WIDTH;
  workh = CHUNK_HEIGHT;
  workx = gdisp->idle_render.x;
  worky = gdisp->idle_render.y;

  if (workx + workw > gdisp->idle_render.basex + gdisp->idle_render.width)
    {
      workw = gdisp->idle_render.basex + gdisp->idle_render.width - workx;
    }

  if (worky+workh > gdisp->idle_render.basey+gdisp->idle_render.height)
    {
      workh = gdisp->idle_render.basey + gdisp->idle_render.height - worky;
    }  

  gimp_display_paint_area (gdisp, workx, worky, workw, workh);

  gimp_display_shell_flush (GIMP_DISPLAY_SHELL (gdisp->shell));

  gdisp->idle_render.x += CHUNK_WIDTH;

  if (gdisp->idle_render.x >=
      gdisp->idle_render.basex + gdisp->idle_render.width)
    {
      gdisp->idle_render.x = gdisp->idle_render.basex;
      gdisp->idle_render.y += CHUNK_HEIGHT;

      if (gdisp->idle_render.y >=
	  gdisp->idle_render.basey + gdisp->idle_render.height)
	{
	  if (! gimp_display_idle_render_next_area (gdisp))
	    {
	      /* FINISHED */
              gdisp->idle_render.idle_id = 0;

	      return FALSE;
	    }
	}
    }

  /* Still work to do. */
  return TRUE;
}

static gboolean
gimp_display_idle_render_next_area (GimpDisplay *gdisp)
{
  GimpArea *ga;
  GSList   *list;
  
  list = gdisp->idle_render.update_areas;

  if (list == NULL)
    {
      return FALSE;
    }

  ga = (GimpArea *) list->data;

  gdisp->idle_render.update_areas =
    g_slist_remove (gdisp->idle_render.update_areas, ga);

  gdisp->idle_render.x = gdisp->idle_render.basex = ga->x1;
  gdisp->idle_render.y = gdisp->idle_render.basey = ga->y1;
  gdisp->idle_render.width = ga->x2 - ga->x1;
  gdisp->idle_render.height = ga->y2 - ga->y1;

  g_free (ga);

  return TRUE;
}

static void
gimp_display_paint_area (GimpDisplay *gdisp,
                         gint         x,
                         gint         y,
                         gint         w,
                         gint         h)
{
  GimpDisplayShell *shell;
  gint              x1, y1, x2, y2;

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /*  Bounds check  */
  x1 = CLAMP (x,     0, gdisp->gimage->width);
  y1 = CLAMP (y,     0, gdisp->gimage->height);
  x2 = CLAMP (x + w, 0, gdisp->gimage->width);
  y2 = CLAMP (y + h, 0, gdisp->gimage->height);
  x = x1;
  y = y1;
  w = (x2 - x1);
  h = (y2 - y1);

  /*  calculate the extents of the update as limited by what's visible  */
  gdisplay_untransform_coords (gdisp,
                               0, 0,
                               &x1, &y1,
                               FALSE, FALSE);
  gdisplay_untransform_coords (gdisp,
                               shell->disp_width, shell->disp_height,
			       &x2, &y2,
                               FALSE, FALSE);

  gimp_image_invalidate (gdisp->gimage, x, y, w, h, x1, y1, x2, y2);

  /*  display the area  */
  gdisplay_transform_coords (gdisp, x, y, &x1, &y1, FALSE);
  gdisplay_transform_coords (gdisp, x + w, y + h, &x2, &y2, FALSE);

  gimp_display_shell_add_expose_area (shell,
                                      x1, y1, (x2 - x1), (y2 - y1));
}
