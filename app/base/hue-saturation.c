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

#include "appenv.h"
#include "drawable.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "gimpui.h"
#include "hue_saturation.h"

#include "libgimp/gimpcolorspace.h"
#include "libgimp/gimpmath.h"

#include "libgimp/gimpintl.h"


#define HUE_PARTITION_MASK  GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK

#define SLIDER_WIDTH  200
#define DA_WIDTH  40
#define DA_HEIGHT 20

#define HUE_PARTITION      0x0
#define HUE_SLIDER         0x1
#define LIGHTNESS_SLIDER   0x2
#define SATURATION_SLIDER  0x4
#define DRAW               0x40
#define ALL                0xFF

/*  the hue-saturation structures  */

typedef struct _HueSaturation HueSaturation;

struct _HueSaturation
{
  gint x, y;    /*  coords for last mouse click  */
};

/*  the hue-saturation tool options  */
static ToolOptions *hue_saturation_options = NULL;

/*  the hue-saturation tool dialog  */
static HueSaturationDialog *hue_saturation_dialog = NULL;

/*  Local variables  */
static gint hue_transfer[6][256];
static gint lightness_transfer[6][256];
static gint saturation_transfer[6][256];
static gint default_colors[6][3] =
{
  { 255,   0,   0 },
  { 255, 255,   0 },
  {   0, 255,   0 },
  {   0, 255, 255 },
  {   0,   0, 255 },
  { 255,   0, 255 }
};

/*  hue saturation action functions  */
static void   hue_saturation_control (Tool *, ToolAction, gpointer);

static HueSaturationDialog * hue_saturation_dialog_new (void);

static void   hue_saturation_update                  (HueSaturationDialog *,
						      gint);
static void   hue_saturation_preview                 (HueSaturationDialog *);
static void   hue_saturation_reset_callback          (GtkWidget *, gpointer);
static void   hue_saturation_ok_callback             (GtkWidget *, gpointer);
static void   hue_saturation_cancel_callback         (GtkWidget *, gpointer);
static void   hue_saturation_partition_callback      (GtkWidget *, gpointer);
static void   hue_saturation_preview_update          (GtkWidget *, gpointer);
static void   hue_saturation_hue_adjustment_update        (GtkAdjustment *,
							   gpointer);
static void   hue_saturation_lightness_adjustment_update  (GtkAdjustment *,
							   gpointer);
static void   hue_saturation_saturation_adjustment_update (GtkAdjustment *,
							   gpointer);
static gint   hue_saturation_hue_partition_events    (GtkWidget *, GdkEvent *,
						      HueSaturationDialog *);

/*  hue saturation machinery  */

void
hue_saturation_calculate_transfers (HueSaturationDialog *hsd)
{
  gint value;
  gint hue;
  gint i;

  /*  Calculate transfers  */
  for (hue = 0; hue < 6; hue++)
    for (i = 0; i < 256; i++)
      {
	value = (hsd->hue[0] + hsd->hue[hue + 1]) * 255.0 / 360.0;
	if ((i + value) < 0)
	  hue_transfer[hue][i] = 255 + (i + value);
	else if ((i + value) > 255)
	  hue_transfer[hue][i] = i + value - 255;
	else
	  hue_transfer[hue][i] = i + value;

	/*  Lightness  */
	value = (hsd->lightness[0] + hsd->lightness[hue + 1]) * 127.0 / 100.0;
	value = CLAMP (value, -255, 255);
	if (value < 0)
	  lightness_transfer[hue][i] = (unsigned char) ((i * (255 + value)) / 255);
	else
	  lightness_transfer[hue][i] = (unsigned char) (i + ((255 - i) * value) / 255);

	/*  Saturation  */
	value = (hsd->saturation[0] + hsd->saturation[hue + 1]) * 255.0 / 100.0;
	value = CLAMP (value, -255, 255);

	/* This change affects the way saturation is computed. With the
	   old code (different code for value < 0), increasing the
	   saturation affected muted colors very much, and bright colors
	   less. With the new code, it affects muted colors and bright
	   colors more or less evenly. For enhancing the color in photos,
	   the new behavior is exactly what you want. It's hard for me
	   to imagine a case in which the old behavior is better.
	*/
	saturation_transfer[hue][i] = CLAMP ((i * (255 + value)) / 255, 0, 255);
      }
}

