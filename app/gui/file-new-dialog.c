#include <stdlib.h>
#include <stdio.h>
#include "appenv.h"
#include "about_dialog.h"
#include "actionarea.h"
#include "app_procs.h"
#include "brightness_contrast.h"
#include "brushes.h"
#include "by_color_select.h"
#include "channels_dialog.h"
#include "colormaps.h"
#include "color_balance.h"
#include "commands.h"
#include "convert.h"
#include "curves.h"
#include "desaturate.h"
#include "devices.h"
#include "channel_ops.h"
#include "drawable.h"
#include "equalize.h"
#include "fileops.h"
#include "floating_sel.h"
#include "gdisplay_ops.h"
#include "general.h"
#include "gimage_cmds.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "global_edit.h"
#include "gradient.h"
#include "histogram_tool.h"
#include "hue_saturation.h"
#include "image_render.h"
#include "indexed_palette.h"
#include "info_window.h"
#include "interface.h"
#include "invert.h"
#include "layers_dialog.h"
#include "layer_select.h"
#include "levels.h"
#include "palette.h"
#include "patterns.h"
#include "plug_in.h"
#include "posterize.h"
#include "resize.h"
#include "scale.h"
#include "threshold.h"
#include "tips_dialog.h"
#include "tools.h"
#include "undo.h"

typedef struct {
  GtkWidget *dlg;
  GtkWidget *height_entry;
  GtkWidget *width_entry;
  GtkWidget *height_units_entry;
  GtkWidget *width_units_entry;
  GtkWidget *resolution_entry;
  float resolution;   /* always in dpi */
  float unit;  /* this is a float that is equal to unit/inch, 2.54 for cm for example */
  int width;
  int height;
  int type;
  int fill_type;
} NewImageValues;

/*  new image local functions  */
static void file_new_ok_callback (GtkWidget *, gpointer);
static void file_new_cancel_callback (GtkWidget *, gpointer);
static gint file_new_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void file_new_toggle_callback (GtkWidget *, gpointer);
static void file_new_width_update_callback (GtkWidget *, gpointer);
static void file_new_height_update_callback (GtkWidget *, gpointer);
static void file_new_width_units_update_callback (GtkWidget *, gpointer);
static void file_new_height_units_update_callback (GtkWidget *, gpointer);
static void file_new_resolution_callback (GtkWidget *, gpointer);
static void file_new_units_inch_menu_callback (GtkWidget *, gpointer);
static void file_new_units_cm_menu_callback (GtkWidget *, gpointer);


/*  static variables  */
static   int          last_width = 256;
static   int          last_height = 256;
static   int          last_type = RGB;
static   int          last_fill_type = BACKGROUND_FILL; 
static   float        last_resolution = 72;     /* always in DPI */
static   float        last_unit = 1;

/* these are temps that should be set in gimprc eventually */
static   float        default_resolution = 72;     /* this needs to be set in gimprc */
static   float        default_unit =1;

static int new_dialog_run;


static void
file_new_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  NewImageValues *vals;
  GImage *gimage;
  GDisplay *gdisplay;
  Layer *layer;
  int type;

  vals = data;

  vals->width = atoi (gtk_entry_get_text (GTK_ENTRY (vals->width_entry)));
  vals->height = atoi (gtk_entry_get_text (GTK_ENTRY (vals->height_entry)));
  vals->resolution = atof (gtk_entry_get_text (GTK_ENTRY (vals->resolution_entry)));

  gtk_widget_destroy (vals->dlg);

  last_width = vals->width;
  last_height = vals->height;
  last_type = vals->type;
  last_fill_type = vals->fill_type;
  last_resolution = vals->resolution;
  last_unit = vals->unit;

  switch (vals->fill_type)
    {
    case BACKGROUND_FILL:
    case WHITE_FILL:
      type = (vals->type == RGB) ? RGB_GIMAGE : GRAY_GIMAGE;
      break;
    case TRANSPARENT_FILL:
      type = (vals->type == RGB) ? RGBA_GIMAGE : GRAYA_GIMAGE;
      break;
    default:
      type = RGB_IMAGE;
      break;
    }

  gimage = gimage_new (vals->width, vals->height, vals->type);

  /*  Make the background (or first) layer  */
  layer = layer_new (gimage->ID, gimage->width, gimage->height,
		     type, "Background", OPAQUE_OPACITY, NORMAL);

  if (layer) {
    /*  add the new layer to the gimage  */
    gimage_disable_undo (gimage);
    gimage_add_layer (gimage, layer, 0);
    gimage_enable_undo (gimage);
    
    drawable_fill (GIMP_DRAWABLE(layer), vals->fill_type);

    gimage_clean_all (gimage);
    
    gdisplay = gdisplay_new (gimage, 0x0101);
  }

  g_free (vals);
}

