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

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontext.h"
#include "core/gimpedit.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "gimpdevices.h"
#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimpitemfactory.h"
#include "gimppreview.h"
#include "gimptoolbox.h"
#include "gimptoolbox-color-area.h"
#include "gimptoolbox-indicator-area.h"
#include "gtkhwrapbox.h"

#include "app_procs.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/default.xpm"
#include "pixmaps/swap.xpm"


#define DEFAULT_TOOL_ICON_SIZE GTK_ICON_SIZE_BUTTON


/*  local function prototypes  */

static void       gimp_toolbox_class_init       (GimpToolboxClass *klass);
static void       gimp_toolbox_init             (GimpToolbox      *toolbox);

static gboolean   gimp_toolbox_delete_event     (GtkWidget      *widget,
                                                 GdkEventAny    *event);
static void       gimp_toolbox_size_allocate    (GtkWidget      *widget,
                                                 GtkAllocation  *allocation);
static void       gimp_toolbox_style_set        (GtkWidget      *widget,
                                                 GtkStyle       *previous_style);

static void       toolbox_create_tools          (GimpToolbox    *toolbox,
                                                 GimpContext    *context);
static void       toolbox_create_color_area     (GimpToolbox    *toolbox,
                                                 GimpContext    *context);
static void       toolbox_create_indicator_area (GimpToolbox    *toolbox,
                                                 GimpContext    *context);

static void       toolbox_tool_changed          (GimpContext    *context,
                                                 GimpToolInfo   *tool_info,
                                                 gpointer        data);

static void       toolbox_tool_button_toggled   (GtkWidget      *widget,
                                                 gpointer        data);
static gboolean   toolbox_tool_button_press     (GtkWidget      *widget,
                                                 GdkEventButton *bevent,
                                                 GimpToolbox    *toolbox);

static gboolean   toolbox_check_device          (GtkWidget      *widget,
                                                 GdkEvent       *event,
                                                 gpointer        data);

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

static GimpDockClass *parent_class = NULL;

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


GType
gimp_toolbox_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GimpToolboxClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_toolbox_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpToolbox),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_toolbox_init,
      };

      type = g_type_register_static (GIMP_TYPE_DOCK,
                                     "GimpToolbox",
                                     &type_info, 0);
    }

  return type;
}

static void
gimp_toolbox_class_init (GimpToolboxClass *klass)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->delete_event  = gimp_toolbox_delete_event;
  widget_class->size_allocate = gimp_toolbox_size_allocate;
  widget_class->style_set     = gimp_toolbox_style_set;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tool_icon_size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_TOOL_ICON_SIZE,
                                                              G_PARAM_READABLE));
}

