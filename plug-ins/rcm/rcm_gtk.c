#include "rcm.h"

GtkWidget *gray_preview;
extern RcmParams Current;

/********************************************************/
/*********************** Callbacks **********************/
/********************************************************/

void
rcm_close_callback (GtkWidget *widget,
		    gpointer   data)
{
  gtk_main_quit ();
}

void
rcm_ok_callback (GtkWidget *widget,
		 gpointer   data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
  Current.Success = 1;
}

void 
rcm_preview_what(GtkWidget *button,
		 gpointer  data)
{
  if (!GTK_TOGGLE_BUTTON(button)->active)
    return;
  if (!strcmp(data, "Entire Image"))
    Current.Slctn = ENTIRE_IMAGE;
  else if (!strcmp(data, "Selection Only"))
    Current.Slctn = SELECTION;
  else if (!strcmp(data, "Selection In Context"))
    Current.Slctn = SELECTION_IN_CONTEXT;
}

void 
rcm_360_degrees (GtkWidget *button, 
		 RcmCircle *circle)
{
  circle->action_flag = DO_NOTHING;
  gtk_widget_draw(circle->preview,NULL);
  circle->angle->beta=circle->angle->alpha-circle->angle->cw_ccw*0.001;
  rcm_draw_arrows(circle->preview->window,
		  circle->preview->style->black_gc,
		  circle->angle); 
  circle->action_flag = VIRGIN;
  rcm_render_preview(Current.Bna->after,CURRENT);
}

void 
rcm_cw_ccw  (GtkWidget *button, 
	     RcmCircle *circle)
{
  circle->angle->cw_ccw *= -1;
  pmg_set_xpm_in_button_label(circle->cw_ccw_xpm_button,
			      (circle->angle->cw_ccw>0)?
			      "/home/pavel/rcm/rcm_cw.xpm":
			      "/home/pavel/rcm/rcm_ccw.xpm");
  gtk_label_set(GTK_LABEL(circle->cw_ccw_xpm_button->additional_label),
		(circle->angle->cw_ccw>0)?
		"Clockwise":
		"C/Clockwise");
  rcm_a_to_b  (button, circle);
}

void 
rcm_a_to_b  (GtkWidget *button, 
	     RcmCircle *circle)
{
  circle->action_flag = DO_NOTHING;
  gtk_widget_draw(circle->preview,NULL);
  SWAP(circle->angle->alpha, circle->angle->beta);
  rcm_draw_arrows(circle->preview->window,
		  circle->preview->style->black_gc,
		  circle->angle); 
  circle->action_flag = VIRGIN;
  rcm_render_preview(Current.Bna->after, CURRENT);
}

void 
rcm_preview_as_you_drag  (GtkWidget *button, 
			  gpointer  *value)
{
  if (GTK_TOGGLE_BUTTON(button)->active) 
    Current.RealTime = TRUE;
  else
    Current.RealTime = FALSE;
}

void 
rcm_switch_to_degrees  (GtkWidget *button, 
			gpointer  *value)
{
  if (GTK_TOGGLE_BUTTON(button)->active)
    rcm_update_entries(Current.Units = DEGREES);  
}

void 
rcm_switch_to_radians  (GtkWidget *button, 
			gpointer  *value)
{
  if (GTK_TOGGLE_BUTTON(button)->active) 
    rcm_update_entries(Current.Units = RADIANS);
}

void 
rcm_switch_to_radians_over_PI  (GtkWidget *button, 
				gpointer  *value)
{
  if (GTK_TOGGLE_BUTTON(button)->active) 
    rcm_update_entries(Current.Units = RADIANS_OVER_PI);
}

void rcm_switch_to_gray_to  (GtkWidget *button, 
			     gpointer  *value)
{
  if (!GTK_TOGGLE_BUTTON(button)->active)    return;

  Current.Gray_to_from=GRAY_TO;
  rcm_render_preview(Current.Bna->after,CURRENT);
}

