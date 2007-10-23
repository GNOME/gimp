/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpgradientselectbutton.c
 * Copyright (C) 1998 Andy Thomas
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimpgradientselectbutton.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


#define CELL_HEIGHT 18
#define CELL_WIDTH  84


#define GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMP_TYPE_GRADIENT_SELECT_BUTTON, GimpGradientSelectButtonPrivate))

typedef struct _GimpGradientSelectButtonPrivate GimpGradientSelectButtonPrivate;

struct _GimpGradientSelectButtonPrivate
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

enum
{
  GRADIENT_SET,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_GRADIENT_NAME
};


/*  local function prototypes  */

static void   gimp_gradient_select_button_finalize     (GObject      *object);

static void   gimp_gradient_select_button_set_property (GObject      *object,
                                                        guint         property_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);
static void   gimp_gradient_select_button_get_property (GObject      *object,
                                                        guint         property_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);

static void   gimp_gradient_select_button_clicked  (GimpGradientSelectButton *button);

static void   gimp_gradient_select_button_callback (const gchar   *gradient_name,
                                                    gint           n_samples,
                                                    const gdouble *gradient_data,
                                                    gboolean       dialog_closing,
                                                    gpointer       user_data);

static void gimp_gradient_select_preview_size_allocate
                                                 (GtkWidget                *widget,
                                                  GtkAllocation            *allocation,
                                                  GimpGradientSelectButton *button);
static void gimp_gradient_select_preview_expose  (GtkWidget                *preview,
                                                  GdkEventExpose           *event,
                                                  GimpGradientSelectButton *button);

static void   gimp_gradient_select_drag_data_received (GimpGradientSelectButton *button,
                                                       GdkDragContext           *context,
                                                       gint                      x,
                                                       gint                      y,
                                                       GtkSelectionData         *selection,
                                                       guint                     info,
                                                       guint                     time);

static GtkWidget * gimp_gradient_select_button_create_inside (GimpGradientSelectButton *button);


static const GtkTargetEntry target = { "application/x-gimp-gradient-name", 0 };

static guint gradient_button_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (GimpGradientSelectButton, gimp_gradient_select_button,
               GIMP_TYPE_SELECT_BUTTON)


