/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradientselectbutton.c
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimpgradientselectbutton.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimpgradientselectbutton
 * @title: GimpGradientSelectButton
 * @short_description: A button which pops up a gradient select dialog.
 *
 * A button which pops up a gradient select dialog.
 **/


struct _GimpGradientSelectButton
{
  /* !! Not a pointer, is contained. */
  GimpResourceSelectButton parent_instance;

  GtkWidget *button;
  GtkWidget *drag_region_widget;
  GtkWidget *preview;             /* needed by draw. */
};

/*  local function prototypes  */

static void   gimp_gradient_select_button_finalize      (GObject                  *object);

static void gimp_gradient_select_button_embed_interior  (GimpGradientSelectButton *self);
static void gimp_gradient_select_button_draw_interior   (GimpResourceSelectButton *self);
static void gimp_gradient_select_button_set_drag_target (GimpGradientSelectButton *self);

static void gimp_gradient_select_preview_size_allocate  (GtkWidget                *widget,
                                                         GtkAllocation            *allocation,
                                                         GimpGradientSelectButton *self);
static gboolean gimp_gradient_select_preview_draw_handler (GtkWidget                *preview,
                                                           cairo_t                  *cr,
                                                           GimpGradientSelectButton *self);


static const GtkTargetEntry drag_target = { "application/x-gimp-gradient-name", 0 };

G_DEFINE_FINAL_TYPE (GimpGradientSelectButton,
                     gimp_gradient_select_button,
                     GIMP_TYPE_RESOURCE_SELECT_BUTTON)

/* Initial dimensions of widget. */
#define CELL_HEIGHT 18
#define CELL_WIDTH  84

static void
gimp_gradient_select_button_class_init (GimpGradientSelectButtonClass *klass)
{
  GObjectClass                  *object_class = G_OBJECT_CLASS (klass);
  GimpResourceSelectButtonClass *superclass   = GIMP_RESOURCE_SELECT_BUTTON_CLASS (klass);

  g_debug ("%s", G_STRFUNC);

  object_class->finalize     = gimp_gradient_select_button_finalize;

  /* Implement pure virtual functions. */
  superclass->draw_interior          = gimp_gradient_select_button_draw_interior;

  /* Set data member of class. */
  superclass->resource_type = GIMP_TYPE_GRADIENT;
}

static void
gimp_gradient_select_button_init (GimpGradientSelectButton *self)
{
  g_debug ("%s", G_STRFUNC);

  gimp_gradient_select_button_embed_interior (self);

  gimp_gradient_select_button_set_drag_target (self);

  gimp_resource_select_button_set_clickable (GIMP_RESOURCE_SELECT_BUTTON (self),
                                             self->button);
}

/**
 * gimp_gradient_select_button_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @gradient: (nullable): Initial gradient.
 *
 * Creates a new #GtkWidget that lets a user choose a gradient.
 * You can use this widget in a table in a plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_gradient_select_button_new (const gchar  *title,
                                 GimpResource *gradient)
{
  GtkWidget *self;

  if (gradient == NULL)
    {
      g_debug ("%s defaulting gradient from context", G_STRFUNC);
      gradient = GIMP_RESOURCE (gimp_context_get_gradient ());
    }
  g_assert (gradient != NULL);
  /* This method is polymorphic, so a factory can call it, but requires gradient. */
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);

  g_debug ("%s", G_STRFUNC);

  if (title)
    self = g_object_new (GIMP_TYPE_GRADIENT_SELECT_BUTTON,
                           "title",    title,
                           "resource", gradient,
                           NULL);
  else
    self = g_object_new (GIMP_TYPE_GRADIENT_SELECT_BUTTON,
                           "resource", gradient,
                           NULL);

  gimp_gradient_select_button_draw_interior (GIMP_RESOURCE_SELECT_BUTTON (self));
  return self;
}


/**
 * gimp_gradient_select_button_get_gradient:
 * @self: A #GimpGradientSelectButton
 *
 * Gets the currently selected gradient.
 *
 * Returns: (transfer none): an internal copy of the gradient, which you must not free.
 *
 * Since: 2.4
 */
GimpGradient *
gimp_gradient_select_button_get_gradient (GimpGradientSelectButton *self)
{
  g_debug ("%s", G_STRFUNC);

  g_return_val_if_fail (GIMP_IS_GRADIENT_SELECT_BUTTON (self), NULL);

  /* Delegate to super w upcast arg and downcast result. */
  return (GimpGradient *) gimp_resource_select_button_get_resource ((GimpResourceSelectButton*) self);
}


/**
 * gimp_gradient_select_button_set_gradient:
 * @self: A #GimpGradientSelectButton
 * @gradient: (nullable): Gradient to set.
 *
 * Sets the current gradient for the gradient select button.
 *
 * Since: 2.4
 */
void
gimp_gradient_select_button_set_gradient (GimpGradientSelectButton *self,
                                          GimpGradient             *gradient)
{
  g_return_if_fail (GIMP_IS_GRADIENT_SELECT_BUTTON (self));

  g_debug ("%s", G_STRFUNC);

  /* Delegate to super with upcasts */
  gimp_resource_select_button_set_resource (GIMP_RESOURCE_SELECT_BUTTON (self), GIMP_RESOURCE (gradient));
}


/*  private functions  */

static void
gimp_gradient_select_button_finalize (GObject *object)
{
  g_debug ("%s", G_STRFUNC);

  /* Nothing was allocated. */

  G_OBJECT_CLASS (gimp_gradient_select_button_parent_class)->finalize (object);
}

