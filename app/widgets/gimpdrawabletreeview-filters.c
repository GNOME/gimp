/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawabletreeview-filters.c
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gimphelp-ids.h"
#include "widgets-types.h"
#include "tools/tools-types.h" /* FIXME */

#include "actions/gimpgeglprocedure.h"
#include "actions/filters-commands.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-filters.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimplist.h"
#include "core/gimprasterizable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo-push.h"

#include "tools/tool_manager.h" /* FIXME */

#include "gimpcontainerlistview.h"
#include "gimpcontainerview.h"
#include "gimpdrawabletreeview.h"
#include "gimpdrawabletreeview-filters.h"

#include "gimp-intl.h"


struct _GimpDrawableTreeViewFiltersEditor
{
  GimpDrawable       *drawable;
  GimpDrawableFilter *filter;

  GtkWidget          *popover;
  GtkWidget          *vbox;
  GtkWidget          *view;

  GtkWidget          *options;
  GtkWidget          *visible_button;
  GtkWidget          *edit_button;
  GtkWidget          *raise_button;
  GtkWidget          *lower_button;
  GtkWidget          *merge_button;
  GtkWidget          *remove_button;

  GQuark              notify_temporary_handler;
};


static void   gimp_drawable_filters_editor_set_sensitive
                                              (GimpDrawableTreeView  *view);

static void   gimp_drawable_filters_editor_view_selection_changed
                                              (GimpContainerView     *view,
                                               GimpDrawableTreeView  *drawable_view);
static void   gimp_drawable_filters_editor_view_item_activated
                                              (GtkWidget             *widget,
                                               GimpViewable          *viewable,
                                               GimpDrawableTreeView  *view);

static void   gimp_drawable_filters_editor_temporary_changed
                                              (GimpFilter            *filter,
                                               const GParamSpec      *pspec,
                                               GimpDrawableTreeView  *view);
static void   gimp_drawable_filters_editor_filters_changed
                                              (GimpDrawable          *drawable,
                                               GimpDrawableTreeView  *view);

static void   gimp_drawable_filters_editor_visible_all_toggled
                                              (GtkWidget             *widget,
                                               GimpDrawableTreeView  *view);
static void   gimp_drawable_filters_editor_edit_clicked
                                              (GtkWidget             *widget,
                                               GimpDrawableTreeView  *view);
static void   gimp_drawable_filters_editor_raise_clicked
                                              (GtkWidget             *widget,
                                               GimpDrawableTreeView  *view);
static void   gimp_drawable_filters_editor_lower_clicked
                                              (GtkWidget             *widget,
                                               GimpDrawableTreeView  *view);
static void   gimp_drawable_filters_editor_merge_clicked
                                              (GtkWidget             *widget,
                                               GimpDrawableTreeView  *view);
static void   gimp_drawable_filters_editor_remove_clicked
                                              (GtkWidget             *widget,
                                               GimpDrawableTreeView  *view);


