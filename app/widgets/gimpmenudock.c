/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagedock.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "gimpdialogfactory.h"
#include "gimpimagedock.h"
#include "gimpcontainercombobox.h"
#include "gimpcontainerview.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimphelp-ids.h"
#include "gimpmenufactory.h"
#include "gimpsessioninfo.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


#define DEFAULT_MINIMAL_WIDTH     250
#define DEFAULT_MENU_PREVIEW_SIZE GTK_ICON_SIZE_SMALL_TOOLBAR


static void   gimp_image_dock_class_init          (GimpImageDockClass *klass);
static void   gimp_image_dock_init                (GimpImageDock      *dock);

static GObject * gimp_image_dock_constructor   (GType                  type,
                                                guint                  n_params,
                                                GObjectConstructParam *params);
static void   gimp_image_dock_destroy                 (GtkObject      *object);

static void   gimp_image_dock_style_set               (GtkWidget      *widget,
                                                       GtkStyle       *prev_style);

static void   gimp_image_dock_setup                   (GimpDock       *dock,
                                                       const GimpDock *template);
static void   gimp_image_dock_set_aux_info            (GimpDock       *dock,
                                                       GList          *aux_info);
static GList *gimp_image_dock_get_aux_info            (GimpDock       *dock);
static void   gimp_image_dock_book_added              (GimpDock       *dock,
                                                       GimpDockbook   *dockbook);
static void   gimp_image_dock_book_removed            (GimpDock       *dock,
                                                       GimpDockbook   *dockbook);

static void   gimp_image_dock_dockbook_changed        (GimpDockbook   *dockbook,
                                                       GimpDockable   *dockable,
                                                       GimpImageDock  *dock);
static void   gimp_image_dock_update_title            (GimpImageDock  *dock);

static void   gimp_image_dock_factory_display_changed (GimpContext    *context,
                                                       GimpObject     *display,
                                                       GimpDock       *dock);
static void   gimp_image_dock_factory_image_changed   (GimpContext    *context,
                                                       GimpImage      *gimage,
                                                       GimpDock       *dock);
static void   gimp_image_dock_display_changed         (GimpContext    *context,
                                                       GimpObject     *display,
                                                       GimpDock       *dock);
static void   gimp_image_dock_image_changed           (GimpContext    *context,
                                                       GimpImage      *gimage,
                                                       GimpDock       *dock);
static void   gimp_image_dock_auto_clicked            (GtkWidget      *widget,
                                                       GimpDock       *dock);
static void   gimp_image_dock_image_flush             (GimpImage      *gimage,
                                                       GimpDock       *dock);


static GimpDockClass *parent_class = NULL;


GType
gimp_image_dock_get_type (void)
{
  static GType dock_type = 0;

  if (! dock_type)
    {
      static const GTypeInfo dock_info =
      {
        sizeof (GimpImageDockClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_image_dock_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpImageDock),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_image_dock_init,
      };

      dock_type = g_type_register_static (GIMP_TYPE_DOCK,
                                          "GimpImageDock",
                                          &dock_info, 0);
    }

  return dock_type;
}

static void
gimp_image_dock_class_init (GimpImageDockClass *klass)
{
  GObjectClass   *object_class;
  GtkObjectClass *gtk_object_class;
  GtkWidgetClass *widget_class;
  GimpDockClass  *dock_class;

  object_class     = G_OBJECT_CLASS (klass);
  gtk_object_class = GTK_OBJECT_CLASS (klass);
  widget_class     = GTK_WIDGET_CLASS (klass);
  dock_class       = GIMP_DOCK_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor = gimp_image_dock_constructor;

  gtk_object_class->destroy = gimp_image_dock_destroy;

  widget_class->style_set   = gimp_image_dock_style_set;

  dock_class->setup         = gimp_image_dock_setup;
  dock_class->set_aux_info  = gimp_image_dock_set_aux_info;
  dock_class->get_aux_info  = gimp_image_dock_get_aux_info;
  dock_class->book_added    = gimp_image_dock_book_added;
  dock_class->book_removed  = gimp_image_dock_book_removed;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("minimal_width",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_MINIMAL_WIDTH,
                                                             G_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("menu_preview_size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_MENU_PREVIEW_SIZE,
                                                              G_PARAM_READABLE));
}

