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
#include "core/gimpimage-colormap.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "gimpdevices.h"
#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimpitemfactory.h"
#include "gimptoolbox.h"
#include "gimptoolbox-color-area.h"
#include "gimptoolbox-indicator-area.h"
#include "gtkhwrapbox.h"

#include "gimp-intl.h"


#define DEFAULT_TOOL_ICON_SIZE GTK_ICON_SIZE_BUTTON
#define DEFAULT_BUTTON_RELIEF  GTK_RELIEF_NONE


/*  local function prototypes  */

static void       gimp_toolbox_class_init       (GimpToolboxClass *klass);
static void       gimp_toolbox_init             (GimpToolbox      *toolbox);

static gboolean   gimp_toolbox_delete_event     (GtkWidget      *widget,
                                                 GdkEventAny    *event);
static void       gimp_toolbox_size_allocate    (GtkWidget      *widget,
                                                 GtkAllocation  *allocation);
static void       gimp_toolbox_style_set        (GtkWidget      *widget,
                                                 GtkStyle       *previous_style);
static void       gimp_toolbox_book_added       (GimpDock       *dock,
                                                 GimpDockbook   *dockbook);
static void       gimp_toolbox_book_removed     (GimpDock       *dock,
                                                 GimpDockbook   *dockbook);
static void       gimp_toolbox_set_geometry     (GimpToolbox    *toolbox);

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
                                                 GimpToolInfo   *tool_info);
static gboolean   toolbox_tool_button_press     (GtkWidget      *widget,
                                                 GdkEventButton *bevent,
                                                 GimpToolbox    *toolbox);

static gboolean   toolbox_check_device          (GtkWidget      *widget,
                                                 GdkEvent       *event,
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

static GimpDockClass *parent_class = NULL;


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
  GimpDockClass  *dock_class;

  widget_class = GTK_WIDGET_CLASS (klass);
  dock_class   = GIMP_DOCK_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->delete_event  = gimp_toolbox_delete_event;
  widget_class->size_allocate = gimp_toolbox_size_allocate;
  widget_class->style_set     = gimp_toolbox_style_set;

  dock_class->book_added      = gimp_toolbox_book_added;
  dock_class->book_removed    = gimp_toolbox_book_removed;

  gtk_widget_class_install_style_property
    (widget_class,
     g_param_spec_enum ("tool_icon_size",
                        NULL, NULL,
                        GTK_TYPE_ICON_SIZE,
                        DEFAULT_TOOL_ICON_SIZE,
                        G_PARAM_READABLE));

  gtk_widget_class_install_style_property
    (widget_class,
     g_param_spec_enum ("button_relief",
                        NULL, NULL,
                        GTK_TYPE_RELIEF_STYLE,
                        DEFAULT_BUTTON_RELIEF,
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

  /* Docks are utility windows by default, but the toolbox doesn't fit
   *  into this category.  'Normal' is not correct as well but there
   *  doesn't seem to be a better match :-(
   */
  gtk_window_set_type_hint (GTK_WINDOW (toolbox), GDK_WINDOW_TYPE_HINT_NORMAL);

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
  gimp_exit (GIMP_DOCK (widget)->context->gimp, FALSE);

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
                                      "gimp-rect-select-tool");
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
  Gimp           *gimp;
  GimpToolInfo   *tool_info;
  GtkWidget      *tool_button;
  GtkIconSize     tool_icon_size;
  GtkReliefStyle  relief;
  GList          *list;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);

  if (! GIMP_DOCK (widget)->context)
    return;

  gimp = GIMP_DOCK (widget)->context->gimp;

  gtk_widget_style_get (widget,
                        "tool_icon_size", &tool_icon_size,
                        "button_relief",  &relief,
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

          gtk_button_set_relief (GTK_BUTTON (tool_button), relief);
        }
    }

  gimp_toolbox_set_geometry (GIMP_TOOLBOX (widget));
}

static void
gimp_toolbox_book_added (GimpDock     *dock,
                         GimpDockbook *dockbook)
{
  if (g_list_length (dock->dockbooks) == 1)
    gimp_toolbox_set_geometry (GIMP_TOOLBOX (dock));
}

static void
gimp_toolbox_book_removed (GimpDock     *dock,
                           GimpDockbook *dockbook)
{
  if (dock->dockbooks == NULL &&
      ! (GTK_OBJECT_FLAGS (dock) & GTK_IN_DESTRUCTION))
    gimp_toolbox_set_geometry (GIMP_TOOLBOX (dock));
}

