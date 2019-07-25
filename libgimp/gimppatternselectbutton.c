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
 * @short_description: A button which pops up a pattern select dialog.
 *
 * A button which pops up a pattern select dialog.
 **/


#define CELL_SIZE 20


#define GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE(obj) ((GimpPatternSelectButtonPrivate *) gimp_pattern_select_button_get_instance_private ((GimpPatternSelectButton *) (obj)))

typedef struct _GimpPatternSelectButtonPrivate GimpPatternSelectButtonPrivate;

struct _GimpPatternSelectButtonPrivate
{
  gchar     *title;

  gchar     *pattern_name;      /* Local copy */
  gint       width;
  gint       height;
  gint       bytes;
  guchar    *mask_data;         /* local copy */

  GtkWidget *inside;
  GtkWidget *preview;
  GtkWidget *popup;
};

enum
{
  PATTERN_SET,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_PATTERN_NAME
};


/*  local function prototypes  */

static void   gimp_pattern_select_button_finalize     (GObject      *object);

static void   gimp_pattern_select_button_set_property (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void   gimp_pattern_select_button_get_property (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);

static void   gimp_pattern_select_button_clicked  (GimpPatternSelectButton *button);

static void   gimp_pattern_select_button_callback (const gchar  *pattern_name,
                                                   gint          width,
                                                   gint          height,
                                                   gint          bytes,
                                                   const guchar *mask_data,
                                                   gboolean      dialog_closing,
                                                   gpointer      user_data);

static void     gimp_pattern_select_preview_resize  (GimpPatternSelectButton *button);
static gboolean gimp_pattern_select_preview_events  (GtkWidget               *widget,
                                                     GdkEvent                *event,
                                                     GimpPatternSelectButton *button);
static void     gimp_pattern_select_preview_update  (GtkWidget               *preview,
                                                     gint                     width,
                                                     gint                     height,
                                                     gint                     bytes,
                                                     const guchar            *mask_data);

static void     gimp_pattern_select_button_open_popup  (GimpPatternSelectButton *button,
                                                        gint                     x,
                                                        gint                     y);
static void     gimp_pattern_select_button_close_popup (GimpPatternSelectButton *button);

static void   gimp_pattern_select_drag_data_received (GimpPatternSelectButton *button,
                                                      GdkDragContext          *context,
                                                      gint                     x,
                                                      gint                     y,
                                                      GtkSelectionData        *selection,
                                                      guint                    info,
                                                      guint                    time);

static GtkWidget * gimp_pattern_select_button_create_inside (GimpPatternSelectButton *button);


static const GtkTargetEntry target = { "application/x-gimp-pattern-name", 0 };

static guint pattern_button_signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (GimpPatternSelectButton, gimp_pattern_select_button,
                            GIMP_TYPE_SELECT_BUTTON)

static void
gimp_pattern_select_button_class_init (GimpPatternSelectButtonClass *klass)
{
  GObjectClass          *object_class        = G_OBJECT_CLASS (klass);
  GimpSelectButtonClass *select_button_class = GIMP_SELECT_BUTTON_CLASS (klass);

  object_class->finalize     = gimp_pattern_select_button_finalize;
  object_class->set_property = gimp_pattern_select_button_set_property;
  object_class->get_property = gimp_pattern_select_button_get_property;

  select_button_class->select_destroy = gimp_pattern_select_destroy;

  klass->pattern_set = NULL;

  /**
   * GimpPatternSelectButton:title:
   *
   * The title to be used for the pattern selection popup dialog.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "Title",
                                                        "The title to be used for the pattern selection popup dialog",
                                                        _("Pattern Selection"),
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /**
   * GimpPatternSelectButton:pattern-name:
   *
   * The name of the currently selected pattern.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_PATTERN_NAME,
                                   g_param_spec_string ("pattern-name",
                                                        "Pattern name",
                                                        "The name of the currently selected pattern",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  /**
   * GimpPatternSelectButton::pattern-set:
   * @widget: the object which received the signal.
   * @pattern_name: the name of the currently selected pattern.
   * @width: width of the pattern
   * @height: height of the pattern
   * @bpp: bpp of the pattern
   * @mask_data: pattern mask data
   * @dialog_closing: whether the dialog was closed or not.
   *
   * The ::pattern-set signal is emitted when the user selects a pattern.
   *
   * Since: 2.4
   */
  pattern_button_signals[PATTERN_SET] =
    g_signal_new ("pattern-set",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPatternSelectButtonClass, pattern_set),
                  NULL, NULL,
                  _gimpui_marshal_VOID__STRING_INT_INT_INT_POINTER_BOOLEAN,
                  G_TYPE_NONE, 6,
                  G_TYPE_STRING,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_POINTER,
                  G_TYPE_BOOLEAN);
}

