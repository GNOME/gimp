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

#include <stdlib.h>
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "actionarea.h"
#include "colormaps.h"
#include "cursorutil.h"
#include "dialog_handler.h"
#include "info_dialog.h"
#include "info_window.h"
#include "gdisplay.h"
#include "gximage.h"
#include "interface.h"
#include "scroll.h"
#include "scale.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpunit.h"

#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"


#define MAX_BUF 256

#define PREVIEW_MASK   GDK_EXPOSURE_MASK | \
                       GDK_BUTTON_PRESS_MASK | \
                       GDK_KEY_PRESS_MASK | \
                       GDK_KEY_RELEASE_MASK
                                                          
/* Navigation preview sizes */
#define NAV_PREVIEW_WIDTH  112
#define NAV_PREVIEW_HEIGHT 112
#define BORDER_PEN_WIDTH   3

#define MAX_SCALE_BUF 20


/* Timeout before preview is updated */
#define PREVIEW_UPDATE_TIMEOUT  1100

typedef struct _NavWinData NavWinData;
struct _NavWinData
{
  gboolean   showingPreview;
  gboolean   installedDirtyTimer;
  InfoDialog *info_win;
  GtkWidget *preview;
  void      *gdisp_ptr; /* I'm not happy 'bout this one */
  GtkWidget *previewBox;
  GtkWidget *previewAlign;
  GtkWidget *zoom_label;
  GtkWidget *zoom_scale;
  GtkObject *zoom_adjustment;
  gdouble    ratio;
  GdkGC     *gc;
  gint       dispx;        /* x pos of top left corner of display area */
  gint       dispy;        /* y pos of top left corner of display area */
  gint       dispwidth;    /* width of display area */
  gint       dispheight;   /* height of display area */
  gint       sig_hand_id;
  gboolean   sq_grabbed;   /* In the process of moving the preview square */
  gint       motion_offsetx;
  gint       motion_offsety;
  gint       pwidth;       /* real preview width */
  gint       pheight;      /* real preview height */
  gint       imagewidth;   /* width of the real image */
  gint       imageheight;  /* height of real image */
  gboolean   block_window_marker; /* Block redraws of window marker */
  gint       nav_preview_width;
  gint       nav_preview_height;
  gboolean   block_adj_sig; 
};


static gint
nav_window_preview_events (GtkWidget *,
			    GdkEvent  *,
			    gpointer  *);
static gint
nav_window_expose_events (GtkWidget *,
			   GdkEvent  *,
			   gpointer  *);

#if 0 
static gint
nav_window_preview_resized (GtkWidget      *,
			    GtkAllocation  *,
			    gpointer       *);

#endif /* 0 */

static void
nav_window_update_preview (NavWinData *);

static void 
nav_window_update_preview_blank(NavWinData *iwd);

static void 
destroy_preview_widget     (NavWinData *);

static void 
create_preview_widget      (NavWinData *);

void
nav_window_update_window_marker(InfoDialog *);

static void
nav_window_draw_sqr(NavWinData *,
		    gboolean,
		    gint ,
		    gint ,
		    gint ,
		    gint );

static void
set_size_data(NavWinData *);

static gint 
nav_preview_update_do_timer(NavWinData *);


static void
nav_window_destroy_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  InfoDialog *info_win;
  
  info_win = (InfoDialog *)client_data;
  dialog_unregister(info_win->shell);
}

static void
nav_window_close_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  InfoDialog *info_win;
  NavWinData *iwd;
  
  info_win = (InfoDialog *)client_data;
  iwd = (NavWinData *)info_win->user_data;

  iwd->showingPreview = FALSE;
  info_dialog_popdown ((InfoDialog *) client_data);
}