static void
gimp_image_dock_init (GimpImageDock *dock)
{
  GtkWidget *hbox;

  dock->image_container      = NULL;
  dock->display_container    = NULL;
  dock->show_image_menu      = FALSE;
  dock->auto_follow_active   = TRUE;
  dock->update_title_idle_id = 0;

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (GIMP_DOCK (dock)->main_vbox), hbox,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (GIMP_DOCK (dock)->main_vbox), hbox, 0);

  if (dock->show_image_menu)
    gtk_widget_show (hbox);

  dock->image_combo = gimp_container_combo_box_new (NULL, NULL, 16, 1);
  gtk_box_pack_start (GTK_BOX (hbox), dock->image_combo, TRUE, TRUE, 0);
  gtk_widget_show (dock->image_combo);

  g_signal_connect (dock->image_combo, "destroy",
		    G_CALLBACK (gtk_widget_destroyed),
		    &dock->image_combo);

  gimp_help_set_help_data (dock->image_combo, NULL, GIMP_HELP_DOCK_IMAGE_MENU);

  dock->auto_button = gtk_toggle_button_new_with_label (_("Auto"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dock->auto_button),
				dock->auto_follow_active);
  gtk_box_pack_start (GTK_BOX (hbox), dock->auto_button, FALSE, FALSE, 0);
  gtk_widget_show (dock->auto_button);

  g_signal_connect (dock->auto_button, "clicked",
		    G_CALLBACK (gimp_image_dock_auto_clicked),
		    dock);

  gimp_help_set_help_data (dock->auto_button,
                           _("When enabled the dialog automatically "
                             "follows the image you are working on."),
                           GIMP_HELP_DOCK_AUTO_BUTTON);
}

static GObject *
gimp_image_dock_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject         *object;
  GimpImageDock   *dock;
  GimpMenuFactory *menu_factory;
  GtkUIManager    *ui_manager;
  GtkAccelGroup   *accel_group;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  dock = GIMP_IMAGE_DOCK (object);

  menu_factory = GIMP_DOCK (object)->dialog_factory->menu_factory;

  dock->ui_manager = gimp_menu_factory_manager_new (menu_factory, "<Dock>",
                                                    dock, FALSE);

  ui_manager = GTK_UI_MANAGER (dock->ui_manager);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);

  gtk_window_add_accel_group (GTK_WINDOW (object), accel_group);

  return object;
}

static void
gimp_image_dock_destroy (GtkObject *object)
{
  GimpImageDock *dock = GIMP_IMAGE_DOCK (object);

  if (dock->update_title_idle_id)
    {
      g_source_remove (dock->update_title_idle_id);
      dock->update_title_idle_id = 0;
    }

  if (dock->image_flush_handler_id)
    {
      gimp_container_remove_handler (dock->image_container,
                                     dock->image_flush_handler_id);
      dock->image_flush_handler_id = 0;
    }

  if (dock->ui_manager)
    {
      g_object_unref (dock->ui_manager);
      dock->ui_manager = NULL;
    }

  /*  remove the image menu and the auto button manually here because
   *  of weird cross-connections with GimpDock's context
   */
  if (GIMP_DOCK (dock)->main_vbox &&
      dock->image_combo           &&
      dock->image_combo->parent)
    {
      gtk_container_remove (GTK_CONTAINER (GIMP_DOCK (dock)->main_vbox),
			    dock->image_combo->parent);
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_image_dock_style_set (GtkWidget *widget,
                           GtkStyle  *prev_style)
{
  GimpImageDock *image_dock;
  gint           minimal_width;
  GtkIconSize    menu_preview_size;
  GdkScreen     *screen;
  gint           menu_preview_width  = 18;
  gint           menu_preview_height = 18;
  gint           focus_line_width;
  gint           focus_padding;
  gint           ythickness;

  image_dock = GIMP_IMAGE_DOCK (widget);

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "minimal_width",     &minimal_width,
                        "menu_preview_size", &menu_preview_size,
			NULL);

  screen = gtk_widget_get_screen (image_dock->image_combo);
  gtk_icon_size_lookup_for_settings (gtk_settings_get_for_screen (screen),
                                     menu_preview_size,
                                     &menu_preview_width,
                                     &menu_preview_height);

  gtk_widget_style_get (image_dock->auto_button,
                        "focus_line_width", &focus_line_width,
                        "focus_padding",    &focus_padding,
			NULL);

  ythickness = image_dock->auto_button->style->ythickness;

  gtk_widget_set_size_request (widget, minimal_width, -1);

  gimp_container_view_set_preview_size (GIMP_CONTAINER_VIEW (image_dock->image_combo),
                                        menu_preview_height, 1);

  gtk_widget_set_size_request (image_dock->auto_button, -1,
                               menu_preview_height +
                               2 * (1 /* CHILD_SPACING */ +
                                    ythickness            +
                                    focus_padding         +
                                    focus_line_width));
}

static void
gimp_image_dock_setup (GimpDock       *dock,
                       const GimpDock *template)
{
  if (GIMP_IS_IMAGE_DOCK (template))
    {
      gboolean auto_follow_active;
      gboolean show_image_menu;

      auto_follow_active = GIMP_IMAGE_DOCK (template)->auto_follow_active;
      show_image_menu    = GIMP_IMAGE_DOCK (template)->show_image_menu;

      gimp_image_dock_set_auto_follow_active (GIMP_IMAGE_DOCK (dock),
                                              auto_follow_active);
      gimp_image_dock_set_show_image_menu (GIMP_IMAGE_DOCK (dock),
                                           show_image_menu);
    }
}

#define AUX_INFO_SHOW_IMAGE_MENU     "show-image-menu"
#define AUX_INFO_FOLLOW_ACTIVE_IMAGE "follow-active-image"

static void
gimp_image_dock_set_aux_info (GimpDock *dock,
                              GList    *aux_info)
{
  GimpImageDock *image_dock  = GIMP_IMAGE_DOCK (dock);
  GList         *list;
  gboolean       menu_shown  = image_dock->show_image_menu;
  gboolean       auto_follow = image_dock->auto_follow_active;

  for (list = aux_info; list; list = g_list_next (list))
    {
      GimpSessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_SHOW_IMAGE_MENU))
        {
          menu_shown = ! g_ascii_strcasecmp (aux->value, "true");
        }
      else if (! strcmp (aux->name, AUX_INFO_FOLLOW_ACTIVE_IMAGE))
        {
          auto_follow = ! g_ascii_strcasecmp (aux->value, "true");
        }
    }

  if (menu_shown != image_dock->show_image_menu)
    gimp_image_dock_set_show_image_menu (image_dock, menu_shown);

  if (auto_follow != image_dock->auto_follow_active)
    gimp_image_dock_set_auto_follow_active (image_dock, auto_follow);
}

