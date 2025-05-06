/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradientchooser.c
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
#include "gimpgradientchooser.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimpgradientchooser
 * @title: GimpGradientChooser
 * @short_description: A button which pops up a gradient select dialog.
 *
 * A button which pops up a gradient select dialog.
 **/

/* Local data needed to draw preview. */
typedef struct
{
  gint     allocation_width;

  gdouble *data;
  gsize    n_samples;
} GimpGradientPreviewData;

struct _GimpGradientChooser
{
  GimpResourceChooser      parent_instance;
  GimpGradientPreviewData  local_grad_data;
  GtkWidget               *preview;
};


/*  local function prototypes  */

static void     gimp_gradient_chooser_finalize             (GObject             *object);

static void     gimp_gradient_chooser_draw_interior        (GimpResourceChooser *self);

static void     gimp_gradient_select_preview_size_allocate (GtkWidget           *widget,
                                                            GtkAllocation       *allocation,
                                                            GimpGradientChooser *self);
static gboolean gimp_gradient_select_preview_draw_handler  (GtkWidget           *preview,
                                                            cairo_t             *cr,
                                                            GimpGradientChooser *self);
static gboolean gimp_gradient_select_model_change_handler  (GimpGradientChooser *self,
                                                            GimpResource        *resource,
                                                            gboolean   is_closing);

static void     local_grad_data_free                        (GimpGradientChooser *self);
static gboolean local_grad_data_exists                      (GimpGradientChooser *self);
static gboolean local_grad_data_refresh                     (GimpGradientChooser *self,
                                                             GimpGradient        *gradient);
static void     local_grad_data_set_allocation_width        (GimpGradientChooser *self,
                                                             gint                width);


static const GtkTargetEntry drag_target = { "application/x-gimp-gradient-name", 0 };

G_DEFINE_FINAL_TYPE (GimpGradientChooser,
                     gimp_gradient_chooser,
                     GIMP_TYPE_RESOURCE_CHOOSER)

/* Initial dimensions of widget. */
#define CELL_HEIGHT 18
#define CELL_WIDTH  84

static void
gimp_gradient_chooser_class_init (GimpGradientChooserClass *klass)
{
  GimpResourceChooserClass *superclass = GIMP_RESOURCE_CHOOSER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  superclass->draw_interior = gimp_gradient_chooser_draw_interior;
  superclass->resource_type = GIMP_TYPE_GRADIENT;

  object_class->finalize    = gimp_gradient_chooser_finalize;
}

static void
gimp_gradient_chooser_init (GimpGradientChooser *self)
{
  GtkWidget *button;

  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (self), button);

  self->local_grad_data.data             = NULL;
  self->local_grad_data.n_samples        = 0;
  self->local_grad_data.allocation_width = 0;

  self->preview = gtk_drawing_area_new ();
  gtk_widget_set_size_request (self->preview, CELL_WIDTH, CELL_HEIGHT);
  gtk_container_add (GTK_CONTAINER (button), self->preview);

  g_signal_connect (self->preview, "size-allocate",
                    G_CALLBACK (gimp_gradient_select_preview_size_allocate),
                    self);

  g_signal_connect (self->preview, "draw",
                    G_CALLBACK (gimp_gradient_select_preview_draw_handler),
                    self);

  gtk_widget_show_all (GTK_WIDGET (self));

  _gimp_resource_chooser_set_drag_target (GIMP_RESOURCE_CHOOSER (self),
                                         self->preview, &drag_target);

  _gimp_resource_chooser_set_clickable (GIMP_RESOURCE_CHOOSER (self), button);

  gimp_gradient_chooser_draw_interior (GIMP_RESOURCE_CHOOSER (self));
}

/* Called when dialog is closed and owning ResourceSelect button is disposed. */
static void
gimp_gradient_chooser_finalize (GObject *object)
{
  GimpGradientChooser *self = GIMP_GRADIENT_CHOOSER (object);

  local_grad_data_free (self);

  /* chain up. */
  G_OBJECT_CLASS (gimp_gradient_chooser_parent_class)->finalize (object);
}

static void
gimp_gradient_chooser_draw_interior (GimpResourceChooser *self)
{
  GimpGradientChooser *gradient_select = GIMP_GRADIENT_CHOOSER (self);

  gtk_widget_queue_draw (gradient_select->preview);
}


/**
 * gimp_gradient_chooser_new:
 * @title:    (nullable): Title of the dialog to use or %NULL to use the default title.
 * @label:    (nullable): Button label or %NULL for no label.
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
gimp_gradient_chooser_new (const gchar  *title,
                           const gchar  *label,
                           GimpGradient *gradient)
{
  GtkWidget *self;

  g_return_val_if_fail (gradient == NULL || GIMP_IS_GRADIENT (gradient), NULL);

  if (gradient == NULL)
    gradient = gimp_context_get_gradient ();

  if (title)
    self = g_object_new (GIMP_TYPE_GRADIENT_CHOOSER,
                         "title",    title,
                         "label",    label,
                         "resource", gradient,
                         NULL);
  else
    self = g_object_new (GIMP_TYPE_GRADIENT_CHOOSER,
                         "label",    label,
                         "resource", gradient,
                         NULL);

  return self;
}


/*  private functions  */

/* Get array of samples from self's gradient.
 * Return array and size at given handles.
 * Return success.
 *
 * Crosses the wire to core.
 * Doesn't know we keep the data locally.
 */
