/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockseparator.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimpdock.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimpdockseparator.h"

#include "gimp-intl.h"


#define DEFAULT_HEIGHT 6
#define LABEL_PADDING  4

#define HELP_TEXT      _("You can drop dockable dialogs here")


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
}

static void
gimp_dock_separator_init (GimpDockSeparator *separator)
{
  separator->dock  = NULL;
  separator->label = NULL;

  separator->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (separator->frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (separator), separator->frame);
  gtk_widget_show (separator->frame);

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

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
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

  if (separator->label)
    {
      gint width = separator->frame->allocation.width - 2 * LABEL_PADDING;

      gtk_widget_set_size_request (separator->label, width, -1);
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

  source = gtk_drag_get_source_widget (context);

  if (source)
    {
      GimpDock     *dock = separator->dock;
      GimpDockable *dockable;

      if (GIMP_IS_DOCKABLE (source))
        dockable = GIMP_DOCKABLE (source);
      else
        dockable = g_object_get_data (G_OBJECT (source), "gimp-dockable");

      if (dockable)
        {
          GtkWidget *dockbook;
          GList     *children;
          gint       index;

          g_object_set_data (G_OBJECT (dockable),
                             "gimp-dock-drag-widget", NULL);

          children = gtk_container_get_children (GTK_CONTAINER (widget->parent));
          index = g_list_index (children, widget);
          g_list_free (children);

          if (index == 0)
            index = 0;
          else if (index == 2)
            index = -1;

          /*  if dropping to the same dock, take care that we don't try
           *  to reorder the *only* dockable in the dock
           */
          if (dockable->dockbook->dock == dock)
            {
              gint n_books;
              gint n_dockables;

              n_books = g_list_length (dock->dockbooks);

              children =
                gtk_container_get_children (GTK_CONTAINER (dockable->dockbook));
              n_dockables = g_list_length (children);
              g_list_free (children);

              if (n_books == 1 && n_dockables == 1)
                return TRUE; /* successfully do nothing */
            }

          g_object_ref (dockable);

          gimp_dockbook_remove (dockable->dockbook, dockable);

          dockbook = gimp_dockbook_new (dock->dialog_factory->menu_factory);
          gimp_dock_add_book (dock, GIMP_DOCKBOOK (dockbook), index);

          gimp_dockbook_add (GIMP_DOCKBOOK (dockbook), dockable, -1);

          g_object_unref (dockable);

          return TRUE;
        }
    }

  return FALSE;
}


/*  public functions  */

GtkWidget *
gimp_dock_separator_new (GimpDock *dock)
{
  GimpDockSeparator *separator;

  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  separator = g_object_new (GIMP_TYPE_DOCK_SEPARATOR, NULL);

  separator->dock = dock;

  return GTK_WIDGET (separator);
}

void
gimp_dock_separator_set_show_label (GimpDockSeparator *separator,
                                    gboolean           show)
{
  g_return_if_fail (GIMP_IS_DOCK_SEPARATOR (separator));

  if (show && ! separator->label)
    {
      separator->label = gtk_label_new (HELP_TEXT);
      gtk_misc_set_padding (GTK_MISC (separator->label),
                            LABEL_PADDING, LABEL_PADDING);
      gtk_label_set_line_wrap (GTK_LABEL (separator->label), TRUE);
      gtk_label_set_justify (GTK_LABEL (separator->label), GTK_JUSTIFY_CENTER);
      gimp_label_set_attributes (GTK_LABEL (separator->label),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                 -1);
      gtk_container_add (GTK_CONTAINER (separator->frame), separator->label);
      gtk_widget_show (separator->label);

      gimp_help_set_help_data (GTK_WIDGET (separator),
                               NULL, GIMP_HELP_DOCK_SEPARATOR);
    }
  else if (! show && separator->label)
    {
      gtk_container_remove (GTK_CONTAINER (separator->frame), separator->label);
      separator->label = NULL;

      gimp_help_set_help_data (GTK_WIDGET (separator),
                               HELP_TEXT, GIMP_HELP_DOCK_SEPARATOR);
    }
}
