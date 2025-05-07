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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimagefile.h"  /* eek */

#include "gimpaction.h"
#include "gimpprocedureaction.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"


enum
{
  ACTIVATE,
  ACCELS_CHANGED,
  LAST_SIGNAL
};

#define GET_PRIVATE(obj) (gimp_action_get_private ((GimpAction *) (obj)))

typedef struct _GimpActionPrivate GimpActionPrivate;

struct _GimpActionPrivate
{
  GimpContext           *context;
  /* This recursive pointer is needed for the finalize(). */
  GimpAction            *action;
  GimpActionGroup       *group;

  gboolean               sensitive;
  gchar                 *disable_reason;

  gboolean               visible;

  gchar                 *label;
  gchar                 *short_label;
  gchar                 *tooltip;
  gchar                 *icon_name;
  GIcon                 *icon;

  gchar                **accels;
  gchar                **default_accels;

  gchar                 *menu_path;

  GeglColor             *color;
  GimpViewable          *viewable;
  PangoEllipsizeMode     ellipsize;
  gint                   max_width_chars;

  GList                 *proxies;
};


static GimpActionPrivate *
              gimp_action_get_private            (GimpAction        *action);
static void   gimp_action_private_finalize       (GimpActionPrivate *priv);

static void   gimp_action_label_notify           (GimpAction        *action,
                                                  const GParamSpec  *pspec,
                                                  gpointer           data);

static void   gimp_action_proxy_destroy          (GtkWidget         *proxy,
                                                  GimpAction        *action);
static void   gimp_action_proxy_button_activate  (GtkButton         *button,
                                                  GimpAction        *action);

static void   gimp_action_update_proxy_sensitive (GimpAction        *action,
                                                  GtkWidget         *proxy);
static void   gimp_action_update_proxy_visible   (GimpAction        *action,
                                                  GtkWidget         *proxy);
static void   gimp_action_update_proxy_tooltip   (GimpAction        *action,
                                                  GtkWidget         *proxy);


G_DEFINE_INTERFACE (GimpAction, gimp_action, GIMP_TYPE_OBJECT)

static guint action_signals[LAST_SIGNAL];


static void
gimp_action_default_init (GimpActionInterface *iface)
{
  action_signals[ACTIVATE] =
    g_signal_new ("activate",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpActionInterface, activate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_VARIANT);

  action_signals[ACCELS_CHANGED] =
    g_signal_new ("accels-changed",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpActionInterface, accels_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRV);

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("context",
                                                            NULL, NULL,
                                                            GIMP_TYPE_CONTEXT,
                                                            GIMP_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY));
  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("sensitive",
                                                             NULL, NULL,
                                                             TRUE,
                                                             GIMP_PARAM_READWRITE |
                                                             G_PARAM_EXPLICIT_NOTIFY));
  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("visible",
                                                             NULL, NULL,
                                                             TRUE,
                                                             GIMP_PARAM_READWRITE |
                                                             G_PARAM_EXPLICIT_NOTIFY));
  g_object_interface_install_property (iface,
                                       g_param_spec_string ("label",
                                                            NULL, NULL,
                                                            NULL,
                                                            GIMP_PARAM_READWRITE |
                                                            G_PARAM_EXPLICIT_NOTIFY));
  g_object_interface_install_property (iface,
                                       g_param_spec_string ("short-label",
                                                            NULL, NULL,
                                                            NULL,
                                                            GIMP_PARAM_READWRITE |
                                                            G_PARAM_EXPLICIT_NOTIFY));
  g_object_interface_install_property (iface,
                                       g_param_spec_string ("tooltip",
                                                            NULL, NULL,
                                                            NULL,
                                                            GIMP_PARAM_READWRITE));
  g_object_interface_install_property (iface,
                                       g_param_spec_string ("icon-name",
                                                            NULL, NULL,
                                                            NULL,
                                                            GIMP_PARAM_READWRITE |
                                                            G_PARAM_EXPLICIT_NOTIFY));
  g_object_interface_install_property (iface,
                                       g_param_spec_object ("icon",
                                                            NULL, NULL,
                                                            G_TYPE_ICON,
                                                            GIMP_PARAM_READWRITE |
                                                            G_PARAM_EXPLICIT_NOTIFY));

  g_object_interface_install_property (iface,
                                       gimp_param_spec_color_from_string ("color",
                                                                          NULL, NULL,
                                                                          TRUE, "black",
                                                                          GIMP_PARAM_READWRITE));
  g_object_interface_install_property (iface,
                                       g_param_spec_object ("viewable",
                                                            NULL, NULL,
                                                            GIMP_TYPE_VIEWABLE,
                                                            GIMP_PARAM_READWRITE));

  g_object_interface_install_property (iface,
                                       g_param_spec_enum ("ellipsize",
                                                          NULL, NULL,
                                                          PANGO_TYPE_ELLIPSIZE_MODE,
                                                          PANGO_ELLIPSIZE_NONE,
                                                          GIMP_PARAM_READWRITE));
  g_object_interface_install_property (iface,
                                       g_param_spec_int ("max-width-chars",
                                                         NULL, NULL,
                                                         -1, G_MAXINT, -1,
                                                         GIMP_PARAM_READWRITE));
}

