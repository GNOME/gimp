#include <stdlib.h>
#include <stdio.h>
#include "appenv.h"
#include "actionarea.h"
#include "app_procs.h"
#include "commands.h"
#include "general.h"
#include "gimage.h"
#include "gimage_cmds.h"
#include "gimprc.h"
#include "global_edit.h"
#include "interface.h"
#include "plug_in.h"
#include "tile_manager_pvt.h"
#include "gdisplay.h"

typedef struct {
  GtkWidget *dlg;
  GtkWidget *height_spinbutton;
  GtkWidget *width_spinbutton;
  GtkWidget *height_units_spinbutton;
  GtkWidget *width_units_spinbutton;
  GtkWidget *resolution_spinbutton;
  float resolution;   /* always in dpi */
  float unit;  /* this is a float that is equal to unit/inch, 2.54 for cm for example */
  float res_unit;  /* same as above but for the res unit */
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
static void file_new_res_units_inch_callback (GtkWidget *, gpointer);
static void file_new_res_units_cm_callback (GtkWidget *, gpointer);
static float file_new_default_unit_parse (int); 

/*  static variables  */
static   int          last_width = 256;
static   int          last_height = 256;
static   int          last_type = RGB;
static   int          last_fill_type = BACKGROUND_FILL; 
static   float        last_resolution = 72;     /* always in DPI */
static   float        last_unit = 1;
static   float        last_res_unit =1;
static   gboolean     last_new_image = TRUE;

/* these are temps that should be set in gimprc eventually */
/*  FIXME */
/* static   float        default_resolution = 72;      this needs to be set in gimprc */
static   float        default_unit = 1;

static float default_res_unit = 1;


static int new_dialog_run;
extern TileManager *global_buf;

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

  vals->width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (vals->width_spinbutton));
  vals->height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (vals->height_spinbutton));
  vals->resolution = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (vals->resolution_spinbutton));


  last_new_image = TRUE;
  gtk_widget_destroy (vals->dlg);

  last_width = vals->width;
  last_height = vals->height;
  last_type = vals->type;
  last_fill_type = vals->fill_type;
  last_resolution = vals->resolution;
  last_unit = vals->unit;
  last_res_unit = vals->res_unit;
  

  switch (vals->fill_type)
    {
    case BACKGROUND_FILL:
    case FOREGROUND_FILL:
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
  layer = layer_new (gimage, gimage->width, gimage->height,
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
  NewImageValues *vals;
  int new_width;
  float temp;
  
  vals = data;
  new_width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (vals->width_spinbutton));
  
  temp = ((float) new_width / (float) vals->resolution) * vals->unit;
 
   gtk_signal_handler_block_by_data (GTK_OBJECT (vals->width_units_spinbutton), vals);
   gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->width_units_spinbutton), temp);
   gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->width_units_spinbutton), vals);

}
static void
file_new_height_update_callback (GtkWidget *widget,
				gpointer data)
{

  NewImageValues *vals;
  int new_height;
  float temp;

  
  vals = data;
  new_height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (vals->height_spinbutton));
  
 
  temp = ((float) new_height / (float) vals->resolution) * vals->unit;
  
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->height_units_spinbutton), vals);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->height_units_spinbutton), temp);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->height_units_spinbutton), vals);
 
}

static void
file_new_width_units_update_callback (GtkWidget *widget,
				      gpointer data)
{
  NewImageValues *vals;
  float new_width_units;
  float temp;

  vals = data;
  new_width_units = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (vals->width_units_spinbutton));

  temp = ((((float) new_width_units) / vals->unit) * ((float) vals->resolution));
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->width_spinbutton), vals);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->width_spinbutton), temp);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->width_spinbutton), vals);
 
}

static void
file_new_height_units_update_callback (GtkWidget *widget,
				      gpointer data)
{
  NewImageValues *vals;
  float new_height_units;
  float temp;

  vals = data;
  
  new_height_units = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (vals->height_units_spinbutton));
 
 
  temp = ((((float) new_height_units) / vals->unit ) * ((float) vals->resolution));
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->height_spinbutton), vals);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->height_spinbutton), temp);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->height_spinbutton), vals);
}

static void
file_new_resolution_callback (GtkWidget *widget,
			      gpointer data)
{
  NewImageValues *vals;
  float temp_units;
  float temp_pixels;
  float temp_resolution;
  
  vals = data;
  temp_resolution = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (vals->resolution_spinbutton));
  

  /* a bit of a kludge to keep height/width from going to zero */
  if(temp_resolution <= 1)
    temp_resolution = 1;

  /* vals->resoluion is always in DPI */
  vals->resolution = (temp_resolution * vals->res_unit);

  /* figure the new height */
   temp_units = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (vals->height_units_spinbutton)); 

  temp_pixels  = (float) vals->resolution * ((float)temp_units / vals->unit) ;
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->height_spinbutton), vals);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->height_spinbutton), temp_pixels);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->height_spinbutton), vals);

  /* figure the new width */


  temp_units = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (vals->width_units_spinbutton));
  temp_pixels = (float) vals->resolution * ((float) temp_units / vals->unit);
 
  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->width_spinbutton), vals);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->width_spinbutton), temp_pixels);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->width_spinbutton), vals);
 

}

