/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptooloptionseditor.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimphelp-ids.h"
#include "gimpmenufactory.h"
#include "gimppropwidgets.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"
#include "gimptooloptionseditor.h"
#include "gimpuimanager.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"

enum
{
  PROP_0,
  PROP_GIMP,
};

struct _GimpToolOptionsEditorPrivate
{
  Gimp            *gimp;

  GtkWidget       *scrolled_window;
  GtkWidget       *options_vbox;
  GtkWidget       *title_label;

  GtkWidget       *save_button;
  GtkWidget       *restore_button;
  GtkWidget       *delete_button;
  GtkWidget       *reset_button;

  GimpToolOptions *visible_tool_options;
};


static void        gimp_tool_options_editor_docked_iface_init (GimpDockedInterface   *iface);
static void        gimp_tool_options_editor_constructed       (GObject               *object);
static void        gimp_tool_options_editor_dispose           (GObject               *object);
static void        gimp_tool_options_editor_set_property      (GObject               *object,
                                                               guint                  property_id,
                                                               const GValue          *value,
                                                               GParamSpec            *pspec);
static void        gimp_tool_options_editor_get_property      (GObject               *object,
                                                               guint                  property_id,
                                                               GValue                *value,
                                                               GParamSpec            *pspec);

static GtkWidget * gimp_tool_options_editor_get_preview       (GimpDocked            *docked,
                                                               GimpContext           *context,
                                                               GtkIconSize            size);
static gchar     * gimp_tool_options_editor_get_title         (GimpDocked            *docked);
static gboolean    gimp_tool_options_editor_get_prefer_icon   (GimpDocked            *docked);
static void        gimp_tool_options_editor_save_clicked      (GtkWidget             *widget,
                                                               GimpToolOptionsEditor *editor);
static void        gimp_tool_options_editor_restore_clicked   (GtkWidget             *widget,
                                                               GimpToolOptionsEditor *editor);
static void        gimp_tool_options_editor_delete_clicked    (GtkWidget             *widget,
                                                               GimpToolOptionsEditor *editor);
static void        gimp_tool_options_editor_drop_tool         (GtkWidget             *widget,
                                                               gint                   x,
                                                               gint                   y,
                                                               GimpViewable          *viewable,
                                                               gpointer               data);
static void        gimp_tool_options_editor_tool_changed      (GimpContext           *context,
                                                               GimpToolInfo          *tool_info,
                                                               GimpToolOptionsEditor *editor);
static void        gimp_tool_options_editor_presets_update    (GimpToolOptionsEditor *editor);


G_DEFINE_TYPE_WITH_CODE (GimpToolOptionsEditor, gimp_tool_options_editor,
                         GIMP_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_tool_options_editor_docked_iface_init))

#define parent_class gimp_tool_options_editor_parent_class


static void
gimp_tool_options_editor_class_init (GimpToolOptionsEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_tool_options_editor_constructed;
  object_class->dispose      = gimp_tool_options_editor_dispose;
  object_class->set_property = gimp_tool_options_editor_set_property;
  object_class->get_property = gimp_tool_options_editor_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpToolOptionsEditorPrivate));
}

static void
gimp_tool_options_editor_docked_iface_init (GimpDockedInterface *docked_iface)
{
  docked_iface->get_preview     = gimp_tool_options_editor_get_preview;
  docked_iface->get_title       = gimp_tool_options_editor_get_title;
  docked_iface->get_prefer_icon = gimp_tool_options_editor_get_prefer_icon;
}

