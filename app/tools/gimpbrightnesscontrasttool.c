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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "actionarea.h"
#include "brightness_contrast.h"
#include "drawable.h"
#include "general.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "image_map.h"
#include "interface.h"
#include "gimplut.h"
#include "lut_funcs.h"

#include "libgimp/gimpintl.h"

#define TEXT_WIDTH 45
#define TEXT_HEIGHT 25
#define SLIDER_WIDTH 200
#define SLIDER_HEIGHT 35

#define BRIGHTNESS_SLIDER 0x1
#define CONTRAST_SLIDER   0x2
#define BRIGHTNESS_TEXT   0x4
#define CONTRAST_TEXT     0x8
#define ALL               0xF

/*  the brightness-contrast structures  */

typedef struct _BrightnessContrast BrightnessContrast;
struct _BrightnessContrast
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _BrightnessContrastDialog BrightnessContrastDialog;
struct _BrightnessContrastDialog
{
  GtkWidget   *shell;
  GtkWidget   *gimage_name;
  GtkWidget   *brightness_text;
  GtkWidget   *contrast_text;
  GtkAdjustment  *brightness_data;
  GtkAdjustment  *contrast_data;

  GimpDrawable *drawable;
  ImageMap     image_map;

  double       brightness;
  double       contrast;

  gint         preview;

  GimpLut      *lut;
};


/*  the brightness-contrast tool options  */
static ToolOptions *brightness_contrast_options = NULL;

/*  the brightness-contrast dialog  */
static BrightnessContrastDialog *brightness_contrast_dialog = NULL;


/*  brightness contrast action functions  */

static void   brightness_contrast_button_press   (Tool *, GdkEventButton *, gpointer);
static void   brightness_contrast_button_release (Tool *, GdkEventButton *, gpointer);
static void   brightness_contrast_motion         (Tool *, GdkEventMotion *, gpointer);
static void   brightness_contrast_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   brightness_contrast_control        (Tool *, int, gpointer);

static BrightnessContrastDialog *  brightness_contrast_new_dialog  (void);
static void   brightness_contrast_update                  (BrightnessContrastDialog *, int);
static void   brightness_contrast_preview                 (BrightnessContrastDialog *);
static void   brightness_contrast_ok_callback             (GtkWidget *, gpointer);
static void   brightness_contrast_cancel_callback         (GtkWidget *, gpointer);
static gint   brightness_contrast_delete_callback         (GtkWidget *, GdkEvent *, gpointer);
static void   brightness_contrast_preview_update          (GtkWidget *, gpointer);
static void   brightness_contrast_brightness_scale_update (GtkAdjustment *, gpointer);
static void   brightness_contrast_contrast_scale_update   (GtkAdjustment *, gpointer);
static void   brightness_contrast_brightness_text_update  (GtkWidget *, gpointer);
static void   brightness_contrast_contrast_text_update    (GtkWidget *, gpointer);


/*  by_color select action functions  */

static void
brightness_contrast_button_press (Tool           *tool,
				  GdkEventButton *bevent,
				  gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = gdisp_ptr;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
}

static void
brightness_contrast_button_release (Tool           *tool,
				    GdkEventButton *bevent,
				    gpointer        gdisp_ptr)
{
}

static void
brightness_contrast_motion (Tool           *tool,
			    GdkEventMotion *mevent,
			    gpointer        gdisp_ptr)
{
}

static void
brightness_contrast_cursor_update (Tool           *tool,
				   GdkEventMotion *mevent,
				   gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
brightness_contrast_control (Tool     *tool,
			     int       action,
			     gpointer  gdisp_ptr)
{
  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (brightness_contrast_dialog)
	{
	  active_tool->preserve = TRUE;
	  image_map_abort (brightness_contrast_dialog->image_map);
	  active_tool->preserve = TRUE;
	  brightness_contrast_dialog->image_map = NULL;
	  brightness_contrast_cancel_callback (NULL, (gpointer) brightness_contrast_dialog);
	}
      break;
    }
}

Tool *
tools_new_brightness_contrast ()
{
  Tool * tool;
  BrightnessContrast * private;

  /*  The tool options  */
  if (!brightness_contrast_options)
    {
      brightness_contrast_options =
	tool_options_new (_("Brightness-Contrast Options"));
      tools_register (BRIGHTNESS_CONTRAST, brightness_contrast_options);
   }

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (BrightnessContrast *) g_malloc (sizeof (BrightnessContrast));

  tool->type = BRIGHTNESS_CONTRAST;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;

  tool->button_press_func = brightness_contrast_button_press;
  tool->button_release_func = brightness_contrast_button_release;
  tool->motion_func = brightness_contrast_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = brightness_contrast_cursor_update;
  tool->control_func = brightness_contrast_control;
  tool->preserve = FALSE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;

  return tool;
}

