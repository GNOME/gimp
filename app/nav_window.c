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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets/widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "widgets/gimpnavigationpreview.h"

#include "app_procs.h"
#include "gdisplay.h"
#include "nav_window.h"

#include "app_procs.h"
#include "gimprc.h"
#include "scroll.h"
#include "scale.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"


#define PREVIEW_MASK   GDK_EXPOSURE_MASK        | \
                       GDK_BUTTON_PRESS_MASK    | \
                       GDK_KEY_PRESS_MASK       | \
                       GDK_KEY_RELEASE_MASK     | \
		       GDK_POINTER_MOTION_MASK

                                                          
/* Navigation preview sizes */
#define NAV_PREVIEW_WIDTH  112
#define NAV_PREVIEW_HEIGHT 112
#define BORDER_PEN_WIDTH   3

#define MAX_SCALE_BUF      20

typedef enum
{
  NAV_WINDOW,
  NAV_POPUP
} NavWinType;


struct _NavigationDialog
{
  NavWinType  ptype;

  GtkWidget  *shell;

  GtkWidget  *new_preview;

  GtkWidget  *preview_frame;
  GtkWidget  *zoom_label;
  GtkObject  *zoom_adjustment;
  GtkWidget  *preview;
  GDisplay   *gdisp;        /* I'm not happy 'bout this one */
  GdkGC      *gc;
  gint        dispx;        /* x pos of top left corner of display area */
  gint        dispy;        /* y pos of top left corner of display area */
  gint        dispwidth;    /* width of display area */
  gint        dispheight;   /* height of display area */

  gboolean    sq_grabbed;   /* In the process of moving the preview square */
  gint        motion_offsetx;
  gint        motion_offsety;

  gint        pwidth;       /* real preview width */
  gint        pheight;      /* real preview height */
  gint        imagewidth;   /* width of the real image */
  gint        imageheight;  /* height of real image */
  gdouble     ratio;
  gint        nav_preview_width;
  gint        nav_preview_height;
  gboolean    block_adj_sig; 
  gboolean    frozen;       /* Has the dialog been frozen ? */
  guint       idle_id;
};


static NavigationDialog * nav_dialog_new      (GDisplay         *gdisp,
					       NavWinType        ptype);
static gchar * nav_dialog_title               (GDisplay         *gdisp);

static GtkWidget * nav_create_button_area     (NavigationDialog *nav_dialog);
static void   nav_dialog_close_callback       (GtkWidget        *widget,
					       gpointer          data);
static void   nav_dialog_disp_area            (NavigationDialog *nav_dialog,
					       GDisplay         *gdisp);
static void   nav_dialog_draw_sqr             (NavigationDialog *nav_dialog,
					       gboolean          undraw,
					       gint              x,
					       gint              y,
					       gint              w,
					       gint              h);
static gboolean   nav_dialog_preview_events   (GtkWidget        *widget, 
					       GdkEvent         *event, 
					       gpointer          data);

static void   nav_dialog_idle_update_preview  (NavigationDialog *nav_dialog);
static void   nav_dialog_update_preview       (NavigationDialog *nav_dialog);
static void   nav_dialog_update_preview_blank (NavigationDialog *nav_dialog);

static void   create_preview_widget           (NavigationDialog *nav_dialog);
static void   set_size_data                   (NavigationDialog *nav_dialog);
static void   nav_image_need_update           (GimpImage        *gimage,
					       gpointer          data);

static void   nav_dialog_display_changed      (GimpContext      *context,
					       GDisplay         *gdisp,
					       gpointer          data);

static GDisplay * nav_dialog_get_gdisp        (void);

static void   nav_dialog_grab_pointer         (NavigationDialog *nav_dialog,
					       GtkWidget        *widget);
static void   update_zoom_label               (NavigationDialog *nav_dialog);
static void   update_zoom_adjustment          (NavigationDialog *nav_dialog);


static NavigationDialog *nav_window_auto = NULL;


/*  public functions  */

static void
nav_dialog_marker_changed (GimpNavigationPreview *nav_preview,
			   gint                   x,
			   gint                   y,
			   NavigationDialog      *nav_dialog)
{
  GDisplay *gdisp;
  gdouble   xratio;
  gdouble   yratio;
  gint      xoffset;
  gint      yoffset;

  gdisp = nav_dialog->gdisp;

  xratio = SCALEFACTOR_X (gdisp);
  yratio = SCALEFACTOR_Y (gdisp);

  xoffset = x * xratio - gdisp->offset_x;
  yoffset = y * yratio - gdisp->offset_y;

  scroll_display (gdisp, xoffset, yoffset);
}

static void
nav_dialog_zoom (GimpNavigationPreview *nav_preview,
		 GimpZoomType           direction,
		 NavigationDialog      *nav_dialog)
{
  change_scale (nav_dialog->gdisp, direction);
}

static void
nav_dialog_scroll (GimpNavigationPreview *nav_preview,
		   GdkScrollDirection     direction,
		   NavigationDialog      *nav_dialog)
{
  GtkAdjustment *adj = NULL;
  gdouble        value;

  g_print ("nav_dialog_scroll(%d)\n", direction);

  switch (direction)
    {
    case GDK_SCROLL_LEFT:
    case GDK_SCROLL_RIGHT:
      adj = nav_dialog->gdisp->hsbdata;
      break;

    case GDK_SCROLL_UP:
    case GDK_SCROLL_DOWN:
      adj = nav_dialog->gdisp->vsbdata;
      break;
    }

  g_assert (adj != NULL);

  value = adj->value;

  switch (direction)
    {
    case GDK_SCROLL_LEFT:
    case GDK_SCROLL_UP:
      value -= adj->page_increment / 2;
      break;

    case GDK_SCROLL_RIGHT:
    case GDK_SCROLL_DOWN:
      value += adj->page_increment / 2;
      break;
    }

  value = CLAMP (value, adj->lower, adj->upper - adj->page_size);

  gtk_adjustment_set_value (adj, value);
}

