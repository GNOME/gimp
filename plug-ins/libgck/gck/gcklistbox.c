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
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,   */
/* USA.                                                                    */
/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gck/gck.h>

extern gint _GckAutoShowFlag;

/*******************/
/* Disable signals */
/*******************/

void gck_listbox_disable_signals(GckListBox *listbox)
{
  g_function_enter("gck_listbox_disable_signals");
  g_assert(listbox!=NULL);

  listbox->disable_signals=TRUE;

  g_function_leave("gck_listbox_disable_signals");
}

/******************/
/* Enable signals */
/******************/

void gck_listbox_enable_signals(GckListBox *listbox)
{
  g_function_enter("gck_listbox_enable_signals");
  g_assert(listbox!=NULL);

  listbox->disable_signals=FALSE;

  g_function_leave("gck_listbox_enable_signals");
}

/*********************************************************************/
/* Get listbox field of the first element of the given list of items */
/*********************************************************************/

GckListBox *gck_listbox_get(GList *itemlist)
{
  g_function_enter("gck_listbox_get");
  g_assert(itemlist!=NULL);
  g_assert(itemlist->data!=NULL);
  g_function_leave("gck_listbox_get");

  return(((GckListBoxItem *)itemlist->data)->listbox);
}

/********************************/
/* Prepend one item to the list */
/********************************/

void gck_listbox_prepend_item(GckListBox *listbox,GckListBoxItem *item)
{
  g_function_enter("gck_listbox_prepend_item");
  g_assert(listbox!=NULL);
  g_assert(item!=NULL);

  gck_listbox_insert_item(listbox,item,0);

  g_function_leave("gck_listbox_prepend_item");
}

/***************************************/
/* Prepend a list of items to the list */
/***************************************/

void gck_listbox_prepend_items(GckListBox *listbox,GList *itemlist)
{
  GList *current;

  g_function_enter("gck_listbox_prepend_items");
  g_assert(listbox!=NULL);
  g_assert(itemlist!=NULL);

  current=g_list_last(itemlist);
  while (current!=NULL)
    {
      gck_listbox_insert_item(listbox,(GckListBoxItem *)current->data,0);
      current=current->prev;
    }

  g_function_leave("gck_listbox_prepend_items");
}

/*******************************/
/* Append one item to the list */
/*******************************/

void gck_listbox_append_item(GckListBox *listbox,GckListBoxItem *item)
{
  g_function_enter("gck_listbox_append_item");
  g_assert(listbox!=NULL);
  g_assert(item!=NULL);

  gck_listbox_insert_item(listbox,item,listbox->num_items);

  g_function_leave("gck_listbox_append_item");
}

/**************************************/
/* Append a list of items to the list */
/**************************************/

void gck_listbox_append_items(GckListBox *listbox,GList *itemlist)
{
  GList *current;
  
  g_function_enter("gck_listbox_append_items");
  g_assert(listbox!=NULL);
  g_assert(itemlist!=NULL);

  current=g_list_first(itemlist);
  while (current!=NULL)
    {
      gck_listbox_insert_item(listbox,(GckListBoxItem *)current->data,listbox->num_items);
      current=current->next;
    }

  g_function_leave("gck_listbox_append_items");
}

/*****************************************/
/* Insert one item at the given position */
/*****************************************/