static gint
file_new_delete_callback (GtkWidget *widget,
			  GdkEvent *event,
			  gpointer data)
{
  file_new_cancel_callback (widget, data);

  return TRUE;
}
  

static void
file_new_cancel_callback (GtkWidget *widget,
			  gpointer   data)
{
  NewImageValues *vals;

  vals = data;

  gtk_widget_destroy (vals->dlg);
  g_free (vals);
}

static void
file_new_toggle_callback (GtkWidget *widget,
			  gpointer   data)
{
  int *val;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      val = data;
      *val = (long) gtk_object_get_user_data (GTK_OBJECT (widget));
    }
}

static void
file_new_width_update_callback (GtkWidget *widget,
				gpointer data)
{

  /* uh. like do something here  */
  gchar *newvalue;
  NewImageValues *vals;
  int new_width;
  float temp;
  char buffer[12];
  
  vals = data;
  
  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->width_entry));
  new_width = atoi(newvalue);

  temp = ((float) new_width / (float) vals->resolution) * vals->unit;
  sprintf (buffer, "%.2f", temp);
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->width_units_entry), vals);
  gtk_entry_set_text (GTK_ENTRY (vals->width_units_entry), buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->width_units_entry), vals);

}
static void
file_new_height_update_callback (GtkWidget *widget,
				gpointer data)
{

  /* uh. like do something here  */
  gchar *newvalue;
  NewImageValues *vals;
  int new_height;
  float temp;
  char buffer[12];
  
  vals = data;
  
  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->height_entry));
  new_height = atoi(newvalue);

  temp = ((float) new_height / (float) vals->resolution) * vals->unit;
  sprintf (buffer, "%.2f", temp);
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->height_units_entry), vals);
  gtk_entry_set_text (GTK_ENTRY (vals->height_units_entry), buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->height_units_entry), vals);

}

static void
file_new_width_units_update_callback (GtkWidget *widget,
				      gpointer data)
{

  gchar *newvalue;
  NewImageValues *vals;
  float new_width_units;
  float temp;
  char buffer[12];

  vals = data;
  
  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->width_units_entry));
  new_width_units = atof(newvalue);

  temp = ((((float) new_width_units) * vals->unit) * ((float) vals->resolution));
  sprintf (buffer,  "%d", (int)temp);
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->width_entry), vals);
  gtk_entry_set_text (GTK_ENTRY (vals->width_entry), buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->width_entry), vals);

}

static void
file_new_height_units_update_callback (GtkWidget *widget,
				      gpointer data)
{
  gchar *newvalue;
  NewImageValues *vals;
  float new_height_units;
  float temp;
  char buffer[12];

  vals = data;
  
  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->height_units_entry));
  new_height_units = atof(newvalue);

  temp = ((((float) new_height_units) * vals->unit ) * ((float) vals->resolution));
  sprintf (buffer,  "%d", (int)temp);
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->height_entry), vals);
  gtk_entry_set_text (GTK_ENTRY (vals->height_entry), buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->height_entry), vals);
}