void
hue_saturation (PixelRegion *srcPR,
		PixelRegion *destPR,
		void        *user_data)
{
  HueSaturationDialog *hsd;
  unsigned char *src, *s;
  unsigned char *dest, *d;
  int alpha;
  int w, h;
  int r, g, b;
  int hue;

  hsd = (HueSaturationDialog *) user_data;

  /*  Set the transfer arrays  (for speed)  */
  h = srcPR->h;
  src = srcPR->data;
  dest = destPR->data;
  alpha = (srcPR->bytes == 4) ? TRUE : FALSE;

  while (h--)
    {
      w = srcPR->w;
      s = src;
      d = dest;
      while (w--)
	{
	  r = s[RED_PIX];
	  g = s[GREEN_PIX];
	  b = s[BLUE_PIX];

	  gimp_rgb_to_hls (&r, &g, &b);

	  if (r < 43)
	    hue = 0;
	  else if (r < 85)
	    hue = 1;
	  else if (r < 128)
	    hue = 2;
	  else if (r < 171)
	    hue = 3;
	  else if (r < 213)
	    hue = 4;
	  else
	    hue = 5;

	  r = hue_transfer[hue][r];
	  g = lightness_transfer[hue][g];
	  b = saturation_transfer[hue][b];

	  gimp_hls_to_rgb (&r, &g, &b);

	  d[RED_PIX] = r;
	  d[GREEN_PIX] = g;
	  d[BLUE_PIX] = b;

	  if (alpha)
	    d[ALPHA_PIX] = s[ALPHA_PIX];

	  s += srcPR->bytes;
	  d += destPR->bytes;
	}

      src += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

/*  hue saturation action functions  */

static void
hue_saturation_control (Tool       *tool,
			ToolAction  action,
			gpointer    gdisp_ptr)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      if (hue_saturation_dialog)
	hue_saturation_cancel_callback (NULL, (gpointer) hue_saturation_dialog);
      break;

    default:
      break;
    }
}

Tool *
tools_new_hue_saturation (void)
{
  Tool * tool;
  HueSaturation * private;

  /*  The tool options  */
  if (!hue_saturation_options)
    {
      hue_saturation_options = tool_options_new (_("Hue-Saturation"));
      tools_register (HUE_SATURATION, hue_saturation_options);
    }

  tool = tools_new_tool (HUE_SATURATION);
  private = g_new (HueSaturation, 1);

  tool->scroll_lock = TRUE;   /*  Disallow scrolling  */
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */

  tool->private = (void *) private;

  tool->control_func = hue_saturation_control;

  return tool;
}

void
tools_free_hue_saturation (Tool *tool)
{
  HueSaturation * color_bal;

  color_bal = (HueSaturation *) tool->private;

  /*  Close the hue saturation dialog  */
  if (hue_saturation_dialog)
    hue_saturation_cancel_callback (NULL, (gpointer) hue_saturation_dialog);

  g_free (color_bal);
}

void
hue_saturation_initialize (GDisplay *gdisp)
{
  gint i;

  if (! drawable_color (gimage_active_drawable (gdisp->gimage)))
    {
      g_message (_("Hue-Saturation operates only on RGB color drawables."));
      return;
    }

  /*  The "hue-saturation" dialog  */
  if (!hue_saturation_dialog)
    hue_saturation_dialog = hue_saturation_dialog_new ();
  else
    if (!GTK_WIDGET_VISIBLE (hue_saturation_dialog->shell))
      gtk_widget_show (hue_saturation_dialog->shell);

  for (i = 0; i < 7; i++)
    {
      hue_saturation_dialog->hue[i] = 0.0;
      hue_saturation_dialog->lightness[i] = 0.0;
      hue_saturation_dialog->saturation[i] = 0.0;
    }

  hue_saturation_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  hue_saturation_dialog->image_map =
    image_map_create (gdisp, hue_saturation_dialog->drawable);

  hue_saturation_update (hue_saturation_dialog, ALL);
}

void
hue_saturation_free (void)
{
  if (hue_saturation_dialog)
    {
      if (hue_saturation_dialog->image_map)
	{
	  active_tool->preserve = TRUE;
	  image_map_abort (hue_saturation_dialog->image_map);
	  active_tool->preserve = FALSE;

	  hue_saturation_dialog->image_map = NULL;
	}
      gtk_widget_destroy (hue_saturation_dialog->shell);
    }
}

