/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbrushselectbutton.c
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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimpbrushselectbutton.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimpbrushselectbutton
 * @title: gimpbrushselectbutton
 * @short_description: A button that pops up a brush selection dialog.
 *
 * A button that pops up a brush selection dialog.
 **/


#define CELL_SIZE 20


#define GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMP_TYPE_BRUSH_SELECT_BUTTON, GimpBrushSelectButtonPrivate))

typedef struct _GimpBrushSelectButtonPrivate GimpBrushSelectButtonPrivate;

struct _GimpBrushSelectButtonPrivate
{
  gchar         *title;

  gchar         *brush_name;      /* Local copy */
  gdouble        opacity;
  gint           spacing;
  GimpLayerMode  paint_mode;
  gint           width;
  gint           height;
  guchar        *mask_data;       /* local copy */

  GtkWidget     *inside;
  GtkWidget     *preview;
  GtkWidget     *popup;
};

enum
{
  BRUSH_SET,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_BRUSH_NAME,
  PROP_BRUSH_OPACITY,
  PROP_BRUSH_SPACING,
  PROP_BRUSH_PAINT_MODE
};


/*  local function prototypes  */

static void   gimp_brush_select_button_finalize     (GObject      *object);

static void   gimp_brush_select_button_set_property (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void   gimp_brush_select_button_get_property (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static void   gimp_brush_select_button_clicked  (GimpBrushSelectButton *button);

static void   gimp_brush_select_button_callback (const gchar          *brush_name,
                                                 gdouble               opacity,
                                                 gint                  spacing,
                                                 GimpLayerMode         paint_mode,
                                                 gint                  width,
                                                 gint                  height,
                                                 const guchar         *mask_data,
                                                 gboolean              dialog_closing,
                                                 gpointer              user_data);

static void     gimp_brush_select_preview_resize  (GimpBrushSelectButton *button);
static gboolean gimp_brush_select_preview_events  (GtkWidget             *widget,
                                                   GdkEvent              *event,
                                                   GimpBrushSelectButton *button);
static void     gimp_brush_select_preview_draw    (GimpPreviewArea       *area,
                                                   gint                   x,
                                                   gint                   y,
                                                   gint                   width,
                                                   gint                   height,
                                                   const guchar          *mask_data,
                                                   gint                   rowstride);
static void     gimp_brush_select_preview_update  (GtkWidget             *preview,
                                                   gint                   brush_width,
                                                   gint                   brush_height,
                                                   const guchar          *mask_data);

static void     gimp_brush_select_button_open_popup  (GimpBrushSelectButton *button,
                                                      gint                   x,
                                                      gint                   y);
static void     gimp_brush_select_button_close_popup (GimpBrushSelectButton *button);

static void   gimp_brush_select_drag_data_received (GimpBrushSelectButton *button,
                                                    GdkDragContext        *context,
                                                    gint                   x,
                                                    gint                   y,
                                                    GtkSelectionData      *selection,
                                                    guint                  info,
                                                    guint                  time);

static GtkWidget * gimp_brush_select_button_create_inside (GimpBrushSelectButton *button);


static const GtkTargetEntry target = { "application/x-gimp-brush-name", 0 };

static guint brush_button_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (GimpBrushSelectButton, gimp_brush_select_button,
               GIMP_TYPE_SELECT_BUTTON)


static void
gimp_brush_select_button_class_init (GimpBrushSelectButtonClass *klass)
{
  GObjectClass          *object_class        = G_OBJECT_CLASS (klass);
  GimpSelectButtonClass *select_button_class = GIMP_SELECT_BUTTON_CLASS (klass);

  object_class->finalize     = gimp_brush_select_button_finalize;
  object_class->set_property = gimp_brush_select_button_set_property;
  object_class->get_property = gimp_brush_select_button_get_property;

  select_button_class->select_destroy = gimp_brush_select_destroy;

  klass->brush_set = NULL;

  /**
   * GimpBrushSelectButton:title:
   *
   * The title to be used for the brush selection popup dialog.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "Title",
                                                        "The title to be used for the brush selection popup dialog",
                                                        _("Brush Selection"),
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /**
   * GimpBrushSelectButton:brush-name:
   *
   * The name of the currently selected brush.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_BRUSH_NAME,
                                   g_param_spec_string ("brush-name",
                                                        "Brush name",
                                                        "The name of the currently selected brush",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  /**
   * GimpBrushSelectButton:opacity:
   *
   * The opacity of the currently selected brush.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_BRUSH_OPACITY,
                                   g_param_spec_double ("brush-opacity",
                                                        "Brush opacity",
                                                        "The opacity of the currently selected brush",
                                                        -1.0, 100.0, -1.0,
                                                        GIMP_PARAM_READWRITE));

  /**
   * GimpBrushSelectButton:spacing:
   *
   * The spacing of the currently selected brush.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_BRUSH_SPACING,
                                   g_param_spec_int ("brush-spacing",
                                                     "Brush spacing",
                                                     "The spacing of the currently selected brush",
                                                     -G_MAXINT, 1000, -1,
                                                     GIMP_PARAM_READWRITE));

  /**
   * GimpBrushSelectButton:paint-mode:
   *
   * The paint mode.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_BRUSH_PAINT_MODE,
                                   g_param_spec_int ("brush-paint-mode",
                                                     "Brush paint mode",
                                                     "The paint mode of the currently selected brush",
                                                     -1, GIMP_LAYER_MODE_LUMINANCE,
                                                     -1,
                                                     GIMP_PARAM_READWRITE));

  /**
   * GimpBrushSelectButton::brush-set:
   * @widget: the object which received the signal.
   * @brush_name: the name of the currently selected brush.
   * @opacity: opacity of the brush
   * @spacing: spacing of the brush
   * @paint_mode: paint mode of the brush
   * @width: width of the brush
   * @height: height of the brush
   * @mask_data: brush mask data
   * @dialog_closing: whether the dialog was closed or not.
   *
   * The ::brush-set signal is emitted when the user selects a brush.
   *
   * Since: 2.4
   */
  brush_button_signals[BRUSH_SET] =
    g_signal_new ("brush-set",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpBrushSelectButtonClass, brush_set),
                  NULL, NULL,
                  _gimpui_marshal_VOID__STRING_DOUBLE_INT_INT_INT_INT_POINTER_BOOLEAN,
                  G_TYPE_NONE, 8,
                  G_TYPE_STRING,
                  G_TYPE_DOUBLE,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_POINTER,
                  G_TYPE_BOOLEAN);

  g_type_class_add_private (object_class,
                            sizeof (GimpBrushSelectButtonPrivate));
}

static void
gimp_brush_select_button_init (GimpBrushSelectButton *button)
{
  GimpBrushSelectButtonPrivate *priv;
  gint                          mask_bpp;
  gint                          mask_data_size;
  gint                          color_bpp;
  gint                          color_data_size;
  guint8                       *color_data;

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (button);

  priv->brush_name = gimp_context_get_brush ();
  gimp_brush_get_pixels (priv->brush_name,
                         &priv->width,
                         &priv->height,
                         &mask_bpp,
                         &mask_data_size,
                         &priv->mask_data,
                         &color_bpp,
                         &color_data_size,
                         &color_data);

  if (color_data)
    g_free (color_data);

  priv->inside = gimp_brush_select_button_create_inside (button);
  gtk_container_add (GTK_CONTAINER (button), priv->inside);

  priv->popup = NULL;
}

/**
 * gimp_brush_select_button_new:
 * @title:      Title of the dialog to use or %NULL means to use the default
 *              title.
 * @brush_name: Initial brush name or %NULL to use current selection.
 * @opacity:    Initial opacity. -1 means to use current opacity.
 * @spacing:    Initial spacing. -1 means to use current spacing.
 * @paint_mode: Initial paint mode.  -1 means to use current paint mode.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a #GimpBrush.  This widget is suitable for placement in a table in
 * a plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_brush_select_button_new (const gchar   *title,
                              const gchar   *brush_name,
                              gdouble        opacity,
                              gint           spacing,
                              GimpLayerMode  paint_mode)
{
  GtkWidget *button;

  if (title)
    button = g_object_new (GIMP_TYPE_BRUSH_SELECT_BUTTON,
                           "title",            title,
                           "brush-name",       brush_name,
                           "brush-opacity",    opacity,
                           "brush-spacing",    spacing,
                           "brush-paint-mode", paint_mode,
                           NULL);
  else
    button = g_object_new (GIMP_TYPE_BRUSH_SELECT_BUTTON,
                           "brush-name",       brush_name,
                           "brush-opacity",    opacity,
                           "brush-spacing",    spacing,
                           "brush-paint-mode", paint_mode,
                           NULL);

  return button;
}

/**
 * gimp_brush_select_button_get_brush:
 * @button: A #GimpBrushSelectButton
 * @opacity: Opacity of the selected brush.
 * @spacing: Spacing of the selected brush.
 * @paint_mode: Paint mode of the selected brush.
 *
 * Retrieves the properties of currently selected brush.
 *
 * Returns: an internal copy of the brush name which must not be freed.
 *
 * Since: 2.4
 */
const gchar *
gimp_brush_select_button_get_brush (GimpBrushSelectButton *button,
                                    gdouble               *opacity,
                                    gint                  *spacing,
                                    GimpLayerMode         *paint_mode)
{
  GimpBrushSelectButtonPrivate *priv;

  g_return_val_if_fail (GIMP_IS_BRUSH_SELECT_BUTTON (button), NULL);

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (button);

  if (opacity)
    *opacity = priv->opacity;

  if (spacing)
    *spacing = priv->spacing;

  if (paint_mode)
    *paint_mode = priv->paint_mode;

  return priv->brush_name;
}

/**
 * gimp_brush_select_button_set_brush:
 * @button: A #GimpBrushSelectButton
 * @brush_name: Brush name to set; %NULL means no change.
 * @opacity:    Opacity to set. -1.0 means no change.
 * @spacing:    Spacing to set. -1 means no change.
 * @paint_mode: Paint mode to set.  -1 means no change.
 *
 * Sets the current brush and other values for the brush select
 * button.
 *
 * Since: 2.4
 */
void
gimp_brush_select_button_set_brush (GimpBrushSelectButton *button,
                                    const gchar           *brush_name,
                                    gdouble                opacity,
                                    gint                   spacing,
                                    GimpLayerMode          paint_mode)
{
  GimpSelectButton *select_button;

  g_return_if_fail (GIMP_IS_BRUSH_SELECT_BUTTON (button));

  select_button = GIMP_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      gimp_brushes_set_popup (select_button->temp_callback, brush_name,
                              opacity, spacing, paint_mode);
    }
  else
    {
      gchar  *name;
      gint    width;
      gint    height;
      gint    bytes;
      gint    mask_data_size;
      guint8 *mask_data;
      gint    color_bpp;
      gint    color_data_size;
      guint8 *color_data;

      if (brush_name && *brush_name)
        name = g_strdup (brush_name);
      else
        name = gimp_context_get_brush ();

      if (gimp_brush_get_pixels (name,
                                 &width,
                                 &height,
                                 &bytes,
                                 &mask_data_size,
                                 &mask_data,
                                 &color_bpp,
                                 &color_data_size,
                                 &color_data))
        {
          if (color_data)
            g_free (color_data);

          if (opacity < 0.0)
            opacity = gimp_context_get_opacity ();

          if (spacing == -1)
            gimp_brush_get_spacing (name, &spacing);

          if (paint_mode == -1)
            paint_mode = gimp_context_get_paint_mode ();

          gimp_brush_select_button_callback (name,
                                             opacity, spacing, paint_mode,
                                             width, height, mask_data,
                                             FALSE, button);

          g_free (mask_data);
        }

      g_free (name);
    }
}