NavigationDialog *
nav_dialog_create (GDisplay *gdisp)
{
  NavigationDialog *nav_dialog;
  GtkWidget        *button_area;
  GtkWidget        *abox;
  gchar            *title;

  title = nav_dialog_title (gdisp);

  nav_dialog = nav_dialog_new (gdisp, NAV_WINDOW);

  nav_dialog->shell = gimp_dialog_new (title, "navigation_dialog",
				       gimp_standard_help_func,
				       "dialogs/navigation_window.html",
				       GTK_WIN_POS_MOUSE,
				       FALSE, TRUE, TRUE,

				       "_delete_event_", 
                                       nav_dialog_close_callback,
				       nav_dialog, NULL, NULL, TRUE, TRUE,

				       NULL);

  g_free (title);

  gtk_dialog_set_has_separator (GTK_DIALOG (nav_dialog->shell), FALSE);
  gtk_widget_hide (GTK_DIALOG (nav_dialog->shell)->action_area);

  g_object_weak_ref (G_OBJECT (nav_dialog->shell),
                     g_free,
                     nav_dialog);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (nav_dialog->shell)->vbox), abox,
		      TRUE, TRUE, 0);
  gtk_widget_show (abox);

  nav_dialog->preview_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (nav_dialog->preview_frame),
			     GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), nav_dialog->preview_frame);
  gtk_widget_show (nav_dialog->preview_frame);

  create_preview_widget (nav_dialog);

  {
    GtkWidget *frame;
    GtkWidget *preview;

    abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (nav_dialog->shell)->vbox), abox,
			TRUE, TRUE, 0);
    gtk_widget_show (abox);

    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
    gtk_container_add (GTK_CONTAINER (abox), frame);
    gtk_widget_show (frame);

    preview = gimp_navigation_preview_new (gdisp->gimage,
					   gimprc.nav_preview_size);
    gtk_container_add (GTK_CONTAINER (frame), preview);
    gtk_widget_show (preview);

    g_signal_connect (G_OBJECT (preview), "marker_changed",
                      G_CALLBACK (nav_dialog_marker_changed),
                      nav_dialog);
    g_signal_connect (G_OBJECT (preview), "zoom",
                      G_CALLBACK (nav_dialog_zoom),
                      nav_dialog);
    g_signal_connect (G_OBJECT (preview), "scroll",
                      G_CALLBACK (nav_dialog_scroll),
                      nav_dialog);

    nav_dialog->new_preview = preview;
  }

  button_area = nav_create_button_area (nav_dialog);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (nav_dialog->shell)->vbox),
		      button_area, FALSE, FALSE, 0);
  gtk_widget_show (button_area);

  if (! g_object_get_data (G_OBJECT (gdisp->gimage), "nav_handlers_installed"))
    {
      g_signal_connect (G_OBJECT (gdisp->gimage), "dirty",
                        G_CALLBACK (nav_image_need_update),
                        nav_dialog);
      g_signal_connect (G_OBJECT (gdisp->gimage), "clean",
                        G_CALLBACK (nav_image_need_update),
                        nav_dialog);

      g_object_set_data (G_OBJECT (gdisp->gimage), "nav_handlers_installed",
                         nav_dialog);
    }
  
  return nav_dialog;
}

void
nav_dialog_free (GDisplay         *del_gdisp,
		 NavigationDialog *nav_dialog)
{
  /* So this functions works both ways..
   * it will come in here with nav_dialog == null
   * if the auto mode is on...
   */

  if (! nav_dialog)
    {
      if (nav_window_auto != NULL)
	{
	  GDisplay *gdisp;

	  nav_dialog = nav_window_auto;

	  /* Only freeze if we are displaying the image we have deleted */
	  if (nav_dialog->gdisp != del_gdisp)
	    return;

	  if (nav_dialog->idle_id)
	    {
	      g_source_remove (nav_dialog->idle_id);
	      nav_dialog->idle_id = 0;
	    }

	  gdisp = nav_dialog_get_gdisp ();

	  if (gdisp)
	    {
	      nav_dialog_display_changed (NULL, gdisp, nav_window_auto);
	    }
	  else
	    {
	      /* Clear window and freeze */
	      nav_dialog->frozen = TRUE;
	      nav_dialog_update_preview_blank (nav_window_auto); 
	      gtk_window_set_title (GTK_WINDOW (nav_window_auto->shell), 
				    _("Navigation: No Image"));

	      gtk_widget_set_sensitive (nav_window_auto->shell, FALSE);
	      nav_window_auto->gdisp = NULL;
	      gtk_widget_hide (GTK_WIDGET (nav_window_auto->shell));
	    }
	}

      return;
    }

  if (nav_dialog->idle_id)
    {
      g_source_remove (nav_dialog->idle_id);
      nav_dialog->idle_id = 0;
    }

  gtk_widget_destroy (nav_dialog->shell);
}

void
nav_dialog_popup (NavigationDialog *nav_dialog)
{
  if (! nav_dialog)
    return;

  if (! GTK_WIDGET_VISIBLE (nav_dialog->shell))
    {
      gtk_widget_show (nav_dialog->shell);

      nav_dialog_idle_update_preview (nav_dialog);
    }

  gdk_window_raise (nav_dialog->shell->window);
}

void
nav_dialog_follow_auto (void)
{
  GimpContext *context;
  GDisplay    *gdisp;

  context = gimp_get_user_context (the_gimp);

  gdisp = gimp_context_get_display (context);

  if (! gdisp)
    return;

  if (! nav_window_auto)
    {
      nav_window_auto = nav_dialog_create (gdisp);

      g_signal_connect (G_OBJECT (context), "display_changed",
                        G_CALLBACK (nav_dialog_display_changed),
                        nav_window_auto);
    }

  nav_dialog_popup (nav_window_auto);

  gtk_widget_set_sensitive (nav_window_auto->shell, TRUE);

  nav_window_auto->frozen = FALSE;
}

void
nav_dialog_update_window_marker (NavigationDialog *nav_dialog)
{
  /* So this functions works both ways..
   * it will come in here with info_win == null
   * if the auto mode is on...
   */

  if (! nav_dialog && nav_window_auto)
    {
      nav_dialog = nav_window_auto;

      if (nav_dialog->frozen)
	return;

      nav_dialog_update_window_marker (nav_window_auto);

      return;
    }
  else if (! nav_dialog)
    {
      return;
    }

  if (! GTK_WIDGET_VISIBLE (nav_dialog->shell))
    return;

  update_zoom_label (nav_dialog);
  update_zoom_adjustment (nav_dialog);

  /* Undraw old size */
  nav_dialog_draw_sqr (nav_dialog,
		       FALSE,
		       nav_dialog->dispx, nav_dialog->dispy,
		       nav_dialog->dispwidth, nav_dialog->dispheight);

  /* Update to new size */
  nav_dialog_disp_area (nav_dialog, nav_dialog->gdisp);

  /* and redraw */
  nav_dialog_draw_sqr (nav_dialog,
		       FALSE,
		       nav_dialog->dispx, nav_dialog->dispy,
		       nav_dialog->dispwidth, nav_dialog->dispheight);
}

