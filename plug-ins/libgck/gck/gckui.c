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

/***********************************************************/
/* This file contains some convenience routines for making */
/* plug-in UIs. There's functions for creating dialogs,    */
/* option menus, entry fields, scales and checkbuttons.    */
/***********************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gck/gckui.h>

gint _GckAutoShowFlag = TRUE;

/*************************/
/* Set cursor for window */
/*************************/

void gck_cursor_set(GdkWindow * window, GdkCursorType cursortype)
{
  GdkCursor *newcursor;

  g_function_enter("gck_cursor_set");
  g_assert(window!=NULL);

  newcursor = gdk_cursor_new(cursortype);
  gdk_window_set_cursor(window, newcursor);
  gdk_cursor_destroy(newcursor);
  gdk_flush();

  g_function_leave("gck_cursor_set");
}

/********************************************************/
/* Toggle auto show flag, it is set to TRUE as default. */
/* Sometimes, however, we want to do things to a widget */
/* before we show it.                                   */
/********************************************************/

void gck_auto_show(gint flag)
{
  g_function_enter("gck_auto_show");

  if (flag == TRUE || flag == FALSE)
    _GckAutoShowFlag = flag;

  g_function_leave("gck_auto_show");
}

/*********************************************************/
/* Create application window, style, visual and colormap */
/*********************************************************/

GckApplicationWindow *gck_application_window_new(char *name)
{
  GckApplicationWindow *appwin;

  g_function_enter("gck_application_window_new");

  /* Create application window */
  /* ========================= */

  appwin = (GckApplicationWindow *) malloc(sizeof(GckApplicationWindow));
  if (appwin == NULL)
    {
      g_function_leave("gck_application_window_new");
      return (NULL);
    }

  /* Set up visual and colors */
  /* ======================== */

  if ((appwin->visinfo = gck_visualinfo_new()) == NULL)
    {
      free(appwin);
      g_function_leave("gck_application_window_new");
      return (NULL);
    }

  /* Set style */
  /* ========= */

  appwin->style = gtk_style_new();
  if (appwin->style == NULL)
    {
      gck_visualinfo_destroy(appwin->visinfo);
      free(appwin);
      g_function_leave("gck_application_window_new");
      return (NULL);
    }

  appwin->style->fg[GTK_STATE_NORMAL] = *gck_rgb_to_gdkcolor(appwin->visinfo, 0, 0, 0);
  appwin->style->fg[GTK_STATE_ACTIVE] = *gck_rgb_to_gdkcolor(appwin->visinfo, 0, 0, 0);
  appwin->style->fg[GTK_STATE_PRELIGHT] = *gck_rgb_to_gdkcolor(appwin->visinfo, 0, 0, 0);
  appwin->style->fg[GTK_STATE_SELECTED] = *gck_rgb_to_gdkcolor(appwin->visinfo, 255, 255, 255);

  appwin->style->bg[GTK_STATE_NORMAL] = *gck_rgb_to_gdkcolor(appwin->visinfo, 215, 215, 215);
  appwin->style->bg[GTK_STATE_SELECTED] = *gck_rgb_to_gdkcolor(appwin->visinfo, 0, 0, 156);
  appwin->style->bg[GTK_STATE_INSENSITIVE] = *gck_rgb_to_gdkcolor(appwin->visinfo, 215, 215, 215);

  appwin->style->klass->xthickness = 2;
  appwin->style->klass->ythickness = 2;

  gtk_widget_push_visual(appwin->visinfo->visual);
  gtk_widget_push_colormap(appwin->visinfo->colormap);
  gtk_widget_push_style(appwin->style);

  /* Create window widget */
  /* ==================== */

  appwin->widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(appwin->widget),name);

  /* Create application accelerator table */
  /* ==================================== */
  
  appwin->accelerator_table = gtk_accelerator_table_new();
  gtk_window_add_accelerator_table(GTK_WINDOW(appwin->widget),appwin->accelerator_table);

  g_function_leave("gck_application_window_new");
  return (appwin);
}

