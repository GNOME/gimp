/***************************************************************************/
/* GCK - The General Convenience Kit. Generally useful conveniece routines */
/* for GIMP plug-in writers and users of the GDK/GTK libraries.            */
/* Copyright (C) 1996 Tom Bech                                             */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.               */
/***************************************************************************/

/*********************************************************/
/* The GCK version of the very handy notebook component. */
/* The basic functionality is there, but there's still   */
/* a thing or two to be desired of the apperance. It     */
/* doesn't handle resizes particulary graciously yet.    */
/*********************************************************/

#include <stdlib.h>
#include <gdk/gdk.h>
#include <gck/gck.h>

#define GCK_NOTEBOOK_TAB_HEIGHT 24
#define GCK_NOTEBOOK_TAB_PADDING 12

extern gint _GckAutoShowFlag;

/***********************/
/* Draw frame and tabs */
/***********************/

void gck_notebook_draw_tabframe(GdkWindow *window,GtkStyle *style,GdkRectangle *area,GtkPositionType pos)
{
  gint foldy,foldx;

  foldy=3;
  foldx=3;

  if (pos==GTK_POS_TOP)
    {
      gdk_draw_line (window, style->light_gc[GTK_STATE_NORMAL],
		     area->x,area->y+area->height,
                     area->x,area->y+foldy);
      gdk_draw_line (window, style->bg_gc[GTK_STATE_NORMAL],
		     area->x+1,area->y+area->height,
                     area->x+1,area->y+foldy);

      gdk_draw_line (window, style->light_gc[GTK_STATE_NORMAL],
		     area->x,area->y+foldy,
                     area->x+foldx,area->y);
      gdk_draw_line (window, style->bg_gc[GTK_STATE_NORMAL],
		     area->x+1,area->y+foldy,
                     area->x+foldx+1,area->y+1);

      gdk_draw_line (window, style->light_gc[GTK_STATE_NORMAL],
		     area->x+foldx,area->y,
                     area->x+area->width-foldx,area->y);
      gdk_draw_line (window, style->bg_gc[GTK_STATE_NORMAL],
		     area->x+foldx,area->y+1,
                     area->x+area->width-foldx,area->y+1);

      gdk_draw_line (window, style->black_gc,
                     area->x+area->width-foldx,area->y,
                     area->x+area->width,area->y+foldy);
      gdk_draw_line (window, style->mid_gc[GTK_STATE_NORMAL],
                     area->x+area->width-foldx,area->y+1,
                     area->x+area->width-1,area->y+foldy);

      gdk_draw_line (window, style->black_gc,
                     area->x+area->width,area->y+foldy,
                     area->x+area->width,area->y+area->height);
      gdk_draw_line (window, style->mid_gc[GTK_STATE_NORMAL],
                     area->x+area->width-1,area->y+foldy,
                     area->x+area->width-1,area->y+area->height);
    }
  else
    {
      gdk_draw_line (window, style->light_gc[GTK_STATE_NORMAL],
                     area->x,area->y+area->height-foldy,
		     area->x,area->y);
      gdk_draw_line (window, style->bg_gc[GTK_STATE_NORMAL],
                     area->x+1,area->y+area->height-foldy,
		     area->x+1,area->y);

      gdk_draw_line (window, style->light_gc[GTK_STATE_NORMAL],
		     area->x,area->y+area->height-foldy,
                     area->x+foldx,area->y+area->height);
      gdk_draw_line (window, style->bg_gc[GTK_STATE_NORMAL],
		     area->x+1,area->y+area->height-foldy,
                     area->x+foldx+1,area->y+area->height-1);

      gdk_draw_line (window, style->black_gc,
		     area->x+foldx+1,area->y+area->height,
                     area->x+area->width-foldx,area->y+area->height);
      gdk_draw_line (window, style->mid_gc[GTK_STATE_NORMAL],
		     area->x+foldx+1,area->y+area->height-1,
                     area->x+area->width-foldx,area->y+area->height-1);

      gdk_draw_line (window, style->black_gc,
                     area->x+area->width-foldx,area->y+area->height,
                     area->x+area->width,area->y+area->height-foldy);
      gdk_draw_line (window, style->mid_gc[GTK_STATE_NORMAL],
                     area->x+area->width-foldx,area->y+area->height-1,
                     area->x+area->width-1,area->y+area->height-foldy);

      gdk_draw_line (window, style->black_gc,
                     area->x+area->width,area->y+area->height-foldy,
                     area->x+area->width,area->y);
      gdk_draw_line (window, style->mid_gc[GTK_STATE_NORMAL],
                     area->x+area->width-1,area->y+area->height-foldy,
                     area->x+area->width-1,area->y);
    }
}