void rcm_switch_to_gray_from (GtkWidget *button, 
			      gpointer *value)
{
  if (!(GTK_TOGGLE_BUTTON(button)->active)) return;

  Current.Gray_to_from=GRAY_FROM;
  rcm_render_preview(Current.Bna->after,CURRENT);
}

void
rcm_change_preview()
{
  gtk_preview_size (GTK_PREVIEW (Current.Bna->before),
		    Current.reduced->width,
		    Current.reduced->height);
  gtk_preview_size (GTK_PREVIEW (Current.Bna->after),
		    Current.reduced->width,
		    Current.reduced->height);
  
  rcm_render_preview(Current.Bna->before, ORIGINAL);
  rcm_render_preview(Current.Bna->after, CURRENT);
  /*
    gtk_container_check_resize(GTK_CONTAINER(Current.Bna->bna_frame), NULL);
    */
  gtk_widget_draw(Current.Bna->before,NULL);
  gtk_widget_draw(Current.Bna->after,NULL);
}

void rcm_selection_in_context (GtkWidget *button, 
			       gpointer  *value)
{
  Current.reduced = Reduce_The_Image(Current.drawable,
				     Current.mask,
				     MAX_PREVIEW_SIZE,
				     SELECTION_IN_CONTEXT);
  rcm_change_preview();
}

void rcm_selection (GtkWidget *button, 
		    gpointer  *value)
{
  Current.reduced = Reduce_The_Image(Current.drawable,Current.mask,
				     MAX_PREVIEW_SIZE,
				     SELECTION);
  rcm_change_preview();
  
}

void rcm_entire_image (GtkWidget *button, 
		       gpointer  *value)
{
  Current.reduced = Reduce_The_Image(Current.drawable,Current.mask,
				     MAX_PREVIEW_SIZE,
				     ENTIRE_IMAGE);
  rcm_change_preview();  
}

/********************************************************/
/********************* Build Helpers ********************/
/********************************************************/

GtkWidget *
rcm_label_in_a_table(GtkWidget *table,
		     guchar    *string,
		     gint       x_spot,		   
		     gint       y_spot)
{
  GtkWidget *label = gtk_label_new (string);
  gtk_widget_show(label);
  
  gtk_table_attach(GTK_TABLE(table),label,x_spot,x_spot+1,y_spot,y_spot+1,
		   GTK_EXPAND|GTK_FILL,
		   GTK_EXPAND,
		   0,0);
  return label;
}

GtkWidget *
rcm_button_in_a_table(GtkWidget     *table,
		      guchar        *string,
		      int            x_spot,		   
		      int            y_spot,		   
		      GtkSignalFunc *function,
		      gpointer      *data)
{
  GtkWidget *button = gtk_button_new_with_label (string);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc)function,
                      data);
  gtk_widget_show (button);
  
  gtk_table_attach(GTK_TABLE(table), button,
		   x_spot, x_spot+1, y_spot, y_spot+1,
		   GTK_EXPAND|GTK_FILL,
		   GTK_EXPAND,
		   0,0);
  return button;
}

GtkWidget *
rcm_entry_in_a_table(GtkWidget    *table,
		     float         value,
		     gint          x_spot,		   
		     gint          y_spot,
		     GtkSignalFunc func,
		     gpointer      data)
{
  GtkWidget *entry=gtk_entry_new();
  gtk_widget_show(entry);
  gtk_widget_set_usize(GTK_WIDGET(entry),42,20);
  rcm_float_in_an_entry(entry,value);
  gtk_table_attach(GTK_TABLE(table),entry,x_spot,x_spot+1,y_spot,y_spot+1,
		   0,0,0,0);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     func, data);
  return entry;
}

void 
Check_Button_In_A_Box       (GtkWidget     *vbox,
			     guchar        *label,
			     GtkSignalFunc  function,
			     gpointer       data,
			     gint           clicked)
{
  GtkWidget *button;

  button = gtk_check_button_new_with_label(label);
  gtk_box_pack_start (GTK_BOX(vbox),button, TRUE, TRUE, 0);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), clicked);
   
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) function,
		      data);

  gtk_widget_show(button);
}

