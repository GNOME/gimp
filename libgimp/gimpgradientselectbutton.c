/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmagradientselectbutton.c
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

#include "libligmawidgets/ligmawidgets.h"

#include "ligma.h"

#include "ligmauitypes.h"
#include "ligmagradientselectbutton.h"
#include "ligmauimarshal.h"

#include "libligma-intl.h"


/**
 * SECTION: ligmagradientselectbutton
 * @title: LigmaGradientSelectButton
 * @short_description: A button which pops up a gradient select dialog.
 *
 * A button which pops up a gradient select dialog.
 **/


#define CELL_HEIGHT 18
#define CELL_WIDTH  84

enum
{
  GRADIENT_SET,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_GRADIENT_NAME,
  N_PROPS
};


struct _LigmaGradientSelectButtonPrivate
{
  gchar     *title;

  gchar     *gradient_name;      /* Local copy */
  gint       sample_size;
  gboolean   reverse;
  gint       n_samples;
  gdouble   *gradient_data;      /* Local copy */

  GtkWidget *inside;
  GtkWidget *preview;
};


/*  local function prototypes  */

static void   ligma_gradient_select_button_finalize     (GObject      *object);

static void   ligma_gradient_select_button_set_property (GObject      *object,
                                                        guint         property_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);
static void   ligma_gradient_select_button_get_property (GObject      *object,
                                                        guint         property_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);

static void   ligma_gradient_select_button_clicked  (LigmaGradientSelectButton *button);

static void   ligma_gradient_select_button_callback (const gchar   *gradient_name,
                                                    gint           n_samples,
                                                    const gdouble *gradient_data,
                                                    gboolean       dialog_closing,
                                                    gpointer       user_data);

static void ligma_gradient_select_preview_size_allocate
                                                 (GtkWidget                *widget,
                                                  GtkAllocation            *allocation,
                                                  LigmaGradientSelectButton *button);
static gboolean ligma_gradient_select_preview_draw     (GtkWidget                *preview,
                                                       cairo_t                  *cr,
                                                       LigmaGradientSelectButton *button);

static void   ligma_gradient_select_drag_data_received (LigmaGradientSelectButton *button,
                                                       GdkDragContext           *context,
                                                       gint                      x,
                                                       gint                      y,
                                                       GtkSelectionData         *selection,
                                                       guint                     info,
                                                       guint                     time);

static GtkWidget * ligma_gradient_select_button_create_inside (LigmaGradientSelectButton *button);


static const GtkTargetEntry target = { "application/x-ligma-gradient-name", 0 };

static guint gradient_button_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *gradient_button_props[N_PROPS] = { NULL, };


G_DEFINE_TYPE_WITH_PRIVATE (LigmaGradientSelectButton,
                            ligma_gradient_select_button,
                            LIGMA_TYPE_SELECT_BUTTON)


static void
ligma_gradient_select_button_class_init (LigmaGradientSelectButtonClass *klass)
{
  GObjectClass          *object_class        = G_OBJECT_CLASS (klass);
  LigmaSelectButtonClass *select_button_class = LIGMA_SELECT_BUTTON_CLASS (klass);

  object_class->finalize     = ligma_gradient_select_button_finalize;
  object_class->set_property = ligma_gradient_select_button_set_property;
  object_class->get_property = ligma_gradient_select_button_get_property;

  select_button_class->select_destroy = ligma_gradient_select_destroy;

  klass->gradient_set = NULL;

  /**
   * LigmaGradientSelectButton:title:
   *
   * The title to be used for the gradient selection popup dialog.
   *
   * Since: 2.4
   */
  gradient_button_props[PROP_TITLE] = g_param_spec_string ("title",
                                                           "Title",
                                                           "The title to be used for the gradient selection popup dialog",
                                                           _("Gradient Selection"),
                                                           LIGMA_PARAM_READWRITE |
                                                           G_PARAM_CONSTRUCT_ONLY);

  /**
   * LigmaGradientSelectButton:gradient-name:
   *
   * The name of the currently selected gradient.
   *
   * Since: 2.4
   */
  gradient_button_props[PROP_GRADIENT_NAME] = g_param_spec_string ("gradient-name",
                                                                   "Gradient name",
                                                                   "The name of the currently selected gradient",
                                                                   NULL,
                                                                   LIGMA_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, gradient_button_props);

  /**
   * LigmaGradientSelectButton::gradient-set:
   * @widget: the object which received the signal.
   * @gradient_name: the name of the currently selected gradient.
   * @width: width of the gradient
   * @grad_data: gradient data
   * @dialog_closing: whether the dialog was closed or not.
   *
   * The ::gradient-set signal is emitted when the user selects a gradient.
   *
   * Since: 2.4
   */
  gradient_button_signals[GRADIENT_SET] =
    g_signal_new ("gradient-set",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaGradientSelectButtonClass, gradient_set),
                  NULL, NULL,
                  _ligmaui_marshal_VOID__STRING_INT_POINTER_BOOLEAN,
                  G_TYPE_NONE, 4,
                  G_TYPE_STRING,
                  G_TYPE_INT,
                  G_TYPE_POINTER,
                  G_TYPE_BOOLEAN);
}

