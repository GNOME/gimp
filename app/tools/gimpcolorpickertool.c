/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#ifdef __GNUC__
#warning FIXME #include "gui/gui-types.h"
#endif
#include "gui/gui-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimppaletteeditor.h"
#include "widgets/gimptoolbox-color-area.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"

#include "gui/info-dialog.h"

#include "gimpcolorpickeroptions.h"
#include "gimpcolorpickertool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define MAX_INFO_BUF 8


/*  local function prototypes  */

static void 	  gimp_color_picker_tool_class_init     (GimpColorPickerToolClass *klass);
static void       gimp_color_picker_tool_init           (GimpColorPickerTool *color_picker_tool);

static void       gimp_color_picker_tool_finalize       (GObject          *object);

static void       gimp_color_picker_tool_control        (GimpTool         *tool,
                                                         GimpToolAction    action,
					       		 GimpDisplay      *gdisp);
static void       gimp_color_picker_tool_button_press   (GimpTool         *tool,
                                                         GimpCoords       *coords,
                                                         guint32           time,
							 GdkModifierType   state,
							 GimpDisplay      *gdisp);
static void       gimp_color_picker_tool_button_release (GimpTool         *tool,
                                                         GimpCoords       *coords,
                                                         guint32           time,
					        	 GdkModifierType   state,
					        	 GimpDisplay      *gdisp);
static void       gimp_color_picker_tool_motion         (GimpTool         *tool,
                                                         GimpCoords       *coords,
                                                         guint32           time,
					       		 GdkModifierType   state,
					       		 GimpDisplay      *gdisp);
static gboolean   gimp_color_picker_tool_pick           (GimpColorTool    *tool,
							 GimpColorOptions *options,
							 GimpDisplay      *display,
							 gint              x,
							 gint              y);

static InfoDialog * gimp_color_picker_tool_info_create (GimpToolInfo *tool_info,
                                                        GimpDrawable *drawable);
static void         gimp_color_picker_tool_info_close  (GtkWidget    *widget,
                                                        gpointer      data);
static void         gimp_color_picker_tool_info_update (GimpTool     *tool,
                                                        gboolean      valid);


static gint                  col_value[5] = { 0, 0, 0, 0, 0 };
static GimpUpdateColorState  update_type;
static GimpImageType         sample_type;
static InfoDialog           *gimp_color_picker_tool_info = NULL;
static GtkWidget            *color_area        = NULL;
static gchar                 red_buf  [MAX_INFO_BUF];
static gchar                 green_buf[MAX_INFO_BUF];
static gchar                 blue_buf [MAX_INFO_BUF];
static gchar                 alpha_buf[MAX_INFO_BUF];
static gchar                 index_buf[MAX_INFO_BUF];
static gchar                 gray_buf [MAX_INFO_BUF];
static gchar                 hex_buf  [MAX_INFO_BUF];

static GimpColorToolClass   *parent_class = NULL;


void
gimp_color_picker_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data)
{
  (* callback) (GIMP_TYPE_COLOR_PICKER_TOOL,
                GIMP_TYPE_COLOR_PICKER_OPTIONS,
                gimp_color_picker_options_gui,
                FALSE,
                "gimp-color-picker-tool",
                _("Color Picker"),
                _("Pick colors from the image"),
                N_("/Tools/Color Picker"), "O",
                NULL, "tools/color_picker.html",
                GIMP_STOCK_TOOL_COLOR_PICKER,
                data);
}

GtkType
gimp_color_picker_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpColorPickerToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_picker_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorPickerTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_picker_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_COLOR_TOOL,
					  "GimpColorPickerTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_color_picker_tool_class_init (GimpColorPickerToolClass *klass)
{
  GObjectClass       *object_class;
  GimpToolClass      *tool_class;
  GimpColorToolClass *color_tool_class;

  object_class     = G_OBJECT_CLASS (klass);
  tool_class       = GIMP_TOOL_CLASS (klass);
  color_tool_class = GIMP_COLOR_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_color_picker_tool_finalize;

  tool_class->control        = gimp_color_picker_tool_control;
  tool_class->button_press   = gimp_color_picker_tool_button_press;
  tool_class->button_release = gimp_color_picker_tool_button_release;
  tool_class->motion         = gimp_color_picker_tool_motion;

  color_tool_class->pick     = gimp_color_picker_tool_pick;
}

