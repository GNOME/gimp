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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "core/core-types.h"
#include "display/display-types.h"
#include "libgimptool/gimptooltypes.h"

#include "base/pixel-region.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "widgets/gimpenummenu.h"

#include "gimpcolorbalancetool.h"
#include "gimpcolorbalancetool-transfer.h"
#include "tool_manager.h"

#include "app_procs.h"
#include "image_map.h"

#include "libgimp/gimpintl.h"


#define CYAN_RED       0x1
#define MAGENTA_GREEN  0x2
#define YELLOW_BLUE    0x4
#define ALL           (CYAN_RED | MAGENTA_GREEN | YELLOW_BLUE)


/*  local function prototypes  */

static void   gimp_color_balance_tool_class_init (GimpColorBalanceToolClass *klass);
static void   gimp_color_balance_tool_init       (GimpColorBalanceTool      *bc_tool);

static void   gimp_color_balance_tool_initialize (GimpTool       *tool,
						  GimpDisplay    *gdisp);
static void   gimp_color_balance_tool_control    (GimpTool       *tool,
						  GimpToolAction  action,
						  GimpDisplay    *gdisp);

static ColorBalanceDialog * color_balance_dialog_new (void);

static void   color_balance_dialog_hide          (void);
static void   color_balance_update               (ColorBalanceDialog *cbd,
						  gint                update);
static void   color_balance_preview              (ColorBalanceDialog *cbd);
static void   color_balance_reset_callback       (GtkWidget          *widget,
						  gpointer            data);
static void   color_balance_ok_callback          (GtkWidget          *widget,
						  gpointer            data);
static void   color_balance_cancel_callback      (GtkWidget          *widget,
						  gpointer            data);
static void   color_balance_range_callback       (GtkWidget          *widget,
						  gpointer            data);
static void   color_balance_preserve_update      (GtkWidget          *widget,
						  gpointer            data);
static void   color_balance_preview_update       (GtkWidget          *widget,
						  gpointer            data);
static void   color_balance_cr_adjustment_update (GtkAdjustment      *adj,
						  gpointer            data);
static void   color_balance_mg_adjustment_update (GtkAdjustment      *adj,
						  gpointer            data);
static void   color_balance_yb_adjustment_update (GtkAdjustment      *adj,
						  gpointer            data);


static ColorBalanceDialog *color_balance_dialog = NULL;

static GimpImageMapToolClass *parent_class = NULL;


/*  functions  */

void
gimp_color_balance_tool_register (GimpToolRegisterCallback  callback,
                                  Gimp                     *gimp)
{
  (* callback) (GIMP_TYPE_COLOR_BALANCE_TOOL,
                NULL,
                FALSE,
                "gimp-color-balance-tool",
                _("Color Balance"),
                _("Adjust color balance"),
                N_("/Layer/Colors/Color Balance..."), NULL,
                NULL, "tools/color_balance.html",
                GIMP_STOCK_TOOL_COLOR_BALANCE,
                gimp);
}

GType
gimp_color_balance_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpColorBalanceToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_balance_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorBalanceTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_balance_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_IMAGE_MAP_TOOL,
					  "GimpColorBalanceTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_color_balance_tool_class_init (GimpColorBalanceToolClass *klass)
{
  GimpToolClass *tool_class;

  tool_class = GIMP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->initialize = gimp_color_balance_tool_initialize;
  tool_class->control    = gimp_color_balance_tool_control;

  gimp_color_balance_tool_transfer_init ();
}

static void
gimp_color_balance_tool_init (GimpColorBalanceTool *bc_tool)
{
  GIMP_TOOL(bc_tool)->control = gimp_tool_control_new  (FALSE,                      /* scroll_lock */
                                                        TRUE,                       /* auto_snap_to */
                                                        TRUE,                       /* preserve */
                                                        FALSE,                      /* handle_empty_image */
                                                        FALSE,                      /* perfectmouse */
                                                        GIMP_MOUSE_CURSOR,          /* cursor */
                                                        GIMP_TOOL_CURSOR_NONE,      /* tool_cursor */
                                                        GIMP_CURSOR_MODIFIER_NONE,  /* cursor_modifier */
                                                        GIMP_MOUSE_CURSOR,          /* toggle_cursor */
                                                        GIMP_TOOL_CURSOR_NONE,      /* toggle_tool_cursor */
                                                        GIMP_CURSOR_MODIFIER_NONE   /* toggle_cursor_modifier */);
}