GSList *  
Button_In_A_Box        (GtkWidget     *vbox,
			GSList        *group,
			guchar        *label,
			GtkSignalFunc  function,
			gpointer       data,
			int            clicked)
{
  GtkWidget *button;
  
  button=gtk_radio_button_new_with_label(group,label);
  gtk_widget_show(button);
  gtk_box_pack_start (GTK_BOX(vbox),button, TRUE, TRUE, 0);
  if (clicked)
    gtk_button_clicked(GTK_BUTTON(button));

  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) function,
		      data);

  return gtk_radio_button_group(GTK_RADIO_BUTTON(button));
}

/********************************************************/
/********************* The 3 Frames *********************/
/********************************************************/

GtkWidget *
rcm_create_bna()
{
  GtkWidget *frame, *blabel, *alabel, *bframe, *aframe, *table;
  
  Create_A_Preview(&Current.Bna->before,
		   &bframe, 
		   Current.reduced->width, 
		   Current.reduced->height,
		   1);
  Create_A_Preview(&Current.Bna->after,
		   &aframe, 
		   Current.reduced->width, 
		   Current.reduced->height,
		   1);
 
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  
  /* All the previews */
  alabel = gtk_label_new("Rotated:");
  gtk_widget_show(alabel);
  
  blabel = gtk_label_new("Original:");
  gtk_widget_show(blabel);

  table = gtk_table_new(4,1,FALSE);
  gtk_container_border_width(GTK_CONTAINER(table),10);
  gtk_table_set_row_spacings(GTK_TABLE(table),0);
  gtk_table_set_col_spacings(GTK_TABLE(table),20);
  
  gtk_container_add (GTK_CONTAINER (frame), table);
  
  gtk_table_attach(GTK_TABLE(table), blabel, 0, 1, 0, 1, 
		   0,
		   GTK_EXPAND|GTK_FILL,
		   0,0);
  gtk_table_attach(GTK_TABLE(table),bframe,0,1,1,2,
		   GTK_EXPAND,
		   GTK_EXPAND,
		   0,0);
  gtk_table_attach(GTK_TABLE(table),alabel,0,1,2,3,
		   0,
		   GTK_EXPAND|GTK_FILL,
		   0,0);
  gtk_table_attach(GTK_TABLE(table),aframe,0,1,3,4,
		   GTK_EXPAND,
		   GTK_EXPAND,
		   0,0);

  gtk_widget_show(table);
  gtk_widget_show (frame);
  return frame;
}