static void
nav_window_disp_area(NavWinData *iwd,GDisplay *gdisp)
{
  GimpImage *gimage;
  gint       newwidth;
  gint       newheight;
  gdouble    ratio; /* Screen res ratio */
  gboolean   need_update = FALSE;

  /* Calculate preview size */
  gimage = gdisp->gimage;

  ratio = ((gdouble)SCALEDEST(gdisp))/((gdouble)SCALESRC(gdisp));

  iwd->dispx = gdisp->offset_x*iwd->ratio/ratio;
  iwd->dispy = gdisp->offset_y*iwd->ratio/ratio;
  iwd->dispwidth = (gdisp->disp_width*iwd->ratio)/ratio;
  iwd->dispheight = (gdisp->disp_height*iwd->ratio)/ratio;

  newwidth = gimage->width;
  newheight = gimage->height;

  if((iwd->imagewidth > 0 && newwidth != iwd->imagewidth) ||
     (iwd->imageheight > 0 && newheight != iwd->imageheight))
    {
      /* Must change the preview size */
      gtk_window_set_focus(GTK_WINDOW (iwd->info_win->shell),NULL);  
      destroy_preview_widget(iwd);
      create_preview_widget(iwd);
      need_update = TRUE;
    }

  iwd->imagewidth = newwidth;
  iwd->imageheight = newheight;

  /* Normalise */
  iwd->dispwidth = MAX(iwd->dispwidth, 2);
  iwd->dispheight = MAX(iwd->dispheight, 2);

  iwd->dispwidth = MIN(iwd->dispwidth, iwd->pwidth/*-BORDER_PEN_WIDTH*/);
  iwd->dispheight = MIN(iwd->dispheight, iwd->pheight/*-BORDER_PEN_WIDTH*/);

  if(need_update == TRUE)
    {
      gtk_widget_hide(iwd->previewAlign);
      /* ALT nav_window_update_preview(iwd);*/
      nav_window_update_preview_blank(iwd); 
      gtk_widget_show(iwd->preview);
      gtk_widget_draw(iwd->preview, NULL); 
      gtk_widget_show(iwd->previewAlign);
      nav_window_draw_sqr(iwd,FALSE,
			   iwd->dispx,iwd->dispy,
			   iwd->dispwidth,iwd->dispheight);
      gtk_window_set_focus(GTK_WINDOW (iwd->info_win->shell),iwd->preview);  
      gtk_timeout_add(PREVIEW_UPDATE_TIMEOUT,(GtkFunction)nav_preview_update_do_timer,(gpointer)iwd); 
    }
}

static void
nav_window_draw_sqr(NavWinData *iwd,
		    gboolean undraw,
		    gint x,
		    gint y,
		    gint w,
		    gint h)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  gdk_gc_set_function (iwd->gc, GDK_INVERT);
  
  if(undraw)
    {
      /* first undraw from last co-ords */
      gdk_draw_rectangle (iwd->preview->window, iwd->gc, FALSE, 
			  iwd->dispx,iwd->dispy, 
			  iwd->dispwidth-BORDER_PEN_WIDTH+1, 
			  iwd->dispheight-BORDER_PEN_WIDTH+1);
    }

/*   gdk_draw_rectangle (iwd->preview->window, iwd->gc, FALSE,  */
/* 		      10,20,  */
/* 		      40,50); */

  gdk_draw_rectangle (iwd->preview->window, iwd->gc, FALSE, 
		      x,y, 
		      w-BORDER_PEN_WIDTH+1,
		      h-BORDER_PEN_WIDTH+1);

  iwd->dispx = x;
  iwd->dispy = y;
  iwd->dispwidth = w;
  iwd->dispheight = h;
}

static void 
destroy_preview_widget(NavWinData *iwd)
{
  if(!iwd->preview)
    return;

  gtk_widget_hide(iwd->previewBox);
  gtk_widget_destroy(iwd->previewBox);
  iwd->previewBox = NULL;
  iwd->preview = NULL;
}

static void
set_size_data(NavWinData *iwd)
{
  gint sel_width, sel_height;
  gint pwidth, pheight;
  GDisplay *gdisp;
  GimpImage *gimage;

  gdisp = (GDisplay *)(iwd->gdisp_ptr);
  gimage = gdisp->gimage;

  sel_width = gimage->width;
  sel_height = gimage->height;

  if (sel_width > sel_height) {
    pwidth  = MIN(sel_width, iwd->nav_preview_width);
    pheight = sel_height * pwidth / sel_width;
    iwd->ratio = (gdouble)pwidth / ((gdouble)sel_width);
  } else {
    pheight = MIN(sel_height, iwd->nav_preview_height);
    pwidth  = sel_width * pheight / sel_height;
    iwd->ratio = (gdouble)pheight / ((gdouble)sel_height);
  }

  iwd->pwidth = pwidth;
  iwd->pheight = pheight;
}