static GList *
gimp_image_dock_get_aux_info (GimpDock *dock)
{
  GimpImageDock      *image_dock = GIMP_IMAGE_DOCK (dock);
  GList              *aux_info   = NULL;
  GimpSessionInfoAux *aux;

  aux = gimp_session_info_aux_new (AUX_INFO_SHOW_IMAGE_MENU,
                                   image_dock->show_image_menu ?
                                   "true" : "false");
  aux_info = g_list_append (aux_info, aux);

  aux = gimp_session_info_aux_new (AUX_INFO_FOLLOW_ACTIVE_IMAGE,
                                   image_dock->auto_follow_active ?
                                   "true" : "false");
  aux_info = g_list_append (aux_info, aux);

  return aux_info;
}

static void
gimp_image_dock_book_added (GimpDock     *dock,
                            GimpDockbook *dockbook)
{
  g_signal_connect (dockbook, "dockable_added",
                    G_CALLBACK (gimp_image_dock_dockbook_changed),
                    dock);
  g_signal_connect (dockbook, "dockable_removed",
                    G_CALLBACK (gimp_image_dock_dockbook_changed),
                    dock);
  g_signal_connect (dockbook, "dockable_reordered",
                    G_CALLBACK (gimp_image_dock_dockbook_changed),
                    dock);

  gimp_image_dock_update_title (GIMP_IMAGE_DOCK (dock));

  GIMP_DOCK_CLASS (parent_class)->book_added (dock, dockbook);
}

static void
gimp_image_dock_book_removed (GimpDock     *dock,
                              GimpDockbook *dockbook)
{
  g_signal_handlers_disconnect_by_func (dockbook,
                                        gimp_image_dock_dockbook_changed,
                                        dock);

  gimp_image_dock_update_title (GIMP_IMAGE_DOCK (dock));

  GIMP_DOCK_CLASS (parent_class)->book_removed (dock, dockbook);
}