RcmGray *
rcm_create_gray()
{
  GtkWidget *frame, *preview, *as_or_to_frame;
  GtkWidget *table, *previewframe, *legend_table;
  GtkWidget *gray_sat_frame;
  GSList *group = NULL;
  
  RcmGray *st = g_new(RcmGray,1);
  st->hue = 0;
  st->satur = 0;
  st->action_flag = VIRGIN;

  st->frame = frame = gtk_frame_new("Gray");
  gtk_widget_show(frame);

 
  {
    GtkWidget *label, *table, *entry;
    gray_sat_frame = gtk_frame_new("What is Gray?");
    gtk_container_border_width (GTK_CONTAINER (gray_sat_frame), 10);
    gtk_widget_show(gray_sat_frame);
    
    table = gtk_table_new(1, 3, FALSE);
    gtk_widget_show(table);
    
    label = gtk_label_new("Sat <=");
    gtk_widget_show(label);
    
    st->gray_sat_entry = entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_widget_set_usize(GTK_WIDGET(entry),42,20);
    rcm_float_in_an_entry(entry,st->gray_sat = INITIAL_GRAY_SAT);
    gtk_signal_connect(GTK_OBJECT(entry), "changed",
		       (GtkSignalFunc)rcm_set_gray_sat,
		       st);
    
    gtk_table_attach(GTK_TABLE(table), label,
		     0, 1, 0, 1,
		     GTK_EXPAND|GTK_FILL,
		     GTK_EXPAND,
		     0,0);

    gtk_table_attach(GTK_TABLE(table), entry,
		     1, 2, 0, 1,
		     GTK_EXPAND|GTK_FILL,
		     GTK_EXPAND,
		     5,0);

    gtk_container_add(GTK_CONTAINER(gray_sat_frame), table);
  }

  {
    previewframe = gtk_frame_new(NULL);
    gtk_container_border_width (GTK_CONTAINER(previewframe),5);
    gtk_widget_show(previewframe);
    
    st->preview = preview = gtk_preview_new (GTK_PREVIEW_COLOR);
    gtk_preview_size (GTK_PREVIEW (preview), GRAY_SUM, GRAY_SUM);
    gtk_widget_show(preview);
    gtk_container_add(GTK_CONTAINER(previewframe), preview);
    
    gtk_widget_set_events(preview,RANGE_ADJUST_MASK);
    gtk_signal_connect_after(GTK_OBJECT(preview),"expose_event",
			     (GtkSignalFunc) rcm_gray_expose_event ,
			     st);
    
    gtk_signal_connect(GTK_OBJECT(preview),"button_press_event",
		       (GtkSignalFunc) rcm_gray_button_press_event,
		       st);
    
    gtk_signal_connect(GTK_OBJECT(preview),"button_release_event",
		       (GtkSignalFunc) rcm_gray_release_event,
		       st);
    
    gtk_signal_connect(GTK_OBJECT(preview),"motion_notify_event",
		       (GtkSignalFunc) rcm_gray_motion_notify_event,
		       st);
  }
  
  {
    legend_table = gtk_table_new(1,5,FALSE);
    
    gtk_widget_show(legend_table);
    gtk_container_border_width (GTK_CONTAINER (legend_table), 5);
    rcm_label_in_a_table(legend_table,"H:",0,0);
    st->hue_entry = rcm_entry_in_a_table(legend_table,
						st->hue,1,0,
						(GtkSignalFunc) rcm_set_hue,
						st);
    st->hue_units_label=
      rcm_label_in_a_table(legend_table,
			   rcm_units_string(Current.Units),2,0);
    rcm_label_in_a_table(legend_table,"  S:",3,0);
    st->satur_entry=rcm_entry_in_a_table(legend_table,
						st->satur,4,0,
						(GtkSignalFunc) rcm_set_satur,
						st);
  }
  
  {
    GtkWidget *box;
    
    as_or_to_frame = gtk_frame_new(NULL);
    gtk_container_border_width (GTK_CONTAINER (as_or_to_frame), 10);
    gtk_widget_show(as_or_to_frame);
    
    box = gtk_vbox_new(FALSE, 3);
    gtk_container_add(GTK_CONTAINER(as_or_to_frame), box);
    gtk_container_border_width(GTK_CONTAINER(box),5);
    gtk_widget_show(box);
    
    group = Button_In_A_Box (box, group,
			     "Treat As This",
			     (GtkSignalFunc) rcm_switch_to_gray_from,
			     &(Current.Gray_to_from),
			     Current.Gray_to_from == GRAY_FROM);
    
    group = Button_In_A_Box (box, group,
			     "Change To This",
			     (GtkSignalFunc) rcm_switch_to_gray_to,
			     &(Current.Gray_to_from),
			     Current.Gray_to_from == GRAY_TO);		
  }
 
  {
    GtkWidget *mini_frame, *mini_table;
    
    mini_frame = gtk_frame_new(NULL);
    gtk_container_border_width (GTK_CONTAINER (mini_frame), 10);
    gtk_widget_show(mini_frame);
    
    mini_table = gtk_table_new(2,1,FALSE);
    gtk_widget_show(mini_table);
    gtk_container_add(GTK_CONTAINER(mini_frame), mini_table);
    
    gtk_table_attach(GTK_TABLE(mini_table), previewframe, 0,1,0,1,
		     GTK_EXPAND,
		     GTK_EXPAND,
		     0,0);
    
    gtk_table_attach(GTK_TABLE(mini_table), legend_table, 0,1,1,2,
		     GTK_EXPAND|GTK_FILL,
		     GTK_EXPAND|GTK_FILL,
		     0,0);
    
    table = gtk_table_new(2, 2, FALSE);
    gtk_widget_show(table);
    gtk_table_attach(GTK_TABLE(table), mini_frame,
		     0, 1, 0, 2,
		     GTK_EXPAND|GTK_FILL,
		     GTK_EXPAND|GTK_FILL,
		     0,0);

    gtk_table_attach(GTK_TABLE(table), as_or_to_frame,
		     1, 2, 0, 1,
		     GTK_EXPAND|GTK_FILL,
		     GTK_EXPAND|GTK_FILL,
		     0,0);

    gtk_table_attach(GTK_TABLE(table), gray_sat_frame,
		     1, 2, 1, 2,
		     GTK_EXPAND|GTK_FILL,
		     GTK_EXPAND|GTK_FILL,
		     0, 0);
  }

  gtk_container_add(GTK_CONTAINER(frame), table);

  rcm_render_circle_preview(preview, GRAY_SUM, GRAY_MARGIN);

  return st;
}

