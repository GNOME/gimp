/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppreview.c
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpmarshal.h"
#include "core/gimpviewable.h"

#include "gimpdnd.h"
#include "gimppreview.h"
#include "gimppreview-popup.h"
#include "gimppreviewrenderer.h"
#include "gimppreviewrenderer-utils.h"


#define PREVIEW_EVENT_MASK (GDK_BUTTON_PRESS_MASK   | \
                            GDK_BUTTON_RELEASE_MASK | \
                            GDK_ENTER_NOTIFY_MASK   | \
                            GDK_LEAVE_NOTIFY_MASK)


enum
{
  CLICKED,
  DOUBLE_CLICKED,
  CONTEXT,
  LAST_SIGNAL
};


static void        gimp_preview_class_init           (GimpPreviewClass *klass);
static void        gimp_preview_init                 (GimpPreview      *preview);

static void        gimp_preview_destroy              (GtkObject        *object);
static void        gimp_preview_realize              (GtkWidget        *widget);
static void        gimp_preview_unrealize            (GtkWidget        *widget);
static void        gimp_preview_map                  (GtkWidget        *widget);
static void        gimp_preview_unmap                (GtkWidget        *widget);
static void        gimp_preview_size_request         (GtkWidget        *widget,
						      GtkRequisition   *requisition);
static void        gimp_preview_size_allocate        (GtkWidget        *widget,
						      GtkAllocation    *allocation);
static gboolean    gimp_preview_expose_event         (GtkWidget        *widget,
                                                      GdkEventExpose   *event);
static gboolean    gimp_preview_button_press_event   (GtkWidget        *widget,
						      GdkEventButton   *bevent);
static gboolean    gimp_preview_button_release_event (GtkWidget        *widget,
						      GdkEventButton   *bevent);
static gboolean    gimp_preview_enter_notify_event   (GtkWidget        *widget,
						      GdkEventCrossing *event);
static gboolean    gimp_preview_leave_notify_event   (GtkWidget        *widget,
						      GdkEventCrossing *event);

static void        gimp_preview_update_callback   (GimpPreviewRenderer *renderer,
                                                   GimpPreview         *preview);

static GimpViewable * gimp_preview_drag_viewable     (GtkWidget        *widget,
						      gpointer          data);



static guint preview_signals[LAST_SIGNAL] = { 0 };

static GtkWidgetClass *parent_class = NULL;


GType
gimp_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpPreviewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpPreview),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_preview_init,
      };

      preview_type = g_type_register_static (GTK_TYPE_WIDGET,
                                             "GimpPreview",
                                             &preview_info, 0);
    }

  return preview_type;
}

static void
gimp_preview_class_init (GimpPreviewClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = GTK_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_signals[CLICKED] =
    g_signal_new ("clicked",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpPreviewClass, clicked),
		  NULL, NULL,
		  gimp_marshal_VOID__FLAGS,
		  G_TYPE_NONE, 1,
		  GDK_TYPE_MODIFIER_TYPE);

  preview_signals[DOUBLE_CLICKED] =
    g_signal_new ("double_clicked",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpPreviewClass, double_clicked),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  preview_signals[CONTEXT] =
    g_signal_new ("context",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpPreviewClass, context),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->destroy              = gimp_preview_destroy;

  widget_class->activate_signal      = preview_signals[CLICKED];
  widget_class->realize              = gimp_preview_realize;
  widget_class->unrealize            = gimp_preview_unrealize;
  widget_class->map                  = gimp_preview_map;
  widget_class->unmap                = gimp_preview_unmap;
  widget_class->size_request         = gimp_preview_size_request;
  widget_class->size_allocate        = gimp_preview_size_allocate;
  widget_class->expose_event         = gimp_preview_expose_event;
  widget_class->button_press_event   = gimp_preview_button_press_event;
  widget_class->button_release_event = gimp_preview_button_release_event;
  widget_class->enter_notify_event   = gimp_preview_enter_notify_event;
  widget_class->leave_notify_event   = gimp_preview_leave_notify_event;

  klass->clicked                     = NULL;
  klass->double_clicked              = NULL;
  klass->context                     = NULL;
}