/******************************************************************/
/* Free memory associated with the GckApplicationWindow structure */
/******************************************************************/

void gck_application_window_destroy(GckApplicationWindow *appwin)
{
  g_function_enter("gck_application_window_destroy");
  g_assert(appwin!=NULL);

  gtk_widget_pop_style();
  gtk_widget_pop_colormap();
  gtk_widget_pop_visual();

  gck_visualinfo_destroy(appwin->visinfo);
  gtk_widget_destroy(appwin->widget);
  free(appwin);

  g_function_leave("gck_application_window_destroy");
}

/**************************/
/* Create dialog template */
/**************************/

GckDialogWindow *gck_dialog_window_new(char *name,GckPosition ActionPos,
                                       GtkSignalFunc ok_pressed_func,
                                       GtkSignalFunc cancel_pressed_func,
                                       GtkSignalFunc help_pressed_func)
{
  GckDialogWindow *dialog;
  GtkWidget *mainbox, *frame;

  g_function_enter("gck_dialog_window_new");

  /* Create dialog window */
  /* ==================== */

  dialog = (GckDialogWindow *) malloc(sizeof(GckDialogWindow));
  if (dialog == NULL)
    {
      g_function_leave("gck_dialog_window_new");
      return (NULL);
    }

  dialog->widget = gtk_window_new(GTK_WINDOW_DIALOG);
  gtk_window_set_title(GTK_WINDOW(dialog->widget),name);
  gtk_window_set_policy(GTK_WINDOW(dialog->widget),FALSE,TRUE,TRUE);
  gtk_window_position(GTK_WINDOW(dialog->widget),GTK_WIN_POS_MOUSE);

  dialog->okbutton=NULL;
  dialog->cancelbutton=NULL;
  dialog->helpbutton=NULL;
  
  if (ActionPos==GCK_TOP || ActionPos==GCK_BOTTOM)
    mainbox= gck_vbox_new(dialog->widget,FALSE,FALSE,FALSE,0,0,2);
  else
    mainbox= gck_hbox_new(dialog->widget,FALSE,FALSE,FALSE,0,0,2);

  /* Create WorkArea (in a frame) and ActionArea */
  /* =========================================== */

  if (ActionPos==GCK_TOP || ActionPos==GCK_BOTTOM)
    {
      if (ActionPos==GCK_TOP)
        {
          dialog->actionbox=gck_hbox_new(mainbox,TRUE,TRUE,TRUE,5,0,5);
          frame = gck_frame_new(NULL,mainbox,GTK_SHADOW_ETCHED_IN,TRUE,TRUE,0,5);
          dialog->workbox=gck_vbox_new(frame,FALSE,TRUE,TRUE,5,0,5);
        }
      else
        {
          frame = gck_frame_new(NULL,mainbox,GTK_SHADOW_ETCHED_IN,TRUE,TRUE,0,5);
          dialog->workbox=gck_vbox_new(frame,FALSE,TRUE,TRUE,5,0,5);
          dialog->actionbox=gck_hbox_new(mainbox,TRUE,TRUE,TRUE,5,0,5);
        }
    }
  else
    {
      if (ActionPos==GCK_LEFT)
        {
          dialog->actionbox=gck_vbox_new(mainbox,FALSE,FALSE,FALSE,5,0,5);
          frame = gck_frame_new(NULL,mainbox,GTK_SHADOW_ETCHED_IN,TRUE,TRUE,0,5);
          dialog->workbox=gck_vbox_new(frame,FALSE,TRUE,TRUE,5,0,5);
        }
      else
        {
          frame = gck_frame_new(NULL,mainbox,GTK_SHADOW_ETCHED_IN,TRUE,TRUE,0,5);
          dialog->workbox=gck_vbox_new(frame,FALSE,TRUE,TRUE,5,0,5);
          dialog->actionbox=gck_vbox_new(mainbox,FALSE,FALSE,FALSE,5,0,5);
        }
    }

  if (ok_pressed_func!=NULL)
    {
      dialog->okbutton = gck_pushbutton_new("Ok",dialog->actionbox,FALSE,TRUE,0,
                                            ok_pressed_func);
      gtk_object_set_data(GTK_OBJECT(dialog->okbutton),"GckDialogWindow",(gpointer)dialog);
    }

  if (cancel_pressed_func!=NULL)
    {
      dialog->cancelbutton = gck_pushbutton_new("Cancel",dialog->actionbox,FALSE,TRUE,0,
                                                cancel_pressed_func);
      gtk_object_set_data(GTK_OBJECT(dialog->cancelbutton),"GckDialogWindow",(gpointer)dialog);
    }

  if (help_pressed_func!=NULL)
    {
      dialog->helpbutton = gck_pushbutton_new("Help",dialog->actionbox,FALSE,TRUE,0,
                                              help_pressed_func);
      gtk_object_set_data(GTK_OBJECT(dialog->helpbutton),"GckDialogWindow",(gpointer)dialog);
    }

  g_function_leave("gck_dialog_window_new");
  return (dialog);
}

