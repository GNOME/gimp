/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Editable label base class
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "animation-editable-label.h"

enum
{
  PROP_0,
  PROP_TEXT,
  PROP_EDITING_WIDGET
};

enum
{
  PREPARE_EDITING,
  EDITING,
  LAST_SIGNAL
};

struct _AnimationEditableLabelPrivate
{
  gchar     *text;

  GtkWidget *viewing_widget;
  GtkWidget *label;

  gboolean   is_editing;

  GtkWidget *image;
  gboolean   show_icon;
};

static void animation_editable_label_constructed   (GObject              *object);
static void animation_editable_label_finalize      (GObject              *object);
static void animation_editable_label_set_property  (GObject              *object,
                                                    guint                 property_id,
                                                    const GValue         *value,
                                                    GParamSpec           *pspec);
static void animation_editable_label_get_property  (GObject              *object,
                                                    guint                 property_id,
                                                    GValue               *value,
                                                    GParamSpec           *pspec);

static gboolean animation_editable_label_clicked   (GtkWidget            *widget,
                                                    GdkEvent             *event,
                                                    gpointer              user_data);

G_DEFINE_TYPE (AnimationEditableLabel, animation_editable_label, GTK_TYPE_FRAME)

#define parent_class animation_editable_label_parent_class

static guint animation_editable_label_signals[LAST_SIGNAL] = { 0 };

static void
animation_editable_label_class_init (AnimationEditableLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  animation_editable_label_signals[PREPARE_EDITING] =
    g_signal_new ("prepare-editing",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
  animation_editable_label_signals[EDITING] =
    g_signal_new ("editing",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  object_class->constructed  = animation_editable_label_constructed;
  object_class->finalize     = animation_editable_label_finalize;
  object_class->set_property = animation_editable_label_set_property;
  object_class->get_property = animation_editable_label_get_property;

  g_object_class_install_property (object_class, PROP_TEXT,
                                   g_param_spec_string ("text",
                                                        NULL, NULL,
                                                        "",
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_EDITING_WIDGET,
                                   g_param_spec_object ("editing-widget",
                                                        NULL, NULL,
                                                        GTK_TYPE_WIDGET,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_type_class_add_private (klass, sizeof (AnimationEditableLabelPrivate));
}

static void
animation_editable_label_init (AnimationEditableLabel *label)
{
  label->priv = G_TYPE_INSTANCE_GET_PRIVATE (label,
                                             ANIMATION_TYPE_EDITABLE_LABEL,
                                             AnimationEditableLabelPrivate);
}

static void
animation_editable_label_constructed (GObject *object)
{
  AnimationEditableLabel *label  = ANIMATION_EDITABLE_LABEL (object);
  GtkWidget              *box;
  GtkWidget              *event_box;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* The viewing widget. */
  event_box = gtk_event_box_new ();
  gtk_widget_add_events (GTK_WIDGET (event_box),
                         GDK_BUTTON_PRESS_MASK);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

  label->priv->label = gtk_label_new (label->priv->text? label->priv->text : "");
  gtk_label_set_justify (GTK_LABEL (label->priv->label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (box),
                      label->priv->label,
                      TRUE, TRUE,
                      2.0);
  gtk_widget_show (label->priv->label);

  label->priv->image = gtk_image_new_from_icon_name ("gtk-edit", GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (box),
                      label->priv->image,
                      FALSE,
                      FALSE,
                      2.0);
  gtk_widget_show (box);

  gtk_container_add (GTK_CONTAINER (event_box), box);
  gtk_container_add (GTK_CONTAINER (label),
                     event_box);
  gtk_widget_show (event_box);
  label->priv->viewing_widget = event_box;
  label->priv->is_editing = FALSE;
  label->priv->show_icon  = FALSE;

  g_signal_connect (event_box, "button-press-event",
                    G_CALLBACK (animation_editable_label_clicked),
                    label);
}

static void
animation_editable_label_finalize (GObject *object)
{
  AnimationEditableLabel *label = ANIMATION_EDITABLE_LABEL (object);

  if (label->priv->text)
    g_free (label->priv->text);
  if (label->priv->is_editing)
    gtk_widget_destroy (label->priv->viewing_widget);
  else
    gtk_widget_destroy (label->editing_widget);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
animation_editable_label_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  AnimationEditableLabel *label = ANIMATION_EDITABLE_LABEL (object);

  switch (property_id)
    {
    case PROP_TEXT:
      if (label->priv->text)
        g_free (label->priv->text);
      label->priv->text = g_value_dup_string (value);
      if (label->priv->label)
        gtk_label_set_text (GTK_LABEL (label->priv->label),
                            label->priv->text);
      break;
    case PROP_EDITING_WIDGET:
      label->editing_widget = g_value_get_object (value);
      gtk_widget_show (label->editing_widget);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
animation_editable_label_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  AnimationEditableLabel *label = ANIMATION_EDITABLE_LABEL (object);

  switch (property_id)
    {
    case PROP_TEXT:
      g_value_set_string (value, label->priv->text);
      break;
    case PROP_EDITING_WIDGET:
      g_value_set_object (value, label->editing_widget);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
animation_editable_label_clicked (GtkWidget *widget,
                                  GdkEvent  *event,
                                  gpointer   user_data)
{
  AnimationEditableLabel *label = ANIMATION_EDITABLE_LABEL (user_data);

  g_signal_emit (label, animation_editable_label_signals[PREPARE_EDITING],
                 0, NULL);
  animation_editable_label_set_editing (label, TRUE);
  g_signal_emit (label, animation_editable_label_signals[EDITING],
                 0, NULL);

  return TRUE;
}

void
animation_editable_label_set_editing (AnimationEditableLabel *label,
                                      gboolean                editing)
{
  if (label->priv->is_editing != editing)
    {
      GtkWidget *current_widget;
      GtkWidget *new_widget;

      if (editing)
        {
          current_widget = label->priv->viewing_widget;
          new_widget     = label->editing_widget;
        }
      else
        {
          current_widget = label->editing_widget;
          new_widget     = label->priv->viewing_widget;
        }
      g_object_ref (current_widget);
      gtk_container_remove (GTK_CONTAINER (label),
                            current_widget);
      gtk_container_add (GTK_CONTAINER (label),
                         new_widget);

      label->priv->is_editing = editing;
    }
}

void
animation_editable_label_show_icon (AnimationEditableLabel *label,
                                    gboolean                show_icon)
{
  if (label->priv->show_icon != show_icon)
    {
      if (show_icon)
        gtk_widget_show (label->priv->image);
      else
        gtk_widget_hide (label->priv->image);
      label->priv->show_icon = show_icon;
    }
}

const gchar *
animation_editable_label_get_text (AnimationEditableLabel *label)
{
  return label->priv->text;
}

GtkWidget *
animation_editable_label_get_label (AnimationEditableLabel *label)
{
  return label->priv->label;
}
