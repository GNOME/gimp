/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpshortcutbutton.c
 * Copyright (C) 2022 Jehan
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpshortcutbutton.h"


enum
{
  PROP_0,
  PROP_ACCELERATOR
};

struct _GimpShortcutButtonPrivate
{
  gchar           *accelerator;

  GtkWidget       *label;

  gboolean         modifier_only_accepted;
  gboolean         single_modifier;

  guint            timer;
  guint            last_keyval;
  GdkModifierType  last_modifiers;
};


static void     gimp_shortcut_button_constructed     (GObject            *object);
static void     gimp_shortcut_button_finalize        (GObject            *object);
static void     gimp_shortcut_button_set_property    (GObject            *object,
                                                      guint               property_id,
                                                      const GValue       *value,
                                                      GParamSpec         *pspec);
static void     gimp_shortcut_button_get_property    (GObject            *object,
                                                      guint               property_id,
                                                      GValue             *value,
                                                      GParamSpec         *pspec);

static gboolean gimp_shortcut_button_key_press_event (GtkWidget   *button,
                                                      GdkEventKey *event,
                                                      gpointer     user_data);
static gboolean gimp_shortcut_button_focus_out_event (GimpShortcutButton* button,
                                                      GdkEventFocus       event,
                                                      gpointer            user_data);

static gboolean gimp_shortcut_button_timeout         (GimpShortcutButton *button);


G_DEFINE_TYPE_WITH_PRIVATE (GimpShortcutButton, gimp_shortcut_button,
                            GTK_TYPE_TOGGLE_BUTTON)

#define parent_class gimp_shortcut_button_parent_class


static void
gimp_shortcut_button_class_init (GimpShortcutButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_shortcut_button_constructed;
  object_class->finalize     = gimp_shortcut_button_finalize;
  object_class->get_property = gimp_shortcut_button_get_property;
  object_class->set_property = gimp_shortcut_button_set_property;

  g_object_class_install_property (object_class, PROP_ACCELERATOR,
                                   g_param_spec_string ("accelerator",
                                                        NULL, NULL, NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_EXPLICIT_NOTIFY));
}

static void
gimp_shortcut_button_init (GimpShortcutButton *button)
{
  button->priv = gimp_shortcut_button_get_instance_private (button);

  button->priv->timer = 0;

  button->priv->label = gtk_shortcut_label_new (NULL);
  gtk_container_add (GTK_CONTAINER (button), button->priv->label);
  gtk_widget_show (button->priv->label);
}

static void
gimp_shortcut_button_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect (object, "key-press-event",
                    G_CALLBACK (gimp_shortcut_button_key_press_event),
                    NULL);
  g_signal_connect (object, "focus-out-event",
                    G_CALLBACK (gimp_shortcut_button_focus_out_event),
                    NULL);
}

static void
gimp_shortcut_button_finalize (GObject *object)
{
  GimpShortcutButton *button = GIMP_SHORTCUT_BUTTON (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);

  g_free (button->priv->accelerator);
  if (button->priv->timer != 0)
    {
      g_source_remove (button->priv->timer);
      button->priv->timer = 0;
    }
}

