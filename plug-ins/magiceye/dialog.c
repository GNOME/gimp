#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpmenu.h>
#include "magiceye.h"

int returnvalue = 0;
gint magiceye_ID;


gint magic_dialog(void)
{
  GtkWidget *dialog, *button, *label, *menue, *omenue, *frame, *table, *scale, *toggle;
  gchar **argv;
  gint argc;

  GtkObject *scale_data;
  argc = 1;
  argv = g_new(gchar *,1);
  argv[0] = g_strdup("magiceye");

  gtk_init(&argc,&argv);

  /* Dialog */
  dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Magic Eye");
  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy", (GtkSignalFunc) close_callback, NULL);

  
  /* OK-Button */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) ok_callback,
                      dialog);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);


  /* Create-Button */
  button = gtk_button_new_with_label("Create");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		    (GtkSignalFunc) magiceye_update,
		     NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show(button);


  /* Cancel-Button */
  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT (dialog));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);


  /* Rahmen */
  frame = gtk_frame_new("Magic Eye Options");
  gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width(GTK_CONTAINER(frame),10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, TRUE, TRUE, 0);


  /* Tabelle */
  table = gtk_table_new(6, 4, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 5);
  gtk_container_add(GTK_CONTAINER(frame), table);

  gtk_table_set_row_spacings (GTK_TABLE (table),5);
  gtk_table_set_col_spacings (GTK_TABLE (table),5);


  /* Label */
  label = gtk_label_new("Background: ");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);


  /* Menue */
  omenue = gtk_option_menu_new();
  menue = GTK_WIDGET(gimp_drawable_menu_new(magiceye_constrain,drawable_callback,&magiceye_ID,magiceye_ID));
  gtk_option_menu_set_menu(GTK_OPTION_MENU(omenue), menue);
  gtk_table_attach(GTK_TABLE(table), omenue, 1, 4, 1, 2, GTK_FILL, GTK_FILL, 0, 0);


  /* Label */
  label = gtk_label_new("Strip-Width: ");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);


  /* Breiten-Regler */
  strip_width = STDWIDTH;
  scale_data = gtk_adjustment_new (strip_width, 0.0, 300.0, 1.0, 10.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) scale_update,
		      &strip_width);
  gtk_table_attach(GTK_TABLE(table), scale, 0, 4, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(scale);
 

  /* Label */
  label = gtk_label_new("Depth: ");
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);


  /* Tiefen-Regler */
  depth = STDDEPTH;
  scale_data = gtk_adjustment_new (depth, 0.0, 100.0, 1.0, 10.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) scale_update,
		      &depth);
  gtk_table_attach(GTK_TABLE(table), scale, 1, 4, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(scale); 


  /* Links oder Mitte? */
  from_left = STDFROMLEFT;
  toggle = gtk_check_button_new_with_label ("From left");
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) toggle_update,
                      &from_left);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), from_left);
  gtk_table_attach(GTK_TABLE(table), toggle, 0, 2, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (toggle);


  /* Hoch oder runter? */
  up_down = STDUPDOWN;
  toggle = gtk_check_button_new_with_label ("Down");
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      (GtkSignalFunc) toggle_update,
                      &up_down);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), up_down);
  gtk_table_attach(GTK_TABLE(table), toggle, 2, 4, 5, 6, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (toggle);
  

  /* Darstellung */
  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_widget_show(omenue);
  gtk_widget_show(table);
  gtk_widget_show(frame);
  gtk_widget_show(dialog);
  gtk_main();
  return returnvalue;
}

void
scale_update (GtkAdjustment *adjustment,
	      double        *scale_val)
{
  *scale_val = adjustment->value;
}

void
toggle_update(GtkWidget *widget,
	      gpointer data)
{
	int *toggle_val;

	toggle_val = (int *) data;

	if (GTK_TOGGLE_BUTTON (widget)->active)
		*toggle_val = TRUE;
	else
		*toggle_val = FALSE;
}

void close_callback(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

void ok_callback(GtkWidget *widget, gpointer data)
{
  returnvalue = 1;
  gtk_widget_destroy(GTK_WIDGET(data));
}

gint magiceye_constrain(gint32 image, gint32 drawable_ID, gpointer data)
{
  gint moeglich;  

  moeglich = (gimp_drawable_color(drawable_ID) || gimp_drawable_indexed(drawable_ID) ||gimp_drawable_gray(drawable_ID)); 
#ifdef DEBUG
  printf("%i %i\n",moeglich,drawable_ID);
#endif
  if (drawable_ID == -1) return TRUE;
  return moeglich;
}

void drawable_callback(gint32 id, gpointer data)
{
  /* (gint32 *) data = id; */
  magiceye_ID = id;
}

void magiceye_update(GtkWidget *widget, gpointer data)
{
  gint32 image_ID, new_layer;

  image_ID = magiceye (map_drawable, &new_layer);
  if (image_ID>0) gimp_display_new (image_ID);
}
