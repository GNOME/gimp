/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpaction.c
 * Copyright (C) 2004-2019 Michael Natterer <mitch@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpmarshal.h"

#include "gimpaction.h"


enum
{
  ACTIVATE,
  CHANGE_STATE,
  LAST_SIGNAL
};


static void   gimp_action_set_proxy_tooltip (GimpAction       *action,
                                             GtkWidget        *proxy);
static void   gimp_action_tooltip_notify    (GimpAction       *action,
                                             const GParamSpec *pspec,
                                             gpointer          data);


G_DEFINE_INTERFACE (GimpAction, gimp_action, GTK_TYPE_ACTION)

static guint action_signals[LAST_SIGNAL];


static void
gimp_action_default_init (GimpActionInterface *iface)
{
  action_signals[ACTIVATE] =
    g_signal_new ("gimp-activate",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpActionInterface, activate),
                  NULL, NULL,
                  gimp_marshal_VOID__VARIANT,
                  G_TYPE_NONE, 1,
                  G_TYPE_VARIANT);

  action_signals[CHANGE_STATE] =
    g_signal_new ("gimp-change-state",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpActionInterface, change_state),
                  NULL, NULL,
                  gimp_marshal_VOID__VARIANT,
                  G_TYPE_NONE, 1,
                  G_TYPE_VARIANT);
}

void
gimp_action_init (GimpAction *action)
{
  g_return_if_fail (GIMP_IS_ACTION (action));

  g_signal_connect (action, "notify::tooltip",
                    G_CALLBACK (gimp_action_tooltip_notify),
                    NULL);
}


/*  public functions  */

void
gimp_action_emit_activate (GimpAction *action,
                           GVariant   *value)
{
  g_return_if_fail (GIMP_IS_ACTION (action));

  if (value)
    g_variant_ref_sink (value);

  g_signal_emit (action, action_signals[ACTIVATE], 0, value);

  if (value)
    g_variant_unref (value);
}

void
gimp_action_emit_change_state (GimpAction *action,
                               GVariant   *value)
{
  g_return_if_fail (GIMP_IS_ACTION (action));

  if (value)
    g_variant_ref_sink (value);

  g_signal_emit (action, action_signals[CHANGE_STATE], 0, value);

  if (value)
    g_variant_unref (value);
}

void
gimp_action_set_proxy (GimpAction *action,
                       GtkWidget  *proxy)
{
  g_return_if_fail (GIMP_IS_ACTION (action));
  g_return_if_fail (GTK_IS_WIDGET (proxy));

  gimp_action_set_proxy_tooltip (action, proxy);
}

const gchar *
gimp_action_get_name (GimpAction *action)
{
  return gtk_action_get_name ((GtkAction *) action);
}

void
gimp_action_set_label (GimpAction  *action,
                       const gchar *label)
{
  gtk_action_set_label ((GtkAction *) action, label);
}

const gchar *
gimp_action_get_label (GimpAction *action)
{
  return gtk_action_get_label ((GtkAction *) action);
}

void
gimp_action_set_tooltip (GimpAction  *action,
                         const gchar *tooltip)
{
  gtk_action_set_tooltip ((GtkAction *) action, tooltip);
}

const gchar *
gimp_action_get_tooltip (GimpAction *action)
{
  return gtk_action_get_tooltip ((GtkAction *) action);
}

void
gimp_action_set_icon_name (GimpAction  *action,
                           const gchar *icon_name)
{
  gtk_action_set_icon_name ((GtkAction *) action, icon_name);
}

const gchar *
gimp_action_get_icon_name (GimpAction *action)
{
  return gtk_action_get_icon_name ((GtkAction *) action);
}

void
gimp_action_set_gicon (GimpAction *action,
                       GIcon      *icon)
{
  gtk_action_set_gicon ((GtkAction *) action, icon);
}

GIcon *
gimp_action_get_gicon (GimpAction *action)
{
  return gtk_action_get_gicon ((GtkAction *) action);
}

void
gimp_action_set_help_id (GimpAction  *action,
                         const gchar *help_id)
{
  g_return_if_fail (GIMP_IS_ACTION (action));

  g_object_set_qdata_full (G_OBJECT (action), GIMP_HELP_ID,
                           g_strdup (help_id),
                           (GDestroyNotify) g_free);
}

const gchar *
gimp_action_get_help_id (GimpAction *action)
{
  g_return_val_if_fail (GIMP_IS_ACTION (action), NULL);

  return g_object_get_qdata (G_OBJECT (action), GIMP_HELP_ID);
}