static void
file_new_resolution_callback (GtkWidget *widget,
			      gpointer data)
{

  gchar *newvalue;
  NewImageValues *vals;
  float temp_units;
  float temp_pixels;
  char buffer[12];
  
  vals = data;

  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->resolution_entry));
  vals->resolution = atof(newvalue);

  /* a bit of a kludge to keep height/width from going to zero */
  if(vals->resolution <= 1)
    vals->resolution = 1;


  /* figure the new height */
  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->height_units_entry));
  temp_units = atof(newvalue); 

  temp_pixels  = (float) vals->resolution * ((float)temp_units / vals->unit) ;
  sprintf (buffer, "%d", (int) temp_pixels);
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->height_entry), vals);
  gtk_entry_set_text (GTK_ENTRY (vals->height_entry), buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->height_entry), vals);

  /* figure the new width */
  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->width_units_entry));
  temp_units = atof(newvalue);
  temp_pixels = (float) vals->resolution * ((float) temp_units / vals->unit);
  sprintf(buffer, "%d", (int) temp_pixels);
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->width_entry), vals);
  gtk_entry_set_text (GTK_ENTRY (vals->width_entry), buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->width_entry), vals);

}

static void
file_new_units_inch_menu_callback (GtkWidget *widget ,
				   gpointer data)
{
  NewImageValues *vals;

  float temp_pixels_height;
  float temp_pixels_width;
  float temp_resolution;
  float temp_units_height;
  float temp_units_width;
  gchar *newvalue;
  char buffer[12];

  vals = data;
  vals->unit = 1;      /* set fo/inch ratio for conversions */

  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->height_entry));
  temp_pixels_height = atoi(newvalue);

  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->width_entry));
  temp_pixels_width = atoi(newvalue);

  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->resolution_entry));
  temp_resolution = atof(newvalue);

  /* remember resoltuion is always in dpi */
  
  temp_units_height = (float) temp_pixels_height / (((float) temp_resolution) * vals->unit);
  temp_units_width = (float)temp_pixels_width / (((float) temp_resolution) * vals->unit);

  sprintf (buffer, "%.2f", temp_units_height);
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->height_units_entry),vals);
  gtk_entry_set_text (GTK_ENTRY (vals->height_units_entry), buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->height_units_entry),vals);

  sprintf(buffer, "%.2f", temp_units_width);
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->width_units_entry),vals);
  gtk_entry_set_text (GTK_ENTRY (vals->width_units_entry), buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->width_units_entry),vals);


}

static void
file_new_units_cm_menu_callback (GtkWidget *widget ,
				   gpointer data)
{
  NewImageValues *vals;

  float temp_pixels_height;
  float temp_pixels_width;
  float temp_resolution;
  float temp_units_height;
  float temp_units_width;
  gchar *newvalue;
  char buffer[12];

  vals = data;
  vals->unit = 2.54;
  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->height_entry));
  temp_pixels_height = atoi(newvalue);

  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->width_entry));
  temp_pixels_width = atoi(newvalue);

  newvalue = gtk_entry_get_text (GTK_ENTRY(vals->resolution_entry));
  temp_resolution = atof(newvalue);

  /* remember resoltuion is always in dpi */
  /* convert from inches to centimeters here */

  temp_units_height = ((float) temp_pixels_height / (((float) temp_resolution)) * vals->unit);
  temp_units_width = ((float)temp_pixels_width / (((float) temp_resolution)) * vals->unit);

  sprintf (buffer, "%.2f", temp_units_height);
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->height_units_entry),vals);
  gtk_entry_set_text (GTK_ENTRY (vals->height_units_entry), buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->height_units_entry),vals);

  sprintf(buffer, "%.2f", temp_units_width);
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->width_units_entry),vals);
  gtk_entry_set_text (GTK_ENTRY (vals->width_units_entry), buffer);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->width_units_entry),vals);



}
  
