/* gap_arr_dialog.h
 * 1998.May.23 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins (Standard array dialog)
 *
 * - p_array_dialog   Dialog Window with one or more rows
 *                    each row can contain one of the following GAP widgets:
 *                       - float pair widget
 *                         (horizontal slidebar combined with a float input field)
 *                       - int pair widget
 *                         (horizontal slidebar combined with a int input field)
 *                       - Toggle Button widget
 *                       - Textentry widget
 *                       - Float entry widget
 *                       - Int entry widget
 * - p_slider_dialog
 *                         simplified call of p_pair_array_dialog,
 *                         using an array with one WGT_INT_PAIR.
 * - p_buttons_dialog
 *
 *
 *
 */
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

/* revision history:
 * gimp    1.1.17b; 2000/01/26  hof: 
 * version 0.96.03; 1998/08/15  hof: p_arr_gtk_init 
 * version 0.96.00; 1998/07/09  hof: 1.st release 
 *                                   (re-implementation of gap_sld_dialog.c)
 */

#ifndef _ARR_DIALOG_H
#define _ARR_DIALOG_H

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

typedef enum
{
   WGT_LABEL        
  ,WGT_TEXT       
  ,WGT_INT        
  ,WGT_FLT        
  ,WGT_TOGGLE     
  ,WGT_RADIO      
  ,WGT_OPTIONMENU      
  ,WGT_FLT_PAIR   
  ,WGT_INT_PAIR   
  ,WGT_ACT_BUTTON 
  ,WGT_FILESEL
  ,WGT_LABEL_LEFT
  ,WGT_LABEL_RIGHT
} t_gap_widget;

typedef int (*t_action_func) ( gpointer action_data);
/*
 * - If one of the Args has set 'has_default' to TRUE
 *   the action Area will contain an additional Button 'Default'
 *
 */

typedef struct {
  t_gap_widget widget_type;

  /* common fields for all widget types */
  char    *label_txt;
  char    *help_txt;
  gint     entry_width;  /* for all Widgets with  an entry */
  gint     scale_width;  /* for the Widgets with a scale */
  gint     constraint;   /* TRUE: check for min/max values */
  gint     has_default;  /* TRUE: default value available */
  
  /* flt_ fileds are used for WGT_FLT and WGT_FLT_PAIR */
  gint     flt_digits;    /* digits behind comma */
  gdouble  flt_min;
  gdouble  flt_max;
  gdouble  flt_step;
  gdouble  flt_default;
  gdouble  flt_ret;
  
  /* int_ fileds are used for WGT_INT and WGT_INT_PAIR WGT_TOGGLE */
  gint     int_min;
  gint     int_max;
  gint     int_step;
  gint     int_default;
  gint     int_ret;
  gint     int_ret_lim;  /* for private (arr_dialog.c) use only */

  /* uncontraint lower /upper limit for WGT_FLT_PAIR and WGT_INT_PAIR */
  gfloat   umin;
  gfloat   umax;
  gfloat   pagestep;


  /* togg_ field are used for WGT_TOGGLE */
  char    *togg_label;    /* extra label attached right to toggle button */
   
  /* radio_ fileds are used for WGT_RADIO and WGT_OPTIONMENU */
  gint     radio_argc;
  gint     radio_default;
  gint     radio_ret;
  char   **radio_argv;
  char   **radio_help_argv;
  
  /* text_ fileds are used for WGT_TEXT */
  gint     text_buf_len;         /* common length for init, default and ret text_buffers */
  char    *text_buf_default;
  char    *text_buf_ret;
  GtkWidget  *text_filesel; /* for private (arr_dialog.c) use only */
  GtkWidget  *text_entry;   /* for private (arr_dialog.c) use only */

  /* action_ fileds are used for WGT_ACT_BUTTON */
  t_action_func action_functon;  
  gpointer      action_data;  

} t_arr_arg;


typedef struct {
  char      *but_txt;
  gint       but_val;
} t_but_arg;

void     p_init_arr_arg  (t_arr_arg *arr_ptr,
                          gint       widget_type);
 
gint     p_array_dialog  (char     *title_txt,
                          char     *frame_txt,
                          int       argc,
                          t_arr_arg argv[]);

long     p_slider_dialog(char *title_txt,
                         char *frame_txt,
                         char *label_txt,
                         char *tooltip_txt,
                         long min, long max, long curr, long constraint);



gint     p_buttons_dialog (char *title_txt,
                         char *frame_txt,
                         int        b_argc,
                         t_but_arg  b_argv[],
                         gint       b_def_val);


gint     p_array_std_dialog  (char     *title_txt,
                          char     *frame_txt,
                          int       argc,
                          t_arr_arg argv[],
                          int       b_argc,
                          t_but_arg b_argv[],
                          gint      b_def_val);
                               
gint     p_arr_gtk_init(gint flag);

#endif