static void
gimp_color_balance_tool_initialize (GimpTool    *tool,
				    GimpDisplay *gdisp)
{
  gint i;

  if (! gdisp)
    {
      color_balance_dialog_hide ();
      return;
    }

  if (! gimp_drawable_is_rgb (gimp_image_active_drawable (gdisp->gimage)))
    {
      g_message (_("Color balance operates only on RGB color drawables."));
      return;
    }

  /*  The color balance dialog  */
  if (!color_balance_dialog)
    color_balance_dialog = color_balance_dialog_new ();
  else
    if (!GTK_WIDGET_VISIBLE (color_balance_dialog->shell))
      gtk_widget_show (color_balance_dialog->shell);

  for (i = 0; i < 3; i++)
    {
      color_balance_dialog->cyan_red[i]      = 0.0;
      color_balance_dialog->magenta_green[i] = 0.0;
      color_balance_dialog->yellow_blue[i]   = 0.0;
    }

  color_balance_dialog->drawable = gimp_image_active_drawable (gdisp->gimage);
  color_balance_dialog->image_map =
    image_map_create (gdisp, color_balance_dialog->drawable);

  color_balance_update (color_balance_dialog, ALL);
}

static void
gimp_color_balance_tool_control (GimpTool       *tool,
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
      color_balance_dialog_hide ();
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}


/*  color balance machinery  */

void
color_balance (PixelRegion *srcPR,
	       PixelRegion *destPR,
	       void        *data)
{
  ColorBalanceDialog *cbd;
  guchar             *src, *s;
  guchar             *dest, *d;
  gboolean            alpha;
  gint                r, g, b;
  gint                r_n, g_n, b_n;
  gint                w, h;

  cbd = (ColorBalanceDialog *) data;

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

	  r_n = cbd->r_lookup[r];
	  g_n = cbd->g_lookup[g];
	  b_n = cbd->b_lookup[b];

	  if (cbd->preserve_luminosity)
	    {
	      gimp_rgb_to_hls_int (&r_n, &g_n, &b_n);
	      g_n = gimp_rgb_to_l_int (r, g, b);
	      gimp_hls_to_rgb_int (&r_n, &g_n, &b_n);
	    }

	  d[RED_PIX] = r_n;
	  d[GREEN_PIX] = g_n;
 	  d[BLUE_PIX] = b_n;

	  if (alpha)
	    d[ALPHA_PIX] = s[ALPHA_PIX];

	  s += srcPR->bytes;
	  d += destPR->bytes;
	}

      src += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

/**************************/
/*  Color Balance dialog  */
/**************************/

static GtkAdjustment *
create_levels_scale (const gchar   *left,
                     const gchar   *right,
                     GtkWidget     *table,
                     gint           col)
{
  GtkWidget     *label;
  GtkWidget     *slider;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;

  adj = GTK_ADJUSTMENT (gtk_adjustment_new (0, -100.0, 100.0, 1.0, 10.0, 0.0));

  label = gtk_label_new (left);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, col, col + 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  slider = gtk_hscale_new (adj);
  gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_widget_set_size_request (slider, 100, -1);
  gtk_table_attach_defaults (GTK_TABLE (table), slider, 1, 2, col, col + 1);
  gtk_widget_show (slider);

  label = gtk_label_new (right);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, col, col + 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_widget_set_size_request (spinbutton, 75, -1);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 3, 4, col, col + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  return adj;
}