void gck_listbox_insert_item(GckListBox *listbox,GckListBoxItem *item,gint position)
{
  GckListBoxItem *newlistboxitem;
  GtkWidget *newitem,*hbox,*align;
  GList *glist=NULL;

  g_function_enter("gck_listbox_insert_item");
  g_assert(listbox!=NULL);
  g_assert(item!=NULL);

  if (position>listbox->num_items)
    position=listbox->num_items;
  if (position<0)
    position=0;

  newlistboxitem=(GckListBoxItem *)malloc(sizeof(GckListBoxItem));
  if (newlistboxitem!=NULL)
    {
      *newlistboxitem=*item;
      newlistboxitem->listbox=listbox;
      listbox->itemlist=g_list_append(listbox->itemlist,newlistboxitem);

      if (newlistboxitem->widget != NULL)
        {
          newitem = gtk_list_item_new ();
          hbox = gck_hbox_new (newitem, FALSE,FALSE,FALSE, 0,0,0);
          gtk_box_pack_start(GTK_BOX (hbox), newlistboxitem->widget, FALSE,FALSE,0);
          align=gtk_alignment_new(0.0,0.5,0.0,0.0);
          gck_label_new (newlistboxitem->label, align);          
          gtk_widget_show(align);
          gtk_box_pack_start(GTK_BOX (hbox), align, FALSE,FALSE,0);
        }
      else
        newitem = gtk_list_item_new_with_label(newlistboxitem->label);

      glist=g_list_append(glist,newitem);
      gtk_object_set_data(GTK_OBJECT(newitem),"_GckListBoxItem",(gpointer)newlistboxitem);
      gtk_widget_show(newitem);
      gtk_list_insert_items(GTK_LIST(listbox->list),glist,position);

      listbox->num_items++;
    }

  g_function_leave("gck_listbox_insert_item");
}

/**************************************************/
/* Insert the list of items at the given position */
/**************************************************/

void gck_listbox_insert_items(GckListBox *listbox,GList *itemlist,gint position)
{
  GList *current;

  g_function_enter("gck_listbox_insert_items");
  g_assert(listbox!=NULL);
  g_assert(itemlist!=NULL);

  current=g_list_last(itemlist);
  while (current!=NULL)
    {
      gck_listbox_insert_item(listbox,(GckListBoxItem *)current->data,position);
      current=current->prev;
    }

  g_function_leave("gck_listbox_insert_items");
}

/*************************************************/
/* Return a list of the currently selected items */
/*************************************************/

GList *gck_listbox_get_current_selection(GckListBox *listbox)
{
  GList *list=NULL,*current;

  g_function_enter("gck_listbox_get_current_selection");
  g_assert(listbox!=NULL);

  current=g_list_first(listbox->current_selection);
  while (current!=NULL)
    {
      list=g_list_append(list,current->data);
      current=current->next;
    }

  g_function_leave("gck_listbox_get_current_selection");
  return(list);
}

/**************************************************************/
/* Rebuild position array and current selection list based on */
/* the current selection of the list in the listbox           */
/**************************************************************/

void gck_listbox_set_current_selection(GckListBox *listbox)
{
  GList *current,*selection;
  gint position=0;
  GtkWidget *widget;

  g_function_enter("gck_listbox_set_current_selection");
  g_assert(listbox!=NULL);
  
  if (listbox->current_selection!=NULL)
    g_list_free(listbox->current_selection);
  
  selection=g_list_first(GTK_LIST(listbox->list)->selection);
  
  listbox->current_selection=NULL;

  while (selection!=NULL)
    {
      widget=(GtkWidget *)selection->data;
      position=gtk_list_child_position(GTK_LIST(listbox->list),widget);
      
      current=g_list_nth(listbox->itemlist,position);
      listbox->current_selection=g_list_append(listbox->current_selection,current->data);

      selection=selection->next;
    }

  g_function_leave("gck_listbox_set_current_selection");
}

/**************************************/
/* Get the item at the given position */
/**************************************/

GList *gck_listbox_item_find_by_position(GckListBox *listbox,gint position)
{
  GList *item=NULL;

  g_function_enter("gck_listbox_item_find_by_position");

  if (position>=0 && position<=listbox->num_items)
    item=g_list_nth(listbox->itemlist,position);

  g_function_leave("gck_listbox_item_find_by_position");
  return(item);
}

/********************************************************/
/* Find and return the first item with a matching label */
/********************************************************/

GList *gck_listbox_item_find_by_label(GckListBox *listbox,char *label,gint *position)
{
  GList *current=NULL;
  gint pos=0;
  
  g_function_enter("gck_listbox_item_find_by_label");
  g_assert(listbox!=NULL);

  pos=0;
  current=g_list_first(listbox->itemlist);
  while (current!=NULL && strcmp(((GckListBoxItem *)current->data)->label,label)!=0)
    {
      current=current->next;
      pos++;
    }

  if (position!=NULL)
    *position=pos;

  g_function_leave("gck_listbox_item_find_by_label");
  return(current);
}