static void
ligma_gradient_select_button_init (LigmaGradientSelectButton *button)
{
  button->priv = ligma_gradient_select_button_get_instance_private (button);

  button->priv->gradient_name = ligma_context_get_gradient ();
  button->priv->sample_size = CELL_WIDTH;
  button->priv->reverse = FALSE;

  button->priv->inside = ligma_gradient_select_button_create_inside (button);
  gtk_container_add (GTK_CONTAINER (button), button->priv->inside);
}

/**
 * ligma_gradient_select_button_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @gradient_name: (nullable): Initial gradient name.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a gradient.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_gradient_select_button_new (const gchar *title,
                                 const gchar *gradient_name)
{
  GtkWidget *button;

  if (title)
    button = g_object_new (LIGMA_TYPE_GRADIENT_SELECT_BUTTON,
                                    "title",         title,
                                    "gradient-name", gradient_name,
                                    NULL);
  else
    button = g_object_new (LIGMA_TYPE_GRADIENT_SELECT_BUTTON,
                                    "gradient-name", gradient_name,
                                    NULL);

  return button;
}

/**
 * ligma_gradient_select_button_get_gradient:
 * @button: A #LigmaGradientSelectButton
 *
 * Retrieves the name of currently selected gradient.
 *
 * Returns: an internal copy of the gradient name which must not be freed.
 *
 * Since: 2.4
 */
const gchar *
ligma_gradient_select_button_get_gradient (LigmaGradientSelectButton *button)
{
  g_return_val_if_fail (LIGMA_IS_GRADIENT_SELECT_BUTTON (button), NULL);

  return button->priv->gradient_name;
}

/**
 * ligma_gradient_select_button_set_gradient:
 * @button: A #LigmaGradientSelectButton
 * @gradient_name: (nullable): Gradient name to set.
 *
 * Sets the current gradient for the gradient select button.
 *
 * Since: 2.4
 */
void
ligma_gradient_select_button_set_gradient (LigmaGradientSelectButton *button,
                                          const gchar              *gradient_name)
{
  LigmaSelectButton *select_button;

  g_return_if_fail (LIGMA_IS_GRADIENT_SELECT_BUTTON (button));

  select_button = LIGMA_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      ligma_gradients_set_popup (select_button->temp_callback, gradient_name);
    }
  else
    {
      gchar   *name;
      gdouble *samples;
      gint     n_samples;

      if (gradient_name && *gradient_name)
        name = g_strdup (gradient_name);
      else
        name = ligma_context_get_gradient ();

      if (ligma_gradient_get_uniform_samples (name,
                                             button->priv->sample_size,
                                             button->priv->reverse,
                                             &n_samples,
                                             &samples))
        {
          ligma_gradient_select_button_callback (name,
                                                n_samples, samples,
                                                FALSE, button);

          g_free (samples);
        }

      g_free (name);
    }
}


/*  private functions  */

static void
ligma_gradient_select_button_finalize (GObject *object)
{
  LigmaGradientSelectButton *button = LIGMA_GRADIENT_SELECT_BUTTON (object);

  g_clear_pointer (&button->priv->gradient_name, g_free);
  g_clear_pointer (&button->priv->gradient_data, g_free);
  g_clear_pointer (&button->priv->title,         g_free);

  G_OBJECT_CLASS (ligma_gradient_select_button_parent_class)->finalize (object);
}

