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

static void 	  gimp_color_picker_tool_class_init (GimpColorPickerToolClass *klass);
static void       gimp_color_picker_tool_init       (GimpColorPickerTool      *tool);
static void       gimp_color_picker_tool_finalize   (GObject                  *object);

static void       gimp_color_picker_tool_initialize (GimpTool       *tool,
                                                     GimpDisplay    *gdisp);
static void       gimp_color_picker_tool_control    (GimpTool       *tool,
                                                     GimpToolAction  action,
                                                     GimpDisplay    *gdisp);
static void       gimp_color_picker_tool_picked     (GimpColorTool  *color_tool,
                                                     GimpImageType   sample_type,
                                                     GimpRGB        *color,
                                                     gint            color_index);

static InfoDialog * gimp_color_picker_tool_info_create (GimpTool      *tool);
static void         gimp_color_picker_tool_info_close  (GtkWidget     *widget,
                                                        gpointer       data);
static void         gimp_color_picker_tool_info_update (GimpImageType  sample_type,
                                                        GimpRGB        *color,
                                                        gint            color_index);


static InfoDialog         *gimp_color_picker_tool_info = NULL;
static GtkWidget          *color_area                  = NULL;
static gchar               red_buf  [MAX_INFO_BUF];
static gchar               green_buf[MAX_INFO_BUF];
static gchar               blue_buf [MAX_INFO_BUF];
static gchar               alpha_buf[3 * MAX_INFO_BUF];
static gchar               index_buf[MAX_INFO_BUF];
static gchar               hex_buf  [MAX_INFO_BUF];

static GimpColorToolClass *parent_class = NULL;