static void 
create_preview_widget(NavWinData *iwd)
{
  GtkWidget *hbox;  
  GtkWidget *image;
  GtkWidget *frame;
  gdouble ratio;
  GDisplay *gdisp;

  gdisp = (GDisplay *)(iwd->gdisp_ptr);

  ratio = ((gdouble)SCALEDEST(gdisp))/((gdouble)SCALESRC(gdisp));

  hbox = gtk_hbox_new(FALSE,0);
  iwd->previewBox = hbox;
  gtk_widget_show(hbox);
  gtk_container_add(GTK_CONTAINER (iwd->previewAlign),hbox);

  image = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_widget_set_events( GTK_WIDGET(image), PREVIEW_MASK );
  iwd->preview = image;
  gtk_widget_show (image);

  gtk_preview_set_dither (GTK_PREVIEW (image), GDK_RGB_DITHER_MAX);

  set_size_data(iwd);

  gtk_preview_size (GTK_PREVIEW (iwd->preview),
		    iwd->pwidth,
		    iwd->pheight);

  gtk_widget_set_usize (iwd->preview,  
			iwd->pwidth, 
			iwd->pheight); 

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER (iwd->previewBox),frame);
  gtk_container_add (GTK_CONTAINER (frame), iwd->preview);
  gtk_widget_show(frame);

  iwd->sig_hand_id = gtk_signal_connect_after (GTK_OBJECT (image), "expose_event",
					       (GtkSignalFunc) nav_window_expose_events,
					       iwd);

  gtk_signal_connect (GTK_OBJECT (image), "event",
		      (GtkSignalFunc) nav_window_preview_events,
		      iwd);

/*   gtk_signal_connect (GTK_OBJECT (image), "size_allocate", */
/* 		      (GtkSignalFunc) nav_window_preview_resized, */
/* 		      iwd); */

  GTK_WIDGET_SET_FLAGS (image, GTK_CAN_FOCUS);
}

static void
update_real_view(NavWinData *iwd,gint tx,gint ty)
{
  GDisplay *gdisp;
  gdouble ratio;
  gint xoffset;
  gint yoffset;
  gint xpnt;
  gint ypnt;

  gdisp = (GDisplay *) iwd->gdisp_ptr;
  ratio = ((gdouble)SCALEDEST(gdisp))/((gdouble)SCALESRC(gdisp));

  if((tx + iwd->dispwidth) >= iwd->pwidth)
    {
      tx = iwd->pwidth; /* Actually should be less... 
			 * but bound check will save us.
			 */
    }

  xpnt = (gint)(((gdouble)(tx)*ratio)/iwd->ratio);

  if((ty + iwd->dispheight) >= iwd->pheight)
    ty = iwd->pheight; /* Same comment as for xpnt above. */

  ypnt = (gint)(((gdouble)(ty)*ratio)/iwd->ratio);

  xoffset = xpnt - gdisp->offset_x;
  yoffset = ypnt - gdisp->offset_y;
  
  iwd->block_window_marker = TRUE;
  scroll_display(iwd->gdisp_ptr,xoffset,yoffset);
  iwd->block_window_marker = FALSE;
}