void
gimp_action_init (GimpAction *action)
{
  GimpActionPrivate *priv;

  g_return_if_fail (GIMP_IS_ACTION (action));

  priv = GET_PRIVATE (action);

  priv->action          = action;
  priv->group           = NULL;
  priv->sensitive       = TRUE;
  priv->visible         = TRUE;
  priv->label           = NULL;
  priv->short_label     = NULL;
  priv->tooltip         = NULL;
  priv->icon_name       = NULL;
  priv->icon            = NULL;
  priv->accels          = NULL;
  priv->default_accels  = NULL;
  priv->menu_path       = NULL;
  priv->ellipsize       = PANGO_ELLIPSIZE_NONE;
  priv->max_width_chars = -1;
  priv->proxies         = NULL;

  g_signal_connect (action, "notify::label",
                    G_CALLBACK (gimp_action_label_notify),
                    NULL);
  g_signal_connect (action, "notify::short-label",
                    G_CALLBACK (gimp_action_label_notify),
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

  g_signal_emit_by_name (action, "change-state", value);

  if (value)
    g_variant_unref (value);
}

const gchar *
gimp_action_get_name (GimpAction *action)
{
  return gimp_object_get_name (GIMP_OBJECT (action));
}

GimpActionGroup *
gimp_action_get_group (GimpAction *action)
{
  return GET_PRIVATE (action)->group;
}

void
gimp_action_set_label (GimpAction  *action,
                       const gchar *label)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  if (g_strcmp0 (priv->label, label) != 0)
    {
      g_free (priv->label);
      priv->label = g_strdup (label);

      /* Set or update the proxy rendering. */
      for (GList *list = priv->proxies; list; list = list->next)
        gimp_action_set_proxy (action, list->data);

      g_object_notify (G_OBJECT (action), "label");
      if (priv->short_label == NULL)
        g_object_notify (G_OBJECT (action), "short-label");
    }
}

const gchar *
gimp_action_get_label (GimpAction *action)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  return priv->label;
}

void
gimp_action_set_short_label (GimpAction  *action,
                             const gchar *label)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  if (g_strcmp0 (priv->short_label, label) != 0)
    {
      g_free (priv->short_label);
      priv->short_label = g_strdup (label);

      /* Set or update the proxy rendering. */
      for (GList *list = priv->proxies; list; list = list->next)
        gimp_action_set_proxy (action, list->data);

      g_object_notify (G_OBJECT (action), "short-label");
    }
}

const gchar *
gimp_action_get_short_label (GimpAction *action)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  return priv->short_label ? priv->short_label : priv->label;
}

void
gimp_action_set_tooltip (GimpAction  *action,
                         const gchar *tooltip)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  if (g_strcmp0 (priv->tooltip, tooltip) != 0)
    {
      g_free (priv->tooltip);
      priv->tooltip = g_strdup (tooltip);

      gimp_action_update_proxy_tooltip (action, NULL);

      g_object_notify (G_OBJECT (action), "tooltip");
    }
}

const gchar *
gimp_action_get_tooltip (GimpAction *action)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  return priv->tooltip;
}

void
gimp_action_set_icon_name (GimpAction  *action,
                           const gchar *icon_name)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  if (g_strcmp0 (priv->icon_name, icon_name) != 0)
    {
      g_free (priv->icon_name);
      priv->icon_name = g_strdup (icon_name);

      /* Set or update the proxy rendering. */
      for (GList *list = priv->proxies; list; list = list->next)
        gimp_action_set_proxy (action, list->data);

      g_object_notify (G_OBJECT (action), "icon-name");
    }
}

const gchar *
gimp_action_get_icon_name (GimpAction *action)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  return priv->icon_name;
}

void
gimp_action_set_gicon (GimpAction *action,
                       GIcon      *icon)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  if (priv->icon != icon)
    {
      g_clear_object (&priv->icon);
      priv->icon = g_object_ref (icon);

      /* Set or update the proxy rendering. */
      for (GList *list = priv->proxies; list; list = list->next)
        gimp_action_set_proxy (action, list->data);

      g_object_notify (G_OBJECT (action), "icon");
    }
}

GIcon *
gimp_action_get_gicon (GimpAction *action)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  return priv->icon ? g_object_ref (priv->icon) : NULL;
}

void
gimp_action_set_help_id (GimpAction  *action,
                         const gchar *help_id)
{
  g_return_if_fail (GIMP_IS_ACTION (action));

  g_object_set_qdata_full (G_OBJECT (action), GIMP_HELP_ID,
                           g_strdup (help_id),
                           (GDestroyNotify) g_free);
  gimp_action_update_proxy_tooltip (action, NULL);
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
  GimpActionPrivate *priv = GET_PRIVATE (action);

  /* Only notify when the state actually changed. This is important for
   * handlers such as visibility of menu items in GimpMenuModel which
   * will assume that the action visibility changed. Otherwise we might
   * remove items by mistake.
   */
  if (priv->visible != visible)
    {
      priv->visible = visible;

      gimp_action_update_proxy_visible (action, NULL);
      g_object_notify (G_OBJECT (action), "visible");
    }
}

