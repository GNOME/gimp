/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockable.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/gimpcontext.h"

#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimpitemfactory.h"
#include "gimpwidgets-utils.h"


static void        gimp_dockable_class_init       (GimpDockableClass *klass);
static void        gimp_dockable_init             (GimpDockable      *dockable);

static void        gimp_dockable_destroy             (GtkObject      *object);
static void        gimp_dockable_size_request        (GtkWidget      *widget,
                                                      GtkRequisition *requisition);
static void        gimp_dockable_size_allocate       (GtkWidget      *widget,
                                                      GtkAllocation  *allocation);
static void        gimp_dockable_style_set           (GtkWidget      *widget,
						      GtkStyle       *prev_style);
static gboolean    gimp_dockable_expose_event        (GtkWidget      *widget,
                                                      GdkEventExpose *event);
static gboolean    gimp_dockable_popup_menu          (GtkWidget      *widget);

static void        gimp_dockable_forall              (GtkContainer   *container,
                                                      gboolean       include_internals,
                                                      GtkCallback     callback,
                                                      gpointer        callback_data);

static GtkWidget * gimp_dockable_real_get_tab_widget (GimpDockable   *dockable,
                                                      GimpContext    *context,
                                                      GimpTabStyle    tab_style,
						      GtkIconSize     size);
static void        gimp_dockable_real_set_context    (GimpDockable   *dockable,
						      GimpContext    *context);
static GimpItemFactory * gimp_dockable_real_get_menu (GimpDockable   *dockable,
                                                      gpointer       *item_factory_data);

static gboolean    gimp_dockable_menu_button_press   (GtkWidget      *button,
                                                      GdkEventButton *bevent,
                                                      GimpDockable   *dockable);
static void        gimp_dockable_close_clicked       (GtkWidget      *button,
                                                      GimpDockable   *dockable);
static gboolean    gimp_dockable_show_menu           (GimpDockable *dockable);


static GtkBinClass *parent_class = NULL;


GType
gimp_dockable_get_type (void)
{
  static GType dockable_type = 0;

  if (! dockable_type)
    {
      static const GTypeInfo dockable_info =
      {
        sizeof (GimpDockableClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_dockable_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpDockable),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_dockable_init,
      };

      dockable_type = g_type_register_static (GTK_TYPE_BIN,
                                              "GimpDockable",
                                              &dockable_info, 0);
    }

  return dockable_type;
}

static void
gimp_dockable_class_init (GimpDockableClass *klass)
{
  GtkObjectClass    *object_class;
  GtkWidgetClass    *widget_class;
  GtkContainerClass *container_class;

  object_class    = GTK_OBJECT_CLASS (klass);
  widget_class    = GTK_WIDGET_CLASS (klass);
  container_class = GTK_CONTAINER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy       = gimp_dockable_destroy;

  widget_class->size_request  = gimp_dockable_size_request;
  widget_class->size_allocate = gimp_dockable_size_allocate;
  widget_class->style_set     = gimp_dockable_style_set;
  widget_class->expose_event  = gimp_dockable_expose_event;
  widget_class->popup_menu    = gimp_dockable_popup_menu;

  container_class->forall     = gimp_dockable_forall;

  klass->get_tab_widget       = gimp_dockable_real_get_tab_widget;
  klass->set_context          = gimp_dockable_real_set_context;
  klass->get_menu             = gimp_dockable_real_get_menu;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content_border",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             G_PARAM_READABLE));
}