/*  private functions  */

static void
gimp_brush_select_button_finalize (GObject *object)
{
  GimpBrushSelectButtonPrivate *priv;

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (object);

  g_free (priv->brush_name);
  priv->brush_name = NULL;

  g_free (priv->mask_data);
  priv->mask_data = NULL;

  g_free (priv->title);
  priv->title = NULL;

  G_OBJECT_CLASS (gimp_brush_select_button_parent_class)->finalize (object);
}

static void
gimp_brush_select_button_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpBrushSelectButton        *button = GIMP_BRUSH_SELECT_BUTTON (object);
  GimpBrushSelectButtonPrivate *priv;
  gdouble opacity;
  gint32 spacing;
  gint32 paint_mode;

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (button);

  switch (property_id)
    {
    case PROP_TITLE:
      priv->title = g_value_dup_string (value);
      break;
    case PROP_BRUSH_NAME:
      gimp_brush_select_button_set_brush (button,
                                          g_value_get_string (value),
                                          -1.0, -1, -1);
      break;
    case PROP_BRUSH_OPACITY:
      opacity = g_value_get_double (value);
      if (opacity >= 0.0)
        priv->opacity = opacity;
      break;
    case PROP_BRUSH_SPACING:
      spacing = g_value_get_int (value);
      if (spacing != -1)
        priv->spacing = spacing;
      break;
    case PROP_BRUSH_PAINT_MODE:
      paint_mode = g_value_get_int (value);
      if (paint_mode != -1)
        priv->paint_mode = paint_mode;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_brush_select_button_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpBrushSelectButton        *button = GIMP_BRUSH_SELECT_BUTTON (object);
  GimpBrushSelectButtonPrivate *priv;

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (button);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_BRUSH_NAME:
      g_value_set_string (value, priv->brush_name);
      break;
    case PROP_BRUSH_OPACITY:
      g_value_set_double (value, priv->opacity);
      break;
    case PROP_BRUSH_SPACING:
      g_value_set_int (value, priv->spacing);
      break;
    case PROP_BRUSH_PAINT_MODE:
      g_value_set_int (value, priv->paint_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_brush_select_button_callback (const gchar   *name,
                                   gdouble        opacity,
                                   gint           spacing,
                                   GimpLayerMode  paint_mode,
                                   gint           width,
                                   gint           height,
                                   const guchar  *mask_data,
                                   gboolean       dialog_closing,
                                   gpointer       data)
{
  GimpBrushSelectButton        *button;
  GimpBrushSelectButtonPrivate *priv;
  GimpSelectButton             *select_button;

  button = GIMP_BRUSH_SELECT_BUTTON (data);

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (button);
  select_button = GIMP_SELECT_BUTTON (button);

  g_free (priv->brush_name);
  g_free (priv->mask_data);

  priv->brush_name = g_strdup (name);
  priv->width      = width;
  priv->height     = height;
  priv->mask_data  = g_memdup (mask_data, width * height);
  priv->opacity    = opacity;
  priv->spacing    = spacing;
  priv->paint_mode = paint_mode;

  gimp_brush_select_preview_update (priv->preview,
                                    width, height, mask_data);

  if (dialog_closing)
    select_button->temp_callback = NULL;

  g_signal_emit (button, brush_button_signals[BRUSH_SET], 0,
                 name, opacity, spacing, paint_mode, width, height, mask_data,
                 dialog_closing);
  g_object_notify (G_OBJECT (button), "brush-name");
}

static void
gimp_brush_select_button_clicked (GimpBrushSelectButton *button)
{
  GimpBrushSelectButtonPrivate *priv;
  GimpSelectButton             *select_button;

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (button);
  select_button = GIMP_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      /*  calling gimp_brushes_set_popup() raises the dialog  */
      gimp_brushes_set_popup (select_button->temp_callback,
                              priv->brush_name,
                              priv->opacity,
                              priv->spacing,
                              priv->paint_mode);
    }
  else
    {
      select_button->temp_callback =
        gimp_brush_select_new (priv->title, priv->brush_name,
                               priv->opacity, priv->spacing, priv->paint_mode,
                               gimp_brush_select_button_callback,
                               button);
    }
}

static void
gimp_brush_select_preview_resize (GimpBrushSelectButton *button)
{
  GimpBrushSelectButtonPrivate *priv;

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (button);

  if (priv->width > 0 && priv->height > 0)
    gimp_brush_select_preview_update (priv->preview,
                                      priv->width,
                                      priv->height,
                                      priv->mask_data);
}

static gboolean
gimp_brush_select_preview_events (GtkWidget             *widget,
                                  GdkEvent              *event,
                                  GimpBrushSelectButton *button)
{
  GimpBrushSelectButtonPrivate *priv;
  GdkEventButton               *bevent;

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (button);

  if (priv->mask_data)
    {
      switch (event->type)
        {
        case GDK_BUTTON_PRESS:
          bevent = (GdkEventButton *) event;

          if (bevent->button == 1)
            {
              gtk_grab_add (widget);
              gimp_brush_select_button_open_popup (button,
                                                   bevent->x, bevent->y);
            }
          break;

        case GDK_BUTTON_RELEASE:
          bevent = (GdkEventButton *) event;

          if (bevent->button == 1)
            {
              gtk_grab_remove (widget);
              gimp_brush_select_button_close_popup (button);
            }
          break;

        default:
          break;
        }
    }

  return FALSE;
}

static void
gimp_brush_select_preview_draw (GimpPreviewArea *area,
                                gint             x,
                                gint             y,
                                gint             width,
                                gint             height,
                                const guchar    *mask_data,
                                gint             rowstride)
{
  const guchar *src;
  guchar       *dest;
  guchar       *buf;
  gint          i, j;

  buf = g_new (guchar, width * height);

  src  = mask_data;
  dest = buf;

  for (j = 0; j < height; j++)
    {
      const guchar *s = src;

      for (i = 0; i < width; i++, s++, dest++)
        *dest = 255 - *s;

      src += rowstride;
    }

  gimp_preview_area_draw (area,
                          x, y, width, height,
                          GIMP_GRAY_IMAGE,
                          buf,
                          width);

  g_free (buf);
}

static void
gimp_brush_select_preview_update (GtkWidget    *preview,
                                  gint          brush_width,
                                  gint          brush_height,
                                  const guchar *mask_data)
{
  GimpPreviewArea *area = GIMP_PREVIEW_AREA (preview);
  GtkAllocation    allocation;
  gint             x, y;
  gint             width, height;

  gtk_widget_get_allocation (preview, &allocation);

  width  = MIN (brush_width,  allocation.width);
  height = MIN (brush_height, allocation.height);

  x = ((allocation.width  - width)  / 2);
  y = ((allocation.height - height) / 2);

  if (x || y)
    gimp_preview_area_fill (area,
                            0, 0,
                            allocation.width,
                            allocation.height,
                            0xFF, 0xFF, 0xFF);

  gimp_brush_select_preview_draw (area,
                                  x, y, width, height,
                                  mask_data, brush_width);
}

static void
gimp_brush_select_button_open_popup (GimpBrushSelectButton *button,
                                     gint                   x,
                                     gint                   y)
{
  GimpBrushSelectButtonPrivate *priv;
  GtkWidget                    *frame;
  GtkWidget                    *preview;
  GdkMonitor                   *monitor;
  GdkRectangle                  workarea;
  gint                          x_org;
  gint                          y_org;

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (button);

  if (priv->popup)
    gimp_brush_select_button_close_popup (button);

  if (priv->width <= CELL_SIZE && priv->height <= CELL_SIZE)
    return;

  priv->popup = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (priv->popup), GDK_WINDOW_TYPE_HINT_DND);
  gtk_window_set_screen (GTK_WINDOW (priv->popup),
                         gtk_widget_get_screen (GTK_WIDGET (button)));

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (priv->popup), frame);
  gtk_widget_show (frame);

  preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview, priv->width, priv->height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  /* decide where to put the popup */
  gdk_window_get_origin (gtk_widget_get_window (priv->preview),
                         &x_org, &y_org);

  monitor = gimp_widget_get_monitor (GTK_WIDGET (button));
  gdk_monitor_get_workarea (monitor, &workarea);

  x = x_org + x - (priv->width  / 2);
  y = y_org + y - (priv->height / 2);

  x = CLAMP (x, workarea.x, workarea.x + workarea.width  - priv->width);
  y = CLAMP (y, workarea.y, workarea.y + workarea.height - priv->height);

  gtk_window_move (GTK_WINDOW (priv->popup), x, y);

  gtk_widget_show (priv->popup);

  /*  Draw the brush  */
  gimp_brush_select_preview_draw (GIMP_PREVIEW_AREA (preview),
                                  0, 0, priv->width, priv->height,
                                  priv->mask_data, priv->width);
}