static void
gimp_gradient_select_button_draw_interior (GimpResourceSelectButton *self)
{
  GimpGradientSelectButton  *self_as_gradient_select;

  g_debug ("%s", G_STRFUNC);

  g_return_if_fail (GIMP_IS_GRADIENT_SELECT_BUTTON (self));
  self_as_gradient_select = GIMP_GRADIENT_SELECT_BUTTON (self);

    /* Not actually draw, just queue. Continuation is the handler. */
  gtk_widget_queue_draw (self_as_gradient_select->preview);
}

/* Get array of samples from self's gradient.
 * Return array and size at given handles.
 * Return success.
 */
static gboolean
get_gradient_data (GimpGradientSelectButton *self,
                   gint                      allocation_width,
                   gint                     *sample_count,
                   gdouble                 **sample_array
                   )
{
  gboolean result;
  gdouble *samples;
  gint     n_samples;

  GimpGradient *gradient;

  g_debug ("%s", G_STRFUNC);

  /* Self's gradient is property "resource"
   * (resource field of super is private.)
   */
  g_object_get (self, "resource", &gradient, NULL);
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), FALSE);

  result = gimp_gradient_get_uniform_samples (
    gradient,
    allocation_width,
    FALSE,  /* not reversed. */
    &n_samples,
    &samples);

  if (result)
    {
      /* Return array of samples to dereferenced handles. */
      *sample_array = samples;
      *sample_count = n_samples;
    }

  /* When result is true, caller must free the array. */
  return result;
}


/* Called on widget resized. */
static void
gimp_gradient_select_preview_size_allocate (GtkWidget                *widget,
                                            GtkAllocation            *allocation,
                                            GimpGradientSelectButton *self)
{
  g_debug ("%s", G_STRFUNC);

  /* Do nothing.
   *
   * In former code, we cached the gradient data in self, on allocate event.
   * But allocate event always seems to be paired with a draw event,
   * so there is no point in caching the gradient data.
   * And caching gradient data is a premature optimization,
   * without deep knowledge of Gtk and actual performance testing,
   * you can't know caching helps performance.
   */
}



/* Draw array of samples.
 * This understands mostly cairo, and little about gradient.
 */
static void
gimp_gradient_select_preview_draw (
  cairo_t   *cr,
  gint       src_width,
  gint       dest_width,
  gdouble   *src)
{
  cairo_pattern_t *pattern;
  cairo_surface_t *surface;
  guchar          *dest;
  gint             x;

  pattern = gimp_cairo_checkerboard_create (cr, GIMP_CHECK_SIZE_SM, NULL, NULL);
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);

  cairo_paint (cr);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, src_width, 1);

  for (x = 0, dest = cairo_image_surface_get_data (surface);
       x < src_width;
       x++, src += 4, dest += 4)
    {
      GimpRGB color;
      guchar  r, g, b, a;

      gimp_rgba_set (&color, src[0], src[1], src[2], src[3]);
      gimp_rgba_get_uchar (&color, &r, &g, &b, &a);

      GIMP_CAIRO_ARGB32_SET_PIXEL (dest, r, g, b, a);
    }

  cairo_surface_mark_dirty (surface);

  pattern = cairo_pattern_create_for_surface (surface);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REFLECT);
  cairo_surface_destroy (surface);

  cairo_scale (cr, (gdouble) dest_width / (gdouble) src_width, 1.0);

  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);

  cairo_paint (cr);
}



/* Handles a draw signal.
 * Draw self, i.e. interior of button.
 *
 * Always returns FALSE, but doesn't draw when fail to get gradient data.
 *
 * Is passed neither gradient nor attributes of gradient: get them now from self.
 */
static gboolean
gimp_gradient_select_preview_draw_handler (
  GtkWidget                *widget,
  cairo_t                  *cr,
  GimpGradientSelectButton *self)
{
  GtkAllocation    allocation;

  /* Attributes of the source.*/
  gdouble   *src;
  gint       n_samples;
  gint       src_width;

  g_debug ("%s", G_STRFUNC);

  gtk_widget_get_allocation (widget, &allocation);

  if (!get_gradient_data (self, allocation.width, &n_samples, &src))
    return FALSE;

  /* Width in pixels of src, since BPP is 4. */
  src_width = n_samples / 4;

  gimp_gradient_select_preview_draw (cr, src_width, allocation.width, src);

  g_free (src);

  return FALSE;
}


static void
gimp_gradient_select_button_embed_interior (GimpGradientSelectButton *self)
{
  GtkWidget *button;
  GtkWidget *preview;

  g_debug ("%s", G_STRFUNC);

  /* Outermost is button. */
  button = gtk_button_new ();

  /* Entire interior of button is a preview image. */
  preview = gtk_drawing_area_new ();
  gtk_widget_set_size_request (preview, CELL_WIDTH, CELL_HEIGHT);
  gtk_container_add (GTK_CONTAINER (button), preview);

  g_signal_connect (preview, "size-allocate",
                    G_CALLBACK (gimp_gradient_select_preview_size_allocate),
                    self);

  g_signal_connect (preview, "draw",
                    G_CALLBACK (gimp_gradient_select_preview_draw_handler),
                    self);

  gtk_widget_show_all (button);

  /* Remember widgets needed later. */
  self->drag_region_widget = preview;
  self->button             = button;
  self->preview            = preview;

  /* Call super with upcasts. */
  gimp_resource_select_button_embed_interior (GIMP_RESOURCE_SELECT_BUTTON (self), button);
}

static void
gimp_gradient_select_button_set_drag_target (GimpGradientSelectButton *self)
{
  /* Self knows the GtkTargetEntry, super knows how to create target and receive drag. */
  gimp_resource_select_button_set_drag_target (GIMP_RESOURCE_SELECT_BUTTON (self),
                                               self->drag_region_widget,
                                               &drag_target);
}
