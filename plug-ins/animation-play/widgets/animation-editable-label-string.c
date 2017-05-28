/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Editable label with entry
 * Copyright (C) 2015-2017 Jehan <jehan@gimp.org>
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

#include <gdk/gdkkeysyms.h>

#include "animation-editable-label.h"
#include "animation-editable-label-string.h"

struct _AnimationEditableLabelStringPrivate
{
  GtkWidget *editing_widget;
};

static void     animation_editable_label_constructed     (GObject              *object);
static void     animation_editable_label_icon_clicked    (GtkWidget            *widget,
                                                          GtkEntryIconPosition  icon_pos,
                                                          GdkEvent             *event,
                                                          gpointer              user_data);
static void     animation_editable_label_activate        (GtkWidget            *widget,
                                                          gpointer              user_data);
static gboolean animation_editable_label_focus_out       (GtkWidget            *widget,
                                                          GdkEvent             *event,
                                                          gpointer              user_data);
static gboolean animation_editable_label_key_press       (GtkWidget            *widget,
                                                          GdkEvent             *event,
                                                          gpointer              user_data);
static void     animation_editable_label_prepare_editing (GtkWidget            *widget,
                                                          gpointer              user_data);
static void     animation_editable_label_editing         (GtkWidget            *widget,
                                                          gpointer              user_data);

G_DEFINE_TYPE (AnimationEditableLabelString, animation_editable_label_string, ANIMATION_TYPE_EDITABLE_LABEL)

#define parent_class animation_editable_label_string_parent_class

static void
animation_editable_label_string_class_init (AnimationEditableLabelStringClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = animation_editable_label_constructed;

  g_type_class_add_private (klass, sizeof (AnimationEditableLabelStringPrivate));
}

static void
animation_editable_label_string_init (AnimationEditableLabelString *label)
{
  label->priv = G_TYPE_INSTANCE_GET_PRIVATE (label,
                                             ANIMATION_TYPE_EDITABLE_LABEL_STRING,
                                             AnimationEditableLabelStringPrivate);
}

static void
animation_editable_label_constructed (GObject *object)
{
  AnimationEditableLabel *label = ANIMATION_EDITABLE_LABEL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* The editing widget. */
  gtk_widget_set_can_focus (label->editing_widget, TRUE);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (label->editing_widget),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     "gtk-apply");
  if (animation_editable_label_get_text (label))
    gtk_entry_set_text (GTK_ENTRY (label->editing_widget),
                        animation_editable_label_get_text (label));
  g_signal_connect (label->editing_widget, "icon-release",
                    G_CALLBACK (animation_editable_label_icon_clicked),
                    label);
  g_signal_connect (label->editing_widget, "activate",
                    G_CALLBACK (animation_editable_label_activate),
                    label);
  g_signal_connect (label->editing_widget, "focus-out-event",
                    G_CALLBACK (animation_editable_label_focus_out),
                    label);
  g_signal_connect (label->editing_widget, "key-press-event",
                    G_CALLBACK (animation_editable_label_key_press),
                    label);
  gtk_widget_show (label->editing_widget);

  g_signal_connect (label, "prepare-editing",
                    G_CALLBACK (animation_editable_label_prepare_editing),
                    NULL);
  g_signal_connect (label, "editing",
                    G_CALLBACK (animation_editable_label_editing),
                    NULL);
}

GtkWidget *
animation_editable_label_string_new (const gchar *default_text)
{
  GtkWidget *widget;
  GtkWidget *entry;

  entry = gtk_entry_new ();
  widget = g_object_new (ANIMATION_TYPE_EDITABLE_LABEL_STRING,
                         "text", default_text,
                         "editing-widget", entry,
                         NULL);

  return widget;
}

static void
animation_editable_label_icon_clicked (GtkWidget            *widget,
                                       GtkEntryIconPosition  icon_pos,
                                       GdkEvent             *event,
                                       gpointer              user_data)
{
  animation_editable_label_activate (widget, user_data);
}

static void
animation_editable_label_activate (GtkWidget *widget,
                                   gpointer   user_data)
{
  AnimationEditableLabel *label = ANIMATION_EDITABLE_LABEL (user_data);
  GtkEntry          *entry      = GTK_ENTRY (widget);

  g_object_set (label,
                "text", gtk_entry_get_text (entry),
                NULL);
  g_signal_handlers_block_by_func (label->editing_widget,
                                   G_CALLBACK (animation_editable_label_focus_out),
                                   label);
  animation_editable_label_set_editing (label, FALSE);
  g_signal_handlers_unblock_by_func (label->editing_widget,
                                     G_CALLBACK (animation_editable_label_focus_out),
                                     label);
}

static gboolean
animation_editable_label_focus_out (GtkWidget *widget,
                                    GdkEvent  *event,
                                    gpointer   user_data)
{
  animation_editable_label_activate (widget, user_data);

  /* Return FALSE because GtkEntry is expecting for this signal. */
  return FALSE;
}

static gboolean
animation_editable_label_key_press (GtkWidget *widget,
                                    GdkEvent  *event,
                                    gpointer   user_data)
{
  AnimationEditableLabel *label = ANIMATION_EDITABLE_LABEL (user_data);
  GdkEventKey       *event_key  = (GdkEventKey*) event;

  if (event_key->keyval == GDK_KEY_Escape)
    {
      /* Discard entry contents and revert to read-only label. */
      g_signal_handlers_block_by_func (label->editing_widget,
                                       G_CALLBACK (animation_editable_label_focus_out),
                                       label);
      animation_editable_label_set_editing (label, FALSE);
      g_signal_handlers_unblock_by_func (label->editing_widget,
                                         G_CALLBACK (animation_editable_label_focus_out),
                                         label);
    }

  return FALSE;
}

static void
animation_editable_label_prepare_editing (GtkWidget *widget,
                                          gpointer   user_data)
{
  AnimationEditableLabel *label = ANIMATION_EDITABLE_LABEL (widget);

  gtk_entry_set_text (GTK_ENTRY (label->editing_widget),
                      animation_editable_label_get_text (label));
}

static void
animation_editable_label_editing (GtkWidget *widget,
                                  gpointer   user_data)
{
  AnimationEditableLabel *label = ANIMATION_EDITABLE_LABEL (widget);

  gtk_widget_grab_focus (GTK_WIDGET (label->editing_widget));
  gtk_editable_set_position (GTK_EDITABLE (label->editing_widget), -1);
}