GtkWidget *
rcm_create_misc()
{
  GtkWidget *frame, *label,*table;
  GtkWidget *units_frame, *units_vbox;
  GtkWidget *misc_frame, *misc_vbox;
  GSList *group = NULL;

  Current.Gray = rcm_create_gray();

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);

  /* All the previews */
     
  units_frame = gtk_frame_new("Units");
  gtk_widget_show(units_frame);

  units_vbox=gtk_vbox_new(3, FALSE);
  gtk_container_add(GTK_CONTAINER(units_frame), units_vbox);
  gtk_container_border_width(GTK_CONTAINER(units_vbox), 5);
  gtk_widget_show(units_vbox);

  group = Button_In_A_Box (units_vbox,
			   group,
			   "Radians",
			   (GtkSignalFunc) rcm_switch_to_radians,
			   NULL,
			   Current.Units==RADIANS);

  group = Button_In_A_Box (units_vbox,
			   group,
			   "Radiand/PI",
			   (GtkSignalFunc) rcm_switch_to_radians_over_PI,
			   NULL,
			   Current.Units==RADIANS_OVER_PI);		
  
  group = Button_In_A_Box (units_vbox,
			   group,
			   "Degrees",
			   (GtkSignalFunc) rcm_switch_to_degrees,
			   NULL,
			   Current.Units==DEGREES);		
  
  misc_frame = gtk_frame_new("Very Misc");
  misc_vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(misc_frame), misc_vbox);
  gtk_widget_show(misc_frame);
  gtk_widget_show(misc_vbox);
  
  Check_Button_In_A_Box (misc_vbox, "Preview as you Drag",
			 (GtkSignalFunc) rcm_preview_as_you_drag,
			 &(Current.RealTime),
			 Current.RealTime);
 
 {
    GSList *group = NULL;
    GtkWidget *item, *menu, *root, *hbox;

    hbox = gtk_hbox_new(FALSE,3);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(misc_vbox),hbox, TRUE, TRUE, 5);
 
 
    label = gtk_label_new("Preview");
    gtk_widget_show(label);
    gtk_box_pack_start (GTK_BOX(hbox),label, FALSE, FALSE, 3);

    menu = gtk_menu_new();
    item =gtk_radio_menu_item_new_with_label(group,"Entire Image");
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (item));
    gtk_menu_append(GTK_MENU (menu), item);
    gtk_widget_show(item);
    gtk_signal_connect (GTK_OBJECT (item), "activate",
			(GtkSignalFunc) rcm_entire_image,
			NULL);
    
    item = gtk_radio_menu_item_new_with_label(group,"Selection");
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (item));
    gtk_menu_append(GTK_MENU (menu), item);
    gtk_widget_show(item);
    gtk_signal_connect (GTK_OBJECT (item), "activate",
			(GtkSignalFunc) rcm_selection,
			NULL);

    item = gtk_radio_menu_item_new_with_label(group,"Context");
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (item));
    gtk_widget_show(item);
    gtk_menu_append(GTK_MENU (menu), item);
    gtk_signal_connect (GTK_OBJECT (item), "activate",
			(GtkSignalFunc) rcm_selection_in_context,
			NULL);
			

    root =  gtk_option_menu_new ();
    gtk_option_menu_set_menu (GTK_OPTION_MENU (root), menu);
    gtk_option_menu_set_history (GTK_OPTION_MENU (root), 4);
    gtk_widget_show(root);
    gtk_box_pack_start (GTK_BOX(hbox), root, FALSE, FALSE, 0);
 }

  table = gtk_table_new(2,2,FALSE);
  gtk_widget_show(table);
  gtk_container_border_width(GTK_CONTAINER(table),0);
  
  gtk_container_add (GTK_CONTAINER (frame), table);
  

  gtk_table_attach(GTK_TABLE(table), Current.Gray->frame, 
		   0, 1, 0, 2,
                   GTK_EXPAND|GTK_FILL,
		   GTK_FILL,
		   5, 3);
  
  gtk_table_attach(GTK_TABLE(table), misc_frame,
		   1, 2, 0, 1,
		   GTK_EXPAND|GTK_FILL,
		   GTK_FILL,
		   5, 3);
  
   gtk_table_attach(GTK_TABLE(table), units_frame,
		    1, 2, 1, 2,
		    GTK_EXPAND|GTK_FILL,
		    GTK_FILL,
		    5, 3);
  
 
  gtk_widget_show (frame);
  return frame;
}


