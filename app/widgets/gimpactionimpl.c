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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimagefile.h"  /* eek */

#include "ligmaaction.h"
#include "ligmaactionimpl.h"
#include "ligmaaction-history.h"
#include "ligmaview.h"
#include "ligmaviewrenderer.h"
#include "ligmawidgets-utils.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_COLOR,
  PROP_VIEWABLE,
  PROP_ELLIPSIZE,
  PROP_MAX_WIDTH_CHARS
};


static void   ligma_action_iface_init              (LigmaActionInterface *iface);

static void   ligma_action_impl_finalize           (GObject             *object);
static void   ligma_action_impl_set_property       (GObject             *object,
                                                   guint                prop_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void   ligma_action_impl_get_property       (GObject             *object,
                                                   guint                prop_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static void   ligma_action_impl_set_disable_reason (LigmaAction          *action,
                                                   const gchar         *reason);
static const gchar *
              ligma_action_impl_get_disable_reason (LigmaAction          *action);

static void   ligma_action_impl_activate           (GtkAction           *action);
static void   ligma_action_impl_connect_proxy      (GtkAction           *action,
                                                   GtkWidget           *proxy);

static void   ligma_action_impl_set_proxy          (LigmaActionImpl      *impl,
                                                   GtkWidget           *proxy);


G_DEFINE_TYPE_WITH_CODE (LigmaActionImpl, ligma_action_impl, GTK_TYPE_ACTION,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_ACTION, ligma_action_iface_init))

#define parent_class ligma_action_impl_parent_class


static void
ligma_action_impl_class_init (LigmaActionImplClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkActionClass *action_class = GTK_ACTION_CLASS (klass);
  LigmaRGB         black;

  object_class->finalize      = ligma_action_impl_finalize;
  object_class->set_property  = ligma_action_impl_set_property;
  object_class->get_property  = ligma_action_impl_get_property;

  action_class->activate      = ligma_action_impl_activate;
  action_class->connect_proxy = ligma_action_impl_connect_proxy;

  ligma_rgba_set (&black, 0.0, 0.0, 0.0, LIGMA_OPACITY_OPAQUE);

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_COLOR,
                                   ligma_param_spec_rgb ("color",
                                                        NULL, NULL,
                                                        TRUE, &black,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_VIEWABLE,
                                   g_param_spec_object ("viewable",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_VIEWABLE,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      NULL, NULL,
                                                      PANGO_TYPE_ELLIPSIZE_MODE,
                                                      PANGO_ELLIPSIZE_NONE,
                                                      LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_MAX_WIDTH_CHARS,
                                   g_param_spec_int ("max-width-chars",
                                                     NULL, NULL,
                                                     -1, G_MAXINT, -1,
                                                     LIGMA_PARAM_READWRITE));
}

static void
ligma_action_iface_init (LigmaActionInterface *iface)
{
  iface->set_disable_reason = ligma_action_impl_set_disable_reason;
  iface->get_disable_reason = ligma_action_impl_get_disable_reason;
}

static void
ligma_action_impl_init (LigmaActionImpl *impl)
{
  impl->ellipsize       = PANGO_ELLIPSIZE_NONE;
  impl->max_width_chars = -1;
  impl->disable_reason  = NULL;

  ligma_action_init (LIGMA_ACTION (impl));
}

static void
ligma_action_impl_finalize (GObject *object)
{
  LigmaActionImpl *impl = LIGMA_ACTION_IMPL (object);

  g_clear_pointer (&impl->disable_reason, g_free);
  g_clear_object  (&impl->context);
  g_clear_pointer (&impl->color, g_free);
  g_clear_object  (&impl->viewable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_action_impl_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaActionImpl *impl = LIGMA_ACTION_IMPL (object);

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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ligma_action_impl_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaActionImpl *impl      = LIGMA_ACTION_IMPL (object);
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

  if (set_proxy)
    {
      GSList *list;

      for (list = ligma_action_get_proxies (LIGMA_ACTION (impl));
           list;
           list = g_slist_next (list))
        {
          ligma_action_impl_set_proxy (impl, list->data);
        }
    }
}

static void
ligma_action_impl_set_disable_reason (LigmaAction  *action,
                                     const gchar *reason)
{
  LigmaActionImpl *impl = LIGMA_ACTION_IMPL (action);

  g_clear_pointer (&impl->disable_reason, g_free);
  if (reason)
    impl->disable_reason = g_strdup (reason);
}

static const gchar *
ligma_action_impl_get_disable_reason (LigmaAction *action)
{
  LigmaActionImpl *impl = LIGMA_ACTION_IMPL (action);

  return (const gchar *) impl->disable_reason;
}

static void
ligma_action_impl_activate (GtkAction *action)
{
  if (GTK_ACTION_CLASS (parent_class)->activate)
    GTK_ACTION_CLASS (parent_class)->activate (action);

  ligma_action_emit_activate (LIGMA_ACTION (action), NULL);

  ligma_action_history_action_activated (LIGMA_ACTION (action));
}

static void
ligma_action_impl_connect_proxy (GtkAction *action,
                                GtkWidget *proxy)
{
  GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);

  ligma_action_impl_set_proxy (LIGMA_ACTION_IMPL (action), proxy);

  ligma_action_set_proxy (LIGMA_ACTION (action), proxy);
}


/*  public functions  */

LigmaAction *
ligma_action_impl_new (const gchar *name,
                      const gchar *label,
                      const gchar *tooltip,
                      const gchar *icon_name,
                      const gchar *help_id)
{
  LigmaAction *action;

  action = g_object_new (LIGMA_TYPE_ACTION_IMPL,
                         "name",      name,
                         "label",     label,
                         "tooltip",   tooltip,
                         "icon-name", icon_name,
                         NULL);

  ligma_action_set_help_id (action, help_id);

  return action;
}


/*  private functions  */

static void
ligma_action_impl_set_proxy (LigmaActionImpl *impl,
                            GtkWidget      *proxy)
{
  if (! GTK_IS_MENU_ITEM (proxy))
    return;

  if (impl->color)
    {
      GtkWidget *area;

      area = ligma_menu_item_get_image (GTK_MENU_ITEM (proxy));

      if (LIGMA_IS_COLOR_AREA (area))
        {
          ligma_color_area_set_color (LIGMA_COLOR_AREA (area), impl->color);
        }
      else
        {
          gint width, height;

          area = ligma_color_area_new (impl->color,
                                      LIGMA_COLOR_AREA_SMALL_CHECKS, 0);
          ligma_color_area_set_draw_border (LIGMA_COLOR_AREA (area), TRUE);

          if (impl->context)
            ligma_color_area_set_color_config (LIGMA_COLOR_AREA (area),
                                              impl->context->ligma->config->color_management);

          gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
          gtk_widget_set_size_request (area, width, height);
          ligma_menu_item_set_image (GTK_MENU_ITEM (proxy), area);
          gtk_widget_show (area);
        }
    }
  else if (impl->viewable)
    {
      GtkWidget *view;

      view = ligma_menu_item_get_image (GTK_MENU_ITEM (proxy));

      if (LIGMA_IS_VIEW (view) &&
          g_type_is_a (G_TYPE_FROM_INSTANCE (impl->viewable),
                       LIGMA_VIEW (view)->renderer->viewable_type))
        {
          ligma_view_set_viewable (LIGMA_VIEW (view), impl->viewable);
        }
      else
        {
          GtkIconSize size;
          gint        width, height;
          gint        border_width;

          if (LIGMA_IS_IMAGEFILE (impl->viewable))
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
          view = ligma_view_new_full (impl->context, impl->viewable,
                                     width, height, border_width,
                                     FALSE, FALSE, FALSE);
          ligma_menu_item_set_image (GTK_MENU_ITEM (proxy), view);
          gtk_widget_show (view);
        }
    }
  else
    {
      GtkWidget *image;

      image = ligma_menu_item_get_image (GTK_MENU_ITEM (proxy));

      if (LIGMA_IS_VIEW (image) || LIGMA_IS_COLOR_AREA (image))
        {
          ligma_menu_item_set_image (GTK_MENU_ITEM (proxy), NULL);
          g_object_notify (G_OBJECT (impl), "icon-name");
        }
    }

  {
    GtkWidget *child = gtk_bin_get_child (GTK_BIN (proxy));

    if (GTK_IS_BOX (child))
      child = g_object_get_data (G_OBJECT (proxy), "ligma-menu-item-label");

    if (GTK_IS_LABEL (child))
      {
        GtkLabel *label = GTK_LABEL (child);

        gtk_label_set_ellipsize (label, impl->ellipsize);
        gtk_label_set_max_width_chars (label, impl->max_width_chars);
      }
  }
}