static void
gimp_toolbox_set_geometry (GimpToolbox *toolbox)
{
  Gimp         *gimp;
  GimpToolInfo *tool_info;
  GtkWidget    *tool_button;

  gimp = GIMP_DOCK (toolbox)->context->gimp;

  tool_info = (GimpToolInfo *)
    gimp_container_get_child_by_name (gimp->tool_info_list,
                                      "gimp-rect-select-tool");
  tool_button = g_object_get_data (G_OBJECT (tool_info), "toolbox-button");

  if (tool_button)
    {
      GtkWidget      *main_vbox;
      GtkRequisition  menubar_requisition;
      GtkRequisition  button_requisition;
      GtkRequisition  color_requisition;
      GtkRequisition  indicator_requisition;
      gint            border_width;
      gint            spacing;
      gint            separator_height;
      GdkGeometry     geometry;

      main_vbox = GIMP_DOCK (toolbox)->main_vbox;

      gtk_widget_size_request (toolbox->menu_bar,       &menubar_requisition);
      gtk_widget_size_request (tool_button,             &button_requisition);
      gtk_widget_size_request (toolbox->color_area,     &color_requisition);
      gtk_widget_size_request (toolbox->indicator_area, &indicator_requisition);

      border_width = gtk_container_get_border_width (GTK_CONTAINER (main_vbox));

      spacing = gtk_box_get_spacing (GTK_BOX (main_vbox));

      gtk_widget_style_get (GTK_WIDGET (toolbox),
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
      geometry.height_inc = (GIMP_DOCK (toolbox)->dockbooks ?
                             1 : button_requisition.height);

      gtk_window_set_geometry_hints (GTK_WINDOW (toolbox),
                                     NULL,
                                     &geometry, 
                                     GDK_HINT_MIN_SIZE   |
                                     GDK_HINT_RESIZE_INC |
                                     GDK_HINT_USER_POS);
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

  gimp_dock_construct (GIMP_DOCK (toolbox), dialog_factory, context);

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
      g_signal_connect (toolbox, "motion_notify_event",
			G_CALLBACK (toolbox_check_device),
			gimp);

      gtk_widget_add_events (GTK_WIDGET (toolbox), GDK_POINTER_MOTION_MASK);
      gtk_widget_set_extension_events (GTK_WIDGET (toolbox),
                                       GDK_EXTENSION_EVENTS_CURSOR);
    }

  toolbox_create_tools (toolbox, context);
  toolbox_create_color_area (toolbox, context);
  toolbox_create_indicator_area (toolbox, context);

  g_signal_connect_object (context, "tool_changed",
			   G_CALLBACK (toolbox_tool_changed),
			   toolbox->wbox,
			   0);

  gimp_dnd_file_dest_add (GTK_WIDGET (toolbox), gimp_dnd_open_files, NULL);

  gimp_dnd_file_dest_add (toolbox->wbox, gimp_dnd_open_files, NULL);

  gimp_dnd_viewable_dest_add (toolbox->wbox, GIMP_TYPE_LAYER,
			      toolbox_drop_drawable,
			      context);
  gimp_dnd_viewable_dest_add (toolbox->wbox, GIMP_TYPE_LAYER_MASK,
			      toolbox_drop_drawable,
			      context);
  gimp_dnd_viewable_dest_add (toolbox->wbox, GIMP_TYPE_CHANNEL,
			      toolbox_drop_drawable,
			      context);
  gimp_dnd_viewable_dest_add (toolbox->wbox, GIMP_TYPE_TOOL_INFO,
			      toolbox_drop_tool,
			      context);
  gimp_dnd_viewable_dest_add (toolbox->wbox, GIMP_TYPE_BUFFER,
			      toolbox_drop_buffer,
			      context);

  gimp_toolbox_style_set (GTK_WIDGET (toolbox), GTK_WIDGET (toolbox)->style);

  return GTK_WIDGET (toolbox);
}


/*  private functions  */

static gboolean
gimp_toolbox_button_accel_find_func (GtkAccelKey *key,
                                     GClosure    *closure,
                                     gpointer     data)
{
  return (GClosure *) data == closure;
}

static void
gimp_toolbox_button_accel_changed (GtkAccelGroup   *accel_group,
                                   guint            unused1,
                                   GdkModifierType  unused2,
                                   GClosure        *accel_closure,
                                   GtkWidget       *tool_button)
{
  GClosure *button_closure;

  button_closure = g_object_get_data (G_OBJECT (tool_button),
                                      "toolbox-accel-closure");

  if (accel_closure == button_closure)
    {
      GimpToolInfo *tool_info;
      GtkAccelKey  *accel_key;
      gchar        *tooltip;

      tool_info = g_object_get_data (G_OBJECT (tool_button), "tool-info");

      accel_key = gtk_accel_group_find (accel_group,
                                        gimp_toolbox_button_accel_find_func,
                                        accel_closure);

      if (accel_key            &&
          accel_key->accel_key &&
          accel_key->accel_flags & GTK_ACCEL_VISIBLE)
        {
          /*  mostly taken from gtk-2-0/gtk/gtkaccellabel.c:1.46
           */
          GtkAccelLabelClass *accel_label_class;
          GString            *gstring;
          gboolean            seen_mod = FALSE;
          gunichar            ch;

          accel_label_class = g_type_class_peek (GTK_TYPE_ACCEL_LABEL);

          gstring = g_string_new (tool_info->help);
          g_string_append (gstring, "     ");

          if (accel_key->accel_mods & GDK_SHIFT_MASK)
            {
              g_string_append (gstring, accel_label_class->mod_name_shift);
              seen_mod = TRUE;
            }
          if (accel_key->accel_mods & GDK_CONTROL_MASK)
            {
              if (seen_mod)
                g_string_append (gstring, accel_label_class->mod_separator);
              g_string_append (gstring, accel_label_class->mod_name_control);
              seen_mod = TRUE;
            }
          if (accel_key->accel_mods & GDK_MOD1_MASK)
            {
              if (seen_mod)
                g_string_append (gstring, accel_label_class->mod_separator);
              g_string_append (gstring, accel_label_class->mod_name_alt);
              seen_mod = TRUE;
            }
          if (seen_mod)
            g_string_append (gstring, accel_label_class->mod_separator);

          ch = gdk_keyval_to_unicode (accel_key->accel_key);
          if (ch && (g_unichar_isgraph (ch) || ch == ' ') &&
              (ch < 0x80 || accel_label_class->latin1_to_char))
            {
              switch (ch)
                {
                case ' ':
                  g_string_append (gstring, "Space");
                  break;
                case '\\':
                  g_string_append (gstring, "Backslash");
                  break;
                default:
                  g_string_append_unichar (gstring, g_unichar_toupper (ch));
                  break;
                }
            }
          else
            {
              gchar *tmp;

              tmp = gtk_accelerator_name (accel_key->accel_key, 0);
              if (tmp[0] != 0 && tmp[1] == 0)
                tmp[0] = g_ascii_toupper (tmp[0]);
              g_string_append (gstring, tmp);
              g_free (tmp);
            }

          tooltip = g_string_free (gstring, FALSE);
        }
      else
        {
          tooltip = g_strdup (tool_info->help);
        }

      gimp_help_set_help_data (tool_button,
			       tooltip,
			       tool_info->help_data);

      g_free (tooltip);
    }
}

static void
toolbox_create_tools (GimpToolbox *toolbox,
                      GimpContext *context)
{
  GimpItemFactory *item_factory;
  GimpToolInfo    *active_tool;
  GList           *list;
  GSList          *group = NULL;

  item_factory = gimp_item_factory_from_path ("<Image>");

  active_tool = gimp_context_get_tool (context);

  for (list = GIMP_LIST (context->gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info;
      GtkWidget    *button;
      GtkWidget    *image;
      GtkWidget    *menu_item;
      GList        *accel_closures;
      const gchar  *stock_id;

      tool_info = (GimpToolInfo *) list->data;

      button = gtk_radio_button_new (group);
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
      gtk_wrap_box_pack (GTK_WRAP_BOX (toolbox->wbox), button,
			 FALSE, FALSE, FALSE, FALSE);
      gtk_widget_show (button);

      g_object_set_data (G_OBJECT (tool_info), "toolbox-button", button);
      g_object_set_data (G_OBJECT (button), "tool-info", tool_info);

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));
      image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      if (tool_info == active_tool)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      g_signal_connect (button, "toggled",
			G_CALLBACK (toolbox_tool_button_toggled),
			tool_info);

      g_signal_connect (button, "button_press_event",
			G_CALLBACK (toolbox_tool_button_press),
                        toolbox);

      if (! item_factory)
        continue;

      menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
                                               tool_info->menu_path);
      accel_closures = gtk_widget_list_accel_closures (menu_item);

      if (g_list_length (accel_closures) != 1)
        {
          g_warning (G_STRLOC ": FIXME: g_list_length (accel_closures) != 1");
        }
      else
        {
          GClosure      *accel_closure;
          GtkAccelGroup *accel_group;

          accel_closure = (GClosure *) accel_closures->data;

          g_object_set_data (G_OBJECT (button), "toolbox-accel-closure",
                             accel_closure);

          accel_group = gtk_accel_group_from_accel_closure (accel_closure);

          g_signal_connect (accel_group, "accel_changed",
                            G_CALLBACK (gimp_toolbox_button_accel_changed),
                            button);

          gimp_toolbox_button_accel_changed (accel_group,
                                             0, 0,
                                             accel_closure,
                                             button);

        }

      g_list_free (accel_closures);
    }
}

