/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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
#include "gimpresourceselect-private.h"

#include "gimpuitypes.h"
#include "gimpresourcechooser.h"
#include "gimpuimarshal.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimpresourcechooser
 * @title: GimpResourceChooser
 * @short_description: Base class for buttons that popup a resource
 *                     selection dialog.
 *
 * A button which pops up a resource selection dialog.
 *
 * Responsibilities:
 *
 *     - implementing outer container widget,
 *     - managing clicks and popping up a remote chooser,
 *     - having a resource property,
 *     - signaling when user selects resource
 *     - receiving drag,
 *     - triggering draws of the button interior (by subclass) and draws of remote popup chooser.
 *
 * Collaborations:
 *
 *     - owned by GimpProcedureDialog via GimpPropWidget
 *     - resource property usually bound to a GimpConfig for a GimpPluginProcedure.
 *     - communicates using GimpResourceSelect with remote GimpPDBDialog,
 *       to choose an installed GimpResource owned by core.
 *
 * Subclass responsibilities:
 *
 *     - creating interior widgets
 *     - drawing the interior (a preview of the chosen resource)
 *     - declaring which interior widgets are drag destinations
 *     - declaring which interior widgets are clickable (generate "clicked" signal)
 *     - generate "clicked" (delegating to GtkButton or implementing from mouse events)
 *
 * Since: 3.0
 **/


enum
{
  RESOURCE_SET,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_LABEL,
  PROP_RESOURCE,
  N_PROPS
};


typedef struct
{
  GimpResource *resource;
  gchar        *title;
  gchar        *label;
  gchar        *callback;

  GtkWidget    *label_widget;
} GimpResourceChooserPrivate;


/*  local function prototypes  */

static void   gimp_resource_chooser_constructed        (GObject             *object);
static void   gimp_resource_chooser_dispose            (GObject             *object);
static void   gimp_resource_chooser_finalize           (GObject             *object);

static void   gimp_resource_chooser_set_property       (GObject             *object,
                                                        guint                property_id,
                                                        const GValue        *value,
                                                        GParamSpec          *pspec);
static void   gimp_resource_chooser_get_property       (GObject             *object,
                                                        guint                property_id,
                                                        GValue              *value,
                                                        GParamSpec          *pspec);

static void   gimp_resource_chooser_clicked            (GimpResourceChooser *self);

static void   gimp_resource_chooser_callback           (GimpResource        *resource,
                                                        gboolean             dialog_closing,
                                                        gpointer             user_data);

static void   gimp_resource_select_drag_data_received  (GimpResourceChooser *self,
                                                        GdkDragContext      *context,
                                                        gint                 x,
                                                        gint                 y,
                                                        GtkSelectionData    *selection,
                                                        guint                info,
                                                        guint                time);

static void   gimp_resource_chooser_set_remote_dialog  (GimpResourceChooser *self,
                                                        GimpResource        *resource);


static guint resource_button_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *resource_button_props[N_PROPS] = { NULL, };

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GimpResourceChooser, gimp_resource_chooser, GTK_TYPE_BOX)


