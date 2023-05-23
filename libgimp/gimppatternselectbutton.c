/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppatternselectbutton.c
 * Copyright (C) 1998 Andy Thomas
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

/* Similar to gimpbrushselectbutton.c.
 * FUTURE: inherit or share common code.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimppatternselectbutton.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimppatternselectbutton
 * @title: GimpPatternSelectButton
 * @short_description: A button which pops up a pattern selection dialog.
 *
 * A button which pops up a pattern selection dialog.
 **/

#define CELL_SIZE 20

/* An image. pixelels is allocated and must be freed. */
typedef struct _PreviewImage
{
  gint           width;
  gint           height;
  gint           bpp;
  guchar        *pixelels;
} _PreviewImage;

struct _GimpPatternSelectButton
{
  /* !! Not a pointer, is contained. */
  GimpResourceSelectButton parent_instance;

  /* Partial view of pattern image. Receives drag.  Receives mouse down to popup zoom. */
  GtkWidget     *peephole_view;
  GtkWidget     *popup;         /* Popup showing entire, full-size pattern image. */
  GtkWidget     *browse_button; /* Clickable area that popups remote chooser. */
};

/*  local  */

/* implement virtual. */
static void gimp_pattern_select_button_finalize        (GObject                  *object);
static void gimp_pattern_select_button_draw_interior   (GimpResourceSelectButton *self);

/* init methods. */
static GtkWidget *gimp_pattern_select_button_create_interior (GimpPatternSelectButton *self);

/* Event handlers. */
static void     gimp_pattern_select_on_preview_resize  (GimpPatternSelectButton *button);
static gboolean gimp_pattern_select_on_preview_events  (GtkWidget               *widget,
                                                        GdkEvent                *event,
                                                        GimpPatternSelectButton *button);

/* local drawing methods. */
static void     gimp_pattern_select_preview_draw      (GimpPreviewArea      *area,
                                                       gint                  x,
                                                       gint                  y,
                                                       _PreviewImage         image,
                                                       gint                  rowstride);
static void     gimp_pattern_select_preview_fill_draw (GtkWidget             *preview,
                                                       _PreviewImage         image);

static void          gimp_pattern_select_button_draw              (GimpPatternSelectButton *self);
static _PreviewImage gimp_pattern_select_button_get_pattern_image (GimpPatternSelectButton *self);

/* Popup methods. */
static void     gimp_pattern_select_button_open_popup  (GimpPatternSelectButton *button,
                                                        gint                     x,
                                                        gint                     y);
static void     gimp_pattern_select_button_close_popup (GimpPatternSelectButton *button);




/* A GtkTargetEntry has a string and two ints.
 * This is one, but we treat it as an array.
 */
static const GtkTargetEntry drag_target = { "application/x-gimp-pattern-name", 0, 0 };

G_DEFINE_FINAL_TYPE (GimpPatternSelectButton,
                     gimp_pattern_select_button,
                     GIMP_TYPE_RESOURCE_SELECT_BUTTON)


static void
gimp_pattern_select_button_class_init (GimpPatternSelectButtonClass *klass)
{
  /* Alias cast klass to super classes. */
  GObjectClass                  *object_class  = G_OBJECT_CLASS (klass);
  GimpResourceSelectButtonClass *superclass    = GIMP_RESOURCE_SELECT_BUTTON_CLASS (klass);

  g_debug ("%s called", G_STRFUNC);

  object_class->finalize     = gimp_pattern_select_button_finalize;

  /* Implement pure virtual functions. */
  superclass->draw_interior = gimp_pattern_select_button_draw_interior;

  /* Set data member of class. */
  superclass->resource_type = GIMP_TYPE_PATTERN;

  /* We don't define property getter/setters: use superclass getter/setters.
   * But super property name is "resource", not "pattern"
   */
}