/*********************************************************/
/* Find and return the first item with a user_data field */
/*********************************************************/

GList *gck_listbox_item_find_by_user_data(GckListBox *listbox,gpointer user_data,gint *position)
{
  GList *current=NULL;
  gint pos=0;
  
  g_function_enter("gck_listbox_item_find_by_user_data");
  g_assert(listbox!=NULL);
  
  pos=0;
  current=g_list_first(listbox->itemlist);
  while (current!=NULL && ((GckListBoxItem *)current->data)->user_data!=user_data)
    {
      current=current->next;
      pos++;
    }

  if (position!=NULL)
    *position=pos;

  g_function_leave("gck_listbox_item_find_by_user_data");
  return(current);
}

/***************************/
/* Delete item by position */
/***************************/

void gck_listbox_delete_item_by_position(GckListBox *listbox,gint position)
{
  GList *current,*selection;

  g_function_enter("gck_listbox_delete_item_by_position");
  g_assert(listbox!=NULL);

  /* Traverse item list and check for first matching label */
  /* ===================================================== */

  current=gck_listbox_item_find_by_position(listbox,position);

  if (current!=NULL)
    {
      /* Clear all (other) selected items  and select the one we found */
      /* ============================================================= */

      gck_listbox_unselect_all(listbox);
      gtk_list_select_item(GTK_LIST(listbox->list),position);

      /* Now, get selection (duh!) and remove it from the GtkList */
      /* ======================================================== */

      selection=g_list_first(GTK_LIST(listbox->list)->selection);
      gtk_list_remove_items(GTK_LIST(listbox->list),selection);

      listbox->itemlist=g_list_remove_link(listbox->itemlist,current);
      listbox->num_items--;

      /* Update current selection item list */
      /* ================================== */

      gck_listbox_set_current_selection(listbox);
    }

  g_function_leave("gck_listbox_delete_item_by_position");
}

/************************/
/* Delete item by label */
/************************/

void gck_listbox_delete_item_by_label(GckListBox *listbox,char *label)
{
  GList *current,*selection;
  gint position;

  g_function_enter("gck_listbox_delete_item_by_label");
  g_assert(listbox!=NULL);

  /* Traverse item list and check for first matching label */
  /* ===================================================== */

  current=gck_listbox_item_find_by_label(listbox,label,&position);

  if (current!=NULL)
    {
      /* Clear all (other) selected items  and select the one we found */
      /* ============================================================= */

      gck_listbox_unselect_all(listbox);
      gtk_list_select_item(GTK_LIST(listbox->list),position);

      /* Now, get selection (duh!) and remove it from the GtkList */
      /* ======================================================== */

      selection=g_list_first(GTK_LIST(listbox->list)->selection);
      gtk_list_remove_items(GTK_LIST(listbox->list),selection);

      listbox->itemlist=g_list_remove_link(listbox->itemlist,current);
      listbox->num_items--;

      /* Update current selection item list */
      /* ================================== */

      gck_listbox_set_current_selection(listbox);
    }

  g_function_leave("gck_listbox_delete_item_by_label");
}

/****************************************************/
/* Delete a list of items according to their labels */
/****************************************************/

void gck_listbox_delete_items_by_label(GckListBox *listbox,GList *itemlist)
{
  GList *current;
  
  g_function_enter("gck_listbox_delete_items_by_label");
  g_assert(listbox!=NULL);

  current=g_list_first(itemlist);
  while (current!=NULL)
    {
      gck_listbox_delete_item_by_label(listbox,((GckListBoxItem *)current->data)->label);
      current=current->next;
    }

  g_function_leave("gck_listbox_delete_items_by_label");
}

/****************************************/
/* Delete a item by its user_data field */
/****************************************/

