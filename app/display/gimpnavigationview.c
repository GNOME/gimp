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

#include <gdk/gdkkeysyms.h>

#include "appenv.h"
#include "cursorutil.h"
#include "dialog_handler.h"
#include "info_dialog.h"
#include "info_window.h"
#include "gdisplay.h"
#include "gimpcontext.h"
#include "gimpset.h"
#include "gimprc.h"
#include "gimpui.h"
#include "gximage.h"
#include "nav_window.h"
#include "scroll.h"
#include "scale.h"

#include "libgimp/gimpunit.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"


#define MAX_BUF 256

#define PREVIEW_MASK   GDK_EXPOSURE_MASK        | \
                       GDK_BUTTON_PRESS_MASK    | \
                       GDK_KEY_PRESS_MASK       | \
                       GDK_KEY_RELEASE_MASK     | \
		       GDK_POINTER_MOTION_MASK

                                                          
/* Navigation preview sizes */
#define NAV_PREVIEW_WIDTH  112
#define NAV_PREVIEW_HEIGHT 112
#define BORDER_PEN_WIDTH   3

#define MAX_SCALE_BUF 20

typedef enum
{
  NAV_WINDOW,
  NAV_POPUP
} NavWinType;

/* Timeout before preview is updated */
#define PREVIEW_UPDATE_TIMEOUT  1100

typedef struct _NavWinData NavWinData;
struct _NavWinData
{
  NavWinType ptype;
  gboolean   showingPreview;
  gboolean   installedDirtyTimer;
  InfoDialog *info_win;
  GtkWidget *previewBox;
  GtkWidget *previewAlign;
  GtkWidget *zoom_label;
  GtkObject *zoom_adjustment;
  GtkWidget *preview;
  void      *gdisp_ptr;    /* I'm not happy 'bout this one */
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
  gdouble    ratio;
  gboolean   block_window_marker; /* Block redraws of window marker */
  gint       nav_preview_width;
  gint       nav_preview_height;
  gboolean   block_adj_sig; 
  gboolean   frozen;       /* Has the dialog been frozen ? */
  guint      timer_id;
};


static gint nav_window_preview_events       (GtkWidget *, GdkEvent *, gpointer *);
static gint nav_window_expose_events        (GtkWidget *, GdkEvent *, gpointer *);
static void nav_window_update_preview       (NavWinData *);
static void nav_window_update_preview_blank (NavWinData *iwd);
static void destroy_preview_widget          (NavWinData *);
static void create_preview_widget           (NavWinData *);
static void nav_window_draw_sqr             (NavWinData *, gboolean, 
					     gint, gint, gint, gint );
static void set_size_data                   (NavWinData *);
static gint nav_preview_update_do_timer     (NavWinData *);



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
nav_window_disp_area (NavWinData *iwd,
		      GDisplay   *gdisp)
{
  GimpImage *gimage;
  gint       newwidth;
  gint       newheight;
  gdouble    xratio;
  gdouble    yratio;     /* Screen res ratio */
  gboolean   need_update = FALSE;

  /* Calculate preview size */
  gimage = gdisp->gimage;

  xratio = SCALEFACTOR_X(gdisp);
  yratio = SCALEFACTOR_Y(gdisp);

  iwd->dispx = gdisp->offset_x * iwd->ratio / xratio + 0.5;
  iwd->dispy = gdisp->offset_y * iwd->ratio/yratio + 0.5;
  iwd->dispwidth = (gdisp->disp_width * iwd->ratio) / xratio + 0.5;
  iwd->dispheight = (gdisp->disp_height * iwd->ratio) / yratio + 0.5;

  newwidth = gimage->width;
  newheight = gimage->height;

  if(!gdisp->dot_for_dot)
    {
      newwidth = (newwidth * gdisp->gimage->yresolution)/ gdisp->gimage->xresolution;
      iwd->dispx = ((gdisp->offset_x * gdisp->gimage->yresolution * iwd->ratio)/ (gdisp->gimage->xresolution *  xratio)) + 0.5; /* here */
      iwd->dispwidth = ((gdisp->disp_width * gdisp->gimage->yresolution * iwd->ratio)/ (gdisp->gimage->xresolution *  xratio)) + 0.5; /* here */
    }

  if((iwd->imagewidth > 0 && newwidth != iwd->imagewidth) ||
     (iwd->imageheight > 0 && newheight != iwd->imageheight))
    {
      /* Must change the preview size */
      if(iwd->ptype != NAV_POPUP)
	{
	  gtk_window_set_focus(GTK_WINDOW (iwd->info_win->shell),NULL);  
	}
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
      nav_window_update_preview_blank(iwd); 
      gtk_widget_show(iwd->preview);
      gtk_widget_draw(iwd->preview, NULL); 
      gtk_widget_show(iwd->previewAlign);
      nav_window_draw_sqr(iwd,FALSE,
			   iwd->dispx,iwd->dispy,
			   iwd->dispwidth,iwd->dispheight);
      if(iwd->ptype != NAV_POPUP)
	{
	  gtk_window_set_focus(GTK_WINDOW (iwd->info_win->shell),iwd->preview);  
	  iwd->timer_id = 
	    gtk_timeout_add(PREVIEW_UPDATE_TIMEOUT,(GtkFunction)nav_preview_update_do_timer,(gpointer)iwd); 
	}
      else
	{
	  nav_window_update_preview(iwd);
	  gtk_widget_draw(iwd->preview, NULL); 
	}
    }
}