static ColorBalanceDialog *
color_balance_dialog_new (void)
{
  ColorBalanceDialog *cbd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *toggle;
  GtkWidget *frame;

  cbd = g_new0 (ColorBalanceDialog, 1);
  cbd->preserve_luminosity = TRUE;
  cbd->preview             = TRUE;
  cbd->transfer_mode       = GIMP_MIDTONES;

  /*  The shell and main vbox  */
  cbd->shell = gimp_dialog_new (_("Color Balance"), "color_balance",
				tool_manager_help_func, NULL,
				GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				GTK_STOCK_CANCEL, color_balance_cancel_callback,
				cbd, NULL, NULL, FALSE, TRUE,

				GIMP_STOCK_RESET, color_balance_reset_callback,
				cbd, NULL, NULL, TRUE, FALSE,

				GTK_STOCK_OK, color_balance_ok_callback,
				cbd, NULL, NULL, TRUE, FALSE,

				NULL);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (cbd->shell)->vbox), vbox);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (_("Color Levels"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  The table containing sliders  */
  table = gtk_table_new (3, 4, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  cbd->cyan_red_adj = 
    create_levels_scale (_("Cyan"), _("Red"), table, 0);
  g_signal_connect (G_OBJECT (cbd->cyan_red_adj), "value_changed",
                    G_CALLBACK (color_balance_cr_adjustment_update),
                    cbd);

  cbd->magenta_green_adj = 
    create_levels_scale (_("Magenta"), _("Green"), table, 1);
  g_signal_connect (G_OBJECT (cbd->magenta_green_adj), "value_changed",
                    G_CALLBACK (color_balance_mg_adjustment_update),
                    cbd);

  cbd->yellow_blue_adj = 
    create_levels_scale (_("Yellow"), _("Blue"), table, 2);
  g_signal_connect (G_OBJECT (cbd->yellow_blue_adj), "value_changed",
                    G_CALLBACK (color_balance_yb_adjustment_update),
                    cbd);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gimp_enum_radio_frame_new (GIMP_TYPE_TRANSFER_MODE,
                                     gtk_label_new (_("Mode")),
                                     2,
                                     G_CALLBACK (color_balance_range_callback),
                                     cbd,
                                     &toggle);
  gimp_radio_group_set_active (GTK_RADIO_BUTTON (toggle),
                               GINT_TO_POINTER (cbd->transfer_mode));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_end (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), cbd->preview);
  gtk_box_pack_end (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (color_balance_preview_update),
                    cbd);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Preserve Luminosity"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				cbd->preserve_luminosity);
  gtk_box_pack_end (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (color_balance_preserve_update),
                    cbd);
  gtk_widget_show (toggle);

  gtk_widget_show (cbd->shell);

  return cbd;
}

static void
color_balance_dialog_hide (void)
{
  if (color_balance_dialog)
    color_balance_cancel_callback (NULL, (gpointer) color_balance_dialog);
}

static void
color_balance_update (ColorBalanceDialog *cbd,
		      gint                update)
{
  if (update & CYAN_RED)
    {
      gtk_adjustment_set_value (cbd->cyan_red_adj,
				cbd->cyan_red[cbd->transfer_mode]);
    }
  if (update & MAGENTA_GREEN)
    {
      gtk_adjustment_set_value (cbd->magenta_green_adj,
				cbd->magenta_green[cbd->transfer_mode]);
    }
  if (update & YELLOW_BLUE)
    {
      gtk_adjustment_set_value (cbd->yellow_blue_adj,
				cbd->yellow_blue[cbd->transfer_mode]);
    }
}

void
color_balance_create_lookup_tables (ColorBalanceDialog *cbd)
{
  gdouble *cyan_red_transfer[3];
  gdouble *magenta_green_transfer[3];
  gdouble *yellow_blue_transfer[3];
  gint i;
  gint32 r_n, g_n, b_n;

  /*  Set the transfer arrays  (for speed)  */
  cyan_red_transfer[GIMP_SHADOWS] = 
    (cbd->cyan_red[GIMP_SHADOWS] > 0) ? shadows_add : shadows_sub;
  cyan_red_transfer[GIMP_MIDTONES] = 
    (cbd->cyan_red[GIMP_MIDTONES] > 0) ? midtones_add : midtones_sub;
  cyan_red_transfer[GIMP_HIGHLIGHTS] = 
    (cbd->cyan_red[GIMP_HIGHLIGHTS] > 0) ? highlights_add : highlights_sub;

  magenta_green_transfer[GIMP_SHADOWS] = 
    (cbd->magenta_green[GIMP_SHADOWS] > 0) ? shadows_add : shadows_sub;
  magenta_green_transfer[GIMP_MIDTONES] = 
    (cbd->magenta_green[GIMP_MIDTONES] > 0) ? midtones_add : midtones_sub;
  magenta_green_transfer[GIMP_HIGHLIGHTS] = 
    (cbd->magenta_green[GIMP_HIGHLIGHTS] > 0) ? highlights_add : highlights_sub;
  yellow_blue_transfer[GIMP_SHADOWS] = 
    (cbd->yellow_blue[GIMP_SHADOWS] > 0) ? shadows_add : shadows_sub;
  yellow_blue_transfer[GIMP_MIDTONES] = 
    (cbd->yellow_blue[GIMP_MIDTONES] > 0) ? midtones_add : midtones_sub;
  yellow_blue_transfer[GIMP_HIGHLIGHTS] = 
    (cbd->yellow_blue[GIMP_HIGHLIGHTS] > 0) ? highlights_add : highlights_sub;

  for (i = 0; i < 256; i++)
    {
      r_n = i;
      g_n = i;
      b_n = i;

      r_n += cbd->cyan_red[GIMP_SHADOWS] * cyan_red_transfer[GIMP_SHADOWS][r_n];
      r_n = CLAMP0255 (r_n);
      r_n += cbd->cyan_red[GIMP_MIDTONES] * cyan_red_transfer[GIMP_MIDTONES][r_n];
      r_n = CLAMP0255 (r_n);
      r_n += cbd->cyan_red[GIMP_HIGHLIGHTS] * cyan_red_transfer[GIMP_HIGHLIGHTS][r_n];
      r_n = CLAMP0255 (r_n);

      g_n += cbd->magenta_green[GIMP_SHADOWS] * magenta_green_transfer[GIMP_SHADOWS][g_n];
      g_n = CLAMP0255 (g_n);
      g_n += cbd->magenta_green[GIMP_MIDTONES] * magenta_green_transfer[GIMP_MIDTONES][g_n];
      g_n = CLAMP0255 (g_n);
      g_n += cbd->magenta_green[GIMP_HIGHLIGHTS] * magenta_green_transfer[GIMP_HIGHLIGHTS][g_n];
      g_n = CLAMP0255 (g_n);

      b_n += cbd->yellow_blue[GIMP_SHADOWS] * yellow_blue_transfer[GIMP_SHADOWS][b_n];
      b_n = CLAMP0255 (b_n);
      b_n += cbd->yellow_blue[GIMP_MIDTONES] * yellow_blue_transfer[GIMP_MIDTONES][b_n];
      b_n = CLAMP0255 (b_n);
      b_n += cbd->yellow_blue[GIMP_HIGHLIGHTS] * yellow_blue_transfer[GIMP_HIGHLIGHTS][b_n];
      b_n = CLAMP0255 (b_n);

      cbd->r_lookup[i] = r_n;
      cbd->g_lookup[i] = g_n;
      cbd->b_lookup[i] = b_n;
    }
}

static void
color_balance_preview (ColorBalanceDialog *cbd)
{
  GimpTool *active_tool;

  active_tool = tool_manager_get_active (the_gimp);

  if (!cbd->image_map)
    {
      g_message ("color_balance_preview(): No image map");
      return;
    }

  gimp_tool_control_set_preserve(active_tool->control, TRUE);
  color_balance_create_lookup_tables (cbd);
  image_map_apply (cbd->image_map, color_balance, (void *) cbd);
  gimp_tool_control_set_preserve(active_tool->control, FALSE);
}

static void
color_balance_reset_callback (GtkWidget *widget,
			      gpointer   data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) data;

  cbd->cyan_red[cbd->transfer_mode]      = 0.0;
  cbd->magenta_green[cbd->transfer_mode] = 0.0;
  cbd->yellow_blue[cbd->transfer_mode]   = 0.0;

  color_balance_update (cbd, ALL);

  if (cbd->preview)
    color_balance_preview (cbd);
}