/*************************************************************/
/* Free memory associated with the GckDialogWindow structure */
/*************************************************************/

void gck_dialog_window_destroy(GckDialogWindow * dialog)
{
  g_function_enter("gck_dialog_window_destroy");
  g_assert(dialog!=NULL);

  gtk_widget_destroy(dialog->widget);
  free(dialog);

  g_function_leave("gck_dialog_window_destroy");
}

/*****************************/
/* Create vertical separator */
/*****************************/

GtkWidget *gck_vseparator_new(GtkWidget *container)
{
  GtkWidget *separator;
  
  g_function_enter("gck_vseparator_new");

  separator=gtk_vseparator_new();
  if (container!=NULL)
    gtk_container_add(GTK_CONTAINER(container),separator);
  
  if (_GckAutoShowFlag==TRUE)
    gtk_widget_show(separator);

  g_function_leave("gck_vseparator_new");
  return(separator);
}

/*******************************/
/* Create horizontal separator */
/*******************************/

GtkWidget *gck_hseparator_new(GtkWidget *container)
{
  GtkWidget *separator;
  
  g_function_enter("gck_hseparator_new");

  separator=gtk_hseparator_new();
  if (container!=NULL)
    gtk_container_add(GTK_CONTAINER(container),separator);
  
  if (_GckAutoShowFlag==TRUE) gtk_widget_show(separator);

  g_function_leave("gck_hseparator_new");
  return(separator);
}

/*************************/
/* Create frame template */
/*************************/

GtkWidget *gck_frame_new(char *name, GtkWidget * container, GtkShadowType shadowtype,
                         gint expand, gint fill, gint padding, gint borderwidth)
{
  GtkWidget *frame;
  gint container_type;

  g_function_enter("gck_frame_new");

  frame = gtk_frame_new(name);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), shadowtype);
  gtk_container_border_width(GTK_CONTAINER(frame), borderwidth);

  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), frame, expand, fill, padding);
      else
        gtk_container_add(GTK_CONTAINER(container), frame);
    }

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(frame);

  g_function_leave("gck_frame_new");
  return (frame);
}

/*************************/
/* Create label template */
/*************************/

GtkWidget *gck_label_new(char *name, GtkWidget *container)
{
  GtkWidget *label;
  gint container_type;

  g_function_enter("gck_label_new");

  if (name == NULL)
    label = gtk_label_new(" ");
  else
    label = gtk_label_new(name);

  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), label, FALSE,FALSE, 0);
      else
        gtk_container_add(GTK_CONTAINER(container), label);
    }

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(label);

  g_function_leave("gck_label_new");
  return (label);
}

GtkWidget *gck_label_aligned_new(char *name, GtkWidget * container, 
                                 gdouble xalign, gdouble yalign)
{
  GtkWidget *label;
  gint container_type;

  g_function_enter("gck_label_aligned_new");

  if (name == NULL)
    label = gtk_label_new(" ");
  else
    label = gtk_label_new(name);

  gtk_misc_set_alignment(GTK_MISC(label),xalign,yalign);

  if (container!=NULL)
   {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), label, FALSE,FALSE, 0);
      else
        gtk_container_add(GTK_CONTAINER(container), label);
   }

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(label);

  g_function_leave("gck_label_aligned_new");
  return (label);
}

