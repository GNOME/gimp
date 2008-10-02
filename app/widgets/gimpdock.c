/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdock.c
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"

#include "gimpdialogfactory.h"
#include "gimpdock.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimpdockseparator.h"
#include "gimpwidgets-utils.h"

#include "gimpsessioninfo.h"    /* FIXME */
#include "core/gimpcontainer.h" /* FIXME */
#include "dialogs/dialogs.h"    /* FIXME */

#include "gimp-intl.h"


#define DEFAULT_DOCK_HEIGHT     300
#define DEFAULT_DOCK_FONT_SCALE PANGO_SCALE_SMALL

enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_DIALOG_FACTORY
};

enum
{
  BOOK_ADDED,
  BOOK_REMOVED,
  LAST_SIGNAL
};


static GObject * gimp_dock_constructor       (GType                  type,
                                              guint                  n_params,
                                              GObjectConstructParam *params);
static void      gimp_dock_set_property      (GObject               *object,
                                              guint                  property_id,
                                              const GValue          *value,
                                              GParamSpec            *pspec);
static void      gimp_dock_get_property      (GObject               *object,
                                              guint                  property_id,
                                              GValue                *value,
                                              GParamSpec            *pspec);

static void      gimp_dock_destroy           (GtkObject             *object);

static gboolean  gimp_dock_delete_event      (GtkWidget             *widget,
                                              GdkEventAny           *event);
static void      gimp_dock_style_set         (GtkWidget             *widget,
                                              GtkStyle              *prev_style);

static void      gimp_dock_real_book_added   (GimpDock              *dock,
                                              GimpDockbook          *dockbook);
static void      gimp_dock_real_book_removed (GimpDock              *dock,
                                              GimpDockbook          *dockbook);


G_DEFINE_TYPE (GimpDock, gimp_dock, GIMP_TYPE_WINDOW)

#define parent_class gimp_dock_parent_class

static guint dock_signals[LAST_SIGNAL] = { 0 };


static void
gimp_dock_class_init (GimpDockClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class     = GTK_WIDGET_CLASS (klass);

  dock_signals[BOOK_ADDED] =
    g_signal_new ("book-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockClass, book_added),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCKBOOK);

  dock_signals[BOOK_REMOVED] =
    g_signal_new ("book-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDockClass, book_removed),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DOCKBOOK);

  object_class->constructor     = gimp_dock_constructor;
  object_class->set_property    = gimp_dock_set_property;
  object_class->get_property    = gimp_dock_get_property;

  gtk_object_class->destroy     = gimp_dock_destroy;

  widget_class->delete_event    = gimp_dock_delete_event;
  widget_class->style_set       = gimp_dock_style_set;

  klass->setup                  = NULL;
  klass->book_added             = gimp_dock_real_book_added;
  klass->book_removed           = gimp_dock_real_book_removed;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DIALOG_FACTORY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("default-height",
                                                             NULL, NULL,
                                                             -1, G_MAXINT,
                                                             DEFAULT_DOCK_HEIGHT,
                                                             GIMP_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_double ("font-scale",
                                                                NULL, NULL,
                                                                0.0,
                                                                G_MAXDOUBLE,
                                                                DEFAULT_DOCK_FONT_SCALE,
                                                                GIMP_PARAM_READABLE));
}

static void
gimp_dock_init (GimpDock *dock)
{
  static gint  dock_ID = 1;
  GtkWidget   *separator;
  gchar       *name;

  dock->context        = NULL;
  dock->dialog_factory = NULL;
  dock->dockbooks      = NULL;
  dock->ID             = dock_ID++;

  name = g_strdup_printf ("gimp-dock-%d", dock->ID);
  gtk_widget_set_name (GTK_WIDGET (dock), name);
  g_free (name);

  gtk_window_set_role (GTK_WINDOW (dock), "gimp-dock");
  gtk_window_set_resizable (GTK_WINDOW (dock), TRUE);
  gtk_window_set_focus_on_map (GTK_WINDOW (dock), FALSE);

  dock->main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dock), dock->main_vbox);
  gtk_widget_show (dock->main_vbox);

  dock->vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dock->main_vbox), dock->vbox);
  gtk_widget_show (dock->vbox);

  separator = gimp_dock_separator_new (dock);
  gtk_box_pack_start (GTK_BOX (dock->vbox), separator, FALSE, FALSE, 0);
  gtk_widget_show (separator);
}

static GObject *
gimp_dock_constructor (GType                  type,
                       guint                  n_params,
                       GObjectConstructParam *params)
{
  GObject       *object;
  GimpDock      *dock;
  GimpGuiConfig *config;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  dock = GIMP_DOCK (object);

  g_assert (GIMP_IS_CONTEXT (dock->context));
  g_assert (GIMP_IS_DIALOG_FACTORY (dock->dialog_factory));

  config = GIMP_GUI_CONFIG (dock->context->gimp->config);

  gimp_window_set_hint (GTK_WINDOW (dock), config->dock_window_hint);

  return object;
}

