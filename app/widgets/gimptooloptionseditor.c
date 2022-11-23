/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatooloptionseditor.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmalist.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatooloptions.h"

#include "ligmadnd.h"
#include "ligmadocked.h"
#include "ligmahelp-ids.h"
#include "ligmamenufactory.h"
#include "ligmapropwidgets.h"
#include "ligmaview.h"
#include "ligmaviewrenderer.h"
#include "ligmatooloptionseditor.h"
#include "ligmauimanager.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"

enum
{
  PROP_0,
  PROP_LIGMA,
};

struct _LigmaToolOptionsEditorPrivate
{
  Ligma            *ligma;

  GtkWidget       *scrolled_window;
  GtkWidget       *options_vbox;
  GtkWidget       *title_label;

  GtkWidget       *save_button;
  GtkWidget       *restore_button;
  GtkWidget       *delete_button;
  GtkWidget       *reset_button;

  LigmaToolOptions *visible_tool_options;
};


static void        ligma_tool_options_editor_docked_iface_init (LigmaDockedInterface   *iface);
static void        ligma_tool_options_editor_constructed       (GObject               *object);
static void        ligma_tool_options_editor_dispose           (GObject               *object);
static void        ligma_tool_options_editor_set_property      (GObject               *object,
                                                               guint                  property_id,
                                                               const GValue          *value,
                                                               GParamSpec            *pspec);
static void        ligma_tool_options_editor_get_property      (GObject               *object,
                                                               guint                  property_id,
                                                               GValue                *value,
                                                               GParamSpec            *pspec);

static GtkWidget * ligma_tool_options_editor_get_preview       (LigmaDocked            *docked,
                                                               LigmaContext           *context,
                                                               GtkIconSize            size);
static gchar     * ligma_tool_options_editor_get_title         (LigmaDocked            *docked);
static gboolean    ligma_tool_options_editor_get_prefer_icon   (LigmaDocked            *docked);
static void        ligma_tool_options_editor_save_clicked      (GtkWidget             *widget,
                                                               LigmaToolOptionsEditor *editor);
static void        ligma_tool_options_editor_restore_clicked   (GtkWidget             *widget,
                                                               LigmaToolOptionsEditor *editor);
static void        ligma_tool_options_editor_delete_clicked    (GtkWidget             *widget,
                                                               LigmaToolOptionsEditor *editor);
static void        ligma_tool_options_editor_drop_tool         (GtkWidget             *widget,
                                                               gint                   x,
                                                               gint                   y,
                                                               LigmaViewable          *viewable,
                                                               gpointer               data);
static void        ligma_tool_options_editor_tool_changed      (LigmaContext           *context,
                                                               LigmaToolInfo          *tool_info,
                                                               LigmaToolOptionsEditor *editor);
static void        ligma_tool_options_editor_presets_update    (LigmaToolOptionsEditor *editor);


G_DEFINE_TYPE_WITH_CODE (LigmaToolOptionsEditor, ligma_tool_options_editor,
                         LIGMA_TYPE_EDITOR,
                         G_ADD_PRIVATE (LigmaToolOptionsEditor)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_tool_options_editor_docked_iface_init))

#define parent_class ligma_tool_options_editor_parent_class


static void
ligma_tool_options_editor_class_init (LigmaToolOptionsEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_tool_options_editor_constructed;
  object_class->dispose      = ligma_tool_options_editor_dispose;
  object_class->set_property = ligma_tool_options_editor_set_property;
  object_class->get_property = ligma_tool_options_editor_get_property;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_tool_options_editor_docked_iface_init (LigmaDockedInterface *docked_iface)
{
  docked_iface->get_preview     = ligma_tool_options_editor_get_preview;
  docked_iface->get_title       = ligma_tool_options_editor_get_title;
  docked_iface->get_prefer_icon = ligma_tool_options_editor_get_prefer_icon;
}

static void
ligma_tool_options_editor_init (LigmaToolOptionsEditor *editor)
{
  GtkScrolledWindow *scrolled_window;

  editor->p = ligma_tool_options_editor_get_instance_private (editor);

  gtk_widget_set_size_request (GTK_WIDGET (editor), -1, 200);

  ligma_dnd_viewable_dest_add (GTK_WIDGET (editor),
                              LIGMA_TYPE_TOOL_INFO,
                              ligma_tool_options_editor_drop_tool,
                              editor);

  /*  The label containing the tool options title */
  editor->p->title_label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (editor->p->title_label), 0.0);
  ligma_label_set_attributes (GTK_LABEL (editor->p->title_label),
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
  gtk_scrolled_window_set_overlay_scrolling (scrolled_window, FALSE);

  gtk_box_pack_start (GTK_BOX (editor), editor->p->scrolled_window,
                      TRUE, TRUE, 0);
  gtk_widget_show (editor->p->scrolled_window);

  /*  The vbox containing the tool options  */
  editor->p->options_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (scrolled_window), editor->p->options_vbox);
  gtk_widget_show (editor->p->options_vbox);
}