static void
nav_window_draw_sqr (NavWinData *iwd,
		     gboolean    undraw,
		     gint        x,
		     gint        y,
		     gint        w,
		     gint        h)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  gdk_gc_set_function (iwd->gc, GDK_INVERT);

  if(undraw)
    {
      /* first undraw from last co-ords */
      gdk_draw_rectangle (iwd->preview->window, iwd->gc, FALSE, 
			  iwd->dispx, iwd->dispy, 
			  iwd->dispwidth-BORDER_PEN_WIDTH+1, 
			  iwd->dispheight-BORDER_PEN_WIDTH+1);
    }

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
destroy_preview_widget (NavWinData *iwd)
{
  if(!iwd->preview)
    return;

  gtk_widget_hide(iwd->previewBox);
  gtk_widget_destroy(iwd->previewBox);
  iwd->previewBox = NULL;
  iwd->preview = NULL;
}

static void
set_size_data (NavWinData *iwd)
{
  gint sel_width, sel_height;
  gint pwidth, pheight;
  GDisplay *gdisp;
  GimpImage *gimage;

  gdisp = (GDisplay *)(iwd->gdisp_ptr);
  gimage = gdisp->gimage;

  sel_width = gimage->width;
  sel_height = gimage->height;

  if(!gdisp->dot_for_dot)
    sel_width = (sel_width * gdisp->gimage->yresolution)/ gdisp->gimage->xresolution;

  if (sel_width > sel_height) {
    pwidth  = iwd->nav_preview_width;
    /*     pheight  = sel_height * pwidth / sel_width; */
    iwd->ratio = (gdouble)pwidth / ((gdouble)sel_width);
    pheight = sel_height * iwd->ratio + 0.5;
    iwd->ratio = (gdouble)pheight/(gdouble)sel_height;
    pwidth  = sel_width * iwd->ratio + 0.5;
  } else {
    pheight = iwd->nav_preview_height;
/*     pwidth  = sel_width * pheight / sel_height; */
    iwd->ratio = (gdouble)pheight / ((gdouble)sel_height);
    pwidth = sel_width * iwd->ratio + 0.5;
    iwd->ratio = (gdouble)pwidth/(gdouble)sel_width;
    pheight = sel_height * iwd->ratio + 0.5;
  }

  iwd->pwidth = pwidth;
  iwd->pheight = pheight;
}