GtkWidget *
rcm_create_from_to()
{
  GtkWidget *frame, *blabel, *alabel, *table;
  
  Current.From = rcm_create_square_preview(SUM, "From:");
  Current.To   = rcm_create_square_preview(SUM, "To:");
   
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  
  /* All the previews */
  alabel = gtk_label_new("To:");
  gtk_widget_show(alabel);
  /* gtk_misc_set_alignment(GTK_MISC(alabel), 0.0, 0.5); */

  blabel=gtk_label_new("From:");
  gtk_widget_show(blabel);
  /* gtk_misc_set_alignment(GTK_MISC(blabel), 0.0, 0.5); */

  table = gtk_table_new(3,2,FALSE);
  gtk_container_border_width(GTK_CONTAINER(table),10);
  
  gtk_container_add (GTK_CONTAINER (frame), table);
 
  gtk_table_attach(GTK_TABLE(table),Current.From->table,0,1,0,1,
		   GTK_EXPAND,
		   GTK_EXPAND,
		   0,0);
  gtk_table_attach(GTK_TABLE(table),Current.To->table,0,1,1,2,
		   GTK_EXPAND,
		   GTK_EXPAND,
		   0,0);

  gtk_widget_show(table);
  
  gtk_widget_show (frame);
  return frame;
}

void 
Create_A_Preview (GtkWidget  **preview,
		  GtkWidget  **frame,
		  int          previewWidth,
		  int          previewHeight,
		  int          colorfulness)
{
  *frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (*frame), GTK_SHADOW_IN);
  gtk_container_border_width (GTK_CONTAINER(*frame),0);
  gtk_widget_show(*frame);
  
  *preview = gtk_preview_new (colorfulness?GTK_PREVIEW_COLOR
			    :GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (*preview), previewWidth, previewHeight);
  gtk_widget_show(*preview);
  gtk_container_add(GTK_CONTAINER(*frame),*preview);
}

