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
#include "gimpactionimpl.h"
#include "gimpaction-history.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_COLOR,
  PROP_VIEWABLE,
  PROP_ELLIPSIZE,
  PROP_MAX_WIDTH_CHARS,

  PROP_ENABLED,
  PROP_PARAMETER_TYPE,
  PROP_STATE_TYPE,
  PROP_STATE
};

enum
{
  CHANGE_STATE,
  LAST_SIGNAL
};

struct _GimpActionImplPrivate
{
  GVariantType *parameter_type;
  GVariant     *state;
  GVariant     *state_hint;
  gboolean      state_set_already;
};

static void   gimp_action_g_action_iface_init     (GActionInterface    *iface);

static void   gimp_action_impl_finalize           (GObject             *object);
static void   gimp_action_impl_set_property       (GObject             *object,
                                                   guint                prop_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void   gimp_action_impl_get_property       (GObject             *object,
                                                   guint                prop_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

/* XXX Implementations for our GimpAction are widely inspired by GSimpleAction
 * implementations.
 */
static void   gimp_action_impl_g_activate         (GAction             *action,
                                                   GVariant            *parameter);
static void   gimp_action_impl_change_state       (GAction             *action,
                                                   GVariant            *value);
static gboolean gimp_action_impl_get_enabled      (GAction             *action);
static const GVariantType *
              gimp_action_impl_get_parameter_type (GAction             *action);
static const GVariantType *
              gimp_action_impl_get_state_type     (GAction             *action);
static GVariant *
              gimp_action_impl_get_state          (GAction             *action);
static GVariant *
              gimp_action_impl_get_state_hint     (GAction             *action);

static void   gimp_action_impl_activate           (GtkAction           *action);
static void   gimp_action_impl_connect_proxy      (GtkAction           *action,
                                                   GtkWidget           *proxy);

static void   gimp_action_impl_set_proxy          (GimpActionImpl      *impl,
                                                   GtkWidget           *proxy);

static void   gimp_action_impl_set_state          (GimpAction          *gimp_action,
                                                   GVariant            *value);


G_DEFINE_TYPE_WITH_CODE (GimpActionImpl, gimp_action_impl, GTK_TYPE_ACTION,
                         G_ADD_PRIVATE (GimpActionImpl)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, gimp_action_g_action_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_ACTION, NULL))

#define parent_class gimp_action_impl_parent_class

static guint gimp_action_impl_signals[LAST_SIGNAL] = { 0 };

static void
gimp_action_impl_class_init (GimpActionImplClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkActionClass *action_class = GTK_ACTION_CLASS (klass);
  GimpRGB         black;

  gimp_action_impl_signals[CHANGE_STATE] =
    g_signal_new ("change-state",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_MUST_COLLECT,
                  G_STRUCT_OFFSET (GimpActionImplClass, change_state),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_VARIANT);

  object_class->finalize      = gimp_action_impl_finalize;
  object_class->set_property  = gimp_action_impl_set_property;
  object_class->get_property  = gimp_action_impl_get_property;

  action_class->activate      = gimp_action_impl_activate;
  action_class->connect_proxy = gimp_action_impl_connect_proxy;

  gimp_rgba_set (&black, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_COLOR,
                                   gimp_param_spec_rgb ("color",
                                                        NULL, NULL,
                                                        TRUE, &black,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_VIEWABLE,
                                   g_param_spec_object ("viewable",
                                                        NULL, NULL,
                                                        GIMP_TYPE_VIEWABLE,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      NULL, NULL,
                                                      PANGO_TYPE_ELLIPSIZE_MODE,
                                                      PANGO_ELLIPSIZE_NONE,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_MAX_WIDTH_CHARS,
                                   g_param_spec_int ("max-width-chars",
                                                     NULL, NULL,
                                                     -1, G_MAXINT, -1,
                                                     GIMP_PARAM_READWRITE));


  /**
   * GimpAction:enabled:
   *
   * If @action is currently enabled.
   *
   * If the action is disabled then calls to g_action_activate() and
   * g_action_change_state() have no effect.
   **/
  g_object_class_install_property (object_class, PROP_ENABLED,
                                   g_param_spec_boolean ("enabled",
                                                         "Enabled",
                                                         "If the action can be activated",
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE));
  /**
   * GimpAction:parameter-type:
   *
   * The type of the parameter that must be given when activating the
   * action.
   **/
  g_object_class_install_property (object_class, PROP_PARAMETER_TYPE,
                                   g_param_spec_boxed ("parameter-type",
                                                       "Parameter Type",
                                                       "The type of GVariant passed to activate()",
                                                       G_TYPE_VARIANT_TYPE,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
  /**
   * GimpAction:state-type:
   *
   * The #GVariantType of the state that the action has, or %NULL if the
   * action is stateless.
   **/
  g_object_class_install_property (object_class, PROP_STATE_TYPE,
                                   g_param_spec_boxed ("state-type",
                                                       "State Type",
                                                       "The type of the state kept by the action",
                                                       G_TYPE_VARIANT_TYPE,
                                                       GIMP_PARAM_READABLE));

  /**
   * GimpAction:state:
   *
   * The state of the action, or %NULL if the action is stateless.
   **/
  g_object_class_install_property (object_class, PROP_STATE,
                                   g_param_spec_variant ("state",
                                                         "State",
                                                         "The state the action is in",
                                                         G_VARIANT_TYPE_ANY,
                                                         NULL,
                                                         GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
gimp_action_g_action_iface_init (GActionInterface *iface)
{
  iface->activate           = gimp_action_impl_g_activate;
  iface->change_state       = gimp_action_impl_change_state;
  iface->get_enabled        = gimp_action_impl_get_enabled;
  iface->get_name           = (const gchar* (*) (GAction*)) gimp_action_get_name;
  iface->get_parameter_type = gimp_action_impl_get_parameter_type;
  iface->get_state_type     = gimp_action_impl_get_state_type;
  iface->get_state          = gimp_action_impl_get_state;
  iface->get_state_hint     = gimp_action_impl_get_state_hint;
}

static void
gimp_action_impl_init (GimpActionImpl *impl)
{
  impl->priv                    = gimp_action_impl_get_instance_private (impl);
  impl->priv->state_set_already = FALSE;

  impl->ellipsize       = PANGO_ELLIPSIZE_NONE;
  impl->max_width_chars = -1;

  gimp_action_init (GIMP_ACTION (impl));
}

static void
gimp_action_impl_finalize (GObject *object)
{
  GimpActionImpl *impl = GIMP_ACTION_IMPL (object);

  g_clear_object  (&impl->context);
  g_clear_pointer (&impl->color, g_free);
  g_clear_object  (&impl->viewable);

  if (impl->priv->parameter_type)
    g_variant_type_free (impl->priv->parameter_type);
  if (impl->priv->state)
    g_variant_unref (impl->priv->state);
  if (impl->priv->state_hint)
    g_variant_unref (impl->priv->state_hint);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_action_impl_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpActionImpl *impl = GIMP_ACTION_IMPL (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, impl->context);
      break;

    case PROP_COLOR:
      g_value_set_boxed (value, impl->color);
      break;

    case PROP_VIEWABLE:
      g_value_set_object (value, impl->viewable);
      break;

    case PROP_ELLIPSIZE:
      g_value_set_enum (value, impl->ellipsize);
      break;

    case PROP_MAX_WIDTH_CHARS:
      g_value_set_int (value, impl->max_width_chars);
      break;

    case PROP_ENABLED:
      g_value_set_boolean (value, gimp_action_impl_get_enabled (G_ACTION (impl)));
      break;
    case PROP_PARAMETER_TYPE:
      g_value_set_boxed (value, gimp_action_impl_get_parameter_type (G_ACTION (impl)));
      break;
    case PROP_STATE_TYPE:
      g_value_set_boxed (value, gimp_action_impl_get_state_type (G_ACTION (impl)));
      break;
    case PROP_STATE:
      g_value_take_variant (value, gimp_action_impl_get_state (G_ACTION (impl)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_action_impl_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpActionImpl *impl      = GIMP_ACTION_IMPL (object);
  gboolean        set_proxy = FALSE;

  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_set_object (&impl->context, g_value_get_object (value));
      break;

    case PROP_COLOR:
      g_clear_pointer (&impl->color, g_free);
      impl->color = g_value_dup_boxed (value);
      set_proxy = TRUE;
      break;

    case PROP_VIEWABLE:
      g_set_object  (&impl->viewable, g_value_get_object (value));
      set_proxy = TRUE;
      break;

    case PROP_ELLIPSIZE:
      impl->ellipsize = g_value_get_enum (value);
      set_proxy = TRUE;
      break;

    case PROP_MAX_WIDTH_CHARS:
      impl->max_width_chars = g_value_get_int (value);
      set_proxy = TRUE;
      break;

    case PROP_ENABLED:
      gimp_action_set_sensitive (GIMP_ACTION (impl),
                                 g_value_get_boolean (value), NULL);
      break;
    case PROP_PARAMETER_TYPE:
      impl->priv->parameter_type = g_value_dup_boxed (value);
      break;
    case PROP_STATE:
      /* The first time we see this (during construct) we should just
       * take the state as it was handed to us.
       *
       * After that, we should make sure we go through the same checks
       * as the C API.
       */
      if (!impl->priv->state_set_already)
        {
          impl->priv->state             = g_value_dup_variant (value);
          impl->priv->state_set_already = TRUE;
        }
      else
        {
          gimp_action_impl_set_state (GIMP_ACTION (impl), g_value_get_variant (value));
        }

      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

  if (set_proxy)
    {
      GSList *list;

      for (list = gimp_action_get_proxies (GIMP_ACTION (impl));
           list;
           list = g_slist_next (list))
        {
          gimp_action_impl_set_proxy (impl, list->data);
        }
    }
}

static void
gimp_action_impl_g_activate (GAction  *action,
                             GVariant *parameter)
{
  gimp_action_emit_activate (GIMP_ACTION (action), parameter);

  gimp_action_history_action_activated (GIMP_ACTION (action));
}

static void
gimp_action_impl_change_state (GAction  *action,
                               GVariant *value)
{
  /* If the user connected a signal handler then they are responsible
   * for handling state changes.
   */
  if (g_signal_has_handler_pending (action, gimp_action_impl_signals[CHANGE_STATE], 0, TRUE))
    g_signal_emit (action, gimp_action_impl_signals[CHANGE_STATE], 0, value);
  else
    /* If not, then the default behaviour is to just set the state. */
    gimp_action_impl_set_state (GIMP_ACTION (action), value);
}

static gboolean
gimp_action_impl_get_enabled (GAction *action)
{
  return gimp_action_is_sensitive (GIMP_ACTION (action), NULL);
}

static const GVariantType *
gimp_action_impl_get_parameter_type (GAction *action)
{
  GimpActionImpl *impl = GIMP_ACTION_IMPL (action);

  return impl->priv->parameter_type;
}

static const GVariantType *
gimp_action_impl_get_state_type (GAction *action)
{
  GimpActionImpl *impl = GIMP_ACTION_IMPL (action);

  if (impl->priv->state != NULL)
    return g_variant_get_type (impl->priv->state);
  else
    return NULL;
}

static GVariant *
gimp_action_impl_get_state (GAction *action)
{
  GimpActionImpl *impl = GIMP_ACTION_IMPL (action);

  return impl->priv->state ? g_variant_ref (impl->priv->state) : NULL;
}

static GVariant *
gimp_action_impl_get_state_hint (GAction *action)
{
  GimpActionImpl *impl = GIMP_ACTION_IMPL (action);

  if (impl->priv->state_hint != NULL)
    return g_variant_ref (impl->priv->state_hint);
  else
    return NULL;
}

static void
gimp_action_impl_activate (GtkAction *action)
{
  if (GTK_ACTION_CLASS (parent_class)->activate)
    GTK_ACTION_CLASS (parent_class)->activate (action);

  gimp_action_emit_activate (GIMP_ACTION (action), NULL);

  gimp_action_history_action_activated (GIMP_ACTION (action));
}

static void
gimp_action_impl_connect_proxy (GtkAction *action,
                                GtkWidget *proxy)
{
  GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);

  gimp_action_impl_set_proxy (GIMP_ACTION_IMPL (action), proxy);

  gimp_action_set_proxy (GIMP_ACTION (action), proxy);
}


/*  public functions  */

GimpAction *
gimp_action_impl_new (const gchar *name,
                      const gchar *label,
                      const gchar *tooltip,
                      const gchar *icon_name,
                      const gchar *help_id)
{
  GimpAction *action;

  action = g_object_new (GIMP_TYPE_ACTION_IMPL,
                         "name",      name,
                         "label",     label,
                         "tooltip",   tooltip,
                         "icon-name", icon_name,
                         NULL);

  gimp_action_set_help_id (action, help_id);

  return action;
}


/*  private functions  */

static void
gimp_action_impl_set_proxy (GimpActionImpl *impl,
                            GtkWidget      *proxy)
{
  if (! GTK_IS_MENU_ITEM (proxy))
    return;

  if (impl->color)
    {
      GtkWidget *area;

      area = gimp_menu_item_get_image (GTK_MENU_ITEM (proxy));

      if (GIMP_IS_COLOR_AREA (area))
        {
          gimp_color_area_set_color (GIMP_COLOR_AREA (area), impl->color);
        }
      else
        {
          gint width, height;

          area = gimp_color_area_new (impl->color,
                                      GIMP_COLOR_AREA_SMALL_CHECKS, 0);
          gimp_color_area_set_draw_border (GIMP_COLOR_AREA (area), TRUE);

          if (impl->context)
            gimp_color_area_set_color_config (GIMP_COLOR_AREA (area),
                                              impl->context->gimp->config->color_management);

          gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
          gtk_widget_set_size_request (area, width, height);
          gimp_menu_item_set_image (GTK_MENU_ITEM (proxy), area);
          gtk_widget_show (area);
        }
    }
  else if (impl->viewable)
    {
      GtkWidget *view;

      view = gimp_menu_item_get_image (GTK_MENU_ITEM (proxy));

      if (GIMP_IS_VIEW (view) &&
          g_type_is_a (G_TYPE_FROM_INSTANCE (impl->viewable),
                       GIMP_VIEW (view)->renderer->viewable_type))
        {
          gimp_view_set_viewable (GIMP_VIEW (view), impl->viewable);
        }
      else
        {
          GtkIconSize size;
          gint        width, height;
          gint        border_width;

          if (GIMP_IS_IMAGEFILE (impl->viewable))
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
          view = gimp_view_new_full (impl->context, impl->viewable,
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
          g_object_notify (G_OBJECT (impl), "icon-name");
        }
    }

  {
    GtkWidget *child = gtk_bin_get_child (GTK_BIN (proxy));

    if (GTK_IS_BOX (child))
      child = g_object_get_data (G_OBJECT (proxy), "gimp-menu-item-label");

    if (GTK_IS_LABEL (child))
      {
        GtkLabel *label = GTK_LABEL (child);

        gtk_label_set_ellipsize (label, impl->ellipsize);
        gtk_label_set_max_width_chars (label, impl->max_width_chars);
      }
  }
}

static void
gimp_action_impl_set_state (GimpAction *gimp_action,
                            GVariant   *value)
{
  GimpActionImpl     *impl;
  const GVariantType *state_type;

  g_return_if_fail (GIMP_IS_ACTION (gimp_action));
  g_return_if_fail (value != NULL);

  impl = GIMP_ACTION_IMPL (gimp_action);

  state_type = impl->priv->state ? g_variant_get_type (impl->priv->state) : NULL;

  g_return_if_fail (state_type != NULL);
  g_return_if_fail (g_variant_is_of_type (value, state_type));

  g_variant_ref_sink (value);

  if (! impl->priv->state || ! g_variant_equal (impl->priv->state, value))
    {
      if (impl->priv->state)
        g_variant_unref (impl->priv->state);

      impl->priv->state = g_variant_ref (value);

      g_object_notify (G_OBJECT (impl), "state");
    }

  g_variant_unref (value);
}