static void
file_new_units_inch_menu_callback (GtkWidget *widget ,
				   gpointer data)
{
  NewImageValues *vals;

  float temp_pixels_height;
  float temp_pixels_width;
  float temp_units_height;
  float temp_units_width;

  vals = data;
  vals->unit = 1;      /* set fo/inch ratio for conversions */

  temp_pixels_height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (vals->height_spinbutton));
  temp_pixels_width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (vals->width_spinbutton));

  
  /* remember vals->resoltuion is always in dpi */
  temp_units_height = (float) temp_pixels_height / (((float) vals->resolution) * vals->unit);
  temp_units_width = (float)temp_pixels_width / (((float) vals->resolution) * vals->unit);

  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->height_units_spinbutton),vals);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->height_units_spinbutton), temp_units_height);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->height_units_spinbutton),vals);

  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->width_units_spinbutton),vals);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->width_units_spinbutton), temp_units_width);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->width_units_spinbutton),vals);


}

static void
file_new_units_cm_menu_callback (GtkWidget *widget ,
				   gpointer data)
{
  NewImageValues *vals;

  float temp_pixels_height;
  float temp_pixels_width;
  float temp_units_height;
  float temp_units_width;

  vals = data;
  vals->unit = 2.54;

  temp_pixels_height = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (vals->height_spinbutton));

  temp_pixels_width = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (vals->width_spinbutton));

  /* remember resoltuion is always in dpi */
  /* convert from inches to centimeters here */

  temp_units_height = ((float) temp_pixels_height / (((float) vals->resolution)) * vals->unit);
  temp_units_width = ((float)temp_pixels_width / (((float) vals->resolution)) * vals->unit);

  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->height_units_spinbutton),vals);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->height_units_spinbutton), temp_units_height);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->height_units_spinbutton),vals);

  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->width_units_spinbutton),vals);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->width_units_spinbutton), temp_units_width);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->width_units_spinbutton),vals);

}

static void file_new_res_units_inch_callback (GtkWidget *widget, gpointer data)
{

  NewImageValues *vals;
  float temp_resolution;

  vals = data;
  vals->res_unit = 1.0;
  
  /* vals->resoltuion is alwayd DPI */
  temp_resolution = (vals->resolution / vals->res_unit);

  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->resolution_spinbutton),vals);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->resolution_spinbutton), temp_resolution);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->resolution_spinbutton),vals);
 
}
  