GtkWidget *gck_drawing_area_new(GtkWidget *container,gint width,gint height,
                                gint event_mask,GtkSignalFunc event_handler)
{
  GtkWidget *drawingarea;
  gint container_type;
  
  g_function_enter("gck_drawing_area_new");

  drawingarea = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawingarea),width,height);

  gtk_widget_set_events(drawingarea,event_mask);

  gtk_signal_connect(GTK_OBJECT(drawingarea),"event",
    (GtkSignalFunc)event_handler,(gpointer)drawingarea);

  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), drawingarea, FALSE, FALSE, 0);
      else
        gtk_container_add(GTK_CONTAINER(container), drawingarea);
    }
  
  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(drawingarea);

  g_function_leave("gck_drawing_area_new");
  return(drawingarea);
}


GtkWidget *gck_pixmap_new (GdkPixmap *pixm,
                           GdkBitmap *mask,
                           GtkWidget *container)
{
  GtkWidget *pixmap,*alignment;

  g_function_enter("gck_pixmap_new");
  g_assert(pixm != NULL);

  pixmap = gtk_pixmap_new(pixm,mask);
/*  gtk_widget_set_events(pixmap,GDK_EXPOSURE_MASK); */

  alignment=gtk_alignment_new(0.5,0.5,0.0,0.0);
  gtk_container_add(GTK_CONTAINER(container),alignment);
  gtk_container_border_width(GTK_CONTAINER(alignment),2);

  gtk_container_add(GTK_CONTAINER(alignment),pixmap);

  gtk_widget_show(pixmap);

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(alignment);

  g_function_leave("gck_pixmap_new");
  return(pixmap);
}

/************************************/
/* Create horizontal scale template */
/************************************/

GtkWidget *gck_hscale_new(char *name, GtkWidget *container,
	                  GckScaleValues *svals,
                          GtkSignalFunc value_changed_func)
{
  GtkWidget *label, *scale;
  GtkObject *adjustment;
  gint container_type;

  g_function_enter("gck_hscale_new");
  g_assert(svals!=NULL);

  if (name != NULL)
    {
      label = gtk_label_new(name);
      gtk_container_add(GTK_CONTAINER(container), label);
      gtk_widget_show(label);
    }

  adjustment = gtk_adjustment_new(svals->value, svals->lower,
	                          svals->upper, svals->step_inc,
                                  svals->page_inc, svals->page_size);

  scale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
  gtk_widget_set_usize(scale, svals->size, 0);
  gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);

  if (value_changed_func!=NULL)
    gtk_signal_connect_object(GTK_OBJECT(adjustment),"value_changed",
      value_changed_func,(gpointer)scale);

  gtk_range_set_update_policy(GTK_RANGE(scale), svals->update_type);
  gtk_scale_set_draw_value(GTK_SCALE(scale), svals->draw_value_flag);

  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), scale, FALSE,FALSE, 0);
      else
        gtk_container_add(GTK_CONTAINER(container), scale);
    }

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(scale);

  g_function_leave("gck_hscale_new");
  return (scale);
}

/**********************************/
/* Create vertical scale template */
/**********************************/

GtkWidget *gck_vscale_new(char *name, GtkWidget * container,
	                  GckScaleValues *svals,
                          GtkSignalFunc value_changed_func)
{
  GtkWidget *label, *scale;
  GtkObject *adjustment;
  gint container_type;

  g_function_enter("gck_vscale_new");
  g_assert(svals!=NULL);

  if (name != NULL)
    {
      label = gtk_label_new(name);
      gtk_container_add(GTK_CONTAINER(container), label);
      gtk_widget_show(label);
    }

  adjustment = gtk_adjustment_new(svals->value, svals->lower,
			          svals->upper, svals->step_inc,
                                  svals->page_inc, svals->page_size);

  scale = gtk_vscale_new(GTK_ADJUSTMENT(adjustment));
  gtk_widget_set_usize(scale, 0, svals->size);
  gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);

  if (value_changed_func!=NULL)
    gtk_signal_connect_object(GTK_OBJECT(adjustment),"value_changed",
      value_changed_func,(gpointer)scale);

  gtk_range_set_update_policy(GTK_RANGE(scale), svals->update_type);
  gtk_scale_set_draw_value(GTK_SCALE(scale), svals->draw_value_flag);

  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), scale, FALSE,FALSE, 0);
      else
        gtk_container_add(GTK_CONTAINER(container), scale);
    }

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(scale);

  g_function_leave("gck_vscale_new");
  return (scale);
}

