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

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpimagemap.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpitemfactory.h"
#include "widgets/gimptooldialog.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpcoloroptions.h"
#include "gimpimagemaptool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_image_map_tool_class_init (GimpImageMapToolClass *klass);
static void   gimp_image_map_tool_init       (GimpImageMapTool      *image_map_tool);

static void   gimp_image_map_tool_finalize   (GObject               *object);

static void   gimp_image_map_tool_initialize (GimpTool              *tool,
                                              GimpDisplay           *gdisp);
static void   gimp_image_map_tool_control    (GimpTool              *tool,
					      GimpToolAction         action,
					      GimpDisplay           *gdisp);

static gboolean gimp_image_map_tool_pick_color (GimpColorTool       *color_tool,
                                                gint                 x,
                                                gint                 y,
                                                GimpImageType       *sample_type,
                                                GimpRGB             *color,
                                                gint                *color_index);
static void   gimp_image_map_tool_map        (GimpImageMapTool      *image_map_tool);
static void   gimp_image_map_tool_dialog     (GimpImageMapTool      *image_map_tool);
static void   gimp_image_map_tool_reset      (GimpImageMapTool      *image_map_tool);

static void   gimp_image_map_tool_flush      (GimpImageMap          *image_map,
                                              GimpImageMapTool      *image_map_tool);

static void   gimp_image_map_tool_response        (GtkWidget        *widget,
                                                   gint              response_id,
                                                   GimpImageMapTool *image_map_tool);

static void   gimp_image_map_tool_preview_toggled (GtkWidget        *widget,
                                                   GimpImageMapTool *image_map_tool);


static GimpToolClass *parent_class = NULL;


GType
gimp_image_map_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpImageMapToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_image_map_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpImageMapTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_image_map_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_COLOR_TOOL,
					  "GimpImageMapTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_image_map_tool_class_init (GimpImageMapToolClass *klass)
{
  GObjectClass       *object_class;
  GimpToolClass      *tool_class;
  GimpColorToolClass *color_tool_class;

  object_class     = G_OBJECT_CLASS (klass);
  tool_class       = GIMP_TOOL_CLASS (klass);
  color_tool_class = GIMP_COLOR_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_image_map_tool_finalize;

  tool_class->initialize = gimp_image_map_tool_initialize;
  tool_class->control    = gimp_image_map_tool_control;

  color_tool_class->pick = gimp_image_map_tool_pick_color;

  klass->map    = NULL;
  klass->dialog = NULL;
  klass->reset  = NULL;
}

static void
gimp_image_map_tool_init (GimpImageMapTool *image_map_tool)
{
  GimpTool *tool = GIMP_TOOL (image_map_tool);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);

  image_map_tool->drawable   = NULL;
  image_map_tool->image_map  = NULL;
  image_map_tool->preview    = TRUE;

  image_map_tool->shell_desc = NULL;
  image_map_tool->shell      = NULL;
  image_map_tool->main_vbox  = NULL;
}

void
gimp_image_map_tool_preview (GimpImageMapTool *image_map_tool)
{
  GimpTool *tool;

  g_return_if_fail (GIMP_IS_IMAGE_MAP_TOOL (image_map_tool));

  tool = GIMP_TOOL (image_map_tool);

  if (image_map_tool->preview)
    {
      gimp_tool_control_set_preserve (tool->control, TRUE);

      gimp_image_map_tool_map (image_map_tool);

      gimp_tool_control_set_preserve (tool->control, FALSE);
    }
}

static void
gimp_image_map_tool_finalize (GObject *object)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (object);

  if (image_map_tool->shell)
    {
      gtk_widget_destroy (image_map_tool->shell);
      image_map_tool->shell     = NULL;
      image_map_tool->main_vbox = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define RESPONSE_RESET 1

static void
gimp_image_map_tool_initialize (GimpTool    *tool,
				GimpDisplay *gdisp)
{
  GimpImageMapTool *image_map_tool;
  GimpToolInfo     *tool_info;
  GimpDrawable     *drawable;

  image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);

  tool_info = tool->tool_info;

  /*  set gdisp so the dialog can be hidden on display destruction  */
  tool->gdisp = gdisp;

  if (! image_map_tool->shell)
    {
      GtkWidget   *shell;
      GtkWidget   *vbox;
      GtkWidget   *toggle;
      const gchar *stock_id;

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));

      image_map_tool->shell = shell =
        gimp_tool_dialog_new (tool_info,
                              gdisp->shell,
                              image_map_tool->shell_desc,

                              GIMP_STOCK_RESET, RESPONSE_RESET,
                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

      g_signal_connect_object (shell, "response",
                               G_CALLBACK (gimp_image_map_tool_response),
                               G_OBJECT (image_map_tool), 0);

      image_map_tool->main_vbox = vbox = gtk_vbox_new (FALSE, 4);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell)->vbox), vbox);

      /*  The preview toggle  */
      toggle = gtk_check_button_new_with_mnemonic (_("_Preview"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                    image_map_tool->preview);
      gtk_box_pack_end (GTK_BOX (image_map_tool->main_vbox), toggle,
                        FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_image_map_tool_preview_toggled),
                        image_map_tool);

      /*  Fill in subclass widgets  */
      gimp_image_map_tool_dialog (image_map_tool);

      gtk_widget_show (vbox);
    }

  drawable = gimp_image_active_drawable (gdisp->gimage);

  gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (image_map_tool->shell),
                                     GIMP_VIEWABLE (drawable));

  gtk_widget_show (image_map_tool->shell);

  image_map_tool->drawable  = drawable;
  image_map_tool->image_map = gimp_image_map_new (TRUE, drawable,
                                                  tool_info->blurb);

  g_signal_connect (image_map_tool->image_map, "flush",
                    G_CALLBACK (gimp_image_map_tool_flush),
                    image_map_tool);

  {
    GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (gdisp->shell);

    gimp_item_factory_update (shell->menubar_factory, shell);
    gimp_item_factory_update (shell->popup_factory,   shell);
  }
}