static void
gimp_resource_chooser_class_init (GimpResourceChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_resource_chooser_constructed;
  object_class->dispose      = gimp_resource_chooser_dispose;
  object_class->finalize     = gimp_resource_chooser_finalize;
  object_class->set_property = gimp_resource_chooser_set_property;
  object_class->get_property = gimp_resource_chooser_get_property;

  klass->resource_set        = NULL;
  klass->draw_interior       = NULL;

  /**
   * GimpResourceChooser:title:
   *
   * The title to be used for the resource selection popup dialog.
   *
   * Since: 3.0
   */
  resource_button_props[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title to be used for the resource selection popup dialog",
                         "Resource Selection",
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * GimpResourceChooser:label:
   *
   * Label text with mnemonic.
   *
   * Since: 3.0
   */
  resource_button_props[PROP_LABEL] =
    g_param_spec_string ("label",
                         "Label",
                         "The label to be used next to the button",
                         NULL,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  /**
   * GimpResourceChooser:resource:
   *
   * The currently selected resource.
   *
   * Since: 3.0
   */
  resource_button_props[PROP_RESOURCE] =
    gimp_param_spec_resource ("resource",
                              "Resource",
                              "The currently selected resource",
                              GIMP_TYPE_RESOURCE,
                              TRUE,  /* none_ok */
                              NULL,  /* no default for this property. */
                              FALSE,
                              GIMP_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     N_PROPS, resource_button_props);

  /**
   * GimpResourceChooser::resource-set:
   * @widget: the object which received the signal.
   * @resource: the currently selected resource.
   * @dialog_closing: whether the dialog was closed or not.
   *
   * The ::resource-set signal is emitted when the user selects a resource.
   *
   * Since: 3.0
   */
  resource_button_signals[RESOURCE_SET] =
    g_signal_new ("resource-set",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpResourceChooserClass, resource_set),
                  NULL, NULL,
                  _gimpui_marshal_VOID__POINTER_BOOLEAN,
                  G_TYPE_NONE, 2,
                  G_TYPE_OBJECT,
                  G_TYPE_BOOLEAN);
}

static void
gimp_resource_chooser_init (GimpResourceChooser *self)
{
  GimpResourceChooserPrivate *priv;

  priv = gimp_resource_chooser_get_instance_private (GIMP_RESOURCE_CHOOSER (self));

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (self), 6);

  priv->label_widget = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (priv->label_widget), 0.0);
  gtk_box_pack_start (GTK_BOX (self), priv->label_widget, FALSE, FALSE, 0);
}

static void
gimp_resource_chooser_constructed (GObject *object)
{
  GimpResourceChooserPrivate *priv;

  priv = gimp_resource_chooser_get_instance_private (GIMP_RESOURCE_CHOOSER (object));
  /* Set text of label widget now since underlying property might have changed.
   * Not when empty string:  set_text throws CRITICAL on empty string!
   */
  if (priv->label != NULL)
    {
      gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->label_widget), priv->label);
      gtk_widget_show (GTK_WIDGET (priv->label_widget));
    }

  G_OBJECT_CLASS (gimp_resource_chooser_parent_class)->constructed (object);
}

static void
gimp_resource_chooser_dispose (GObject *self)
{
  GimpResourceChooserPrivate *priv;
  GimpResourceChooserClass   *klass;

  priv  = gimp_resource_chooser_get_instance_private (GIMP_RESOURCE_CHOOSER (self));
  klass = GIMP_RESOURCE_CHOOSER_GET_CLASS (self);

  if (priv->callback)
    {
      GType resource_type = klass->resource_type;

      if (resource_type == GIMP_TYPE_FONT)
        gimp_fonts_close_popup (priv->callback);
      else if (resource_type == GIMP_TYPE_GRADIENT)
        gimp_gradients_close_popup (priv->callback);
      else if (resource_type == GIMP_TYPE_BRUSH)
        gimp_brushes_close_popup (priv->callback);
      else if (resource_type == GIMP_TYPE_PALETTE)
        gimp_palettes_close_popup (priv->callback);
      else if (resource_type == GIMP_TYPE_PATTERN)
        gimp_patterns_close_popup (priv->callback);
      else
        g_warning ("%s: unhandled resource type", G_STRFUNC);

      gimp_plug_in_remove_temp_procedure (gimp_get_plug_in (), priv->callback);
      g_clear_pointer (&priv->callback, g_free);
    }

  G_OBJECT_CLASS (gimp_resource_chooser_parent_class)->dispose (self);
}

static void
gimp_resource_chooser_finalize (GObject *object)
{
  GimpResourceChooser *self = GIMP_RESOURCE_CHOOSER (object);
  GimpResourceChooserPrivate *priv = gimp_resource_chooser_get_instance_private (self);

  g_clear_pointer (&priv->title, g_free);
  g_clear_pointer (&priv->label, g_free);

  G_OBJECT_CLASS (gimp_resource_chooser_parent_class)->finalize (object);
}

/**
 * _gimp_resource_chooser_set_drag_target:
 * @chooser:            A [class@ResourceChooser]
 * @drag_region_widget: An interior widget to be a droppable region
 *                      and emit "drag-data-received" signal
 * @drag_target:        The drag target to accept
 *
 * Called by a subclass init to specialize the instance.
 *
 * Subclass knows its interior widget whose region is a drop zone.
 * Subclass knows what things can be dropped (target.)
 * Self (super) handles the drop.
 *
 * Since: 3.0
 **/