static void
gimp_toolbox_init (GimpToolbox *toolbox)
{
  GimpItemFactory *toolbox_factory;
  GtkWidget       *main_vbox;
  GtkWidget       *vbox;

  gtk_window_set_wmclass (GTK_WINDOW (toolbox), "toolbox", "Gimp");
  gtk_window_set_title (GTK_WINDOW (toolbox), _("The GIMP"));
  gtk_window_set_resizable (GTK_WINDOW (toolbox), TRUE);

  main_vbox = GIMP_DOCK (toolbox)->main_vbox;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_vbox), vbox, 0);
  gtk_widget_show (vbox);

  toolbox_factory = gimp_item_factory_from_path ("<Toolbox>");

  toolbox->menu_bar = GTK_ITEM_FACTORY (toolbox_factory)->widget;
  gtk_box_pack_start (GTK_BOX (vbox), toolbox->menu_bar, FALSE, FALSE, 0);
  gtk_widget_show (toolbox->menu_bar);

  gtk_window_add_accel_group (GTK_WINDOW (toolbox),
                              GTK_ITEM_FACTORY (toolbox_factory)->accel_group);

  /*  Connect the "F1" help key  */
  gimp_help_connect (GTK_WIDGET (toolbox),
		     gimp_standard_help_func,
		     "toolbox/toolbox.html");

  toolbox->wbox = gtk_hwrap_box_new (FALSE);
  gtk_wrap_box_set_justify (GTK_WRAP_BOX (toolbox->wbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify (GTK_WRAP_BOX (toolbox->wbox), GTK_JUSTIFY_LEFT);

  gtk_box_pack_start (GTK_BOX (vbox), toolbox->wbox, FALSE, FALSE, 0);
  gtk_widget_show (toolbox->wbox);
}

static gboolean
gimp_toolbox_delete_event (GtkWidget   *widget,
                           GdkEventAny *event)
{
  app_exit (FALSE);

  return TRUE;
}

static void
gimp_toolbox_size_allocate (GtkWidget     *widget,
                            GtkAllocation *allocation)
{
  Gimp         *gimp;
  GimpToolInfo *tool_info;
  GtkWidget    *tool_button;

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (! GIMP_DOCK (widget)->context)
    return;

  gimp = GIMP_DOCK (widget)->context->gimp;

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (gimp->tool_info_list,
                                      "gimp:rect_select_tool");
  tool_button = g_object_get_data (G_OBJECT (tool_info), "toolbox-button");

  if (tool_button)
    {
      GimpToolbox    *toolbox;
      GtkRequisition  button_requisition;
      GtkRequisition  color_requisition;
      GtkRequisition  indicator_requisition;
      gint            n_tools;
      gint            tool_rows;
      gint            tool_columns;

      toolbox = GIMP_TOOLBOX (widget);

      gtk_widget_size_request (tool_button,             &button_requisition);
      gtk_widget_size_request (toolbox->color_area,     &color_requisition);
      gtk_widget_size_request (toolbox->indicator_area, &indicator_requisition);

      n_tools = gimp_container_num_children (gimp->tool_info_list);

      tool_columns = MAX (1, (allocation->width /
                              button_requisition.width));

      tool_rows = n_tools / tool_columns;

      if (n_tools % tool_columns)
        tool_rows++;

      if (toolbox->tool_rows    != tool_rows  ||
          toolbox->tool_columns != tool_columns)
        {
          toolbox->tool_rows    = tool_rows;
          toolbox->tool_columns = tool_columns;

          if ((tool_columns * button_requisition.width) >=
              (color_requisition.width + indicator_requisition.width))
            {
              gtk_widget_set_size_request (toolbox->wbox, -1,
                                           tool_rows *
                                           button_requisition.height +
                                           MAX (color_requisition.height,
                                                indicator_requisition.height));
            }
          else
            {
              gtk_widget_set_size_request (toolbox->wbox, -1,
                                           tool_rows *
                                           button_requisition.height +
                                           color_requisition.height  +
                                           indicator_requisition.height);
            }
        }
    }
}

static void
gimp_toolbox_style_set (GtkWidget *widget,
                        GtkStyle  *previous_style)
{
  Gimp         *gimp;
  GimpToolInfo *tool_info;
  GtkWidget    *tool_button;
  GtkIconSize   tool_icon_size;
  GList        *list;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);

  if (! GIMP_DOCK (widget)->context)
    return;

  gimp = GIMP_DOCK (widget)->context->gimp;

  gtk_widget_style_get (widget,
                        "tool_icon_size", &tool_icon_size,
                        NULL);

  for (list = GIMP_LIST (gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      tool_info = GIMP_TOOL_INFO (list->data);

      tool_button = g_object_get_data (G_OBJECT (tool_info), "toolbox-button");

      if (tool_button)
        {
          GtkImage *image;
          gchar    *stock_id;

          image = GTK_IMAGE (GTK_BIN (tool_button)->child);

          gtk_image_get_stock (image, &stock_id, NULL);
          gtk_image_set_from_stock (image, stock_id, tool_icon_size);
        }
    }

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (gimp->tool_info_list,
                                      "gimp:rect_select_tool");
  tool_button = g_object_get_data (G_OBJECT (tool_info), "toolbox-button");

  if (tool_button)
    {
      GimpToolbox    *toolbox;
      GtkWidget      *main_vbox;
      GtkRequisition  menubar_requisition;
      GtkRequisition  button_requisition;
      GtkRequisition  color_requisition;
      GtkRequisition  indicator_requisition;
      gint            border_width;
      gint            spacing;
      gint            separator_height;
      GdkGeometry     geometry;

      toolbox   = GIMP_TOOLBOX (widget);
      main_vbox = GIMP_DOCK (widget)->main_vbox;

      gtk_widget_size_request (toolbox->menu_bar,       &menubar_requisition);
      gtk_widget_size_request (tool_button,             &button_requisition);
      gtk_widget_size_request (toolbox->color_area,     &color_requisition);
      gtk_widget_size_request (toolbox->indicator_area, &indicator_requisition);

      border_width = gtk_container_get_border_width (GTK_CONTAINER (main_vbox));

      spacing = gtk_box_get_spacing (GTK_BOX (main_vbox));

      gtk_widget_style_get (widget,
                            "separator_height", &separator_height,
                            NULL);

      geometry.min_width  = (2 * border_width +
                             2 * button_requisition.width);
      geometry.min_height = (2 * border_width +
                             spacing          +
                             separator_height +
                             button_requisition.height  +
                             menubar_requisition.height +
                             MAX (color_requisition.height,
                                  indicator_requisition.height));
      geometry.width_inc  = button_requisition.width;
      geometry.height_inc = button_requisition.height;

      gtk_window_set_geometry_hints (GTK_WINDOW (widget), 
                                     NULL,
                                     &geometry, 
                                     GDK_HINT_MIN_SIZE | GDK_HINT_RESIZE_INC);
    }
}

GtkWidget *
gimp_toolbox_new (GimpDialogFactory *dialog_factory,
                  Gimp              *gimp)
{
  GimpContext *context;
  GimpToolbox *toolbox;
  GList       *list;

  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  context = gimp_get_user_context (gimp);

  toolbox = g_object_new (GIMP_TYPE_TOOLBOX, NULL);

  gimp_dock_construct (GIMP_DOCK (toolbox), dialog_factory, context, FALSE);

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
      g_signal_connect (G_OBJECT (toolbox), "motion_notify_event",
			G_CALLBACK (toolbox_check_device),
			gimp);

      gtk_widget_set_events (GTK_WIDGET (toolbox), GDK_POINTER_MOTION_MASK);
      gtk_widget_set_extension_events (GTK_WIDGET (toolbox),
                                       GDK_EXTENSION_EVENTS_CURSOR);
    }

  toolbox_create_tools (toolbox, context);
  toolbox_create_color_area (toolbox, context);
  toolbox_create_indicator_area (toolbox, context);

  g_signal_connect_object (G_OBJECT (context), "tool_changed",
			   G_CALLBACK (toolbox_tool_changed),
			   G_OBJECT (toolbox->wbox),
			   0);

  gtk_drag_dest_set (toolbox->wbox,
		     GTK_DEST_DEFAULT_ALL,
		     toolbox_target_table,
                     G_N_ELEMENTS (toolbox_target_table),
		     GDK_ACTION_COPY);

  gimp_dnd_file_dest_set (toolbox->wbox, gimp_dnd_open_files, NULL);

  gimp_dnd_viewable_dest_set (toolbox->wbox, GIMP_TYPE_LAYER,
			      toolbox_drop_drawable,
			      context);
  gimp_dnd_viewable_dest_set (toolbox->wbox, GIMP_TYPE_LAYER_MASK,
			      toolbox_drop_drawable,
			      context);
  gimp_dnd_viewable_dest_set (toolbox->wbox, GIMP_TYPE_CHANNEL,
			      toolbox_drop_drawable,
			      context);
  gimp_dnd_viewable_dest_set (toolbox->wbox, GIMP_TYPE_TOOL_INFO,
			      toolbox_drop_tool,
			      context);
  gimp_dnd_viewable_dest_set (toolbox->wbox, GIMP_TYPE_BUFFER,
			      toolbox_drop_buffer,
			      context);

  gimp_toolbox_style_set (GTK_WIDGET (toolbox), GTK_WIDGET (toolbox)->style);

  return GTK_WIDGET (toolbox);
}