gboolean
gimp_action_get_visible (GimpAction *action)
{
  return GET_PRIVATE (action)->visible;
}

gboolean
gimp_action_is_visible (GimpAction *action)
{
  gboolean visible;

  visible = gimp_action_get_visible (action);

  if (visible)
    {
      /* TODO: check if the action group itself is visible.
       * See implementation of gtk_action_is_visible().
       */
    }

  return visible;
}

void
gimp_action_set_sensitive (GimpAction  *action,
                           gboolean     sensitive,
                           const gchar *reason)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  if (priv->sensitive != sensitive ||
      (! sensitive && g_strcmp0 (reason, priv->disable_reason) != 0))
    {
      priv->sensitive = sensitive;

      g_clear_pointer (&priv->disable_reason, g_free);
      if (reason && ! sensitive)
        priv->disable_reason = g_strdup (reason);

      g_object_notify (G_OBJECT (action), "sensitive");

      gimp_action_update_proxy_sensitive (action, NULL);

      g_object_notify (G_OBJECT (action), "enabled");
    }
}

gboolean
gimp_action_get_sensitive (GimpAction   *action,
                           const gchar **reason)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);
  gboolean           sensitive;

  sensitive = priv->sensitive;

  if (reason)
    {
      GimpActionPrivate *priv = GET_PRIVATE (action);

      *reason = NULL;

      if (! sensitive && priv->disable_reason != NULL)
        *reason = (const gchar *) priv->disable_reason;
    }

  return sensitive;
}

gboolean
gimp_action_is_sensitive (GimpAction   *action,
                          const gchar **reason)
{
  gboolean sensitive;

  sensitive = gimp_action_get_sensitive (action, reason);

  if (sensitive)
    {
      /* TODO: check if the action group itself is sensitive.
       * See implementation of gtk_action_is_sensitive().
       */
    }

  return sensitive;
}

/**
 * gimp_action_set_accels:
 * @action: a #GimpAction
 * @accels: accelerators in the format understood by gtk_accelerator_parse().
 *          The first accelerator is the main one.
 *
 * Set the accelerators to be associated with the given @action.
 *
 * Note that the #GimpAction API will emit the signal "accels-changed" whereas
 * GtkApplication has no signal (that we could find) to connect to. It means we
 * must always change accelerators with this function.
 * Never use gtk_application_set_accels_for_action() directly!
 */
void
gimp_action_set_accels (GimpAction   *action,
                        const gchar **accels)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  g_return_if_fail (GIMP_IS_ACTION (action));

  if (accels != NULL && priv->accels != NULL &&
      g_strv_equal (accels, (const gchar **) priv->accels))
    return;

  g_strfreev (priv->accels);
  priv->accels = g_strdupv ((gchar **) accels);

  g_signal_emit (action, action_signals[ACCELS_CHANGED], 0, accels);
}

/**
 * gimp_action_get_accels:
 * @action: a #GimpAction
 *
 * Gets the accelerators that are currently associated with
 * the given @action.
 *
 * Returns: (transfer full): accelerators for @action, as a %NULL-terminated
 *          array. Free with g_strfreev() when no longer needed
 */
const gchar **
gimp_action_get_accels (GimpAction *action)
{
  g_return_val_if_fail (GIMP_IS_ACTION (action), NULL);

  return (const gchar **) GET_PRIVATE (action)->accels;
}

/**
 * gimp_action_get_default_accels:
 * @action: a #GimpAction
 *
 * Gets the accelerators that are associated with the given @action by default.
 * These might be different from gimp_action_get_accels().
 *
 * Returns: (transfer full): default accelerators for @action, as a
 *                           %NULL-terminated array. Free with g_strfreev() when
 *                           no longer needed
 */
const gchar **
gimp_action_get_default_accels (GimpAction *action)
{
  g_return_val_if_fail (GIMP_IS_ACTION (action), NULL);

  return (const gchar **) GET_PRIVATE (action)->default_accels;
}

/**
 * gimp_action_get_display_accels:
 * @action: a #GimpAction
 *
 * Gets the accelerators that are currently associated with the given @action,
 * in a format which can be presented to people on the GUI.
 *
 * Returns: (transfer full): accelerators for @action, as a %NULL-terminated
 *          array. Free with g_strfreev() when no longer needed
 */
gchar **
gimp_action_get_display_accels (GimpAction *action)
{
  gchar **accels;
  gint    i;

  g_return_val_if_fail (GIMP_IS_ACTION (action), NULL);

  accels = g_strdupv (GET_PRIVATE (action)->accels);

  for (i = 0; accels != NULL && accels[i] != NULL; i++)
    {
      guint           accel_key = 0;
      GdkModifierType accel_mods = 0;

      gtk_accelerator_parse (accels[i], &accel_key, &accel_mods);

      if (accel_key != 0 || accel_mods != 0)
        {
          gchar *accel;

          accel = gtk_accelerator_get_label (accel_key, accel_mods);

          g_free (accels[i]);
          accels[i] = accel;
        }
    }

  return accels;
}