static void
toolbox_create_color_area (GimpToolbox *toolbox,
                           GimpContext *context)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *col_area;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_wrap_box_pack_wrapped (GTK_WRAP_BOX (toolbox->wbox), frame,
			     TRUE, TRUE, FALSE, TRUE, TRUE);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  gimp_help_set_help_data (alignment, NULL, "#color_area");

  col_area = gimp_toolbox_color_area_create (toolbox, 54, 42);

  gtk_container_add (GTK_CONTAINER (alignment), col_area);
  gimp_help_set_help_data
    (col_area, _("Foreground & background colors.  The black and white squares "
                 "reset colors.  The arrows swap colors. Double click to open "
                 "the color selection dialog."), NULL);

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
	  g_signal_handlers_block_by_func (toolbox_button,
					   toolbox_tool_button_toggled,
					   tool_info);

	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbox_button),
                                        TRUE);

	  g_signal_handlers_unblock_by_func (toolbox_button,
					     toolbox_tool_button_toggled,
					     tool_info);
	}
    }
}

static void
toolbox_tool_button_toggled (GtkWidget    *widget,
			     GimpToolInfo *tool_info)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
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
                                        "gimp-tool-options",
                                        -1);
    }

  return FALSE;
}

static gboolean
toolbox_check_device (GtkWidget *widget,
		      GdkEvent  *event,
		      Gimp      *gimp)
{
  gimp_devices_check_change (gimp, event);

  return FALSE;
}