void 
nav_dialog_preview_resized (NavigationDialog *nav_dialog)
{
  if (! nav_dialog)
    return;

  /* force regeneration of the widgets
   * bit of a fiddle... could cause if the image really is 1x1
   * but the preview would not really matter in that case.
   */
  nav_dialog->imagewidth = 1;
  nav_dialog->imageheight = 1;

  nav_dialog->nav_preview_width = 
    (gimprc.nav_preview_size < 0 || gimprc.nav_preview_size > 256) ? 
    NAV_PREVIEW_WIDTH : gimprc.nav_preview_size;

  nav_dialog->nav_preview_height = 
    (gimprc.nav_preview_size < 0 || gimprc.nav_preview_size > 256) ? 
    NAV_PREVIEW_HEIGHT : gimprc.nav_preview_size;

  nav_dialog_update_window_marker (nav_dialog);
}

void 
nav_popup_click_handler (GtkWidget      *widget, 
			 GdkEventButton *event, 
			 gpointer        data)
{
  GdkEventButton   *bevent;
  GDisplay         *gdisp = data;
  NavigationDialog *nav_dialog;
  gint              x, y;
  gint              x_org, y_org;
  gint              scr_w, scr_h;

  bevent = (GdkEventButton *) event;

  if (! gdisp->nav_popup)
    {
      GtkWidget *frame;

      gdisp->nav_popup = nav_dialog = nav_dialog_new (gdisp, NAV_POPUP);

      nav_dialog->shell = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_widget_set_events (nav_dialog->shell, PREVIEW_MASK);

      g_object_weak_ref (G_OBJECT (nav_dialog->shell),
                         g_free,
                         nav_dialog);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (nav_dialog->shell), frame);
      gtk_widget_show (frame);

      nav_dialog->preview_frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (nav_dialog->preview_frame),
				 GTK_SHADOW_IN);
      gtk_container_add (GTK_CONTAINER (frame), nav_dialog->preview_frame);
      gtk_widget_show (nav_dialog->preview_frame);

      create_preview_widget (nav_dialog);

      gtk_widget_set_extension_events (nav_dialog->preview,
				       GDK_EXTENSION_EVENTS_ALL);

      nav_dialog_disp_area (nav_dialog, nav_dialog->gdisp);
    }
  else
    {
      nav_dialog = gdisp->nav_popup;

      gtk_widget_hide (nav_dialog->shell);

      nav_dialog_disp_area (nav_dialog, nav_dialog->gdisp);
      nav_dialog_update_preview (nav_dialog); 
    }

  /* decide where to put the popup */
  gdk_window_get_origin (widget->window, &x_org, &y_org);

  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();

  x = x_org + bevent->x - nav_dialog->dispx - 
    ((nav_dialog->dispwidth - BORDER_PEN_WIDTH + 1) * 0.5) - 2;
  y = y_org + bevent->y - nav_dialog->dispy - 
    ((nav_dialog->dispheight - BORDER_PEN_WIDTH + 1)* 0.5) - 2;

  /* If the popup doesn't fit into the screen, we have a problem.
   * We move the popup onscreen and risk that the pointer is not
   * in the square representing the viewable area anymore. Moving
   * the pointer will make the image scroll by a large amount,
   * but then it works as usual. Probably better than a popup that
   * is completely unusable in the lower right of the screen.
   *
   * Warping the pointer would be another solution ... 
   */
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + nav_dialog->pwidth  > scr_w) ? scr_w - nav_dialog->pwidth  - 2: x;
  y = (y + nav_dialog->pheight > scr_h) ? scr_h - nav_dialog->pheight - 2: y;

  gtk_widget_set_uposition (nav_dialog->shell, x, y);
  gtk_widget_show (nav_dialog->shell);

  gdk_flush();

  /* fill in then set up handlers for mouse motion etc */
  nav_dialog->motion_offsetx =
    (nav_dialog->dispwidth  - BORDER_PEN_WIDTH + 1) * 0.5;

  nav_dialog->motion_offsety =
    (nav_dialog->dispheight - BORDER_PEN_WIDTH + 1) * 0.5;

  if (GTK_WIDGET_VISIBLE (nav_dialog->preview))
    nav_dialog_grab_pointer (nav_dialog, nav_dialog->preview);
}


/*  private functions  */

static NavigationDialog *
nav_dialog_new (GDisplay   *gdisp,
		NavWinType  ptype)
{
  NavigationDialog *nav_dialog;

  nav_dialog = g_new0 (NavigationDialog, 1);

  nav_dialog->ptype               = ptype;
  nav_dialog->shell               = NULL;
  nav_dialog->preview             = NULL;
  nav_dialog->zoom_label          = NULL;
  nav_dialog->zoom_adjustment     = NULL;
  nav_dialog->gdisp               = gdisp;
  nav_dialog->dispx               = -1;
  nav_dialog->dispy               = -1;
  nav_dialog->dispwidth           = -1;
  nav_dialog->dispheight          = -1;
  nav_dialog->imagewidth          = -1;
  nav_dialog->imageheight         = -1;
  nav_dialog->sq_grabbed          = FALSE;
  nav_dialog->ratio               = 1.0;

  nav_dialog->nav_preview_width   = 
    (gimprc.nav_preview_size < 0 || gimprc.nav_preview_size > 256) ? 
    NAV_PREVIEW_WIDTH : gimprc.nav_preview_size;

  nav_dialog->nav_preview_height  = 
    (gimprc.nav_preview_size < 0 || gimprc.nav_preview_size > 256) ? 
    NAV_PREVIEW_HEIGHT : gimprc.nav_preview_size;

  nav_dialog->block_adj_sig       = FALSE;
  nav_dialog->frozen              = FALSE;
  nav_dialog->idle_id             = 0;

  return nav_dialog;
}

static gchar *
nav_dialog_title (GDisplay *gdisp)
{
  const gchar *basename;
  gchar *title;

  basename = g_basename (gimp_image_filename (gdisp->gimage));

  title = g_strdup_printf (_("Navigation: %s-%d.%d"), 
			   basename,
			   gimp_image_get_ID (gdisp->gimage),
			   gdisp->instance);

  return title;
}