void gck_notebook_draw_frames(GckNoteBook *notebook)
{
  GList *current_page;
  GckNoteBookPage *page,*activepage=NULL;
  GtkStyle *style=notebook->widget->style;
  GdkWindow *mainwindow,*window;
  GtkAllocation mainallocation;
  gint xpos,ypos,strwid,fontheight;
  gint win_x,win_y,win_width,win_height;

  mainwindow=notebook->widget->window;
  mainallocation=notebook->widget->allocation;

  if (notebook->tab_position==GTK_POS_TOP)
    {
      mainallocation.y-=1;
      mainallocation.height+=1;
    }
  else
    mainallocation.height+=2;

  gtk_draw_shadow(style, mainwindow, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
    mainallocation.x,mainallocation.y,mainallocation.width,mainallocation.height); 

  current_page=g_list_first(notebook->page_list);

  window=notebook->tab_box->window;

  win_x=notebook->tab_box->allocation.x;
  win_y=notebook->tab_box->allocation.y;
  win_width=notebook->tab_box->allocation.width;
  win_height=notebook->tab_box->allocation.height;

  /* Font height is the distance from the baseline to the top (ascent) */
  /* plus the distance from the baseline to the bottom (descent).      */
  /* ================================================================= */

  fontheight=style->font->ascent+style->font->descent;
  xpos=GCK_NOTEBOOK_TAB_PADDING;
  ypos=(win_height-fontheight)>>1;

  /* First, draw the tabs of the inactive pages */
  /* ========================================== */

  while (current_page!=NULL)
    {
      page=(GckNoteBookPage *)current_page->data;
      if (page->active==FALSE)
        {
          page->tab->area.x=xpos-GCK_NOTEBOOK_TAB_PADDING+2;
          strwid=gdk_string_width(style->font,page->tab->label);
          page->tab->area.width=strwid+(GCK_NOTEBOOK_TAB_PADDING<<1)-4;    

          if (notebook->tab_position==GTK_POS_TOP)
            {
              gdk_draw_string(window,style->font,style->fg_gc[GTK_STATE_NORMAL],
                              xpos,ypos+style->font->ascent+2,page->tab->label);
    
              page->tab->area.y=2;
              page->tab->area.height=win_height;
            }
          else
            {
              gdk_draw_string(window,style->font,style->fg_gc[GTK_STATE_NORMAL],
                              xpos,ypos+style->font->ascent-2,page->tab->label);

              page->tab->area.y=-2;
              page->tab->area.height=win_height-1;
            }

          gck_notebook_draw_tabframe(window,style,&page->tab->area,notebook->tab_position);
          xpos+=((GCK_NOTEBOOK_TAB_PADDING<<1)+strwid)-4+1;
        }
      else
        {
          activepage=page;
          page->tab->area.x=xpos-GCK_NOTEBOOK_TAB_PADDING;
          if (notebook->tab_position==GTK_POS_TOP)
            {
              page->tab->area.y=0;
              page->tab->area.height=win_height+2;
            }
          else
            {
              page->tab->area.y=-2;
              page->tab->area.height=win_height+1;
            }
          strwid=gdk_string_width(style->font,page->tab->label);    
          page->tab->area.width=strwid+(GCK_NOTEBOOK_TAB_PADDING<<1);
          xpos+=((GCK_NOTEBOOK_TAB_PADDING<<1)+strwid)-4+1;
        }

      current_page=current_page->next;
    }
  
  /* Draw the active page tab a little */
  /* bigger and with "raised" text.    */
  /* ================================= */
  
  gdk_draw_rectangle(window,style->bg_gc[GTK_STATE_NORMAL],TRUE,
                     activepage->tab->area.x,activepage->tab->area.y,
                     activepage->tab->area.width,activepage->tab->area.height);

  gdk_draw_string(window,style->font,style->fg_gc[GTK_STATE_NORMAL],
                  activepage->tab->area.x+GCK_NOTEBOOK_TAB_PADDING,
                  ypos+style->font->ascent,activepage->tab->label);
  
  gck_notebook_draw_tabframe(window,style,&activepage->tab->area,notebook->tab_position);
  
  if (notebook->tab_position==GTK_POS_TOP)
    {
      gdk_draw_line (window, style->light_gc[GTK_STATE_NORMAL],
                     0,win_height-1,activepage->tab->area.x,win_height-1);
      gdk_draw_line (window, style->light_gc[GTK_STATE_NORMAL],
                     activepage->tab->area.x+activepage->tab->area.width,win_height-1,
                     win_width,win_height-1);
    }
  else
    {
      if (activepage->tab->area.x!=0)
        {
          gdk_draw_line (window, style->mid_gc[GTK_STATE_NORMAL],0,0,activepage->tab->area.x,0);
          gdk_draw_line (window, style->black_gc,0,1,activepage->tab->area.x-1,1);
        }

      gdk_draw_line (window, style->mid_gc[GTK_STATE_NORMAL],
                     activepage->tab->area.x+activepage->tab->area.width,0,win_width-2,0);
      gdk_draw_line (window, style->black_gc,
                     activepage->tab->area.x+activepage->tab->area.width,1,win_width,1);
      gdk_draw_line (window, style->black_gc,win_width-1,1,win_width-1,0);
    }
}

