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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontext.h"
#include "core/gimpedit.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpdevices.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdnd.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimppreview.h"
#include "widgets/gtkhwrapbox.h"

#include "color-area.h"
#include "dialogs.h"
#include "indicator-area.h"

#include "app_procs.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/default.xpm"
#include "pixmaps/swap.xpm"


/*  local function prototypes  */

static void       toolbox_create_tools          (GtkWidget      *wbox,
                                                 GimpContext    *context);
static void       toolbox_create_color_area     (GtkWidget      *wbox,
                                                 GimpContext    *context);
static void       toolbox_create_indicator_area (GtkWidget      *wbox,
                                                 GimpContext    *context);

static void       toolbox_tool_changed          (GimpContext    *context,
                                                 GimpToolInfo   *tool_info,
                                                 gpointer        data);

static void       toolbox_tool_button_toggled   (GtkWidget      *widget,
                                                 gpointer        data);
static gboolean   toolbox_tool_button_press     (GtkWidget      *widget,
                                                 GdkEventButton *bevent,
                                                 gpointer        data);

static gboolean   toolbox_delete                (GtkWidget      *widget,
                                                 GdkEvent       *event,
                                                 gpointer        data);
static gboolean   toolbox_check_device          (GtkWidget      *widget,
                                                 GdkEvent       *event,
                                                 gpointer        data);
static void       toolbox_style_set             (GtkWidget      *window,
                                                 GtkStyle       *previous_style,
                                                 Gimp           *gimp);

static void       toolbox_drop_drawable         (GtkWidget      *widget,
                                                 GimpViewable   *viewable,
                                                 gpointer        data);
static void       toolbox_drop_tool             (GtkWidget      *widget,
                                                 GimpViewable   *viewable,
                                                 gpointer        data);
static void       toolbox_drop_buffer           (GtkWidget      *widget,
                                                 GimpViewable   *viewable,
                                                 gpointer        data);


/*  local variables  */
static GtkWidget *toolbox_shell = NULL;

static GtkTargetEntry toolbox_target_table[] =
{
  GIMP_TARGET_URI_LIST,
  GIMP_TARGET_TEXT_PLAIN,
  GIMP_TARGET_NETSCAPE_URL,
  GIMP_TARGET_LAYER,
  GIMP_TARGET_CHANNEL,
  GIMP_TARGET_LAYER_MASK,
  GIMP_TARGET_TOOL,
  GIMP_TARGET_BUFFER
};


/*  public functions  */