static void
nav_dialog_close_callback (GtkWidget *widget,
			   gpointer   data)
{
  NavigationDialog *nav_dialog;

  nav_dialog = (NavigationDialog *) data;

  gtk_widget_hide (nav_dialog->shell);
}

static void
nav_dialog_disp_area (NavigationDialog *nav_dialog,
		      GDisplay         *gdisp)
{
  GimpImage *gimage;
  gint       newwidth;
  gint       newheight;
  gdouble    xratio;
  gdouble    yratio;     /* Screen res ratio */
  gboolean   need_update = FALSE;

  /* Calculate preview size */
  gimage = gdisp->gimage;

  xratio = SCALEFACTOR_X (gdisp);
  yratio = SCALEFACTOR_Y (gdisp);

  if (nav_dialog->new_preview)
    {
      if (GIMP_PREVIEW (nav_dialog->new_preview)->dot_for_dot !=
	  gdisp->dot_for_dot)
	gimp_preview_set_dot_for_dot (GIMP_PREVIEW (nav_dialog->new_preview),
				      gdisp->dot_for_dot);

      gimp_navigation_preview_set_marker
	(GIMP_NAVIGATION_PREVIEW (nav_dialog->new_preview),
	 RINT (gdisp->offset_x    / xratio),
	 RINT (gdisp->offset_y    / yratio),
	 RINT (gdisp->disp_width  / xratio),
	 RINT (gdisp->disp_height / yratio));
    }

  nav_dialog->dispx = gdisp->offset_x * nav_dialog->ratio / xratio + 0.5;
  nav_dialog->dispy = gdisp->offset_y * nav_dialog->ratio/yratio + 0.5;

  nav_dialog->dispwidth  = (gdisp->disp_width  * nav_dialog->ratio) / xratio + 0.5;
  nav_dialog->dispheight = (gdisp->disp_height * nav_dialog->ratio) / yratio + 0.5;

  newwidth  = gimage->width;
  newheight = gimage->height;

  if (! gdisp->dot_for_dot)
    {
      newwidth =
	(newwidth * gdisp->gimage->yresolution) / gdisp->gimage->xresolution;

      nav_dialog->dispx =
	((gdisp->offset_x * 
	  gdisp->gimage->yresolution * nav_dialog->ratio) / 
	 (gdisp->gimage->xresolution *  xratio)) + 0.5;     /*here*/

      nav_dialog->dispwidth =
	((gdisp->disp_width * 
	  gdisp->gimage->yresolution * nav_dialog->ratio) / 
	 (gdisp->gimage->xresolution *  xratio)) + 0.5; /*here*/
    }

  if ((nav_dialog->imagewidth  > 0 && newwidth  != nav_dialog->imagewidth) ||
      (nav_dialog->imageheight > 0 && newheight != nav_dialog->imageheight))
    {
      /* Must change the preview size */

      if (nav_dialog->ptype != NAV_POPUP)
	{
	  gtk_window_set_focus (GTK_WINDOW (nav_dialog->shell), NULL);  
	}

      gtk_widget_destroy (nav_dialog->preview);
      create_preview_widget (nav_dialog);

      need_update = TRUE;
    }

  nav_dialog->imagewidth  = newwidth;
  nav_dialog->imageheight = newheight;

  /* Normalise */
  nav_dialog->dispwidth  = MAX (nav_dialog->dispwidth,  2);
  nav_dialog->dispheight = MAX (nav_dialog->dispheight, 2);

  nav_dialog->dispwidth  = MIN (nav_dialog->dispwidth,  nav_dialog->pwidth);
  nav_dialog->dispheight = MIN (nav_dialog->dispheight, nav_dialog->pheight);

  if (need_update)
    {
      if (nav_dialog->ptype != NAV_POPUP)
	{
	  gtk_window_set_focus (GTK_WINDOW (nav_dialog->shell),
				nav_dialog->preview);

	  nav_dialog_idle_update_preview (nav_dialog);
	}
      else
	{
	  nav_dialog_update_preview (nav_dialog);

	  gtk_widget_draw (nav_dialog->preview, NULL); 
	}
    }
}