/***********************************************************/
/* Check if we've hit any of the tabs; return TRUE and the */
/* page we hit if we did, FALSE otherwise.                 */
/***********************************************************/

gint gck_notebook_tab_hit(GckNoteBook *notebook,gint x,gint y,gint *tab_num)
{
  gint tabno=0;
  GList *current;
  GckNoteBookPage *page;

  /* First check if we've hit the current page tab */
  /* ============================================= */
  
  current=g_list_nth(notebook->page_list,notebook->current_page);

  page=(GckNoteBookPage *)current->data;
  if (x>page->tab->area.x && x<(page->tab->area.x+page->tab->area.width) &&
      y>page->tab->area.y && y<(page->tab->area.y+page->tab->area.height))
    {
      if (tab_num!=NULL) *tab_num=notebook->current_page;
      return(TRUE);
    }
  
  current=g_list_first(notebook->page_list);
  while (current!=NULL)
    {
      page=(GckNoteBookPage *)current->data;
      if (page->active==FALSE &&
          x>page->tab->area.x && x<(page->tab->area.x+page->tab->area.width) &&
          y>page->tab->area.y && y<(page->tab->area.y+page->tab->area.height))
        {
          if (tab_num!=NULL) *tab_num=tabno;
          return(TRUE);
        }
      current=current->next;
      tabno++;
    }

  return(FALSE);
}

/******************************/
/* The notebook event handler */
/******************************/

gint _gck_notebook_eventhandler(GtkWidget *widget,GdkEvent *event)
{
  GckNoteBook *notebook;
  gint returnval=FALSE,page;
  
  notebook=(GckNoteBook *)gtk_object_get_data(GTK_OBJECT(widget),"_GckNoteBook");

  switch (event->type)
    {
      case GDK_EXPOSE:
        gck_notebook_draw_frames(notebook);
        break;
        case GDK_BUTTON_PRESS:
        if (gck_notebook_tab_hit(notebook,event->button.x,event->button.y,&page)==TRUE)
          notebook->button_down=TRUE;
        break;
      case GDK_BUTTON_RELEASE:
        if (notebook->button_down==TRUE && gck_notebook_tab_hit(notebook,
            event->button.x,event->button.y,&page)==TRUE)
          gck_notebook_set_page(notebook,page);
        notebook->button_down=FALSE;
      default:
        break;
    }

  return(returnval);
}