static void
toolbox_drop_drawable (GtkWidget    *widget,
		       GimpViewable *viewable,
		       gpointer      data)
{
  GimpDrawable      *drawable;
  GimpItem          *item;
  GimpImage         *gimage;
  GimpImage         *new_gimage;
  GimpLayer         *new_layer;
  gint               width, height;
  gint               off_x, off_y;
  gint               bytes;
  GimpImageBaseType  type;

  drawable = GIMP_DRAWABLE (viewable);
  item     = GIMP_ITEM (viewable);
  gimage   = gimp_item_get_image (item);

  width  = gimp_item_width  (item);
  height = gimp_item_height (item);
  bytes  = gimp_drawable_bytes (drawable);

  type = GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable));

  new_gimage = gimp_create_image (gimage->gimp,
				  width, height,
				  type,
				  FALSE);
  gimp_image_undo_disable (new_gimage);

  if (type == GIMP_INDEXED) /* copy the colormap */
    gimp_image_set_colormap (new_gimage,
                             gimp_image_get_colormap (gimage),
                             gimp_image_get_colormap_size (gimage),
                             FALSE);

  gimp_image_set_resolution (new_gimage,
			     gimage->xresolution, gimage->yresolution);
  gimp_image_set_unit (new_gimage, gimage->unit);

  if (GIMP_IS_LAYER (drawable))
    {
      new_layer = GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (drawable),
                                                   G_TYPE_FROM_INSTANCE (drawable),
                                                   FALSE));
    }
  else
    {
      new_layer = GIMP_LAYER (gimp_item_duplicate (GIMP_ITEM (drawable),
                                                   GIMP_TYPE_LAYER,
                                                   TRUE));
    }

  gimp_item_set_image (GIMP_ITEM (new_layer), new_gimage);

  gimp_object_set_name (GIMP_OBJECT (new_layer),
			gimp_object_get_name (GIMP_OBJECT (drawable)));

  gimp_drawable_offsets (GIMP_DRAWABLE (new_layer), &off_x, &off_y);
  gimp_layer_translate (new_layer, -off_x, -off_y, FALSE);

  gimp_image_add_layer (new_gimage, new_layer, 0);

  gimp_image_undo_enable (new_gimage);

  gimp_create_display (gimage->gimp, new_gimage, 0x0101);

  g_object_unref (new_gimage);
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
