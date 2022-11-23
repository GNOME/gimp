/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaaction.c
 * Copyright (C) 2004-2019 Michael Natterer <mitch@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "ligmaaction.h"


enum
{
  ACTIVATE,
  CHANGE_STATE,
  LAST_SIGNAL
};


static void   ligma_action_set_proxy_tooltip (LigmaAction       *action,
                                             GtkWidget        *proxy);
static void   ligma_action_label_notify      (LigmaAction       *action,
                                             const GParamSpec *pspec,
                                             gpointer          data);
static void   ligma_action_tooltip_notify    (LigmaAction       *action,
                                             const GParamSpec *pspec,
                                             gpointer          data);


G_DEFINE_INTERFACE (LigmaAction, ligma_action, GTK_TYPE_ACTION)

static guint action_signals[LAST_SIGNAL];


static void
ligma_action_default_init (LigmaActionInterface *iface)
{
  action_signals[ACTIVATE] =
    g_signal_new ("ligma-activate",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaActionInterface, activate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_VARIANT);

  action_signals[CHANGE_STATE] =
    g_signal_new ("ligma-change-state",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaActionInterface, change_state),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_VARIANT);
}

void
ligma_action_init (LigmaAction *action)
{
  g_return_if_fail (LIGMA_IS_ACTION (action));

  g_signal_connect (action, "notify::label",
                    G_CALLBACK (ligma_action_label_notify),
                    NULL);
  g_signal_connect (action, "notify::tooltip",
                    G_CALLBACK (ligma_action_tooltip_notify),
                    NULL);
}


/*  public functions  */

void
ligma_action_emit_activate (LigmaAction *action,
                           GVariant   *value)
{
  g_return_if_fail (LIGMA_IS_ACTION (action));

  if (value)
    g_variant_ref_sink (value);

  g_signal_emit (action, action_signals[ACTIVATE], 0, value);

  if (value)
    g_variant_unref (value);
}

void
ligma_action_emit_change_state (LigmaAction *action,
                               GVariant   *value)
{
  g_return_if_fail (LIGMA_IS_ACTION (action));

  if (value)
    g_variant_ref_sink (value);

  g_signal_emit (action, action_signals[CHANGE_STATE], 0, value);

  if (value)
    g_variant_unref (value);
}

void
ligma_action_set_proxy (LigmaAction *action,
                       GtkWidget  *proxy)
{
  g_return_if_fail (LIGMA_IS_ACTION (action));
  g_return_if_fail (GTK_IS_WIDGET (proxy));

  ligma_action_set_proxy_tooltip (action, proxy);
}

const gchar *
ligma_action_get_name (LigmaAction *action)
{
  return gtk_action_get_name ((GtkAction *) action);
}

void
ligma_action_set_label (LigmaAction  *action,
                       const gchar *label)
{
  gtk_action_set_label ((GtkAction *) action, label);
}

const gchar *
ligma_action_get_label (LigmaAction *action)
{
  return gtk_action_get_label ((GtkAction *) action);
}

void
ligma_action_set_tooltip (LigmaAction  *action,
                         const gchar *tooltip)
{
  gtk_action_set_tooltip ((GtkAction *) action, tooltip);
}

const gchar *
ligma_action_get_tooltip (LigmaAction *action)
{
  return gtk_action_get_tooltip ((GtkAction *) action);
}

void
ligma_action_set_icon_name (LigmaAction  *action,
                           const gchar *icon_name)
{
  gtk_action_set_icon_name ((GtkAction *) action, icon_name);
}

const gchar *
ligma_action_get_icon_name (LigmaAction *action)
{
  return gtk_action_get_icon_name ((GtkAction *) action);
}

void
ligma_action_set_gicon (LigmaAction *action,
                       GIcon      *icon)
{
  gtk_action_set_gicon ((GtkAction *) action, icon);
}

GIcon *
ligma_action_get_gicon (LigmaAction *action)
{
  return gtk_action_get_gicon ((GtkAction *) action);
}

void
ligma_action_set_help_id (LigmaAction  *action,
                         const gchar *help_id)
{
  g_return_if_fail (LIGMA_IS_ACTION (action));

  g_object_set_qdata_full (G_OBJECT (action), LIGMA_HELP_ID,
                           g_strdup (help_id),
                           (GDestroyNotify) g_free);
}

const gchar *
ligma_action_get_help_id (LigmaAction *action)
{
  g_return_val_if_fail (LIGMA_IS_ACTION (action), NULL);

  return g_object_get_qdata (G_OBJECT (action), LIGMA_HELP_ID);
}

void
ligma_action_set_visible (LigmaAction *action,
                         gboolean    visible)
{
  gtk_action_set_visible ((GtkAction *) action, visible);
}

gboolean
ligma_action_get_visible (LigmaAction *action)
{
  return gtk_action_get_visible ((GtkAction *) action);
}

gboolean
ligma_action_is_visible (LigmaAction *action)
{
  return gtk_action_is_visible ((GtkAction *) action);
}