gboolean
_gimp_drawable_tree_view_filter_editor_show (GimpDrawableTreeView *view,
                                             GimpDrawable         *drawable,
                                             GdkRectangle         *rect)
{
  GimpDrawableTreeViewFiltersEditor *editor    = view->editor;
  GimpContainerTreeView             *tree_view = GIMP_CONTAINER_TREE_VIEW (view);
  GimpContainer                     *filters;
  GList                             *list;
  gint                               n_editable;
  gboolean                           visible   = FALSE;

  /* Prevents "duplicate" filter views if you click the popover too fast */
  if (editor && editor->view)
    _gimp_drawable_tree_view_filter_editor_hide (view);

  if (! editor)
    {
      GtkWidget   *vbox;
      GtkWidget   *image;
      GtkWidget   *label;
      gchar       *text;
      GtkIconSize  button_icon_size = GTK_ICON_SIZE_SMALL_TOOLBAR;
      gint         pixel_icon_size  = 16;
      gint         button_spacing;

      view->editor = editor = g_new0 (GimpDrawableTreeViewFiltersEditor, 1);

      gtk_widget_style_get (GTK_WIDGET (view),
                            "button-icon-size", &button_icon_size,
                            "button-spacing",   &button_spacing,
                            NULL);
      gtk_icon_size_lookup (button_icon_size, &pixel_icon_size, NULL);

      /*  filters popover  */
      editor->popover = gtk_popover_new (GTK_WIDGET (tree_view->view));
      gtk_popover_set_modal (GTK_POPOVER (editor->popover), TRUE);

      /*  main vbox  */
      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, button_spacing);
      gtk_container_add (GTK_CONTAINER (editor->popover), vbox);
      gtk_widget_show (vbox);

      gimp_help_connect (vbox, NULL, gimp_standard_help_func,
                         GIMP_HELP_LAYER_EFFECTS, NULL, NULL);

      /*  top label  */
      label = gtk_label_new (NULL);
      text = g_strdup_printf ("<b>%s</b>",
                              _("Layer Effects"));
      gtk_label_set_markup (GTK_LABEL (label), text);
      g_free (text);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      /*  main vbox within  */
      editor->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, button_spacing);
      gtk_box_pack_end (GTK_BOX (vbox), editor->vbox, TRUE, TRUE, 0);
      gtk_widget_show (editor->vbox);

      /*  bottom buttons  */
      editor->options = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, button_spacing);
      gtk_box_pack_end (GTK_BOX (editor->vbox), editor->options, FALSE, FALSE, 0);
      gtk_widget_show (editor->options);

      /*  "visible" button  */
      editor->visible_button = gtk_toggle_button_new ();
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (editor->visible_button),
                                    TRUE);
      image = gtk_image_new_from_icon_name (GIMP_ICON_VISIBLE,
                                            GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (editor->visible_button), image);
      gtk_widget_show (image);

      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->visible_button, TRUE, TRUE, 0);
      gtk_widget_show (editor->visible_button);

      gimp_help_set_help_data (editor->visible_button,
                               _("Toggle the visibility of all filters."),
                               GIMP_HELP_LAYER_EFFECTS);

      g_signal_connect (editor->visible_button, "toggled",
                        G_CALLBACK (gimp_drawable_filters_editor_visible_all_toggled),
                        view);

      /*  "edit" button  */
      editor->edit_button =
        gtk_button_new_from_icon_name (GIMP_ICON_EDIT,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->edit_button, TRUE, TRUE, 0);
      gtk_widget_show (editor->edit_button);

      gimp_help_set_help_data (editor->edit_button,
                               _("Edit the selected filter."),
                               GIMP_HELP_LAYER_EFFECTS);

      g_signal_connect (editor->edit_button, "clicked",
                        G_CALLBACK (gimp_drawable_filters_editor_edit_clicked),
                        view);

      /*  "raise" button  */
      editor->raise_button =
        gtk_button_new_from_icon_name (GIMP_ICON_GO_UP,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->raise_button, TRUE, TRUE, 0);
      gtk_widget_show (editor->raise_button);

      gimp_help_set_help_data (editor->raise_button,
                               _("Raise filter one step up in the stack."),
                               GIMP_HELP_LAYER_EFFECTS);
      g_signal_connect (editor->raise_button, "clicked",
                        G_CALLBACK (gimp_drawable_filters_editor_raise_clicked),
                        view);

      /*  "lower" button  */
      editor->lower_button =
        gtk_button_new_from_icon_name (GIMP_ICON_GO_DOWN,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->lower_button, TRUE, TRUE, 0);
      gtk_widget_show (editor->lower_button);

      gimp_help_set_help_data (editor->lower_button,
                               _("Lower filter one step down in the stack."),
                               GIMP_HELP_LAYER_EFFECTS);

      g_signal_connect (editor->lower_button, "clicked",
                        G_CALLBACK (gimp_drawable_filters_editor_lower_clicked),
                        view);

      /*  "merge" button  */
      editor->merge_button =
        gtk_button_new_from_icon_name (GIMP_ICON_LAYER_MERGE_DOWN,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->merge_button, TRUE, TRUE, 0);
      gtk_widget_show (editor->merge_button);

      gimp_help_set_help_data (editor->merge_button,
                               _("Merge all active filters down."),
                               GIMP_HELP_LAYER_EFFECTS);

      g_signal_connect (editor->merge_button, "clicked",
                        G_CALLBACK (gimp_drawable_filters_editor_merge_clicked),
                        view);

      /*  "remove" button  */
      editor->remove_button =
        gtk_button_new_from_icon_name (GIMP_ICON_EDIT_DELETE,
                                       GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (editor->options),
                          editor->remove_button, TRUE, TRUE, 0);
      gtk_widget_show (editor->remove_button);

      gimp_help_set_help_data (editor->remove_button,
                               _("Remove the selected filter."),
                               GIMP_HELP_LAYER_EFFECTS);

      g_signal_connect (editor->remove_button, "clicked",
                        G_CALLBACK (gimp_drawable_filters_editor_remove_clicked),
                        view);
    }

  filters = gimp_drawable_get_filters (drawable);

  for (list = GIMP_LIST (filters)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      if (GIMP_IS_DRAWABLE_FILTER (list->data))
        {
          if (gimp_filter_get_active (list->data))
            visible = TRUE;
        }
    }

  /* Set the initial value for the effect visibility toggle */
  g_signal_handlers_block_by_func (editor->visible_button,
                                   gimp_drawable_filters_editor_visible_all_toggled,
                                   view);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (editor->visible_button),
                                visible);
  g_signal_handlers_unblock_by_func (editor->visible_button,
                                     gimp_drawable_filters_editor_visible_all_toggled,
                                     view);

  gimp_drawable_n_editable_filters (drawable, &n_editable, NULL, NULL);

  /*  only show if we have at least one editable filter  */
  if (n_editable > 0)
    {
      GtkWidget *scrolled_window;

      editor->drawable = drawable;

      editor->view = gimp_container_list_view_new (filters,
                                                   gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view)),
                                                   GIMP_VIEW_SIZE_SMALL, 0);

      editor->notify_temporary_handler =
        gimp_container_add_handler (filters, "notify::temporary",
                                    G_CALLBACK (gimp_drawable_filters_editor_temporary_changed),
                                    view);

      g_signal_connect (drawable, "filters-changed",
                        G_CALLBACK (gimp_drawable_filters_editor_filters_changed),
                        view);

      g_signal_connect (editor->view, "selection-changed",
                        G_CALLBACK (gimp_drawable_filters_editor_view_selection_changed),
                        view);
      g_signal_connect_object (editor->view, "item-activated",
                               G_CALLBACK (gimp_drawable_filters_editor_view_item_activated),
                               view, 0);

      gtk_box_pack_start (GTK_BOX (editor->vbox), editor->view,
                          TRUE, TRUE, 0);
      gtk_widget_show (editor->view);

      scrolled_window = GIMP_CONTAINER_BOX (editor->view)->scrolled_win;
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                      GTK_POLICY_NEVER,
                                      GTK_POLICY_NEVER);

      gimp_drawable_filters_editor_set_sensitive (view);

      gtk_popover_set_pointing_to (GTK_POPOVER (editor->popover), rect);
      gtk_popover_popup (GTK_POPOVER (editor->popover));

      g_signal_connect_swapped (editor->popover, "closed",
                                G_CALLBACK (_gimp_drawable_tree_view_filter_editor_hide),
                                view);

      return TRUE;
    }

  return FALSE;
}