static void
gimp_dockable_init (GimpDockable *dockable)
{
  GtkWidget *image;

  dockable->name             = NULL;
  dockable->blurb            = NULL;
  dockable->stock_id         = NULL;
  dockable->help_id          = NULL;
  dockable->tab_style        = GIMP_TAB_STYLE_PREVIEW;
  dockable->dockbook         = NULL;
  dockable->context          = NULL;
  dockable->get_preview_func = NULL;
  dockable->get_preview_data = NULL;
  dockable->set_context_func = NULL;

  dockable->title_layout     = NULL;

  gtk_widget_push_composite_child ();
  dockable->menu_button = gtk_button_new ();
  gtk_widget_pop_composite_child ();

  GTK_WIDGET_UNSET_FLAGS (dockable->menu_button, GTK_CAN_FOCUS);
  gtk_widget_set_parent (dockable->menu_button, GTK_WIDGET (dockable));
  gtk_button_set_relief (GTK_BUTTON (dockable->menu_button), GTK_RELIEF_NONE);
  gtk_widget_show (dockable->menu_button);

  image = gtk_image_new_from_stock (GIMP_STOCK_MENU_LEFT, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (dockable->menu_button), image);
  gtk_widget_show (image);

  g_signal_connect (dockable->menu_button, "button_press_event",
                    G_CALLBACK (gimp_dockable_menu_button_press),
                    dockable);

  gtk_widget_push_composite_child ();
  dockable->close_button = gtk_button_new ();
  gtk_widget_pop_composite_child ();

  GTK_WIDGET_UNSET_FLAGS (dockable->close_button, GTK_CAN_FOCUS);
  gtk_widget_set_parent (dockable->close_button, GTK_WIDGET (dockable));
  gtk_button_set_relief (GTK_BUTTON (dockable->close_button), GTK_RELIEF_NONE);
  gtk_widget_show (dockable->close_button);

  image = gtk_image_new_from_stock (GIMP_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (dockable->close_button), image);
  gtk_widget_show (image);

  g_signal_connect (dockable->close_button, "clicked",
                    G_CALLBACK (gimp_dockable_close_clicked),
                    dockable);
}

static void
gimp_dockable_destroy (GtkObject *object)
{
  GimpDockable *dockable = GIMP_DOCKABLE (object);

  if (dockable->context)
    gimp_dockable_set_context (dockable, NULL);

  if (dockable->blurb && dockable->blurb != dockable->name)
    {
      g_free (dockable->blurb);
      dockable->blurb = NULL;
    }

  if (dockable->name)
    {
      g_free (dockable->name);
      dockable->name = NULL;
    }

  if (dockable->stock_id)
    {
      g_free (dockable->stock_id);
      dockable->stock_id = NULL;
    }

  if (dockable->help_id)
    {
      g_free (dockable->help_id);
      dockable->help_id = NULL;
    }

  if (dockable->title_layout)
    {
      g_object_unref (dockable->title_layout);
      dockable->title_layout = NULL;
    }

  if (dockable->menu_button)
    {
      gtk_widget_unparent (dockable->menu_button);
      dockable->menu_button = NULL;
    }

  if (dockable->close_button)
    {
      gtk_widget_unparent (dockable->close_button);
      dockable->close_button = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_dockable_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  GtkContainer   *container;
  GtkBin         *bin;
  GimpDockable   *dockable;
  GtkRequisition  child_requisition;

  container = GTK_CONTAINER (widget);
  bin       = GTK_BIN (widget);
  dockable  = GIMP_DOCKABLE (widget);

  requisition->width  = container->border_width * 2;
  requisition->height = container->border_width * 2;

  if (dockable->close_button && GTK_WIDGET_VISIBLE (dockable->close_button))
    {
      gtk_widget_size_request (dockable->close_button, &child_requisition);

      if (! bin->child)
        requisition->width += child_requisition.width;

      requisition->height += child_requisition.height;
    }

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      gtk_widget_size_request (bin->child, &child_requisition);

      requisition->width  += child_requisition.width;
      requisition->height += child_requisition.height;
    }
}