RcmCircle *
rcm_create_square_preview(gint   height,
			  gchar *label_content)
{
  GtkWidget *label, *frame, *button_table, *legend_table;
  
  RcmCircle *st = g_new(RcmCircle, 1);
  st->action_flag = VIRGIN;
  
  st->angle = g_new(RcmAngle,1);
  st->angle->alpha = INITIAL_ALPHA;
  st->angle->beta = INITIAL_BETA;
  st->angle->cw_ccw = 1;

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_border_width (GTK_CONTAINER(frame),0);
  gtk_widget_show(frame);
  

  st->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (st->preview),height,height);
  gtk_widget_show(st->preview);
  gtk_container_add(GTK_CONTAINER(frame),st->preview);
  

  gtk_widget_set_events(st->preview,RANGE_ADJUST_MASK);
  gtk_signal_connect_after(GTK_OBJECT(st->preview),"expose_event",
			   (GtkSignalFunc) rcm_expose_event ,
			   st);
  
  gtk_signal_connect(GTK_OBJECT(st->preview),"button_press_event",
		     (GtkSignalFunc) rcm_button_press_event,
		     st);
  gtk_signal_connect(GTK_OBJECT(st->preview),"button_release_event",
		     (GtkSignalFunc) rcm_release_event,
		     st);
  
  gtk_signal_connect(GTK_OBJECT(st->preview),"motion_notify_event",
		     (GtkSignalFunc) rcm_motion_notify_event,
		     st);
  
  rcm_render_circle_preview(st->preview,SUM,MARGIN);
  
  label = gtk_label_new(label_content);
  gtk_widget_show(label);
  

 
  {    
    PmgButtonLabelXpm *xpm_button;
    
    button_table = gtk_table_new(3,1,FALSE);
    gtk_widget_show(button_table);
    st->cw_ccw_xpm_button = 
    xpm_button = pmg_button_label_xpm_new(gtk_window_new(0),
					  (st->angle->cw_ccw > 0)?
					  RCM_XPM_DIR"rcm_cw.xpm":
					  RCM_XPM_DIR"rcm_ccw.xpm",
					  "Switch To",
					  (st->angle->cw_ccw>0)?
					  "Clockwise":
					  "C/Clockwise",
					  'h',
					  (GtkSignalFunc *) rcm_cw_ccw, 
					  (gpointer) st);
    
    gtk_table_attach(GTK_TABLE(button_table), xpm_button->button,
		     0,1,0,1,
		     GTK_EXPAND|GTK_FILL,
		     GTK_FILL,
		     5,2);
    
    xpm_button = pmg_button_label_xpm_new(gtk_window_new(0),
					  RCM_XPM_DIR"rcm_a_b.xpm", 
					  "Switch Order",
					  "Of Arrows",
					  'h',
					  (GtkSignalFunc *) rcm_a_to_b, 
					  (gpointer) st);
    
    gtk_table_attach(GTK_TABLE(button_table), xpm_button->button,
		     0,1,1,2,
		     GTK_EXPAND|GTK_FILL,
		     GTK_FILL,
		     5,2);
    
    xpm_button = pmg_button_label_xpm_new(gtk_window_new(0),
					  RCM_XPM_DIR"rcm_360.xpm", 
					  "Select The",
					  "Entire Range",
					  'h',
					  (GtkSignalFunc *) rcm_360_degrees, 
					  (gpointer) st);
    
    gtk_table_attach(GTK_TABLE(button_table), xpm_button->button,
		     0,1,2,3,
		     GTK_EXPAND|GTK_FILL,
		     GTK_FILL,
		     5,2);
  }   

  legend_table = gtk_table_new(1, 6, FALSE);
  gtk_widget_show(legend_table);
  
  rcm_label_in_a_table(legend_table,"From ",0,0);
  
  st->alpha_entry = 
    rcm_entry_in_a_table(legend_table,
			 st->angle->alpha*
			 rcm_units_factor(Current.Units),
			 1,0,
			 (GtkSignalFunc) rcm_set_alpha,
			 st)
;
  st->alpha_units_label =
    rcm_label_in_a_table(legend_table,
			 rcm_units_string(Current.Units),2,0);
  
  rcm_label_in_a_table(legend_table,"  to ",3,0);
  st->beta_entry = 
    rcm_entry_in_a_table(legend_table,
			 st->angle->beta*
			 rcm_units_factor(Current.Units),
			 4,0,
			 (GtkSignalFunc) rcm_set_beta,
			 st);
  
  st->beta_units_label=
    rcm_label_in_a_table(legend_table,
			 rcm_units_string(Current.Units),5,0);
  
  st->table= gtk_table_new(3,2,FALSE);
  gtk_widget_show(st->table);
  
  gtk_table_attach(GTK_TABLE(st->table),label,0,1,0,1,
		   0,
		   GTK_EXPAND|GTK_FILL,
		   0,0);
  
  gtk_table_attach(GTK_TABLE(st->table),frame,0,1,1,2,
		   0,
		   GTK_EXPAND,
		   0,0);
  
  gtk_table_attach(GTK_TABLE(st->table),button_table,1,2,1,2,
		   0,
		   GTK_EXPAND,
		   0,0);
  
  gtk_table_attach(GTK_TABLE(st->table),legend_table,0,1,2,3,
		   0,
		   GTK_EXPAND|GTK_FILL,
		   0,10);
  return st;
}