gint _gck_notebook_frame_eventhandler(GtkWidget *widget,GdkEvent *event)
{
  GckNoteBook *notebook;
  gint returnval=FALSE;
  
  notebook=(GckNoteBook *)gtk_object_get_data(GTK_OBJECT(widget),"_GckNoteBook");
  gck_notebook_draw_frames(notebook);

  return(returnval);
}

/**************************************************/
/* Create and initialize a instance of a notebook */
/**************************************************/

GckNoteBook *gck_notebook_new(GtkWidget *container,
                              gint width,gint height,
                              GtkPositionType tab_position)
{
  GckNoteBook *notebook;

  g_function_enter("gck_notebook_new");
  g_assert(container!=NULL);

  notebook=(GckNoteBook *)malloc(sizeof(GckNoteBook));
  if (notebook==NULL)
    {
      g_function_leave("gck_notebook_new");
      return(NULL);
    }

  notebook->page_list=NULL;
  notebook->num_pages=0;
  notebook->tab_position=tab_position;
  notebook->button_down=FALSE;
  notebook->current_page=0;
  notebook->width=width;
  notebook->height=height;

  notebook->workbox=gck_vbox_new(container,FALSE,FALSE,FALSE,0,0,0);

  gtk_widget_set_events(notebook->workbox,GDK_EXPOSURE_MASK);

  gtk_signal_connect(GTK_OBJECT(notebook->workbox),"event",
    (GtkSignalFunc)_gck_notebook_frame_eventhandler,(gpointer)notebook->workbox);

  gtk_object_set_data(GTK_OBJECT(notebook->workbox),"_GckNoteBook",(gpointer)notebook);

  notebook->tab_box=gck_drawing_area_new(NULL,
    notebook->width,GCK_NOTEBOOK_TAB_HEIGHT,
    GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK,
    (GtkSignalFunc)_gck_notebook_eventhandler);

  gtk_object_set_data(GTK_OBJECT(notebook->tab_box),"_GckNoteBook",(gpointer)notebook);

  if (notebook->tab_position==GTK_POS_TOP)
    {
      gtk_container_add(GTK_CONTAINER(notebook->workbox),notebook->tab_box);
      notebook->widget=gck_vbox_new(notebook->workbox,FALSE,FALSE,FALSE,0,0,2);
    }
  else
    {
      notebook->widget=gck_vbox_new(notebook->workbox,FALSE,FALSE,FALSE,0,0,2);
      gtk_container_add(GTK_CONTAINER(notebook->workbox),notebook->tab_box);
    }

  gtk_widget_show(notebook->tab_box);

  g_function_leave("gck_notebook_new");
  return(notebook);
}

/*********************************************************/
/* Free memory associated with the GckNoteBook structure */
/*********************************************************/

void gck_notebook_destroy(GckNoteBook *notebook)
{
  GList *current;
  GckNoteBookPage *page;

  g_function_enter("gck_notebook_destroy");
  g_assert(notebook!=NULL);

  if (notebook->page_list!=NULL)
    {
      current=g_list_first(notebook->page_list);
      while (current!=NULL)
        {
          page=(GckNoteBookPage *)current->data;
          if (page->tab!=NULL)
            {
              if (page->tab->image!=NULL)
                gdk_image_destroy(page->tab->image);
              if (page->tab->pixmap!=NULL)
                gdk_pixmap_unref(page->tab->pixmap);
              free(page->tab);
            }
          free(page);
          current=current->next;
        }
      g_list_free(notebook->page_list);
    }

  gtk_widget_destroy(notebook->workbox);
  free(notebook);

  g_function_leave("gck_notebook_destroy");
}

/*****************************************/
/* Create and initialize a notebook page */
/*****************************************/

GckNoteBookPage *gck_notebook_page_new(gchar *label,GckNoteBook *notebook)
{
  GckNoteBookPage *page=NULL;
  GckNoteBookTab *tab=NULL;
  
  g_function_enter("gck_notebook_page_new");
  g_assert(notebook!=NULL);

  page=(GckNoteBookPage *)malloc(sizeof(GckNoteBookPage));

  if (page==NULL)
    return(NULL);

  tab=(GckNoteBookTab *)malloc(sizeof(GckNoteBookTab));

  if (tab==NULL)
    {
      free(page);
      page=NULL;
    }
  else
    {
      tab->label=label;
      tab->image=NULL;
      tab->pixmap=NULL;
  
      page->label=label;
      page->position=0;
      page->active=FALSE;
    
      page->tab=tab;
      page->notebook=notebook;
      page->widget=gck_vbox_new(notebook->widget,FALSE,FALSE,FALSE,0,0,0);
    }
  
  g_function_leave("gck_notebook_page_new");
  return(page);
}