static void
gimp_dockable_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  GtkContainer   *container;
  GtkBin         *bin;
  GimpDockable   *dockable;
  GtkRequisition  button_requisition = { 0, };
  GtkAllocation   child_allocation;

  container = GTK_CONTAINER (widget);
  bin       = GTK_BIN (widget);
  dockable  = GIMP_DOCKABLE (widget);

  widget->allocation = *allocation;

  if (dockable->close_button)
    {
      gtk_widget_size_request (dockable->close_button, &button_requisition);

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
        child_allocation.x    = (allocation->x +
                                 allocation->width -
                                 container->border_width -
                                 button_requisition.width);
      else
        child_allocation.x    = allocation->x + container->border_width;

      child_allocation.y      = allocation->y + container->border_width;
      child_allocation.width  = button_requisition.width;
      child_allocation.height = button_requisition.height;

      gtk_widget_size_allocate (dockable->close_button, &child_allocation);
    }

  if (dockable->menu_button)
    {
      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
        child_allocation.x    = (allocation->x +
                                 allocation->width -
                                 container->border_width -
                                 2 * button_requisition.width);
      else
        child_allocation.x    = (allocation->x + container->border_width +
                                 button_requisition.width);

      child_allocation.y      = allocation->y + container->border_width;
      child_allocation.width  = button_requisition.width;
      child_allocation.height = button_requisition.height;

      gtk_widget_size_allocate (dockable->menu_button, &child_allocation);
    }

  if (bin->child)
    {
      child_allocation.x      = allocation->x + container->border_width;
      child_allocation.y      = allocation->y + container->border_width;
      child_allocation.width  = MAX (allocation->width  -
                                     container->border_width * 2,
                                     0);
      child_allocation.height = MAX (allocation->height -
                                     container->border_width * 2 -
                                     button_requisition.height,
                                     0);

      child_allocation.y += button_requisition.height;

      gtk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static void
gimp_dockable_style_set (GtkWidget *widget,
			 GtkStyle  *prev_style)
{
  GimpDockable *dockable;
  gint          content_border;

  gtk_widget_style_get (widget,
                        "content_border", &content_border,
			NULL);

  gtk_container_set_border_width (GTK_CONTAINER (widget), content_border);

  dockable = GIMP_DOCKABLE (widget);

  if (dockable->title_layout)
    {
      g_object_unref (dockable->title_layout);
      dockable->title_layout = NULL;
    }

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

static gboolean
gimp_dockable_expose_event (GtkWidget      *widget,
                            GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GimpDockable *dockable;
      GtkContainer *container;
      GdkRectangle  title_area;
      GdkRectangle  expose_area;

      dockable  = GIMP_DOCKABLE (widget);
      container = GTK_CONTAINER (widget);

      title_area.x      = widget->allocation.x + container->border_width;
      title_area.y      = widget->allocation.y + container->border_width;

      title_area.width  = (widget->allocation.width -
                           2 * container->border_width -
                           2 * dockable->close_button->allocation.width);
      title_area.height = dockable->close_button->allocation.height;

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        title_area.x   += 2 * dockable->close_button->allocation.width;

      if (gdk_rectangle_intersect (&title_area, &event->area, &expose_area))
        {
          gint layout_width;
          gint layout_height;
          gint text_x;
          gint text_y;

          if (!dockable->title_layout)
            {
              dockable->title_layout =
                gtk_widget_create_pango_layout (widget, dockable->blurb);
            }

          pango_layout_get_pixel_size (dockable->title_layout,
                                       &layout_width, &layout_height);

          if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
            {
              text_x = title_area.x;
            }
          else
            {
              text_x = title_area.x + title_area.width - layout_width;
            }

          text_y = title_area.y + (title_area.height - layout_height) / 2;

          gtk_paint_layout (widget->style, widget->window,
                            widget->state, TRUE,
                            &expose_area, widget, NULL,
                            text_x, text_y, dockable->title_layout);
        }
    }

  return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
}

static gboolean
gimp_dockable_popup_menu (GtkWidget *widget)
{
  return gimp_dockable_show_menu (GIMP_DOCKABLE (widget));
}

static void
gimp_dockable_forall (GtkContainer *container,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
  GimpDockable *dockable = GIMP_DOCKABLE (container);

  if (include_internals)
    {
      if (dockable->menu_button)
        (* callback) (dockable->menu_button, callback_data);

      if (dockable->close_button)
        (* callback) (dockable->close_button, callback_data);
    }

  GTK_CONTAINER_CLASS (parent_class)->forall (container, include_internals,
                                              callback, callback_data);
}

GtkWidget *
gimp_dockable_new (const gchar                *name,
		   const gchar                *blurb,
                   const gchar                *stock_id,
                   const gchar                *help_id,
		   GimpDockableGetPreviewFunc  get_preview_func,
                   gpointer                    get_preview_data,
		   GimpDockableSetContextFunc  set_context_func,
                   GimpDockableGetMenuFunc     get_menu_func)
{
  GimpDockable *dockable;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  dockable = g_object_new (GIMP_TYPE_DOCKABLE, NULL);

  dockable->name     = g_strdup (name);
  dockable->stock_id = g_strdup (stock_id);
  dockable->help_id  = g_strdup (help_id);

  if (blurb)
    dockable->blurb  = g_strdup (blurb);
  else
    dockable->blurb  = dockable->name;

  dockable->get_preview_func = get_preview_func;
  dockable->get_preview_data = get_preview_data;
  dockable->set_context_func = set_context_func;
  dockable->get_menu_func    = get_menu_func;

  if (! get_preview_func)
    {
      switch (dockable->tab_style)
        {
        case GIMP_TAB_STYLE_PREVIEW:
          dockable->tab_style = GIMP_TAB_STYLE_ICON;
          break;
        case GIMP_TAB_STYLE_PREVIEW_NAME:
          dockable->tab_style = GIMP_TAB_STYLE_ICON_BLURB;
          break;
        case GIMP_TAB_STYLE_PREVIEW_BLURB:
          dockable->tab_style = GIMP_TAB_STYLE_ICON_BLURB;
          break;
        default:
          break;
        }
    }

  gimp_help_set_help_data (GTK_WIDGET (dockable), NULL, help_id);

  return GTK_WIDGET (dockable);
}

GtkWidget *
gimp_dockable_get_tab_widget (GimpDockable *dockable,
                              GimpContext  *context,
                              GimpTabStyle  tab_style,
                              GtkIconSize   size)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return GIMP_DOCKABLE_GET_CLASS (dockable)->get_tab_widget (dockable, context,
                                                             tab_style, size);
}

