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

#include "widgets-types.h"

#include "core/gimpcontext.h"

#include "gimpdockable.h"
#include "gimpdockbook.h"


static void        gimp_dockable_class_init       (GimpDockableClass *klass);
static void        gimp_dockable_init             (GimpDockable      *dockable);

static void        gimp_dockable_destroy             (GtkObject      *object);
static void        gimp_dockable_size_request        (GtkWidget      *widget,
                                                      GtkRequisition *requisition);
static void        gimp_dockable_size_allocate       (GtkWidget      *widget,
                                                      GtkAllocation  *allocation);
static void        gimp_dockable_style_set           (GtkWidget      *widget,
						      GtkStyle       *prev_style);

static GtkWidget * gimp_dockable_real_get_tab_widget (GimpDockable   *dockable,
                                                      GimpContext    *context,
                                                      GimpTabStyle    tab_style,
						      GtkIconSize     size);
static void        gimp_dockable_real_set_context    (GimpDockable   *dockable,
						      GimpContext    *context);


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
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = GTK_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy       = gimp_dockable_destroy;

  widget_class->size_request  = gimp_dockable_size_request;
  widget_class->size_allocate = gimp_dockable_size_allocate;
  widget_class->style_set     = gimp_dockable_style_set;

  klass->get_tab_widget       = gimp_dockable_real_get_tab_widget;
  klass->set_context          = gimp_dockable_real_set_context;

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
  dockable->name             = NULL;
  dockable->blurb            = NULL;
  dockable->stock_id         = NULL;
  dockable->tab_style        = GIMP_TAB_STYLE_ICON;
  dockable->dockbook         = NULL;
  dockable->context          = NULL;
  dockable->get_preview_func = NULL;
  dockable->get_preview_data = NULL;
  dockable->set_context_func = NULL;
}

static void
gimp_dockable_destroy (GtkObject *object)
{
  GimpDockable *dockable;

  dockable = GIMP_DOCKABLE (object);

  if (dockable->context)
    gimp_dockable_set_context (dockable, NULL);

  if (dockable->name)
    {
      g_free (dockable->name);
      dockable->name = NULL;
    }

  if (dockable->blurb)
    {
      g_free (dockable->blurb);
      dockable->blurb = NULL;
    }

  if (dockable->stock_id)
    {
      g_free (dockable->stock_id);
      dockable->stock_id = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_dockable_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  GtkBin *bin = GTK_BIN (widget);

  requisition->width  = GTK_CONTAINER (widget)->border_width * 2;
  requisition->height = GTK_CONTAINER (widget)->border_width * 2;

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
    {
      GtkRequisition child_requisition;
      
      gtk_widget_size_request (bin->child, &child_requisition);

      requisition->width  += child_requisition.width;
      requisition->height += child_requisition.height;
    }
}

static void
gimp_dockable_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  GtkBin *bin = GTK_BIN (widget);

  widget->allocation = *allocation;

  if (bin->child)
    {
      GtkAllocation  child_allocation;

      child_allocation.x      = allocation->x;
      child_allocation.y      = allocation->y;
      child_allocation.width  = MAX (allocation->width  -
                                     GTK_CONTAINER (widget)->border_width * 2,
                                     0);
      child_allocation.height = MAX (allocation->height -
                                     GTK_CONTAINER (widget)->border_width * 2,
                                     0);

      gtk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static void
gimp_dockable_style_set (GtkWidget *widget,
			 GtkStyle  *prev_style)
{
  gint content_border;

  gtk_widget_style_get (widget,
                        "content_border", &content_border,
			NULL);

  gtk_container_set_border_width (GTK_CONTAINER (widget), content_border);

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

GtkWidget *
gimp_dockable_new (const gchar                *name,
		   const gchar                *blurb,
                   const gchar                *stock_id,
		   GimpDockableGetPreviewFunc  get_preview_func,
                   gpointer                    get_preview_data,
		   GimpDockableSetContextFunc  set_context_func)
{
  GimpDockable *dockable;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (blurb != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);

  dockable = g_object_new (GIMP_TYPE_DOCKABLE, NULL);

  dockable->name     = g_strdup (name);
  dockable->blurb    = g_strdup (blurb);
  dockable->stock_id = g_strdup (stock_id);

  dockable->get_preview_func = get_preview_func;
  dockable->get_preview_data = get_preview_data;
  dockable->set_context_func = set_context_func;

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
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (dockable->set_context_func)
    dockable->set_context_func (dockable, context);

  dockable->context = context;
}