/***************************/
/*  Hue-Saturation dialog  */
/***************************/

static HueSaturationDialog *
hue_saturation_dialog_new (void)
{
  HueSaturationDialog *hsd;
  GtkWidget *main_vbox;
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *abox;
  GtkWidget *spinbutton;
  GtkWidget *toggle;
  GtkWidget *radio_button;
  GtkWidget *frame;
  GtkObject *data;
  GSList *group = NULL;
  gint i;

  gchar *hue_partition_names[] =
  {
    N_("Master"),
    N_("R"),
    N_("Y"),
    N_("G"),
    N_("C"),
    N_("B"),
    N_("M")
  };

  hsd = g_new (HueSaturationDialog, 1);
  hsd->hue_partition = ALL_HUES;
  hsd->preview       = TRUE;

  /*  The shell and main vbox  */
  hsd->shell = gimp_dialog_new (_("Hue-Saturation"), "hue_saturation",
				tools_help_func, NULL,
				GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				_("OK"), hue_saturation_ok_callback,
				hsd, NULL, NULL, TRUE, FALSE,
				_("Reset"), hue_saturation_reset_callback,
				hsd, NULL, NULL, FALSE, FALSE,
				_("Cancel"), hue_saturation_cancel_callback,
				hsd, NULL, NULL, FALSE, TRUE,

				NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (hsd->shell)->vbox), main_vbox);

  /*  The main hbox containing hue partitions and sliders  */
  main_hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_hbox, FALSE, FALSE, 0);

  /*  The table containing hue partitions  */
  table = gtk_table_new (7, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (main_hbox), table, FALSE, FALSE, 0);

  /*  the radio buttons for hue partitions  */
  for (i = 0; i < 7; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, gettext (hue_partition_names[i]));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_object_set_data (GTK_OBJECT (radio_button), "hue_partition",
			   (gpointer) i);

      if (!i)
	{
	  gtk_table_attach (GTK_TABLE (table), radio_button, 0, 2, 0, 1,
			    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	}
      else
	{
	  gtk_table_attach (GTK_TABLE (table), radio_button, 0, 1, i, i + 1,
			    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	  frame = gtk_frame_new (NULL);
	  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	  gtk_table_attach (GTK_TABLE (table), frame,  1, 2, i, i + 1,
			    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	  hsd->hue_partition_da[i - 1] = gtk_preview_new (GTK_PREVIEW_COLOR);
	  gtk_preview_size (GTK_PREVIEW (hsd->hue_partition_da[i - 1]),
			    DA_WIDTH, DA_HEIGHT);
	  gtk_widget_set_events (hsd->hue_partition_da[i - 1],
				 HUE_PARTITION_MASK);
	  gtk_signal_connect (GTK_OBJECT (hsd->hue_partition_da[i - 1]), "event",
			      GTK_SIGNAL_FUNC (hue_saturation_hue_partition_events),
			      hsd);
	  gtk_container_add (GTK_CONTAINER (frame), hsd->hue_partition_da[i - 1]);

	  gtk_widget_show (hsd->hue_partition_da[i - 1]);
	  gtk_widget_show (frame);
	}

      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  GTK_SIGNAL_FUNC (hue_saturation_partition_callback),
			  hsd);

      gtk_widget_show (radio_button);
    }

  gtk_widget_show (table);

  /*  The vbox for the table and preview toggle  */
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Hue / Lightness / Saturation Adjustments"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  The table containing sliders  */
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  Create the hue scale widget  */
  label = gtk_label_new (_("Hue"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  data = gtk_adjustment_new (0, -180, 180.0, 1.0, 1.0, 0.0);
  hsd->hue_data = GTK_ADJUSTMENT (data);

  slider = gtk_hscale_new (hsd->hue_data);
  gtk_widget_set_usize (slider, SLIDER_WIDTH, -1);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  abox = gtk_vbox_new (FALSE, 0);
  spinbutton = gtk_spin_button_new (hsd->hue_data, 1.0, 0);
  gtk_widget_set_usize (spinbutton, 74, -1);
  gtk_box_pack_end (GTK_BOX (abox), spinbutton, FALSE, FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), abox, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_signal_connect (GTK_OBJECT (hsd->hue_data), "value_changed",
		      GTK_SIGNAL_FUNC (hue_saturation_hue_adjustment_update),
		      hsd);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (spinbutton);
  gtk_widget_show (abox);

  /*  Create the lightness scale widget  */
  label = gtk_label_new (_("Lightness"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  data = gtk_adjustment_new (0, -100.0, 100.0, 1.0, 1.0, 0.0);
  hsd->lightness_data = GTK_ADJUSTMENT (data);

  slider = gtk_hscale_new (hsd->lightness_data);
  gtk_widget_set_usize (slider, SLIDER_WIDTH, -1);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  abox = gtk_vbox_new (FALSE, 0);
  spinbutton = gtk_spin_button_new (hsd->lightness_data, 1.0, 0);
  gtk_widget_set_usize (spinbutton, 75, -1);
  gtk_box_pack_end (GTK_BOX (abox), spinbutton, FALSE, FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), abox, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_signal_connect (GTK_OBJECT (hsd->lightness_data), "value_changed",
		      GTK_SIGNAL_FUNC (hue_saturation_lightness_adjustment_update),
		      hsd);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (spinbutton);
  gtk_widget_show (abox);

  /*  Create the saturation scale widget  */
  label = gtk_label_new (_("Saturation"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  data = gtk_adjustment_new (0, -100.0, 100.0, 1.0, 1.0, 0.0);
  hsd->saturation_data = GTK_ADJUSTMENT (data);

  slider = gtk_hscale_new (hsd->saturation_data);
  gtk_widget_set_usize (slider, SLIDER_WIDTH, -1);
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 2, 3,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  abox = gtk_vbox_new (FALSE, 0);
  spinbutton = gtk_spin_button_new (hsd->saturation_data, 1.0, 0);
  gtk_widget_set_usize (spinbutton, 75, -1);
  gtk_box_pack_end (GTK_BOX (abox), spinbutton, FALSE, FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), abox, 2, 3, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_signal_connect (GTK_OBJECT (hsd->saturation_data), "value_changed",
		      GTK_SIGNAL_FUNC (hue_saturation_saturation_adjustment_update),
		      hsd);

  gtk_widget_show (label);
  gtk_widget_show (slider);
  gtk_widget_show (spinbutton);
  gtk_widget_show (abox);

  gtk_widget_show (table);

  /*  Horizontal box for preview toggle button  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), hsd->preview);
  gtk_box_pack_end (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (hue_saturation_preview_update),
		      hsd);

  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (main_hbox);
  gtk_widget_show (main_vbox);
  gtk_widget_show (hsd->shell);

  return hsd;
}

static void
hue_saturation_update (HueSaturationDialog *hsd,
		       gint                 update)
{
  gint i, j, b;
  gint rgb[3];
  guchar buf[DA_WIDTH * 3];

  if (update & HUE_SLIDER)
    {
      gtk_adjustment_set_value (GTK_ADJUSTMENT (hsd->hue_data),
				hsd->hue[hsd->hue_partition]);
    }
  if (update & LIGHTNESS_SLIDER)
    {
      gtk_adjustment_set_value (GTK_ADJUSTMENT (hsd->lightness_data),
				hsd->lightness[hsd->hue_partition]);
    }
  if (update & SATURATION_SLIDER)
    {
      gtk_adjustment_set_value (GTK_ADJUSTMENT (hsd->saturation_data),
				hsd->saturation[hsd->hue_partition]);
    }

  hue_saturation_calculate_transfers (hsd);

  for (i = 0; i < 6; i++)
    {
      rgb[RED_PIX]   = default_colors[i][RED_PIX];
      rgb[GREEN_PIX] = default_colors[i][GREEN_PIX];
      rgb[BLUE_PIX]  = default_colors[i][BLUE_PIX];

      gimp_rgb_to_hls (rgb, rgb + 1, rgb + 2);

      rgb[RED_PIX]   = hue_transfer[i][rgb[RED_PIX]];
      rgb[GREEN_PIX] = lightness_transfer[i][rgb[GREEN_PIX]];
      rgb[BLUE_PIX]  = saturation_transfer[i][rgb[BLUE_PIX]];

      gimp_hls_to_rgb (rgb, rgb + 1, rgb + 2);

      for (j = 0; j < DA_WIDTH; j++)
	for (b = 0; b < 3; b++)
	  buf[j * 3 + b] = (guchar) rgb[b];

      for (j = 0; j < DA_HEIGHT; j++)
	gtk_preview_draw_row (GTK_PREVIEW (hsd->hue_partition_da[i]),
			      buf, 0, j, DA_WIDTH);

      if (update & DRAW)
	gtk_widget_draw (hsd->hue_partition_da[i], NULL);
    }
}

static void
hue_saturation_preview (HueSaturationDialog *hsd)
{
  if (!hsd->image_map)
    {
      g_warning ("hue_saturation_preview(): No image map");
      return;
    }

  active_tool->preserve = TRUE;
  image_map_apply (hsd->image_map, hue_saturation, (void *) hsd);
  active_tool->preserve = FALSE;
}

static void
hue_saturation_reset_callback (GtkWidget *widget,
			       gpointer   data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) data;

  hsd->hue[hsd->hue_partition] = 0.0;
  hsd->lightness[hsd->hue_partition] = 0.0;
  hsd->saturation[hsd->hue_partition] = 0.0;

  hue_saturation_update (hsd, ALL);

  if (hsd->preview)
    hue_saturation_preview (hsd);
}

static void
hue_saturation_ok_callback (GtkWidget *widget,
			    gpointer   data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) data;

  gimp_dialog_hide (hsd->shell);

  active_tool->preserve = TRUE;

  if (!hsd->preview)
    image_map_apply (hsd->image_map, hue_saturation, (void *) hsd);

  if (hsd->image_map)
    image_map_commit (hsd->image_map);

  active_tool->preserve = FALSE;

  hsd->image_map = NULL;

  active_tool->gdisp_ptr = NULL;
  active_tool->drawable = NULL;
}

static void
hue_saturation_cancel_callback (GtkWidget *widget,
				gpointer   data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) data;

  gimp_dialog_hide (hsd->shell);

  if (hsd->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (hsd->image_map);
      active_tool->preserve = FALSE;

      gdisplays_flush ();
      hsd->image_map = NULL;
    }

  active_tool->gdisp_ptr = NULL;
  active_tool->drawable = NULL;
}

static void
hue_saturation_partition_callback (GtkWidget *widget,
				   gpointer   data)
{
  HueSaturationDialog *hsd;
  HueRange partition;

  hsd = (HueSaturationDialog *) data;

  partition = (HueRange) gtk_object_get_data (GTK_OBJECT (widget),
					      "hue_partition");
  hsd->hue_partition = partition;

  hue_saturation_update (hsd, ALL);
}

static void
hue_saturation_preview_update (GtkWidget *widget,
			       gpointer   data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      hsd->preview = TRUE;
      hue_saturation_preview (hsd);
    }
  else
    hsd->preview = FALSE;
}

static void
hue_saturation_hue_adjustment_update (GtkAdjustment *adjustment,
				      gpointer       data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) data;

  if (hsd->hue[hsd->hue_partition] != adjustment->value)
    {
      hsd->hue[hsd->hue_partition] = adjustment->value;
      hue_saturation_update (hsd, DRAW);

      if (hsd->preview)
	hue_saturation_preview (hsd);
    }
}

static void
hue_saturation_lightness_adjustment_update (GtkAdjustment *adjustment,
					    gpointer       data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) data;

  if (hsd->lightness[hsd->hue_partition] != adjustment->value)
    {
      hsd->lightness[hsd->hue_partition] = adjustment->value;
      hue_saturation_update (hsd, DRAW);

      if (hsd->preview)
	hue_saturation_preview (hsd);
    }
}

static void
hue_saturation_saturation_adjustment_update (GtkAdjustment *adjustment,
					     gpointer       data)
{
  HueSaturationDialog *hsd;

  hsd = (HueSaturationDialog *) data;

  if (hsd->saturation[hsd->hue_partition] != adjustment->value)
    {
      hsd->saturation[hsd->hue_partition] = adjustment->value;
      hue_saturation_update (hsd, DRAW);

      if (hsd->preview)
	hue_saturation_preview (hsd);
    }
}

static gint
hue_saturation_hue_partition_events (GtkWidget           *widget,
				     GdkEvent            *event,
				     HueSaturationDialog *hsd)
{
  switch (event->type)
    {
    case GDK_EXPOSE:
      hue_saturation_update (hsd, HUE_PARTITION);
      break;

    default:
      break;
    }

  return FALSE;
}