GtkWidget *
toolbox_create (Gimp *gimp)
{
  GimpContext     *context;
  GimpItemFactory *toolbox_factory;
  GtkWidget       *window;
  GtkWidget       *main_vbox;
  GtkWidget       *wbox;
  GList           *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  context = gimp_get_user_context (gimp);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_wmclass (GTK_WINDOW (window), "toolbox", "Gimp");
  gtk_window_set_title (GTK_WINDOW (window), _("The GIMP"));
  gtk_window_set_resizable (GTK_WINDOW (window), TRUE);

  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (toolbox_delete),
		    NULL);

  g_signal_connect (G_OBJECT (window), "style_set",
		    G_CALLBACK (toolbox_style_set),
		    gimp);

  /* We need to know when the current device changes, so we can update
   * the correct tool - to do this we connect to motion events.
   * We can't just use EXTENSION_EVENTS_CURSOR though, since that
   * would get us extension events for the mouse pointer, and our
   * device would change to that and not change back. So we check
   * manually that all devices have a cursor, before establishing the check.
   */
  for (list = gdk_devices_list (); list; list = g_list_next (list))
    {
      if (! ((GdkDevice *) (list->data))->has_cursor)
	break;
    }

  if (! list)  /* all devices have cursor */
    {
      g_signal_connect (G_OBJECT (window), "motion_notify_event",
			G_CALLBACK (toolbox_check_device),
			gimp);

      gtk_widget_set_events (window, GDK_POINTER_MOTION_MASK);
      gtk_widget_set_extension_events (window, GDK_EXTENSION_EVENTS_CURSOR);
    }

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
  gtk_widget_show (main_vbox);

  toolbox_factory = gimp_item_factory_from_path ("<Toolbox>");
  gtk_box_pack_start (GTK_BOX (main_vbox),
                      GTK_ITEM_FACTORY (toolbox_factory)->widget,
		      FALSE, FALSE, 0);
  gtk_widget_show (GTK_ITEM_FACTORY (toolbox_factory)->widget);

  gtk_window_add_accel_group (GTK_WINDOW (window),
                              GTK_ITEM_FACTORY (toolbox_factory)->accel_group);

  /*  Connect the "F1" help key  */
  gimp_help_connect (window,
		     gimp_standard_help_func,
		     "toolbox/toolbox.html");

  wbox = gtk_hwrap_box_new (FALSE);
  gtk_wrap_box_set_justify (GTK_WRAP_BOX (wbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify (GTK_WRAP_BOX (wbox), GTK_JUSTIFY_LEFT);
  /*  magic number to set a default 5x5 layout  */
  gtk_wrap_box_set_aspect_ratio (GTK_WRAP_BOX (wbox), 5.0 / 5.9);
  gtk_box_pack_start (GTK_BOX (main_vbox), wbox, TRUE, TRUE, 0);
  gtk_widget_show (wbox);

  toolbox_create_tools (wbox, context);

  g_signal_connect_object (G_OBJECT (context), "tool_changed",
			   G_CALLBACK (toolbox_tool_changed),
			   G_OBJECT (wbox),
			   0);

  toolbox_create_color_area (wbox, context);

  if (gimprc.show_indicators)
    toolbox_create_indicator_area (wbox, context);

  gtk_drag_dest_set (window,
		     GTK_DEST_DEFAULT_ALL,
		     toolbox_target_table,
                     G_N_ELEMENTS (toolbox_target_table),
		     GDK_ACTION_COPY);

  gimp_dnd_file_dest_set (window, gimp_dnd_open_files, NULL);

  gimp_dnd_viewable_dest_set (window, GIMP_TYPE_LAYER,
			      toolbox_drop_drawable,
			      context);
  gimp_dnd_viewable_dest_set (window, GIMP_TYPE_LAYER_MASK,
			      toolbox_drop_drawable,
			      context);
  gimp_dnd_viewable_dest_set (window, GIMP_TYPE_CHANNEL,
			      toolbox_drop_drawable,
			      context);
  gimp_dnd_viewable_dest_set (window, GIMP_TYPE_TOOL_INFO,
			      toolbox_drop_tool,
			      context);
  gimp_dnd_viewable_dest_set (window, GIMP_TYPE_BUFFER,
			      toolbox_drop_buffer,
			      context);

  toolbox_style_set (window, NULL, gimp);

  gtk_widget_show (window);

  toolbox_shell = window;

  return toolbox_shell;
}

void
toolbox_free (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gtk_widget_destroy (toolbox_shell);
}


/*  private functions  */

static void
toolbox_create_tools (GtkWidget   *wbox,
                      GimpContext *context)
{
  GList  *list;
  GSList *group = NULL;

  for (list = GIMP_LIST (context->gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info;
      GtkWidget    *button;
      GtkWidget    *image;

      tool_info = (GimpToolInfo *) list->data;

      button = gtk_radio_button_new (group);
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_wrap_box_pack (GTK_WRAP_BOX (wbox), button,
			 FALSE, FALSE, FALSE, FALSE);
      gtk_widget_show (button);

      g_object_set_data (G_OBJECT (tool_info), "toolbox-button", button);

      image = gtk_image_new_from_stock (tool_info->stock_id,
					GTK_ICON_SIZE_BUTTON);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      g_signal_connect (G_OBJECT (button), "toggled",
			G_CALLBACK (toolbox_tool_button_toggled),
			tool_info);

      g_signal_connect (G_OBJECT (button), "button_press_event",
			G_CALLBACK (toolbox_tool_button_press),
			tool_info);

      gimp_help_set_help_data (button,
			       tool_info->help,
			       tool_info->help_data);

    }
}

static void
toolbox_create_color_area (GtkWidget   *wbox,
                           GimpContext *context)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *col_area;
  GdkPixmap *default_pixmap;
  GdkBitmap *default_mask;
  GdkPixmap *swap_pixmap;
  GdkBitmap *swap_mask;

  if (! GTK_WIDGET_REALIZED (wbox))
    gtk_widget_realize (wbox);

  default_pixmap = gdk_pixmap_create_from_xpm_d (wbox->window,
                                                 &default_mask,
                                                 &wbox->style->bg[GTK_STATE_NORMAL],
                                                 default_xpm);
  swap_pixmap = gdk_pixmap_create_from_xpm_d (wbox->window,
					      &swap_mask,
					      &wbox->style->bg[GTK_STATE_NORMAL],
					      swap_xpm);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_wrap_box_pack_wrapped (GTK_WRAP_BOX (wbox), frame,
			     TRUE, TRUE, TRUE, TRUE, TRUE);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  gimp_help_set_help_data (alignment, NULL, "#color_area");

  col_area = color_area_create (context,
                                54, 42,
				default_pixmap, default_mask,
				swap_pixmap, swap_mask);
  gtk_container_add (GTK_CONTAINER (alignment), col_area);
  gimp_help_set_help_data
    (col_area,
     _("Foreground & background colors.  The black "
       "and white squares reset colors.  The arrows swap colors. Double "
       "click to select a color from a colorrequester."), NULL);

  gtk_widget_show (col_area);
  gtk_widget_show (alignment);
  gtk_widget_show (frame);

  g_object_set_data (G_OBJECT (wbox), "color-area", frame);
}

static void
toolbox_create_indicator_area (GtkWidget   *wbox,
                               GimpContext *context)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *ind_area;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_wrap_box_pack (GTK_WRAP_BOX (wbox), frame, TRUE, TRUE, TRUE, TRUE);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  gimp_help_set_help_data (alignment, NULL, "#indicator_area");

  ind_area = indicator_area_create (context);
  gtk_container_add (GTK_CONTAINER (alignment), ind_area);

  gtk_widget_show (ind_area);
  gtk_widget_show (alignment);
  gtk_widget_show (frame);

  g_object_set_data (G_OBJECT (wbox), "indicator-area", frame);
}

