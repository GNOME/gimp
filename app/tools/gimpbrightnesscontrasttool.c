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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/gimplut.h"
#include "base/lut-funcs.h"

#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "gimpbrightnesscontrasttool.h"
#include "tool_manager.h"

#include "app_procs.h"
#include "image_map.h"

#include "libgimp/gimpintl.h"


#define SLIDER_WIDTH 200

#define BRIGHTNESS  0x1
#define CONTRAST    0x2
#define ALL        (BRIGHTNESS | CONTRAST)


/*  the brightness-contrast structures  */

typedef struct _BrightnessContrastDialog BrightnessContrastDialog;

struct _BrightnessContrastDialog
{
  GtkWidget     *shell;
  GtkWidget     *gimage_name;

  GtkAdjustment *brightness_data;
  GtkAdjustment *contrast_data;

  GimpDrawable  *drawable;
  ImageMap      *image_map;

  gdouble        brightness;
  gdouble        contrast;

  gboolean       preview;

  GimpLut       *lut;
};


/*  local function prototypes  */

static void   gimp_brightness_contrast_tool_class_init (GimpBrightnessContrastToolClass *klass);
static void   gimp_brightness_contrast_tool_init       (GimpBrightnessContrastTool      *bc_tool);

static void   gimp_brightness_contrast_tool_initialize (GimpTool       *tool,
							GimpDisplay    *gdisp);
static void   gimp_brightness_contrast_tool_control    (GimpTool       *tool,
							GimpToolAction  action,
							GimpDisplay    *gdisp);

static BrightnessContrastDialog * brightness_contrast_dialog_new (void);

static void   brightness_contrast_dialog_hide     (void);
static void   brightness_contrast_update          (BrightnessContrastDialog *bcd,
						   gint                      update);
static void   brightness_contrast_preview         (BrightnessContrastDialog *bcd);
static void   brightness_contrast_reset_callback  (GtkWidget *widget,
						   gpointer   data);
static void   brightness_contrast_ok_callback     (GtkWidget *widget,
						   gpointer   data);
static void   brightness_contrast_cancel_callback (GtkWidget *widget,
						   gpointer   data);
static void   brightness_contrast_preview_update  (GtkWidget *widget,
						   gpointer   data);
static void   brightness_contrast_brightness_adjustment_update (GtkAdjustment *adj,
								gpointer       data);
static void   brightness_contrast_contrast_adjustment_update   (GtkAdjustment *adj,
								gpointer      data);


static BrightnessContrastDialog *brightness_contrast_dialog = NULL;

static GimpImageMapToolClass *parent_class = NULL;


/*  functions  */

void
gimp_brightness_contrast_tool_register (GimpToolRegisterCallback  callback,
                                        gpointer                  data)
{
  (* callback) (GIMP_TYPE_BRIGHTNESS_CONTRAST_TOOL,
                NULL,
                FALSE,
                "gimp-brightness-contrast-tool",
                _("Brightness-Contrast"),
                _("Adjust brightness and contrast"),
                N_("/Layer/Colors/Brightness-Contrast..."), NULL,
                NULL, "tools/brightness_contrast.html",
                GIMP_STOCK_TOOL_BRIGHTNESS_CONTRAST,
                data);
}

GType
gimp_brightness_contrast_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpBrightnessContrastToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_brightness_contrast_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpBrightnessContrastTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_brightness_contrast_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_IMAGE_MAP_TOOL,
					  "GimpBrightnessContrastTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_brightness_contrast_tool_class_init (GimpBrightnessContrastToolClass *klass)
{
  GimpToolClass *tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->initialize = gimp_brightness_contrast_tool_initialize;
  tool_class->control    = gimp_brightness_contrast_tool_control;
}

static void
gimp_brightness_contrast_tool_init (GimpBrightnessContrastTool *bc_tool)
{
}

static void
gimp_brightness_contrast_tool_initialize (GimpTool    *tool,
					  GimpDisplay *gdisp)
{
  if (! gdisp)
    {
      brightness_contrast_dialog_hide ();
      return;
    }

  if (gimp_drawable_is_indexed (gimp_image_active_drawable (gdisp->gimage)))
    {
      g_message (_("Brightness-Contrast does not operate on indexed drawables."));
      return;
    }

  /*  The brightness-contrast dialog  */
  if (!brightness_contrast_dialog)
    brightness_contrast_dialog = brightness_contrast_dialog_new ();
  else
    if (!GTK_WIDGET_VISIBLE (brightness_contrast_dialog->shell))
      gtk_widget_show (brightness_contrast_dialog->shell);

  brightness_contrast_dialog->brightness = 0.0;
  brightness_contrast_dialog->contrast   = 0.0;

  brightness_contrast_dialog->drawable =
    gimp_image_active_drawable (gdisp->gimage);
  brightness_contrast_dialog->image_map =
    image_map_create (gdisp, brightness_contrast_dialog->drawable);

  brightness_contrast_update (brightness_contrast_dialog, ALL);
}

