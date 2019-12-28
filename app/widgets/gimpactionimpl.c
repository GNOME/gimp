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
#include "core/gimpmarshal.h"
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
  PROP_MAX_WIDTH_CHARS
};


static void   gimp_action_impl_finalize      (GObject        *object);
static void   gimp_action_impl_set_property  (GObject        *object,
                                              guint           prop_id,
                                              const GValue   *value,
                                              GParamSpec     *pspec);
static void   gimp_action_impl_get_property  (GObject        *object,
                                              guint           prop_id,
                                              GValue         *value,
                                              GParamSpec     *pspec);

static void   gimp_action_impl_activate      (GtkAction      *action);
static void   gimp_action_impl_connect_proxy (GtkAction      *action,
                                              GtkWidget      *proxy);

static void   gimp_action_impl_set_proxy     (GimpActionImpl *impl,
                                              GtkWidget      *proxy);


G_DEFINE_TYPE_WITH_CODE (GimpActionImpl, gimp_action_impl, GTK_TYPE_ACTION,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_ACTION, NULL))

#define parent_class gimp_action_impl_parent_class


static void
gimp_action_impl_class_init (GimpActionImplClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkActionClass *action_class = GTK_ACTION_CLASS (klass);
  GimpRGB         black;

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
}

static void
gimp_action_impl_init (GimpActionImpl *impl)
{
  impl->ellipsize       = PANGO_ELLIPSIZE_NONE;
  impl->max_width_chars = -1;

  gimp_action_init (GIMP_ACTION (impl));
}

static void
gimp_action_impl_finalize (GObject *object)
{
  GimpActionImpl *impl = GIMP_ACTION_IMPL (object);

  g_clear_object (&impl->context);
  g_clear_pointer (&impl->color, g_free);
  g_clear_object (&impl->viewable);

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
  if (! GTK_IS_IMAGE_MENU_ITEM (proxy))
    return;

  if (impl->color)
    {
      GtkWidget *area;

      area = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (proxy));

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

          gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (proxy),
                                             GTK_ICON_SIZE_MENU,
                                             &width, &height);

          gtk_widget_set_size_request (area, width, height);
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (proxy), area);
          gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (proxy),
                                                     TRUE);
          gtk_widget_show (area);
        }
    }
  else if (impl->viewable)
    {
      GtkWidget *view;

      view = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (proxy));

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

          gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (proxy),
                                             size, &width, &height);

          view = gimp_view_new_full (impl->context, impl->viewable,
                                     width, height, border_width,
                                     FALSE, FALSE, FALSE);
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (proxy), view);
          gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (proxy),
                                                     TRUE);
          gtk_widget_show (view);
        }
    }
  else
    {
      GtkWidget *image;

      image = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (proxy));

      if (GIMP_IS_VIEW (image) || GIMP_IS_COLOR_AREA (image))
        {
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (proxy), NULL);
          gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (proxy),
                                                     FALSE);
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