static void
gimp_pattern_select_button_init (GimpPatternSelectButton *button)
{
  GimpPatternSelectButtonPrivate *priv;
  gint                            mask_data_size;

  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (button);

  priv->pattern_name = gimp_context_get_pattern ();
  gimp_pattern_get_pixels (priv->pattern_name,
                           &priv->width,
                           &priv->height,
                           &priv->bytes,
                           &mask_data_size,
                           &priv->mask_data);

  priv->inside = gimp_pattern_select_button_create_inside (button);
  gtk_container_add (GTK_CONTAINER (button), priv->inside);

  priv->popup = NULL;
}

/**
 * gimp_pattern_select_button_new:
 * @title:        Title of the dialog to use or %NULL to use the default title.
 * @pattern_name: Initial pattern name or %NULL to use current selection.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a pattern.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_pattern_select_button_new (const gchar *title,
                                const gchar *pattern_name)
{
  GtkWidget *button;

  if (title)
    button = g_object_new (GIMP_TYPE_PATTERN_SELECT_BUTTON,
                           "title",        title,
                           "pattern-name", pattern_name,
                           NULL);
  else
    button = g_object_new (GIMP_TYPE_PATTERN_SELECT_BUTTON,
                           "pattern-name", pattern_name,
                           NULL);

  return button;
}

/**
 * gimp_pattern_select_button_get_pattern:
 * @button: A #GimpPatternSelectButton
 *
 * Retrieves the name of currently selected pattern.
 *
 * Returns: an internal copy of the pattern name which must not be freed.
 *
 * Since: 2.4
 */
const gchar *
gimp_pattern_select_button_get_pattern (GimpPatternSelectButton *button)
{
  GimpPatternSelectButtonPrivate *priv;

  g_return_val_if_fail (GIMP_IS_PATTERN_SELECT_BUTTON (button), NULL);

  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (button);
  return priv->pattern_name;
}

/**
 * gimp_pattern_select_button_set_pattern:
 * @button: A #GimpPatternSelectButton
 * @pattern_name: Pattern name to set; %NULL means no change.
 *
 * Sets the current pattern for the pattern select button.
 *
 * Since: 2.4
 */
void
gimp_pattern_select_button_set_pattern (GimpPatternSelectButton *button,
                                        const gchar             *pattern_name)
{
  GimpSelectButton *select_button;

  g_return_if_fail (GIMP_IS_PATTERN_SELECT_BUTTON (button));

  select_button = GIMP_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      gimp_patterns_set_popup (select_button->temp_callback, pattern_name);
    }
  else
    {
      gchar  *name;
      gint    width;
      gint    height;
      gint    bytes;
      gint    mask_data_size;
      guint8 *mask_data;

      if (pattern_name && *pattern_name)
        name = g_strdup (pattern_name);
      else
        name = gimp_context_get_pattern ();

      if (gimp_pattern_get_pixels (name,
                                   &width,
                                   &height,
                                   &bytes,
                                   &mask_data_size,
                                   &mask_data))
        {
          gimp_pattern_select_button_callback (name,
                                               width, height, bytes, mask_data,
                                               FALSE, button);

          g_free (mask_data);
        }

      g_free (name);
    }
}


/*  private functions  */