void
file_new_cmd_callback (GtkWidget           *widget,
		       gpointer             callback_data,
		       guint                callback_action)
{
  GDisplay *gdisp;
  NewImageValues *vals;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *radio_box;
  GtkWidget *menu;
  GtkWidget *menuitem;
  GtkWidget *optionmenu;
  GSList *group;
  char buffer[32];
  float temp;



  if(!new_dialog_run)
    {
      last_width = default_width;
      last_height = default_height;
      last_type = default_type;
      last_resolution = default_resolution;  /* this isnt set in gimprc yet */
      last_unit = default_unit;  /* not in gimprc either, inches for now */
      new_dialog_run = 1;  
    }

  /*  Before we try to determine the responsible gdisplay,
   *  make sure this wasn't called from the toolbox
   */
  if (callback_action)
    gdisp = gdisplay_active ();
  else
    gdisp = NULL;

  vals = g_malloc (sizeof (NewImageValues));
  vals->fill_type = last_fill_type;

  if (gdisp)
    {
      vals->width = gdisp->gimage->width;
      vals->height = gdisp->gimage->height;
      vals->type = gimage_base_type (gdisp->gimage);
      /* this is wrong, but until resolution and unit is stored in the image... */
      vals->resolution = last_resolution;
      vals->unit = last_unit;
    }
  else
    {
      vals->width = last_width;
      vals->height = last_height;
      vals->type = last_type;
      vals->resolution = last_resolution;
      vals->unit = last_unit;
    }

  if (vals->type == INDEXED)
    vals->type = RGB;    /* no indexed images */

  vals->dlg = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (vals->dlg), "new_image", "Gimp");
  gtk_window_set_title (GTK_WINDOW (vals->dlg), "New Image");
  gtk_window_position (GTK_WINDOW (vals->dlg), GTK_WIN_POS_MOUSE);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (vals->dlg), "delete_event",
		      GTK_SIGNAL_FUNC (file_new_delete_callback),
		      vals);

  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (vals->dlg)->action_area), 2);

  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) file_new_ok_callback,
                      vals);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) file_new_cancel_callback,
                      vals);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);


  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->vbox),
		      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  /* label for top of table, Width  */
  label = gtk_label_new ("Width");
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Label for top of table, Height */
  label = gtk_label_new ("Height");
   gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* width in pixels entry  */
  vals->width_entry = gtk_entry_new ();
  gtk_widget_set_usize (vals->width_entry, 75, 0);
  sprintf (buffer, "%d", vals->width);
  gtk_entry_set_text (GTK_ENTRY (vals->width_entry), buffer);
  gtk_table_attach (GTK_TABLE (table), vals->width_entry, 0, 1, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (vals->width_entry), "changed",
		      (GtkSignalFunc) file_new_width_update_callback, vals);
  gtk_widget_show (vals->width_entry);

  /* height in pixels entry */
  vals->height_entry = gtk_entry_new ();
  gtk_widget_set_usize (vals->height_entry, 75, 0);
  sprintf (buffer, "%d", vals->height);
  gtk_entry_set_text (GTK_ENTRY (vals->height_entry), buffer);
  gtk_table_attach (GTK_TABLE (table), vals->height_entry, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (vals->height_entry), "changed",
		      (GtkSignalFunc) file_new_height_update_callback, vals);
  gtk_widget_show (vals->height_entry);

  /* width in units entry */
  vals->width_units_entry = gtk_entry_new ();
  gtk_widget_set_usize (vals->width_units_entry, 75, 0);
  temp = (float) vals->width / vals->resolution;
  sprintf (buffer, "%.2f", temp);
  gtk_entry_set_text (GTK_ENTRY (vals->width_units_entry), buffer);
  gtk_signal_connect (GTK_OBJECT (vals->width_units_entry), "changed",
		      (GtkSignalFunc) file_new_width_units_update_callback, vals);
  gtk_table_attach (GTK_TABLE (table), vals->width_units_entry , 0, 1, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (vals->width_units_entry);

  /* height in units entry */
  vals->height_units_entry = gtk_entry_new ();
  gtk_widget_set_usize (vals->height_units_entry, 75, 0);
  temp = (float) vals->height / vals->resolution; 
  sprintf (buffer, "%.2f", temp);
  gtk_entry_set_text (GTK_ENTRY (vals->height_units_entry), buffer);
  gtk_signal_connect (GTK_OBJECT (vals->height_units_entry), "changed",
		      (GtkSignalFunc) file_new_height_units_update_callback, vals);
  gtk_table_attach (GTK_TABLE (table), vals->height_units_entry , 1, 2, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (vals->height_units_entry);

  /* Label for right hand side of pixel size boxes */
  label = gtk_label_new ("Pixels");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label , 2, 3, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* menu and menu items for the units pop-up menu for the units entries */
  menu = gtk_menu_new();
  menuitem = gtk_menu_item_new_with_label ("Inch");
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
      (GtkSignalFunc) file_new_units_inch_menu_callback, vals); 
  gtk_widget_show(menuitem);
  menuitem = gtk_menu_item_new_with_label ("Cm");
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
      (GtkSignalFunc) file_new_units_cm_menu_callback, vals); 
  gtk_widget_show(menuitem);

  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
  gtk_table_attach (GTK_TABLE (table), optionmenu , 2, 3, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(optionmenu);
  /*  man this is lame, the unit stuff really needs to be in a struct */
  printf(" vals unit is: %.2f\n", vals->unit);

  /* resolution frame */
  frame = gtk_frame_new ("Resolution");
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* hbox containing the label, the entry, and the optionmenu */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add(GTK_CONTAINER (frame), hbox);
  gtk_widget_show(hbox);

  /* resoltuion entry   */
  vals->resolution_entry = gtk_entry_new ();
  gtk_widget_set_usize (vals->resolution_entry, 76, 0);
  sprintf(buffer, "%.2f", vals->resolution);
  gtk_entry_set_text (GTK_ENTRY (vals->resolution_entry), buffer);
  gtk_box_pack_start (GTK_BOX (hbox), vals->resolution_entry , TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (vals->resolution_entry), "changed",
		      (GtkSignalFunc) file_new_resolution_callback,
		      vals);
  gtk_widget_show (vals->resolution_entry);
 
  /* resoltuion lable */
  label =gtk_label_new (" pixels per  ");
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  menu = gtk_menu_new();
  /* This units stuff doesnt do anything yet. I'm not real sure if it
     it should do anything yet, excpet for maybe set some default resolutions
     and change the image default rulers. But the rulers stuff probabaly
     needs some gtk acking first to really be useful.
  */

  /* probabaly should be more general here */
  menuitem = gtk_menu_item_new_with_label ("Inch");
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show(menuitem);
  menuitem = gtk_menu_item_new_with_label ("Cm");
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show(menuitem);

  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
  gtk_box_pack_start (GTK_BOX (hbox), optionmenu, TRUE, TRUE, 0);
  gtk_widget_show(optionmenu);


  /* hbox containing thje Image ype and fill type frames */
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);

  /* frame for Image Type */
  frame = gtk_frame_new ("Image Type");
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* radio buttons and box */
  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  button = gtk_radio_button_new_with_label (NULL, "RGB");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) RGB);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->type);
  if (vals->type == RGB)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, "Grayscale");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) GRAY);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->type);
  if (vals->type == GRAY)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);


  /* frame for fill type */
  frame = gtk_frame_new ("Fill Type");
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  button = gtk_radio_button_new_with_label (NULL, "Background");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) BACKGROUND_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == BACKGROUND_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, "White");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) WHITE_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == WHITE_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, "Transparent");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) TRANSPARENT_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == TRANSPARENT_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  gtk_widget_show (vals->dlg);
}