static void
nav_dialog_draw_sqr (NavigationDialog *nav_dialog,
		     gboolean          undraw,
		     gint              x,
		     gint              y,
		     gint              w,
		     gint              h)
{
  if (! nav_dialog->gc)
    {
      if (! GTK_WIDGET_REALIZED (nav_dialog->shell))
	gtk_widget_realize (nav_dialog->shell);

      nav_dialog->gc = gdk_gc_new (nav_dialog->shell->window);

      gdk_gc_set_function (nav_dialog->gc, GDK_INVERT);
      gdk_gc_set_line_attributes (nav_dialog->gc, BORDER_PEN_WIDTH, 
				  GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
    }

  if (undraw)
    {
      if (nav_dialog->dispx != 0 ||
	  nav_dialog->dispy != 0 ||
	  nav_dialog->pwidth  != nav_dialog->dispwidth ||
	  nav_dialog->pheight != nav_dialog->dispheight)
	{
	  /* first undraw from last co-ords */
	  gdk_draw_rectangle (nav_dialog->preview->window, nav_dialog->gc,
			      FALSE, 
			      nav_dialog->dispx,
			      nav_dialog->dispy, 
			      nav_dialog->dispwidth  - BORDER_PEN_WIDTH + 1, 
			      nav_dialog->dispheight - BORDER_PEN_WIDTH + 1);
	}
    }

  if (x != 0 || y != 0 ||
      w != nav_dialog->pwidth || h != nav_dialog->pheight)
    {
      gdk_draw_rectangle (nav_dialog->preview->window, nav_dialog->gc,
			  FALSE, 
			  x, y, 
			  w - BORDER_PEN_WIDTH + 1,
			  h - BORDER_PEN_WIDTH + 1);
    }

  nav_dialog->dispx      = x;
  nav_dialog->dispy      = y;
  nav_dialog->dispwidth  = w;
  nav_dialog->dispheight = h;
}

static void
set_size_data (NavigationDialog *nav_dialog)
{
  GDisplay  *gdisp;
  GimpImage *gimage;
  gint       sel_width;
  gint       sel_height;
  gint       pwidth;
  gint       pheight;

  gdisp  = nav_dialog->gdisp;
  gimage = gdisp->gimage;

  sel_width  = gimage->width;
  sel_height = gimage->height;

  if (! gdisp->dot_for_dot)
    sel_width = 
      (sel_width * gdisp->gimage->yresolution) / gdisp->gimage->xresolution;

  if (sel_width > sel_height) 
    {
      pwidth  = nav_dialog->nav_preview_width;

      nav_dialog->ratio = (gdouble) pwidth / (gdouble) sel_width;
    } 
  else 
    {
      pheight = nav_dialog->nav_preview_height;

      nav_dialog->ratio = (gdouble) pheight / (gdouble) sel_height;
    }

  pwidth  = sel_width  * nav_dialog->ratio + 0.5;
  pheight = sel_height * nav_dialog->ratio + 0.5;

  nav_dialog->pwidth  = pwidth;
  nav_dialog->pheight = pheight;
}

static void 
create_preview_widget (NavigationDialog *nav_dialog)
{
  GtkWidget *preview;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_widget_set_events (GTK_WIDGET (preview), PREVIEW_MASK);
  GTK_WIDGET_SET_FLAGS (preview, GTK_CAN_FOCUS);
  gtk_preview_set_dither (GTK_PREVIEW (preview), GDK_RGB_DITHER_MAX);
  gtk_container_add (GTK_CONTAINER (nav_dialog->preview_frame), preview);
  gtk_widget_show (preview);

  nav_dialog->preview = preview;

  set_size_data (nav_dialog);

  gtk_preview_size (GTK_PREVIEW (preview), 
                    nav_dialog->pwidth, nav_dialog->pheight);

  g_signal_connect (G_OBJECT (preview), "event",
                    G_CALLBACK (nav_dialog_preview_events),
                    nav_dialog);
}

static gboolean
nav_dialog_update_preview_idle_func (NavigationDialog *nav_dialog)
{
  nav_dialog->idle_id = 0;

  /* If the gdisp has gone then don't do anything in this timer */
  if (! nav_dialog->gdisp)
    return FALSE;

  nav_dialog_update_preview (nav_dialog);
  nav_dialog_disp_area (nav_dialog, nav_dialog->gdisp);
  gtk_widget_queue_draw (nav_dialog->preview); 

  return FALSE;
}

static void
nav_dialog_idle_update_preview (NavigationDialog *nav_dialog)
{
  if (nav_dialog->idle_id)
    return;

  nav_dialog->idle_id =
    g_idle_add_full (G_PRIORITY_LOW,
		     (GSourceFunc) nav_dialog_update_preview_idle_func,
		     nav_dialog,
		     NULL);
}

static void
nav_dialog_update_preview (NavigationDialog *nav_dialog)
{
  GDisplay  *gdisp;
  TempBuf   *preview_buf;
  TempBuf   *preview_buf_ptr;
  TempBuf   *preview_buf_notdot = NULL;
  guchar    *src, *buf, *dest;
  gint       x, y;
  gint       pwidth, pheight;
  GimpImage *gimage;
  gdouble    r, g, b, a, chk;
  gint       xoff = 0;
  gint       yoff = 0;

  gdisp  = nav_dialog->gdisp;
  gimage = gdisp->gimage;

  gimp_set_busy (gimage->gimp); 

  /* Min size is 2 */
  pwidth  = nav_dialog->pwidth;
  pheight = nav_dialog->pheight;

  /* we need a large normal preview which we will cut down later.
   *  gimp_image_construct_composite_preview() can't cope with 
   *  dot_for_dot not been set.
   */
  if (! gdisp->dot_for_dot)
    {
      gint    sel_width  = gimage->width;
      gint    sel_height = gimage->height;
      gdouble tratio;

      if (sel_width > sel_height)
	tratio = (gdouble) nav_dialog->nav_preview_width  / ((gdouble) sel_width);
      else
	tratio = (gdouble) nav_dialog->nav_preview_height / ((gdouble) sel_height);

      pwidth  = sel_width  * tratio + 0.5;
      pheight = sel_height * tratio + 0.5;
    }

  if (nav_dialog->ratio > 1.0)    /*  Preview is scaling up!  */
    {
      TempBuf *tmp;

      tmp = gimp_viewable_get_new_preview (GIMP_VIEWABLE (gimage),
					   gimage->width,
					   gimage->height);
      preview_buf = temp_buf_scale (tmp, 
				    pwidth, 
				    pheight);
      temp_buf_free (tmp);
    }
  else
    {
      preview_buf = gimp_viewable_get_new_preview (GIMP_VIEWABLE (gimage),
						   MAX (pwidth, 2),
						   MAX (pheight, 2));
    }

  /* reset & get new preview
   *
   * FIXME: should use gimp_preview_scale()
   */
  if (! gdisp->dot_for_dot)
    {
      gint     loop1, loop2;
      gdouble  x_ratio, y_ratio;
      guchar  *src_data;
      guchar  *dest_data;

      preview_buf_notdot = temp_buf_new (nav_dialog->pwidth,
					 nav_dialog->pheight,
					 preview_buf->bytes, 
					 0, 0, NULL);

      x_ratio = (gdouble) pwidth  / (gdouble) nav_dialog->pwidth;
      y_ratio = (gdouble) pheight / (gdouble) nav_dialog->pheight;

      src_data  = temp_buf_data (preview_buf);
      dest_data = temp_buf_data (preview_buf_notdot);

      for (loop1 = 0 ; loop1 < nav_dialog->pheight ; loop1++)
	for (loop2 = 0 ; loop2 < nav_dialog->pwidth ; loop2++)
	  {
	    gint    i;
	    guchar *src_pixel;
	    guchar *dest_pixel;

	    src_pixel = src_data +
	      ((gint) (loop2 * x_ratio)) * preview_buf->bytes +
	      ((gint) (loop1 * y_ratio)) * pwidth * preview_buf->bytes;

	    dest_pixel = dest_data +
	      (loop2 + loop1 * nav_dialog->pwidth) * preview_buf->bytes;

	    for (i = 0 ; i < preview_buf->bytes; i++)
	      *dest_pixel++ = *src_pixel++;
	  }

      pwidth  = nav_dialog->pwidth;
      pheight = nav_dialog->pheight;

      src = temp_buf_data (preview_buf_notdot);
      preview_buf_ptr = preview_buf_notdot;
    }
  else
    {
      src = temp_buf_data (preview_buf);
      preview_buf_ptr = preview_buf;
    }
  
  buf = g_new (gchar, preview_buf_ptr->width * 3);

  for (y = 0; y < preview_buf_ptr->height ; y++)
    {
      dest = buf;
      switch (preview_buf_ptr->bytes)
	{
	case 4:
	  for (x = 0; x < preview_buf_ptr->width; x++)
	    {
	      r = ((gdouble) (*(src++))) / 255.0;
	      g = ((gdouble) (*(src++))) / 255.0;
	      b = ((gdouble) (*(src++))) / 255.0;
	      a = ((gdouble) (*(src++))) / 255.0;
	      chk = ((gdouble) ((( (x^y) & 4 ) << 4) | 128)) / 255.0;
	      *(dest++) = (guchar) ((chk + (r - chk) * a) * 255.0);
	      *(dest++) = (guchar) ((chk + (g - chk) * a) * 255.0);
	      *(dest++) = (guchar) ((chk + (b - chk) * a) * 255.0);
	    }
	  break;

	case 2:
	  for (x = 0; x < preview_buf_ptr->width; x++)
	    {
	      r = ((gdouble) (*(src++))) / 255.0;
	      a = ((gdouble) (*(src++))) / 255.0;
	      chk = ((gdouble) ((( (x^y) & 4 ) << 4) | 128)) / 255.0;
	      *(dest++) = (guchar) ((chk + (r - chk) * a) * 255.0);
	      *(dest++) = (guchar) ((chk + (r - chk) * a) * 255.0);
	      *(dest++) = (guchar) ((chk + (r - chk) * a) * 255.0);
	    }
	  break;

	default:
	  g_warning ("nav_dialog_update_preview(): UNKNOWN TempBuf bpp");
	}
      
      gtk_preview_draw_row (GTK_PREVIEW (nav_dialog->preview),
			    buf, 
			    xoff, yoff + y, 
			    preview_buf_ptr->width);
    }

  g_free (buf);

  temp_buf_free (preview_buf);

  if (preview_buf_notdot)
    temp_buf_free (preview_buf_notdot);

  gimp_unset_busy (gimage->gimp);
}

static void 
nav_dialog_update_preview_blank (NavigationDialog *nav_dialog)
{
  guchar  *buf;
  guchar  *dest;
  gint     x, y;
  gdouble  chk;

  buf = g_new (gchar,  nav_dialog->pwidth * 3);

  for (y = 0; y < nav_dialog->pheight ; y++)
    {
      dest = buf;

      for (x = 0; x < nav_dialog->pwidth; x++)
	{
	  chk = ((gdouble) ((( (x^y) & 4 ) << 4) | 128)) / 255.0;
	  chk *= 128.0;

	  *(dest++) = (guchar) chk;
	  *(dest++) = (guchar) chk;
	  *(dest++) = (guchar) chk;
	}
      
      gtk_preview_draw_row (GTK_PREVIEW (nav_dialog->preview),
			    buf, 
			    0, y, nav_dialog->pwidth);
    }

  g_free (buf);
}

static void
update_zoom_label (NavigationDialog *nav_dialog)
{
  gchar scale_str[MAX_SCALE_BUF];

  if (! nav_dialog->zoom_label)
    return;

  /* Update the zoom scale string */
  g_snprintf (scale_str, MAX_SCALE_BUF, "%d:%d",
	      SCALEDEST (nav_dialog->gdisp), 
	      SCALESRC (nav_dialog->gdisp));

  gtk_label_set_text (GTK_LABEL (nav_dialog->zoom_label), scale_str);
}

static void 
update_zoom_adjustment (NavigationDialog *nav_dialog)
{
  GtkAdjustment *adj;
  gdouble        f;
  gdouble        val;

  if (! nav_dialog->zoom_adjustment)
    return;

  adj = GTK_ADJUSTMENT (nav_dialog->zoom_adjustment);

  f = (((gdouble) SCALEDEST (nav_dialog->gdisp)) / 
       ((gdouble) SCALESRC (nav_dialog->gdisp)));
  
  if (f < 1.0)
    {
      val = -1.0 / f;
    }
  else
    {
      val = f;
    }

  if (abs ((gint) adj->value) != (gint) (val - 1) &&
      ! nav_dialog->block_adj_sig)
    {
      adj->value = val;

      gtk_adjustment_changed (GTK_ADJUSTMENT (nav_dialog->zoom_adjustment));
    }
}

static void
nav_dialog_update_real_view (NavigationDialog *nav_dialog,
			     gint              tx,
			     gint              ty)
{
  GDisplay *gdisp;
  gdouble   xratio;
  gdouble   yratio;
  gint      xoffset;
  gint      yoffset;
  gint      xpnt;
  gint      ypnt;

  gdisp = nav_dialog->gdisp;

  xratio = SCALEFACTOR_X (gdisp);
  yratio = SCALEFACTOR_Y (gdisp);

  if ((tx + nav_dialog->dispwidth) >= nav_dialog->pwidth)
    {
      tx = nav_dialog->pwidth; /* Actually should be less... 
				* but bound check will save us.
				*/
    }

  xpnt = (gint) (((gdouble) (tx) * xratio) / nav_dialog->ratio + 0.5);

  if ((ty + nav_dialog->dispheight) >= nav_dialog->pheight)
    {
      ty = nav_dialog->pheight; /* Same comment as for xpnt above. */
    }

  ypnt = (gint) (((gdouble) (ty) * yratio) / nav_dialog->ratio + 0.5);

  if (! gdisp->dot_for_dot) /* here */
    xpnt = ((gdouble) xpnt * 
	    gdisp->gimage->xresolution) / gdisp->gimage->yresolution + 0.5;

  xoffset = xpnt - gdisp->offset_x;
  yoffset = ypnt - gdisp->offset_y;

  scroll_display (nav_dialog->gdisp, xoffset, yoffset);
}

static void
nav_dialog_move_to_point (NavigationDialog *nav_dialog,
			  gint              tx,
			  gint              ty)
{
  tx = CLAMP (tx, 0, nav_dialog->pwidth);
  ty = CLAMP (ty, 0, nav_dialog->pheight);
  
  if ((tx + nav_dialog->dispwidth) >= nav_dialog->pwidth)
    {
      tx = nav_dialog->pwidth - nav_dialog->dispwidth;
    }
  
  if ((ty + nav_dialog->dispheight) >= nav_dialog->pheight)
    {
      ty = nav_dialog->pheight - nav_dialog->dispheight;
    }

  if (nav_dialog->dispx == tx && nav_dialog->dispy == ty)
    return;

  nav_dialog_update_real_view (nav_dialog, tx, ty);
}

static void
nav_dialog_grab_pointer (NavigationDialog *nav_dialog,
			 GtkWidget        *widget)
{
  GdkCursor *cursor;

  nav_dialog->sq_grabbed = TRUE;

  gtk_grab_add (widget);

  cursor = gdk_cursor_new (GDK_CROSSHAIR);

  gdk_pointer_grab (widget->window, TRUE,
		    GDK_BUTTON_RELEASE_MASK |
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON_MOTION_MASK |
		    GDK_EXTENSION_EVENTS_ALL,
		    widget->window, cursor, 0);

  gdk_cursor_destroy (cursor); 
}

static gboolean
nav_dialog_inside_preview_square (NavigationDialog *nav_dialog,
				  gint              x,
				  gint              y)
{
  return (x > nav_dialog->dispx &&
          x < nav_dialog->dispx + nav_dialog->dispwidth &&
          y > nav_dialog->dispy &&
          y < nav_dialog->dispy + nav_dialog->dispheight);
}

static gboolean
nav_dialog_preview_events (GtkWidget *widget,
			   GdkEvent  *event,
			   gpointer   data)
{
  NavigationDialog *nav_dialog;
  GDisplay         *gdisp;
  GdkEventButton   *bevent;
  GdkEventMotion   *mevent;
  GdkEventKey      *kevent;
  GdkModifierType   mask;
  gint              tx = 0;
  gint              ty = 0;
  gint              mx;
  gint              my;
  gboolean          arrow_key = FALSE;

  nav_dialog = (NavigationDialog *) data;

  if (! nav_dialog || nav_dialog->frozen)
    return FALSE;

  gdisp = nav_dialog->gdisp;

  switch (event->type)
    {
    case GDK_EXPOSE:
      /* call default handler explicitly, then draw the square */
      GTK_WIDGET_GET_CLASS (widget)->expose_event (widget, 
                                                   (GdkEventExpose *) event);
      nav_dialog_draw_sqr (nav_dialog, FALSE,
			   nav_dialog->dispx, nav_dialog->dispy,
			   nav_dialog->dispwidth, nav_dialog->dispheight);
      return TRUE;

    case GDK_MAP:
      if (nav_dialog->ptype == NAV_POPUP)
	{
	  nav_dialog_update_preview (nav_dialog);

	  /* First time displayed.... get the pointer! */
	  nav_dialog_grab_pointer (nav_dialog, nav_dialog->preview);
	}
      else
	{
	  nav_dialog_idle_update_preview (nav_dialog); 
	}
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      tx = bevent->x;
      ty = bevent->y;

      /* Must start the move */
      switch (bevent->button)
	{
	case 1:
	  if (! nav_dialog_inside_preview_square (nav_dialog, tx, ty))
	    {
	      /* Direct scroll to the location
	       * view scrolled to the center or nearest possible point
	       */
	      tx -= nav_dialog->dispwidth  / 2;
	      ty -= nav_dialog->dispheight / 2;

	      nav_dialog_move_to_point (nav_dialog, tx, ty);

	      nav_dialog->motion_offsetx = nav_dialog->dispwidth  / 2;
	      nav_dialog->motion_offsety = nav_dialog->dispheight / 2;
	    }
	  else
	    {
	      nav_dialog->motion_offsetx = tx - nav_dialog->dispx;
	      nav_dialog->motion_offsety = ty - nav_dialog->dispy;
	    }

	  nav_dialog_grab_pointer (nav_dialog, widget);

	  break;

	  /*  wheelmouse support  */
	case 4:
	  if (bevent->state & GDK_SHIFT_MASK)
	    {
	      change_scale (gdisp, GIMP_ZOOM_IN);
	    }
	  else
	    {
	      GtkAdjustment *adj =
		(bevent->state & GDK_CONTROL_MASK) ?
		gdisp->hsbdata : gdisp->vsbdata;
	      gfloat new_value = adj->value - adj->page_increment / 2;
	      new_value =
		CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	      gtk_adjustment_set_value (adj, new_value);
	    }
	  break;

	case 5:
	  if (bevent->state & GDK_SHIFT_MASK)
	    {
	      change_scale (gdisp, GIMP_ZOOM_OUT);
	    }
	  else
	    {
	      GtkAdjustment *adj =
		(bevent->state & GDK_CONTROL_MASK) ?
		gdisp->hsbdata : gdisp->vsbdata;
	      gfloat new_value = adj->value + adj->page_increment / 2;
	      new_value = CLAMP (new_value,
				 adj->lower, adj->upper - adj->page_size);
	      gtk_adjustment_set_value (adj, new_value);
	    }
	  break;

	default:
	  break;
	}
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;
      tx = bevent->x;
      ty = bevent->y;

      switch (bevent->button)
	{
	case 1:
	  nav_dialog->sq_grabbed = FALSE;
	  gtk_grab_remove (widget);
	  gdk_pointer_ungrab (0);
	  if (nav_dialog->ptype == NAV_POPUP)
	    {
	      gtk_widget_hide (nav_dialog->shell);
	    }
	  gdk_flush ();
	  break;
	default:
	  break;
	}
      break;

    case GDK_KEY_PRESS:
      /* hack for the update preview... needs to be fixed */
      kevent = (GdkEventKey *) event;

      switch (kevent->keyval)
	{
	case GDK_space:
	  gdk_window_raise (gdisp->shell->window);
	  break;
	case GDK_Up:
	  arrow_key = TRUE;
	  tx = nav_dialog->dispx;
	  ty = nav_dialog->dispy - 1;
	  break;
	case GDK_Left:
	  arrow_key = TRUE;
	  tx = nav_dialog->dispx - 1;
	  ty = nav_dialog->dispy;
	  break;
	case GDK_Right:
	  arrow_key = TRUE;
	  tx = nav_dialog->dispx + 1;
	  ty = nav_dialog->dispy;
	  break;
	case GDK_Down:
	  arrow_key = TRUE;
	  tx = nav_dialog->dispx;
	  ty = nav_dialog->dispy + 1;
	  break;
	case GDK_equal:
	  change_scale (gdisp, GIMP_ZOOM_IN);
	  break;
	case GDK_minus:
	  change_scale (gdisp, GIMP_ZOOM_OUT);
	  break;
	default:
	  break;
	}

      if (arrow_key)
	{
	  nav_dialog_move_to_point (nav_dialog, tx, ty);
	  return TRUE;
	}
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;

      if (! nav_dialog->sq_grabbed)
	break;

      gdk_window_get_pointer (widget->window, &mx, &my, &mask);
      tx = mx;
      ty = my;

      tx = tx - nav_dialog->motion_offsetx;
      ty = ty - nav_dialog->motion_offsety;

      nav_dialog_move_to_point (nav_dialog, tx, ty);
     break;

    default:
      break;
    }

  return FALSE;
}


static void
nav_image_need_update (GimpImage *gimage,
		       gpointer   data)
{
  NavigationDialog *nav_dialog;

  nav_dialog = (NavigationDialog *) data;

  if (! nav_dialog                             ||
      ! GTK_WIDGET_VISIBLE (nav_dialog->shell) ||
      nav_dialog->frozen)
    return;

  nav_dialog_idle_update_preview (nav_dialog); 
}

static void
navwindow_zoomin (GtkWidget *widget,
		  gpointer   data)
{
  NavigationDialog *nav_dialog;

  nav_dialog = (NavigationDialog *) data;

  if(! nav_dialog || nav_dialog->frozen)
    return;

  change_scale (nav_dialog->gdisp, GIMP_ZOOM_IN);
}

static void
navwindow_zoomout (GtkWidget *widget,
		   gpointer   data)
{
  NavigationDialog *nav_dialog;

  nav_dialog = (NavigationDialog *)data;

  if (! nav_dialog || nav_dialog->frozen)
    return;

  change_scale (nav_dialog->gdisp, GIMP_ZOOM_OUT);
}

static void
zoom_adj_changed (GtkAdjustment *adj, 
		  gpointer       data)
{
  NavigationDialog *nav_dialog;
  gint              scalesrc;
  gint              scaledest;

  nav_dialog = (NavigationDialog *)data;

  if (! nav_dialog || nav_dialog->frozen)
    return;
  
  if (adj->value < 0.0)
    {
      scalesrc = abs ((gint) adj->value - 1);
      scaledest = 1;
    }
  else
    {
      scaledest = abs ((gint) adj->value + 1);
      scalesrc = 1;
    }

  nav_dialog->block_adj_sig = TRUE;
  change_scale (nav_dialog->gdisp, (scaledest * 100) + scalesrc);
  nav_dialog->block_adj_sig = FALSE;
}

static GtkWidget *
nav_create_button_area (NavigationDialog *nav_dialog)
{
  GtkWidget  *hbox;  
  GtkWidget  *vbox;
  GtkWidget  *button;
  GtkWidget  *hscale;
  gchar       scale_str[MAX_SCALE_BUF];

  vbox = gtk_vbox_new (FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gimp_pixmap_button_new (zoom_out_xpm, NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked", 
                    G_CALLBACK (navwindow_zoomout),
                    nav_dialog);

  /*  user zoom ratio  */
  g_snprintf (scale_str, MAX_SCALE_BUF, "%d:%d",
	      SCALEDEST (nav_dialog->gdisp), 
	      SCALESRC (nav_dialog->gdisp));
  
  nav_dialog->zoom_label = gtk_label_new (scale_str);
  gtk_box_pack_start (GTK_BOX (hbox), nav_dialog->zoom_label, TRUE, TRUE, 0);
  gtk_widget_show (nav_dialog->zoom_label);

  button = gimp_pixmap_button_new (zoom_in_xpm, NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked", 
                    G_CALLBACK (navwindow_zoomin),
                    nav_dialog);

  nav_dialog->zoom_adjustment =
    gtk_adjustment_new (0.0, -15.0, 16.0, 1.0, 1.0, 1.0);

  g_signal_connect (G_OBJECT (nav_dialog->zoom_adjustment), "value_changed",
                    G_CALLBACK (zoom_adj_changed),
                    nav_dialog);

  hscale = gtk_hscale_new (GTK_ADJUSTMENT (nav_dialog->zoom_adjustment));
  gtk_scale_set_draw_value (GTK_SCALE (hscale), FALSE);
  gtk_scale_set_digits (GTK_SCALE (hscale), 0);
  gtk_box_pack_start (GTK_BOX (vbox), hscale, TRUE, TRUE, 0);
  gtk_widget_show (hscale);

  return vbox;
}

static void
nav_dialog_display_changed (GimpContext *context,
			    GDisplay    *gdisp,
			    gpointer     data)
{
  GDisplay         *old_gdisp;
  GimpImage        *gimage;
  NavigationDialog *nav_dialog;
  gchar            *title;

  nav_dialog = (NavigationDialog *) data;

  old_gdisp  = nav_dialog->gdisp;

  if (! nav_dialog || gdisp == old_gdisp || ! gdisp)
    return;

  gtk_widget_set_sensitive (nav_window_auto->shell, TRUE);

  nav_dialog->frozen = FALSE;

  title = nav_dialog_title (gdisp);

  gtk_window_set_title (GTK_WINDOW (nav_dialog->shell), title);

  g_free (title);

  gimage = gdisp->gimage;

  if (gimage && gimp_container_have (context->gimp->images,
				     GIMP_OBJECT (gimage)))
    {
      nav_dialog->gdisp = gdisp;

      /* Update preview to new display */
      nav_dialog_preview_resized (nav_dialog);

      if (nav_dialog->new_preview)
	gimp_preview_set_viewable (GIMP_PREVIEW (nav_dialog->new_preview),
				   GIMP_VIEWABLE (gdisp->gimage));

      /* Tie into the dirty signal so we can update the preview
       * provided we haven't already
       */
      if (! g_object_get_data (G_OBJECT (gimage), "nav_handlers_installed"))
	{
	  g_signal_connect (G_OBJECT (gimage), "dirty",
                            G_CALLBACK (nav_image_need_update),
                            nav_dialog);
	  g_signal_connect (G_OBJECT (gimage), "clean",
                            G_CALLBACK (nav_image_need_update),
                            nav_dialog);

	  g_object_set_data (G_OBJECT (gimage),
                             "nav_handlers_installed",
                             nav_dialog);
	}
    }
}

static GDisplay *
nav_dialog_get_gdisp (void)
{
  GList *list;

  for (list = GIMP_LIST (the_gimp->images)->list;
       list;
       list = g_list_next (list))
    {
      GimpImage *gimage;
      GDisplay  *gdisp;

      gimage = GIMP_IMAGE (list->data);

      gdisp = gdisplays_check_valid (NULL, gimage);

      if (gdisp)
	return gdisp;
    }

  return NULL;
}