static void 
create_preview_widget (NavWinData *iwd)
{
  GtkWidget *hbox;  
  GtkWidget *image;
  GtkWidget *frame;
  GDisplay *gdisp;

  gdisp = (GDisplay *)(iwd->gdisp_ptr);

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
update_real_view (NavWinData *iwd,
		  gint        tx,
		  gint        ty)
{
  GDisplay *gdisp;
  gdouble xratio;
  gdouble yratio;
  gint xoffset;
  gint yoffset;
  gint xpnt;
  gint ypnt;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  xratio = SCALEFACTOR_X(gdisp);
  yratio = SCALEFACTOR_Y(gdisp);

  if((tx + iwd->dispwidth) >= iwd->pwidth)
    {
      tx = iwd->pwidth; /* Actually should be less... 
			 * but bound check will save us.
			 */
    }

  xpnt = (gint)(((gdouble)(tx)*xratio)/iwd->ratio+0.5);

  if((ty + iwd->dispheight) >= iwd->pheight)
    ty = iwd->pheight; /* Same comment as for xpnt above. */

  ypnt = (gint)(((gdouble)(ty)*yratio)/iwd->ratio+0.5);

  if (!gdisp->dot_for_dot) /* here */
      xpnt = ((gdouble)xpnt * gdisp->gimage->xresolution) / gdisp->gimage->yresolution + 0.5;

  xoffset = xpnt - gdisp->offset_x;
  yoffset = ypnt - gdisp->offset_y;
  
  iwd->block_window_marker = TRUE;
  scroll_display(iwd->gdisp_ptr,xoffset,yoffset);
  iwd->block_window_marker = FALSE;
}

static void 
nav_window_update_preview (NavWinData *iwd)
{
  GDisplay *gdisp;
  TempBuf * preview_buf;
  TempBuf * preview_buf_ptr;
  TempBuf * preview_buf_notdot = NULL;
  guchar *src, *buf, *dest;
  gint x, y;
  gint pwidth, pheight;
  GimpImage *gimage;
  gdouble r, g, b, a, chk;
  gint xoff = 0;
  gint yoff = 0;

  gimp_add_busy_cursors(); 

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  /* Calculate preview size */
  gimage = ((GDisplay *)(iwd->gdisp_ptr))->gimage;

  /* Min size is 2 */
  pwidth = iwd->pwidth;
  pheight = iwd->pheight;

  /* we need a large normal preview which we will cut down later.
   *  gimp_image_construct_composite_preview() can't cope with 
   *  dot_for_dot not been set.
   */
  if (!gdisp->dot_for_dot) /* ALT */ 
    {
      gint sel_width  = gimage->width;
      gint sel_height = gimage->height;
      gdouble tratio;
      
      if (sel_width > sel_height) {
	pwidth  = iwd->nav_preview_width;
	tratio = (gdouble)pwidth / ((gdouble)sel_width);
	pheight = sel_height * tratio + 0.5;
	tratio = (gdouble)pheight / (gdouble)sel_height;
	pwidth  = sel_width * tratio + 0.5;
      } else {
	pheight = iwd->nav_preview_height;
	tratio = (gdouble)pheight / ((gdouble)sel_height);
	pwidth = sel_width * tratio + 0.5;
	tratio = (gdouble)pwidth / (gdouble)sel_width;
	pheight = sel_height * tratio + 0.5;
      }
    }

  preview_buf = gimp_image_construct_composite_preview (gimage,
							MAX (pwidth, 2),
							MAX (pheight, 2));
  /* reset & get new preview */
  if (!gdisp->dot_for_dot)
    {
      gint loop1,loop2;
      gdouble x_ratio,y_ratio;
      guchar *src_data;
      guchar *dest_data;

      preview_buf_notdot = temp_buf_new(iwd->pwidth,
					iwd->pheight,
					preview_buf->bytes,0,0,NULL);

      x_ratio   = (gdouble)pwidth  / (gdouble)iwd->pwidth;
      y_ratio   = (gdouble)pheight / (gdouble)iwd->pheight;
      src_data  = temp_buf_data (preview_buf);
      dest_data = temp_buf_data (preview_buf_notdot);

      for (loop1 = 0 ; loop1 < iwd->pheight ; loop1++)
	for (loop2 = 0 ; loop2 < iwd->pwidth ; loop2++)
	  {
	    gint i;
	    guchar *src_pixel = src_data +
	      ((gint)(loop2 * x_ratio)) * preview_buf->bytes +
	      ((gint)(loop1 * y_ratio)) * pwidth*preview_buf->bytes;
	    guchar *dest_pixel = dest_data + 
	      (loop2 + loop1 * iwd->pwidth) * preview_buf->bytes;

	    for(i = 0 ; i < preview_buf->bytes; i++)
	      *dest_pixel++ = *src_pixel++;
	  }

      pwidth = iwd->pwidth;
      pheight = iwd->pheight;
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
	      r = ((gdouble)(*(src++))) / 255.0;
	      g = ((gdouble)(*(src++))) / 255.0;
	      b = ((gdouble)(*(src++))) / 255.0;
	      a = ((gdouble)(*(src++))) / 255.0;
	      chk = ((gdouble)((( (x^y) & 4 ) << 4) | 128)) / 255.0;
	      *(dest++) = (guchar)((chk + (r - chk) * a) * 255.0);
	      *(dest++) = (guchar)((chk + (g - chk) * a) * 255.0);
	      *(dest++) = (guchar)((chk + (b - chk) * a) * 255.0);
	    }
	  break;
	case 2:
	  for (x = 0; x < preview_buf_ptr->width; x++)
	    {
	      r = ((gdouble)(*(src++))) / 255.0;
	      a = ((gdouble)(*(src++))) / 255.0;
	      chk = ((gdouble)((( (x^y) & 4 ) << 4) | 128)) / 255.0;
	      *(dest++) = (guchar)((chk + (r - chk) * a) * 255.0);
	      *(dest++) = (guchar)((chk + (r - chk) * a) * 255.0);
	      *(dest++) = (guchar)((chk + (r - chk) * a) * 255.0);
	    }
	  break;
	default:
	  g_warning("UNKNOWN TempBuf bpp in nav_window_update_preview()");
	}
      
      gtk_preview_draw_row (GTK_PREVIEW (iwd->preview),
			    (guchar *)buf, xoff, yoff + y, preview_buf_ptr->width);
    }

  g_free (buf);
  temp_buf_free (preview_buf);

  if (preview_buf_notdot)
    temp_buf_free (preview_buf_notdot);

  gimp_remove_busy_cursors (NULL);
}