static void
gimp_image_map_tool_control (GimpTool       *tool,
			     GimpToolAction  action,
			     GimpDisplay    *gdisp)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (tool);

  switch (action)
    {
    case HALT:
      if (image_map_tool->shell)
        gtk_dialog_response (GTK_DIALOG (image_map_tool->shell),
                             GTK_RESPONSE_CANCEL);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static gboolean
gimp_image_map_tool_pick_color (GimpColorTool *color_tool,
                                gint           x,
                                gint           y,
                                GimpImageType *sample_type,
                                GimpRGB       *color,
                                gint          *color_index)
{
  GimpImageMapTool *tool = GIMP_IMAGE_MAP_TOOL (color_tool);

  *sample_type = gimp_drawable_type (tool->drawable);

  return gimp_image_pick_color_by_func (GIMP_OBJECT (tool->image_map), x, y,
                                        (GimpImagePickColorFunc) gimp_image_map_get_color_at,
                                        color_tool->options->sample_average,
                                        color_tool->options->average_radius,
                                        color, color_index);
}

static void
gimp_image_map_tool_map (GimpImageMapTool *image_map_tool)
{
  GIMP_IMAGE_MAP_TOOL_GET_CLASS (image_map_tool)->map (image_map_tool);
}

static void
gimp_image_map_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GIMP_IMAGE_MAP_TOOL_GET_CLASS (image_map_tool)->dialog (image_map_tool);
}

static void
gimp_image_map_tool_reset (GimpImageMapTool *image_map_tool)
{
  GIMP_IMAGE_MAP_TOOL_GET_CLASS (image_map_tool)->reset (image_map_tool);
}

static void
gimp_image_map_tool_flush (GimpImageMap     *image_map,
                           GimpImageMapTool *image_map_tool)
{
  GimpTool *tool = GIMP_TOOL (image_map_tool);

  gimp_display_flush_now (tool->gdisp);
}

static void
gimp_image_map_tool_response (GtkWidget        *widget,
                              gint              response_id,
                              GimpImageMapTool *image_map_tool)
{
  GimpDisplayShell *shell;
  GimpTool         *tool;

  tool  = GIMP_TOOL (image_map_tool);
  shell = GIMP_DISPLAY_SHELL (tool->gdisp->shell);

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_image_map_tool_reset (image_map_tool);
      gimp_image_map_tool_preview (image_map_tool);
      break;

    case GTK_RESPONSE_OK:
      gtk_widget_hide (image_map_tool->shell);

      gimp_tool_control_set_preserve (tool->control, TRUE);

      if (! image_map_tool->preview)
        {
          gimp_image_map_tool_map (image_map_tool);
        }

      if (image_map_tool->image_map)
        {
          gimp_image_map_commit (image_map_tool->image_map);
          image_map_tool->image_map = NULL;
        }

      gimp_tool_control_set_preserve (tool->control, FALSE);

      gimp_item_factory_update (shell->menubar_factory, shell);
      gimp_item_factory_update (shell->popup_factory,   shell);

      tool->gdisp    = NULL;
      tool->drawable = NULL;
      break;

    default:
      gtk_widget_hide (image_map_tool->shell);

      if (image_map_tool->image_map)
        {
          gimp_tool_control_set_preserve (tool->control, TRUE);

          gimp_image_map_abort (image_map_tool->image_map);
          image_map_tool->image_map = NULL;

          gimp_tool_control_set_preserve (tool->control, FALSE);

          gimp_image_flush (tool->gdisp->gimage);
        }

      tool->gdisp    = NULL;
      tool->drawable = NULL;
      break;
    }
}

static void
gimp_image_map_tool_preview_toggled (GtkWidget        *widget,
                                     GimpImageMapTool *image_map_tool)
{
  GimpTool *tool = GIMP_TOOL (image_map_tool);

  image_map_tool->preview = GTK_TOGGLE_BUTTON (widget)->active;

  if (image_map_tool->preview)
    {
      gimp_tool_control_set_preserve (tool->control, TRUE);

      gimp_image_map_tool_map (image_map_tool);

      gimp_tool_control_set_preserve (tool->control, FALSE);
    }
  else
    {
      if (image_map_tool->image_map)
	{
	  gimp_tool_control_set_preserve (tool->control, TRUE);

	  gimp_image_map_clear (image_map_tool->image_map);

	  gimp_tool_control_set_preserve (tool->control, FALSE);

	  gimp_image_flush (tool->gdisp->gimage);
	}
    }
}
