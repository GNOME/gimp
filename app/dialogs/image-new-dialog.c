#include <stdlib.h>
#include <stdio.h>
#include "appenv.h"
#include "actionarea.h"
#include "app_procs.h"
#include "commands.h"
#include "general.h"
#include "gimage.h"
#include "gimprc.h"
#include "global_edit.h"
#include "interface.h"
#include "plug_in.h"
#include "tile_manager_pvt.h"
#include "gdisplay.h"
#include "libgimp/gimpchainbutton.h"
#include "libgimp/gimpsizeentry.h"

#include "libgimp/gimpintl.h"

typedef struct {
  GtkWidget *dlg;

  GtkWidget *size_sizeentry;
  GtkWidget *simple_res;
  GtkWidget *resolution_sizeentry;
  GtkWidget *change_size;
  GtkWidget *couple_resolutions;

  int width;
  int height;
  GUnit unit;

  float xresolution;
  float yresolution;
  GUnit res_unit;

  int type;
  int fill_type;
} NewImageValues;

/*  new image local functions  */
static void file_new_ok_callback (GtkWidget *, gpointer);
static void file_new_cancel_callback (GtkWidget *, gpointer);
static gint file_new_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void file_new_toggle_callback (GtkWidget *, gpointer);
static void file_new_resolution_callback (GtkWidget *, gpointer);
static void file_new_advanced_res_callback (GtkWidget *, gpointer);

/*  static variables  */
static   int          last_width = 256;
static   int          last_height = 256;
static   int          last_type = RGB;
static   int          last_fill_type = BACKGROUND_FILL; 
static   float        last_xresolution = 72;     /* always in DPI */
static   float        last_yresolution = 72;
static   GUnit        last_unit = UNIT_INCH;
static   GUnit        last_res_unit = UNIT_INCH;
static   gboolean     last_new_image = TRUE;

/* these are temps that should be set in gimprc eventually */
/*  FIXME */
static   float        default_xresolution = 72;
static   float        default_yresolution = 72;
static   GUnit        default_unit = UNIT_INCH;

static   GUnit        default_res_unit = UNIT_INCH;


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

  /* get the image size in pixels */
  vals->width =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (vals->size_sizeentry), 0);
  vals->height =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (vals->size_sizeentry), 1);

  /* get the resolution in dpi */
  vals->xresolution =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (vals->resolution_sizeentry), 0);
  vals->yresolution =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (vals->resolution_sizeentry), 1);

  /* get the units */
  vals->unit =
    gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (vals->size_sizeentry));
  vals->res_unit =
    gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (vals->resolution_sizeentry));

  last_new_image = TRUE;
  gtk_widget_destroy (vals->dlg);

  last_width = vals->width;
  last_height = vals->height;
  last_type = vals->type;
  last_fill_type = vals->fill_type;
  last_xresolution = vals->xresolution;
  last_yresolution = vals->yresolution;
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

  gimp_image_set_resolution (gimage, vals->xresolution, vals->yresolution);
  gimp_image_set_unit (gimage, vals->unit);

  /*  Make the background (or first) layer  */
  layer = layer_new (gimage, gimage->width, gimage->height,
		     type, _("Background"), OPAQUE_OPACITY, NORMAL);

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
file_new_advanced_res_callback (GtkWidget *widget,
				gpointer data)
{
  NewImageValues *vals;

  vals = data;
  gtk_widget_hide(widget);
  gtk_widget_hide(vals->simple_res);
  gtk_widget_show(vals->resolution_sizeentry);
  gtk_widget_show (vals->couple_resolutions);
}

static void
file_new_resolution_callback (GtkWidget *widget,
			      gpointer data)
{
  NewImageValues *vals;

  static float xres = 0.0;
  static float yres = 0.0;
  float new_xres;
  float new_yres;

  vals = data;

  new_xres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);

  if (widget == vals->simple_res)
    {
      if (new_xres != xres)
	{
	  xres = new_xres;
	  yres = xres;
	  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->resolution_sizeentry), 1, xres);
	  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->resolution_sizeentry), 0, yres);
	}
    } 
  else 
    {
      new_yres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);
      if (gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (vals->couple_resolutions)))
	{
	  if (new_xres != xres)
	    {
	      yres = new_yres = xres = new_xres;
	      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->resolution_sizeentry), 1, yres);
	    }
	  
	  if (new_yres != yres)
	    {
	      xres = new_xres = yres = new_yres;
	      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->resolution_sizeentry), 0, xres);
	    }
	}
    }
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (vals->size_sizeentry), 0,
				  xres,
				  FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (vals->size_sizeentry), 1,
				  yres,
				  FALSE);
}