static void 
nav_window_update_preview_blank (NavWinData *iwd)
{

  guchar       *buf, *dest;
  gint          x,y;
  gdouble       chk;

#if 0 
  GimpImage    *gimage;
  GDisplay     *gdisp;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  /* Calculate preview size */
  gimage = ((GDisplay *)(iwd->gdisp_ptr))->gimage;

#endif /* 0 */
  
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
}

static gint
inside_preview_square (NavWinData *iwd,
		       gint        x,
		       gint        y)
{
  if(x > iwd->dispx &&
     x < (iwd->dispx + iwd->dispwidth) &&
     y > iwd->dispy &&
     y < iwd->dispy + iwd->dispheight)
    return TRUE;

  return FALSE;
}

static void
update_zoom_label (NavWinData *iwd)
{
  gchar      scale_str[MAX_SCALE_BUF];

  if(!iwd->zoom_label)
    return;

  /* Update the zoom scale string */
  g_snprintf (scale_str, MAX_SCALE_BUF, "%d:%d",
	      SCALEDEST (((GDisplay *)iwd->gdisp_ptr)), 
	      SCALESRC (((GDisplay *)iwd->gdisp_ptr)));
  
  gtk_label_set_text(GTK_LABEL(iwd->zoom_label),scale_str);
}