static void
gimp_shortcut_button_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpShortcutButton *button = GIMP_SHORTCUT_BUTTON (object);

  switch (property_id)
    {
    case PROP_ACCELERATOR:
      gimp_shortcut_button_set_accelerator (button, g_value_get_string (value));
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_shortcut_button_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpShortcutButton *button = GIMP_SHORTCUT_BUTTON (object);

  switch (property_id)
    {
    case PROP_ACCELERATOR:
      g_value_set_string (value, button->priv->accelerator);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/*  public functions  */

GtkWidget *
gimp_shortcut_button_new (const gchar *accelerator)
{
  GimpShortcutButton *button;

  button = g_object_new (GIMP_TYPE_SHORTCUT_BUTTON,
                         "accelerator", accelerator,
                         NULL);

  return GTK_WIDGET (button);
}

const gchar *
gimp_shortcut_button_get_accelerator (GimpShortcutButton *button)
{
  g_return_val_if_fail (GIMP_IS_SHORTCUT_BUTTON (button), NULL);

  return button->priv->accelerator;
}

void
gimp_shortcut_button_set_accelerator (GimpShortcutButton *button,
                                      const gchar        *accelerator)
{
  g_return_if_fail (GIMP_IS_SHORTCUT_BUTTON (button));

  if (g_strcmp0 (accelerator, button->priv->accelerator) != 0)
    {
      g_free (button->priv->accelerator);
      button->priv->accelerator = accelerator ? g_strdup (accelerator) : NULL;

      if (accelerator)
        gtk_shortcut_label_set_accelerator (GTK_SHORTCUT_LABEL (button->priv->label), accelerator);

      g_object_notify (G_OBJECT (button), "accelerator");
    }
}

/**
 * gimp_shortcut_button_accepts_modifier:
 * @accepted: are shortcuts with only modifiers accepted?
 * @unique:   if @only and @unique are both %TRUE, then the button will
 *            grab a single modifier
 *
 * By default, the shortcut button will accept shortcuts such as `a` or
 * `Ctrl-a` or `Shift-Ctrl-a`. A modifier-only shortcut such as `Ctrl`
 * will not be recorded.
 * If @accepted is %TRUE, then it is the opposite. In such case, 2 cases
 * are possible: if @unique is %FALSE, then single modifiers are
 * recorded, such as `Ctrl` or `Shift`, but not `Ctrl-Shift`; if @unique
 * is %FALSE, it may record both `Ctrl` and `Ctrl-Shift` or any modifier
 * combination.
 */
void
gimp_shortcut_button_accepts_modifier (GimpShortcutButton *button,
                                       gboolean            accepted,
                                       gboolean            unique)
{
  g_return_if_fail (GIMP_IS_SHORTCUT_BUTTON (button));

  button->priv->modifier_only_accepted = accepted;
  button->priv->single_modifier        = unique;
}

/* Private functions. */

static gboolean
gimp_shortcut_button_key_press_event (GtkWidget   *widget,
                                      GdkEventKey *event,
                                      gpointer     user_data)
{
  GimpShortcutButton *button = GIMP_SHORTCUT_BUTTON (widget);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      GdkModifierType accel_mods = 0;

      accel_mods = event->state;
      accel_mods &= gtk_accelerator_get_default_mod_mask ();

      if (button->priv->modifier_only_accepted)
        {
          if (button->priv->single_modifier)
            {
              if (event->is_modifier)
                {
                  gchar *accelerator;

                  accelerator = gtk_accelerator_name (event->keyval, 0);
                  gimp_shortcut_button_set_accelerator (button, accelerator);
                  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
                  g_free (accelerator);
                }
            }
          else
            {
              button->priv->last_keyval    = event->keyval;
              button->priv->last_modifiers = accel_mods;
              if ((event->is_modifier || accel_mods != 0) && button->priv->timer == 0)
                {
                  button->priv->timer = g_timeout_add (250,
                                                       (GSourceFunc) gimp_shortcut_button_timeout,
                                                       button);
                }
            }
        }
      else if (! event->is_modifier)
        {
          gchar *accelerator;

          accelerator = gtk_accelerator_name (event->keyval, accel_mods);
          gimp_shortcut_button_set_accelerator (button, accelerator);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
          g_free (accelerator);
        }
      return TRUE;
    }
  return FALSE;
}

static gboolean
gimp_shortcut_button_focus_out_event (GimpShortcutButton* button,
                                      GdkEventFocus       event,
                                      gpointer            user_data)
{
  /* On losing focus, we untoggle, so we don't have to grab anything.
   * Let's avoid cases with several shortcut buttons all grabing the
   * same shortcuts for instance.
   */
  if (button->priv->timer != 0)
    {
      g_source_remove (button->priv->timer);
      button->priv->timer = 0;
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

  /* Propagate event further. */
  return FALSE;
}

static gboolean
gimp_shortcut_button_timeout (GimpShortcutButton *button)
{
  gchar *accelerator;

  accelerator = gtk_accelerator_name (button->priv->last_keyval,
                                      button->priv->last_modifiers);
  gimp_shortcut_button_set_accelerator (button, accelerator);
  g_free (accelerator);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

  button->priv->timer = 0;

  return G_SOURCE_REMOVE;
}