static void
toolbox_tool_changed (GimpContext  *context,
		      GimpToolInfo *tool_info,
		      gpointer      data)
{
  if (tool_info)
    {
      GtkWidget *toolbox_button;

      toolbox_button =
	g_object_get_data (G_OBJECT (tool_info), "toolbox-button");

      if (toolbox_button && ! GTK_TOGGLE_BUTTON (toolbox_button)->active)
	{
	  g_signal_handlers_block_by_func (G_OBJECT (toolbox_button),
					   toolbox_tool_button_toggled,
					   tool_info);

	  gtk_widget_activate (toolbox_button);

	  g_signal_handlers_unblock_by_func (G_OBJECT (toolbox_button),
					     toolbox_tool_button_toggled,
					     tool_info);
	}
    }
}

static void
toolbox_tool_button_toggled (GtkWidget *widget,
			     gpointer   data)
{
  GimpToolInfo *tool_info;

  tool_info = GIMP_TOOL_INFO (data);

  if ((tool_info) && GTK_TOGGLE_BUTTON (widget)->active)
    gimp_context_set_tool (gimp_get_user_context (tool_info->gimp), tool_info);
}

static gboolean
toolbox_tool_button_press (GtkWidget      *widget,
			   GdkEventButton *event,
			   gpointer        data)
{
  if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
    {
      gimp_dialog_factory_dialog_new (global_dialog_factory,
				      "gimp:tool-options-dialog", -1);
    }

  return FALSE;
}

static gboolean
toolbox_delete (GtkWidget *widget,
		GdkEvent  *event,
		gpointer   data)
{
  app_exit (FALSE);

  return TRUE;
}

static gboolean
toolbox_check_device (GtkWidget *widget,
		      GdkEvent  *event,
		      gpointer   data)
{
  gimp_devices_check_change (GIMP (data), event);

  return FALSE;
}

