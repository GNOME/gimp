/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginaction.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpmarshal.h"

#include "plug-in/gimppluginprocedure.h"

#include "gimppluginaction.h"


enum
{
  SELECTED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_PROCEDURE
};


static void   gimp_plug_in_action_finalize      (GObject      *object);
static void   gimp_plug_in_action_set_property  (GObject      *object,
                                                 guint         prop_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void   gimp_plug_in_action_get_property  (GObject      *object,
                                                 guint         prop_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);

static void   gimp_plug_in_action_activate      (GtkAction    *action);
static void   gimp_plug_in_action_connect_proxy (GtkAction    *action,
                                                 GtkWidget    *proxy);


G_DEFINE_TYPE (GimpPlugInAction, gimp_plug_in_action, GIMP_TYPE_ACTION)

#define parent_class gimp_plug_in_action_parent_class

static guint action_signals[LAST_SIGNAL] = { 0 };


static void
gimp_plug_in_action_class_init (GimpPlugInActionClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkActionClass *action_class = GTK_ACTION_CLASS (klass);

  object_class->finalize      = gimp_plug_in_action_finalize;
  object_class->set_property  = gimp_plug_in_action_set_property;
  object_class->get_property  = gimp_plug_in_action_get_property;

  action_class->activate      = gimp_plug_in_action_activate;
  action_class->connect_proxy = gimp_plug_in_action_connect_proxy;

  g_object_class_install_property (object_class, PROP_PROCEDURE,
                                   g_param_spec_object ("procedure",
                                                        NULL, NULL,
                                                        GIMP_TYPE_PLUG_IN_PROCEDURE,
                                                        GIMP_PARAM_READWRITE));

  action_signals[SELECTED] =
    g_signal_new ("selected",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPlugInActionClass, selected),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_PLUG_IN_PROCEDURE);
}

static void
gimp_plug_in_action_init (GimpPlugInAction *action)
{
  action->procedure = NULL;
}

static void
gimp_plug_in_action_finalize (GObject *object)
{
  GimpPlugInAction *action = GIMP_PLUG_IN_ACTION (object);

  if (action->procedure)
    {
      g_object_unref (action->procedure);
      action->procedure = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_plug_in_action_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpPlugInAction *action = GIMP_PLUG_IN_ACTION (object);

  switch (prop_id)
    {
    case PROP_PROCEDURE:
      g_value_set_object (value, action->procedure);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_plug_in_action_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpPlugInAction *action = GIMP_PLUG_IN_ACTION (object);

  switch (prop_id)
    {
    case PROP_PROCEDURE:
      if (action->procedure)
        g_object_unref (action->procedure);
      action->procedure = GIMP_PLUG_IN_PROCEDURE (g_value_dup_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gimp_plug_in_action_activate (GtkAction *action)
{
  GimpPlugInAction *plug_in_action = GIMP_PLUG_IN_ACTION (action);

  gimp_plug_in_action_selected (plug_in_action, plug_in_action->procedure);
}

static void
gimp_plug_in_action_connect_proxy (GtkAction *action,
                                   GtkWidget *proxy)
{
  GimpPlugInAction *plug_in_action = GIMP_PLUG_IN_ACTION (action);

  GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);

  if (GTK_IS_IMAGE_MENU_ITEM (proxy) && plug_in_action->procedure)
    {
      GdkPixbuf *pixbuf;

      pixbuf = gimp_plug_in_procedure_get_pixbuf (plug_in_action->procedure);

      if (pixbuf)
        {
          GtkSettings *settings = gtk_widget_get_settings (proxy);
          gint         width;
          gint         height;
          GtkWidget   *image;

          gtk_icon_size_lookup_for_settings (settings, GTK_ICON_SIZE_MENU,
                                             &width, &height);

          if (width  != gdk_pixbuf_get_width  (pixbuf) ||
              height != gdk_pixbuf_get_height (pixbuf))
            {
              GdkPixbuf *copy;

              copy = gdk_pixbuf_scale_simple (pixbuf, width, height,
                                              GDK_INTERP_BILINEAR);
              g_object_unref (pixbuf);
              pixbuf = copy;
            }

          image = gtk_image_new_from_pixbuf (pixbuf);
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (proxy), image);
          g_object_unref (pixbuf);
        }
    }
}


/*  public functions  */

GimpPlugInAction *
gimp_plug_in_action_new (const gchar         *name,
                         const gchar         *label,
                         const gchar         *tooltip,
                         const gchar         *stock_id,
                         GimpPlugInProcedure *procedure)
{
  return g_object_new (GIMP_TYPE_PLUG_IN_ACTION,
                       "name",      name,
                       "label",     label,
                       "tooltip",   tooltip,
                       "stock-id",  stock_id,
                       "procedure", procedure,
                       NULL);
}

void
gimp_plug_in_action_selected (GimpPlugInAction    *action,
                              GimpPlugInProcedure *procedure)
{
  g_return_if_fail (GIMP_IS_PLUG_IN_ACTION (action));

  g_signal_emit (action, action_signals[SELECTED], 0, procedure);
}