/*********************************/
/* Insert a page in the notebook */
/*********************************/

void gck_notebook_insert_page(GckNoteBook *notebook,GckNoteBookPage *page,gint position)
{
  GList *node,*current;

  g_function_enter("gck_notebook_insert_page");
  g_assert(notebook!=NULL);
  g_assert(page!=NULL);

  if (position<=0)
    gck_notebook_prepend_page(notebook,page);    
  else if (position>=notebook->num_pages)
    gck_notebook_append_page(notebook,page);
  else
    {
      node=g_list_alloc();
      current=g_list_nth(notebook->page_list,position-1);
      node->next=current->next;
      node->prev=current;
      node->data=(gpointer)page;
      if (current->next!=NULL) current->next->prev=node;
      current->next=node;

      page->position=position;
      page->active=FALSE;
      gtk_widget_set_usize(page->widget,notebook->width,notebook->height);
      gtk_widget_set_usize(notebook->widget,notebook->width,notebook->height);

      while (current->next!=NULL)
        {
          page=((GckNoteBookPage *)current->data);
          page->position++;
          current=current->next;
        }
      
      notebook->num_pages++;
    }

  g_function_leave("gck_notebook_insert_page");
}

/*********************************/
/* Append a page to the notebook */
/*********************************/

void gck_notebook_append_page(GckNoteBook *notebook,GckNoteBookPage *page)
{
  g_function_enter("gck_notebook_append_page");
  g_assert(notebook!=NULL);
  g_assert(page!=NULL);

  notebook->page_list=g_list_append(notebook->page_list,(gpointer)page);

  if (notebook->num_pages>0)
    {
      page->active=FALSE;
      gtk_widget_unmap(page->widget);
      gtk_widget_hide(page->widget);
    }
  else
    page->active=TRUE;

  page->position=notebook->num_pages;
  gtk_widget_set_usize(page->widget,notebook->width,notebook->height);
  gtk_widget_set_usize(notebook->widget,notebook->width,notebook->height);
  notebook->num_pages++;

  g_function_leave("gck_notebook_append_page");
}

/**********************************/
/* Prepend a page to the notebook */
/**********************************/

void gck_notebook_prepend_page(GckNoteBook *notebook,GckNoteBookPage *page)
{
  GckNoteBookPage *current_page;
  GList *current;

  g_function_enter("gck_notebook_prepend_page");
  g_assert(notebook!=NULL);
  g_assert(page!=NULL);

  if (notebook->num_pages>0)
    {
      current_page=gck_notebook_get_page(notebook);
      current_page->active=FALSE;
      gtk_widget_unmap(current_page->widget);
    }

  notebook->page_list=g_list_prepend(notebook->page_list,(gpointer)page);
  page->active=TRUE;
  page->position=0;

  current=g_list_first(notebook->page_list);
  current=current->next;
  while (current!=NULL)
    {
      current_page=(GckNoteBookPage *)current->data;
      current_page->position++;
      current=current->next;
    }

  gtk_widget_set_usize(page->widget,notebook->width,notebook->height);
  gtk_widget_set_usize(notebook->widget,notebook->width,notebook->height);
  notebook->num_pages++;

  g_function_leave("gck_notebook_prepend_page");
}

/*************************************/
/* Tear out a page from the notebook */
/*************************************/