static void
gimp_tool_options_editor_init (GimpToolOptionsEditor *editor)
{
  GtkScrolledWindow *scrolled_window;
  GtkWidget         *viewport;

  editor->p = G_TYPE_INSTANCE_GET_PRIVATE (editor,
                                           GIMP_TYPE_TOOL_OPTIONS_EDITOR,
                                           GimpToolOptionsEditorPrivate);

  gtk_widget_set_size_request (GTK_WIDGET (editor), -1, 200);

  gimp_dnd_viewable_dest_add (GTK_WIDGET (editor),
                              GIMP_TYPE_TOOL_INFO,
                              gimp_tool_options_editor_drop_tool,
                              editor);

  /*  The label containing the tool options title */
  editor->p->title_label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (editor->p->title_label), 0.0);
  gimp_label_set_attributes (GTK_LABEL (editor->p->title_label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (editor), editor->p->title_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->p->title_label);

  editor->p->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  scrolled_window = GTK_SCROLLED_WINDOW (editor->p->scrolled_window);

  gtk_scrolled_window_set_policy (scrolled_window,
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (editor), editor->p->scrolled_window,
                      TRUE, TRUE, 0);
  gtk_widget_show (editor->p->scrolled_window);

  viewport = gtk_viewport_new (gtk_scrolled_window_get_hadjustment (scrolled_window),
                               gtk_scrolled_window_get_vadjustment (scrolled_window));
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport);
  gtk_widget_show (viewport);

  /*  The vbox containing the tool options  */
  editor->p->options_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (viewport), editor->p->options_vbox);
  gtk_widget_show (editor->p->options_vbox);
}

static void
gimp_tool_options_editor_constructed (GObject *object)
{
  GimpToolOptionsEditor *editor = GIMP_TOOL_OPTIONS_EDITOR (object);
  GimpContext           *user_context;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  editor->p->save_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GIMP_ICON_DOCUMENT_SAVE,
                            _("Save Tool Preset..."),
                            GIMP_HELP_TOOL_OPTIONS_SAVE,
                            G_CALLBACK (gimp_tool_options_editor_save_clicked),
                            NULL,
                            editor);

  editor->p->restore_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GIMP_ICON_DOCUMENT_REVERT,
                            _("Restore Tool Preset..."),
                            GIMP_HELP_TOOL_OPTIONS_RESTORE,
                            G_CALLBACK (gimp_tool_options_editor_restore_clicked),
                            NULL,
                            editor);

  editor->p->delete_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GIMP_ICON_EDIT_DELETE,
                            _("Delete Tool Preset..."),
                            GIMP_HELP_TOOL_OPTIONS_DELETE,
                            G_CALLBACK (gimp_tool_options_editor_delete_clicked),
                            NULL,
                            editor);

  editor->p->reset_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "tool-options",
                                   "tool-options-reset",
                                   "tool-options-reset-all",
                                   GDK_SHIFT_MASK,
                                   NULL);

  user_context = gimp_get_user_context (editor->p->gimp);

  g_signal_connect_object (user_context, "tool-changed",
                           G_CALLBACK (gimp_tool_options_editor_tool_changed),
                           editor,
                           0);

  gimp_tool_options_editor_tool_changed (user_context,
                                         gimp_context_get_tool (user_context),
                                         editor);
}