static void
gimp_preview_init (GimpPreview *preview)
{
  GTK_WIDGET_SET_FLAGS (preview, GTK_NO_WINDOW);

  preview->viewable          = NULL;
  preview->renderer          = NULL;

  preview->clickable         = FALSE;
  preview->eat_button_events = TRUE;
  preview->show_popup        = FALSE;
  preview->expand            = FALSE;

  preview->in_button         = FALSE;
}

static void
gimp_preview_destroy (GtkObject *object)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (object);

  if (preview->viewable)
    gimp_preview_set_viewable (preview, NULL);

  if (preview->renderer)
    {
      g_object_unref (preview->renderer);
      preview->renderer = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_preview_realize (GtkWidget *widget)
{
  GimpPreview   *preview;
  GdkWindowAttr  attributes;
  gint           attributes_mask;

  preview = GIMP_PREVIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x      = widget->allocation.x;
  attributes.y      = widget->allocation.y;
  attributes.width  = widget->allocation.width;
  attributes.height = widget->allocation.height;

  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= PREVIEW_EVENT_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  preview->event_window = gdk_window_new (widget->window,
                                          &attributes, attributes_mask);
  gdk_window_set_user_data (preview->event_window, preview);
}

static void
gimp_preview_unrealize (GtkWidget *widget)
{
  GimpPreview *preview = GIMP_PREVIEW (widget);

  if (preview->event_window)
    {
      gdk_window_set_user_data (preview->event_window, NULL);
      gdk_window_destroy (preview->event_window);
      preview->event_window = NULL;
    }

  if (preview->renderer)
    gimp_preview_renderer_unrealize (preview->renderer);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_preview_map (GtkWidget *widget)
{
  GimpPreview *preview = GIMP_PREVIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  if (preview->event_window)
    gdk_window_show (preview->event_window);
}

static void
gimp_preview_unmap (GtkWidget *widget)
{
  GimpPreview *preview = GIMP_PREVIEW (widget);

  if (preview->has_grab)
    gtk_grab_remove (widget);

  if (preview->event_window)
    gdk_window_hide (preview->event_window);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gimp_preview_size_request (GtkWidget      *widget,
                           GtkRequisition *requisition)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (widget);

  if (preview->expand)
    {
      requisition->width  = 2 * preview->renderer->border_width + 1;
      requisition->height = 2 * preview->renderer->border_width + 1;
    }
  else
    {
      requisition->width  = (preview->renderer->width +
                             2 * preview->renderer->border_width);
      requisition->height = (preview->renderer->height +
                             2 * preview->renderer->border_width);
    }

  if (GTK_WIDGET_CLASS (parent_class)->size_request)
    GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);
}

static void
gimp_preview_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
  GimpPreview *preview;
  gint         width;
  gint         height;

  preview = GIMP_PREVIEW (widget);

  if (preview->expand)
    {
      width  = MIN (GIMP_VIEWABLE_MAX_PREVIEW_SIZE,
                    allocation->width - 2 * preview->renderer->border_width);
      height = MIN (GIMP_VIEWABLE_MAX_PREVIEW_SIZE,
                    allocation->height - 2 * preview->renderer->border_width);

      if (preview->renderer->width  != width ||
          preview->renderer->height != height)
        {
          gint border_width = preview->renderer->border_width;

          if (preview->renderer->size != -1 && preview->renderer->viewable)
            {
              gint preview_width;
              gint preview_height;
              gint scaled_width;
              gint scaled_height;

              gimp_viewable_get_preview_size (preview->renderer->viewable,
                                              GIMP_VIEWABLE_MAX_PREVIEW_SIZE,
                                              preview->renderer->is_popup,
                                              preview->renderer->dot_for_dot,
                                              &preview_width,
                                              &preview_height);

              gimp_viewable_calc_preview_size (preview_width, preview_height,
                                               width, height,
                                               TRUE, 1.0, 1.0,
                                               &scaled_width, &scaled_height,
                                               NULL);

              if (scaled_width > width)
                {
                  scaled_height = scaled_height * width / scaled_width;
                  scaled_width  = scaled_width  * width / scaled_width;
                }
              else if (scaled_height > height)
                {
                  scaled_width  = scaled_width  * height / scaled_height;
                  scaled_height = scaled_height * height / scaled_height;
                }

              gimp_preview_renderer_set_size (preview->renderer,
                                              MAX (scaled_width, scaled_height),
                                              border_width);
            }
          else
            {
              gimp_preview_renderer_set_size_full (preview->renderer,
                                                   width, height,
                                                   border_width);
            }

          gimp_preview_renderer_remove_idle (preview->renderer);
        }
    }

  width  = (preview->renderer->width +
            2 * preview->renderer->border_width);
  height = (preview->renderer->height +
            2 * preview->renderer->border_width);

  if (allocation->width > width)
    allocation->x += (allocation->width - width) / 2;

  if (allocation->height > height)
    allocation->y += (allocation->height - height) / 2;

  allocation->width  = width;
  allocation->height = height;

  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (preview->event_window,
			    allocation->x,
			    allocation->y,
			    allocation->width,
                            allocation->height);
}