static void
gimp_pattern_select_button_init (GimpPatternSelectButton *self)
{
  GtkWidget *interior;

  g_debug ("%s called", G_STRFUNC);

  /* Specialize:
   *     - embed our widget interior instance to super widget instance.
   *     - connect our widget to dnd
   * These will call superclass methods.
   * These are on instance, not our subclass.
   */
  interior = gimp_pattern_select_button_create_interior (self);

  gimp_resource_select_button_embed_interior (GIMP_RESOURCE_SELECT_BUTTON (self), interior);

  /* Self knows the GtkTargetEntry, super knows how to create target and receive drag. */
  /* Only the peephole_view receives drag. */
  gimp_resource_select_button_set_drag_target (GIMP_RESOURCE_SELECT_BUTTON (self),
                                               self->peephole_view,
                                               &drag_target);

  /* Tell super which sub widget pops up remote chooser.
   * Only the browse_button.
   * A click in the peephole_view does something else: popup a zoom.
   */
  gimp_resource_select_button_set_clickable (GIMP_RESOURCE_SELECT_BUTTON (self),
                                             self->browse_button);
}

/**
 * gimp_pattern_select_button_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @resource: (nullable): Initial pattern.
 *
 * Creates a new #GtkWidget that lets a user choose a pattern.
 * You can put this widget in a table in a plug-in dialog.
 *
 * When pattern is NULL, initial choice is from context.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_pattern_select_button_new (const gchar  *title,
                             GimpResource *resource)
{
  GtkWidget *self;

  g_debug ("%s called", G_STRFUNC);

  if (resource == NULL)
    {
      g_debug ("%s defaulting pattern from context", G_STRFUNC);
      resource = GIMP_RESOURCE (gimp_context_get_pattern ());
    }
  g_assert (resource != NULL);
  /* This method is polymorphic, so a factory can call it, but requires Pattern. */
  g_return_val_if_fail (GIMP_IS_PATTERN (resource), NULL);

  /* Create instance of self (not super.)
   * This will call superclass init, self class init, superclass init, and instance init.
   * Self subclass class_init will specialize by implementing virtual funcs
   * that open and set remote chooser dialog, and that draw self interior.
   *
   * !!! property belongs to superclass and is named "resource"
   */
   if (title)
     self = g_object_new (GIMP_TYPE_PATTERN_SELECT_BUTTON,
                          "title",     title,
                          "resource",  resource,
                          NULL);
   else
     self = g_object_new (GIMP_TYPE_PATTERN_SELECT_BUTTON,
                          "resource",  resource,
                          NULL);

  /* We don't subscribe to events from super.
   * Super will queue a draw when it's resource changes.
   * Except that the above setting of property happens too late,
   * so we now draw the initial resource.
   */

  /* Draw it with the initial resource.
   * Easier to call draw method than to queue a draw.
   *
   * Cast self from Widget.
   */
  gimp_pattern_select_button_draw_interior (GIMP_RESOURCE_SELECT_BUTTON (self));

  g_debug ("%s returns", G_STRFUNC);

  return self;
}


/* Getter and setter.
 * We could omit these, and use only the superclass methods.
 * But script-fu-interface.c uses these, until FUTURE.
 */

/**
 * gimp_pattern_select_button_get_pattern:
 * @self: A #GimpPatternSelectButton
 *
 * Gets the currently selected pattern.
 *
 * Returns: (transfer none): an internal copy of the pattern which must not be freed.
 *
 * Since: 2.4
 */
GimpPattern *
gimp_pattern_select_button_get_pattern (GimpPatternSelectButton *self)
{
  g_return_val_if_fail (GIMP_IS_PATTERN_SELECT_BUTTON (self), NULL);

  /* Delegate to super w upcast arg and downcast result. */
  return (GimpPattern *) gimp_resource_select_button_get_resource ((GimpResourceSelectButton*) self);
}

/**
 * gimp_pattern_select_button_set_pattern:
 * @self: A #GimpPatternSelectButton
 * @pattern: Pattern to set.
 *
 * Sets the currently selected pattern.
 * Usually you should not call this; the user is in charge.
 * Changes the selection in both the button and it's popup chooser.
 *
 * Since: 2.4
 */
void
gimp_pattern_select_button_set_pattern (GimpPatternSelectButton *self,
                                  GimpPattern             *pattern)
{
  g_return_if_fail (GIMP_IS_PATTERN_SELECT_BUTTON (self));

  g_debug ("%s", G_STRFUNC);

  /* Delegate to super with upcasts */
  gimp_resource_select_button_set_resource (GIMP_RESOURCE_SELECT_BUTTON (self), GIMP_RESOURCE (pattern));
}


/*  private functions  */