void gck_listbox_delete_item_by_user_data(GckListBox *listbox,gpointer user_data)
{
  GList *current,*selection;
  gint position;

  g_function_enter("gck_listbox_delete_item_by_user_data");
  g_assert(listbox!=NULL);

  /* Traverse item list and check for first matching user_data field */
  /* =============================================================== */

  current=gck_listbox_item_find_by_user_data(listbox,user_data,&position);

  if (current!=NULL)
    {
      /* Clear all (other) selected items  and select the one we found */
      /* ============================================================= */

      gck_listbox_unselect_all(listbox);
      gtk_list_select_item(GTK_LIST(listbox->list),position);

      /* Now, get selection (duh!) and remove it from the GtkList */
      /* ======================================================== */

      selection=g_list_first(GTK_LIST(listbox->list)->selection);
      gtk_list_remove_items(GTK_LIST(listbox->list),selection);

      listbox->itemlist=g_list_remove_link(listbox->itemlist,current);
      listbox->num_items--;

      /* Update current selection item list */
      /* ================================== */

      gck_listbox_set_current_selection(listbox);
    }

  g_function_leave("gck_listbox_delete_item_by_user_data");
}

/**************************************************************/
/* Remove a list of items according to their user_data fields */
/**************************************************************/

void gck_listbox_delete_items_by_user_data(GckListBox *listbox,GList *itemlist)
{
  GList *current;
  
  g_function_enter("gck_listbox_delete_items_by_user_data");
  g_assert(listbox!=NULL);
  g_assert(itemlist!=NULL);

  current=g_list_first(itemlist);
  while (current!=NULL)
    {
      gck_listbox_delete_item_by_user_data(listbox,((GckListBoxItem *)current->data)->user_data);
      current=current->next;
    }

  g_function_leave("gck_listbox_delete_items_by_user_data");
}

/********************************************************/
/* Clear all selected items between pos #start and #end */
/********************************************************/

void gck_listbox_clear_items(GckListBox *listbox,gint start,gint end)
{
  g_function_enter("gck_listbox_clear_items");
  g_assert(listbox!=NULL);

  if (start<0) start=0;
  if (end>listbox->num_items) end=listbox->num_items;

  gtk_list_clear_items(GTK_LIST(listbox->list),start,end);
  gck_listbox_set_current_selection(listbox);

  g_function_leave("gck_listbox_clear_items");
}

/*************************************/
/* Select item at the given position */
/*************************************/

GList *gck_listbox_select_item_by_position(GckListBox *listbox,gint position)
{
  GList *current;
  
  g_function_enter("gck_listbox_select_item_by_position");
  g_assert(listbox!=NULL);

  current=gck_listbox_item_find_by_position(listbox,position);
  if (current!=NULL)
    {
      gtk_list_select_item(GTK_LIST(listbox->list),position);

      /* Update current selection item list */
      /* ================================== */

      gck_listbox_set_current_selection(listbox);      
    }

  g_function_leave("gck_listbox_select_item_by_position");
  return(current);
}

/******************************************/
/* Select first item with the given label */
/******************************************/

GList *gck_listbox_select_item_by_label(GckListBox *listbox,char *label)
{ 
  GList *current;
  gint position;
  
  g_function_enter("gck_listbox_select_item_by_label");
  g_assert(listbox!=NULL);

  current=gck_listbox_item_find_by_label(listbox,label,&position);
  if (current!=NULL)
    {
      gtk_list_select_item(GTK_LIST(listbox->list),position);

      /* Update current selection item list */
      /* ================================== */

      gck_listbox_set_current_selection(listbox);      
    }

  g_function_leave("gck_listbox_select_item_by_label");
  return(current);
}

/****************************************************/
/* Select first item with the given user_data field */
/****************************************************/

GList *gck_listbox_select_item_by_user_data(GckListBox *listbox,gpointer user_data)
{  
  GList *current;
  gint position;
  
  g_function_enter("gck_listbox_select_item_by_user_data");
  g_assert(listbox!=NULL);

  current=gck_listbox_item_find_by_user_data(listbox,user_data,&position);
  if (current!=NULL)
    {
      gtk_list_select_item(GTK_LIST(listbox->list),position);

      /* Update current selection item list */
      /* ================================== */

      gck_listbox_set_current_selection(listbox);      
    }

  g_function_leave("gck_listbox_select_item_by_user_data");
  return(current);
}