static void
gimp_pattern_select_button_finalize (GObject *object)
{
  GimpPatternSelectButtonPrivate *priv;

  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (object);

  g_clear_pointer (&priv->pattern_name, g_free);
  g_clear_pointer (&priv->mask_data,    g_free);
  g_clear_pointer (&priv->title,        g_free);

  G_OBJECT_CLASS (gimp_pattern_select_button_parent_class)->finalize (object);
}

static void
gimp_pattern_select_button_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  GimpPatternSelectButton        *button;
  GimpPatternSelectButtonPrivate *priv;

  button = GIMP_PATTERN_SELECT_BUTTON (object);
  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (button);

  switch (property_id)
    {
    case PROP_TITLE:
      priv->title = g_value_dup_string (value);
      break;
    case PROP_PATTERN_NAME:
      gimp_pattern_select_button_set_pattern (button,
                                              g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_pattern_select_button_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  GimpPatternSelectButton        *button;
  GimpPatternSelectButtonPrivate *priv;

  button = GIMP_PATTERN_SELECT_BUTTON (object);
  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (button);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_PATTERN_NAME:
      g_value_set_string (value, priv->pattern_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_pattern_select_button_callback (const gchar  *pattern_name,
                                     gint          width,
                                     gint          height,
                                     gint          bytes,
                                     const guchar *mask_data,
                                     gboolean      dialog_closing,
                                     gpointer      user_data)
{
  GimpPatternSelectButton        *button;
  GimpPatternSelectButtonPrivate *priv;
  GimpSelectButton               *select_button;

  button = GIMP_PATTERN_SELECT_BUTTON (user_data);

  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (button);
  select_button = GIMP_SELECT_BUTTON (button);

  g_free (priv->pattern_name);
  g_free (priv->mask_data);

  priv->pattern_name = g_strdup (pattern_name);
  priv->width        = width;
  priv->height       = height;
  priv->bytes        = bytes;
  priv->mask_data    = g_memdup (mask_data, width * height * bytes);

  gimp_pattern_select_preview_update (priv->preview,
                                      width, height, bytes, mask_data);

  if (dialog_closing)
    select_button->temp_callback = NULL;

  g_signal_emit (button, pattern_button_signals[PATTERN_SET], 0,
                 pattern_name, width, height, bytes, dialog_closing);
  g_object_notify (G_OBJECT (button), "pattern-name");
}

static void
gimp_pattern_select_button_clicked (GimpPatternSelectButton *button)
{
  GimpPatternSelectButtonPrivate *priv;
  GimpSelectButton               *select_button;

  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (button);
  select_button = GIMP_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      /*  calling gimp_patterns_set_popup() raises the dialog  */
      gimp_patterns_set_popup (select_button->temp_callback,
                               priv->pattern_name);
    }
  else
    {
      select_button->temp_callback =
        gimp_pattern_select_new (priv->title, priv->pattern_name,
                                 gimp_pattern_select_button_callback,
                                 button);
    }
}

static void
gimp_pattern_select_preview_resize (GimpPatternSelectButton *button)
{
  GimpPatternSelectButtonPrivate *priv;

  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (button);

  if (priv->width > 0 && priv->height > 0)
    gimp_pattern_select_preview_update (priv->preview,
                                        priv->width,
                                        priv->height,
                                        priv->bytes,
                                        priv->mask_data);
}

static gboolean
gimp_pattern_select_preview_events (GtkWidget               *widget,
                                    GdkEvent                *event,
                                    GimpPatternSelectButton *button)
{
  GimpPatternSelectButtonPrivate *priv;
  GdkEventButton                 *bevent;

  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (button);

  if (priv->mask_data)
    {
      switch (event->type)
        {
        case GDK_BUTTON_PRESS:
          bevent = (GdkEventButton *) event;

          if (bevent->button == 1)
            {
              gtk_grab_add (widget);
              gimp_pattern_select_button_open_popup (button,
                                                     bevent->x, bevent->y);
            }
          break;

        case GDK_BUTTON_RELEASE:
          bevent = (GdkEventButton *) event;

          if (bevent->button == 1)
            {
              gtk_grab_remove (widget);
              gimp_pattern_select_button_close_popup (button);
            }
          break;

        default:
          break;
        }
    }

  return FALSE;
}

static void
gimp_pattern_select_preview_update (GtkWidget    *preview,
                                    gint          width,
                                    gint          height,
                                    gint          bytes,
                                    const guchar *mask_data)
{
  GimpImageType type;

  switch (bytes)
    {
    case 1:  type = GIMP_GRAY_IMAGE;   break;
    case 2:  type = GIMP_GRAYA_IMAGE;  break;
    case 3:  type = GIMP_RGB_IMAGE;    break;
    case 4:  type = GIMP_RGBA_IMAGE;   break;
    default:
      return;
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                          0, 0, width, height,
                          type,
                          mask_data,
                          width * bytes);
}

static void
gimp_pattern_select_button_open_popup (GimpPatternSelectButton *button,
                                       gint                     x,
                                       gint                     y)
{
  GimpPatternSelectButtonPrivate *priv;
  GtkWidget                      *frame;
  GtkWidget                      *preview;
  GdkScreen                      *screen;
  gint                            x_org;
  gint                            y_org;
  gint                            scr_w;
  gint                            scr_h;

  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (button);

  if (priv->popup)
    gimp_pattern_select_button_close_popup (button);

  if (priv->width <= CELL_SIZE && priv->height <= CELL_SIZE)
    return;

  screen = gtk_widget_get_screen (GTK_WIDGET (button));

  priv->popup = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (priv->popup), GDK_WINDOW_TYPE_HINT_DND);
  gtk_window_set_screen (GTK_WINDOW (priv->popup), screen);

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

  scr_w = gdk_screen_get_width (screen);
  scr_h = gdk_screen_get_height (screen);

  x = x_org + x - (priv->width  / 2);
  y = y_org + y - (priv->height / 2);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + priv->width  > scr_w) ? scr_w - priv->width  : x;
  y = (y + priv->height > scr_h) ? scr_h - priv->height : y;

  gtk_window_move (GTK_WINDOW (priv->popup), x, y);

  gtk_widget_show (priv->popup);

  /*  Draw the pattern  */
  gimp_pattern_select_preview_update (preview,
                                      priv->width,
                                      priv->height,
                                      priv->bytes,
                                      priv->mask_data);
}