static void 
update_zoom_adjustment (NavWinData *iwd)
{
  GtkAdjustment *adj;
  gdouble f;
  gdouble val;

  if(!iwd->zoom_adjustment)
    return;

  adj = GTK_ADJUSTMENT(iwd->zoom_adjustment);
  f = ((gdouble)SCALEDEST (((GDisplay *)iwd->gdisp_ptr)))/((gdouble)SCALESRC (((GDisplay *)iwd->gdisp_ptr)));
  
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
move_to_point (NavWinData *iwd,
	       gint        tx,
	       gint        ty)
{
  tx = CLAMP (tx, 0, iwd->pwidth);
  ty = CLAMP (ty, 0, iwd->pheight);
  
  if ((tx + iwd->dispwidth) >= iwd->pwidth)
    {
      tx = iwd->pwidth - iwd->dispwidth;
    }
  
  if ((ty + iwd->dispheight) >= iwd->pheight)
    {
      ty = iwd->pheight - iwd->dispheight;
    }
  
  if (iwd->dispx == tx && iwd->dispy == ty)
    return;

  /* Update the real display */
  update_real_view (iwd, tx, ty);
  
  nav_window_draw_sqr (iwd,
		       TRUE,
		       tx, ty,
		       iwd->dispwidth, iwd->dispheight);
  
}

static void
nav_window_grab_pointer (NavWinData *iwd,
			 GtkWidget  *widget)
{
  GdkCursor *cursor;
  int ret;

  iwd->sq_grabbed = TRUE;
  gtk_grab_add (widget);
  cursor = gdk_cursor_new (GDK_CROSSHAIR); 
  ret = gdk_pointer_grab (widget->window, TRUE,
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_POINTER_MOTION_HINT_MASK |
			  GDK_BUTTON_MOTION_MASK |
			  GDK_EXTENSION_EVENTS_ALL,
			  widget->window, cursor, 0);

  gdk_cursor_destroy (cursor); 
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

  if(!iwd || iwd->frozen == TRUE)
    return FALSE;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;
    case GDK_MAP:
      if(iwd->ptype == NAV_POPUP)
	{
	  nav_window_update_preview(iwd);
	  /* First time displayed.... get the pointer! */
	  nav_window_grab_pointer(iwd,iwd->preview);
	}
      else
	{
	  nav_window_update_preview_blank(iwd); 
	  iwd->timer_id = 
	    gtk_timeout_add(PREVIEW_UPDATE_TIMEOUT,(GtkFunction)nav_preview_update_do_timer,(gpointer)iwd); 
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
	  if(!inside_preview_square(iwd,tx,ty))
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
				  tx, ty,
				  iwd->dispwidth,iwd->dispheight);
	      iwd->motion_offsetx = iwd->dispwidth/2;
	      iwd->motion_offsety = iwd->dispheight/2;
	    }
	  else
	    {
	      iwd->motion_offsetx = tx - iwd->dispx;
	      iwd->motion_offsety = ty - iwd->dispy;
	    }
	  
	  nav_window_grab_pointer (iwd, widget);

	  break;

	  /*  wheelmouse support  */
	case 4:
	  if (bevent->state & GDK_SHIFT_MASK)
	    {
	      change_scale (gdisp, ZOOMIN);
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
	      change_scale (gdisp, ZOOMOUT);
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
	  iwd->sq_grabbed = FALSE;
	  gtk_grab_remove(widget);
	  gdk_pointer_ungrab (0);
	  if(iwd->ptype == NAV_POPUP)
	    {
	      gtk_widget_hide(gdisp->nav_popup);
	    }
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

  if(!iwd || iwd->frozen == TRUE)
    return FALSE;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  switch (event->type)
    {
    case GDK_EXPOSE:
      /* we must have the grab if in nav_popup*/
      gtk_signal_handler_block(GTK_OBJECT(widget),iwd->sig_hand_id); 
      gtk_widget_draw(iwd->preview, NULL); 
      gtk_signal_handler_unblock(GTK_OBJECT(widget),iwd->sig_hand_id ); 
      
      nav_window_draw_sqr(iwd,FALSE,
			  iwd->dispx,iwd->dispy,
			  iwd->dispwidth,iwd->dispheight);
      if(!gtk_grab_get_current() && iwd->ptype == NAV_POPUP)
	nav_window_grab_pointer(iwd,iwd->preview);
      break;
    default:
      break;
    }

  return FALSE;
}

static gint 
nav_preview_update_do (NavWinData *iwd)
{
  /* If the gdisp_ptr has gone then don't do anything in this timer */
  if(!iwd->gdisp_ptr)
    return FALSE;
  nav_window_update_preview(iwd);
  nav_window_disp_area(iwd,iwd->gdisp_ptr);
  gtk_widget_queue_draw(iwd->preview); 
  iwd->installedDirtyTimer = FALSE;
  return FALSE;
}

static gint 
nav_preview_update_do_timer(NavWinData *iwd)
{
  iwd->timer_id = 0;
  gtk_idle_add((GtkFunction)nav_preview_update_do,(gpointer)iwd);

  return FALSE;
}

static void
nav_image_need_update (GtkObject *obj,
		       gpointer   client_data)
{
  NavWinData *iwd;

  iwd = (NavWinData *)client_data;

  if(!iwd || 
     !iwd->showingPreview || 
     iwd->installedDirtyTimer == TRUE ||
     iwd->frozen == TRUE)
    return;

  iwd->installedDirtyTimer = TRUE;

  /* Update preview at a less busy time */
  nav_window_update_preview_blank(iwd); 
  gtk_widget_draw(iwd->preview, NULL); 
  iwd->timer_id = 
    gtk_timeout_add(PREVIEW_UPDATE_TIMEOUT,(GtkFunction)nav_preview_update_do_timer,(gpointer)iwd); 
}

static void
navwindow_zoomin (GtkWidget *widget,
		  gpointer   data)
{
  NavWinData *iwd;
  GDisplay *gdisp;

  iwd = (NavWinData *)data;

  if(!iwd || iwd->frozen == TRUE)
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

  if(!iwd || iwd->frozen == TRUE)
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

  if(!iwd || iwd->frozen == TRUE)
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
nav_create_button_area (InfoDialog *info_win)
{
  GtkWidget  *hbox1;  
  GtkWidget  *vbox1;
  GtkWidget  *button;
  GtkWidget  *hscale1;
  GtkWidget  *label1;
  GtkObject  *adjustment;
  NavWinData *iwd;
  gchar       scale_str[MAX_SCALE_BUF];

  iwd = (NavWinData *) info_win->user_data;

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);

  button = gimp_pixmap_button_new (zoom_out_xpm, NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
		      GTK_SIGNAL_FUNC (navwindow_zoomout),
		      (gpointer) info_win->user_data);
  gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
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
  gtk_widget_show (hscale1);

  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (zoom_adj_changed),
		      iwd);

  gtk_box_pack_start (GTK_BOX (vbox1), hscale1, TRUE, TRUE, 0);
  gtk_scale_set_draw_value (GTK_SCALE (hscale1), FALSE);

  button = gimp_pixmap_button_new (zoom_in_xpm, NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
		      GTK_SIGNAL_FUNC (navwindow_zoomin),
		      (gpointer) info_win->user_data); 
  gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return vbox1;
}

static GtkWidget *
create_preview_containers (NavWinData *iwd,
			   GtkWidget  *shell)
{
  GtkWidget *hbox1;
  GtkWidget *vbox1;
  GtkWidget *alignment;

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);

  gtk_widget_realize(shell);

  /* need gc to draw the preview sqr with */
  iwd->gc = gdk_gc_new(shell->window);
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

  return vbox1;
}

static GtkWidget *
info_window_image_preview_new (InfoDialog *info_win)
{
  GtkWidget *button_area;
  GtkWidget *vbox1;

  NavWinData *iwd;

  iwd = (NavWinData *)info_win->user_data;

  vbox1 = create_preview_containers(iwd,info_win->shell);

  /* This one has (may) have the buttons in */
  button_area = nav_create_button_area(info_win);
  gtk_box_pack_start (GTK_BOX (vbox1), button_area, TRUE, TRUE, 0);

  return vbox1;
}