void
gimp_action_set_visible (GimpAction *action,
                         gboolean    visible)
{
  gtk_action_set_visible ((GtkAction *) action, visible);
}

gboolean
gimp_action_get_visible (GimpAction *action)
{
  return gtk_action_get_visible ((GtkAction *) action);
}

gboolean
gimp_action_is_visible (GimpAction *action)
{
  return gtk_action_is_visible ((GtkAction *) action);
}

void
gimp_action_set_sensitive (GimpAction *action,
                           gboolean    sensitive)
{
  gtk_action_set_sensitive ((GtkAction *) action, sensitive);
}

gboolean
gimp_action_get_sensitive (GimpAction *action)
{
  return gtk_action_get_sensitive ((GtkAction *) action);
}

gboolean
gimp_action_is_sensitive (GimpAction *action)
{
  return gtk_action_is_sensitive ((GtkAction *) action);
}

GClosure *
gimp_action_get_accel_closure (GimpAction *action)
{
  return gtk_action_get_accel_closure ((GtkAction *) action);
}

void
gimp_action_set_accel_path (GimpAction  *action,
                            const gchar *accel_path)
{
  gtk_action_set_accel_path ((GtkAction *) action, accel_path);
}

const gchar *
gimp_action_get_accel_path (GimpAction *action)
{
  return gtk_action_get_accel_path ((GtkAction *) action);
}

void
gimp_action_set_accel_group (GimpAction  *action,
                             GtkAccelGroup *accel_group)
{
  gtk_action_set_accel_group ((GtkAction *) action, accel_group);
}

void
gimp_action_connect_accelerator (GimpAction  *action)
{
  gtk_action_connect_accelerator ((GtkAction *) action);
}

GSList *
gimp_action_get_proxies (GimpAction *action)
{
  return gtk_action_get_proxies ((GtkAction *) action);
}

void
gimp_action_activate (GimpAction *action)
{
  gtk_action_activate ((GtkAction *) action);
}

gint
gimp_action_name_compare (GimpAction  *action1,
                          GimpAction  *action2)
{
  return strcmp (gimp_action_get_name (action1),
                 gimp_action_get_name (action2));
}

gboolean
gimp_action_is_gui_blacklisted (const gchar *action_name)
{
  static const gchar *suffixes[] =
    {
      "-menu",
      "-popup"
    };

  static const gchar *prefixes[] =
    {
      "<",
      "tools-color-average-radius-",
      "tools-paintbrush-size-",
      "tools-paintbrush-aspect-ratio-",
      "tools-paintbrush-angle-",
      "tools-paintbrush-spacing-",
      "tools-paintbrush-hardness-",
      "tools-paintbrush-force-",
      "tools-ink-blob-size-",
      "tools-ink-blob-aspect-",
      "tools-ink-blob-angle-",
      "tools-mypaint-brush-radius-",
      "tools-mypaint-brush-hardness-",
      "tools-foreground-select-brush-size-",
      "tools-transform-preview-opacity-",
      "tools-warp-effect-size-",
      "tools-warp-effect-hardness-"
    };

  static const gchar *actions[] =
    {
      "tools-brightness-contrast",
      "tools-curves",
      "tools-levels",
      "tools-offset",
      "tools-threshold"
    };

  gint i;

  if (! (action_name && *action_name))
    return TRUE;

  for (i = 0; i < G_N_ELEMENTS (suffixes); i++)
    {
      if (g_str_has_suffix (action_name, suffixes[i]))
        return TRUE;
    }

  for (i = 0; i < G_N_ELEMENTS (prefixes); i++)
    {
      if (g_str_has_prefix (action_name, prefixes[i]))
        return TRUE;
    }

  for (i = 0; i < G_N_ELEMENTS (actions); i++)
    {
      if (! strcmp (action_name, actions[i]))
        return TRUE;
    }

  return FALSE;
}


/*  private functions  */

static void
gimp_action_set_proxy_tooltip (GimpAction *action,
                               GtkWidget  *proxy)
{
  const gchar *tooltip = gimp_action_get_tooltip (action);

  if (tooltip)
    gimp_help_set_help_data (proxy, tooltip,
                             g_object_get_qdata (G_OBJECT (proxy),
                                                 GIMP_HELP_ID));
}

static void
gimp_action_tooltip_notify (GimpAction       *action,
                            const GParamSpec *pspec,
                            gpointer          data)
{
  GSList *list;

  for (list = gimp_action_get_proxies (action);
       list;
       list = g_slist_next (list))
    {
      gimp_action_set_proxy_tooltip (action, list->data);
    }
}