/**************************************/
/* Create (text) entry field template */
/**************************************/

GtkWidget *gck_entryfield_text_new (char *name, GtkWidget *container,
                                    char *initial_text,
                                    GtkSignalFunc text_changed_func)
{
  GtkWidget *entry, *label=NULL, *hbox=NULL;
  gint container_type;

  g_function_enter("gck_entryfield_text_new");

  if (name!=NULL)
    {
      hbox = gtk_hbox_new(FALSE, 0);

      if (container!=NULL)
        { 
          container_type=GTK_WIDGET_TYPE(container);
          if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
            gtk_box_pack_start(GTK_BOX(container), hbox, FALSE, FALSE, 0);
          else
            gtk_container_add(GTK_CONTAINER(container), hbox);
        }
  
      gtk_container_border_width(GTK_CONTAINER(hbox), 2);
      gtk_widget_show(hbox);

      label = gtk_label_new(name);
      gtk_container_add(GTK_CONTAINER(hbox), label);
      gtk_widget_show(label);
   }

  entry = gtk_entry_new();

  if (initial_text!=NULL)
    gtk_entry_set_text(GTK_ENTRY(entry), initial_text);

  if (hbox!=NULL)
    gtk_container_add(GTK_CONTAINER(hbox), entry);
  else if (container!=NULL)
    gtk_container_add(GTK_CONTAINER(container), entry);

  if (text_changed_func!=NULL)
    gtk_signal_connect_object(GTK_OBJECT(entry),"changed",text_changed_func,(gpointer)entry);

  if (_GckAutoShowFlag == TRUE && (container!=NULL || hbox!=NULL))
    gtk_widget_show(entry);
  
  gtk_object_set_data(GTK_OBJECT(entry),"EntryLabel",(gpointer)label);
  
  g_function_leave("gck_entryfield_text_new");

  return (entry);
}

/****************************************/
/* Create (double) entry field template */
/****************************************/

GtkWidget *gck_entryfield_new(char *name, GtkWidget *container,
                              double initial_value,
                              GtkSignalFunc value_changed_func)
{
  char buffer[64];

  sprintf(buffer, "%f", initial_value);

  return (gck_entryfield_text_new(name,container,buffer,value_changed_func));
}

/*******************************/
/* Create push button template */
/*******************************/

GtkWidget *gck_pushbutton_new(char *name, GtkWidget *container,
                              gint expand, gint fill, gint padding,
                              GtkSignalFunc button_clicked_func)
{
  GtkWidget *button, *label;
  gint container_type;

  g_function_enter("gck_pushbutton_new");

  button = gtk_button_new();
  
  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), button, expand, fill, padding);
      else
        gtk_container_add(GTK_CONTAINER(container), button);
    }

  if (button_clicked_func != NULL)
    gtk_signal_connect_object(GTK_OBJECT(button),"clicked",button_clicked_func,(gpointer)button);

  if (name != NULL)
    {
      label = gtk_label_new(name);
      gtk_container_add(GTK_CONTAINER(button), label);
      gtk_widget_show(label);
    }

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(button);

  g_function_leave("gck_pushbutton_new");
  return (button);
}