/***********************************************************/
/********************  MAIN DIALOG  ************************/
/***********************************************************/
gint 
rcm_dialog()
{
  GtkWidget *table, *dlg, *circles, *bna, *misc;
  GtkWidget *OKbutton, *CANCELbutton; 
  GtkWidget *buttonTable;
  
  guchar *color_cube;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("rcm");
  
  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());  
  

  Current.Bna = g_new(RcmBna, 1);

  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
                              color_cube[2], color_cube[3]);

  gtk_widget_set_default_visual(gtk_preview_get_visual()); 
  gtk_widget_set_default_colormap(gtk_preview_get_cmap());  

  Current.Bna->dlg = dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Colormap Rotation");
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) rcm_close_callback,
		      NULL);
  
  OKbutton = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (OKbutton, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT (OKbutton), "clicked",
		     (GtkSignalFunc) rcm_ok_callback,
		     dlg);
  gtk_widget_grab_default (OKbutton);
  gtk_widget_show (OKbutton);

  CANCELbutton = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (CANCELbutton, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (CANCELbutton), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_widget_show (CANCELbutton);

  buttonTable = gtk_table_new(1,4,TRUE);
  gtk_container_border_width(GTK_CONTAINER(buttonTable),0);
  gtk_table_set_col_spacings(GTK_TABLE(buttonTable),3);
  
   
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), 
		      buttonTable, 
		      TRUE, TRUE, 0);

  gtk_table_attach( GTK_TABLE(buttonTable), OKbutton,1,2,0,1, 
		    GTK_FILL|GTK_EXPAND, 0,
		    0, 0);

  gtk_table_attach( GTK_TABLE(buttonTable), CANCELbutton,2,3,0,1, 
		    GTK_FILL|GTK_EXPAND, 0,
		    0, 0);
		   
  gtk_widget_show (buttonTable);

  /*************************************************************/

  Current.reduced = Reduce_The_Image(Current.drawable,
				     Current.mask,
				     MAX_PREVIEW_SIZE,
				     ENTIRE_IMAGE);

  circles                      = rcm_create_from_to(); 
  Current.Bna->bna_frame = bna = rcm_create_bna(); 
  misc                         = rcm_create_misc();

  rcm_render_preview(Current.Bna->before, ORIGINAL);
  rcm_render_preview(Current.Bna->after, CURRENT);

  {
    GtkWidget *notebook;

    notebook = gtk_notebook_new();
    gtk_container_border_width(GTK_CONTAINER(notebook), 10);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), 
      notebook, TRUE, TRUE, 0);
    gtk_widget_show (notebook);
    
    table = gtk_table_new(1, 2, FALSE);
    gtk_widget_show(table);
  
    gtk_table_attach(GTK_TABLE(table),
		     bna, 0, 1, 0, 1,
		     GTK_EXPAND|GTK_FILL,
		     GTK_EXPAND|GTK_FILL,
		     5, 0);
    
    gtk_table_attach(GTK_TABLE(table),
		     circles, 1, 2, 0, 1,
		     GTK_EXPAND|GTK_FILL,
		     GTK_EXPAND|GTK_FILL,
		     5, 0);
    
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, 
			      gtk_label_new("        Main            "));
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), misc, 
			      gtk_label_new("        Misc            "));
  }

  gtk_widget_show(dlg);

  gdk_flush (); 
  gtk_main ();

  return Current.Success;
}
