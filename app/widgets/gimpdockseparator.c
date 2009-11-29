/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockseparator.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#undef GSEAL_ENABLE

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpdockseparator.h"

#include "gimp-intl.h"


#define DEFAULT_HEIGHT 6
#define LABEL_PADDING  4

#define HELP_TEXT      _("You can drop dockable dialogs here")


struct _GimpDockSeparatorPrivate
{
  GimpPanedBoxDroppedFunc  dropped_cb;
  gpointer                *dropped_cb_data;

  GtkWidget               *frame;
  GtkWidget               *label;

  GtkAnchorType            anchor;
};


static void     gimp_dock_separator_style_set     (GtkWidget      *widget,
                                                   GtkStyle       *prev_style);
static void     gimp_dock_separator_size_allocate (GtkWidget      *widget,
                                                   GtkAllocation  *allocation);
static void     gimp_dock_separator_drag_leave    (GtkWidget      *widget,
                                                   GdkDragContext *context,
                                                   guint           time);
static gboolean gimp_dock_separator_drag_motion   (GtkWidget      *widget,
                                                   GdkDragContext *context,
                                                   gint            x,
                                                   gint            y,
                                                   guint           time);
static gboolean gimp_dock_separator_drag_drop     (GtkWidget      *widget,
                                                   GdkDragContext *context,
                                                   gint            x,
                                                   gint            y,
                                                   guint           time);


G_DEFINE_TYPE (GimpDockSeparator, gimp_dock_separator, GTK_TYPE_EVENT_BOX)

#define parent_class gimp_dock_separator_parent_class

static const GtkTargetEntry dialog_target_table[] = { GIMP_TARGET_DIALOG };


static void
gimp_dock_separator_class_init (GimpDockSeparatorClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->style_set     = gimp_dock_separator_style_set;
  widget_class->size_allocate = gimp_dock_separator_size_allocate;
  widget_class->drag_leave    = gimp_dock_separator_drag_leave;
  widget_class->drag_motion   = gimp_dock_separator_drag_motion;
  widget_class->drag_drop     = gimp_dock_separator_drag_drop;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("height",
                                                             NULL, NULL,
                                                             0, G_MAXINT,
                                                             DEFAULT_HEIGHT,
                                                             GIMP_PARAM_READABLE));

  g_type_class_add_private (klass, sizeof (GimpDockSeparatorPrivate));
}

static void
gimp_dock_separator_init (GimpDockSeparator *separator)
{
  separator->p = G_TYPE_INSTANCE_GET_PRIVATE (separator,
                                              GIMP_TYPE_DOCK_SEPARATOR,
                                              GimpDockSeparatorPrivate);

  separator->p->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (separator->p->frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (separator), separator->p->frame);
  gtk_widget_show (separator->p->frame);

  gimp_help_set_help_data (GTK_WIDGET (separator),
                           HELP_TEXT, GIMP_HELP_DOCK_SEPARATOR);

  gtk_drag_dest_set (GTK_WIDGET (separator),
                     GTK_DEST_DEFAULT_ALL,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);
}

static void
gimp_dock_separator_style_set (GtkWidget *widget,
                               GtkStyle  *prev_style)
{
  gint height;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget, "height", &height, NULL);

  gtk_widget_set_size_request (widget, -1, height);
}

static void
gimp_dock_separator_size_allocate (GtkWidget     *widget,
                                   GtkAllocation *allocation)
{
  GimpDockSeparator *separator = GIMP_DOCK_SEPARATOR (widget);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (separator->p->label)
    {
      gint width = separator->p->frame->allocation.width - 2 * LABEL_PADDING;

      gtk_widget_set_size_request (separator->p->label, width, -1);
    }
}

static void
gimp_dock_separator_drag_leave (GtkWidget      *widget,
                                GdkDragContext *context,
                                guint           time)
{
  if (GTK_WIDGET_CLASS (parent_class)->drag_leave)
    GTK_WIDGET_CLASS (parent_class)->drag_leave (widget, context, time);

  gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, NULL);
}

