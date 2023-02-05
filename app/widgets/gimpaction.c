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
#include "gimpview.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"


enum
{
  ACTIVATE,
  CHANGE_STATE,
  LAST_SIGNAL
};

#define GET_PRIVATE(obj) (gimp_action_get_private ((GimpAction *) (obj)))

typedef struct _GimpActionPrivate GimpActionPrivate;

struct _GimpActionPrivate
{
  GimpContext           *context;

  gchar                 *disable_reason;

  GimpRGB               *color;
  GimpViewable          *viewable;
  PangoEllipsizeMode     ellipsize;
  gint                   max_width_chars;
};


static GimpActionPrivate *
              gimp_action_get_private       (GimpAction        *action);
static void   gimp_action_private_finalize  (GimpActionPrivate *priv);

static void   gimp_action_label_notify      (GimpAction        *action,
                                             const GParamSpec  *pspec,
                                             gpointer           data);
static void   gimp_action_tooltip_notify    (GimpAction        *action,
                                             const GParamSpec  *pspec,
                                             gpointer           data);


G_DEFINE_INTERFACE (GimpAction, gimp_action, GTK_TYPE_ACTION)

static guint action_signals[LAST_SIGNAL];


static void
gimp_action_default_init (GimpActionInterface *iface)
{
  GimpRGB black;

  action_signals[ACTIVATE] =
    g_signal_new ("gimp-activate",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpActionInterface, activate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_VARIANT);

  action_signals[CHANGE_STATE] =
    g_signal_new ("gimp-change-state",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpActionInterface, change_state),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_VARIANT);

  g_object_interface_install_property (iface,
                                       g_param_spec_object ("context",
                                                            NULL, NULL,
                                                            GIMP_TYPE_CONTEXT,
                                                            GIMP_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT_ONLY));
  gimp_rgba_set (&black, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
  g_object_interface_install_property (iface,
                                       gimp_param_spec_rgb ("color",
                                                            NULL, NULL,
                                                            TRUE, &black,
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

  priv->ellipsize       = PANGO_ELLIPSIZE_NONE;
  priv->max_width_chars = -1;

  g_signal_connect (action, "notify::label",
                    G_CALLBACK (gimp_action_label_notify),
                    NULL);
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
gimp_action_set_proxy_tooltip (GimpAction *action,
                               GtkWidget  *proxy)
{
  const gchar *tooltip;
  const gchar *reason         = NULL;
  gchar       *escaped_reason = NULL;
  gchar       *markup;

  g_return_if_fail (GIMP_IS_ACTION (action));
  g_return_if_fail (GTK_IS_WIDGET (proxy));

  tooltip = gimp_action_get_tooltip (action);

  gimp_action_get_sensitive (action, &reason);
  if (reason)
    escaped_reason = g_markup_escape_text (reason, -1);

  markup = g_strdup_printf ("%s%s"                                   /* Action tooltip  */
                            "<i><span weight='light'>%s</span></i>", /* Inactive reason */
                            tooltip,
                            escaped_reason && tooltip ? "\n" : "",
                            escaped_reason ? escaped_reason : "");

  if (tooltip || escaped_reason)
    gimp_help_set_help_data_with_markup (proxy, markup,
                                         g_object_get_qdata (G_OBJECT (proxy),
                                                             GIMP_HELP_ID));

  g_free (escaped_reason);
  g_free (markup);
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
gimp_action_set_sensitive (GimpAction  *action,
                           gboolean     sensitive,
                           const gchar *reason)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  gtk_action_set_sensitive ((GtkAction *) action, sensitive);

  g_clear_pointer (&priv->disable_reason, g_free);
  if (reason && ! sensitive)
    priv->disable_reason = g_strdup (reason);
}

gboolean
gimp_action_get_sensitive (GimpAction   *action,
                           const gchar **reason)
{
  gboolean sensitive;

  sensitive = gtk_action_get_sensitive ((GtkAction *) action);

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

  sensitive = gtk_action_is_sensitive ((GtkAction *) action);

  if (reason)
    {
      GimpActionPrivate *priv = GET_PRIVATE (action);

      *reason = NULL;

      if (! sensitive && priv->disable_reason != NULL)
        *reason = (const gchar *) priv->disable_reason;
    }

  return sensitive;
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

void
gimp_action_set_accels (GimpAction   *action,
                        const gchar **accels)
{
  GimpContext *context;
  gchar       *detailed_action_name;

  g_return_if_fail (GIMP_IS_ACTION (action));

  context = GET_PRIVATE (action)->context;
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (GTK_IS_APPLICATION (context->gimp->app));

  detailed_action_name = g_strdup_printf ("app.%s",
                                          g_action_get_name (G_ACTION (action)));
  gtk_application_set_accels_for_action (GTK_APPLICATION (context->gimp->app),
                                         detailed_action_name,
                                         accels);
  g_free (detailed_action_name);
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
gchar **
gimp_action_get_accels (GimpAction *action)
{
  gchar       **accels;
  GimpContext  *context;
  gchar        *detailed_action_name;

  g_return_val_if_fail (GIMP_IS_ACTION (action), NULL);

  context = GET_PRIVATE (action)->context;
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_APPLICATION (context->gimp->app), NULL);

  detailed_action_name = g_strdup_printf ("app.%s",
                                          g_action_get_name (G_ACTION (action)));
  accels = gtk_application_get_accels_for_action (GTK_APPLICATION (context->gimp->app),
                                                  detailed_action_name);
  g_free (detailed_action_name);

  return accels;
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
      "tools-threshold",
      "layers-mask-add-button"
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
  g_object_class_override_property (klass, GIMP_ACTION_PROP_COLOR,           "color");
  g_object_class_override_property (klass, GIMP_ACTION_PROP_VIEWABLE,        "viewable");

  g_object_class_override_property (klass, GIMP_ACTION_PROP_ELLIPSIZE,       "ellipsize");
  g_object_class_override_property (klass, GIMP_ACTION_PROP_MAX_WIDTH_CHARS, "max-width-chars");
}

void
gimp_action_constructed (GObject *object)
{
  GimpActionPrivate *priv = GET_PRIVATE (object);

  g_return_if_fail (priv->context != NULL);
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
    case GIMP_ACTION_PROP_COLOR:
      g_value_set_boxed (value, priv->color);
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
    case GIMP_ACTION_PROP_COLOR:
      g_clear_pointer (&priv->color, g_free);
      priv->color = g_value_dup_boxed (value);
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
      GSList *list;

      for (list = gimp_action_get_proxies (GIMP_ACTION (object));
           list;
           list = g_slist_next (list))
        {
          gimp_action_set_proxy (GIMP_ACTION (object), list->data);
        }
    }
}

void
gimp_action_set_proxy (GimpAction *action,
                       GtkWidget  *proxy)
{
  GimpActionPrivate *priv = GET_PRIVATE (action);

  if (! GTK_IS_MENU_ITEM (proxy))
    return;

  if (priv->color)
    {
      GtkWidget *area;

      area = gimp_menu_item_get_image (GTK_MENU_ITEM (proxy));

      if (GIMP_IS_COLOR_AREA (area))
        {
          gimp_color_area_set_color (GIMP_COLOR_AREA (area), priv->color);
        }
      else
        {
          gint width, height;

          area = gimp_color_area_new (priv->color,
                                      GIMP_COLOR_AREA_SMALL_CHECKS, 0);
          gimp_color_area_set_draw_border (GIMP_COLOR_AREA (area), TRUE);

          if (priv->context)
            gimp_color_area_set_color_config (GIMP_COLOR_AREA (area),
                                              priv->context->gimp->config->color_management);

          gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
          gtk_widget_set_size_request (area, width, height);
          gimp_menu_item_set_image (GTK_MENU_ITEM (proxy), area);
          gtk_widget_show (area);
        }
    }
  else if (priv->viewable)
    {
      GtkWidget *view;

      view = gimp_menu_item_get_image (GTK_MENU_ITEM (proxy));

      if (GIMP_IS_VIEW (view) &&
          g_type_is_a (G_TYPE_FROM_INSTANCE (priv->viewable),
                       GIMP_VIEW (view)->renderer->viewable_type))
        {
          gimp_view_set_viewable (GIMP_VIEW (view), priv->viewable);
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
          view = gimp_view_new_full (priv->context, priv->viewable,
                                     width, height, border_width,
                                     FALSE, FALSE, FALSE);
          gimp_menu_item_set_image (GTK_MENU_ITEM (proxy), view);
          gtk_widget_show (view);
        }
    }
  else
    {
      GtkWidget *image;

      image = gimp_menu_item_get_image (GTK_MENU_ITEM (proxy));

      if (GIMP_IS_VIEW (image) || GIMP_IS_COLOR_AREA (image))
        {
          gimp_menu_item_set_image (GTK_MENU_ITEM (proxy), NULL);
          g_object_notify (G_OBJECT (action), "icon-name");
        }
    }

  {
    GtkWidget *child = gtk_bin_get_child (GTK_BIN (proxy));

    if (GTK_IS_BOX (child))
      child = g_object_get_data (G_OBJECT (proxy), "gimp-menu-item-label");

    if (GTK_IS_LABEL (child))
      {
        GtkLabel *label = GTK_LABEL (child);

        gtk_label_set_ellipsize (label, priv->ellipsize);
        gtk_label_set_max_width_chars (label, priv->max_width_chars);
      }
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
  g_clear_pointer (&priv->color, g_free);
  g_clear_object  (&priv->viewable);

  g_slice_free (GimpActionPrivate, priv);
}

static void
gimp_action_label_notify (GimpAction       *action,
                          const GParamSpec *pspec,
                          gpointer          data)
{
  GSList *list;

  for (list = gimp_action_get_proxies (action);
       list;
       list = g_slist_next (list))
    {
      if (GTK_IS_MENU_ITEM (list->data))
        {
          GtkWidget *child = gtk_bin_get_child (GTK_BIN (list->data));

          if (GTK_IS_BOX (child))
            {
              child = g_object_get_data (G_OBJECT (list->data),
                                         "gimp-menu-item-label");

              if (GTK_IS_LABEL (child))
                gtk_label_set_text (GTK_LABEL (child),
                                    gimp_action_get_label (action));
            }
        }
    }
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