/* This should return FALSE if the currently set accelerators are not the
 * default accelerators or even if they are in different order (different
 * primary accelerator in particular).
 */
gboolean
gimp_action_use_default_accels (GimpAction *action)
{
  gchar **default_accels;
  gchar **accels;

  g_return_val_if_fail (GIMP_IS_ACTION (action), TRUE);

  default_accels = GET_PRIVATE (action)->default_accels;
  accels         = GET_PRIVATE (action)->accels;

  if ((default_accels == NULL || g_strv_length (default_accels) == 0) &&
      (accels == NULL || g_strv_length (accels) == 0))
    return TRUE;

  if (default_accels == NULL || accels == NULL ||
      g_strv_length (default_accels) != g_strv_length (accels))
    return FALSE;

  /* An easy looking variant would be to simply compare with g_strv_equal() but
   * this actually doesn't work because gtk_accelerator_parse() is liberal with
   * the format (casing and abbreviations) and thus we need to have a canonical
   * string version, or simply parse the accelerators down to get to the base
   * key/modes, which is what I do here.
   */
  for (gint i = 0; accels[i] != NULL; i++)
    {
      guint           default_key;
      GdkModifierType default_mods;
      guint           accelerator_key;
      GdkModifierType accelerator_mods;

      gtk_accelerator_parse (default_accels[i], &default_key, &default_mods);
      gtk_accelerator_parse (accels[i], &accelerator_key, &accelerator_mods);

      if (default_key != accelerator_key || default_mods != accelerator_mods)
        return FALSE;
    }

  return TRUE;
}

gboolean
gimp_action_is_default_accel (GimpAction  *action,
                              const gchar *accel)
{
  gchar           **default_accels;
  guint             accelerator_key  = 0;
  GdkModifierType   accelerator_mods = 0;

  g_return_val_if_fail (GIMP_IS_ACTION (action), TRUE);
  g_return_val_if_fail (accel != NULL, TRUE);

  gtk_accelerator_parse (accel, &accelerator_key, &accelerator_mods);
  g_return_val_if_fail (accelerator_key != 0 || accelerator_mods == 0, FALSE);

  default_accels = GET_PRIVATE (action)->default_accels;

  if (default_accels == NULL)
    return FALSE;

  for (gint i = 0; default_accels[i] != NULL; i++)
    {
      guint           default_key;
      GdkModifierType default_mods;

      gtk_accelerator_parse (default_accels[i], &default_key, &default_mods);

      if (default_key == accelerator_key && default_mods == accelerator_mods)
        return TRUE;
    }

  return FALSE;
}

const gchar *
gimp_action_get_menu_path (GimpAction *action)
{
  g_return_val_if_fail (GIMP_IS_ACTION (action), NULL);

  return GET_PRIVATE (action)->menu_path;
}

void
gimp_action_activate (GimpAction *action)
{
  g_return_if_fail (G_IS_ACTION (action));

  g_action_activate (G_ACTION (action), NULL);
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
  static const gchar *prefixes[] =
    {
      "windows-display-"
    };

  if (! (action_name && *action_name))
    return TRUE;

  for (guint i = 0; i < G_N_ELEMENTS (prefixes); i++)
    {
      if (g_str_has_prefix (action_name, prefixes[i]))
        return TRUE;
    }

  if (gimp_action_is_action_search_blacklisted (action_name))
    return TRUE;

  return FALSE;
}

gboolean
gimp_action_is_action_search_blacklisted (const gchar *action_name)
{
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
      "tools-threshold",
      "layers-mask-add-button"
    };

  gint i;

  if (! (action_name && *action_name))
    return TRUE;

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

GimpViewable *
gimp_action_get_viewable (GimpAction *action)
{
  g_return_val_if_fail (GIMP_IS_ACTION (action), NULL);

  return GET_PRIVATE (action)->viewable;
}

GimpContext *
gimp_action_get_context (GimpAction *action)
{
  g_return_val_if_fail (GIMP_IS_ACTION (action), NULL);

  return GET_PRIVATE (action)->context;
}


/* Protected functions. */

/**
 * gimp_action_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #GimpAction. Please call this function in the *_class_init()
 * function of the child class.
 **/
void
gimp_action_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass, GIMP_ACTION_PROP_CONTEXT,         "context");
  g_object_class_override_property (klass, GIMP_ACTION_PROP_SENSITIVE,       "sensitive");
  g_object_class_override_property (klass, GIMP_ACTION_PROP_VISIBLE,         "visible");

  g_object_class_override_property (klass, GIMP_ACTION_PROP_LABEL,           "label");
  g_object_class_override_property (klass, GIMP_ACTION_PROP_SHORT_LABEL,     "short-label");
  g_object_class_override_property (klass, GIMP_ACTION_PROP_TOOLTIP,         "tooltip");
  g_object_class_override_property (klass, GIMP_ACTION_PROP_ICON_NAME,       "icon-name");
  g_object_class_override_property (klass, GIMP_ACTION_PROP_ICON,            "icon");

  g_object_class_override_property (klass, GIMP_ACTION_PROP_COLOR,           "color");
  g_object_class_override_property (klass, GIMP_ACTION_PROP_VIEWABLE,        "viewable");

  g_object_class_override_property (klass, GIMP_ACTION_PROP_ELLIPSIZE,       "ellipsize");
  g_object_class_override_property (klass, GIMP_ACTION_PROP_MAX_WIDTH_CHARS, "max-width-chars");
}