static void
toolbox_style_set (GtkWidget *window,
                   GtkStyle  *previous_style,
                   Gimp      *gimp)
{
  GimpToolInfo *tool_info;
  GtkWidget    *tool_button;

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (gimp->tool_info_list,
                                      "gimp:rect_select_tool");
  tool_button = g_object_get_data (G_OBJECT (tool_info), "toolbox-button");

  if (tool_button)
    {
      GtkWidget      *wbox;
      GtkWidget      *widget;
      GtkRequisition  menubar_requisition;
      GtkRequisition  button_requisition;
      GtkRequisition  color_requisition;
      GtkRequisition  indicator_requisition;
      GdkGeometry     geometry;
      gint            border_width;
      GList          *children;

      children =
        gtk_container_get_children (GTK_CONTAINER (GTK_BIN (window)->child));

      gtk_widget_size_request (g_list_nth_data (children, 0),
                               &menubar_requisition);

      wbox = g_list_nth_data (children, 1);

      g_list_free (children);

      gtk_widget_size_request (tool_button, &button_requisition);

      widget = g_object_get_data (G_OBJECT (wbox), "color-area");
      gtk_widget_size_request (widget, &color_requisition);

      widget = g_object_get_data (G_OBJECT (wbox), "indicator-area");
      gtk_widget_size_request (widget, &indicator_requisition);

      border_width =
        gtk_container_get_border_width (GTK_CONTAINER (GTK_BIN (window)->child));

      geometry.min_width  = (2 * border_width +
                             2 * button_requisition.width);
      geometry.min_height = (2 * border_width +
                             button_requisition.height +
                             menubar_requisition.height +
                             MAX (color_requisition.height,
                                  indicator_requisition.height));
      geometry.width_inc  = button_requisition.width;
      geometry.height_inc = button_requisition.height;

      gtk_window_set_geometry_hints (GTK_WINDOW (window), 
                                     NULL,
                                     &geometry, 
                                     GDK_HINT_MIN_SIZE | GDK_HINT_RESIZE_INC);
    }
}

static void
toolbox_drop_drawable (GtkWidget    *widget,
		       GimpViewable *viewable,
		       gpointer      data)
{
  GimpDrawable      *drawable;
  GimpImage         *gimage;
  GimpImage         *new_gimage;
  GimpLayer         *new_layer;
  gint               width, height;
  gint               off_x, off_y;
  gint               bytes;
  GimpImageBaseType  type;

  drawable = GIMP_DRAWABLE (viewable);

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));
  width  = gimp_drawable_width  (drawable);
  height = gimp_drawable_height (drawable);
  bytes  = gimp_drawable_bytes  (drawable);

  type = GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable));

  new_gimage = gimp_create_image (gimage->gimp,
				  width, height,
				  type,
				  FALSE);
  gimp_image_undo_disable (new_gimage);

  if (type == GIMP_INDEXED) /* copy the colormap */
    {
      new_gimage->num_cols = gimage->num_cols;
      memcpy (new_gimage->cmap, gimage->cmap, COLORMAP_SIZE);
    }

  gimp_image_set_resolution (new_gimage,
			     gimage->xresolution, gimage->yresolution);
  gimp_image_set_unit (new_gimage, gimage->unit);

  if (GIMP_IS_LAYER (drawable))
    {
      new_layer = gimp_layer_copy (GIMP_LAYER (drawable),
                                   G_TYPE_FROM_INSTANCE (drawable),
                                   FALSE);
    }
  else
    {
      new_layer = GIMP_LAYER (gimp_drawable_copy (drawable,
                                                  GIMP_TYPE_LAYER,
                                                  TRUE));
    }

  gimp_item_set_image (GIMP_ITEM (new_layer), new_gimage);

  gimp_object_set_name (GIMP_OBJECT (new_layer),
			gimp_object_get_name (GIMP_OBJECT (drawable)));

  gimp_drawable_offsets (GIMP_DRAWABLE (new_layer), &off_x, &off_y);
  gimp_layer_translate (new_layer, -off_x, -off_y);

  gimp_image_add_layer (new_gimage, new_layer, 0);

  gimp_image_undo_enable (new_gimage);

  gimp_create_display (gimage->gimp, new_gimage, 0x0101);

  g_object_unref (G_OBJECT (new_gimage));
}

static void
toolbox_drop_tool (GtkWidget    *widget,
		   GimpViewable *viewable,
		   gpointer      data)
{
  GimpContext *context;

  context = (GimpContext *) data;

  gimp_context_set_tool (context, GIMP_TOOL_INFO (viewable));
}

static void
toolbox_drop_buffer (GtkWidget    *widget,
		     GimpViewable *viewable,
		     gpointer      data)
{
  GimpContext *context;

  context = (GimpContext *) data;

  if (context->gimp->busy)
    return;

  gimp_edit_paste_as_new (context->gimp, NULL, GIMP_BUFFER (viewable));
}