static void
color_balance_ok_callback (GtkWidget *widget,
			   gpointer   data)
{
  ColorBalanceDialog *cbd;
  GimpTool           *active_tool;

  cbd = (ColorBalanceDialog *) data;

  gtk_widget_hide (cbd->shell);
  
  active_tool = tool_manager_get_active (the_gimp);

  gimp_tool_control_set_preserve(active_tool->control, TRUE);

  if (!cbd->preview)
    image_map_apply (cbd->image_map, color_balance, (void *) cbd);

  if (cbd->image_map)
    image_map_commit (cbd->image_map);

  gimp_tool_control_set_preserve(active_tool->control, FALSE);

  cbd->image_map = NULL;

  active_tool->gdisp    = NULL;
  active_tool->drawable = NULL;
}

static void
color_balance_cancel_callback (GtkWidget *widget,
			       gpointer   data)
{
  ColorBalanceDialog *cbd;
  GimpTool           *active_tool;

  cbd = (ColorBalanceDialog *) data;

  gtk_widget_hide (cbd->shell);

  active_tool = tool_manager_get_active (the_gimp);

  if (cbd->image_map)
    {
      gimp_tool_control_set_preserve(active_tool->control, TRUE);
      image_map_abort (cbd->image_map);
      gimp_tool_control_set_preserve(active_tool->control, FALSE);

      gdisplays_flush ();
      cbd->image_map = NULL;
    }

  active_tool->gdisp    = NULL;
  active_tool->drawable = NULL;
}