GtkWidget *gck_pushbutton_pixmap_new(char *name,
                                     GdkPixmap *pixm,
                                     GdkBitmap *mask,
                                     GtkWidget *container,
                                     gint expand,gint fill,gint padding,
                                     GtkSignalFunc button_clicked_func)
{
  GtkWidget *button, *cont;
  gint container_type;

  g_function_enter("gck_pushbutton_pixmap_new");

  button = gtk_button_new();
  
  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), button, expand, fill, padding);
      else
        gtk_container_add(GTK_CONTAINER(container), button);
    }

  if (button_clicked_func != NULL)
    gtk_signal_connect_object(GTK_OBJECT(button),"clicked",button_clicked_func,(gpointer)button);

  if (name!=NULL && pixm!=NULL)
    cont=gck_hbox_new(button, FALSE,FALSE,TRUE,0,0,1);
  else
    cont=button;

  if (pixm!=NULL)
    gck_pixmap_new(pixm,mask,cont);

  if (name != NULL)
    gck_label_new(name,cont);

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(button);

  g_function_leave("gck_pushbutton_pixmap_new");
  return (button);  
}

GtkWidget *gck_togglebutton_pixmap_new(char *name,
                                       GdkPixmap *pixm,
                                       GdkBitmap *mask,
                                       GtkWidget *container,
                                       gint expand,gint fill,gint padding,
                                       GtkSignalFunc button_toggled_func)
{
  GtkWidget *button, *cont;
  gint container_type;

  g_function_enter("gck_togglebutton_pixmap_new");

  button = gtk_toggle_button_new();
  
  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), button, expand, fill, padding);
      else
        gtk_container_add(GTK_CONTAINER(container), button);
    }

  if (button_toggled_func != NULL)
    gtk_signal_connect_object(GTK_OBJECT(button),"toggled",button_toggled_func,(gpointer)button);

  if (name!=NULL && pixm!=NULL)
    cont=gck_hbox_new(button, FALSE,FALSE,TRUE,0,0,1);
  else
    cont=button;

  if (pixm!=NULL)
    gck_pixmap_new(pixm,mask,cont);

  if (name != NULL)
    gck_label_new(name,cont);

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(button);

  g_function_leave("gck_togglebutton_pixmap_new");
  return (button);  
}

/********************************/
/* Create check button template */
/********************************/

GtkWidget *gck_checkbutton_new(char *name, GtkWidget *container,
                               gint value,
                               GtkSignalFunc status_changed_func)
{
  GtkWidget *button;
  gint container_type;

  g_function_enter("gck_checkbutton_new");

  if (name == NULL)
    button = gtk_check_button_new();
  else
    button = gtk_check_button_new_with_label(name);

  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), button, TRUE, TRUE, 0);
      else
        gtk_container_add(GTK_CONTAINER(container), button);
    }

  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button),value);

  if (status_changed_func!=NULL)
    gtk_signal_connect_object(GTK_OBJECT(button),"toggled",status_changed_func,(gpointer)button);

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(button);

  g_function_leave("gck_checkbutton_new");
  return (button);
}

/********************************/
/* Create radio-button template */
/********************************/

GtkWidget *gck_radiobutton_new(char *name, 
                               GtkWidget *container,
                               GtkWidget *previous,
                               GtkSignalFunc status_changed_func)
{
  GtkWidget *button;
  GSList *group=NULL;
  gint container_type;

  g_function_enter("gck_radiobutton_new");

  if (previous!=NULL)
    group=gtk_radio_button_group(GTK_RADIO_BUTTON(previous));
  
  if (name==NULL)
    button = gtk_radio_button_new(group);
  else
    button = gtk_radio_button_new_with_label(group,name);

  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), button, TRUE, TRUE, 0);
      else
        gtk_container_add(GTK_CONTAINER(container), button);
    }

  if (status_changed_func != NULL)
    gtk_signal_connect_object(GTK_OBJECT(button),"toggled",status_changed_func,(gpointer)button);

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(button);

  g_function_leave("gck_radiobutton_new");
  return (button);
}