void
_gimp_drawable_tree_view_filter_editor_hide (GimpDrawableTreeView *view)
{
  GimpDrawableTreeViewFiltersEditor *editor = view->editor;

  if (! editor)
    return;

  if (editor->notify_temporary_handler)
    {
      GimpContainer *filters = gimp_drawable_get_filters (editor->drawable);

      g_signal_handlers_disconnect_by_func (editor->popover,
                                            _gimp_drawable_tree_view_filter_editor_hide,
                                            view);

      g_signal_handlers_disconnect_by_func (editor->drawable,
                                            gimp_drawable_filters_editor_filters_changed,
                                            view);

      gimp_container_remove_handler (filters,
                                     editor->notify_temporary_handler);
      editor->notify_temporary_handler = 0;
    }

  gtk_popover_popdown (GTK_POPOVER (editor->popover));

  if (editor->view)
    {
      g_signal_handlers_disconnect_by_func (editor->view,
                                            gimp_drawable_filters_editor_view_selection_changed,
                                            view);
      g_signal_handlers_disconnect_by_func (editor->view,
                                            gimp_drawable_filters_editor_view_item_activated,
                                            view);

      g_clear_pointer (&editor->view, gtk_widget_destroy);
    }

  editor->drawable = NULL;
  editor->filter   = NULL;
}