static gboolean
gimp_preview_expose_event (GtkWidget      *widget,
                           GdkEventExpose *event)
{
  if (! GTK_WIDGET_DRAWABLE (widget))
    return FALSE;

  gimp_preview_renderer_draw (GIMP_PREVIEW (widget)->renderer,
                              widget->window, widget,
                              &widget->allocation,
                              &event->area);

  return FALSE;
}


#define DEBUG_MEMSIZE 1

#ifdef DEBUG_MEMSIZE
extern gboolean gimp_debug_memsize;
#endif

static gboolean
gimp_preview_button_press_event (GtkWidget      *widget,
				 GdkEventButton *bevent)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (widget);

#ifdef DEBUG_MEMSIZE
  if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 2)
    {
      gimp_debug_memsize = TRUE;

      gimp_object_get_memsize (GIMP_OBJECT (preview->viewable), NULL);

      gimp_debug_memsize = FALSE;
    }
#endif /* DEBUG_MEMSIZE */

  if (! preview->clickable &&
      ! preview->show_popup)
    return FALSE;

  if (! GTK_WIDGET_REALIZED (widget))
    return FALSE;

  if (bevent->type == GDK_BUTTON_PRESS)
    {
      if (bevent->button == 1)
	{
	  gtk_grab_add (widget);

          preview->has_grab    = TRUE;
	  preview->press_state = bevent->state;

	  if (preview->show_popup)
            {
              bevent->x += widget->allocation.x;
              bevent->y += widget->allocation.y;

              gimp_preview_popup_show (widget, bevent,
                                       preview->viewable,
                                       preview->renderer->width,
                                       preview->renderer->height,
                                       preview->renderer->dot_for_dot);
	    }
	}
      else
	{
	  preview->press_state = 0;

          if (bevent->button == 3)
            g_signal_emit (widget, preview_signals[CONTEXT], 0);
	  else
            return FALSE;
	}
    }
  else if (bevent->type == GDK_2BUTTON_PRESS)
    {
      if (bevent->button == 1)
        g_signal_emit (widget, preview_signals[DOUBLE_CLICKED], 0);
    }

  return preview->eat_button_events ? TRUE : FALSE;
}

static gboolean
gimp_preview_button_release_event (GtkWidget      *widget,
				   GdkEventButton *bevent)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (widget);

  if (! preview->clickable &&
      ! preview->show_popup)
    return FALSE;

  if (bevent->button == 1)
    {
      gtk_grab_remove (widget);

      preview->has_grab = FALSE;

      if (preview->clickable && preview->in_button)
	{
          g_signal_emit (widget, preview_signals[CLICKED], 0,
                         preview->press_state);
	}
    }
  else
    {
      return FALSE;
    }

  return preview->eat_button_events ? TRUE : FALSE;
}