GtkWidget *gck_radiobutton_pixmap_new(char *name, 
                                      GdkPixmap *pixm,
                                      GdkBitmap *mask,
                                      GtkWidget *container,
                                      GtkWidget *previous,
                                      GtkSignalFunc status_changed_func)
{
  GtkWidget *button,*cont;
  GSList *group=NULL;

  g_function_enter("gck_radiobutton_pixmap_new");

  if (previous != NULL)
    group=gtk_radio_button_group(GTK_RADIO_BUTTON(previous));
  
  button = gtk_radio_button_new(group);

  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button),FALSE);

  if (container!=NULL)
    {
      if (GTK_IS_VBOX(container) || GTK_IS_HBOX(container))
        gtk_box_pack_start(GTK_BOX(container), button, TRUE, TRUE, 0);
      else
        gtk_container_add(GTK_CONTAINER(container), button);
    }

  if (name!=NULL && pixm!=NULL)
    cont=gck_hbox_new(button, FALSE,FALSE,TRUE,0,0,1);
  else
    cont=button;

  if (pixm != NULL)
    gck_pixmap_new(pixm,mask,cont);

  if (name != NULL)
    gck_label_new(name,cont);

  if (status_changed_func != NULL)
    gtk_signal_connect_object(GTK_OBJECT(button),"toggled",status_changed_func,(gpointer)button);

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(button);

  g_function_leave("gck_radiobutton_pixmap_new");
  return (button);
}

/********************************/
/* Create vertical box template */
/********************************/

GtkWidget *gck_vbox_new(GtkWidget * container,
                        gint homogenous, gint expand, gint fill,
	                gint spacing, gint padding, gint borderwidth)
{
  GtkWidget *vbox;
  gint container_type;

  g_function_enter("gck_vbox_new");

  vbox = gtk_vbox_new(homogenous, spacing);
  
  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), vbox, expand, fill, padding);
      else
        gtk_container_add(GTK_CONTAINER(container), vbox);
    }

  gtk_container_border_width(GTK_CONTAINER(vbox), borderwidth);

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(vbox);

  g_function_leave("gck_vbox_new");
  return (vbox);
}

/**********************************/
/* Create horizontal box template */
/**********************************/

GtkWidget *gck_hbox_new(GtkWidget * container,
                        gint homogenous, gint expand, gint fill,
	                gint spacing, gint padding, gint borderwidth)
{
  GtkWidget *hbox;
  gint container_type;

  g_function_enter("gck_hbox_new");

  hbox = gtk_hbox_new(homogenous, spacing);

  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), hbox, expand, fill, padding);
      else
        gtk_container_add(GTK_CONTAINER(container), hbox);
    }

  gtk_container_border_width(GTK_CONTAINER(hbox), borderwidth);

  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(hbox);

  g_function_leave("gck_hbox_new");
  return (hbox);
}

/**********************/
/* Create option menu */
/**********************/

GtkWidget *gck_option_menu_new(char *name, GtkWidget *container,
                               gint expand, gint fill, gint padding,
                               char *item_labels[], 
                               GtkSignalFunc item_selected_func,
                               gpointer data)
{
  GtkWidget *optionmenu, *menu, *menuitem,*cont,*label;
  gint i = 0, container_type;

  g_function_enter("gck_option_menu_new");

  optionmenu = gtk_option_menu_new();

  if (name!=NULL)
    {
      cont=gck_hbox_new(container,FALSE,FALSE,FALSE,5,0,0);
      label=gck_label_new(name,cont);
    }
  else
    cont=container;

  if (cont!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(cont);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(cont), optionmenu, expand, fill, padding);
      else
        gtk_container_add(GTK_CONTAINER(cont),optionmenu);
    }

  menu = gtk_menu_new();

  while (item_labels[i] != NULL)
    {
      menuitem = gtk_menu_item_new_with_label(item_labels[i]);

      gtk_object_set_data(GTK_OBJECT(menuitem),"_GckOptionMenuItemID",(gpointer)i);

      if (item_selected_func!=NULL)
        gtk_signal_connect(GTK_OBJECT(menuitem),"activate",
          item_selected_func,data);

      gtk_container_add(GTK_CONTAINER(menu), menuitem);
      gtk_widget_show(menuitem);
      i++;
    }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
  if (_GckAutoShowFlag == TRUE)
    gtk_widget_show(optionmenu);

  g_function_leave("gck_option_menu_new");
  return (optionmenu);
}