void
_gimp_drawable_tree_view_filter_editor_destroy (GimpDrawableTreeView *view)
{
  GimpDrawableTreeViewFiltersEditor *editor = view->editor;

  if (! editor)
    return;

  _gimp_drawable_tree_view_filter_editor_hide (view);

  g_clear_pointer (&editor->popover, gtk_widget_destroy);
  g_clear_pointer (&view->editor, g_free);
}

static void
gimp_drawable_filters_editor_set_sensitive (GimpDrawableTreeView *view)
{
  GimpDrawableTreeViewFiltersEditor *editor= view->editor;
  gboolean                           editable;
  gint                               n_editable;
  gint                               first_editable;
  gint                               last_editable;

  editable = gimp_drawable_n_editable_filters (editor->drawable,
                                               &n_editable,
                                               &first_editable,
                                               &last_editable);

  /*  lock everything if none is editable  */
  gtk_widget_set_sensitive (editor->vbox, editable);
  if (! editable)
    return;

  gtk_widget_set_sensitive (editor->options,
                            GIMP_IS_DRAWABLE_FILTER (editor->filter));

  if (GIMP_IS_DRAWABLE_FILTER (editor->filter))
    {
      GimpContainer *filters;
      gboolean       is_group    = FALSE;
      gboolean       is_editable = FALSE;
      gint           index;

      filters = gimp_drawable_get_filters (editor->drawable);

      index = gimp_container_get_child_index (filters,
                                              GIMP_OBJECT (editor->filter));

      /*  do not allow merging down effects on group layers  */
      if (gimp_viewable_get_children (GIMP_VIEWABLE (editor->drawable)))
        is_group = TRUE;

      is_editable = (index >= first_editable &&
                     index <= last_editable);

      gtk_widget_set_sensitive (editor->visible_button,
                                TRUE);
      gtk_widget_set_sensitive (editor->edit_button,
                                is_editable);
      gtk_widget_set_sensitive (editor->raise_button,
                                index > first_editable);
      gtk_widget_set_sensitive (editor->lower_button,
                                index < last_editable);
      gtk_widget_set_sensitive (editor->merge_button,
                                ! is_group &&
                                (! GIMP_IS_RASTERIZABLE (editor->drawable) ||
                                 gimp_rasterizable_is_rasterized (GIMP_RASTERIZABLE (editor->drawable))));
      gtk_widget_set_sensitive (editor->remove_button,
                                is_editable);
    }
}

static void
gimp_drawable_filters_editor_view_selection_changed (GimpContainerView    *view,
                                                     GimpDrawableTreeView *drawable_view)
{
  GimpDrawableTreeViewFiltersEditor *editor = drawable_view->editor;
  GimpViewable                      *viewable;

  viewable = gimp_container_view_get_1_selected (view);

  editor->filter = NULL;

  if (viewable)
    {
      /* Don't set floating selection as active filter */
      if (GIMP_IS_DRAWABLE_FILTER (viewable))
        {
          editor->filter = GIMP_DRAWABLE_FILTER (viewable);
        }
    }

  gimp_drawable_filters_editor_set_sensitive (drawable_view);
}

static void
gimp_drawable_filters_editor_view_item_activated (GtkWidget            *widget,
                                                  GimpViewable         *viewable,
                                                  GimpDrawableTreeView *view)
{
  GimpDrawableTreeViewFiltersEditor *editor = view->editor;

  if (gtk_widget_is_sensitive (editor->edit_button))
    gimp_drawable_filters_editor_edit_clicked (widget, view);
}