void
gimp_action_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpActionPrivate *priv;

  priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case GIMP_ACTION_PROP_CONTEXT:
      g_value_set_object (value, priv->context);
      break;
    case GIMP_ACTION_PROP_SENSITIVE:
      g_value_set_boolean (value, priv->sensitive);
      break;
    case GIMP_ACTION_PROP_VISIBLE:
      g_value_set_boolean (value, priv->visible);
      break;

    case GIMP_ACTION_PROP_LABEL:
      g_value_set_string (value, priv->label);
      break;
    case GIMP_ACTION_PROP_SHORT_LABEL:
      g_value_set_string (value, priv->short_label ? priv->short_label : priv->label);
      break;
    case GIMP_ACTION_PROP_TOOLTIP:
      g_value_set_string (value, priv->tooltip);
      break;
    case GIMP_ACTION_PROP_ICON_NAME:
      g_value_set_string (value, priv->icon_name);
      break;
    case GIMP_ACTION_PROP_ICON:
      g_value_set_object (value, priv->icon);
      break;

    case GIMP_ACTION_PROP_COLOR:
      g_value_set_object (value, priv->color);
      break;
    case GIMP_ACTION_PROP_VIEWABLE:
      g_value_set_object (value, priv->viewable);
      break;
    case GIMP_ACTION_PROP_ELLIPSIZE:
      g_value_set_enum (value, priv->ellipsize);
      break;
    case GIMP_ACTION_PROP_MAX_WIDTH_CHARS:
      g_value_set_int (value, priv->max_width_chars);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_action_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpActionPrivate *priv;
  gboolean           set_proxy = FALSE;

  priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case GIMP_ACTION_PROP_CONTEXT:
      g_set_object (&priv->context, g_value_get_object (value));
      break;
    case GIMP_ACTION_PROP_SENSITIVE:
      gimp_action_set_sensitive (GIMP_ACTION (object),
                                 g_value_get_boolean (value),
                                 NULL);
      break;
    case GIMP_ACTION_PROP_VISIBLE:
      gimp_action_set_visible (GIMP_ACTION (object),
                               g_value_get_boolean (value));
      break;

    case GIMP_ACTION_PROP_LABEL:
      gimp_action_set_label (GIMP_ACTION (object), g_value_get_string (value));
      break;
    case GIMP_ACTION_PROP_SHORT_LABEL:
      gimp_action_set_short_label (GIMP_ACTION (object), g_value_get_string (value));
      break;
    case GIMP_ACTION_PROP_TOOLTIP:
      gimp_action_set_tooltip (GIMP_ACTION (object), g_value_get_string (value));
      break;
    case GIMP_ACTION_PROP_ICON_NAME:
      gimp_action_set_icon_name (GIMP_ACTION (object), g_value_get_string (value));
      break;
    case GIMP_ACTION_PROP_ICON:
      gimp_action_set_gicon (GIMP_ACTION (object), g_value_get_object (value));
      break;

    case GIMP_ACTION_PROP_COLOR:
      g_clear_object (&priv->color);
      if (g_value_get_object (value))
        priv->color = gegl_color_duplicate (g_value_get_object (value));

      set_proxy = TRUE;
      break;
    case GIMP_ACTION_PROP_VIEWABLE:
      g_set_object (&priv->viewable, g_value_get_object (value));
      set_proxy = TRUE;
      break;
    case GIMP_ACTION_PROP_ELLIPSIZE:
      priv->ellipsize = g_value_get_enum (value);
      set_proxy = TRUE;
      break;
    case GIMP_ACTION_PROP_MAX_WIDTH_CHARS:
      priv->max_width_chars = g_value_get_int (value);
      set_proxy = TRUE;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (set_proxy)
    {
      /* Set or update the proxy rendering. */
      for (GList *list = priv->proxies; list; list = list->next)
        gimp_action_set_proxy (GIMP_ACTION (object), list->data);
    }
}