static void nav_window_update_preview(NavWinData *iwd)
{
  GDisplay *gdisp;
  TempBuf * preview_buf;
  guchar *src, *buf, *dest;
  gint x,y,has_alpha;
  gint pwidth, pheight;
  GimpImage *gimage;
  gdouble r,g,b,a,chk;
  gint xoff = 0;
  gint yoff = 0;

  gimp_add_busy_cursors();

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  /* Calculate preview size */
  gimage = ((GDisplay *)(iwd->gdisp_ptr))->gimage;

  /* Min size is 2 */
  pwidth = iwd->pwidth;
  pheight = iwd->pheight;


  preview_buf = gimp_image_construct_composite_preview (gimage,
							MAX (pwidth, 2),
							MAX (pheight, 2));
  

  buf = g_new (gchar,  iwd->nav_preview_width * 3);
  src = (gchar *) temp_buf_data (preview_buf);
  has_alpha = (preview_buf->bytes == 2 || preview_buf->bytes == 4);

  for (y = 0; y <pheight ; y++)
    {
      dest = buf;
      switch (preview_buf->bytes)
	{
	case 4:
	      for (x = 0; x < pwidth; x++)
		{
		  r = ((gdouble)(*(src++)))/255.0;
		  g = ((gdouble)(*(src++)))/255.0;
		  b = ((gdouble)(*(src++)))/255.0;
		  a = ((gdouble)(*(src++)))/255.0;
		  chk = ((gdouble)((( (x^y) & 4 ) << 4) | 128))/255.0;
		  *(dest++) = (guchar)((chk + (r - chk)*a)*255.0);
		  *(dest++) = (guchar)((chk + (g - chk)*a)*255.0);
		  *(dest++) = (guchar)((chk + (b - chk)*a)*255.0);
		}
	      break;
	case 2:
	  for (x = 0; x < pwidth; x++)
	    {
	      r = ((gdouble)(*(src++)))/255.0;
	      a = ((gdouble)(*(src++)))/255.0;
	      chk = ((gdouble)((( (x^y) & 4 ) << 4) | 128))/255.0;
	      *(dest++) = (guchar)((chk + (r - chk)*a)*255.0);
	      *(dest++) = (guchar)((chk + (r - chk)*a)*255.0);
	      *(dest++) = (guchar)((chk + (r - chk)*a)*255.0);
	    }
	  break;
	default:
	  g_warning("UNKNOWN TempBuf bpp in nav_window_update_preview()");
	}
      
      gtk_preview_draw_row (GTK_PREVIEW (iwd->preview),
			    (guchar *)buf, xoff, yoff+y, preview_buf->width);
    }

  g_free (buf);
  temp_buf_free (preview_buf);

/*   nav_window_disp_area(iwd,gdisp); */

  gimp_remove_busy_cursors (NULL);
}

static void nav_window_update_preview_blank(NavWinData *iwd)
{
  GDisplay     *gdisp;
  guchar       *buf, *dest;
  gint          x,y;
  GimpImage    *gimage;
  gdouble       chk;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  /* Calculate preview size */
  gimage = ((GDisplay *)(iwd->gdisp_ptr))->gimage;
  
  buf = g_new (gchar,  iwd->pwidth * 3);

  for (y = 0; y < iwd->pheight ; y++)
    {
      dest = buf;
      for (x = 0; x < iwd->pwidth; x++)
	{
	  chk = ((gdouble)((( (x^y) & 4 ) << 4) | 128))/255.0;
	  chk *= 128.0;
	  *(dest++) = (guchar)chk;
	  *(dest++) = (guchar)chk;
	  *(dest++) = (guchar)chk;
	}
      
      gtk_preview_draw_row (GTK_PREVIEW (iwd->preview),
			    (guchar *)buf, 0, y, iwd->pwidth);
    }

  g_free (buf);

  gdk_flush();
  /*  nav_window_disp_area(iwd,gdisp);*/
}

static gint
inside_preview_square(NavWinData *iwd, gint x, gint y)
{
  if(x > iwd->dispx &&
     x < (iwd->dispx + iwd->dispwidth) &&
     y > iwd->dispy &&
     y < iwd->dispy + iwd->dispheight)
    return TRUE;

  return FALSE;
}

static void
update_zoom_label(NavWinData *iwd)
{
  gchar      scale_str[MAX_SCALE_BUF];

  /* Update the zoom scale string */
  g_snprintf (scale_str, MAX_SCALE_BUF, "%d:%d",
	      SCALEDEST (((GDisplay *)iwd->gdisp_ptr)), 
	      SCALESRC (((GDisplay *)iwd->gdisp_ptr)));
  
  gtk_label_set_text(GTK_LABEL(iwd->zoom_label),scale_str);
}

static void 
update_zoom_adjustment(NavWinData *iwd)
{
  GtkAdjustment *adj = GTK_ADJUSTMENT(iwd->zoom_adjustment);
  gdouble f = ((gdouble)SCALEDEST (((GDisplay *)iwd->gdisp_ptr)))/((gdouble)SCALESRC (((GDisplay *)iwd->gdisp_ptr)));
  gdouble val;
  
  if(f < 1.0)
    {
      val = -1/f;
    }
  else
    {
      val = f;
    }

  if(abs((gint)adj->value) != (gint)(val - 1) && iwd->block_adj_sig != TRUE)
    {
      adj->value = val;
      gtk_signal_emit_by_name (GTK_OBJECT (iwd->zoom_adjustment), "changed");
    }
}