void
tools_free_brightness_contrast (Tool *tool)
{
  BrightnessContrast * bc;

  bc = (BrightnessContrast *) tool->private;

  /*  Close the color select dialog  */
  if (brightness_contrast_dialog)
    brightness_contrast_cancel_callback (NULL, (gpointer) brightness_contrast_dialog);

  g_free (bc);
}

void
brightness_contrast_initialize (GDisplay *gdisp)
{
  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message (_("Brightness-Contrast does not operate on indexed drawables."));
      return;
    }

  /*  The brightness-contrast dialog  */
  if (!brightness_contrast_dialog)
    brightness_contrast_dialog = brightness_contrast_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (brightness_contrast_dialog->shell))
      gtk_widget_show (brightness_contrast_dialog->shell);

  /*  Initialize dialog fields  */
  brightness_contrast_dialog->image_map = NULL;
  brightness_contrast_dialog->brightness = 0.0;
  brightness_contrast_dialog->contrast = 0.0;

  brightness_contrast_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  brightness_contrast_dialog->image_map = image_map_create (gdisp,
							    brightness_contrast_dialog->drawable);

  brightness_contrast_update (brightness_contrast_dialog, ALL);
}


/********************************/
/*  Brightness Contrast dialog  */
/********************************/

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { N_("OK"), brightness_contrast_ok_callback, NULL, NULL },
  { N_("Cancel"), brightness_contrast_cancel_callback, NULL, NULL }
};

static BrightnessContrastDialog *
brightness_contrast_new_dialog ()
{
  BrightnessContrastDialog *bcd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *toggle;
  GtkObject *data;

  bcd = g_malloc (sizeof (BrightnessContrastDialog));
  bcd->preview = TRUE;

  bcd->lut = gimp_lut_new();

  /*  The shell and main vbox  */
  bcd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (bcd->shell), "brightness_contrast", "Gimp");
  gtk_window_set_title (GTK_WINDOW (bcd->shell), _("Brightness-Contrast"));
  
  /* handle wm close signal */
  gtk_signal_connect (GTK_OBJECT (bcd->shell), "delete_event",
		      GTK_SIGNAL_FUNC (brightness_contrast_delete_callback),
		      bcd);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bcd->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  The table containing sliders  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  Create the brightness scale widget  */
  label = gtk_label_new (_("Brightness"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (0, -127, 127.0, 1.0, 1.0, 0.0);
  bcd->brightness_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_usize (slider, SLIDER_WIDTH, SLIDER_HEIGHT);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) brightness_contrast_brightness_scale_update,
		      bcd);

  bcd->brightness_text = gtk_entry_new ();
  gtk_widget_set_usize (bcd->brightness_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), bcd->brightness_text, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (bcd->brightness_text), "changed",
		      (GtkSignalFunc) brightness_contrast_brightness_text_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (bcd->brightness_text);
  gtk_widget_show (slider);

  /*  Create the contrast scale widget  */
  label = gtk_label_new (_("Contrast"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  data = gtk_adjustment_new (0, -127.0, 127.0, 1.0, 1.0, 0.0);
  bcd->contrast_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_usize (slider, SLIDER_WIDTH, SLIDER_HEIGHT);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) brightness_contrast_contrast_scale_update,
		      bcd);

  bcd->contrast_text = gtk_entry_new ();
  gtk_widget_set_usize (bcd->contrast_text, TEXT_WIDTH, TEXT_HEIGHT);
  gtk_table_attach (GTK_TABLE (table), bcd->contrast_text, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  gtk_signal_connect (GTK_OBJECT (bcd->contrast_text), "changed",
		      (GtkSignalFunc) brightness_contrast_contrast_text_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (bcd->contrast_text);
  gtk_widget_show (slider);


  /*  Horizontal box for preview and preserve luminosity toggle buttons  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bcd->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) brightness_contrast_preview_update,
		      bcd);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);


  /*  The action area  */
  action_items[0].user_data = bcd;
  action_items[1].user_data = bcd;
  build_action_area (GTK_DIALOG (bcd->shell), action_items, 2, 0);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (bcd->shell);

  return bcd;
}

static void
brightness_contrast_update (BrightnessContrastDialog *bcd,
			    int                       update)
{
  char text[12];

  if (update & BRIGHTNESS_SLIDER)
    {
      bcd->brightness_data->value = bcd->brightness;
      gtk_signal_emit_by_name (GTK_OBJECT (bcd->brightness_data), "value_changed");
    }
  if (update & CONTRAST_SLIDER)
    {
      bcd->contrast_data->value = bcd->contrast;
      gtk_signal_emit_by_name (GTK_OBJECT (bcd->contrast_data), "value_changed");
    }
  if (update & BRIGHTNESS_TEXT)
    {
      sprintf (text, "%0.0f", bcd->brightness);
      gtk_entry_set_text (GTK_ENTRY (bcd->brightness_text), text);
    }
  if (update & CONTRAST_TEXT)
    {
      sprintf (text, "%0.0f", bcd->contrast);
      gtk_entry_set_text (GTK_ENTRY (bcd->contrast_text), text);
    }
}

static void
brightness_contrast_preview (BrightnessContrastDialog *bcd)
{
  if (!bcd->image_map)
    g_message (_("brightness_contrast_preview(): No image map"));
  active_tool->preserve = TRUE;
  brightness_contrast_lut_setup(bcd->lut, bcd->brightness / 255.0,
				bcd->contrast / 127.0,
				gimp_drawable_bytes(bcd->drawable));
  image_map_apply (bcd->image_map, (ImageMapApplyFunc)gimp_lut_process_2,
		   (void *) bcd->lut);
  active_tool->preserve = FALSE;
}

static void
brightness_contrast_ok_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (bcd->shell))
    gtk_widget_hide (bcd->shell);

  active_tool->preserve = TRUE;

  if (!bcd->preview)
  {
    brightness_contrast_lut_setup(bcd->lut, bcd->brightness / 255.0,
				  bcd->contrast / 127.0,
				  gimp_drawable_bytes(bcd->drawable));
    image_map_apply (bcd->image_map, (ImageMapApplyFunc)gimp_lut_process_2,
		     (void *) bcd->lut);
  }

  if (bcd->image_map)
    image_map_commit (bcd->image_map);

  active_tool->preserve = FALSE;

  bcd->image_map = NULL;
}