static void
ligma_tool_options_editor_constructed (GObject *object)
{
  LigmaToolOptionsEditor *editor = LIGMA_TOOL_OPTIONS_EDITOR (object);
  LigmaContext           *user_context;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  editor->p->save_button =
    ligma_editor_add_button (LIGMA_EDITOR (editor),
                            LIGMA_ICON_DOCUMENT_SAVE,
                            _("Save Tool Preset..."),
                            LIGMA_HELP_TOOL_OPTIONS_SAVE,
                            G_CALLBACK (ligma_tool_options_editor_save_clicked),
                            NULL,
                            editor);

  editor->p->restore_button =
    ligma_editor_add_button (LIGMA_EDITOR (editor),
                            LIGMA_ICON_DOCUMENT_REVERT,
                            _("Restore Tool Preset..."),
                            LIGMA_HELP_TOOL_OPTIONS_RESTORE,
                            G_CALLBACK (ligma_tool_options_editor_restore_clicked),
                            NULL,
                            editor);

  editor->p->delete_button =
    ligma_editor_add_button (LIGMA_EDITOR (editor),
                            LIGMA_ICON_EDIT_DELETE,
                            _("Delete Tool Preset..."),
                            LIGMA_HELP_TOOL_OPTIONS_DELETE,
                            G_CALLBACK (ligma_tool_options_editor_delete_clicked),
                            NULL,
                            editor);

  editor->p->reset_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor), "tool-options",
                                   "tool-options-reset",
                                   "tool-options-reset-all",
                                   GDK_SHIFT_MASK,
                                   NULL);

  user_context = ligma_get_user_context (editor->p->ligma);

  g_signal_connect_object (user_context, "tool-changed",
                           G_CALLBACK (ligma_tool_options_editor_tool_changed),
                           editor,
                           0);

  ligma_tool_options_editor_tool_changed (user_context,
                                         ligma_context_get_tool (user_context),
                                         editor);
}