/*  private functions  */

static void
toolbox_create_tools (GimpToolbox *toolbox,
                      GimpContext *context)
{
  GimpToolInfo *active_tool;
  GList        *list;
  GSList       *group = NULL;

  active_tool = gimp_context_get_tool (context);

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
      gtk_wrap_box_pack (GTK_WRAP_BOX (toolbox->wbox), button,
			 FALSE, FALSE, FALSE, FALSE);
      gtk_widget_show (button);

      g_object_set_data (G_OBJECT (tool_info), "toolbox-button", button);

      image = gtk_image_new_from_stock (tool_info->stock_id,
					GTK_ICON_SIZE_BUTTON);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      if (tool_info == active_tool)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      g_signal_connect (G_OBJECT (button), "toggled",
			G_CALLBACK (toolbox_tool_button_toggled),
			tool_info);

      g_signal_connect (G_OBJECT (button), "button_press_event",
			G_CALLBACK (toolbox_tool_button_press),
                        toolbox);

      gimp_help_set_help_data (button,
			       tool_info->help,
			       tool_info->help_data);

    }
}

static void
toolbox_create_color_area (GimpToolbox *toolbox,
                           GimpContext *context)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *col_area;
  GdkPixmap *default_pixmap;
  GdkBitmap *default_mask;
  GdkPixmap *swap_pixmap;
  GdkBitmap *swap_mask;

  if (! GTK_WIDGET_REALIZED (toolbox->wbox))
    gtk_widget_realize (toolbox->wbox);

  default_pixmap = gdk_pixmap_create_from_xpm_d (toolbox->wbox->window,
                                                 &default_mask,
                                                 &toolbox->wbox->style->bg[GTK_STATE_NORMAL],
                                                 default_xpm);
  swap_pixmap = gdk_pixmap_create_from_xpm_d (toolbox->wbox->window,
					      &swap_mask,
					      &toolbox->wbox->style->bg[GTK_STATE_NORMAL],
					      swap_xpm);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_wrap_box_pack_wrapped (GTK_WRAP_BOX (toolbox->wbox), frame,
			     TRUE, TRUE, FALSE, TRUE, TRUE);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  gimp_help_set_help_data (alignment, NULL, "#color_area");

  col_area = gimp_toolbox_color_area_create (toolbox,
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

  toolbox->color_area = frame;
}