void
_gimp_resource_chooser_set_drag_target (GimpResourceChooser  *chooser,
                                        GtkWidget            *drag_region_widget,
                                        const GtkTargetEntry *drag_target)
{
  g_return_if_fail (GIMP_IS_RESOURCE_CHOOSER (chooser));
  g_return_if_fail (drag_target != NULL);
  g_return_if_fail (drag_region_widget != NULL);

  gtk_drag_dest_set (drag_region_widget,
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     drag_target, 1,  /* Pass array of size 1 */
                     GDK_ACTION_COPY);

  /* connect drag_region_widget's drag_received signal to chooser's callback. */
  g_signal_connect_swapped (drag_region_widget, "drag-data-received",
                            G_CALLBACK (gimp_resource_select_drag_data_received),
                            chooser);
}

/**
 * _gimp_resource_chooser_set_clickable:
 * @chooser: A [class@ResourceChooser]
 * @widget:  An interior widget that emits "clicked" signal
 *
 * Called by a subclass init to specialize the instance.
 *
 * Subclass knows its interior widget whose region when clicked
 * should popup remote chooser.
 * Self handles the click event.
 *
 * Since: 3.0
 **/
void
_gimp_resource_chooser_set_clickable (GimpResourceChooser *chooser,
                                      GtkWidget           *widget)
{
  g_return_if_fail (GIMP_IS_RESOURCE_CHOOSER (chooser));
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));

  /* Require the widget have a signal "clicked", usually a button. */
  g_signal_connect_swapped (widget, "clicked",
                            G_CALLBACK (gimp_resource_chooser_clicked),
                            chooser);
}

/**
 * gimp_resource_chooser_get_resource:
 * @chooser: A #GimpResourceChooser
 *
 * Gets the currently selected resource.
 *
 * Returns: (transfer none): an internal copy of the resource which must not be freed.
 *
 * Since: 3.0
 */
GimpResource *
gimp_resource_chooser_get_resource (GimpResourceChooser *chooser)
{
  GimpResourceChooserPrivate *priv;

  g_return_val_if_fail (GIMP_IS_RESOURCE_CHOOSER (chooser), NULL);

  priv = gimp_resource_chooser_get_instance_private (chooser);

  return priv->resource;
}

/**
 * gimp_resource_chooser_set_resource:
 * @chooser: A #GimpResourceChooser
 * @resource: Resource to set.
 *
 * Sets the currently selected resource.
 * This will select the resource in both the button and any chooser popup.
 *
 * Since: 3.0
 */
void
gimp_resource_chooser_set_resource (GimpResourceChooser *chooser,
                                    GimpResource        *resource)
{
  GimpResourceChooserPrivate *priv;

  g_return_if_fail (GIMP_IS_RESOURCE_CHOOSER (chooser));
  g_return_if_fail (resource != NULL);

  priv = gimp_resource_chooser_get_instance_private (chooser);

  if (priv->callback)
    {
      /* A popup chooser dialog is already shown.
       * Call its setter to change the selection there
       * (since all views of the resource must be consistent.)
       * That will call back, which will change our own view of the resource.
       */
      gimp_resource_chooser_set_remote_dialog (chooser, resource);
    }
  else
    {
      /* Call our own setter. */
      gimp_resource_chooser_callback (resource, FALSE, chooser);
    }
}

/**
 * gimp_resource_chooser_get_label:
 * @widget: A [class@ResourceChooser].
 *
 * Returns the label widget.
 *
 * Returns: (transfer none): the [class@Gtk.Widget] showing the label text.
 * Since: 3.0
 */
GtkWidget *
gimp_resource_chooser_get_label (GimpResourceChooser *widget)
{
  GimpResourceChooserPrivate *priv;

  g_return_val_if_fail (GIMP_IS_RESOURCE_CHOOSER (widget), NULL);

  priv = gimp_resource_chooser_get_instance_private (widget);
  return priv->label_widget;
}


/*  private functions  */

static void
gimp_resource_chooser_set_remote_dialog (GimpResourceChooser *self,
                                         GimpResource        *resource)
{
  GimpResourceChooserPrivate *priv;
  GimpResourceChooserClass   *klass;

  g_return_if_fail (GIMP_IS_RESOURCE_CHOOSER (self));
  g_return_if_fail (resource != NULL);

  priv  = gimp_resource_chooser_get_instance_private (self);
  klass = GIMP_RESOURCE_CHOOSER_GET_CLASS (self);

  g_return_if_fail (klass->resource_type != G_TYPE_INVALID);
  g_return_if_fail (klass->resource_type == G_TYPE_FROM_INSTANCE (resource));

  gimp_resource_select_set (priv->callback, resource);
}