void
file_new_cmd_callback (GtkWidget           *widget,
		       gpointer             callback_data,
		       guint                callback_action)
{
  GDisplay *gdisp;
  NewImageValues *vals;
  GtkWidget *button;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *radio_box;
  gboolean   advanced_options = FALSE;
  GtkWidget *advanced_button;
  GSList *group;

  /* this value is from gimprc.h */
  default_res_unit = UNIT_INCH; /* ruler_units; */

  if(!new_dialog_run)
    {
      last_width = default_width;
      last_height = default_height;
      last_type = default_type;
      last_xresolution = default_xresolution;  /* this isnt set in gimprc yet */
      last_yresolution = default_yresolution;  /* this isnt set in gimprc yet */
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
      vals->xresolution = gdisp->gimage->xresolution;
      vals->yresolution = gdisp->gimage->yresolution;
      vals->unit = gdisp->gimage->unit;
      vals->res_unit = UNIT_INCH;
    }
  else
    {
      vals->width = last_width;
      vals->height = last_height;
      vals->type = last_type;
      vals->xresolution = last_xresolution;
      vals->yresolution = last_yresolution;
      vals->unit = last_unit;
      vals->res_unit = last_res_unit;
    }

  if (vals->xresolution != vals->yresolution)
    advanced_options = TRUE;

  if (vals->type == INDEXED)
    vals->type = RGB;    /* no indexed images */

  /* if a cut buffer exist, default to using its size for the new image */
  /* also check to see if a new_image has been opened */

  if(global_buf && !last_new_image)
    {
      vals->width = global_buf->width;
      vals->height = global_buf->height;
      /* It would be good to set the resolution here, but that would
       * require TileManagers to know about the resolution of tiles
       * they're dealing with.  It's not clear we want to go down that
       * road. -- austin */
    }

  vals->dlg = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (vals->dlg), "new_image", "Gimp");
  gtk_window_set_title (GTK_WINDOW (vals->dlg), _("New Image"));
  gtk_window_set_position (GTK_WINDOW (vals->dlg), GTK_WIN_POS_MOUSE);
	gtk_window_set_policy(GTK_WINDOW (vals->dlg), FALSE, FALSE, FALSE);

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (vals->dlg), "delete_event",
		      GTK_SIGNAL_FUNC (file_new_delete_callback),
		      vals);

  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (vals->dlg)->action_area), 2);

  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) file_new_ok_callback,
                      vals);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) file_new_cancel_callback,
                      vals);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /* vbox holding the rest of the dialog */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dlg)->vbox),
		      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  vals->size_sizeentry = gimp_size_entry_new (2, vals->unit, "%p",
					      FALSE, TRUE, 75,
					      GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (vals->size_sizeentry), 0,
				  vals->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (vals->size_sizeentry), 1,
				  vals->yresolution, FALSE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (vals->size_sizeentry),
					 0, 1, 32767);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (vals->size_sizeentry),
					 1, 1, 32767);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->size_sizeentry), 0,
			      vals->width);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->size_sizeentry), 1,
			      vals->height);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (vals->size_sizeentry),
				_("Width"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (vals->size_sizeentry),
				_("Height"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (vals->size_sizeentry),
				_("Pixels"), 1, 4, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (vals->size_sizeentry), 4);
  gtk_box_pack_start (GTK_BOX (vbox), vals->size_sizeentry, TRUE, TRUE, 0);

  gtk_widget_show (vals->size_sizeentry);

  /* resolution frame */
  frame = gtk_frame_new (_("Resolution"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  vals->simple_res = gimp_size_entry_new (1, vals->res_unit, "%s",
					  FALSE, FALSE, 75,
					  GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (vals->simple_res),
					 0, 1, 32767);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->simple_res), 0,
			      MIN(vals->xresolution, vals->yresolution));
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (vals->simple_res),
				_("pixels per "), 1, 2, 0.0);
  gtk_signal_connect (GTK_OBJECT (vals->simple_res), "value_changed",
		      (GtkSignalFunc)file_new_resolution_callback, vals);
  gtk_box_pack_start (GTK_BOX (vbox2), vals->simple_res, TRUE, TRUE, 0);
  if (!advanced_options)
    gtk_widget_show (vals->simple_res);

  /* the advanced resolution stuff
     (not shown by default, but used to keep track of all the variables) */
  vals->resolution_sizeentry = gimp_size_entry_new (2, vals->res_unit, "%s",
						    FALSE, TRUE, 75,
						    GIMP_SIZE_ENTRY_UPDATE_RESOLUTION);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (vals->resolution_sizeentry),
					 0, 1, 32767);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (vals->resolution_sizeentry),
					 1, 1, 32767);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->resolution_sizeentry), 0,
			      vals->xresolution);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (vals->resolution_sizeentry), 1,
			      vals->yresolution);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (vals->resolution_sizeentry),
				_("Horizontal"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (vals->resolution_sizeentry),
				_("Vertical"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (vals->resolution_sizeentry),
				_("dpi"), 1, 3, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (vals->resolution_sizeentry),
				_("pixels per "), 2, 3, 0.0);
  vals->couple_resolutions = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (vals->couple_resolutions), TRUE);
  gtk_table_attach_defaults (GTK_TABLE (vals->resolution_sizeentry), 
			     vals->couple_resolutions, 1, 3, 3, 4);
  gtk_signal_connect (GTK_OBJECT (vals->resolution_sizeentry), "value_changed",
		      (GtkSignalFunc)file_new_resolution_callback, vals);
  gtk_signal_connect (GTK_OBJECT (vals->resolution_sizeentry), "refval_changed",
		      (GtkSignalFunc)file_new_resolution_callback, vals);
  gtk_box_pack_start (GTK_BOX (vbox2), vals->resolution_sizeentry,
		      TRUE, TRUE, 0);
  if (advanced_options)
    {
      gtk_widget_show (vals->resolution_sizeentry);
      gtk_widget_show (vals->couple_resolutions);
    }

  /* This code is commented out since it seems to be overkill...
   * vals->change_size = gtk_check_button_new_with_label (_("Change image size with resolution changes"));
   * gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vals->change_size), TRUE);
   * gtk_box_pack_start (GTK_BOX (vbox2), vals->change_size, TRUE, TRUE, 0);
   * gtk_widget_show (vals->change_size);
  */

  if (advanced_options)
    advanced_button = gtk_button_new_with_label (_("Advanced options <<"));
  else
    advanced_button = gtk_button_new_with_label (_("Advanced options >>"));
  gtk_box_pack_start (GTK_BOX (vbox2), advanced_button,
		      FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (advanced_button), "clicked",
		      (GtkSignalFunc)file_new_advanced_res_callback, vals);
  gtk_widget_show (advanced_button);

  /* hbox containing the Image type and fill type frames */
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);

  /* frame for Image Type */
  frame = gtk_frame_new (_("Image Type"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /* radio buttons and box */
  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  button = gtk_radio_button_new_with_label (NULL, _("RGB"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, FALSE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) RGB);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->type);
  if (vals->type == RGB)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, _("Grayscale"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, FALSE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) GRAY);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->type);
  if (vals->type == GRAY)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);


  /* frame for fill type */
  frame = gtk_frame_new (_("Fill Type"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  button = gtk_radio_button_new_with_label (NULL, _("Foreground"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) FOREGROUND_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == FOREGROUND_FILL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, _("Background"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) BACKGROUND_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == BACKGROUND_FILL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, _("White"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) WHITE_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == WHITE_FILL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, _("Transparent"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (radio_box), button, TRUE, TRUE, 0);
  gtk_object_set_user_data (GTK_OBJECT (button), (gpointer) TRANSPARENT_FILL);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) file_new_toggle_callback,
		      &vals->fill_type);
  if (vals->fill_type == TRANSPARENT_FILL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  gtk_widget_show (vals->dlg);
}

void file_new_reset_current_cut_buffer()
{
  /* this function just changes the status of last_image_new
     so i can if theres been a cut/copy since the last file new */
  last_new_image = FALSE;
}