void
gimp_color_picker_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data)
{
  (* callback) (GIMP_TYPE_COLOR_PICKER_TOOL,
                GIMP_TYPE_COLOR_PICKER_OPTIONS,
                gimp_color_picker_options_gui,
                0,
                "gimp-color-picker-tool",
                _("Color Picker"),
                _("Pick colors from the image"),
                N_("/Tools/C_olor Picker"), "O",
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

  object_class->finalize   = gimp_color_picker_tool_finalize;

  tool_class->initialize   = gimp_color_picker_tool_initialize;
  tool_class->control      = gimp_color_picker_tool_control;

  color_tool_class->picked = gimp_color_picker_tool_picked;
}

static void
gimp_color_picker_tool_init (GimpColorPickerTool *tool)
{
  gimp_tool_control_set_preserve (GIMP_TOOL (tool)->control, FALSE);

  gimp_tool_control_set_tool_cursor (GIMP_TOOL (tool)->control,
                                     GIMP_COLOR_PICKER_TOOL_CURSOR);
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
gimp_color_picker_tool_initialize (GimpTool    *tool,
                                   GimpDisplay *gdisp)
{
  GIMP_TOOL_CLASS (parent_class)->initialize (tool, gdisp);

  /*  always pick colors  */
  gimp_color_tool_enable (GIMP_COLOR_TOOL (tool),
                          GIMP_COLOR_OPTIONS (tool->tool_info->tool_options));
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
        {
          info_dialog_free (gimp_color_picker_tool_info);
          gimp_color_picker_tool_info = NULL;
        }
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_color_picker_tool_picked (GimpColorTool *color_tool,
                               GimpImageType  sample_type,
                               GimpRGB       *color,
                               gint           color_index)
{
  GimpTool               *tool;
  GimpColorPickerOptions *options;

  tool = GIMP_TOOL (color_tool);

  if (! gimp_color_picker_tool_info)
    gimp_color_picker_tool_info = gimp_color_picker_tool_info_create (tool);

  gimp_color_picker_tool_info_update (sample_type, color, color_index);

  options = GIMP_COLOR_PICKER_OPTIONS (color_tool->options);

  if (options->update_active)
    {
      GimpContext *user_context;
      
      user_context = gimp_get_user_context (tool->gdisp->gimage->gimp);
      
#if 0
      gimp_palette_editor_update_color (user_context, color, update_state);
#endif

      if (active_color == FOREGROUND)
        gimp_context_set_foreground (user_context, color);
      else if (active_color == BACKGROUND)
        gimp_context_set_background (user_context, color);
    }
}

static InfoDialog *
gimp_color_picker_tool_info_create (GimpTool *tool)
{
  InfoDialog *info_dialog;
  GtkWidget  *hbox;
  GtkWidget  *frame;
  GimpRGB     color;

  g_return_val_if_fail (tool->drawable != NULL, NULL);

  info_dialog = info_dialog_new (NULL,
                                 _("Color Picker"), "color_picker",
                                 GIMP_STOCK_TOOL_COLOR_PICKER,
                                 _("Color Picker Information"),
                                 gimp_standard_help_func,
                                 tool->tool_info->help_data);

  gimp_dialog_create_action_area (GIMP_DIALOG (info_dialog->shell),

                                  GTK_STOCK_CLOSE,
                                  gimp_color_picker_tool_info_close,
                                  info_dialog, NULL, NULL, TRUE, TRUE,

                                  NULL);

  /*  if the gdisplay is for a color image, the dialog must have RGB  */
  switch (GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (tool->drawable)))
    {
    case GIMP_RGB:
      info_dialog_add_label (info_dialog, _("Red:"),       red_buf);
      info_dialog_add_label (info_dialog, _("Green:"),     green_buf);
      info_dialog_add_label (info_dialog, _("Blue:"),      blue_buf);
      break;

    case GIMP_GRAY:
      info_dialog_add_label (info_dialog, _("Intensity:"), red_buf);
      break;

    case GIMP_INDEXED:
      info_dialog_add_label (info_dialog, _("Index:"),     index_buf);
      info_dialog_add_label (info_dialog, _("Red:"),       red_buf);
      info_dialog_add_label (info_dialog, _("Green:"),     green_buf);
      info_dialog_add_label (info_dialog, _("Blue:"),      blue_buf);
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  info_dialog_add_label (info_dialog, _("Alpha:"),       alpha_buf);
  info_dialog_add_label (info_dialog, _("Hex Triplet:"), hex_buf);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (info_dialog->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  gtk_widget_reparent (info_dialog->info_table, hbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 0.0);
  color_area = gimp_color_area_new (&color,
                                    gimp_drawable_has_alpha (tool->drawable) ?
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

  gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (info_dialog->shell),
                                     GIMP_VIEWABLE (tool->drawable));

  return info_dialog;
}

static void
gimp_color_picker_tool_info_close (GtkWidget *widget,
                                   gpointer   client_data)
{
  info_dialog_free (gimp_color_picker_tool_info);
  gimp_color_picker_tool_info = NULL;
}

static void
gimp_color_picker_tool_info_update (GimpImageType  sample_type,
                                    GimpRGB       *color,
                                    gint           color_index)
{
  guchar r, g, b, a;

  gimp_rgba_get_uchar (color, &r, &g, &b, &a);

  g_snprintf (red_buf,   MAX_INFO_BUF, "%d", r);
  g_snprintf (green_buf, MAX_INFO_BUF, "%d", g);
  g_snprintf (blue_buf,  MAX_INFO_BUF, "%d", b);
  
  if (GIMP_IMAGE_TYPE_HAS_ALPHA (sample_type))
    g_snprintf (alpha_buf, sizeof (alpha_buf),
		"%d (%d %%)", a, (gint) (color->a * 100.0));
  else
    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
  
  switch (GIMP_IMAGE_TYPE_BASE_TYPE (sample_type))
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      g_snprintf (index_buf, MAX_INFO_BUF, _("N/A"));
      break;

    case GIMP_INDEXED:
      g_snprintf (index_buf, MAX_INFO_BUF, "%d", color_index);
      break;
    }

  g_snprintf (hex_buf, MAX_INFO_BUF, "#%.2x%.2x%.2x", r, g, b);
  
  gimp_color_area_set_color (GIMP_COLOR_AREA (color_area), color);

  info_dialog_update (gimp_color_picker_tool_info);
  info_dialog_popup (gimp_color_picker_tool_info);
}