static void
gimp_dock_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpDock *dock = GIMP_DOCK (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      dock->context = g_value_dup_object (value);
      break;

    case PROP_DIALOG_FACTORY:
      dock->dialog_factory = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dock_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpDock *dock = GIMP_DOCK (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, dock->context);
      break;

    case PROP_DIALOG_FACTORY:
      g_value_set_object (value, dock->dialog_factory);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dock_destroy (GtkObject *object)
{
  GimpDock *dock = GIMP_DOCK (object);

  while (dock->dockbooks)
    gimp_dock_remove_book (dock, GIMP_DOCKBOOK (dock->dockbooks->data));

  if (dock->context)
    {
      g_object_unref (dock->context);
      dock->context = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static gboolean
gimp_dock_delete_event (GtkWidget   *widget,
                        GdkEventAny *event)
{
  GimpDock *dock = GIMP_DOCK (widget);
  GList    *list;
  gint      n;

  for (list = dock->dockbooks, n = 0; list; list = list->next)
    n += gtk_notebook_get_n_pages (GTK_NOTEBOOK (list->data));

  if (n > 1)
    {
      GimpSessionInfo *info = gimp_session_info_new ();

      gimp_object_set_name (GIMP_OBJECT (info),
                            gtk_window_get_title (GTK_WINDOW (widget)));

      info->widget = widget;
      gimp_session_info_get_info (info);
      info->widget = NULL;

      gimp_container_add (global_recent_docks, GIMP_OBJECT (info));
      g_object_unref (info);
    }

  return FALSE;
}

static void
gimp_dock_style_set (GtkWidget *widget,
                     GtkStyle  *prev_style)
{
  gint    default_height;
  gdouble font_scale;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "default-height", &default_height,
                        "font-scale",     &font_scale,
                        NULL);

  gtk_window_set_default_size (GTK_WINDOW (widget), -1, default_height);

  if (font_scale != 1.0)
    {
      PangoContext         *context;
      PangoFontDescription *font_desc;
      gint                  font_size;
      gchar                *font_str;
      gchar                *rc_string;

      context = gtk_widget_get_pango_context (widget);
      font_desc = pango_context_get_font_description (context);
      font_desc = pango_font_description_copy (font_desc);

      font_size = pango_font_description_get_size (font_desc);
      font_size = font_scale * font_size;
      pango_font_description_set_size (font_desc, font_size);

      font_str = pango_font_description_to_string (font_desc);
      pango_font_description_free (font_desc);

      rc_string =
        g_strdup_printf ("style \"gimp-dock-style\""
                         "{"
                         "  font_name = \"%s\""
                         "}"
                         "widget \"gimp-dock-%d.*\" style \"gimp-dock-style\"",
                         font_str,
                         GIMP_DOCK (widget)->ID);
      g_free (font_str);

      gtk_rc_parse_string (rc_string);
      g_free (rc_string);

      if (gtk_bin_get_child (GTK_BIN (widget)))
        gtk_widget_reset_rc_styles (gtk_bin_get_child (GTK_BIN (widget)));
    }
}

static void
gimp_dock_real_book_added (GimpDock     *dock,
                           GimpDockbook *dockbook)
{
}

static void
gimp_dock_real_book_removed (GimpDock     *dock,
                             GimpDockbook *dockbook)
{
  if (dock->dockbooks == NULL)
    gtk_widget_destroy (GTK_WIDGET (dock));
}


/*  public functions  */

void
gimp_dock_setup (GimpDock       *dock,
                 const GimpDock *template)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GIMP_IS_DOCK (template));

  if (GIMP_DOCK_GET_CLASS (dock)->setup)
    GIMP_DOCK_GET_CLASS (dock)->setup (dock, template);
}

void
gimp_dock_set_aux_info (GimpDock *dock,
                        GList    *aux_info)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));

  if (GIMP_DOCK_GET_CLASS (dock)->set_aux_info)
    GIMP_DOCK_GET_CLASS (dock)->set_aux_info (dock, aux_info);
}

GList *
gimp_dock_get_aux_info (GimpDock *dock)
{
  g_return_val_if_fail (GIMP_IS_DOCK (dock), NULL);

  if (GIMP_DOCK_GET_CLASS (dock)->get_aux_info)
    return GIMP_DOCK_GET_CLASS (dock)->get_aux_info (dock);

  return NULL;
}

void
gimp_dock_add (GimpDock     *dock,
               GimpDockable *dockable,
               gint          section,
               gint          position)
{
  GimpDockbook *dockbook;

  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (dockable->dockbook == NULL);

  dockbook = GIMP_DOCKBOOK (dock->dockbooks->data);

  gimp_dockbook_add (dockbook, dockable, position);
}

void
gimp_dock_remove (GimpDock     *dock,
                  GimpDockable *dockable)
{
  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GIMP_IS_DOCKABLE (dockable));
  g_return_if_fail (dockable->dockbook != NULL);
  g_return_if_fail (dockable->dockbook->dock == dock);

  gimp_dockbook_remove (dockable->dockbook, dockable);
}