static void
color_balance_range_callback (GtkWidget *widget,
			      gpointer   data)
{
  ColorBalanceDialog *cbd = (ColorBalanceDialog *) data;

  gimp_radio_button_update (widget, &cbd->transfer_mode);

  color_balance_update (cbd, ALL);
}

static void
color_balance_preserve_update (GtkWidget *widget,
			       gpointer   data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    cbd->preserve_luminosity = TRUE;
  else
    cbd->preserve_luminosity = FALSE;

  if (cbd->preview)
    color_balance_preview (cbd);
}

static void
color_balance_preview_update (GtkWidget *widget,
			      gpointer   data)
{
  ColorBalanceDialog *cbd;
  GimpTool           *active_tool;

  cbd = (ColorBalanceDialog *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      cbd->preview = TRUE;
      color_balance_preview (cbd);
    }
  else
    {
      cbd->preview = FALSE;
      if (cbd->image_map)
	{
	  active_tool = tool_manager_get_active (the_gimp);

	  gimp_tool_control_set_preserve(active_tool->control, TRUE);
	  image_map_clear (cbd->image_map);
	  gimp_tool_control_set_preserve(active_tool->control, FALSE);
	  gdisplays_flush ();
	}
    }
}

static void
color_balance_cr_adjustment_update (GtkAdjustment *adjustment,
				    gpointer       data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) data;

  if (cbd->cyan_red[cbd->transfer_mode] != adjustment->value)
    {
      cbd->cyan_red[cbd->transfer_mode] = adjustment->value;

      if (cbd->preview)
	color_balance_preview (cbd);
    }
}

static void
color_balance_mg_adjustment_update (GtkAdjustment *adjustment,
				    gpointer       data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) data;

  if (cbd->magenta_green[cbd->transfer_mode] != adjustment->value)
    {
      cbd->magenta_green[cbd->transfer_mode] = adjustment->value;

      if (cbd->preview)
	color_balance_preview (cbd);
    }
}

static void
color_balance_yb_adjustment_update (GtkAdjustment *adjustment,
				    gpointer       data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) data;

  if (cbd->yellow_blue[cbd->transfer_mode] != adjustment->value)
    {
      cbd->yellow_blue[cbd->transfer_mode] = adjustment->value;

      if (cbd->preview)
	color_balance_preview (cbd);
    }
}