static void
move_to_point(NavWinData *iwd,
	      gint tx,
	      gint ty)
{
  if(tx < 0)
    {
      tx = 0;
    }
  
  if(tx > iwd->pwidth)
    {
      tx = iwd->pwidth;
    }
  
  if(ty < 0)
    {
      ty = 0;
    }
  
  if(ty > iwd->pheight)
    {
      ty = iwd->pwidth;
    }
  
  if((tx + iwd->dispwidth) >= iwd->pwidth)
    {
      tx = iwd->pwidth - iwd->dispwidth;
    }
  
  if((ty + iwd->dispheight) >= iwd->pheight)
    {
      ty = iwd->pheight - iwd->dispheight;
    }
  
  /* Update the real display */
  update_real_view(iwd,tx,ty);
  
  nav_window_draw_sqr(iwd,
		      TRUE,
		      tx,ty,
		      iwd->dispwidth,iwd->dispheight);
  
}

static gint
nav_window_preview_events (GtkWidget *widget,
			   GdkEvent  *event,
			   gpointer  *data)
{
  NavWinData *iwd;
  GDisplay *gdisp;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  GdkEventKey    *kevent;
  GdkModifierType mask;
  gint tx = 0,ty = 0; /* So compiler complaints */
  gint mx,my;
  gboolean arrowKey = FALSE;

  iwd = (NavWinData *)data;

  if(!iwd)
    return FALSE;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;
    case GDK_MAP:
      nav_window_update_preview_blank(iwd); 
      gtk_timeout_add(PREVIEW_UPDATE_TIMEOUT,(GtkFunction)nav_preview_update_do_timer,(gpointer)iwd); 
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      tx = bevent->x;
      ty = bevent->y;

      /* Must start the move */

      switch (bevent->button)
	{
	case 1:
	  if(inside_preview_square(iwd,tx,ty))
	    {
	      iwd->motion_offsetx = tx - iwd->dispx;
	      iwd->motion_offsety = ty - iwd->dispy;
	      iwd->sq_grabbed = TRUE;
	      gtk_grab_add(widget);
	      gdk_pointer_grab (widget->window, TRUE,
				GDK_BUTTON_RELEASE_MASK |
				GDK_POINTER_MOTION_HINT_MASK |
				GDK_BUTTON_MOTION_MASK,
				widget->window, NULL, 0);
	    }
	  else
	    {
	      /* Direct scroll to the location */
	      /* view scrolled to the center or nearest possible point */
	      
	      tx -= iwd->dispwidth/2;
	      ty -= iwd->dispheight/2;

	      if(tx < 0)
		tx = 0;

	      if((tx + iwd->dispwidth) > iwd->pwidth)
		tx = iwd->pwidth - iwd->dispwidth;

	      if(ty < 0)
		ty = 0;

	      if((ty + iwd->dispheight) > iwd->pheight)
		ty = iwd->pheight - iwd->dispheight;

	      update_real_view(iwd,tx,ty);

	      nav_window_draw_sqr(iwd,
				  TRUE,
				  tx,ty,
				  iwd->dispwidth,iwd->dispheight);
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
	  iwd->sq_grabbed = FALSE;
	  gtk_grab_remove(widget);
	  gdk_pointer_ungrab (0);
	  gdk_flush();
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
	  gdk_window_raise(gdisp->shell->window);
	  break;
	case GDK_Up:
	  arrowKey = TRUE;
	  tx = iwd->dispx;
	  ty = iwd->dispy - 1;
	  break;
	case GDK_Left:
	  arrowKey = TRUE;
	  tx = iwd->dispx - 1;
	  ty = iwd->dispy;
	  break;
	case GDK_Right:
	  arrowKey = TRUE;
	  tx = iwd->dispx + 1;
	  ty = iwd->dispy;
	  break;
	case GDK_Down:
	  arrowKey = TRUE;
	  tx = iwd->dispx;
	  ty = iwd->dispy + 1;
	  break;
	case GDK_equal:
	  change_scale(gdisp,ZOOMIN);
	  break;
	case GDK_minus:
	  change_scale(gdisp,ZOOMOUT);
	  break;
	default:
	  break;
	}

      if(arrowKey)
	{
	  move_to_point(iwd,tx,ty);
	  return TRUE;
	}
      break;
    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;

      if(!iwd->sq_grabbed)
	break;

      gdk_window_get_pointer (widget->window, &mx, &my, &mask);
      tx = mx;
      ty = my;

      tx = tx - iwd->motion_offsetx;
      ty = ty - iwd->motion_offsety;

      move_to_point(iwd,tx,ty);
     break;

    default:
      break;
    }

  return FALSE;
}