GtkWidget *
gimp_image_dock_new (GimpDialogFactory *dialog_factory,
		     GimpContainer     *image_container,
                     GimpContainer     *display_container)
{
  GimpImageDock *image_dock;
  GimpContext   *context;
  GdkScreen     *screen;
  gint           menu_preview_width;
  gint           menu_preview_height;

  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (image_container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (display_container), NULL);

  context = gimp_context_new (dialog_factory->context->gimp,
                              "Dock Context", NULL);

  image_dock = g_object_new (GIMP_TYPE_IMAGE_DOCK,
                             "context",        context,
                             "dialog-factory", dialog_factory,
                             NULL);

  image_dock->image_container   = image_container;
  image_dock->display_container = display_container;

  image_dock->image_flush_handler_id =
    gimp_container_add_handler (image_container, "flush",
                                G_CALLBACK (gimp_image_dock_image_flush),
                                image_dock);

  gimp_help_connect (GTK_WIDGET (image_dock), gimp_standard_help_func,
                     GIMP_HELP_DOCK, NULL);

  gimp_context_define_properties (context,
				  GIMP_CONTEXT_ALL_PROPS_MASK &
				  ~(GIMP_CONTEXT_IMAGE_MASK |
				    GIMP_CONTEXT_DISPLAY_MASK),
				  FALSE);
  gimp_context_set_parent (context, dialog_factory->context);

  if (image_dock->auto_follow_active)
    {
      if (gimp_context_get_display (dialog_factory->context))
        gimp_context_copy_property (dialog_factory->context, context,
                                    GIMP_CONTEXT_PROP_DISPLAY);
      else
        gimp_context_copy_property (dialog_factory->context, context,
                                    GIMP_CONTEXT_PROP_IMAGE);
    }

  g_signal_connect_object (dialog_factory->context, "display_changed",
			   G_CALLBACK (gimp_image_dock_factory_display_changed),
			   image_dock,
			   0);
  g_signal_connect_object (dialog_factory->context, "image_changed",
			   G_CALLBACK (gimp_image_dock_factory_image_changed),
			   image_dock,
			   0);

  g_signal_connect_object (context, "display_changed",
			   G_CALLBACK (gimp_image_dock_display_changed),
			   image_dock,
			   0);
  g_signal_connect_object (context, "image_changed",
			   G_CALLBACK (gimp_image_dock_image_changed),
			   image_dock,
			   0);

  screen = gtk_widget_get_screen (GTK_WIDGET (image_dock));
  gtk_icon_size_lookup_for_settings (gtk_settings_get_for_screen (screen),
                                     DEFAULT_MENU_PREVIEW_SIZE,
                                     &menu_preview_width,
                                     &menu_preview_height);

  g_object_set (image_dock->image_combo,
                "container", image_container,
                "context",   context,
                NULL);

  return GTK_WIDGET (image_dock);
}

void
gimp_image_dock_set_auto_follow_active (GimpImageDock *image_dock,
					gboolean       auto_follow_active)
{
  g_return_if_fail (GIMP_IS_IMAGE_DOCK (image_dock));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (image_dock->auto_button),
				auto_follow_active ? TRUE : FALSE);
}

void
gimp_image_dock_set_show_image_menu (GimpImageDock *image_dock,
				     gboolean       show)
{
  g_return_if_fail (GIMP_IS_IMAGE_DOCK (image_dock));

  if (show)
    gtk_widget_show (image_dock->image_combo->parent);
  else
    gtk_widget_hide (image_dock->image_combo->parent);

  image_dock->show_image_menu = show ? TRUE : FALSE;
}

static void
gimp_image_dock_dockbook_changed (GimpDockbook  *dockbook,
                                  GimpDockable  *dockable,
                                  GimpImageDock *dock)
{
  gimp_image_dock_update_title (dock);
}

static gboolean
gimp_image_dock_update_title_idle (GimpImageDock *image_dock)
{
  GString *title;
  GList   *list;

  title = g_string_new (NULL);

  for (list = GIMP_DOCK (image_dock)->dockbooks;
       list;
       list = g_list_next (list))
    {
      GimpDockbook *dockbook = list->data;
      GList        *children;
      GList        *child;

      children = gtk_container_get_children (GTK_CONTAINER (dockbook));

      for (child = children; child; child = g_list_next (child))
        {
          GimpDockable *dockable = child->data;

          g_string_append (title, dockable->name);

          if (g_list_next (child))
            g_string_append (title, ", ");
        }

      g_list_free (children);

      if (g_list_next (list))
        g_string_append (title, " | ");
    }

  gtk_window_set_title (GTK_WINDOW (image_dock), title->str);

  g_string_free (title, TRUE);

  image_dock->update_title_idle_id = 0;

  return FALSE;
}

static void
gimp_image_dock_update_title (GimpImageDock *image_dock)
{
  if (image_dock->update_title_idle_id)
    g_source_remove (image_dock->update_title_idle_id);

  image_dock->update_title_idle_id =
    g_idle_add ((GSourceFunc) gimp_image_dock_update_title_idle,
                image_dock);
}