static void
gimp_brush_select_button_close_popup (GimpBrushSelectButton *button)
{
  GimpBrushSelectButtonPrivate *priv;

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (button);

  if (priv->popup)
    {
      gtk_widget_destroy (priv->popup);
      priv->popup = NULL;
    }
}

static void
gimp_brush_select_drag_data_received (GimpBrushSelectButton *button,
                                      GdkDragContext        *context,
                                      gint                   x,
                                      gint                   y,
                                      GtkSelectionData      *selection,
                                      guint                  info,
                                      guint                  time)
{
  gint   length = gtk_selection_data_get_length (selection);
  gchar *str;

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("%s: received invalid brush data", G_STRFUNC);
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
          pid == gimp_getpid () && name_offset > 0)
        {
          gchar *name = str + name_offset;

          gimp_brush_select_button_set_brush (button, name, -1.0, -1, -1);
        }
    }

  g_free (str);
}

static GtkWidget *
gimp_brush_select_button_create_inside (GimpBrushSelectButton *brush_button)
{
  GtkWidget                    *hbox;
  GtkWidget                    *frame;
  GtkWidget                    *button;
  GimpBrushSelectButtonPrivate *priv;

  priv = GIMP_BRUSH_SELECT_BUTTON_GET_PRIVATE (brush_button);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  priv->preview = gimp_preview_area_new ();
  gtk_widget_add_events (priv->preview,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_set_size_request (priv->preview, CELL_SIZE, CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), priv->preview);

  g_signal_connect_swapped (priv->preview, "size-allocate",
                            G_CALLBACK (gimp_brush_select_preview_resize),
                            brush_button);
  g_signal_connect (priv->preview, "event",
                    G_CALLBACK (gimp_brush_select_preview_events),
                    brush_button);

  gtk_drag_dest_set (GTK_WIDGET (priv->preview),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect_swapped (priv->preview, "drag-data-received",
                            G_CALLBACK (gimp_brush_select_drag_data_received),
                            brush_button);

  button = gtk_button_new_with_mnemonic (_("_Browse..."));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_brush_select_button_clicked),
                            brush_button);

  gtk_widget_show_all (hbox);

  return hbox;
}