NavWinData *
create_dummy_iwd (void       *gdisp_ptr,
		  NavWinType  ptype)
{
  NavWinData *iwd;
  iwd = (NavWinData *) g_malloc (sizeof (NavWinData));
  iwd->ptype = ptype;
  iwd->info_win = NULL;
  iwd->showingPreview = TRUE;
  iwd->installedDirtyTimer = FALSE;
  iwd->preview = NULL;
  iwd->zoom_label = NULL;
  iwd->zoom_adjustment = NULL;
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
  iwd->nav_preview_width = 
    (nav_preview_size < 0 || nav_preview_size > 256)?NAV_PREVIEW_WIDTH:nav_preview_size;
  iwd->nav_preview_height = 
    (nav_preview_size < 0 || nav_preview_size > 256)?NAV_PREVIEW_HEIGHT:nav_preview_size;
  iwd->block_adj_sig = FALSE;
  iwd->frozen = FALSE;
  iwd->timer_id = 0;
  
  return(iwd);
}

/* Create a version of the nav window that follows the active
 * image.
 */

static InfoDialog *nav_window_auto = NULL;

static gchar *
nav_window_title(GDisplay   *gdisp)
{
  char       *title;
  gchar       *title_buf;

  title = g_basename (gimage_filename (gdisp->gimage));

  /*  create the info dialog  */
  title_buf = g_strdup_printf (_("Navigation: %s-%d.%d"), 
			       title,
			       pdb_image_to_id (gdisp->gimage),
			       gdisp->instance);

  return title_buf;
}

static void
nav_window_change_display (GimpContext *context, /* NOT USED */
			   GDisplay    *newdisp,
			   gpointer     data /* Not used */)
{
  GDisplay * gdisp = newdisp;
  GDisplay * old_gdisp;
  GimpImage * gimage;
  NavWinData *iwd;
  gchar       *title_buf;

  iwd = (NavWinData *)nav_window_auto->user_data;
  old_gdisp = (GDisplay *) iwd->gdisp_ptr;

  if (!nav_window_auto || gdisp == old_gdisp || !gdisp)
    {
      return;
    }

  gtk_widget_set_sensitive(nav_window_auto->vbox,TRUE);
  iwd->frozen = FALSE;

  title_buf = nav_window_title(gdisp);

  gtk_window_set_title (GTK_WINDOW (nav_window_auto->shell), title_buf);

  g_free (title_buf);

  gimage = gdisp->gimage;

  if (gimage && gimp_set_have (image_context, gimage))
    {
      iwd->gdisp_ptr = gdisp;

      /* Update preview to new display */
      nav_window_preview_resized (nav_window_auto);
      /* Tie into the dirty signal so we can update the preview */
      /* provided we haven't already */
      if(!gtk_object_get_data(GTK_OBJECT (gdisp->gimage),
			     "nav_handlers_installed"))
	{
	  gtk_signal_connect_after (GTK_OBJECT (gdisp->gimage), "dirty",
				    GTK_SIGNAL_FUNC(nav_image_need_update),iwd);
	  
	  /* Also clean signal so undos get caught as well..*/
	  gtk_signal_connect_after (GTK_OBJECT (gdisp->gimage), "clean",
				    GTK_SIGNAL_FUNC(nav_image_need_update),iwd);
	  gtk_object_set_data(GTK_OBJECT (gdisp->gimage),"nav_handlers_installed",iwd);
	}
    }
}

void
nav_window_follow_auto()
{
  GDisplay * gdisp;
  NavWinData * iwd;

  gdisp = gdisplay_active (); 
  
  if (!gdisp) 
    return;

  if(!nav_window_auto)
    {
      nav_window_auto = nav_window_create ((void *) gdisp);
      gtk_signal_connect (GTK_OBJECT (gimp_context_get_user ()), "display_changed",
			  GTK_SIGNAL_FUNC (nav_window_change_display), NULL);
    }

  nav_dialog_popup (nav_window_auto);

  iwd = (NavWinData *)nav_window_auto->user_data;
  gtk_widget_set_sensitive(nav_window_auto->vbox,TRUE);
  iwd->frozen = FALSE;
}