static void
gimp_pattern_select_button_finalize (GObject *object)
{
  g_debug ("%s called", G_STRFUNC);

  /* Has no allocations.*/

  /* Chain up. */
  G_OBJECT_CLASS (gimp_pattern_select_button_parent_class)->finalize (object);
}


/* Create a widget that is the interior of another widget, a GimpPatternSelectButton.
 * Super creates the exterior, self creates interior.
 * Exterior is-a container and self calls super to add interior to the container.
 */
static GtkWidget *
gimp_pattern_select_button_create_interior (GimpPatternSelectButton *self)
{
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *peephole_view;
  GtkWidget *browse_button;

  g_debug ("%s", G_STRFUNC);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  peephole_view = gimp_preview_area_new ();
  gtk_widget_add_events (peephole_view,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_set_size_request (peephole_view, CELL_SIZE, CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), peephole_view);

  g_signal_connect_swapped (peephole_view, "size-allocate",
                            G_CALLBACK (gimp_pattern_select_on_preview_resize),
                            self);

  /* preview receives mouse events to popup and down. */
  g_signal_connect (peephole_view, "event",
                    G_CALLBACK (gimp_pattern_select_on_preview_events),
                    self);

  browse_button = gtk_button_new_with_mnemonic (_("_Browse..."));
  gtk_box_pack_start (GTK_BOX (hbox), browse_button, TRUE, TRUE, 0);

  gtk_widget_show_all (hbox);

  /* Ensure self knows sub widgets. */
  self->peephole_view = peephole_view;
  self->browse_button = browse_button;

  /* Return the whole, it is not a button, only contains a button. */
  return hbox;
}


/* Knows how to draw self interior.
 * Self knows resource, it is not passed.
 *
 * Overrides virtual method in super, so it is generic on Resource.
 */
static void
gimp_pattern_select_button_draw_interior (GimpResourceSelectButton *self)
{
  g_debug ("%s", G_STRFUNC);

  g_return_if_fail (GIMP_IS_PATTERN_SELECT_BUTTON (self));

  gimp_pattern_select_button_draw (GIMP_PATTERN_SELECT_BUTTON (self));
}


/* Draw self.
 *
 * This knows that we only draw the peephole_view. Gtk draws the browse button.
 */
static void
gimp_pattern_select_button_draw (GimpPatternSelectButton *self)
{
  _PreviewImage image;

  g_debug ("%s", G_STRFUNC);

  /* Draw the peephole. */
  image = gimp_pattern_select_button_get_pattern_image (self);
  gimp_pattern_select_preview_fill_draw (self->peephole_view, image);
  g_free (image.pixelels);
}



/* Return the image of self's pattern.
 * Caller must free pixelels.
 */
static _PreviewImage
gimp_pattern_select_button_get_pattern_image (GimpPatternSelectButton *self)
{
  GimpPattern    *pattern;
  gsize           pixelels_size;
  GBytes         *color_bytes;
  _PreviewImage   result;

  g_debug ("%s", G_STRFUNC);

  g_object_get (self, "resource", &pattern, NULL);

  gimp_pattern_get_pixels (pattern,
                           &result.width,
                           &result.height,
                           &result.bpp,
                           &color_bytes);
  result.pixelels = g_bytes_unref_to_data (color_bytes, &pixelels_size);

  return result;
}


/* On resize event, redraw. */
static void
gimp_pattern_select_on_preview_resize (GimpPatternSelectButton *self)
{
  g_debug ("%s", G_STRFUNC);

  gimp_pattern_select_button_draw (self);
}


/* On mouse events in self's peephole_view, popup a zoom view of entire pattern */
static gboolean
gimp_pattern_select_on_preview_events (GtkWidget               *widget,
                                       GdkEvent                *event,
                                       GimpPatternSelectButton *self)
{
  GdkEventButton *bevent;

  g_debug ("%s", G_STRFUNC);

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
        {
          gtk_grab_add (widget);
          gimp_pattern_select_button_open_popup (self,
                                               bevent->x, bevent->y);
        }
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
        {
          gtk_grab_remove (widget);
          gimp_pattern_select_button_close_popup (self);
        }
      break;

    default:
      break;
    }

  return FALSE;
}