/******************/
/* Create menubar */
/******************/

GtkWidget *gck_menu_bar_new(GtkWidget *container,GckMenuItem menu_items[],
                            GtkAcceleratorTable *acc_table)
{
  GtkWidget *menubar,*menu_item;
  gint container_type;

  g_function_enter("gck_menu_bar_new");

  menubar=gtk_menu_bar_new();

  if (container!=NULL)
    {
      container_type=GTK_WIDGET_TYPE(container);
      if (container_type == gtk_vbox_get_type() || container_type == gtk_hbox_get_type())
        gtk_box_pack_start(GTK_BOX(container), menubar, FALSE, TRUE, 0);
      else
        gtk_container_add(GTK_CONTAINER(container),menubar);
    }

  if (menu_items!=NULL)
    {
      while (menu_items->label!=NULL)
        {
          menu_item = gtk_menu_item_new_with_label(menu_items->label);
          gtk_container_add(GTK_CONTAINER(menubar),menu_item);
    
          gtk_object_set_data(GTK_OBJECT(menu_item),"_GckMenuItem",(gpointer)menu_items);
    
          if (menu_items->item_selected_func!=NULL)
            gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
              (GtkSignalFunc)menu_items->item_selected_func,(gpointer)menu_item);
    
          if (menu_items->subitems!=NULL)
	    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),
              gck_menu_new(menu_items->subitems,acc_table));
    
          gtk_widget_show(menu_item);
          menu_items->widget = menu_item;
    
          menu_items++;
        }
    }

  if (_GckAutoShowFlag==TRUE)
    gtk_widget_show(menubar);

  g_function_leave("gck_menu_bar_new");
  return(menubar);
}

/***************/
/* Create menu */
/***************/

GtkWidget *gck_menu_new(GckMenuItem *menu_items,GtkAcceleratorTable *acc_table)
{
  GtkWidget *menu,*menu_item;
  gint i=0;

  g_function_enter("gck_menu_new");

  menu = gtk_menu_new();
  
  while (menu_items[i].label!=NULL)
    {
      if (menu_items[i].label[0] == '-')
        menu_item = gtk_menu_item_new();
      else 
	{
	  menu_item = gtk_menu_item_new_with_label(menu_items[i].label);
	  if (menu_items->accelerator_key && acc_table)
            gtk_widget_install_accelerator(menu_item,acc_table,
                                           menu_items[i].label,
                                           menu_items[i].accelerator_key,
					   menu_items[i].accelerator_mods);

          gtk_object_set_data(GTK_OBJECT(menu_item),"_GckMenuItem",(gpointer)&menu_items[i]);

          if (menu_items[i].item_selected_func!=NULL)
            gtk_signal_connect(GTK_OBJECT(menu_item),"activate",
              (GtkSignalFunc)menu_items[i].item_selected_func,(gpointer)menu_item);
	}

      gtk_container_add(GTK_CONTAINER(menu),menu_item);

      if (menu_items[i].subitems!=NULL)
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),
          gck_menu_new(menu_items[i].subitems,acc_table));

      gtk_widget_show(menu_item);
      menu_items[i].widget = menu_item;

      i++;
    }

  g_function_leave("gck_menu_new");
  return(menu);
}

/**************************************************/
/* Create option menu with image names. Constrain */
/* shown types with mask given in "Constrain".    */
/**************************************************/

GtkWidget *gck_image_menu_new(char *name,
                              GtkWidget * container,
                              gint expand, gint fill,
                              gint padding, gint Constrain,
                              GtkSignalFunc item_selected_func)
{
  GtkWidget *imagemenu = NULL;

  g_function_enter("gck_image_menu_new");

  /* Umm.. Hmm... I think I'll rather wait for Pete's */
  /* procedural database stuff :)                     */

  g_function_leave("gck_image_menu_new");
  return (imagemenu);
}