InfoDialog *
nav_window_create (void *gdisp_ptr)
{
  InfoDialog *info_win;
  GDisplay   *gdisp;
  NavWinData *iwd;
  GtkWidget  *container;
  char       *title_buf;
  int type;

  gdisp = (GDisplay *) gdisp_ptr;
  
  type = gimage_base_type (gdisp->gimage);

  title_buf = nav_window_title(gdisp);

  info_win = info_dialog_new (title_buf,
			      gimp_standard_help_func,
			      "dialogs/navigation_window.html");
  g_free (title_buf);

  dialog_register (info_win->shell);

  /*  create the action area  */
  gimp_dialog_create_action_area (GTK_DIALOG (info_win->shell),

				  _("Close"), nav_window_close_callback,
				  info_win, NULL, NULL, TRUE, FALSE,

				  NULL);

  gtk_signal_connect (GTK_OBJECT (info_win->shell), "destroy",
		      (GtkSignalFunc) nav_window_destroy_callback,
		      info_win);

  iwd = create_dummy_iwd (gdisp_ptr, NAV_WINDOW);
  info_win->user_data = iwd;
  iwd->info_win = info_win;

  /* Add preview */
  container = info_window_image_preview_new (info_win);
  gtk_table_attach_defaults (GTK_TABLE (info_win->info_table), container, 
 			     0, 2, 0, 1); 

  /* Tie into the dirty signal so we can update the preview */
  gtk_signal_connect_after (GTK_OBJECT (gdisp->gimage), "dirty",
			    GTK_SIGNAL_FUNC(nav_image_need_update),iwd);

  /* Also clean signal so undos get caught as well..*/
  gtk_signal_connect_after (GTK_OBJECT (gdisp->gimage), "clean",
			    GTK_SIGNAL_FUNC(nav_image_need_update),iwd);


  /* Mark the image as having the handlers installed...
   */
  gtk_object_set_data(GTK_OBJECT (gdisp->gimage),"nav_handlers_installed",iwd);
  
  return info_win;
}

void
nav_window_update_window_marker (InfoDialog *info_win)
{
  NavWinData *iwd;

  /* So this functions works both ways..
   * it will come in here with info_win == null
   * if the auto mode is on...
   */

  if(!info_win && nav_window_auto)
    {
      iwd = (NavWinData *)nav_window_auto->user_data;
      if(iwd->frozen)
	return;
      nav_window_update_window_marker(nav_window_auto);

      return;
    }

  if(!info_win)
    return;

  iwd = (NavWinData *)info_win->user_data;

  if(!iwd || 
     iwd->showingPreview == FALSE || 
     iwd->block_window_marker == TRUE)
    return;

  update_zoom_label (iwd);
  update_zoom_adjustment (iwd);

  /* Undraw old size */
  nav_window_draw_sqr (iwd,
		       FALSE,
		       iwd->dispx,
		       iwd->dispy,
		       iwd->dispwidth, iwd->dispheight);

  /* Update to new size */
  nav_window_disp_area (iwd,iwd->gdisp_ptr);

  /* and redraw */
  nav_window_draw_sqr (iwd,
		       FALSE,
		       iwd->dispx,
		       iwd->dispy,
		       iwd->dispwidth, iwd->dispheight);
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
      iwd->timer_id = 
	gtk_timeout_add (PREVIEW_UPDATE_TIMEOUT,
			 (GtkFunction) nav_preview_update_do_timer,
			 (gpointer) iwd); 
    }

  gdk_window_raise (GTK_WIDGET (idialog->shell)->window);
}

/* Chose the first image.. */
static void
gimlist_cb (gpointer im, 
	    gpointer data)
{
  GSList** l;

  l = (GSList**) data;
  *l = g_slist_prepend (*l, im);
}

static GDisplay *
nav_window_get_gdisp (void)
{
  GSList *list=NULL;
  GSList *listPtr=NULL;
  GimpImage *gimage  ;
  GDisplay * gdisp = NULL;

  gimage_foreach (gimlist_cb, &list);

  if(!list)
    return NULL;
  
  /* Find first valid image */
  for (listPtr = list; listPtr; listPtr = g_slist_next (listPtr))
    {
      gimage = GIMP_IMAGE (listPtr->data);
      gdisp = gdisplays_check_valid (NULL,gimage);
      if(gdisp)
	break;
    }
  
  g_slist_free(list);

  return (gdisp);
}


void
nav_window_free (GDisplay *del_gdisp,InfoDialog *info_win)
{
  NavWinData *iwd;

  /* So this functions works both ways..
   * it will come in here with info_win == null
   * if the auto mode is on...
   */

  if(!info_win)
    {
      if(nav_window_auto != NULL)
	{
	  GDisplay * gdisp;

	  iwd = (NavWinData *)nav_window_auto->user_data;

	  /* Only freeze if we are displaying the image we have deleted */
	  if((GDisplay *) iwd->gdisp_ptr != del_gdisp)
	    return;

	  if(iwd->timer_id)
	    gtk_timeout_remove(iwd->timer_id);

	  iwd->timer_id = 0;

	  gdisp = nav_window_get_gdisp();

	  if(gdisp)
	    nav_window_change_display(NULL,gdisp,NULL);
	  else
	    {
	      /* Clear window and freeze */
	      iwd->frozen = TRUE;
	      nav_window_update_preview_blank(iwd); 
	      gtk_window_set_title (GTK_WINDOW (nav_window_auto->shell), 
				    _("Navigation: No Image"));

	      gtk_widget_set_sensitive(nav_window_auto->vbox,FALSE);
	      iwd->gdisp_ptr = NULL;
	      gtk_widget_hide(GTK_WIDGET(nav_window_auto->shell));
	    }
	}
      return;
    }

  /* We are actually freeing a window here so remove any timers left. */
  iwd = (NavWinData *)info_win->user_data;
  if(iwd->timer_id)
    gtk_timeout_remove(iwd->timer_id);

  g_free (info_win->user_data);
  info_dialog_free (info_win);
}