static gint
nav_window_expose_events (GtkWidget *widget,
			  GdkEvent  *event,
			  gpointer  *data)
{
  NavWinData *iwd;
  GDisplay *gdisp;

  iwd = (NavWinData *)data;

  if(!iwd)
    return FALSE;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_signal_handler_block(GTK_OBJECT(widget),iwd->sig_hand_id); 
      gtk_widget_draw(iwd->preview, NULL); 
      gtk_signal_handler_unblock(GTK_OBJECT(widget),iwd->sig_hand_id ); 
      
      nav_window_draw_sqr(iwd,FALSE,
			  iwd->dispx,iwd->dispy,
			  iwd->dispwidth,iwd->dispheight);
      
      break;
    default:
      break;
    }

  return FALSE;
}

static gint 
nav_preview_update_do(NavWinData *iwd)
{
  nav_window_update_preview(iwd);
  nav_window_disp_area(iwd,iwd->gdisp_ptr);
  gtk_widget_draw(iwd->preview, NULL); 
  iwd->installedDirtyTimer = FALSE;
  return FALSE;
}

static gint 
nav_preview_update_do_timer(NavWinData *iwd)
{
  gtk_idle_add((GtkFunction)nav_preview_update_do,(gpointer)iwd);

  return FALSE;
}

static void
nav_image_need_update (GtkObject *obj,
		       gpointer   client_data)
{
  NavWinData *iwd;

  iwd = (NavWinData *)client_data;

  if(!iwd || !iwd->showingPreview || iwd->installedDirtyTimer == TRUE)
    return;

  iwd->installedDirtyTimer = TRUE;

  /* Update preview at a less busy time */
  nav_window_update_preview_blank(iwd); 
  gtk_widget_draw(iwd->preview, NULL); 
  gtk_timeout_add(PREVIEW_UPDATE_TIMEOUT,(GtkFunction)nav_preview_update_do_timer,(gpointer)iwd); 
}

static void
navwindow_zoomin (GtkWidget *widget,
		  gpointer   data)
{
  NavWinData *iwd;
  GDisplay *gdisp;

  iwd = (NavWinData *)data;

  if(!iwd)
    return;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  change_scale(gdisp,ZOOMIN);
}

static void
navwindow_zoomout (GtkWidget *widget,
		   gpointer   data)
{
  NavWinData *iwd;
  GDisplay *gdisp;

  iwd = (NavWinData *)data;

  if(!iwd)
    return;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  change_scale(gdisp,ZOOMOUT);
}

static void
zoom_adj_changed (GtkAdjustment *adj, 
		  gpointer   data)
{
  NavWinData *iwd;
  GDisplay *gdisp;
  gint scalesrc, scaledest;

  iwd = (NavWinData *)data;

  if(!iwd)
    return;
  
  gdisp = (GDisplay *) iwd->gdisp_ptr;

  if(adj->value < 0.0)
    {
      scalesrc = abs((gint)adj->value-1);
      scaledest = 1;
    }
  else
    {
      scaledest = abs((gint)adj->value+1);
      scalesrc = 1;
    }

  iwd->block_adj_sig = TRUE;
  change_scale(gdisp,(scaledest*100)+scalesrc);
  iwd->block_adj_sig = FALSE;
}