void gck_notebook_remove_page(GckNoteBook *notebook,gint page_num)
{
  GList *current,*temp;
  GckNoteBookPage *page,*pg;
  gint newactive=page_num;

  g_function_enter("gck_notebook_remove_page");
  g_assert(notebook!=NULL);

  if (notebook->num_pages>1)
    {
      if (page_num<0)
        page_num=0;
      else if (page_num>=notebook->num_pages)
        page_num=notebook->num_pages-1;

      current=g_list_nth(notebook->page_list,page_num);
      page=(GckNoteBookPage *)current->data;

      if (GTK_WIDGET_DRAWABLE(page->widget))
        gtk_widget_unmap(page->widget);

      temp=current;
      while (temp!=NULL) {
        pg=(GckNoteBookPage *)temp->data;
        pg->position--;
        temp=temp->next;
      }

      notebook->page_list=g_list_remove_link(notebook->page_list,current);
      notebook->num_pages--;
      
      if (page_num<=notebook->current_page)
        notebook->current_page--;

      if (page->active==TRUE)
        {
          if (newactive>=notebook->num_pages)
            newactive=notebook->num_pages-1; 

          gck_notebook_set_page (notebook,newactive);
        }
      else 
        gck_notebook_set_page (notebook,gck_notebook_get_page(notebook)->position);

/*      gtk_widget_destroy (page->widget);
      free(page); */
      
      g_list_free(current);
    }

  g_function_leave("gck_notebook_remove_page");
}

/*
void gck_notebook_remove_page(GckNoteBook *notebook,gint page_num)
{
  GList *current,*temp;
  GckNoteBookPage *page;
  gint newactive=page_num;

  g_function_enter("gck_notebook_remove_page");
  g_assert(notebook!=NULL);

  if (notebook->num_pages>1)
    {
      if (page_num<0)
        page_num=0;
      else if (page_num>notebook->num_pages)
        page_num=notebook->num_pages;

      current=g_list_nth(notebook->page_list,page_num);
      notebook->page_list=g_list_remove_link(notebook->page_list,current);
      page=(GckNoteBookPage *)current->data;
      gtk_widget_unmap(page->widget);

      if (page->active==TRUE)
        {
          if (current->next==NULL) newactive--;
          notebook->current_page=newactive;
          free(page);
          page=gck_notebook_get_page(notebook);
          page->active=TRUE;
          gtk_widget_map(page->widget);
        }
      temp=current;
      while (temp!=NULL)
        {
          page=(GckNoteBookPage *)temp->data;
          page->position--;
          temp=temp->next;
        }
      g_list_free(current);
    }

  g_function_leave("gck_notebook_remove_page");
}
*/

/*******************************/
/* Get the current active page */
/*******************************/

GckNoteBookPage *gck_notebook_get_page(GckNoteBook *notebook)
{
  GList *current=NULL;
  GckNoteBookPage *page=NULL;
  
  g_function_enter("gck_notebook_get_page");
  g_assert(notebook!=NULL);

  if (notebook->num_pages>0)
    {
      current=g_list_nth(g_list_first(notebook->page_list),notebook->current_page);
      page=(GckNoteBookPage *)current->data;
    }

  g_function_leave("gck_notebook_get_page");
  return(page);  
}

/*******************************/
/* Set the current active page */
/*******************************/

void gck_notebook_set_page(GckNoteBook *notebook,gint page_num)
{
  GckNoteBookPage *page;

  g_function_enter("gck_notebook_set_page");
  g_assert(notebook!=NULL);

  page=gck_notebook_get_page(notebook);
  page->active=FALSE;

  gtk_widget_unmap(page->widget);
  gtk_widget_hide(page->widget);

  /* Hmm.. some widgets (gtkframe for example) doesn't handle */
  /* unmap calls very graciously. Lets clean up to be safe..  */
  /* ======================================================== */

  gdk_window_clear(page->widget->window);

  notebook->current_page=page_num;
  page=gck_notebook_get_page(notebook);
  page->active=TRUE;

  gtk_widget_set_uposition(page->widget,notebook->widget->allocation.x+2,
                           notebook->widget->allocation.y+2);
  gtk_widget_set_usize(page->widget,notebook->width,notebook->height);
  gtk_widget_set_usize(notebook->widget,notebook->width,notebook->height);

  gtk_widget_show(page->widget);

  gdk_window_clear(notebook->tab_box->window);
  gck_notebook_draw_frames(notebook);

  g_function_leave("gck_notebook_set_page");
}