/* nav popup ...
 * should share code with the nav_window dialog since they are 
 * essentially the same.
 */

void 
nav_popup_click_handler (GtkWidget      *widget, 
			 GdkEventButton *event, 
			 gpointer        data)
{
  GdkEventButton *bevent;
  GDisplay   *gdisp = data;
  NavWinData *iwp;  /* dummy shorter version for the popups */
  gint x, y;
  gint x_org, y_org;
  gint scr_w, scr_h;

  bevent = (GdkEventButton *)event;

  if(!gdisp->nav_popup)
    {
      /* popup a simplfied window with the nav box in it */
      GtkWidget *frame;
      GtkWidget *vbox;

      iwp = create_dummy_iwd(gdisp,NAV_POPUP); 
      gdisp->nav_popup = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_widget_set_events (gdisp->nav_popup, 
			     PREVIEW_MASK);

      gtk_window_set_policy (GTK_WINDOW (gdisp->nav_popup),
			     FALSE, FALSE, TRUE);
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (gdisp->nav_popup), frame);
      gtk_widget_show (frame);

      vbox = create_preview_containers(iwp,gdisp->nav_popup);
      gtk_widget_set_extension_events (iwp->preview, GDK_EXTENSION_EVENTS_ALL);

      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_object_set_data(GTK_OBJECT(gdisp->nav_popup),"navpop_prt",
			  (gpointer)iwp);
      nav_window_disp_area(iwp,iwp->gdisp_ptr);
    }
  else
    {
      gtk_widget_hide (gdisp->nav_popup);
      iwp = (NavWinData *)gtk_object_get_data(GTK_OBJECT(gdisp->nav_popup),"navpop_prt");
      nav_window_disp_area(iwp,iwp->gdisp_ptr); 
      nav_window_update_preview(iwp); 
    }

  /* decide where to put the popup */
  gdk_window_get_origin (widget->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  x = x_org + bevent->x - iwp->dispx - ((iwp->dispwidth - BORDER_PEN_WIDTH+1) * 0.5) - 2;
  y = y_org + bevent->y - iwp->dispy - ((iwp->dispheight - BORDER_PEN_WIDTH+1)* 0.5) - 2;

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
  x = (x + iwp->pwidth > scr_w) ? scr_w - iwp->pwidth - 2: x;
  y = (y + iwp->pheight > scr_h) ? scr_h - iwp->pheight - 2: y;

  gtk_widget_popup (gdisp->nav_popup, x, y);
  gdk_flush();

  /* fill in then set up handlers for mouse motion etc */
  iwp->motion_offsetx = (iwp->dispwidth - BORDER_PEN_WIDTH+1) * 0.5;
  iwp->motion_offsety = (iwp->dispheight - BORDER_PEN_WIDTH+1)* 0.5;
  if(GTK_WIDGET_VISIBLE(iwp->preview))
    nav_window_grab_pointer(iwp,iwp->preview);
}

void
nav_popup_free (GtkWidget *nav_popup)
{
  gtk_widget_destroy (nav_popup);
}

void 
nav_window_preview_resized (InfoDialog *idialog)
{
  NavWinData *iwd;

  if (!idialog)
    return;

  iwd = (NavWinData *)idialog->user_data;

  /* force regeneration of the widgets */
  /* bit of a fiddle... could cause if the image really is 1x1
   * but the preview would not really matter in that case.
   */
  iwd->imagewidth = 1;
  iwd->imageheight = 1;

  iwd->nav_preview_width = 
    (nav_preview_size < 0 || nav_preview_size > 256)?NAV_PREVIEW_WIDTH:nav_preview_size;
  iwd->nav_preview_height = 
    (nav_preview_size < 0 || nav_preview_size > 256)?NAV_PREVIEW_HEIGHT:nav_preview_size;

  nav_window_update_window_marker(idialog); 
}

void 
nav_window_popup_preview_resized (GtkWidget **widget)
{
  NavWinData *iwp;  /* dummy shorter version for the popups */

  iwp = (NavWinData *)gtk_object_get_data (GTK_OBJECT (*widget),"navpop_prt");

  g_free (iwp);

  gtk_widget_destroy (*widget);

  *widget = NULL;
}
