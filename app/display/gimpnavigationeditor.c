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

#include "appenv.h"
#include "actionarea.h"
#include "colormaps.h"
#include "info_dialog.h"
#include "info_window.h"
#include "gdisplay.h"
#include "gximage.h"
#include "interface.h"
#include "scroll.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpunit.h"


#define MAX_BUF 256

#define PREVIEW_MASK   GDK_EXPOSURE_MASK | \
                       GDK_MOTION_NOTIFY | \
                       GDK_POINTER_MOTION_MASK | \
                       GDK_BUTTON_PRESS_MASK | \
                       GDK_BUTTON_RELEASE_MASK | \
                       GDK_BUTTON_MOTION_MASK | \
                       GDK_KEY_PRESS_MASK | \
                       GDK_KEY_RELEASE_MASK
                                                          
/* Navigation preview sizes */
#define NAV_PREVIEW_WIDTH  112
#define NAV_PREVIEW_HEIGHT 112
#define BORDER_PEN_WIDTH   3

typedef struct _NavWinData NavWinData;
struct _NavWinData
{
  gboolean   showingPreview;
  GtkWidget *preview;
  void      *gdisp_ptr; /* I'a not happy 'bout this one */
  GtkWidget *previewBox;
  GtkWidget *previewAlign;
  gdouble    ratio;
  GdkGC     *gc;
  gint       dispx;        /* x pos of top left corner of display area */
  gint       dispy;        /* y pos of top left corner of display area */
  gint       dispwidth;    /* width of display area */
  gint       dispheight;   /* height left corner of display area */
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

static void
nav_window_close_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  InfoDialog *info_win;
  NavWinData *iwd;
  
  info_win = (InfoDialog *)client_data;
  iwd = (NavWinData *)info_win->user_data;

  /*   iwd->showingPreview = FALSE;  ALT. Needs to be sorted out */
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
      destroy_preview_widget(iwd);
      create_preview_widget(iwd);
      need_update = TRUE;
    }

  iwd->imagewidth = newwidth;
  iwd->imageheight = newheight;

  /* Normalise */
  iwd->dispwidth = MIN(iwd->dispwidth, iwd->pwidth-BORDER_PEN_WIDTH);
  iwd->dispheight = MIN(iwd->dispheight, iwd->pheight-BORDER_PEN_WIDTH);

  if(need_update == TRUE)
    {
      gtk_widget_hide(iwd->previewAlign);
      nav_window_update_preview(iwd);
      gtk_widget_show(iwd->preview);
      gtk_widget_draw(iwd->preview, NULL); 
      gtk_widget_show(iwd->previewAlign);
      nav_window_draw_sqr(iwd,FALSE,
			   iwd->dispx,iwd->dispy,
			   iwd->dispwidth,iwd->dispheight);
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
			  iwd->dispwidth, 
			  iwd->dispheight);
    }

/*   gdk_draw_rectangle (iwd->preview->window, iwd->gc, FALSE,  */
/* 		      10,20,  */
/* 		      40,50); */

  gdk_draw_rectangle (iwd->preview->window, iwd->gc, FALSE, 
		      x,y, 
		      w,
		      h);

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
  iwd->preview = image;
  gtk_widget_show (image);

  gtk_preview_set_dither (GTK_PREVIEW (image), GDK_RGB_DITHER_MAX);
  gtk_widget_set_events( GTK_WIDGET(image), PREVIEW_MASK );

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

  gtk_widget_grab_focus(image);

}

#if 0
static void
info_window_page_switch (GtkWidget *widget, 
			 GtkNotebookPage *page, 
			 gint page_num)
{
  InfoDialog *info_win;
  InfoWinData *iwd;
  
  info_win = (InfoDialog *)gtk_object_get_user_data(GTK_OBJECT (widget));
  iwd = (InfoWinData *)info_win->user_data;

  /* Only deal with the second page */
  if(page_num != 1)
    {
      iwd->showingPreview = FALSE;
      return;
    }

  iwd->showingPreview = TRUE;

}
#endif /* 0 */

static void
update_real_view(NavWinData *iwd,gint tx,gint ty)
{
  GDisplay *gdisp;
  gdouble ratio;
  gint xoffset;
  gint yoffset;

  gdisp = (GDisplay *) iwd->gdisp_ptr;
  ratio = ((gdouble)SCALEDEST(gdisp))/((gdouble)SCALESRC(gdisp));

  xoffset = tx - iwd->dispx;
  yoffset = ty - iwd->dispy;
  
  xoffset = (gint)(((gdouble)xoffset*ratio)/iwd->ratio);
  yoffset = (gint)(((gdouble)yoffset*ratio)/iwd->ratio);

  iwd->block_window_marker = TRUE;
  scroll_display(iwd->gdisp_ptr,xoffset,yoffset);
  iwd->block_window_marker = FALSE;
}