static gboolean
gimp_dock_separator_drag_motion (GtkWidget      *widget,
                                 GdkDragContext *context,
                                 gint            x,
                                 gint            y,
                                 guint           time)
{
  GdkColor *color;

  if (GTK_WIDGET_CLASS (parent_class)->drag_motion)
    GTK_WIDGET_CLASS (parent_class)->drag_motion (widget, context, x, y, time);

  color = gtk_widget_get_style (widget)->bg + GTK_STATE_SELECTED;

  gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, color);

  return FALSE;
}

static gboolean
gimp_dock_separator_drag_drop (GtkWidget      *widget,
                               GdkDragContext *context,
                               gint            x,
                               gint            y,
                               guint           time)
{
  GimpDockSeparator *separator = GIMP_DOCK_SEPARATOR (widget);
  GtkWidget         *source;

  if (GTK_WIDGET_CLASS (parent_class)->drag_drop)
    GTK_WIDGET_CLASS (parent_class)->drag_drop (widget, context, x, y, time);

  if (! separator->p->dropped_cb)
    return FALSE;

  source = gtk_drag_get_source_widget (context);

  if (source)
    {
      return separator->p->dropped_cb (source,
                                       gimp_dock_separator_get_insert_pos (separator),
                                       separator->p->dropped_cb_data);
    }

  return FALSE;
}


/*  public functions  */

GtkWidget *
gimp_dock_separator_new (GtkAnchorType anchor)
{
  GimpDockSeparator *separator;

  separator = g_object_new (GIMP_TYPE_DOCK_SEPARATOR, NULL);

  separator->p->anchor = anchor;

  return GTK_WIDGET (separator);
}

void
gimp_dock_separator_set_dropped_cb (GimpDockSeparator       *separator,
                                    GimpPanedBoxDroppedFunc  dropped_cb,
                                    gpointer                 dropped_cb_data)
{
  g_return_if_fail (GIMP_IS_DOCK_SEPARATOR (separator));

  separator->p->dropped_cb      = dropped_cb;
  separator->p->dropped_cb_data = dropped_cb_data;
}

/**
 * gimp_dock_separator_get_index_pos:
 * @separator:
 *
 * Returns: The insert position the separator represents.
 **/
gint
gimp_dock_separator_get_insert_pos (GimpDockSeparator *separator)
{
  g_return_val_if_fail (GIMP_IS_DOCK_SEPARATOR (separator), GTK_ANCHOR_CENTER);

  switch (separator->p->anchor)
    {
    case GTK_ANCHOR_NORTH:
    case GTK_ANCHOR_WEST:
      return 0;

    case GTK_ANCHOR_SOUTH:
    case GTK_ANCHOR_EAST:
      return -1;
      
    default:
      g_assert_not_reached ();
    }
}

void
gimp_dock_separator_set_show_label (GimpDockSeparator *separator,
                                    gboolean           show)
{
  g_return_if_fail (GIMP_IS_DOCK_SEPARATOR (separator));

  if (show && ! separator->p->label)
    {
      separator->p->label = gtk_label_new (HELP_TEXT);
      gtk_misc_set_padding (GTK_MISC (separator->p->label),
                            LABEL_PADDING, LABEL_PADDING);
      gtk_label_set_line_wrap (GTK_LABEL (separator->p->label), TRUE);
      gtk_label_set_justify (GTK_LABEL (separator->p->label), GTK_JUSTIFY_CENTER);
      gimp_label_set_attributes (GTK_LABEL (separator->p->label),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                 -1);
      gtk_container_add (GTK_CONTAINER (separator->p->frame), separator->p->label);
      gtk_widget_show (separator->p->label);

      gimp_help_set_help_data (GTK_WIDGET (separator),
                               NULL, GIMP_HELP_DOCK_SEPARATOR);
    }
  else if (! show && separator->p->label)
    {
      gtk_container_remove (GTK_CONTAINER (separator->p->frame), separator->p->label);
      separator->p->label = NULL;

      gimp_help_set_help_data (GTK_WIDGET (separator),
                               HELP_TEXT, GIMP_HELP_DOCK_SEPARATOR);
    }
}