static void file_new_res_units_cm_callback (GtkWidget *widget, gpointer data)
{

  NewImageValues *vals;
  float temp_resolution;

  vals = data;
  vals->res_unit = 2.54;

  /* vals->resolution is always DPI */
  temp_resolution = (vals->resolution / vals->res_unit);

  gtk_signal_handler_block_by_data (GTK_OBJECT (vals->resolution_spinbutton),vals);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (vals->resolution_spinbutton), temp_resolution);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (vals->resolution_spinbutton),vals);

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
  GtkAdjustment *adj;
  GSList *group;
  float temp;

  default_res_unit = file_new_default_unit_parse(default_resolution_units);

  if(!new_dialog_run)
    {
      last_width = default_width;
      last_height = default_height;
      last_type = default_type;
      last_resolution = default_resolution;  /* this isnt set in gimprc yet */
      last_unit = default_unit;  /* not in gimprc either, inches for now */
      last_res_unit = default_res_unit;

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
      vals->res_unit = last_res_unit;
    }
  else
    {
      vals->width = last_width;
      vals->height = last_height;
      vals->type = last_type;
      vals->resolution = last_resolution;
      vals->unit = last_unit;
      vals->res_unit = last_res_unit;
    }

  if (vals->type == INDEXED)
    vals->type = RGB;    /* no indexed images */

  /*  if a cut buffer exist, default to using its size for the new image */
  /* also check to see if a new_image has been opened */

  if(global_buf && !last_new_image)
    {
      vals->width = global_buf->width;
      vals->height = global_buf->height;
    }

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

  /* width in pixels spinbutton  */
  adj = (GtkAdjustment *) gtk_adjustment_new (vals->width, 1.0, 32767.0,
                                              1.0, 50.0, 0.0);
  vals->width_spinbutton = gtk_spin_button_new (adj, 1.0, 0.0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(vals->width_spinbutton), GTK_SHADOW_NONE);
  gtk_widget_set_usize (vals->width_spinbutton, 75, 0);

  gtk_table_attach (GTK_TABLE (table), vals->width_spinbutton, 0, 1, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (vals->width_spinbutton), "changed",
 		      (GtkSignalFunc) file_new_width_update_callback, vals);
  gtk_widget_show (vals->width_spinbutton);
 
  /* height in pixels spinbutton */
  adj = (GtkAdjustment *) gtk_adjustment_new (vals->height, 1.0, 32767.0,
                                              1.0, 50.0, 0.0);
  vals->height_spinbutton = gtk_spin_button_new (adj, 1.0, 0.0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(vals->height_spinbutton), GTK_SHADOW_NONE);
  gtk_widget_set_usize (vals->height_spinbutton, 75, 0);
  gtk_table_attach (GTK_TABLE (table), vals->height_spinbutton, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (vals->height_spinbutton), "changed",
		      (GtkSignalFunc) file_new_height_update_callback, vals);
  gtk_widget_show (vals->height_spinbutton);

  /* width in units spinbutton */
  temp = (float) vals->width / vals->resolution;
  adj = (GtkAdjustment *) gtk_adjustment_new (temp, 1.0, 32767.0,
                                              1.0, 0.01, 0.0);
  vals->width_units_spinbutton = gtk_spin_button_new (adj, 1.0, 2.0);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON(vals->width_units_spinbutton), GTK_UPDATE_ALWAYS
				     | GTK_UPDATE_IF_VALID );
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(vals->width_units_spinbutton), GTK_SHADOW_NONE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(vals->width_units_spinbutton), 0);
  gtk_widget_set_usize (vals->width_units_spinbutton, 75, 0);
  gtk_signal_connect (GTK_OBJECT (vals->width_units_spinbutton), "changed",
		      (GtkSignalFunc) file_new_width_units_update_callback, vals);
  gtk_table_attach (GTK_TABLE (table), vals->width_units_spinbutton , 0, 1, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (vals->width_units_spinbutton);

  /* height in units spinbutton */
  temp = (float) vals->height / vals->resolution; 
  adj = (GtkAdjustment *) gtk_adjustment_new (temp, 1.0, 32767.0,
                                              1.0, 0.01, 0.0);
  vals->height_units_spinbutton = gtk_spin_button_new (adj, 1.0, 2.0);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON(vals->height_units_spinbutton), GTK_UPDATE_ALWAYS |
				     GTK_UPDATE_IF_VALID);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(vals->height_units_spinbutton), GTK_SHADOW_NONE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(vals->height_units_spinbutton), 0);
  gtk_widget_set_usize (vals->height_units_spinbutton, 75, 0);
  gtk_signal_connect (GTK_OBJECT (vals->height_units_spinbutton), "changed",
		      (GtkSignalFunc) file_new_height_units_update_callback, vals);
  gtk_table_attach (GTK_TABLE (table), vals->height_units_spinbutton , 1, 2, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (vals->height_units_spinbutton);

  /* Label for right hand side of pixel size boxes */
  label = gtk_label_new ("Pixels");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label , 2, 3, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* menu and menu items for the units pop-up menu for the units entries */
  menu = gtk_menu_new();
  menuitem = gtk_menu_item_new_with_label ("inches");
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
      (GtkSignalFunc) file_new_units_inch_menu_callback, vals); 
  gtk_widget_show(menuitem);
  menuitem = gtk_menu_item_new_with_label ("cm");
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
      (GtkSignalFunc) file_new_units_cm_menu_callback, vals); 
  gtk_widget_show(menuitem);

  optionmenu = gtk_option_menu_new();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
  gtk_table_attach (GTK_TABLE (table), optionmenu , 2, 3, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show(optionmenu);

  /* resolution frame */
  frame = gtk_frame_new ("Resolution");
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* hbox containing the label, the spinbutton, and the optionmenu */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add(GTK_CONTAINER (frame), hbox);
  gtk_widget_show(hbox);

  /* resoltuion spinbutton  */
  adj = (GtkAdjustment *) gtk_adjustment_new (vals->resolution, 1.0, 32767.0,
                                              1.0, 5.0, 0.0);
  vals->resolution_spinbutton = gtk_spin_button_new (adj, 1.0, 2.0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(vals->resolution_spinbutton), GTK_SHADOW_NONE);
  gtk_widget_set_usize (vals->resolution_spinbutton, 76, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vals->resolution_spinbutton, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (vals->resolution_spinbutton), "changed",
		      (GtkSignalFunc) file_new_resolution_callback,
		      vals);
  gtk_widget_show (vals->resolution_spinbutton);
 
  /* resolution label */
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
  menuitem = gtk_menu_item_new_with_label ("inch");
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
      (GtkSignalFunc) file_new_res_units_inch_callback, vals); 
  gtk_widget_show(menuitem);
  menuitem = gtk_menu_item_new_with_label ("cm");
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
      (GtkSignalFunc) file_new_res_units_cm_callback, vals); 
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

  button = gtk_radio_button_new_with_label (NULL, "Foreground");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) FOREGROUND_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == FOREGROUND_FILL)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, "Background");
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

void file_new_reset_current_cut_buffer()
{
  /* this unction just changes the status of last_image_new
     so i can if theres been a cut/copy since the last file new */
  last_new_image = FALSE;

}

static float file_new_default_unit_parse (int ruler_unit)
{

  /* checks the enum passed from gimprc to see what the values are */

  if(ruler_unit == 1)
    return 1.0;
  if(ruler_unit == 2)
    return 2.54;
  
  return 1.0;

}