static void
nav_window_update_preview(NavWinData *iwd)
{
  GDisplay *gdisp;
  TempBuf * preview_buf;
  gchar *src, *buf;
  gint x,y,has_alpha;
  gint pwidth, pheight;
  GimpImage *gimage;
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
  for (y = 0; y <preview_buf->height ; y++)
    {
      if (preview_buf->bytes == (1+has_alpha))
	for (x = 0; x < preview_buf->width; x++)
	  {
	    buf[x*3+0] = src[x];
	    buf[x*3+1] = src[x];
	    buf[x*3+2] = src[x];
	  }
      else
	for (x = 0; x < preview_buf->width; x++)
	  {
	    gint stride = 3 + has_alpha;
	    buf[x*3+0] = src[x*stride+0];
	    buf[x*3+1] = src[x*stride+1];
	    buf[x*3+2] = src[x*stride+2];
	  }
      gtk_preview_draw_row (GTK_PREVIEW (iwd->preview),
			    (guchar *)buf, 0, y, preview_buf->width);
      src += preview_buf->width * preview_buf->bytes;
    }

  g_free (buf);
  temp_buf_free (preview_buf);

  nav_window_disp_area(iwd,gdisp);
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

#if 0
static gint
nav_window_preview_resized (GtkWidget      *widget,
			    GtkAllocation  *alloc,
			    gpointer       *data)
{
  NavWinData *iwd;

  iwd = (NavWinData *)data;

  if(!iwd || !iwd->preview)
    return FALSE;

  printf("Now at [x,y] = [%d,%d] [w,h] = [%d,%d]\n",
	 alloc->x,alloc->y,
	 alloc->width,alloc->height);

  if(iwd->nav_preview_width == alloc->width &&
     iwd->nav_preview_height == alloc->height)
    return FALSE;

  iwd->nav_preview_width = alloc->width;
  iwd->nav_preview_height = alloc->height;
  set_size_data(iwd);
  gtk_preview_size(GTK_PREVIEW(iwd->preview),alloc->width,alloc->height);
  nav_window_update_preview(iwd);
  nav_window_draw_sqr(iwd,FALSE,
		      iwd->dispx,iwd->dispy,
		      iwd->dispwidth,iwd->dispheight);

#if 0 
  destroy_preview_widget(iwd);
  create_preview_widget(iwd);
  nav_window_update_preview(iwd);


  nav_window_update_preview(iwd);
  gtk_widget_show(iwd->preview);
  gtk_widget_draw(iwd->preview, NULL); 
  gtk_widget_show(iwd->previewAlign);
  nav_window_draw_sqr(iwd,FALSE,
		      iwd->dispx,iwd->dispy,
		      iwd->dispwidth,iwd->dispheight);

#endif /* 0 */
  return FALSE;
}

#endif /* 0 */

static gint
nav_window_preview_events (GtkWidget *widget,
			   GdkEvent  *event,
			   gpointer  *data)
{
  NavWinData *iwd;
  GDisplay *gdisp;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint tx,ty;

  iwd = (NavWinData *)data;

  if(!iwd)
    return FALSE;

  gdisp = (GDisplay *) iwd->gdisp_ptr;

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_MAP:
      nav_window_update_preview(iwd); 
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
	  break;
	default:
	}
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;

      if(!iwd->sq_grabbed)
	break;

      tx = mevent->x;
      ty = mevent->y;

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

      tx = tx - iwd->motion_offsetx;
      ty = ty - iwd->motion_offsety;

     if(tx < 0 || (tx + iwd->dispwidth) >= iwd->pwidth)
	{
	  iwd->motion_offsetx = mevent->x - iwd->dispx;
	  tx = iwd->dispx;
	}

     if(ty < 0 || (ty + iwd->dispheight) >= iwd->pheight)
       {
	 iwd->motion_offsety = mevent->y - iwd->dispy;
	 ty = iwd->dispy;
       }
     
     /* Update the real display */
     update_real_view(iwd,tx,ty);
      
     nav_window_draw_sqr(iwd,
			 TRUE,
			 tx,ty,
			 iwd->dispwidth,iwd->dispheight);
     
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
    case GDK_KEY_PRESS:
      /* hack for the update preview... needs to be fixed */
      nav_window_update_preview(iwd);
      gtk_widget_draw(iwd->preview, NULL); 
      break;
    default:
      break;
    }

  return FALSE;
}


static GtkWidget *
info_window_image_preview_new(InfoDialog *info_win)
{
  GtkWidget *hbox1;
  GtkWidget *alignment;

  NavWinData *iwd;

  iwd = (NavWinData *)info_win->user_data;

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);

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

  return hbox1;
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
  g_free (title_buf);
/*   gtk_window_set_policy (GTK_WINDOW (info_win->shell), */
/* 			 FALSE,FALSE,FALSE); */
  
  iwd = (NavWinData *) g_malloc (sizeof (NavWinData));
  info_win->user_data = iwd;
  iwd->showingPreview = TRUE;
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

  /* Add preview */
  container = info_window_image_preview_new(info_win);
  gtk_table_attach_defaults (GTK_TABLE (info_win->info_table), container, 
 			     0, 2, 0, 1); 
  /* Create the action area  */
  action_items[0].user_data = info_win;
  build_action_area (GTK_DIALOG (info_win->shell), action_items, 1, 0);

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
}

void
nav_window_free (InfoDialog *info_win)
{
  g_free (info_win->user_data);
  info_dialog_free (info_win);
}