static void
gimp_color_picker_tool_init (GimpColorPickerTool *color_picker_tool)
{
  GimpTool *tool = GIMP_TOOL (color_picker_tool);

  gimp_tool_control_set_preserve (tool->control, FALSE);
}

static void
gimp_color_picker_tool_finalize (GObject *object)
{
  if (gimp_color_picker_tool_info)
    {
      info_dialog_free (gimp_color_picker_tool_info);
      gimp_color_picker_tool_info = NULL;

      color_area = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_picker_tool_control (GimpTool       *tool,
				GimpToolAction  action,
		      		GimpDisplay    *gdisp)
{
  switch (action)
    {
    case HALT:
      if (gimp_color_picker_tool_info)
	info_dialog_popdown (gimp_color_picker_tool_info);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_color_picker_tool_button_press (GimpTool        *tool,
                                     GimpCoords      *coords,
                                     guint32          time,
				     GdkModifierType  state,
			   	     GimpDisplay     *gdisp)
{
  GimpColorPickerTool *cp_tool;
  GimpColorOptions    *options;

  GIMP_TOOL_CLASS (parent_class)->button_press (tool,
						coords, time, state, gdisp);

  cp_tool = GIMP_COLOR_PICKER_TOOL (tool);
  options = GIMP_COLOR_OPTIONS (tool->tool_info->tool_options);

  if (! gimp_color_picker_tool_info)
    gimp_color_picker_tool_info =
      gimp_color_picker_tool_info_create (tool->tool_info,
                                          tool->drawable);

  gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (gimp_color_picker_tool_info->shell),
                                     GIMP_VIEWABLE (tool->drawable));

  gimp_color_picker_tool_info_update (tool,
				      gimp_color_tool_pick (GIMP_COLOR_TOOL (tool),
							    options,
							    gdisp,
							    coords->x,
							    coords->y));
 
  /*  if the shift key is down, create a new color.
   *  otherwise, modify the current color.
   */
  if (state & GDK_SHIFT_MASK)
    {
      update_type = GIMP_UPDATE_COLOR_STATE_UPDATE_NEW;
    }
  else
    {
      update_type = GIMP_UPDATE_COLOR_STATE_UPDATE;
    }
}

static void
gimp_color_picker_tool_button_release (GimpTool        *tool,
                                       GimpCoords      *coords,
                                       guint32          time,
				       GdkModifierType  state,
				       GimpDisplay     *gdisp)
{
  GimpColorPickerTool *cp_tool;
  GimpColorOptions    *options;

  cp_tool = GIMP_COLOR_PICKER_TOOL(tool);
  options = GIMP_COLOR_OPTIONS (tool->tool_info->tool_options);

  gimp_color_picker_tool_info_update (tool,
				      gimp_color_tool_pick (GIMP_COLOR_TOOL (tool),
							    options,
							    gdisp,
							    coords->x,
							    coords->y));

  GIMP_TOOL_CLASS (parent_class)->button_release (tool,
						  coords, time, state, gdisp);
}

static void
gimp_color_picker_tool_motion (GimpTool        *tool,
                               GimpCoords      *coords,
                               guint32          time,
		               GdkModifierType  state,
		               GimpDisplay     *gdisp)
{
  GimpColorPickerTool *cp_tool;
  GimpColorOptions    *options;

  GIMP_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, gdisp);

  cp_tool = GIMP_COLOR_PICKER_TOOL (tool);
  options = GIMP_COLOR_OPTIONS (tool->tool_info->tool_options);

  gimp_color_picker_tool_info_update (tool,
				      gimp_color_tool_pick (GIMP_COLOR_TOOL (tool),
							    options,
							    gdisp,
							    coords->x,
							    coords->y));
}

static gboolean
gimp_color_picker_tool_pick (GimpColorTool    *tool,
			     GimpColorOptions *options,
			     GimpDisplay      *gdisp,
			     gint              x,
			     gint              y)
{
  GimpRGB   color;
  gint      color_index;
  gboolean  success;

  success = gimp_image_pick_color (gdisp->gimage,
                                   GIMP_TOOL (tool)->drawable,
				   options->sample_merged,
                                   x, y,
				   options->sample_average,
				   options->average_radius,
                                   &sample_type,
                                   &color,
                                   &color_index);
  
  if (success)
    {
      guchar r, g, b, a;

      gimp_rgba_get_uchar (&color, &r, &g, &b, &a);

      col_value[RED_PIX]   = r;
      col_value[GREEN_PIX] = g;
      col_value[BLUE_PIX]  = b;
      col_value[ALPHA_PIX] = a;
      col_value[4]         = color_index;

      if (GIMP_COLOR_PICKER_OPTIONS (options)->update_active)
        {
          GimpContext *user_context;

          user_context = gimp_get_user_context (gdisp->gimage->gimp);

#if 0
          gimp_palette_editor_update_color (user_context,
					    &color, update_state);
#endif

          if (active_color == FOREGROUND)
            gimp_context_set_foreground (user_context, &color);
          else if (active_color == BACKGROUND)
            gimp_context_set_background (user_context, &color);
        }
    }

  return success;
}

static InfoDialog *
gimp_color_picker_tool_info_create (GimpToolInfo *tool_info,
                                    GimpDrawable *drawable)
{
  InfoDialog *info_dialog;
  GtkWidget  *hbox;
  GtkWidget  *frame;
  GimpRGB     color;

  info_dialog = info_dialog_new (NULL,
                                 _("Color Picker"), "color_picker",
                                 GIMP_STOCK_TOOL_COLOR_PICKER,
                                 _("Color Picker Information"),
                                 gimp_standard_help_func,
                                 tool_info->help_data);

  gimp_dialog_create_action_area (GIMP_DIALOG (info_dialog->shell),

                                  GTK_STOCK_CLOSE,
                                  gimp_color_picker_tool_info_close,
                                  info_dialog, NULL, NULL, TRUE, TRUE,

                                  NULL);

  /*  if the gdisplay is for a color image, the dialog must have RGB  */
  switch (GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable)))
    {
    case GIMP_RGB:
      info_dialog_add_label (info_dialog, _("Red:"),         red_buf);
      info_dialog_add_label (info_dialog, _("Green:"),       green_buf);
      info_dialog_add_label (info_dialog, _("Blue:"),        blue_buf);
      info_dialog_add_label (info_dialog, _("Alpha:"),       alpha_buf);
      info_dialog_add_label (info_dialog, _("Hex Triplet:"), hex_buf);
      break;

    case GIMP_GRAY:
      info_dialog_add_label (info_dialog, _("Intensity:"),   gray_buf);
      info_dialog_add_label (info_dialog, _("Alpha:"),       alpha_buf);
      info_dialog_add_label (info_dialog, _("Hex Triplet:"), hex_buf);
      break;

    case GIMP_INDEXED:
      info_dialog_add_label (info_dialog, _("Index:"),      index_buf);
      info_dialog_add_label (info_dialog, _("Red:"),        red_buf);
      info_dialog_add_label (info_dialog, _("Green:"),      green_buf);
      info_dialog_add_label (info_dialog, _("Blue:"),       blue_buf);
      info_dialog_add_label (info_dialog, _("Alpha:"),      alpha_buf);
      info_dialog_add_label (info_dialog, _("Hex Triplet"), hex_buf);
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (info_dialog->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  gtk_widget_reparent (info_dialog->info_table, hbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 0.0);
  color_area = gimp_color_area_new (&color,
                                    gimp_drawable_has_alpha (drawable) ?
                                    GIMP_COLOR_AREA_LARGE_CHECKS :
                                    GIMP_COLOR_AREA_FLAT,
                                    GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
  gtk_widget_set_size_request (color_area, 48, 64);
  gtk_drag_dest_unset (color_area);
  gtk_container_add (GTK_CONTAINER (frame), color_area);
  gtk_widget_show (color_area);
  gtk_widget_show (frame);

  gimp_dialog_factory_add_foreign (gimp_dialog_factory_from_name ("toplevel"),
                                   "gimp-color-picker-tool-dialog",
                                   info_dialog->shell);

  return info_dialog;
}

static void
gimp_color_picker_tool_info_close (GtkWidget *widget,
                                   gpointer   client_data)
{
  info_dialog_popdown ((InfoDialog *) client_data);
}

static void
gimp_color_picker_tool_info_update (GimpTool  *tool,
				    gboolean   valid)
{
  if (!valid)
    {
      if (GTK_WIDGET_IS_SENSITIVE (color_area))
	gtk_widget_set_sensitive (color_area, FALSE);

      g_snprintf (red_buf,   MAX_INFO_BUF, _("N/A"));
      g_snprintf (green_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (blue_buf,  MAX_INFO_BUF, _("N/A"));
      g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (index_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (gray_buf,  MAX_INFO_BUF, _("N/A"));
      g_snprintf (hex_buf,   MAX_INFO_BUF, _("N/A"));
    }
  else
    {
      GimpRGB  color;
      guchar   r = 0;
      guchar   g = 0;
      guchar   b = 0;
      guchar   a = 0;

      if (! GTK_WIDGET_IS_SENSITIVE (color_area))
	gtk_widget_set_sensitive (color_area, TRUE);

      switch (GIMP_IMAGE_TYPE_BASE_TYPE (sample_type))
	{
	case GIMP_RGB:
	  g_snprintf (index_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (red_buf,   MAX_INFO_BUF, "%d", col_value [RED_PIX]);
	  g_snprintf (green_buf, MAX_INFO_BUF, "%d", col_value [GREEN_PIX]);
	  g_snprintf (blue_buf,  MAX_INFO_BUF, "%d", col_value [BLUE_PIX]);
	  if (sample_type == GIMP_RGBA_IMAGE)
	    g_snprintf (alpha_buf, MAX_INFO_BUF, "%d", col_value [ALPHA_PIX]);
	  else
	    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (hex_buf, MAX_INFO_BUF, "#%.2x%.2x%.2x",
		      col_value [RED_PIX],
		      col_value [GREEN_PIX],
		      col_value [BLUE_PIX]);

	  r = col_value [RED_PIX];
	  g = col_value [GREEN_PIX];
	  b = col_value [BLUE_PIX];
	  if (sample_type == GIMP_RGBA_IMAGE)
	    a = col_value [ALPHA_PIX];
	  break;

	case GIMP_GRAY:
	  g_snprintf (gray_buf, MAX_INFO_BUF, "%d", col_value [GRAY_PIX]);
	  if (sample_type == GIMP_GRAYA_IMAGE)
	    g_snprintf (alpha_buf, MAX_INFO_BUF, "%d", col_value [ALPHA_PIX]);
	  else
	    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (hex_buf, MAX_INFO_BUF, "#%.2x%.2x%.2x",
		      col_value [GRAY_PIX],
		      col_value [GRAY_PIX],
		      col_value [GRAY_PIX]);

	  r = col_value [GRAY_PIX];
	  g = col_value [GRAY_PIX];
	  b = col_value [GRAY_PIX];
	  if (sample_type == GIMP_GRAYA_IMAGE)
	    a = col_value [ALPHA_PIX];
	  break;

	case GIMP_INDEXED:
	  g_snprintf (index_buf, MAX_INFO_BUF, "%d", col_value [4]);
	  g_snprintf (red_buf,   MAX_INFO_BUF, "%d", col_value [RED_PIX]);
	  g_snprintf (green_buf, MAX_INFO_BUF, "%d", col_value [GREEN_PIX]);
	  g_snprintf (blue_buf,  MAX_INFO_BUF, "%d", col_value [BLUE_PIX]);
	  if (sample_type == GIMP_INDEXEDA_IMAGE)
	    g_snprintf (alpha_buf, MAX_INFO_BUF, "%d", col_value [ALPHA_PIX]);
	  else
	    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (hex_buf,   MAX_INFO_BUF, "#%.2x%.2x%.2x",
		      col_value [RED_PIX],
		      col_value [GREEN_PIX],
		      col_value [BLUE_PIX]);

	  r = col_value [RED_PIX];
	  g = col_value [GREEN_PIX];
	  b = col_value [BLUE_PIX];
	  if (sample_type == GIMP_INDEXEDA_IMAGE)
	    a = col_value [ALPHA_PIX];
	  break;
	}

      gimp_rgba_set_uchar (&color, r, g, b, a);

      gimp_color_area_set_color (GIMP_COLOR_AREA (color_area), &color);
    }

  info_dialog_update (gimp_color_picker_tool_info);
  info_dialog_popup (gimp_color_picker_tool_info);
}