void
gimp_dockable_set_context (GimpDockable *dockable,
                           GimpContext  *context)
{
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (context != dockable->context)
    GIMP_DOCKABLE_GET_CLASS (dockable)->set_context (dockable, context);
}

GimpItemFactory *
gimp_dockable_get_menu (GimpDockable *dockable,
                        gpointer     *item_factory_data)
{
  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);
  g_return_val_if_fail (item_factory_data != NULL, NULL);

  return GIMP_DOCKABLE_GET_CLASS (dockable)->get_menu (dockable,
                                                       item_factory_data);
}

static GtkWidget *
gimp_dockable_real_get_tab_widget (GimpDockable *dockable,
                                   GimpContext  *context,
                                   GimpTabStyle  tab_style,
                                   GtkIconSize   size)
{
  GtkWidget *tab_widget = NULL;
  GtkWidget *label      = NULL;
  GtkWidget *icon       = NULL;

  switch (tab_style)
    {
    case GIMP_TAB_STYLE_NAME:
    case GIMP_TAB_STYLE_ICON_NAME:
    case GIMP_TAB_STYLE_PREVIEW_NAME:
      label = gtk_label_new (dockable->name);
      break;

    case GIMP_TAB_STYLE_BLURB:
    case GIMP_TAB_STYLE_ICON_BLURB:
    case GIMP_TAB_STYLE_PREVIEW_BLURB:
      label = gtk_label_new (dockable->blurb);
      break;

    default:
      break;
    }

  switch (tab_style)
    {
    case GIMP_TAB_STYLE_ICON:
    case GIMP_TAB_STYLE_ICON_NAME:
    case GIMP_TAB_STYLE_ICON_BLURB:
      icon = gtk_image_new_from_stock (dockable->stock_id, size);
      break;

    case GIMP_TAB_STYLE_PREVIEW:
    case GIMP_TAB_STYLE_PREVIEW_NAME:
    case GIMP_TAB_STYLE_PREVIEW_BLURB:
      if (dockable->get_preview_func)
        icon = dockable->get_preview_func (dockable, context, size,
                                           dockable->get_preview_data);
      else
        icon = gtk_image_new_from_stock (dockable->stock_id, size);
      break;

    default:
      break;
    }

  switch (tab_style)
    {
    case GIMP_TAB_STYLE_ICON:
    case GIMP_TAB_STYLE_PREVIEW:
      tab_widget = icon;
      break;

    case GIMP_TAB_STYLE_NAME:
    case GIMP_TAB_STYLE_BLURB:
      tab_widget = label;
      break;

    case GIMP_TAB_STYLE_ICON_NAME:
    case GIMP_TAB_STYLE_ICON_BLURB:
    case GIMP_TAB_STYLE_PREVIEW_NAME:
    case GIMP_TAB_STYLE_PREVIEW_BLURB:
      tab_widget = gtk_hbox_new (FALSE, 4);

      gtk_box_pack_start (GTK_BOX (tab_widget), icon, FALSE, FALSE, 0);
      gtk_widget_show (icon);

      gtk_box_pack_start (GTK_BOX (tab_widget), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
      break;
    }

  return tab_widget;
}

static void
gimp_dockable_real_set_context (GimpDockable *dockable,
				GimpContext  *context)
{
  if (dockable->set_context_func)
    dockable->set_context_func (dockable, context);

  dockable->context = context;
}

static GimpItemFactory *
gimp_dockable_real_get_menu (GimpDockable *dockable,
                             gpointer     *item_factory_data)
{
  if (dockable->get_menu_func)
    return dockable->get_menu_func (dockable, item_factory_data);

  return NULL;
}

static gboolean
gimp_dockable_menu_button_press (GtkWidget      *button,
                                 GdkEventButton *bevent,
                                 GimpDockable   *dockable)
{
  if (bevent->button == 1 && bevent->type == GDK_BUTTON_PRESS)
    {
      return gimp_dockable_show_menu (dockable);
    }

  return FALSE;
}

static void
gimp_dockable_close_clicked (GtkWidget    *button,
                             GimpDockable *dockable)
{
  gimp_dockbook_remove (dockable->dockbook, dockable);
}

static void
gimp_dockable_menu_position (GtkMenu  *menu,
                             gint     *x,
                             gint     *y,
                             gpointer  data)
{
  GimpDockable *dockable = GIMP_DOCKABLE (data);

  gimp_button_menu_position (dockable->menu_button, menu, GTK_POS_LEFT, x, y);
}

static void
gimp_dockable_menu_end (GimpDockable *dockable)
{
  GimpItemFactory *dialog_item_factory;
  gpointer         dialog_item_factory_data;

  dialog_item_factory = gimp_dockable_get_menu (dockable,
                                                &dialog_item_factory_data);

  if (dialog_item_factory)
    gtk_menu_detach (GTK_MENU (GTK_ITEM_FACTORY (dialog_item_factory)->widget));

  /*  release gimp_dockable_show_menu()'s reference  */
  g_object_unref (dockable);
}

static gboolean
gimp_dockable_show_menu (GimpDockable *dockable)
{
  GimpItemFactory *dockbook_item_factory;
  GimpItemFactory *dialog_item_factory;
  gpointer         dialog_item_factory_data;

  dockbook_item_factory = dockable->dockbook->item_factory;

  if (! dockbook_item_factory)
    return FALSE;

  dialog_item_factory = gimp_dockable_get_menu (dockable,
                                                &dialog_item_factory_data);

  if (dialog_item_factory)
    {
      GtkWidget *dialog_menu_widget;
      GtkWidget *image;

      gimp_item_factory_set_label (GTK_ITEM_FACTORY (dockbook_item_factory),
                                   "/dialog-menu",
                                   dialog_item_factory->title);
      gimp_item_factory_set_visible (GTK_ITEM_FACTORY (dockbook_item_factory),
                                     "/dialog-menu", TRUE);

      dialog_menu_widget =
        gtk_item_factory_get_widget (GTK_ITEM_FACTORY (dockbook_item_factory),
                                     "/dialog-menu");

      image = gtk_image_new_from_stock (dockable->stock_id,
                                        GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (dialog_menu_widget),
                                     image);
      gtk_widget_show (image);

      gtk_menu_item_set_submenu (GTK_MENU_ITEM (dialog_menu_widget),
                                 GTK_ITEM_FACTORY (dialog_item_factory)->widget);

      gimp_item_factory_update (dialog_item_factory,
                                dialog_item_factory_data);
    }
  else
    {
      gimp_item_factory_set_visible (GTK_ITEM_FACTORY (dockbook_item_factory),
                                     "/dialog-menu", FALSE);
    }

  gimp_item_factory_set_visible (GTK_ITEM_FACTORY (dockbook_item_factory),
                                 "/Select Tab", FALSE);

  /*  an item factory callback may destroy the dockable, so reference
   *  if for gimp_dockable_menu_end()
   */
  g_object_ref (dockable);

  gimp_item_factory_popup_with_data (dockbook_item_factory,
                                     dockable,
                                     gimp_dockable_menu_position,
                                     dockable,
                                     (GtkDestroyNotify) gimp_dockable_menu_end);

  return TRUE;
}