static void
gimp_drawable_filters_editor_temporary_changed (GimpFilter           *filter,
                                                const GParamSpec     *pspec,
                                                GimpDrawableTreeView *view)
{
  gimp_drawable_filters_editor_set_sensitive (view);
}

static void
gimp_drawable_filters_editor_filters_changed (GimpDrawable         *drawable,
                                              GimpDrawableTreeView *view)
{
  GimpContainer *filters;
  gint           n_filters;
  gint           height;

  filters   = gimp_drawable_get_filters (drawable);
  n_filters = gimp_container_get_n_children (filters);

  height = gimp_container_view_get_view_size (GIMP_CONTAINER_VIEW (view->editor->view), NULL);

  /*  it doesn't get any more hackish  */
  gtk_widget_set_size_request (view->editor->view,
                               -1, (height + 6) * n_filters);

  gimp_drawable_filters_editor_set_sensitive (view);
}

static void
gimp_drawable_filters_editor_visible_all_toggled (GtkWidget            *widget,
                                                  GimpDrawableTreeView *view)
{
  GimpDrawableTreeViewFiltersEditor *editor = view->editor;
  GimpContainer                     *filters;
  GList                             *list;
  gboolean                           visible;

  visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  filters = gimp_drawable_get_filters (editor->drawable);

  for (list = GIMP_LIST (filters)->queue->head;
       list;
       list = g_list_next (list))
    {
      if (GIMP_IS_DRAWABLE_FILTER (list->data))
        {
          GimpFilter *filter = list->data;

          gimp_filter_set_active (filter, visible);
        }
    }

  gimp_drawable_update (editor->drawable, 0, 0, -1, -1);
  gimp_image_flush (gimp_item_get_image (GIMP_ITEM (editor->drawable)));
}

static void
gimp_drawable_filters_editor_edit_clicked (GtkWidget            *widget,
                                           GimpDrawableTreeView *view)
{
  GimpDrawableTreeViewFiltersEditor *editor = view->editor;
  GimpImage                         *image;
  GeglNode                          *op;

  if (! GIMP_IS_DRAWABLE_FILTER (editor->filter))
    return;

  image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));

  if (! gimp_item_is_visible (GIMP_ITEM (editor->drawable)) &&
      ! GIMP_GUI_CONFIG (image->gimp->config)->edit_non_visible)
    {
      gimp_message_literal (image->gimp, G_OBJECT (view),
                            GIMP_MESSAGE_ERROR,
                            _("A selected layer is not visible."));
      return;
    }
  else if (gimp_item_get_lock_content (GIMP_ITEM (editor->drawable)))
    {
      gimp_message_literal (image->gimp, G_OBJECT (view),
                            GIMP_MESSAGE_WARNING,
                            _("A selected layer's pixels are locked."));
      return;
    }

  op = gimp_drawable_filter_get_operation (editor->filter);
  if (op)
    {
      GimpProcedure *procedure;
      GVariant      *variant;
      gchar         *operation;
      gchar         *name;

      g_object_get (editor->filter,
                    "name", &name,
                    NULL);
      g_object_get (op,
                    "operation", &operation,
                    NULL);

      if (operation)
        {
          procedure = gimp_gegl_procedure_new (image->gimp,
                                               editor->filter,
                                               GIMP_RUN_INTERACTIVE, NULL,
                                               operation,
                                               name,
                                               name,
                                               NULL, NULL, NULL);

          variant = g_variant_new_uint64 (GPOINTER_TO_SIZE (procedure));
          g_variant_take_ref (variant);
          filters_run_procedure (image->gimp,
                                 gimp_context_get_display (gimp_get_user_context (image->gimp)),
                                 procedure, GIMP_RUN_INTERACTIVE);

          g_variant_unref (variant);
          g_object_unref (procedure);
        }

      g_free (name);
      g_free (operation);
    }
}