void
gimp_action_set_proxy (GimpAction *action,
                       GtkWidget  *proxy)
{
  GimpActionPrivate *priv        = GET_PRIVATE (action);
  GtkWidget         *proxy_image = NULL;
  GdkPixbuf         *pixbuf      = NULL;
  GtkWidget         *label;

  if (! GTK_IS_MENU_ITEM (proxy)   &&
      ! GTK_IS_TOOL_BUTTON (proxy) &&
      ! GTK_IS_BUTTON (proxy))
    return;

  /* Current implementation accepts GtkButton as proxies but don't modify their
   * render. TODO?
   */
  if (! GTK_IS_BUTTON (proxy))
    {
      if (priv->color)
        {
          if (GTK_IS_MENU_ITEM (proxy))
            proxy_image = gimp_menu_item_get_image (GTK_MENU_ITEM (proxy));
          else
            proxy_image = gtk_tool_button_get_label_widget (GTK_TOOL_BUTTON (proxy));

          if (GIMP_IS_COLOR_AREA (proxy_image))
            {
              gimp_color_area_set_color (GIMP_COLOR_AREA (proxy_image), priv->color);
              proxy_image = NULL;
            }
          else
            {
              gint width, height;

              proxy_image = gimp_color_area_new (priv->color, GIMP_COLOR_AREA_SMALL_CHECKS, 0);
              gimp_color_area_set_draw_border (GIMP_COLOR_AREA (proxy_image), TRUE);

              if (priv->context)
                gimp_color_area_set_color_config (GIMP_COLOR_AREA (proxy_image),
                                                  priv->context->gimp->config->color_management);

              gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
              gtk_widget_set_size_request (proxy_image, width, height);
              gtk_widget_show (proxy_image);
            }
        }
      else if (priv->viewable)
        {
          if (GTK_IS_MENU_ITEM (proxy))
            proxy_image = gimp_menu_item_get_image (GTK_MENU_ITEM (proxy));
          else
            proxy_image = gtk_tool_button_get_label_widget (GTK_TOOL_BUTTON (proxy));

          if (GIMP_IS_VIEW (proxy_image) &&
              g_type_is_a (G_TYPE_FROM_INSTANCE (priv->viewable),
                           GIMP_VIEW (proxy_image)->renderer->viewable_type))
            {
              gimp_view_set_viewable (GIMP_VIEW (proxy_image), priv->viewable);
              proxy_image = NULL;
            }
          else
            {
              GtkIconSize size;
              gint        width, height;
              gint        border_width;

              if (GIMP_IS_IMAGEFILE (priv->viewable))
                {
                  size         = GTK_ICON_SIZE_LARGE_TOOLBAR;
                  border_width = 0;
                }
              else
                {
                  size         = GTK_ICON_SIZE_MENU;
                  border_width = 1;
                }

              gtk_icon_size_lookup (size, &width, &height);
              proxy_image = gimp_view_new_full (priv->context, priv->viewable,
                                                width, height, border_width,
                                                FALSE, FALSE, FALSE);
              gtk_widget_show (proxy_image);
            }
        }
      else
        {
          if (GIMP_IS_PROCEDURE_ACTION (action) &&
              /* Some special cases GimpProcedureAction have no procedure attached
               * (e.g. "filters-recent-*" actions.
               */
              G_IS_OBJECT (GIMP_PROCEDURE_ACTION (action)->procedure))
            {
              /* Special-casing procedure actions as plug-ins can create icons with
               * gimp_procedure_set_icon_pixbuf().
               */
              g_object_get (GIMP_PROCEDURE_ACTION (action)->procedure,
                            "icon-pixbuf", &pixbuf,
                            NULL);

              if (pixbuf != NULL)
                {
                  gint width;
                  gint height;

                  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);

                  if (width  != gdk_pixbuf_get_width  (pixbuf) ||
                      height != gdk_pixbuf_get_height (pixbuf))
                    {
                      GdkPixbuf *copy;

                      copy = gdk_pixbuf_scale_simple (pixbuf, width, height,
                                                      GDK_INTERP_BILINEAR);
                      g_object_unref (pixbuf);
                      pixbuf = copy;
                    }

                  proxy_image = gtk_image_new_from_pixbuf (pixbuf);
                }
            }

          if (proxy_image == NULL)
            {
              if (GTK_IS_MENU_ITEM (proxy))
                proxy_image = gimp_menu_item_get_image (GTK_MENU_ITEM (proxy));
              else
                proxy_image = gtk_tool_button_get_label_widget (GTK_TOOL_BUTTON (proxy));

              if (GIMP_IS_VIEW (proxy_image) || GIMP_IS_COLOR_AREA (proxy_image))
                {
                  if (GTK_IS_MENU_ITEM (proxy))
                    gimp_menu_item_set_image (GTK_MENU_ITEM (proxy), NULL, action);
                  else if (proxy_image)
                    gtk_widget_destroy (proxy_image);

                  g_object_notify (G_OBJECT (action), "icon-name");
                }

              proxy_image = NULL;
            }
        }
    }

  if (proxy_image != NULL)
    {
      if (GTK_IS_MENU_ITEM (proxy))
        {
          gimp_menu_item_set_image (GTK_MENU_ITEM (proxy), proxy_image, action);
        }
      else /* GTK_IS_TOOL_BUTTON (proxy) */
        {
          GtkWidget *prev_widget;

          prev_widget = gtk_tool_button_get_label_widget (GTK_TOOL_BUTTON (proxy));
          if (prev_widget)
            gtk_widget_destroy (prev_widget);

          gtk_tool_button_set_label_widget (GTK_TOOL_BUTTON (proxy), proxy_image);
        }
    }
  else if (GTK_IS_LABEL (gtk_bin_get_child (GTK_BIN (proxy))) &&
           GTK_IS_MENU_ITEM (proxy))
    {
      /* Ensure we rebuild the contents of the GtkMenuItem with an image (which
       * might be NULL), a label (taken from action) and an optional shortcut
       * (also taken from action).
       */
      gimp_menu_item_set_image (GTK_MENU_ITEM (proxy), NULL, action);
    }

  label = g_object_get_data (G_OBJECT (proxy), "gimp-menu-item-label");
  if (label)
    {
      gtk_label_set_ellipsize (GTK_LABEL (label), priv->ellipsize);
      gtk_label_set_max_width_chars (GTK_LABEL (label), priv->max_width_chars);
    }

  if (! g_list_find (priv->proxies, proxy))
    {
      priv->proxies = g_list_prepend (priv->proxies, proxy);
      g_signal_connect (proxy, "destroy",
                        (GCallback) gimp_action_proxy_destroy,
                        action);

      if (GTK_IS_BUTTON (proxy))
        g_signal_connect (proxy, "clicked",
                          (GCallback) gimp_action_proxy_button_activate,
                          action);

      gimp_action_update_proxy_sensitive (action, proxy);
    }

  g_clear_object (&pixbuf);
}