static GtkWidget *
nav_create_button_area(InfoDialog *info_win)
{
  GtkWidget *hbox1;  
  GtkWidget *vbox1;
  GtkWidget *button;
  GtkWidget *hscale1;
  GtkWidget *label1;
  GtkObject *adjustment;
  GdkPixmap *pixmap;
  GtkWidget *pixmapwid;
  GdkBitmap *mask;
  GtkStyle  *style;
  NavWinData *iwd;
  gchar      scale_str[MAX_SCALE_BUF];

  iwd = (NavWinData *)info_win->user_data;

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);

  style = gtk_widget_get_style (info_win->shell);

  button = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
		      GTK_SIGNAL_FUNC (navwindow_zoomout), (gpointer) info_win->user_data);
  gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);

  pixmap = gdk_pixmap_create_from_xpm_d (info_win->shell->window, &mask,
					 &style->bg[GTK_STATE_NORMAL], 
					 zoom_out_xpm);
  pixmapwid = gtk_pixmap_new (pixmap, mask);
  gtk_container_add (GTK_CONTAINER (button), pixmapwid);
  gtk_widget_show (pixmapwid);
  gtk_widget_show (button);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

  /*  user zoom ratio  */
  g_snprintf (scale_str, MAX_SCALE_BUF, "%d:%d",
	      SCALEDEST (((GDisplay *)iwd->gdisp_ptr)), 
	      SCALESRC (((GDisplay *)iwd->gdisp_ptr)));
  
  label1 = gtk_label_new (scale_str);
  gtk_widget_show (label1);
  iwd->zoom_label = label1;
  gtk_box_pack_start (GTK_BOX (hbox1), label1, TRUE, TRUE, 0);

  adjustment = gtk_adjustment_new (0.0, -15.0, 16.0, 1.0, 1.0, 1.0);
  hscale1 = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_scale_set_digits (GTK_SCALE (hscale1), 0);
  iwd->zoom_adjustment = adjustment;
  iwd->zoom_scale = hscale1;
  gtk_widget_show (hscale1);

  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (zoom_adj_changed),
		      iwd);

  gtk_box_pack_start (GTK_BOX (vbox1), hscale1, TRUE, TRUE, 0);
  gtk_scale_set_draw_value (GTK_SCALE (hscale1), FALSE);

  button = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
		      GTK_SIGNAL_FUNC (navwindow_zoomin), (gpointer) info_win->user_data); 
  gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);

  pixmap = gdk_pixmap_create_from_xpm_d (info_win->shell->window, &mask,
					 &style->bg[GTK_STATE_NORMAL], 
					 zoom_in_xpm);
  pixmapwid = gtk_pixmap_new (pixmap, mask);
  gtk_container_add (GTK_CONTAINER (button), pixmapwid);
  gtk_widget_show (pixmapwid);
  gtk_widget_show (button);

  return vbox1;
}

static GtkWidget *
info_window_image_preview_new(InfoDialog *info_win)
{
  GtkWidget *hbox1;
  GtkWidget *vbox1;
  GtkWidget *alignment;
  GtkWidget *button_area;

  NavWinData *iwd;

  iwd = (NavWinData *)info_win->user_data;

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);

  gtk_widget_realize(info_win->shell);

  /* need gc to draw the preview sqr with */
  iwd->gc = gdk_gc_new(info_win->shell->window);
  gdk_gc_set_function (iwd->gc, GDK_INVERT);
  gdk_gc_set_line_attributes (iwd->gc, BORDER_PEN_WIDTH, GDK_LINE_SOLID,
			      GDK_CAP_BUTT, GDK_JOIN_ROUND);

  alignment = gtk_alignment_new(0.5,0.5,0.0,0.0);
  iwd->previewAlign = alignment;
  gtk_widget_show (alignment);
  
  /* Create the preview in which to draw the thumbnail image */

  create_preview_widget(iwd);

  gtk_box_pack_start (GTK_BOX (hbox1), alignment, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

  button_area = nav_create_button_area(info_win);
  gtk_box_pack_start (GTK_BOX (vbox1), button_area, TRUE, TRUE, 0);

  return vbox1;
}

static ActionAreaItem action_items[] =
{
  { N_("Close"), nav_window_close_callback, NULL, NULL },
};