static gint
brightness_contrast_delete_callback (GtkWidget *w,
				     GdkEvent *e,
				     gpointer d)
{
  brightness_contrast_cancel_callback (w, d);

  return TRUE;
}

static void
brightness_contrast_cancel_callback (GtkWidget *widget,
				     gpointer   client_data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (bcd->shell))
    gtk_widget_hide (bcd->shell);

  if (bcd->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (bcd->image_map);
      active_tool->preserve = FALSE;
      gdisplays_flush ();
    }

  bcd->image_map = NULL;
}

static void
brightness_contrast_preview_update (GtkWidget *w,
				    gpointer   data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      bcd->preview = TRUE;
      brightness_contrast_preview (bcd);
    }
  else
    bcd->preview = FALSE;
}

static void
brightness_contrast_brightness_scale_update (GtkAdjustment *adjustment,
					     gpointer       data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) data;

  if (bcd->brightness != adjustment->value)
    {
      bcd->brightness = adjustment->value;
      brightness_contrast_update (bcd, BRIGHTNESS_TEXT);

      if (bcd->preview)
	brightness_contrast_preview (bcd);
    }
}

static void
brightness_contrast_contrast_scale_update (GtkAdjustment *adjustment,
					   gpointer       data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) data;

  if (bcd->contrast != adjustment->value)
    {
      bcd->contrast = adjustment->value;
      brightness_contrast_update (bcd, CONTRAST_TEXT);

      if (bcd->preview)
	brightness_contrast_preview (bcd);
    }
}

static void
brightness_contrast_brightness_text_update (GtkWidget *w,
					    gpointer   data)
{
  BrightnessContrastDialog *bcd;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  bcd = (BrightnessContrastDialog *) data;
  value = BOUNDS (((int) atof (str)), -127, 127);

  if ((int) bcd->brightness != value)
    {
      bcd->brightness = value;
      brightness_contrast_update (bcd, BRIGHTNESS_SLIDER);

      if (bcd->preview)
	brightness_contrast_preview (bcd);
    }
}

static void
brightness_contrast_contrast_text_update (GtkWidget *w,
					  gpointer   data)
{
  BrightnessContrastDialog *bcd;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  bcd = (BrightnessContrastDialog *) data;
  value = BOUNDS (((int) atof (str)), -127, 127);

  if ((int) bcd->contrast != value)
    {
      bcd->contrast = value;
      brightness_contrast_update (bcd, CONTRAST_SLIDER);

      if (bcd->preview)
	brightness_contrast_preview (bcd);
    }
}