static gboolean
gimp_preview_enter_notify_event (GtkWidget        *widget,
				 GdkEventCrossing *event)
{
  GimpPreview *preview;
  GtkWidget   *event_widget;

  preview = GIMP_PREVIEW (widget);
  event_widget = gtk_get_event_widget ((GdkEvent *) event);

  if ((event_widget == widget) &&
      (event->detail != GDK_NOTIFY_INFERIOR))
    {
      preview->in_button = TRUE;
    }

  return FALSE;
}

static gboolean
gimp_preview_leave_notify_event (GtkWidget        *widget,
				 GdkEventCrossing *event)
{
  GimpPreview *preview;
  GtkWidget   *event_widget;

  preview = GIMP_PREVIEW (widget);
  event_widget = gtk_get_event_widget ((GdkEvent *) event);

  if ((event_widget == widget) &&
      (event->detail != GDK_NOTIFY_INFERIOR))
    {
      preview->in_button = FALSE;
    }

  return FALSE;
}


/*  public functions  */

GtkWidget *
gimp_preview_new (GimpViewable *viewable,
		  gint          size,
		  gint          border_width,
		  gboolean      is_popup)
{
  GtkWidget *preview;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  preview = gimp_preview_new_by_types (GIMP_TYPE_PREVIEW,
                                       G_TYPE_FROM_INSTANCE (viewable),
                                       size, border_width, is_popup);

  if (preview)
    gimp_preview_set_viewable (GIMP_PREVIEW (preview), viewable);

  gimp_preview_renderer_remove_idle (GIMP_PREVIEW (preview)->renderer);

  return preview;
}

GtkWidget *
gimp_preview_new_full (GimpViewable *viewable,
		       gint          width,
		       gint          height,
		       gint          border_width,
		       gboolean      is_popup,
		       gboolean      clickable,
		       gboolean      show_popup)
{
  GtkWidget *preview;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);

  preview = gimp_preview_new_full_by_types (GIMP_TYPE_PREVIEW,
                                            G_TYPE_FROM_INSTANCE (viewable),
                                            width, height, border_width,
                                            is_popup, clickable, show_popup);

  if (preview)
    gimp_preview_set_viewable (GIMP_PREVIEW (preview), viewable);

  gimp_preview_renderer_remove_idle (GIMP_PREVIEW (preview)->renderer);

  return preview;
}