InfoDialog *
nav_window_create (void *gdisp_ptr)
{
  InfoDialog *info_win;
  GDisplay   *gdisp;
  NavWinData *iwd;
  GtkWidget  *container;
  char       *title;
  char       *title_buf;
  int type;

  gdisp = (GDisplay *) gdisp_ptr;

  title = g_basename (gimage_filename (gdisp->gimage));
  type = gimage_base_type (gdisp->gimage);

  /*  create the info dialog  */
  title_buf = g_strdup_printf (_("%s: Window Navigation"), title);
  info_win = info_dialog_new (title_buf);
  dialog_register(info_win->shell);
  gtk_signal_connect (GTK_OBJECT (info_win->shell), "destroy",
		      (GtkSignalFunc) nav_window_destroy_callback,
		      info_win);
  g_free (title_buf);
/*   gtk_window_set_policy (GTK_WINDOW (info_win->shell), */
/* 			 FALSE,FALSE,FALSE); */
  
  iwd = (NavWinData *) g_malloc (sizeof (NavWinData));
  info_win->user_data = iwd;
  iwd->info_win = info_win;
  iwd->showingPreview = TRUE;
  iwd->installedDirtyTimer = FALSE;
  iwd->preview = NULL;
  iwd->gdisp_ptr = gdisp_ptr;
  iwd->dispx   = -1;
  iwd->dispy   = -1;
  iwd->dispwidth  = -1;
  iwd->dispheight = -1;
  iwd->imagewidth = -1;
  iwd->imageheight = -1;
  iwd->sq_grabbed = FALSE;
  iwd->ratio = 1.0;
  iwd->block_window_marker = FALSE;
  iwd->nav_preview_width = NAV_PREVIEW_WIDTH;
  iwd->nav_preview_height = NAV_PREVIEW_HEIGHT;
  iwd->block_adj_sig = FALSE;


  /* Add preview */
  container = info_window_image_preview_new(info_win);
/*   gtk_container_set_focus_child(GTK_CONTAINER(container),iwd->preview); */
  gtk_window_set_focus(GTK_WINDOW (info_win->shell),iwd->preview); 
  gtk_table_attach_defaults (GTK_TABLE (info_win->info_table), container, 
 			     0, 2, 0, 1); 
  /* Create the action area  */
  action_items[0].user_data = info_win;
  build_action_area (GTK_DIALOG (info_win->shell), action_items, 1, 0);

  /* Tie into the dirty signal so we can update the preview */
  gtk_signal_connect_after (GTK_OBJECT (gdisp->gimage), "dirty",
			    GTK_SIGNAL_FUNC(nav_image_need_update),iwd);

  /* Also clean signal so undos get caught as well..*/
  gtk_signal_connect_after (GTK_OBJECT (gdisp->gimage), "clean",
			    GTK_SIGNAL_FUNC(nav_image_need_update),iwd);

  return info_win;
}

void
nav_window_update_window_marker(InfoDialog *info_win)
{
  NavWinData *iwd;

  if(!info_win)
    return;

  iwd = (NavWinData *)info_win->user_data;

  if(!iwd || 
     iwd->showingPreview == FALSE || 
     iwd->block_window_marker == TRUE)
    return;

  /* Undraw old size */
  nav_window_draw_sqr(iwd,
		      FALSE,
		      iwd->dispx,
		      iwd->dispy,
		      iwd->dispwidth,iwd->dispheight);

  /* Update to new size */
  nav_window_disp_area(iwd,iwd->gdisp_ptr);
  
  /* and redraw */
  nav_window_draw_sqr(iwd,
		       FALSE,
		       iwd->dispx,
		       iwd->dispy,
		       iwd->dispwidth,iwd->dispheight);
  
  update_zoom_label(iwd);

  update_zoom_adjustment(iwd);
}

void
nav_dialog_popup (InfoDialog *idialog)
{
  NavWinData *iwd;

  if (!idialog)
    return;

  iwd = (NavWinData *)idialog->user_data;
  iwd->showingPreview = TRUE;

  if (! GTK_WIDGET_VISIBLE (idialog->shell))
    {
      gtk_widget_show (idialog->shell);
      nav_window_update_preview_blank(iwd); 
      nav_window_update_window_marker(idialog);
      gtk_timeout_add(PREVIEW_UPDATE_TIMEOUT,(GtkFunction)nav_preview_update_do_timer,(gpointer)iwd); 
    }
}

void
nav_window_free (InfoDialog *info_win)
{
  g_free (info_win->user_data);
  info_dialog_free (info_win);
}