static gboolean
get_gradient_data (GimpGradient  *gradient,
                   gint           allocation_width,
                   gsize         *sample_count,
                   gdouble      **sample_array)
{
  gboolean    result = FALSE;
  GeglColor **samples;

  samples = gimp_gradient_get_uniform_samples (gradient, allocation_width,
                                               FALSE  /* not reversed. */);

  if (samples)
    {
      gdouble *dsamples;

      /* Return array of samples to dereferenced handles. */
      *sample_count = gimp_color_array_get_length (samples);;
      *sample_array = dsamples = g_new0 (gdouble, *sample_count * 4);
      for (gint i = 0; samples[i] != NULL; i++)
        {
          gegl_color_get_pixel (samples[i], babl_format ("R'G'B'A double"), dsamples);
          dsamples += 4;
        }
      gimp_color_array_free (samples);

      result = TRUE;
    }

  /* When result is true, caller must free the array. */
  return result;
}


/* Called on widget resized. */
static void
gimp_gradient_select_preview_size_allocate (GtkWidget           *widget,
                                            GtkAllocation       *allocation,
                                            GimpGradientChooser *self)
{
  /* Width needed to draw preview. */
  local_grad_data_set_allocation_width (self, allocation->width);

  g_signal_handlers_disconnect_by_func (self,
                                        G_CALLBACK (gimp_gradient_select_model_change_handler),
                                        self);
  g_signal_connect (self, "resource-set",
                    G_CALLBACK (gimp_gradient_select_model_change_handler),
                    self);
}

/* Draw array of samples.
 * This understands mostly cairo, and little about gradient.
 *
 * src is a local copy of gradient data.
 */
static void
gimp_gradient_select_preview_draw (cairo_t *cr,
                                   gint     src_width,
                                   gint     dest_width,
                                   gdouble *src)
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
      GeglColor *color = gegl_color_new ("black");
      guchar     rgba[4];

      gegl_color_set_pixel (color, babl_format ("R'G'B'A double"), src);
      gegl_color_get_pixel (color, babl_format ("R'G'B'A u8"), rgba);

      GIMP_CAIRO_ARGB32_SET_PIXEL (dest, rgba[0], rgba[1], rgba[2], rgba[3]);
      g_object_unref (color);
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
gimp_gradient_select_preview_draw_handler (GtkWidget           *widget,
                                           cairo_t             *cr,
                                           GimpGradientChooser *self)
{
  /* Ensure local gradient data exists.
   * Draw from local data to avoid crossing wire on expose events.
   */
  if (! local_grad_data_exists (self))
    {
      /* This is first draw, self's resource is the initial one.
       * Cross wire to get local data.
       */
      GimpGradient *gradient = NULL;

      g_object_get (self, "resource", &gradient, NULL);
      if ( ! local_grad_data_refresh (self, gradient))
        {
          /* Failed to get data for initial gradient.  Return without drawing. */
          g_object_unref (gradient);
          return FALSE;
        }
      g_object_unref (gradient);
    }

  /* Width in pixels of src, since BPP is 4. */
  gimp_gradient_select_preview_draw (cr,
                                     self->local_grad_data.n_samples,
                                     self->local_grad_data.allocation_width,
                                     self->local_grad_data.data);

  return FALSE;
}

/* Handler for event resource-set.
 * From superclass, but ultimately from remote chooser.
 *
 * Event comes only when user touches the remote chooser.
 * Event not come when plugin dialog widget is exposed and needs redraw.
 */
static gboolean
gimp_gradient_select_model_change_handler (GimpGradientChooser *self,
                                           GimpResource        *resource,
                                           gboolean             is_closing)
{
  local_grad_data_refresh (self, GIMP_GRADIENT (resource));
  return FALSE;
  /* is_closing is not used, but handled upstream, in GimpResourceSelect. */
}


/* Methods for gradient data stored locally.
 *
 * Could be a class.
 * Mostly encapsulated except preview_draw_handler accesses it also.
 *
 * Store grad data locally.
 * To avoid an asynchronous call back to core
 * to get gradient data at redraw for expose events.
 * Such an asynch call can improperly interleave
 * with transactions in other direction, across wire from core to plugin.
 * This is not just for performance, i.e. not just to save wire crossings.
 */

static gboolean
local_grad_data_exists (GimpGradientChooser *self)
{
  return self->local_grad_data.data != NULL;
}

static void
local_grad_data_free (GimpGradientChooser *self)
{
  g_clear_pointer (&self->local_grad_data.data, g_free);
  self->local_grad_data.n_samples = 0;
}

/* Called at initial draw to get local data for the model gradient.
 * Also called when remote chooser sets a new gradient into model.
 *
 * Returns success in crossing the wire and looking up gradient.
 */
static gboolean
local_grad_data_refresh (GimpGradientChooser *self, GimpGradient *gradient)
{
  gdouble *src;
  gsize    n_samples;

  /* Must not be called before widget is allocated. */
  g_assert (self->local_grad_data.allocation_width != 0);

  if (!get_gradient_data (gradient,
                          self->local_grad_data.allocation_width,
                          &n_samples,
                          &src))
    {
      g_warning ("Failed get gradient data");
      return FALSE;
    }
  else
    {
      local_grad_data_free (self);
      self->local_grad_data.data = src;
      self->local_grad_data.n_samples = n_samples;
      return TRUE;
    }
}

static void
local_grad_data_set_allocation_width (GimpGradientChooser *self, gint width)
{
  self->local_grad_data.allocation_width = width;
}