static void
gimp_drawable_filters_editor_raise_clicked (GtkWidget            *widget,
                                            GimpDrawableTreeView *view)
{
  GimpDrawableTreeViewFiltersEditor *editor = view->editor;
  GimpImage                         *image;

  if (! GIMP_IS_DRAWABLE_FILTER (editor->filter))
    return;

  image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));

  if (gimp_drawable_filter_get_mask (editor->filter) == NULL)
    {
      gimp_message_literal (image->gimp, G_OBJECT (view), GIMP_MESSAGE_ERROR,
                            _("Cannot reorder a filter that is being edited."));
      return;
    }

  if (gimp_drawable_raise_filter (editor->drawable,
                                  GIMP_FILTER (editor->filter)))
    {
      gimp_image_flush (image);
    }
}

static void
gimp_drawable_filters_editor_lower_clicked (GtkWidget            *widget,
                                            GimpDrawableTreeView *view)
{
  GimpDrawableTreeViewFiltersEditor *editor = view->editor;
  GimpImage                         *image;

  if (! GIMP_IS_DRAWABLE_FILTER (editor->filter))
    return;

  image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));

  if (gimp_drawable_filter_get_mask (editor->filter) == NULL)
    {
      gimp_message_literal (image->gimp, G_OBJECT (view), GIMP_MESSAGE_ERROR,
                            _("Cannot reorder a filter that is being edited."));
      return;
    }

  if (gimp_drawable_lower_filter (editor->drawable,
                                  GIMP_FILTER (editor->filter)))
    {
      gimp_image_flush (image);
    }
}

static void
gimp_drawable_filters_editor_merge_clicked (GtkWidget            *widget,
                                            GimpDrawableTreeView *view)
{
  GimpDrawableTreeViewFiltersEditor *editor = view->editor;
  GimpContext                       *context;
  GimpToolInfo                      *active_tool;

  if (! GIMP_IS_DRAWABLE_FILTER (editor->filter))
    return;

  /* Commit GEGL-based tools before trying to merge filters */
  context     = gimp_container_view_get_context (GIMP_CONTAINER_VIEW (view));
  active_tool = gimp_context_get_tool (context);

  if (! strcmp (gimp_object_get_name (active_tool), "gimp-cage-tool")     ||
      ! strcmp (gimp_object_get_name (active_tool), "gimp-gradient-tool") ||
      ! strcmp (gimp_object_get_name (active_tool), "gimp-warp-tool"))
    {
      tool_manager_control_active (context->gimp, GIMP_TOOL_ACTION_COMMIT,
                                   gimp_context_get_display (context));
    }

  if (! gimp_viewable_get_children (GIMP_VIEWABLE (editor->drawable)))
    {
      GimpImage *image = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));

      /* Don't merge if the layer is currently locked */
      if (gimp_item_get_lock_content (GIMP_ITEM (editor->drawable)))
        {
          gimp_message_literal (image->gimp, G_OBJECT (view),
                                GIMP_MESSAGE_WARNING,
                                _("The layer to merge down to is locked."));
          return;
        }

      gimp_drawable_merge_filters (GIMP_DRAWABLE (editor->drawable));

      _gimp_drawable_tree_view_filter_editor_hide (view);

      gimp_image_flush (image);
    }
}

static void
gimp_drawable_filters_editor_remove_clicked (GtkWidget            *widget,
                                             GimpDrawableTreeView *view)
{
  GimpDrawableTreeViewFiltersEditor *editor = view->editor;
  GimpContainer                     *filters;
  GimpImage                         *image;

  if (! GIMP_IS_DRAWABLE_FILTER (editor->filter))
    return;

  filters = gimp_drawable_get_filters (editor->drawable);
  image   = gimp_item_tree_view_get_image (GIMP_ITEM_TREE_VIEW (view));

  gimp_image_undo_push_filter_remove (image, _("Remove filter"),
                                      editor->drawable,
                                      editor->filter);

  gimp_drawable_filter_abort (editor->filter);

  /*  close the popover if all effects are deleted  */
  if (gimp_container_get_n_children (filters) == 0)
    _gimp_drawable_tree_view_filter_editor_hide (view);

  gimp_image_flush (image);
}