void
gimp_dock_add_book (GimpDock     *dock,
                    GimpDockbook *dockbook,
                    gint          index)
{
  gint old_length;

  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));
  g_return_if_fail (dockbook->dock == NULL);

  old_length = g_list_length (dock->dockbooks);

  if (index >= old_length || index < 0)
    index = old_length;

  dockbook->dock  = dock;
  dock->dockbooks = g_list_insert (dock->dockbooks, dockbook, index);

  if (old_length == 0)
    {
      GtkWidget *separator;

      gtk_box_pack_start (GTK_BOX (dock->vbox), GTK_WIDGET (dockbook),
                          TRUE, TRUE, 0);

      separator = gimp_dock_separator_new (dock);
      gtk_box_pack_end (GTK_BOX (dock->vbox), separator, FALSE, FALSE, 0);
      gtk_widget_show (separator);
    }
  else
    {
      GtkWidget *old_book;
      GtkWidget *parent;
      GtkWidget *paned;

      if (index == 0)
        old_book = g_list_nth_data (dock->dockbooks, index + 1);
      else
        old_book = g_list_nth_data (dock->dockbooks, index - 1);

      parent = gtk_widget_get_parent (old_book);

      if ((old_length > 1) && (index > 0))
        {
          GtkWidget *grandparent;

          grandparent = gtk_widget_get_parent (parent);

          old_book = parent;
          parent   = grandparent;
        }

      g_object_ref (old_book);

      gtk_container_remove (GTK_CONTAINER (parent), old_book);

      paned = gtk_vpaned_new ();

      if (GTK_IS_VPANED (parent))
        gtk_paned_pack1 (GTK_PANED (parent), paned, TRUE, FALSE);
      else
        gtk_box_pack_start (GTK_BOX (parent), paned, TRUE, TRUE, 0);

      gtk_widget_show (paned);

      if (index == 0)
        {
          gtk_paned_pack1 (GTK_PANED (paned), GTK_WIDGET (dockbook),
                           TRUE, FALSE);
          gtk_paned_pack2 (GTK_PANED (paned), old_book,
                           TRUE, FALSE);
        }
      else
        {
          gtk_paned_pack1 (GTK_PANED (paned), old_book,
                           TRUE, FALSE);
          gtk_paned_pack2 (GTK_PANED (paned), GTK_WIDGET (dockbook),
                           TRUE, FALSE);
        }

      g_object_unref (old_book);
    }

  gtk_widget_show (GTK_WIDGET (dockbook));

  g_signal_emit (dock, dock_signals[BOOK_ADDED], 0, dockbook);
}

void
gimp_dock_remove_book (GimpDock     *dock,
                       GimpDockbook *dockbook)
{
  gint old_length;
  gint index;

  g_return_if_fail (GIMP_IS_DOCK (dock));
  g_return_if_fail (GIMP_IS_DOCKBOOK (dockbook));

  g_return_if_fail (dockbook->dock == dock);

  old_length = g_list_length (dock->dockbooks);
  index      = g_list_index (dock->dockbooks, dockbook);

  dockbook->dock  = NULL;
  dock->dockbooks = g_list_remove (dock->dockbooks, dockbook);

  g_object_ref (dockbook);

  if (old_length == 1)
    {
      GtkWidget *separator;
      GList     *children;

      children = gtk_container_get_children (GTK_CONTAINER (dock->vbox));

      separator = g_list_nth_data (children, 2);

      gtk_container_remove (GTK_CONTAINER (dock->vbox), separator);
      gtk_container_remove (GTK_CONTAINER (dock->vbox), GTK_WIDGET (dockbook));

      g_list_free (children);
    }
  else
    {
      GtkWidget *other_book;
      GtkWidget *parent;
      GtkWidget *grandparent;

      parent      = gtk_widget_get_parent (GTK_WIDGET (dockbook));
      grandparent = gtk_widget_get_parent (parent);

      if (index == 0)
        other_book = gtk_paned_get_child2 (GTK_PANED (parent));
      else
        other_book = gtk_paned_get_child1 (GTK_PANED (parent));

      g_object_ref (other_book);

      gtk_container_remove (GTK_CONTAINER (parent), other_book);
      gtk_container_remove (GTK_CONTAINER (parent), GTK_WIDGET (dockbook));

      gtk_container_remove (GTK_CONTAINER (grandparent), parent);

      if (GTK_IS_VPANED (grandparent))
        gtk_paned_pack1 (GTK_PANED (grandparent), other_book, TRUE, FALSE);
      else
        gtk_box_pack_start (GTK_BOX (dock->vbox), other_book, TRUE, TRUE, 0);

      g_object_unref (other_book);
    }

  g_signal_emit (dock, dock_signals[BOOK_REMOVED], 0, dockbook);

  g_object_unref (dockbook);
}