static void
ligma_gradient_select_button_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  LigmaGradientSelectButton *button = LIGMA_GRADIENT_SELECT_BUTTON (object);

  switch (property_id)
    {
    case PROP_TITLE:
      button->priv->title = g_value_dup_string (value);
      break;

    case PROP_GRADIENT_NAME:
      ligma_gradient_select_button_set_gradient (button,
                                                g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_gradient_select_button_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  LigmaGradientSelectButton *button = LIGMA_GRADIENT_SELECT_BUTTON (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, button->priv->title);
      break;

    case PROP_GRADIENT_NAME:
      g_value_set_string (value, button->priv->gradient_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_gradient_select_button_callback (const gchar   *gradient_name,
                                      gint           n_samples,
                                      const gdouble *gradient_data,
                                      gboolean       dialog_closing,
                                      gpointer       user_data)
{
  LigmaGradientSelectButton *button        = user_data;
  LigmaSelectButton         *select_button = LIGMA_SELECT_BUTTON (button);

  g_free (button->priv->gradient_name);
  g_free (button->priv->gradient_data);

  button->priv->gradient_name = g_strdup (gradient_name);
  button->priv->n_samples     = n_samples;
  button->priv->gradient_data = g_memdup2 (gradient_data,
                                           n_samples * sizeof (gdouble));

  gtk_widget_queue_draw (button->priv->preview);

  if (dialog_closing)
    select_button->temp_callback = NULL;

  g_signal_emit (button, gradient_button_signals[GRADIENT_SET], 0,
                 gradient_name, n_samples, gradient_data, dialog_closing);
  g_object_notify_by_pspec (G_OBJECT (button), gradient_button_props[PROP_GRADIENT_NAME]);
}

static void
ligma_gradient_select_button_clicked (LigmaGradientSelectButton *button)
{
  LigmaSelectButton *select_button = LIGMA_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      /*  calling ligma_gradients_set_popup() raises the dialog  */
      ligma_gradients_set_popup (select_button->temp_callback,
                                button->priv->gradient_name);
    }
  else
    {
      select_button->temp_callback =
        ligma_gradient_select_new (button->priv->title,
                                  button->priv->gradient_name,
                                  button->priv->sample_size,
                                  ligma_gradient_select_button_callback,
                                  button, NULL);
    }
}

static void
ligma_gradient_select_preview_size_allocate (GtkWidget                *widget,
                                            GtkAllocation            *allocation,
                                            LigmaGradientSelectButton *button)
{
  gdouble *samples;
  gint     n_samples;

  if (ligma_gradient_get_uniform_samples (button->priv->gradient_name,
                                         allocation->width,
                                         button->priv->reverse,
                                         &n_samples,
                                         &samples))
    {
      g_free (button->priv->gradient_data);

      button->priv->sample_size   = allocation->width;
      button->priv->n_samples     = n_samples;
      button->priv->gradient_data = samples;
    }
}

static gboolean
ligma_gradient_select_preview_draw (GtkWidget                *widget,
                                   cairo_t                  *cr,
                                   LigmaGradientSelectButton *button)
{
  GtkAllocation    allocation;
  cairo_pattern_t *pattern;
  cairo_surface_t *surface;
  const gdouble   *src;
  guchar          *dest;
  gint             width;
  gint             x;

  src = button->priv->gradient_data;
  if (! src)
    return FALSE;

  gtk_widget_get_allocation (widget, &allocation);

  pattern = ligma_cairo_checkerboard_create (cr, LIGMA_CHECK_SIZE_SM, NULL, NULL);
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);

  cairo_paint (cr);

  width = button->priv->n_samples / 4;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, 1);

  for (x = 0, dest = cairo_image_surface_get_data (surface);
       x < width;
       x++, src += 4, dest += 4)
    {
      LigmaRGB color;
      guchar  r, g, b, a;

      ligma_rgba_set (&color, src[0], src[1], src[2], src[3]);
      ligma_rgba_get_uchar (&color, &r, &g, &b, &a);

      LIGMA_CAIRO_ARGB32_SET_PIXEL(dest, r, g, b, a);
    }

  cairo_surface_mark_dirty (surface);

  pattern = cairo_pattern_create_for_surface (surface);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REFLECT);
  cairo_surface_destroy (surface);

  cairo_scale (cr, (gdouble) allocation.width / (gdouble) width, 1.0);

  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);

  cairo_paint (cr);

  return FALSE;
}

static void
ligma_gradient_select_drag_data_received (LigmaGradientSelectButton *button,
                                        GdkDragContext            *context,
                                        gint                       x,
                                        gint                       y,
                                        GtkSelectionData          *selection,
                                        guint                      info,
                                        guint                      time)
{
  gint   length = gtk_selection_data_get_length (selection);
  gchar *str;

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("%s: received invalid gradient data", G_STRFUNC);
      return;
    }

  str = g_strndup ((const gchar *) gtk_selection_data_get_data (selection),
                   length);

  if (g_utf8_validate (str, -1, NULL))
    {
      gint     pid;
      gpointer unused;
      gint     name_offset = 0;

      if (sscanf (str, "%i:%p:%n", &pid, &unused, &name_offset) >= 2 &&
          pid == ligma_getpid () && name_offset > 0)
        {
          gchar *name = str + name_offset;

          ligma_gradient_select_button_set_gradient (button, name);
        }
    }

  g_free (str);
}

static GtkWidget *
ligma_gradient_select_button_create_inside (LigmaGradientSelectButton *gradient_button)
{
  LigmaGradientSelectButtonPrivate *priv = gradient_button->priv;
  GtkWidget                       *button;

  button = gtk_button_new ();

  priv->preview = gtk_drawing_area_new ();
  gtk_widget_set_size_request (priv->preview, CELL_WIDTH, CELL_HEIGHT);
  gtk_container_add (GTK_CONTAINER (button), priv->preview);

  g_signal_connect (priv->preview, "size-allocate",
                    G_CALLBACK (ligma_gradient_select_preview_size_allocate),
                    gradient_button);

  g_signal_connect (priv->preview, "draw",
                    G_CALLBACK (ligma_gradient_select_preview_draw),
                    gradient_button);

  gtk_widget_show_all (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (ligma_gradient_select_button_clicked),
                            gradient_button);

  gtk_drag_dest_set (GTK_WIDGET (button),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect_swapped (button, "drag-data-received",
                            G_CALLBACK (ligma_gradient_select_drag_data_received),
                            gradient_button);

  return button;
}