static void
toolbox_create_indicator_area (GimpToolbox *toolbox,
                               GimpContext *context)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *ind_area;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_wrap_box_pack (GTK_WRAP_BOX (toolbox->wbox), frame,
                     TRUE, TRUE, FALSE, TRUE);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  gimp_help_set_help_data (alignment, NULL, "#indicator_area");

  ind_area = gimp_toolbox_indicator_area_create (toolbox);
  gtk_container_add (GTK_CONTAINER (alignment), ind_area);

  gtk_widget_show (ind_area);
  gtk_widget_show (alignment);
  gtk_widget_show (frame);

  toolbox->indicator_area = frame;
}

static void
toolbox_tool_changed (GimpContext  *context,
		      GimpToolInfo *tool_info,
		      gpointer      data)
{
  if (tool_info)
    {
      GtkWidget *toolbox_button;

      toolbox_button = g_object_get_data (G_OBJECT (tool_info),
                                          "toolbox-button");

      if (toolbox_button && ! GTK_TOGGLE_BUTTON (toolbox_button)->active)
	{
	  g_signal_handlers_block_by_func (G_OBJECT (toolbox_button),
					   toolbox_tool_button_toggled,
					   tool_info);

	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbox_button),
                                        TRUE);

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
			   GimpToolbox    *toolbox)
{
  if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1))
    {
      gimp_dialog_factory_dialog_raise (GIMP_DOCK (toolbox)->dialog_factory,
                                        "gimp:tool-options",
                                        -1);
    }

  return FALSE;
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