/*****************************/
/* Unselect item by position */
/*****************************/

GList *gck_listbox_unselect_item_by_position(GckListBox *listbox,gint position)
{
  GList *current;
  
  g_function_enter("gck_listbox_unselect_item_by_position");
  g_assert(listbox!=NULL);

  current=gck_listbox_item_find_by_position(listbox,position);
  if (current!=NULL)
    {
      gtk_list_unselect_item(GTK_LIST(listbox->list),position);

      /* Update current selection item list */
      /* ================================== */

      gck_listbox_set_current_selection(listbox);
    }

  g_function_leave("gck_listbox_unselect_item_by_position");
  return(current);
}

/**************************/
/* Unselect item by label */
/**************************/

GList *gck_listbox_unselect_item_by_label(GckListBox *listbox,char *label)
{
  GList *current;
  gint position;
  
  g_function_enter("gck_listbox_unselect_item_by_label");
  g_assert(listbox!=NULL);

  current=gck_listbox_item_find_by_label(listbox,label,&position);
  if (current!=NULL)
    {
      gtk_list_unselect_item(GTK_LIST(listbox->list),position);

      /* Update current selection item list */
      /* ================================== */

      gck_listbox_set_current_selection(listbox);      
    }

  g_function_leave("gck_listbox_unselect_item_by_label");
  return(current);
}

/******************************/
/* Unselect item by user_data */
/******************************/

GList *gck_listbox_unselect_item_by_user_data(GckListBox *listbox,gpointer user_data)
{
  GList *current;
  gint position;
  
  g_function_enter("gck_listbox_unselect_item_by_user_data");
  g_assert(listbox!=NULL);

  current=gck_listbox_item_find_by_user_data(listbox,user_data,&position);
  if (current!=NULL)
    {
      gtk_list_unselect_item(GTK_LIST(listbox->list),position);

      /* Update current selection item list */
      /* ================================== */

      gck_listbox_set_current_selection(listbox);      
    }

  g_function_leave("gck_listbox_unselect_item_by_user_data");
  return(current);
}

/********************************/
/* Select all items in the list */
/********************************/

void gck_listbox_select_all(GckListBox *listbox)
{
  gint position;
  
  g_function_enter("gck_listbox_select_all");
  g_assert(listbox!=NULL);

  gck_listbox_unselect_all(listbox);
  
  for (position=0;position<listbox->num_items;position++)
    gtk_list_select_item(GTK_LIST(listbox->list),position);

  /* Update current selection item list */
  /* ================================== */

  gck_listbox_set_current_selection(listbox);
  g_function_leave("gck_listbox_select_all");
}

/**********************************/
/* Unselect all items in the list */
/**********************************/

void gck_listbox_unselect_all(GckListBox *listbox)
{
  GList *selection;

  g_function_enter("gck_listbox_unselect_all");
  g_assert(listbox!=NULL);

  selection=g_list_first(GTK_LIST(listbox->list)->selection);

  while (selection!=NULL)
    {
      gtk_list_unselect_child(GTK_LIST(listbox->list),(GtkWidget *)selection->data);
      selection=g_list_first(GTK_LIST(listbox->list)->selection);
    }
  gck_listbox_set_current_selection(listbox);

  g_function_leave("gck_listbox_unselect_all");
}

/**************************************************************/
/* Event handler. Filters out uninteresting events and stores */
/* the even, so we can use it when the list signal comes      */
/**************************************************************/

void _gck_listbox_signalhandler(GtkWidget *widget,GtkListItem *item,gpointer user_data)
{
  GckListBox *listbox;

  listbox=(GckListBox *)gtk_object_get_data(GTK_OBJECT(widget),"_GckListBox");

  if (listbox->disable_signals==TRUE)
    return;

  /* Update selected items */
  /* ===================== */

  gck_listbox_set_current_selection(listbox);

  /* If a user event_handler is defined, call it */
  /* =========================================== */

  if (listbox->event_handler!=NULL)
    (*listbox->event_handler)(widget,&listbox->last_event,user_data);
}