static void
gimp_gradient_select_button_class_init (GimpGradientSelectButtonClass *klass)
{
  GObjectClass          *object_class        = G_OBJECT_CLASS (klass);
  GimpSelectButtonClass *select_button_class = GIMP_SELECT_BUTTON_CLASS (klass);

  object_class->finalize     = gimp_gradient_select_button_finalize;
  object_class->set_property = gimp_gradient_select_button_set_property;
  object_class->get_property = gimp_gradient_select_button_get_property;

  select_button_class->select_destroy = gimp_gradient_select_destroy;

  klass->gradient_set = NULL;

  /**
   * GimpGradientSelectButton:title:
   *
   * The title to be used for the gradient selection popup dialog.
   *
   * Since: GIMP 2.4
   */
  g_object_class_install_property (object_class, PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "Title",
                                                        "The title to be used for the gradient selection popup dialog",
                                                        _("Gradient Selection"),
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /**
   * GimpGradientSelectButton:gradient-name:
   *
   * The name of the currently selected gradient.
   *
   * Since: GIMP 2.4
   */
  g_object_class_install_property (object_class, PROP_GRADIENT_NAME,
                                   g_param_spec_string ("gradient-name",
                                                        "Gradient name",
                                                        "The name of the currently selected gradient",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  /**
   * GimpGradientSelectButton::gradient-set:
   * @widget: the object which received the signal.
   * @gradient_name: the name of the currently selected gradient.
   * @width: width of the gradient
   * @grad_data: gradient data
   * @dialog_closing: whether the dialog was closed or not.
   *
   * The ::gradient-set signal is emitted when the user selects a gradient.
   *
   * Since: GIMP 2.4
   */
  gradient_button_signals[GRADIENT_SET] =
    g_signal_new ("gradient-set",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpGradientSelectButtonClass, gradient_set),
                  NULL, NULL,
                  _gimpui_marshal_VOID__STRING_INT_POINTER_BOOLEAN,
                  G_TYPE_NONE, 4,
                  G_TYPE_STRING,
                  G_TYPE_INT,
                  G_TYPE_POINTER,
                  G_TYPE_BOOLEAN);

  g_type_class_add_private (object_class,
                            sizeof (GimpGradientSelectButtonPrivate));
}

static void
gimp_gradient_select_button_init (GimpGradientSelectButton *button)
{
  GimpGradientSelectButtonPrivate *priv;

  priv = GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE (button);

  priv->gradient_name = gimp_context_get_gradient ();
  priv->sample_size = CELL_WIDTH;
  priv->reverse = FALSE;

  priv->inside = gimp_gradient_select_button_create_inside (button);
  gtk_container_add (GTK_CONTAINER (button), priv->inside);
}

/**
 * gimp_gradient_select_button_new:
 * @title:         Title of the dialog to use or %NULL to use the default title.
 * @gradient_name: Initial gradient name.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a gradient.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: GIMP 2.4
 */
GtkWidget *
gimp_gradient_select_button_new (const gchar *title,
                                 const gchar *gradient_name)
{
  GtkWidget *button;

  if (title)
    button = g_object_new (GIMP_TYPE_GRADIENT_SELECT_BUTTON,
                                    "title",         title,
                                    "gradient-name", gradient_name,
                                    NULL);
  else
    button = g_object_new (GIMP_TYPE_GRADIENT_SELECT_BUTTON,
                                    "gradient-name", gradient_name,
                                    NULL);

  return button;
}

/**
 * gimp_gradient_select_button_get_gradient:
 * @button: A #GimpGradientSelectButton
 *
 * Retrieves the name of currently selected gradient.
 *
 * Returns: an internal copy of the gradient name which must not be freed.
 *
 * Since: GIMP 2.4
 */
const gchar *
gimp_gradient_select_button_get_gradient (GimpGradientSelectButton *button)
{
  GimpGradientSelectButtonPrivate *priv;

  g_return_val_if_fail (GIMP_IS_GRADIENT_SELECT_BUTTON (button), NULL);

  priv = GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE (button);
  return priv->gradient_name;
}

/**
 * gimp_gradient_select_button_set_gradient:
 * @button: A #GimpGradientSelectButton
 * @gradient_name: Gradient name to set.
 *
 * Sets the current gradient for the gradient select button.
 *
 * Since: GIMP 2.4
 */
void
gimp_gradient_select_button_set_gradient (GimpGradientSelectButton *button,
                                          const gchar              *gradient_name)
{
  GimpGradientSelectButtonPrivate *priv;
  GimpSelectButton                *select_button;

  g_return_if_fail (GIMP_IS_GRADIENT_SELECT_BUTTON (button));

  priv = GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE (button);
  select_button = GIMP_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      gimp_gradients_set_popup (select_button->temp_callback, gradient_name);
    }
  else
    {
      gchar   *name;
      gdouble *samples;
      gint     n_samples;

      if (gradient_name && *gradient_name)
        name = g_strdup (gradient_name);
      else
        name = gimp_context_get_gradient ();

      if (gimp_gradient_get_uniform_samples (name,
                                             priv->sample_size,
                                             priv->reverse,
                                             &n_samples,
                                             &samples))
        {
          gimp_gradient_select_button_callback (name,
                                                n_samples, samples,
                                                FALSE, button);

          g_free (samples);
        }

      g_free (name);
    }
}


/*  private functions  */

static void
gimp_gradient_select_button_finalize (GObject *object)
{
  GimpGradientSelectButtonPrivate *priv;

  priv = GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE (object);

  g_free (priv->gradient_name);
  priv->gradient_name = NULL;

  g_free (priv->gradient_data);
  priv->gradient_data = NULL;

  g_free (priv->title);
  priv->title = NULL;

  G_OBJECT_CLASS (gimp_gradient_select_button_parent_class)->finalize (object);
}

static void
gimp_gradient_select_button_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  GimpGradientSelectButton        *button;
  GimpGradientSelectButtonPrivate *priv;

  button = GIMP_GRADIENT_SELECT_BUTTON (object);
  priv = GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE (button);

  switch (property_id)
    {
    case PROP_TITLE:
      priv->title = g_value_dup_string (value);
      break;
    case PROP_GRADIENT_NAME:
      gimp_gradient_select_button_set_gradient (button,
                                                g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_gradient_select_button_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  GimpGradientSelectButton        *button;
  GimpGradientSelectButtonPrivate *priv;

  button = GIMP_GRADIENT_SELECT_BUTTON (object);
  priv = GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE (button);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_GRADIENT_NAME:
      g_value_set_string (value, priv->gradient_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_gradient_select_button_callback (const gchar   *gradient_name,
                                      gint           n_samples,
                                      const gdouble *gradient_data,
                                      gboolean       dialog_closing,
                                      gpointer       user_data)
{
  GimpGradientSelectButton        *button;
  GimpGradientSelectButtonPrivate *priv;
  GimpSelectButton                *select_button;

  button = GIMP_GRADIENT_SELECT_BUTTON (user_data);

  priv = GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE (button);
  select_button = GIMP_SELECT_BUTTON (button);

  g_free (priv->gradient_name);
  g_free (priv->gradient_data);

  priv->gradient_name = g_strdup (gradient_name);
  priv->n_samples     = n_samples;
  priv->gradient_data = g_memdup (gradient_data, n_samples * sizeof (gdouble));

  gtk_widget_queue_draw (priv->preview);

  if (dialog_closing)
    select_button->temp_callback = NULL;

  g_signal_emit (button, gradient_button_signals[GRADIENT_SET], 0,
                 gradient_name, n_samples, gradient_data, dialog_closing);
  g_object_notify (G_OBJECT (button), "gradient-name");
}

static void
gimp_gradient_select_button_clicked (GimpGradientSelectButton *button)
{
  GimpGradientSelectButtonPrivate *priv;
  GimpSelectButton                *select_button;

  priv = GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE (button);
  select_button = GIMP_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      /*  calling gimp_gradients_set_popup() raises the dialog  */
      gimp_gradients_set_popup (select_button->temp_callback,
                                priv->gradient_name);
    }
  else
    {
      select_button->temp_callback =
        gimp_gradient_select_new (priv->title, priv->gradient_name,
                                  priv->sample_size,
                                  gimp_gradient_select_button_callback,
                                  button);
    }
}

static void
gimp_gradient_select_preview_size_allocate (GtkWidget                *widget,
                                            GtkAllocation            *allocation,
                                            GimpGradientSelectButton *button)
{
  gdouble                         *samples;
  gint                             n_samples;
  GimpGradientSelectButtonPrivate *priv;

  priv = GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE (button);

  if (gimp_gradient_get_uniform_samples (priv->gradient_name,
                                         allocation->width,
                                         priv->reverse,
                                         &n_samples,
                                         &samples))
    {
      g_free (priv->gradient_data);

      priv->sample_size   = allocation->width;
      priv->n_samples     = n_samples;
      priv->gradient_data = samples;
    }
}

static void
gimp_gradient_select_preview_expose (GtkWidget                *widget,
                                     GdkEventExpose           *event,
                                     GimpGradientSelectButton *button)
{
  const gdouble                   *src;
  guchar                          *p0;
  guchar                          *p1;
  guchar                          *even;
  guchar                          *odd;
  gint                             width;
  gint                             x, y;
  GimpGradientSelectButtonPrivate *priv;

  priv = GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE (button);

  src = priv->gradient_data;
  if (! src)
    return;

  width = priv->n_samples / 4;

  p0 = even = g_malloc (width * 3);
  p1 = odd  = g_malloc (width * 3);

  for (x = 0; x < width; x++)
    {
      gdouble  r, g, b, a;
      gdouble  c0, c1;

      r = src[x * 4 + 0];
      g = src[x * 4 + 1];
      b = src[x * 4 + 2];
      a = src[x * 4 + 3];

      if ((x / GIMP_CHECK_SIZE_SM) & 1)
        {
          c0 = GIMP_CHECK_LIGHT;
          c1 = GIMP_CHECK_DARK;
        }
      else
        {
          c0 = GIMP_CHECK_DARK;
          c1 = GIMP_CHECK_LIGHT;
        }

      *p0++ = (c0 + (r - c0) * a) * 255.0;
      *p0++ = (c0 + (g - c0) * a) * 255.0;
      *p0++ = (c0 + (b - c0) * a) * 255.0;

      *p1++ = (c1 + (r - c1) * a) * 255.0;
      *p1++ = (c1 + (g - c1) * a) * 255.0;
      *p1++ = (c1 + (b - c1) * a) * 255.0;
    }

  for (y = event->area.y; y < event->area.y + event->area.height; y++)
    {
      guchar *buf = ((y / GIMP_CHECK_SIZE_SM) & 1) ? odd : even;

      gdk_draw_rgb_image_dithalign (widget->window,
                                    widget->style->fg_gc[widget->state],
                                    event->area.x, y,
                                    event->area.width, 1,
                                    GDK_RGB_DITHER_MAX,
                                    buf + event->area.x * 3,
                                    width * 3,
                                    - event->area.x, - y);
    }

  g_free (odd);
  g_free (even);
}

static void
gimp_gradient_select_drag_data_received (GimpGradientSelectButton *button,
                                        GdkDragContext            *context,
                                        gint                       x,
                                        gint                       y,
                                        GtkSelectionData          *selection,
                                        guint                      info,
                                        guint                      time)
{
  gchar *str;

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid gradient data!");
      return;
    }

  str = g_strndup ((const gchar *) selection->data, selection->length);

  if (g_utf8_validate (str, -1, NULL))
    {
      gint     pid;
      gpointer unused;
      gint     name_offset = 0;

      if (sscanf (str, "%i:%p:%n", &pid, &unused, &name_offset) >= 2 &&
          pid == gimp_getpid () && name_offset > 0)
        {
          gchar *name = str + name_offset;

          gimp_gradient_select_button_set_gradient (button, name);
        }
    }

  g_free (str);
}

static GtkWidget *
gimp_gradient_select_button_create_inside (GimpGradientSelectButton *gradient_button)
{
  GtkWidget                       *button;
  GimpGradientSelectButtonPrivate *priv;

  priv = GIMP_GRADIENT_SELECT_BUTTON_GET_PRIVATE (gradient_button);

  gtk_widget_push_composite_child ();

  button = gtk_button_new ();

  priv->preview = gtk_drawing_area_new ();
  gtk_widget_set_size_request (priv->preview, CELL_WIDTH, CELL_HEIGHT);
  gtk_container_add (GTK_CONTAINER (button), priv->preview);

  g_signal_connect (priv->preview, "size-allocate",
                    G_CALLBACK (gimp_gradient_select_preview_size_allocate),
                    gradient_button);

  g_signal_connect (priv->preview, "expose-event",
                    G_CALLBACK (gimp_gradient_select_preview_expose),
                    gradient_button);

  gtk_widget_show_all (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_gradient_select_button_clicked),
                            gradient_button);

  gtk_drag_dest_set (GTK_WIDGET (button),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect_swapped (button, "drag-data-received",
                            G_CALLBACK (gimp_gradient_select_drag_data_received),
                            gradient_button);

  gtk_widget_pop_composite_child ();

  return button;
}