static void
gimp_tool_options_editor_dispose (GObject *object)
{
  GimpToolOptionsEditor *editor = GIMP_TOOL_OPTIONS_EDITOR (object);

  if (editor->p->options_vbox)
    {
      GList *options;
      GList *list;

      options =
        gtk_container_get_children (GTK_CONTAINER (editor->p->options_vbox));

      for (list = options; list; list = g_list_next (list))
        {
          gtk_container_remove (GTK_CONTAINER (editor->p->options_vbox),
                                GTK_WIDGET (list->data));
        }

      g_list_free (options);
      editor->p->options_vbox = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tool_options_editor_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpToolOptionsEditor *editor = GIMP_TOOL_OPTIONS_EDITOR (object);

  switch (property_id)
    {
    case PROP_GIMP:
      editor->p->gimp = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_options_editor_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpToolOptionsEditor *editor = GIMP_TOOL_OPTIONS_EDITOR (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, editor->p->gimp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
gimp_tool_options_editor_get_preview (GimpDocked   *docked,
                                      GimpContext  *context,
                                      GtkIconSize   size)
{
  GtkWidget *view;
  gint       width;
  gint       height;

  gtk_icon_size_lookup (size, &width, &height);
  view = gimp_prop_view_new (G_OBJECT (context), "tool", context, height);
  GIMP_VIEW (view)->renderer->size = -1;
  gimp_view_renderer_set_size_full (GIMP_VIEW (view)->renderer,
                                    width, height, 0);

  return view;
}

static gchar *
gimp_tool_options_editor_get_title (GimpDocked *docked)
{
  GimpToolOptionsEditor *editor = GIMP_TOOL_OPTIONS_EDITOR (docked);
  GimpContext           *context;
  GimpToolInfo          *tool_info;

  context = gimp_get_user_context (editor->p->gimp);

  tool_info = gimp_context_get_tool (context);

  return tool_info ? g_strdup (tool_info->label) : NULL;
}

static gboolean
gimp_tool_options_editor_get_prefer_icon (GimpDocked *docked)
{
  /* We support get_preview() for tab tyles, but we prefer to show our
   * icon
   */
  return TRUE;
}


/*  public functions  */

GtkWidget *
gimp_tool_options_editor_new (Gimp            *gimp,
                              GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (GIMP_TYPE_TOOL_OPTIONS_EDITOR,
                       "gimp",            gimp,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<ToolOptions>",
                       "ui-path",         "/tool-options-popup",
                       NULL);
}

GimpToolOptions *
gimp_tool_options_editor_get_tool_options (GimpToolOptionsEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS_EDITOR (editor), NULL);

  return editor->p->visible_tool_options;
}

/*  private functions  */

static void
gimp_tool_options_editor_menu_pos (GtkMenu  *menu,
                                   gint     *x,
                                   gint     *y,
                                   gpointer  data)
{
  gimp_button_menu_position (GTK_WIDGET (data), menu, GTK_POS_RIGHT, x, y);
}

static void
gimp_tool_options_editor_menu_popup (GimpToolOptionsEditor *editor,
                                     GtkWidget             *button,
                                     const gchar           *path)
{
  GimpEditor *gimp_editor = GIMP_EDITOR (editor);

  gtk_ui_manager_get_widget (GTK_UI_MANAGER (gimp_editor_get_ui_manager (gimp_editor)),
                             gimp_editor_get_ui_path (gimp_editor));
  gimp_ui_manager_update (gimp_editor_get_ui_manager (gimp_editor),
                          gimp_editor_get_popup_data (gimp_editor));

  gimp_ui_manager_ui_popup (gimp_editor_get_ui_manager (gimp_editor), path,
                            button,
                            gimp_tool_options_editor_menu_pos, button,
                            NULL, NULL);
}

static void
gimp_tool_options_editor_save_clicked (GtkWidget             *widget,
                                       GimpToolOptionsEditor *editor)
{
  if (gtk_widget_get_sensitive (editor->p->restore_button) /* evil but correct */)
    {
      gimp_tool_options_editor_menu_popup (editor, widget,
                                           "/tool-options-popup/Save");
    }
  else
    {
      gimp_ui_manager_activate_action (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                                       "tool-options",
                                       "tool-options-save-new-preset");
    }
}

static void
gimp_tool_options_editor_restore_clicked (GtkWidget             *widget,
                                          GimpToolOptionsEditor *editor)
{
  gimp_tool_options_editor_menu_popup (editor, widget,
                                       "/tool-options-popup/Restore");
}

static void
gimp_tool_options_editor_delete_clicked (GtkWidget             *widget,
                                         GimpToolOptionsEditor *editor)
{
  gimp_tool_options_editor_menu_popup (editor, widget,
                                       "/tool-options-popup/Delete");
}

static void
gimp_tool_options_editor_drop_tool (GtkWidget    *widget,
                                    gint          x,
                                    gint          y,
                                    GimpViewable *viewable,
                                    gpointer      data)
{
  GimpToolOptionsEditor *editor = GIMP_TOOL_OPTIONS_EDITOR (data);
  GimpContext           *context;

  context = gimp_get_user_context (editor->p->gimp);

  gimp_context_set_tool (context, GIMP_TOOL_INFO (viewable));
}

static void
gimp_tool_options_editor_tool_changed (GimpContext           *context,
                                       GimpToolInfo          *tool_info,
                                       GimpToolOptionsEditor *editor)
{
  GimpContainer *presets;
  GtkWidget     *options_gui;

  if (tool_info && tool_info->tool_options == editor->p->visible_tool_options)
    return;

  if (editor->p->visible_tool_options)
    {
      presets = editor->p->visible_tool_options->tool_info->presets;

      if (presets)
        g_signal_handlers_disconnect_by_func (presets,
                                              gimp_tool_options_editor_presets_update,
                                              editor);

      options_gui = gimp_tools_get_tool_options_gui (editor->p->visible_tool_options);

      if (options_gui)
        gtk_widget_hide (options_gui);

      editor->p->visible_tool_options = NULL;
    }

  if (tool_info && tool_info->tool_options)
    {
      presets = tool_info->presets;

      if (presets)
        {
          g_signal_connect_object (presets, "add",
                                   G_CALLBACK (gimp_tool_options_editor_presets_update),
                                   G_OBJECT (editor), G_CONNECT_SWAPPED);
          g_signal_connect_object (presets, "remove",
                                   G_CALLBACK (gimp_tool_options_editor_presets_update),
                                   G_OBJECT (editor), G_CONNECT_SWAPPED);
          g_signal_connect_object (presets, "thaw",
                                   G_CALLBACK (gimp_tool_options_editor_presets_update),
                                   G_OBJECT (editor), G_CONNECT_SWAPPED);
        }

      options_gui = gimp_tools_get_tool_options_gui (tool_info->tool_options);

      if (! gtk_widget_get_parent (options_gui))
        gtk_box_pack_start (GTK_BOX (editor->p->options_vbox), options_gui,
                            FALSE, FALSE, 0);

      gtk_widget_show (options_gui);

      editor->p->visible_tool_options = tool_info->tool_options;

      gimp_help_set_help_data (editor->p->scrolled_window, NULL,
                               tool_info->help_id);

      gimp_tool_options_editor_presets_update (editor);
    }

  if (editor->p->title_label != NULL)
    {
      gchar *title;

      title = gimp_docked_get_title (GIMP_DOCKED (editor));
      gtk_label_set_text (GTK_LABEL (editor->p->title_label), title);
      g_free (title);
    }

  gimp_docked_title_changed (GIMP_DOCKED (editor));
}

static void
gimp_tool_options_editor_presets_update (GimpToolOptionsEditor *editor)
{
  GimpToolInfo *tool_info         = editor->p->visible_tool_options->tool_info;
  gboolean      save_sensitive    = FALSE;
  gboolean      restore_sensitive = FALSE;
  gboolean      delete_sensitive  = FALSE;
  gboolean      reset_sensitive   = FALSE;

  if (tool_info->presets)
    {
      save_sensitive  = TRUE;
      reset_sensitive = TRUE;

      if (! gimp_container_is_empty (tool_info->presets))
        {
          restore_sensitive = TRUE;
          delete_sensitive  = TRUE;
        }
    }

  gtk_widget_set_sensitive (editor->p->save_button,    save_sensitive);
  gtk_widget_set_sensitive (editor->p->restore_button, restore_sensitive);
  gtk_widget_set_sensitive (editor->p->delete_button,  delete_sensitive);
  gtk_widget_set_sensitive (editor->p->reset_button,   reset_sensitive);
}