static void
gimp_image_dock_factory_display_changed (GimpContext *context,
                                         GimpObject  *display,
                                         GimpDock    *dock)
{
  GimpImageDock *image_dock = GIMP_IMAGE_DOCK (dock);

  if (display && image_dock->auto_follow_active)
    gimp_context_set_display (dock->context, display);
}

static void
gimp_image_dock_factory_image_changed (GimpContext *context,
				       GimpImage   *gimage,
				       GimpDock    *dock)
{
  GimpImageDock *image_dock = GIMP_IMAGE_DOCK (dock);

  /*  won't do anything if we already set the display above  */
  if (gimage && image_dock->auto_follow_active)
    gimp_context_set_image (dock->context, gimage);
}

static void
gimp_image_dock_display_changed (GimpContext *context,
                                 GimpObject  *display,
                                 GimpDock    *dock)
{
  GimpImageDock *image_dock = GIMP_IMAGE_DOCK (dock);

  gimp_ui_manager_update (image_dock->ui_manager, display);
}

static void
gimp_image_dock_image_changed (GimpContext *context,
			       GimpImage   *gimage,
			       GimpDock    *dock)
{
  GimpImageDock *image_dock = GIMP_IMAGE_DOCK (dock);

  if (gimage == NULL &&
      gimp_container_num_children (image_dock->image_container) > 0)
    {
      gimage = GIMP_IMAGE (gimp_container_get_child_by_index (image_dock->image_container, 0));

      if (gimage)
	{
	  /*  this invokes this function recursively but we don't enter
	   *  the if() branch the second time
	   */
	  gimp_context_set_image (context, gimage);

	  /*  stop the emission of the original signal (the emission of
	   *  the recursive signal is finished)
	   */
	  g_signal_stop_emission_by_name (context, "image_changed");
	}
    }
  else if (gimage != NULL &&
           gimp_container_num_children (image_dock->display_container) > 0)
    {
      GimpObject *gdisp;
      GimpImage  *gdisp_gimage;
      gboolean    find_display = TRUE;

      gdisp = gimp_context_get_display (context);

      if (gdisp)
        {
          g_object_get (gdisp, "image", &gdisp_gimage, NULL);

          if (gdisp_gimage)
            {
              g_object_unref (gdisp_gimage);

              if (gdisp_gimage == gimage)
                find_display = FALSE;
            }
        }

      if (find_display)
        {
          GList *list;

          for (list = GIMP_LIST (image_dock->display_container)->list;
               list;
               list = g_list_next (list))
            {
              gdisp = GIMP_OBJECT (list->data);

              g_object_get (gdisp, "image", &gdisp_gimage, NULL);

              if (gdisp_gimage)
                {
                  g_object_unref (gdisp_gimage);

                  if (gdisp_gimage == gimage)
                    {
                      /*  this invokes this function recursively but we
                       *  don't enter the if(find_display) branch the
                       *  second time
                       */
                      gimp_context_set_display (context, gdisp);

                      /*  don't stop signal emission here because the
                       *  context's image was not changed by the
                       *  recursive call
                       */
                      break;
                    }
                }
            }
        }
    }
}

static void
gimp_image_dock_auto_clicked (GtkWidget *widget,
			      GimpDock  *dock)
{
  GimpImageDock *image_dock = GIMP_IMAGE_DOCK (dock);

  gimp_toggle_button_update (widget, &image_dock->auto_follow_active);

  if (image_dock->auto_follow_active)
    {
      if (gimp_context_get_display (dock->dialog_factory->context))
        gimp_context_copy_property (dock->dialog_factory->context,
                                    dock->context,
                                    GIMP_CONTEXT_PROP_DISPLAY);
      else
        gimp_context_copy_property (dock->dialog_factory->context,
                                    dock->context,
                                    GIMP_CONTEXT_PROP_IMAGE);
    }
}

static void
gimp_image_dock_image_flush (GimpImage *gimage,
                             GimpDock  *dock)
{
  GimpImageDock *image_dock = GIMP_IMAGE_DOCK (dock);
  GimpImage     *dock_image;

  dock_image = gimp_context_get_image (dock->context);

  if (dock_image == gimage)
    {
      GimpObject *display = gimp_context_get_display (dock->context);

      if (display)
        gimp_ui_manager_update (image_dock->ui_manager, display);
    }
}