static void
gimp_brightness_contrast_tool_control (GimpTool       *tool,
				       GimpToolAction  action,
				       GimpDisplay    *gdisp)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      brightness_contrast_dialog_hide ();
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

/********************************/
/*  Brightness Contrast dialog  */
/********************************/

static BrightnessContrastDialog *
brightness_contrast_dialog_new (void)
{
  BrightnessContrastDialog *bcd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *abox;
  GtkWidget *spinbutton;
  GtkWidget *slider;
  GtkWidget *toggle;
  GtkObject *data;

  bcd = g_new (BrightnessContrastDialog, 1);
  bcd->preview = TRUE;

  bcd->lut = gimp_lut_new ();

  /*  The shell and main vbox  */
  bcd->shell =
    gimp_dialog_new (_("Brightness-Contrast"), "brightness_contrast",
		     tool_manager_help_func, NULL,
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     GTK_STOCK_CANCEL, brightness_contrast_cancel_callback,
		     bcd, NULL, NULL, FALSE, TRUE,

		     GIMP_STOCK_RESET, brightness_contrast_reset_callback,
		     bcd, NULL, NULL, TRUE, FALSE,

		     GTK_STOCK_OK, brightness_contrast_ok_callback,
		     bcd, NULL, NULL, TRUE, FALSE,

		     NULL);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (bcd->shell)->vbox), vbox);

  /*  The table containing sliders  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  Create the brightness scale widget  */
  label = gtk_label_new (_("Brightness:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  data = gtk_adjustment_new (0, -127, 127.0, 1.0, 10.0, 0.0);
  bcd->brightness_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_size_request (slider, SLIDER_WIDTH, -1);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach_defaults (GTK_TABLE (table), slider, 1, 2, 0, 1);

  abox = gtk_vbox_new (FALSE, 0);
  spinbutton = gtk_spin_button_new (bcd->brightness_data, 1.0, 0);
  gtk_widget_set_size_request (spinbutton, 75, -1);
  gtk_box_pack_end (GTK_BOX (abox), spinbutton, FALSE, FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), abox, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  g_signal_connect (G_OBJECT (data), "value_changed",
                    G_CALLBACK (brightness_contrast_brightness_adjustment_update),
                    bcd);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (spinbutton);
  gtk_widget_show (abox);

  /*  Create the contrast scale widget  */
  label = gtk_label_new (_("Contrast:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  data = gtk_adjustment_new (0, -127.0, 127.0, 1.0, 10.0, 0.0);
  bcd->contrast_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_widget_set_size_request (slider, SLIDER_WIDTH, -1);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach_defaults (GTK_TABLE (table), slider, 1, 2, 1, 2);

  abox = gtk_vbox_new (FALSE, 0);
  spinbutton = gtk_spin_button_new (bcd->contrast_data, 1.0, 0);
  gtk_widget_set_size_request (spinbutton, 75, -1);
  gtk_box_pack_end (GTK_BOX (abox), spinbutton, FALSE, FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), abox, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  g_signal_connect (G_OBJECT (data), "value_changed",
                    G_CALLBACK (brightness_contrast_contrast_adjustment_update),
                    bcd);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (spinbutton);
  gtk_widget_show (abox);

  /*  Horizontal box for preview toggle button  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bcd->preview);
  gtk_box_pack_end (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (brightness_contrast_preview_update),
                    bcd);

  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (bcd->shell);

  return bcd;
}

static void
brightness_contrast_dialog_hide (void)
{
  if (brightness_contrast_dialog)
    brightness_contrast_cancel_callback (NULL,
	                                 (gpointer) brightness_contrast_dialog);
}

static void
brightness_contrast_update (BrightnessContrastDialog *bcd,
			    gint                      update)
{
  if (update & BRIGHTNESS)
    {
      gtk_adjustment_set_value (bcd->brightness_data, bcd->brightness);
    }
  if (update & CONTRAST)
    {
      gtk_adjustment_set_value (bcd->contrast_data, bcd->contrast);
    }
}

static void
brightness_contrast_preview (BrightnessContrastDialog *bcd)
{
  if (!bcd->image_map)
    {
      g_message ("brightness_contrast_preview(): No image map");
      return;
    }

  gimp_tool_control_set_preserve (tool_manager_get_active (the_gimp)->control, TRUE);

  brightness_contrast_lut_setup (bcd->lut, bcd->brightness / 255.0,
				 bcd->contrast / 127.0,
				 gimp_drawable_bytes (bcd->drawable));
  image_map_apply (bcd->image_map, (ImageMapApplyFunc) gimp_lut_process_2,
		   (void *) bcd->lut);

  gimp_tool_control_set_preserve (tool_manager_get_active (the_gimp)->control, FALSE);
}

static void
brightness_contrast_reset_callback (GtkWidget *widget,
				    gpointer   data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) data;

  bcd->brightness = 0.0;
  bcd->contrast   = 0.0;

  brightness_contrast_update (bcd, ALL);

  if (bcd->preview)
    brightness_contrast_preview (bcd);
}

static void
brightness_contrast_ok_callback (GtkWidget *widget,
				 gpointer   data)
{
  BrightnessContrastDialog *bcd;
  GimpTool                 *active_tool;

  bcd = (BrightnessContrastDialog *) data;

  gtk_widget_hide (bcd->shell);

  active_tool = tool_manager_get_active (the_gimp);

  gimp_tool_control_set_preserve (active_tool->control, TRUE);

  if (!bcd->preview)
    {
      brightness_contrast_lut_setup (bcd->lut, bcd->brightness / 255.0,
				     bcd->contrast / 127.0,
				     gimp_drawable_bytes (bcd->drawable));
      image_map_apply (bcd->image_map, (ImageMapApplyFunc) gimp_lut_process_2,
		       (void *) bcd->lut);
    }

  if (bcd->image_map)
    image_map_commit (bcd->image_map);

  gimp_tool_control_set_preserve (active_tool->control, FALSE);

  bcd->image_map = NULL;

  active_tool->gdisp    = NULL;
  active_tool->drawable = NULL;
}

static void
brightness_contrast_cancel_callback (GtkWidget *widget,
				     gpointer   data)
{
  BrightnessContrastDialog *bcd;
  GimpTool                 *active_tool;

  bcd = (BrightnessContrastDialog *) data;

  gtk_widget_hide (bcd->shell);

  active_tool = tool_manager_get_active (the_gimp);

  if (bcd->image_map)
    {
      gimp_tool_control_set_preserve (active_tool->control, TRUE);
      image_map_abort (bcd->image_map);
      gimp_tool_control_set_preserve (active_tool->control, FALSE);

      bcd->image_map = NULL;
      gdisplays_flush ();
    }

  active_tool->gdisp    = NULL;
  active_tool->drawable = NULL;
}

static void
brightness_contrast_preview_update (GtkWidget *widget,
				    gpointer   data)
{
  BrightnessContrastDialog *bcd;
  GimpTool                 *active_tool;

  bcd = (BrightnessContrastDialog *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      bcd->preview = TRUE;
      brightness_contrast_preview (bcd);
    }
  else
    {
      bcd->preview = FALSE;
      if (bcd->image_map)
	{
	  active_tool = tool_manager_get_active (the_gimp);

	  gimp_tool_control_set_preserve (active_tool->control, TRUE);
	  image_map_clear (bcd->image_map);
	  gimp_tool_control_set_preserve (active_tool->control, FALSE);
	  gdisplays_flush ();
	}
    }
}

static void
brightness_contrast_brightness_adjustment_update (GtkAdjustment *adjustment,
						  gpointer       data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) data;

  if (bcd->brightness != adjustment->value)
    {
      bcd->brightness = adjustment->value;

      if (bcd->preview)
	brightness_contrast_preview (bcd);
    }
}

static void
brightness_contrast_contrast_adjustment_update (GtkAdjustment *adjustment,
						gpointer       data)
{
  BrightnessContrastDialog *bcd;

  bcd = (BrightnessContrastDialog *) data;

  if (bcd->contrast != adjustment->value)
    {
      bcd->contrast = adjustment->value;

      if (bcd->preview)
	brightness_contrast_preview (bcd);
    }
}