static void
gimp_resource_chooser_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *gvalue,
                                    GParamSpec   *pspec)
{
  GimpResourceChooser        *self = GIMP_RESOURCE_CHOOSER (object);
  GimpResourceChooserPrivate *priv = gimp_resource_chooser_get_instance_private (self);

  switch (property_id)
    {
    case PROP_TITLE:
      priv->title = g_value_dup_string (gvalue);
      break;

    case PROP_LABEL:
      priv->label = g_value_dup_string (gvalue);
      break;

    case PROP_RESOURCE:
      gimp_resource_chooser_set_resource (self, g_value_get_object (gvalue));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_resource_chooser_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpResourceChooser        *self = GIMP_RESOURCE_CHOOSER (object);
  GimpResourceChooserPrivate *priv = gimp_resource_chooser_get_instance_private (self);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;

    case PROP_LABEL:
      g_value_set_string (value, priv->label);
      break;

    case PROP_RESOURCE:
      g_value_set_object (value, priv->resource);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* A callback from the remote resource select popup.
 * When user chooses a resource.
 * Via a temporary PDB procedure.
 *
 * Set self's model (priv->resource)
 * Notify any parent widget subscribed on the "resource" property
 * typically a prop widget.
 * Update the view, since model changed.
 */
static void
gimp_resource_chooser_callback (GimpResource *resource,
                                gboolean      dialog_closing,
                                gpointer      user_data)
{
  GimpResourceChooser        *self = GIMP_RESOURCE_CHOOSER (user_data);
  GimpResourceChooserPrivate *priv = gimp_resource_chooser_get_instance_private (self);

  priv->resource = resource;

  GIMP_RESOURCE_CHOOSER_GET_CLASS (self)->draw_interior (self);

  if (dialog_closing)
    g_clear_pointer (&priv->callback, g_free);

  g_signal_emit (self, resource_button_signals[RESOURCE_SET], 0, resource, dialog_closing);
  g_object_notify_by_pspec (G_OBJECT (self), resource_button_props[PROP_RESOURCE]);
}

static void
gimp_resource_chooser_clicked (GimpResourceChooser *self)
{
  GimpResourceChooserPrivate *priv  = gimp_resource_chooser_get_instance_private (self);
  GimpResourceChooserClass   *klass = GIMP_RESOURCE_CHOOSER_GET_CLASS (self);

  if (priv->callback)
    {
      /* Popup already created.  Calling setter raises the popup. */
      gimp_resource_chooser_set_remote_dialog (self, priv->resource);
    }
  else
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
      GBytes    *handle   = NULL;

      if (GIMP_IS_DIALOG (toplevel))
        handle = gimp_dialog_get_native_handle (GIMP_DIALOG (toplevel));

      priv->callback = g_strdup (gimp_resource_select_new (priv->title,
                                                           handle,
                                                           priv->resource,
                                                           klass->resource_type,
                                                           gimp_resource_chooser_callback,
                                                           self,
                                                           NULL));
      gimp_resource_chooser_set_remote_dialog (self, priv->resource);
    }
}


/* Drag methods. */

static void
gimp_resource_select_drag_data_received (GimpResourceChooser *self,
                                         GdkDragContext      *context,
                                         gint                 x,
                                         gint                 y,
                                         GtkSelectionData    *selection,
                                         guint                info,
                                         guint                time)
{
  gint   length = gtk_selection_data_get_length (selection);
  gchar *str;

  GimpResourceChooserClass *klass;

  klass = GIMP_RESOURCE_CHOOSER_GET_CLASS (self);
  /* Require class resource_type was initialized. */
  g_assert (klass->resource_type != 0);

  /* Drag data is a string that is the ID of the resource. */

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("%s: received invalid resource data", G_STRFUNC);
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
          gchar        *name = str + name_offset;
          GimpResource *resource;

          resource = gimp_resource_get_by_name (klass->resource_type, name);
          gimp_resource_chooser_set_resource (self, resource);
        }
    }

  g_free (str);
}