static void
ligma_tool_options_editor_dispose (GObject *object)
{
  LigmaToolOptionsEditor *editor = LIGMA_TOOL_OPTIONS_EDITOR (object);

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
ligma_tool_options_editor_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  LigmaToolOptionsEditor *editor = LIGMA_TOOL_OPTIONS_EDITOR (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      editor->p->ligma = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_tool_options_editor_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  LigmaToolOptionsEditor *editor = LIGMA_TOOL_OPTIONS_EDITOR (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, editor->p->ligma);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
ligma_tool_options_editor_get_preview (LigmaDocked   *docked,
                                      LigmaContext  *context,
                                      GtkIconSize   size)
{
  GtkWidget *view;
  gint       width;
  gint       height;

  gtk_icon_size_lookup (size, &width, &height);
  view = ligma_prop_view_new (G_OBJECT (context), "tool", context, height);
  LIGMA_VIEW (view)->renderer->size = -1;
  ligma_view_renderer_set_size_full (LIGMA_VIEW (view)->renderer,
                                    width, height, 0);

  return view;
}

static gchar *
ligma_tool_options_editor_get_title (LigmaDocked *docked)
{
  LigmaToolOptionsEditor *editor = LIGMA_TOOL_OPTIONS_EDITOR (docked);
  LigmaContext           *context;
  LigmaToolInfo          *tool_info;

  context = ligma_get_user_context (editor->p->ligma);

  tool_info = ligma_context_get_tool (context);

  return tool_info ? g_strdup (tool_info->label) : NULL;
}

static gboolean
ligma_tool_options_editor_get_prefer_icon (LigmaDocked *docked)
{
  /* We support get_preview() for tab tyles, but we prefer to show our
   * icon
   */
  return TRUE;
}


/*  public functions  */

GtkWidget *
ligma_tool_options_editor_new (Ligma            *ligma,
                              LigmaMenuFactory *menu_factory)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (LIGMA_TYPE_TOOL_OPTIONS_EDITOR,
                       "ligma",            ligma,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<ToolOptions>",
                       "ui-path",         "/tool-options-popup",
                       NULL);
}

LigmaToolOptions *
ligma_tool_options_editor_get_tool_options (LigmaToolOptionsEditor *editor)
{
  g_return_val_if_fail (LIGMA_IS_TOOL_OPTIONS_EDITOR (editor), NULL);

  return editor->p->visible_tool_options;
}

/*  private functions  */

static void
ligma_tool_options_editor_menu_popup (LigmaToolOptionsEditor *editor,
                                     GtkWidget             *button,
                                     const gchar           *path)
{
  LigmaEditor *ligma_editor = LIGMA_EDITOR (editor);

  ligma_ui_manager_get_widget (ligma_editor_get_ui_manager (ligma_editor),
                              ligma_editor_get_ui_path (ligma_editor));
  ligma_ui_manager_update (ligma_editor_get_ui_manager (ligma_editor),
                          ligma_editor_get_popup_data (ligma_editor));

  ligma_ui_manager_ui_popup_at_widget (ligma_editor_get_ui_manager (ligma_editor),
                                      path,
                                      button,
                                      GDK_GRAVITY_WEST,
                                      GDK_GRAVITY_NORTH_EAST,
                                      NULL,
                                      NULL, NULL);
}

static void
ligma_tool_options_editor_save_clicked (GtkWidget             *widget,
                                       LigmaToolOptionsEditor *editor)
{
  if (gtk_widget_get_sensitive (editor->p->restore_button) /* evil but correct */)
    {
      ligma_tool_options_editor_menu_popup (editor, widget,
                                           "/tool-options-popup/Save");
    }
  else
    {
      ligma_ui_manager_activate_action (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                                       "tool-options",
                                       "tool-options-save-new-preset");
    }
}

static void
ligma_tool_options_editor_restore_clicked (GtkWidget             *widget,
                                          LigmaToolOptionsEditor *editor)
{
  ligma_tool_options_editor_menu_popup (editor, widget,
                                       "/tool-options-popup/Restore");
}

static void
ligma_tool_options_editor_delete_clicked (GtkWidget             *widget,
                                         LigmaToolOptionsEditor *editor)
{
  ligma_tool_options_editor_menu_popup (editor, widget,
                                       "/tool-options-popup/Delete");
}

static void
ligma_tool_options_editor_drop_tool (GtkWidget    *widget,
                                    gint          x,
                                    gint          y,
                                    LigmaViewable *viewable,
                                    gpointer      data)
{
  LigmaToolOptionsEditor *editor = LIGMA_TOOL_OPTIONS_EDITOR (data);
  LigmaContext           *context;

  context = ligma_get_user_context (editor->p->ligma);

  ligma_context_set_tool (context, LIGMA_TOOL_INFO (viewable));
}

static void
ligma_tool_options_editor_tool_changed (LigmaContext           *context,
                                       LigmaToolInfo          *tool_info,
                                       LigmaToolOptionsEditor *editor)
{
  LigmaContainer *presets;
  GtkWidget     *options_gui;

  if (tool_info && tool_info->tool_options == editor->p->visible_tool_options)
    return;

  if (editor->p->visible_tool_options)
    {
      presets = editor->p->visible_tool_options->tool_info->presets;

      if (presets)
        g_signal_handlers_disconnect_by_func (presets,
                                              ligma_tool_options_editor_presets_update,
                                              editor);

      options_gui = ligma_tools_get_tool_options_gui (editor->p->visible_tool_options);

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
                                   G_CALLBACK (ligma_tool_options_editor_presets_update),
                                   G_OBJECT (editor), G_CONNECT_SWAPPED);
          g_signal_connect_object (presets, "remove",
                                   G_CALLBACK (ligma_tool_options_editor_presets_update),
                                   G_OBJECT (editor), G_CONNECT_SWAPPED);
          g_signal_connect_object (presets, "thaw",
                                   G_CALLBACK (ligma_tool_options_editor_presets_update),
                                   G_OBJECT (editor), G_CONNECT_SWAPPED);
        }

      options_gui = ligma_tools_get_tool_options_gui (tool_info->tool_options);

      if (! gtk_widget_get_parent (options_gui))
        gtk_box_pack_start (GTK_BOX (editor->p->options_vbox), options_gui,
                            FALSE, FALSE, 0);

      gtk_widget_show (options_gui);

      editor->p->visible_tool_options = tool_info->tool_options;

      ligma_help_set_help_data (editor->p->scrolled_window, NULL,
                               tool_info->help_id);

      ligma_tool_options_editor_presets_update (editor);
    }

  if (editor->p->title_label != NULL)
    {
      gchar *title;

      title = ligma_docked_get_title (LIGMA_DOCKED (editor));
      gtk_label_set_text (GTK_LABEL (editor->p->title_label), title);
      g_free (title);
    }

  ligma_docked_title_changed (LIGMA_DOCKED (editor));
}

static void
ligma_tool_options_editor_presets_update (LigmaToolOptionsEditor *editor)
{
  LigmaToolInfo *tool_info         = editor->p->visible_tool_options->tool_info;
  gboolean      save_sensitive    = FALSE;
  gboolean      restore_sensitive = FALSE;
  gboolean      delete_sensitive  = FALSE;
  gboolean      reset_sensitive   = FALSE;

  if (tool_info->presets)
    {
      save_sensitive  = TRUE;
      reset_sensitive = TRUE;

      if (! ligma_container_is_empty (tool_info->presets))
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