gint _gck_listbox_eventhandler(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
  GckListBox *listbox;
  GtkWidget *event_widget;

  /* First, check if the issuing widget is a list_item, if not we'll */
  /* get the last selection and not the new.                         */
  /* =============================================================== */

  event_widget=gtk_get_event_widget(event);

  if (GTK_IS_LIST_ITEM(event_widget))
    {
      if (event->type==GDK_BUTTON_PRESS || event->type==GDK_2BUTTON_PRESS)
        {     
          /* Ok, get listbox */ 
          /* =============== */

          listbox=(GckListBox *)gtk_object_get_data(GTK_OBJECT(widget),"_GckListBox");    
          listbox->last_event=*event;
          
          if (listbox->disable_signals==TRUE)
            return(FALSE);
          
          if (event->type==GDK_2BUTTON_PRESS)
            {
              if (listbox->event_handler!=NULL)
                (*listbox->event_handler)(widget,&listbox->last_event,user_data);
            }
        }
    }

  return(FALSE);
}

/*******************/
/* Create list box */
/*******************/

GckListBox *gck_listbox_new(GtkWidget *container,gint expand,gint fill,gint padding,
                            gint width,gint height,GtkSelectionMode selection_mode,
                            GckListBoxItem *list_items,
                            GckEventFunction event_handler)
{
  GckListBox *listbox;
  gint i=0;

  g_function_enter("gck_listbox_new");
  g_assert(container!=NULL);

  listbox=(GckListBox *)malloc(sizeof(GckListBox));
  if (listbox==NULL)
    { 
      g_function_leave("gck_listbox_new");
      return(NULL);
    }

  listbox->itemlist=NULL;
  listbox->current_selection=NULL;
  listbox->selected_items=NULL;
  listbox->num_selected_items=0;
  listbox->num_items=0;
  listbox->width=width;
  listbox->height=height;
  listbox->event_handler=event_handler;
  listbox->disable_signals=FALSE;

  listbox->widget=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(listbox->widget),
    GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_widget_set_usize(listbox->widget,width,height);

  if (GTK_WIDGET_TYPE(container) == gtk_vbox_get_type() ||
      GTK_WIDGET_TYPE(container) == gtk_hbox_get_type())
    gtk_box_pack_start(GTK_BOX(container), listbox->widget, expand, fill, padding);
  else
    gtk_container_add(GTK_CONTAINER(container),listbox->widget);

  listbox->list=gtk_list_new();
  gtk_list_set_selection_mode(GTK_LIST(listbox->list),selection_mode);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(listbox->widget),listbox->list);
  gtk_object_set_data(GTK_OBJECT(listbox->list),"_GckListBox",(gpointer)listbox);
  gtk_widget_show(listbox->list);

  if (list_items!=NULL)
    {
      while (list_items[i].label!=NULL)
        gck_listbox_append_item(listbox,&list_items[i++]);
      gck_listbox_unselect_all(listbox);
    }

  if (_GckAutoShowFlag==TRUE)
    gtk_widget_show(listbox->widget);

  /* Make sure we're done with stuff, before we attach any handlers */
  /* ============================================================== */

  gtk_signal_connect(GTK_OBJECT(listbox->list),"button_press_event",
    (GtkSignalFunc)_gck_listbox_eventhandler,(gpointer)listbox->list);

  gtk_signal_connect(GTK_OBJECT(listbox->list),"selection_changed",
    (GtkSignalFunc)_gck_listbox_signalhandler,(gpointer)listbox->list);

  g_function_leave("gck_listbox_new");
  return(listbox);
}

/************************************************************/
/* Free the memory associated with the GckListBox structure */
/************************************************************/

void  gck_listbox_destroy(GckListBox *listbox)
{
  g_function_enter("gck_listbox_destroy");
  g_assert(listbox!=NULL);

  if (listbox->itemlist!=NULL)
    g_list_free(listbox->itemlist);

  if (listbox->current_selection!=NULL)
    g_list_free(listbox->current_selection);

  if (listbox->selected_items!=NULL)
    free(listbox->selected_items);

  g_function_leave("gck_listbox_destroy");
}

