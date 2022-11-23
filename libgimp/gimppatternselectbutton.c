/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapatternselectbutton.c
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
#include "ligmapatternselectbutton.h"
#include "ligmauimarshal.h"

#include "libligma-intl.h"


/**
 * SECTION: ligmapatternselectbutton
 * @title: LigmaPatternSelectButton
 * @short_description: A button which pops up a pattern select dialog.
 *
 * A button which pops up a pattern select dialog.
 **/


#define CELL_SIZE 20


#define GET_PRIVATE(obj) (((LigmaPatternSelectButtonPrivate *) (obj))->priv)

struct _LigmaPatternSelectButtonPrivate
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
  PROP_PATTERN_NAME,
  N_PROPS
};


/*  local function prototypes  */

static void   ligma_pattern_select_button_finalize     (GObject      *object);

static void   ligma_pattern_select_button_set_property (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void   ligma_pattern_select_button_get_property (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);

static void   ligma_pattern_select_button_clicked  (LigmaPatternSelectButton *button);

static void   ligma_pattern_select_button_callback (const gchar  *pattern_name,
                                                   gint          width,
                                                   gint          height,
                                                   gint          bytes,
                                                   const guchar *mask_data,
                                                   gboolean      dialog_closing,
                                                   gpointer      user_data);

static void     ligma_pattern_select_preview_resize  (LigmaPatternSelectButton *button);
static gboolean ligma_pattern_select_preview_events  (GtkWidget               *widget,
                                                     GdkEvent                *event,
                                                     LigmaPatternSelectButton *button);
static void     ligma_pattern_select_preview_update  (GtkWidget               *preview,
                                                     gint                     width,
                                                     gint                     height,
                                                     gint                     bytes,
                                                     const guchar            *mask_data);

static void     ligma_pattern_select_button_open_popup  (LigmaPatternSelectButton *button,
                                                        gint                     x,
                                                        gint                     y);
static void     ligma_pattern_select_button_close_popup (LigmaPatternSelectButton *button);

static void   ligma_pattern_select_drag_data_received (LigmaPatternSelectButton *button,
                                                      GdkDragContext          *context,
                                                      gint                     x,
                                                      gint                     y,
                                                      GtkSelectionData        *selection,
                                                      guint                    info,
                                                      guint                    time);

static GtkWidget * ligma_pattern_select_button_create_inside (LigmaPatternSelectButton *button);


static const GtkTargetEntry target = { "application/x-ligma-pattern-name", 0 };

static guint pattern_button_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *pattern_button_props[N_PROPS] = { NULL, };


G_DEFINE_TYPE_WITH_PRIVATE (LigmaPatternSelectButton, ligma_pattern_select_button,
                            LIGMA_TYPE_SELECT_BUTTON)


static void
ligma_pattern_select_button_class_init (LigmaPatternSelectButtonClass *klass)
{
  GObjectClass          *object_class        = G_OBJECT_CLASS (klass);
  LigmaSelectButtonClass *select_button_class = LIGMA_SELECT_BUTTON_CLASS (klass);

  object_class->finalize     = ligma_pattern_select_button_finalize;
  object_class->set_property = ligma_pattern_select_button_set_property;
  object_class->get_property = ligma_pattern_select_button_get_property;

  select_button_class->select_destroy = ligma_pattern_select_destroy;

  klass->pattern_set = NULL;

  /**
   * LigmaPatternSelectButton:title:
   *
   * The title to be used for the pattern selection popup dialog.
   *
   * Since: 2.4
   */
  pattern_button_props[PROP_TITLE] = g_param_spec_string ("title",
                                                          "Title",
                                                          "The title to be used for the pattern selection popup dialog",
                                                          _("Pattern Selection"),
                                                          LIGMA_PARAM_READWRITE |
                                                          G_PARAM_CONSTRUCT_ONLY);

  /**
   * LigmaPatternSelectButton:pattern-name:
   *
   * The name of the currently selected pattern.
   *
   * Since: 2.4
   */
  pattern_button_props[PROP_PATTERN_NAME] = g_param_spec_string ("pattern-name",
                                                                 "Pattern name",
                                                                 "The name of the currently selected pattern",
                                                                 NULL,
                                                                 LIGMA_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, pattern_button_props);

  /**
   * LigmaPatternSelectButton::pattern-set:
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
                  G_STRUCT_OFFSET (LigmaPatternSelectButtonClass, pattern_set),
                  NULL, NULL,
                  _ligmaui_marshal_VOID__STRING_INT_INT_INT_POINTER_BOOLEAN,
                  G_TYPE_NONE, 6,
                  G_TYPE_STRING,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_POINTER,
                  G_TYPE_BOOLEAN);
}

static void
ligma_pattern_select_button_init (LigmaPatternSelectButton *button)
{
  gint mask_data_size;

  button->priv = ligma_pattern_select_button_get_instance_private (button);

  button->priv->pattern_name = ligma_context_get_pattern ();
  ligma_pattern_get_pixels (button->priv->pattern_name,
                           &button->priv->width,
                           &button->priv->height,
                           &button->priv->bytes,
                           &mask_data_size,
                           &button->priv->mask_data);

  button->priv->inside = ligma_pattern_select_button_create_inside (button);
  gtk_container_add (GTK_CONTAINER (button), button->priv->inside);
}

/**
 * ligma_pattern_select_button_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @pattern_name: (nullable): Initial pattern name or %NULL to use current selection.
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
ligma_pattern_select_button_new (const gchar *title,
                                const gchar *pattern_name)
{
  GtkWidget *button;

  if (title)
    button = g_object_new (LIGMA_TYPE_PATTERN_SELECT_BUTTON,
                           "title",        title,
                           "pattern-name", pattern_name,
                           NULL);
  else
    button = g_object_new (LIGMA_TYPE_PATTERN_SELECT_BUTTON,
                           "pattern-name", pattern_name,
                           NULL);

  return button;
}

/**
 * ligma_pattern_select_button_get_pattern:
 * @button: A #LigmaPatternSelectButton
 *
 * Retrieves the name of currently selected pattern.
 *
 * Returns: an internal copy of the pattern name which must not be freed.
 *
 * Since: 2.4
 */
const gchar *
ligma_pattern_select_button_get_pattern (LigmaPatternSelectButton *button)
{
  g_return_val_if_fail (LIGMA_IS_PATTERN_SELECT_BUTTON (button), NULL);

  return button->priv->pattern_name;
}

/**
 * ligma_pattern_select_button_set_pattern:
 * @button: A #LigmaPatternSelectButton
 * @pattern_name: (nullable): Pattern name to set; %NULL means no change.
 *
 * Sets the current pattern for the pattern select button.
 *
 * Since: 2.4
 */
void
ligma_pattern_select_button_set_pattern (LigmaPatternSelectButton *button,
                                        const gchar             *pattern_name)
{
  LigmaSelectButton *select_button;

  g_return_if_fail (LIGMA_IS_PATTERN_SELECT_BUTTON (button));

  select_button = LIGMA_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      ligma_patterns_set_popup (select_button->temp_callback, pattern_name);
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
        name = ligma_context_get_pattern ();

      if (ligma_pattern_get_pixels (name,
                                   &width,
                                   &height,
                                   &bytes,
                                   &mask_data_size,
                                   &mask_data))
        {
          ligma_pattern_select_button_callback (name,
                                               width, height, bytes, mask_data,
                                               FALSE, button);

          g_free (mask_data);
        }

      g_free (name);
    }
}


/*  private functions  */

static void
ligma_pattern_select_button_finalize (GObject *object)
{
  LigmaPatternSelectButton *button = LIGMA_PATTERN_SELECT_BUTTON (object);

  g_clear_pointer (&button->priv->pattern_name, g_free);
  g_clear_pointer (&button->priv->mask_data,    g_free);
  g_clear_pointer (&button->priv->title,        g_free);

  G_OBJECT_CLASS (ligma_pattern_select_button_parent_class)->finalize (object);
}

static void
ligma_pattern_select_button_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  LigmaPatternSelectButton *button = LIGMA_PATTERN_SELECT_BUTTON (object);

  switch (property_id)
    {
    case PROP_TITLE:
      button->priv->title = g_value_dup_string (value);
      break;

    case PROP_PATTERN_NAME:
      ligma_pattern_select_button_set_pattern (button,
                                              g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_pattern_select_button_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  LigmaPatternSelectButton *button = LIGMA_PATTERN_SELECT_BUTTON (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, button->priv->title);
      break;

    case PROP_PATTERN_NAME:
      g_value_set_string (value, button->priv->pattern_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_pattern_select_button_callback (const gchar  *pattern_name,
                                     gint          width,
                                     gint          height,
                                     gint          bytes,
                                     const guchar *mask_data,
                                     gboolean      dialog_closing,
                                     gpointer      user_data)
{
  LigmaPatternSelectButton *button        = user_data;
  LigmaSelectButton        *select_button = LIGMA_SELECT_BUTTON (button);

  g_free (button->priv->pattern_name);
  g_free (button->priv->mask_data);

  button->priv->pattern_name = g_strdup (pattern_name);
  button->priv->width        = width;
  button->priv->height       = height;
  button->priv->bytes        = bytes;
  button->priv->mask_data    = g_memdup2 (mask_data, width * height * bytes);

  ligma_pattern_select_preview_update (button->priv->preview,
                                      width, height, bytes, mask_data);

  if (dialog_closing)
    select_button->temp_callback = NULL;

  g_signal_emit (button, pattern_button_signals[PATTERN_SET], 0,
                 pattern_name, width, height, bytes, dialog_closing);
  g_object_notify_by_pspec (G_OBJECT (button), pattern_button_props[PROP_PATTERN_NAME]);
}

static void
ligma_pattern_select_button_clicked (LigmaPatternSelectButton *button)
{
  LigmaSelectButton *select_button = LIGMA_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      /*  calling ligma_patterns_set_popup() raises the dialog  */
      ligma_patterns_set_popup (select_button->temp_callback,
                               button->priv->pattern_name);
    }
  else
    {
      select_button->temp_callback =
        ligma_pattern_select_new (button->priv->title,
                                 button->priv->pattern_name,
                                 ligma_pattern_select_button_callback,
                                 button, NULL);
    }
}

static void
ligma_pattern_select_preview_resize (LigmaPatternSelectButton *button)
{
  if (button->priv->width > 0 && button->priv->height > 0)
    ligma_pattern_select_preview_update (button->priv->preview,
                                        button->priv->width,
                                        button->priv->height,
                                        button->priv->bytes,
                                        button->priv->mask_data);
}

static gboolean
ligma_pattern_select_preview_events (GtkWidget               *widget,
                                    GdkEvent                *event,
                                    LigmaPatternSelectButton *button)
{
  GdkEventButton *bevent;

  if (button->priv->mask_data)
    {
      switch (event->type)
        {
        case GDK_BUTTON_PRESS:
          bevent = (GdkEventButton *) event;

          if (bevent->button == 1)
            {
              gtk_grab_add (widget);
              ligma_pattern_select_button_open_popup (button,
                                                     bevent->x, bevent->y);
            }
          break;

        case GDK_BUTTON_RELEASE:
          bevent = (GdkEventButton *) event;

          if (bevent->button == 1)
            {
              gtk_grab_remove (widget);
              ligma_pattern_select_button_close_popup (button);
            }
          break;

        default:
          break;
        }
    }

  return FALSE;
}

static void
ligma_pattern_select_preview_update (GtkWidget    *preview,
                                    gint          width,
                                    gint          height,
                                    gint          bytes,
                                    const guchar *mask_data)
{
  LigmaImageType type;

  switch (bytes)
    {
    case 1:  type = LIGMA_GRAY_IMAGE;   break;
    case 2:  type = LIGMA_GRAYA_IMAGE;  break;
    case 3:  type = LIGMA_RGB_IMAGE;    break;
    case 4:  type = LIGMA_RGBA_IMAGE;   break;
    default:
      return;
    }

  ligma_preview_area_draw (LIGMA_PREVIEW_AREA (preview),
                          0, 0, width, height,
                          type,
                          mask_data,
                          width * bytes);
}

static void
ligma_pattern_select_button_open_popup (LigmaPatternSelectButton *button,
                                       gint                     x,
                                       gint                     y)
{
  LigmaPatternSelectButtonPrivate *priv = button->priv;
  GtkWidget                      *frame;
  GtkWidget                      *preview;
  GdkMonitor                    *monitor;
  GdkRectangle                   workarea;
  gint                            x_org;
  gint                            y_org;

  if (priv->popup)
    ligma_pattern_select_button_close_popup (button);

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

  preview = ligma_preview_area_new ();
  gtk_widget_set_size_request (preview, priv->width, priv->height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  /* decide where to put the popup */
  gdk_window_get_origin (gtk_widget_get_window (priv->preview),
                         &x_org, &y_org);

  monitor = ligma_widget_get_monitor (GTK_WIDGET (button));
  gdk_monitor_get_workarea (monitor, &workarea);

  x = x_org + x - (priv->width  / 2);
  y = y_org + y - (priv->height / 2);

  x = CLAMP (x, workarea.x, workarea.x + workarea.width  - priv->width);
  y = CLAMP (y, workarea.y, workarea.y + workarea.height - priv->height);

  gtk_window_move (GTK_WINDOW (priv->popup), x, y);

  gtk_widget_show (priv->popup);

  /*  Draw the pattern  */
  ligma_pattern_select_preview_update (preview,
                                      priv->width,
                                      priv->height,
                                      priv->bytes,
                                      priv->mask_data);
}

static void
ligma_pattern_select_button_close_popup (LigmaPatternSelectButton *button)
{
  g_clear_pointer (&button->priv->popup, gtk_widget_destroy);
}

static void
ligma_pattern_select_drag_data_received (LigmaPatternSelectButton *button,
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
          pid == ligma_getpid () && name_offset > 0)
        {
          gchar *name = str + name_offset;

          ligma_pattern_select_button_set_pattern (button, name);
        }
    }

  g_free (str);
}

static GtkWidget *
ligma_pattern_select_button_create_inside (LigmaPatternSelectButton *pattern_button)
{
  LigmaPatternSelectButtonPrivate *priv = pattern_button->priv;
  GtkWidget                      *hbox;
  GtkWidget                      *frame;
  GtkWidget                      *button;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  priv->preview = ligma_preview_area_new ();
  gtk_widget_add_events (priv->preview,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_set_size_request (priv->preview, CELL_SIZE, CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), priv->preview);

  g_signal_connect_swapped (priv->preview, "size-allocate",
                            G_CALLBACK (ligma_pattern_select_preview_resize),
                            pattern_button);
  g_signal_connect (priv->preview, "event",
                    G_CALLBACK (ligma_pattern_select_preview_events),
                    pattern_button);

  gtk_drag_dest_set (GTK_WIDGET (priv->preview),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect_swapped (priv->preview, "drag-data-received",
                            G_CALLBACK (ligma_pattern_select_drag_data_received),
                            pattern_button);

  button = gtk_button_new_with_mnemonic (_("_Browse..."));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (ligma_pattern_select_button_clicked),
                            pattern_button);

  gtk_widget_show_all (hbox);

  return hbox;
}