void
ligma_action_set_sensitive (LigmaAction  *action,
                           gboolean     sensitive,
                           const gchar *reason)
{
  gtk_action_set_sensitive ((GtkAction *) action, sensitive);

  if (LIGMA_ACTION_GET_INTERFACE (action)->set_disable_reason)
    LIGMA_ACTION_GET_INTERFACE (action)->set_disable_reason (action,
                                                            ! sensitive ? reason : NULL);
}

gboolean
ligma_action_get_sensitive (LigmaAction   *action,
                           const gchar **reason)
{
  gboolean sensitive;

  sensitive = gtk_action_get_sensitive ((GtkAction *) action);

  if (reason)
    {
      *reason = NULL;
      if (! sensitive && LIGMA_ACTION_GET_INTERFACE (action)->get_disable_reason)
        *reason = LIGMA_ACTION_GET_INTERFACE (action)->get_disable_reason (action);
    }

  return sensitive;
}

gboolean
ligma_action_is_sensitive (LigmaAction   *action,
                          const gchar **reason)
{
  gboolean sensitive;

  sensitive = gtk_action_is_sensitive ((GtkAction *) action);

  if (reason)
    {
      *reason = NULL;
      if (! sensitive && LIGMA_ACTION_GET_INTERFACE (action)->get_disable_reason)
        *reason = LIGMA_ACTION_GET_INTERFACE (action)->get_disable_reason (action);
    }

  return sensitive;
}

GClosure *
ligma_action_get_accel_closure (LigmaAction *action)
{
  return gtk_action_get_accel_closure ((GtkAction *) action);
}

void
ligma_action_set_accel_path (LigmaAction  *action,
                            const gchar *accel_path)
{
  gtk_action_set_accel_path ((GtkAction *) action, accel_path);
}

const gchar *
ligma_action_get_accel_path (LigmaAction *action)
{
  return gtk_action_get_accel_path ((GtkAction *) action);
}

void
ligma_action_set_accel_group (LigmaAction  *action,
                             GtkAccelGroup *accel_group)
{
  gtk_action_set_accel_group ((GtkAction *) action, accel_group);
}

void
ligma_action_connect_accelerator (LigmaAction  *action)
{
  gtk_action_connect_accelerator ((GtkAction *) action);
}

GSList *
ligma_action_get_proxies (LigmaAction *action)
{
  return gtk_action_get_proxies ((GtkAction *) action);
}

void
ligma_action_activate (LigmaAction *action)
{
  gtk_action_activate ((GtkAction *) action);
}

gint
ligma_action_name_compare (LigmaAction  *action1,
                          LigmaAction  *action2)
{
  return strcmp (ligma_action_get_name (action1),
                 ligma_action_get_name (action2));
}

gboolean
ligma_action_is_gui_blacklisted (const gchar *action_name)
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
ligma_action_set_proxy_tooltip (LigmaAction *action,
                               GtkWidget  *proxy)
{
  const gchar *tooltip;
  const gchar *reason         = NULL;
  gchar       *escaped_reason = NULL;
  gchar       *markup;

  tooltip = ligma_action_get_tooltip (action);

  ligma_action_get_sensitive (action, &reason);
  if (reason)
    escaped_reason = g_markup_escape_text (reason, -1);

  markup = g_strdup_printf ("%s%s"                                   /* Action tooltip  */
                            "<i><span weight='light'>%s</span></i>", /* Inactive reason */
                            tooltip,
                            escaped_reason && tooltip ? "\n" : "",
                            escaped_reason ? escaped_reason : "");

  if (tooltip || escaped_reason)
    ligma_help_set_help_data_with_markup (proxy, markup,
                                         g_object_get_qdata (G_OBJECT (proxy),
                                                             LIGMA_HELP_ID));

  g_free (escaped_reason);
  g_free (markup);
}

static void
ligma_action_label_notify (LigmaAction       *action,
                          const GParamSpec *pspec,
                          gpointer          data)
{
  GSList *list;

  for (list = ligma_action_get_proxies (action);
       list;
       list = g_slist_next (list))
    {
      if (GTK_IS_MENU_ITEM (list->data))
        {
          GtkWidget *child = gtk_bin_get_child (GTK_BIN (list->data));

          if (GTK_IS_BOX (child))
            {
              child = g_object_get_data (G_OBJECT (list->data),
                                         "ligma-menu-item-label");

              if (GTK_IS_LABEL (child))
                gtk_label_set_text (GTK_LABEL (child),
                                    ligma_action_get_label (action));
            }
        }
    }
}

static void
ligma_action_tooltip_notify (LigmaAction       *action,
                            const GParamSpec *pspec,
                            gpointer          data)
{
  GSList *list;

  for (list = ligma_action_get_proxies (action);
       list;
       list = g_slist_next (list))
    {
      ligma_action_set_proxy_tooltip (action, list->data);
    }
}