GtkWidget *
gimp_preview_new_by_types (GType    preview_type,
                           GType    viewable_type,
                           gint     size,
                           gint     border_width,
                           gboolean is_popup)
{
  GimpPreviewRenderer *renderer;
  GimpPreview         *preview;

  g_return_val_if_fail (g_type_is_a (preview_type, GIMP_TYPE_PREVIEW), NULL);
  g_return_val_if_fail (g_type_is_a (viewable_type, GIMP_TYPE_VIEWABLE), NULL);
  g_return_val_if_fail (size >  0 &&
                        size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (border_width >= 0 &&
                        border_width <= GIMP_PREVIEW_MAX_BORDER_WIDTH, NULL);

  renderer = gimp_preview_renderer_new (viewable_type, size,
                                        border_width, is_popup);

  g_return_val_if_fail (renderer != NULL, NULL);

  preview = g_object_new (preview_type, NULL);

  g_signal_connect (renderer, "update",
                    G_CALLBACK (gimp_preview_update_callback),
                    preview);

  preview->renderer = renderer;

  return GTK_WIDGET (preview);
}

GtkWidget *
gimp_preview_new_full_by_types (GType    preview_type,
                                GType    viewable_type,
                                gint     width,
                                gint     height,
                                gint     border_width,
                                gboolean is_popup,
                                gboolean clickable,
                                gboolean show_popup)
{
  GimpPreviewRenderer *renderer;
  GimpPreview         *preview;

  g_return_val_if_fail (g_type_is_a (preview_type, GIMP_TYPE_PREVIEW), NULL);
  g_return_val_if_fail (g_type_is_a (viewable_type, GIMP_TYPE_VIEWABLE), NULL);
  g_return_val_if_fail (width >  0 &&
                        width <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (height >  0 &&
                        height <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (border_width >= 0 &&
                        border_width <= GIMP_PREVIEW_MAX_BORDER_WIDTH, NULL);

  renderer = gimp_preview_renderer_new_full (viewable_type, width, height,
                                             border_width, is_popup);

  g_return_val_if_fail (renderer != NULL, NULL);

  preview = g_object_new (preview_type, NULL);

  g_signal_connect (renderer, "update",
                    G_CALLBACK (gimp_preview_update_callback),
                    preview);

  preview->renderer   = renderer;
  preview->clickable  = clickable  ? TRUE : FALSE;
  preview->show_popup = show_popup ? TRUE : FALSE;

  return GTK_WIDGET (preview);
}

void
gimp_preview_set_viewable (GimpPreview  *preview,
			   GimpViewable *viewable)
{
  GType viewable_type = G_TYPE_NONE;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));
  g_return_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable));

  if (viewable == preview->viewable)
    return;

  if (viewable)
    {
      viewable_type = G_TYPE_FROM_INSTANCE (viewable);

      g_return_if_fail (g_type_is_a (viewable_type,
                                     preview->renderer->viewable_type));
    }

  if (preview->viewable)
    {
      g_object_remove_weak_pointer (G_OBJECT (preview->viewable),
				    (gpointer *) &preview->viewable);

      if (! viewable && ! preview->renderer->is_popup)
        {
          if (gimp_dnd_viewable_source_remove (GTK_WIDGET (preview),
                                               G_TYPE_FROM_INSTANCE (preview->viewable)))
            {
              gtk_drag_source_unset (GTK_WIDGET (preview));
            }
        }
    }
  else if (viewable && ! preview->renderer->is_popup)
    {
      if (gimp_dnd_drag_source_set_by_type (GTK_WIDGET (preview),
                                            GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                                            viewable_type,
                                            GDK_ACTION_COPY))
        {
          gimp_dnd_viewable_source_add (GTK_WIDGET (preview),
                                        viewable_type,
                                        gimp_preview_drag_viewable,
                                        NULL);
        }
    }

  gimp_preview_renderer_set_viewable (preview->renderer, viewable);
  preview->viewable = viewable;

  if (preview->viewable)
    {
      g_object_add_weak_pointer (G_OBJECT (preview->viewable),
				 (gpointer *) &preview->viewable);
    }
}

void
gimp_preview_set_expand (GimpPreview *preview,
                         gboolean     expand)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  if (preview->expand != expand)
    {
      preview->expand = expand ? TRUE : FALSE;
      gtk_widget_queue_resize (GTK_WIDGET (preview));
    }
}


/*  private functions  */

static void
gimp_preview_update_callback (GimpPreviewRenderer *renderer,
                              GimpPreview         *preview)
{
  GtkWidget *widget;
  gint       width;
  gint       height;

  widget = GTK_WIDGET (preview);

  width  = renderer->width  + 2 * renderer->border_width;
  height = renderer->height + 2 * renderer->border_width;

  if (width  != widget->requisition.width ||
      height != widget->requisition.height)
    {
      widget->requisition.width  = width;
      widget->requisition.height = height;

      gtk_widget_queue_resize (widget);
    }
  else
    {
      gtk_widget_queue_draw (widget);
    }
}

static GimpViewable *
gimp_preview_drag_viewable (GtkWidget *widget,
			    gpointer   data)
{
  return GIMP_PREVIEW (widget)->viewable;
}