/*  Friend functions  */

/* This function is only meant to be run by the GimpActionGroup class. */
void
gimp_action_set_group (GimpAction      *action,
                       GimpActionGroup *group)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  /* We can't change groups! */
  g_return_if_fail (priv->group == NULL);

  priv->group = group;
}

/* This function is only meant to be run by the GimpActionGroup class. */
void
gimp_action_set_default_accels (GimpAction   *action,
                                const gchar **accels)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  g_return_if_fail (GIMP_IS_ACTION (action));
  /* This should be set only once and before any accelerator was set. */
  g_return_if_fail (priv->accels == NULL);
  g_return_if_fail (priv->default_accels == NULL);

  priv->default_accels = g_strdupv ((gchar **) accels);
  priv->accels         = g_strdupv ((gchar **) accels);

  g_signal_emit (action, action_signals[ACCELS_CHANGED], 0, accels);
}

/* This function is only meant to be run by the GimpMenuModel class. */
void
gimp_action_set_menu_path (GimpAction  *action,
                           const gchar *menu_path)
{
  GimpActionPrivate  *priv;
  gchar             **paths;

  g_return_if_fail (GIMP_IS_ACTION (action));

  priv = GET_PRIVATE (action);

  if (priv->menu_path != NULL)
    /* There are cases where we put some actions in 2 menu paths, for instance:
     * - filters-color-to-alpha in both /Layer/Transparency and /Colors
     * - dialogs-histogram in both /Colors/Info and /Windows/Dockable Dialogs
     *
     * Anyway this is not an error, it's just how it is. Let's simply skip such
     * cases silently and keep the first path as reference to show in helper
     * widgets.
     */
    return;

  if (menu_path)
    {
      paths = gimp_utils_break_menu_path (menu_path, NULL, NULL);

      /* MacOS does not support the "rightwards triangle arrowhead" symbol,
       * so we'll use the MacOS submenu separator symbol per Lukas Oberhuber. */
#ifdef PLATFORM_OSX
      priv->menu_path = g_strjoinv (" > ", paths);
#else
      /* The 4 raw bytes are the "rightwards triangle arrowhead" unicode character. */
      priv->menu_path = g_strjoinv (" \xF0\x9F\xA2\x92 ", paths);
#endif
      g_strfreev (paths);
    }
}


/*  Private functions  */