static void
gimp_pattern_select_button_close_popup (GimpPatternSelectButton *button)
{
  GimpPatternSelectButtonPrivate *priv;

  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (button);

  if (priv->popup)
    {
      gtk_widget_destroy (priv->popup);
      priv->popup = NULL;
    }
}

static void
gimp_pattern_select_drag_data_received (GimpPatternSelectButton *button,
                                        GdkDragContext          *context,
                                        gint                     x,
                                        gint                     y,
                                        GtkSelectionData        *selection,
                                        guint                    info,
                                        guint                    time)
{
  gint   length = gtk_selection_data_get_length (selection);
  gchar *str;

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("Received invalid pattern data!");
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

          gimp_pattern_select_button_set_pattern (button, name);
        }
    }

  g_free (str);
}

static GtkWidget *
gimp_pattern_select_button_create_inside (GimpPatternSelectButton *pattern_button)
{
  GtkWidget                      *hbox;
  GtkWidget                      *frame;
  GtkWidget                      *button;
  GimpPatternSelectButtonPrivate *priv;

  priv = GIMP_PATTERN_SELECT_BUTTON_GET_PRIVATE (pattern_button);

  gtk_widget_push_composite_child ();

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
                            G_CALLBACK (gimp_pattern_select_preview_resize),
                            pattern_button);
  g_signal_connect (priv->preview, "event",
                    G_CALLBACK (gimp_pattern_select_preview_events),
                    pattern_button);

  gtk_drag_dest_set (GTK_WIDGET (priv->preview),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect_swapped (priv->preview, "drag-data-received",
                            G_CALLBACK (gimp_pattern_select_drag_data_received),
                            pattern_button);

  button = gtk_button_new_with_mnemonic (_("_Browse..."));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_pattern_select_button_clicked),
                            pattern_button);

  gtk_widget_show_all (hbox);

  gtk_widget_pop_composite_child ();

  return hbox;
}