/* Draw a GimpPreviewArea with a given image. */
static void
gimp_pattern_select_preview_draw (GimpPreviewArea *area,
                                  gint             x,
                                  gint             y,
                                  _PreviewImage    image,
                                  gint             rowstride)
{
  GimpImageType type;

  g_debug ("%s", G_STRFUNC);

  switch (image.bpp)
    {
    case 1:  type = GIMP_GRAY_IMAGE;   break;
    case 2:  type = GIMP_GRAYA_IMAGE;  break;
    case 3:  type = GIMP_RGB_IMAGE;    break;
    case 4:  type = GIMP_RGBA_IMAGE;   break;
    default:
      g_warning ("Invalid bpp");
      return;
    }

  gimp_preview_area_draw (area,
                          x, y,
                          image.width, image.height,
                          type,
                          image.pixelels,
                          rowstride);
}

/* Fill a GimpPreviewArea with a image then draw. */
static void
gimp_pattern_select_preview_fill_draw (GtkWidget      *preview,
                                       _PreviewImage  image)
{
  GimpPreviewArea *area = GIMP_PREVIEW_AREA (preview);
  GtkAllocation    allocation;
  gint             x, y;
  gint             width, height;
  _PreviewImage    drawn_image;

  g_debug ("%s", G_STRFUNC);

  gtk_widget_get_allocation (preview, &allocation);

  width  = MIN (image.width,  allocation.width);
  height = MIN (image.height, allocation.height);

  x = ((allocation.width  - width)  / 2);
  y = ((allocation.height - height) / 2);

  if (x || y)
    gimp_preview_area_fill (area,
                            0, 0,
                            allocation.width,
                            allocation.height,
                            0xFF, 0xFF, 0xFF);

  /* Draw same data to new bounds.
   * drawn_image.pixelels points to same array.
   */
  drawn_image.width = width;
  drawn_image.height = height;
  drawn_image.pixelels = image.pixelels;
  drawn_image.bpp = image.bpp;

  gimp_pattern_select_preview_draw (area,
                                    x, y,
                                    drawn_image,
                                    image.width * image.bpp );  /* row stride */
  /* Caller will free image.pixelels */
}

/* popup methods. */

static void
gimp_pattern_select_button_open_popup (GimpPatternSelectButton *self,
                                       gint                     x,
                                       gint                     y)
{
  GtkWidget    *frame;
  GtkWidget    *preview;
  GdkMonitor   *monitor;
  GdkRectangle  workarea;
  gint          x_org;
  gint          y_org;
  _PreviewImage image;

  g_debug ("%s", G_STRFUNC);

  if (self->popup)
    gimp_pattern_select_button_close_popup (self);

  image = gimp_pattern_select_button_get_pattern_image (self);

  if (image.width <= CELL_SIZE && image.height <= CELL_SIZE)
    {
      g_debug ("%s: omit popup smaller than peephole.", G_STRFUNC);
      return;
    }

  self->popup = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (self->popup), GDK_WINDOW_TYPE_HINT_DND);
  gtk_window_set_screen (GTK_WINDOW (self->popup),
                         gtk_widget_get_screen (GTK_WIDGET (self)));

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (self->popup), frame);
  gtk_widget_show (frame);

  preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview, image.width, image.height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  /* decide where to put the popup: near the peephole_view i.e. at mousedown coords */
  gdk_window_get_origin (gtk_widget_get_window (self->peephole_view),
                         &x_org, &y_org);

  monitor = gimp_widget_get_monitor (GTK_WIDGET (self));
  gdk_monitor_get_workarea (monitor, &workarea);

  x = x_org + x - (image.width  / 2);
  y = y_org + y - (image.height / 2);

  x = CLAMP (x, workarea.x, workarea.x + workarea.width  - image.width);
  y = CLAMP (y, workarea.y, workarea.y + workarea.height - image.height);

  gtk_window_move (GTK_WINDOW (self->popup), x, y);

  gtk_widget_show (self->popup);

  /*  Draw popup now. Usual events do not cause a draw. */
  gimp_pattern_select_preview_draw (GIMP_PREVIEW_AREA (preview),
                                    0, 0,
                                    image,
                                    image.width * image.bpp);
  g_free (image.pixelels);
}

static void
gimp_pattern_select_button_close_popup (GimpPatternSelectButton *self)
{
  g_debug ("%s", G_STRFUNC);

  g_clear_pointer (&self->popup, gtk_widget_destroy);
}