static GimpActionPrivate *
gimp_action_get_private (GimpAction *action)
{
  GimpActionPrivate *priv;
  static GQuark      private_key = 0;

  g_return_val_if_fail (GIMP_IS_ACTION (action), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("gimp-action-priv");

  priv = g_object_get_qdata ((GObject *) action, private_key);

  if (! priv)
    {
      priv = g_slice_new0 (GimpActionPrivate);

      g_object_set_qdata_full ((GObject *) action, private_key, priv,
                               (GDestroyNotify) gimp_action_private_finalize);
    }

  return priv;
}

static void
gimp_action_private_finalize (GimpActionPrivate *priv)
{
  g_clear_pointer (&priv->disable_reason, g_free);
  g_clear_object  (&priv->context);
  g_clear_object  (&priv->color);
  g_clear_object  (&priv->viewable);

  g_free (priv->label);
  g_free (priv->short_label);
  g_free (priv->tooltip);
  g_free (priv->icon_name);
  g_clear_object (&priv->icon);

  g_strfreev (priv->accels);
  g_strfreev (priv->default_accels);

  g_free (priv->menu_path);

  for (GList *iter = priv->proxies; iter; iter = iter->next)
    {
      /* TODO GAction: if an action associated to a proxy menu item disappears,
       * shouldn't we also destroy the item itself (not just disconnect it)? It
       * would now point to a non-existing action.
       */
      g_signal_handlers_disconnect_by_func (iter->data,
                                            gimp_action_proxy_destroy,
                                            priv->action);
      g_signal_handlers_disconnect_by_func (iter->data,
                                            gimp_action_proxy_button_activate,
                                            priv->action);
    }
  g_list_free (priv->proxies);
  priv->proxies = NULL;

  g_slice_free (GimpActionPrivate, priv);
}

static void
gimp_action_set_proxy_label (GimpAction *action,
                             GtkWidget  *proxy)
{
  if (GTK_IS_MENU_ITEM (proxy))
    {
      GtkWidget   *child = gtk_bin_get_child (GTK_BIN (proxy));
      const gchar *label;

      /* For menus, we assume that their position ensure some context, hence the
       * short label is chosen in priority.
       */
      label = gimp_action_get_short_label (action);

      if (GTK_IS_BOX (child))
        {
          child = g_object_get_data (G_OBJECT (proxy),
                                     "gimp-menu-item-label");

          if (GTK_IS_LABEL (child))
            gtk_label_set_text_with_mnemonic (GTK_LABEL (child), label);
        }
      else if (GTK_IS_LABEL (child))
        {
          gtk_menu_item_set_label (GTK_MENU_ITEM (proxy), label);
        }
    }
}

static void
gimp_action_label_notify (GimpAction       *action,
                          const GParamSpec *pspec,
                          gpointer          data)
{
  for (GList *iter = GET_PRIVATE (action)->proxies; iter; iter = iter->next)
    gimp_action_set_proxy_label (action, iter->data);
}

static void
gimp_action_proxy_destroy (GtkWidget  *proxy,
                           GimpAction *action)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  priv->proxies = g_list_remove (priv->proxies, proxy);
}

static void
gimp_action_proxy_button_activate (GtkButton  *button,
                                   GimpAction *action)
{
  /* Activate with the default parameter value. */
  gimp_action_activate (action);
}

static void
gimp_action_update_proxy_sensitive (GimpAction *action,
                                    GtkWidget  *proxy)
{
  GimpActionPrivate *priv      = GET_PRIVATE (action);
  gboolean           sensitive = gimp_action_is_sensitive (action, NULL);

  if (proxy)
    {
      gtk_widget_set_sensitive (proxy, sensitive);
      gimp_action_update_proxy_tooltip (action, proxy);
    }
  else
    {
      for (GList *list = priv->proxies; list; list = list->next)
        gtk_widget_set_sensitive (list->data, sensitive);

      gimp_action_update_proxy_tooltip (action, NULL);
    }
}

static void
gimp_action_update_proxy_visible (GimpAction *action,
                                  GtkWidget  *proxy)
{
  GimpActionPrivate *priv    = GET_PRIVATE (action);
  gboolean           visible = gimp_action_is_visible (action);

  if (proxy)
    {
      gtk_widget_set_visible (proxy, visible);
    }
  else
    {
      for (GList *list = priv->proxies; list; list = list->next)
        gtk_widget_set_visible (list->data, visible);
    }
}

static void
gimp_action_update_proxy_tooltip (GimpAction *action,
                                  GtkWidget  *proxy)
{
  GimpActionPrivate *priv;
  const gchar       *tooltip;
  const gchar       *help_id;
  const gchar       *reason          = NULL;
  gchar             *escaped_tooltip = NULL;
  gchar             *escaped_reason  = NULL;
  gchar             *markup          = NULL;

  g_return_if_fail (GIMP_IS_ACTION (action));

  priv    = GET_PRIVATE (action);
  tooltip = gimp_action_get_tooltip (action);
  help_id = gimp_action_get_help_id (action);

  gimp_action_get_sensitive (action, &reason);
  escaped_reason  = reason ? g_markup_escape_text (reason, -1) : NULL;
  escaped_tooltip = tooltip ? g_markup_escape_text (tooltip, -1) : NULL;

  if (escaped_tooltip || escaped_reason)
    markup = g_strdup_printf ("%s%s"                                   /* Action tooltip  */
                              "<i><span weight='light'>%s</span></i>", /* Inactive reason */
                              escaped_tooltip ? escaped_tooltip : "",
                              escaped_tooltip && escaped_reason ? "\n" : "",
                              escaped_reason ? escaped_reason : "");

  /*  This hack makes sure we don't replace the tooltips of GimpButtons
   *  with extended callbacks (for Shift+Click etc.), because these
   *  buttons already have customly constructed multi-line tooltips
   *  which we want to keep.
   */
#define HAS_EXTENDED_ACTIONS(widget) \
  (g_object_get_data (G_OBJECT (widget), "extended-actions") != NULL)

  if (proxy != NULL)
    {
      if (! HAS_EXTENDED_ACTIONS (proxy))
        gimp_help_set_help_data_with_markup (proxy, markup, help_id);
    }
  else
    {
      for (GList *list = priv->proxies; list; list = list->next)
        if (! HAS_EXTENDED_ACTIONS (list->data))
          gimp_help_set_help_data_with_markup (list->data, markup, help_id);
    }

  g_free (escaped_tooltip);
  g_free (escaped_reason);
  g_free (markup);
}
