/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC     "plug-in-unit-editor"
#define PLUG_IN_BINARY   "unit-editor"
#define PLUG_IN_ROLE     "gimp-unit-editor"

enum
{
  SAVE,
  NAME,
  FACTOR,
  DIGITS,
  SYMBOL,
  ABBREVIATION,
  UNIT,
  USER_UNIT,
  NUM_COLUMNS
};

typedef struct
{
  const gchar *title;
  const gchar *help;

} UnitColumn;


#define GIMP_UNIT_EDITOR_TYPE  (gimp_unit_editor_get_type ())
G_DECLARE_FINAL_TYPE (GimpUnitEditor, gimp_unit_editor, GIMP, UNIT_EDITOR, GimpPlugIn)

struct _GimpUnitEditor
{
  GimpPlugIn      parent_instance;

  GtkApplication *app;

  GtkWindow      *window;
  GtkWidget      *tv;
};


static GList          * editor_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * editor_create_procedure (GimpPlugIn           *plug_in,
                                                 const gchar          *name);

static GimpValueArray * editor_run              (GimpProcedure        *procedure,
                                                 GimpProcedureConfig  *config,
                                                 gpointer              run_data);

static GimpUnit       * new_unit_dialog         (GtkWindow             *main_window,
                                                 GimpUnit              *template);
static void     on_app_activate                 (GApplication          *gapp,
                                                 gpointer               user_data);

static gboolean unit_editor_key_press_event     (GtkWidget            *window,
                                                 GdkEventKey          *event,
                                                 gpointer              user_data);
static void     unit_editor_help_clicked        (GtkWidget            *window);

static void     new_unit_action                 (GSimpleAction         *action,
                                                 GVariant              *param,
                                                 gpointer               user_data);
static void     duplicate_unit_action           (GSimpleAction         *action,
                                                 GVariant              *param,
                                                 gpointer               user_data);
static void     refresh_action                  (GSimpleAction         *action,
                                                 GVariant              *param,
                                                 gpointer               user_data);
static void     saved_toggled_callback          (GtkCellRendererToggle *celltoggle,
                                                 gchar                 *path_string,
                                                 GtkListStore          *list_store);
static void     unit_list_init                  (GtkTreeView           *tv);


G_DEFINE_TYPE (GimpUnitEditor, gimp_unit_editor, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIMP_UNIT_EDITOR_TYPE)
DEFINE_STD_SET_I18N


static const UnitColumn columns[] =
{
  { N_("Saved"),        N_("A unit definition will only be saved before "
                           "GIMP exits if this column is checked.")         },
  { N_("Name"),         N_("The name to be used to identify this unit in "
                           "the graphical interface")                       },
  { N_("Factor"),       N_("How many units make up an inch.")               },
  { N_("Digits"),       N_("This field is a hint for numerical input "
                           "fields. It specifies how many decimal digits "
                           "the input field should provide to get "
                           "approximately the same accuracy as an "
                           "\"inch\" input field with two decimal digits.") },
  { N_("Symbol"),       N_("The unit's symbol if it has one (e.g. \" "
                           "for inches). The unit's abbreviation is used "
                           "if doesn't have a symbol.")                     },
  { N_("Abbreviation"), N_("The unit's abbreviation (e.g. \"cm\" for "
                           "centimeters).")                                 },
};

static GActionEntry ACTIONS[] =
{
  { "new-unit", new_unit_action },
  { "duplicate-unit", duplicate_unit_action },
  { "refresh", refresh_action },
};


static void
gimp_unit_editor_class_init (GimpUnitEditorClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = editor_query_procedures;
  plug_in_class->create_procedure = editor_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gimp_unit_editor_init (GimpUnitEditor *self)
{
}

static GList *
editor_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
editor_create_procedure (GimpPlugIn  *plug_in,
                         const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      editor_run, plug_in, NULL);

      gimp_procedure_set_menu_label (procedure, _("U_nits"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_TOOL_MEASURE);
      gimp_procedure_add_menu_path (procedure, "<Image>/Edit/[Preferences]");

      gimp_procedure_set_documentation (procedure,
                                        _("Create or alter units used in GIMP"),
                                        "The GIMP unit editor",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Michael Natterer <mitch@gimp.org>",
                                      "Michael Natterer <mitch@gimp.org>",
                                      "2000");

      gimp_procedure_add_enum_argument (procedure, "run-mode",
                                       "Run mode",
                                       "The run mode",
                                       GIMP_TYPE_RUN_MODE,
                                       GIMP_RUN_INTERACTIVE,
                                       G_PARAM_READWRITE);
    }

  return procedure;
}

static void
on_app_activate (GApplication *gapp, gpointer user_data)
{
  GimpUnitEditor    *self = GIMP_UNIT_EDITOR (user_data);
  GtkWidget         *headerbar;
  GtkWidget         *vbox;
  GtkWidget         *button_box;
  GtkWidget         *scrolled_win;
  GtkListStore      *list_store;
  GtkTreeViewColumn *col;
  GtkWidget         *col_widget;
  GtkWidget         *button;
  GtkCellRenderer   *rend;

  list_store = gtk_list_store_new (NUM_COLUMNS,
                                   G_TYPE_BOOLEAN,   /*  SAVE          */
                                   G_TYPE_STRING,    /*  NAME          */
                                   G_TYPE_DOUBLE,    /*  FACTOR        */
                                   G_TYPE_INT,       /*  DIGITS        */
                                   G_TYPE_STRING,    /*  SYMBOL        */
                                   G_TYPE_STRING,    /*  ABBREVIATION  */
                                   G_TYPE_OBJECT,    /*  UNIT          */
                                   G_TYPE_BOOLEAN);  /*  USER_UNIT     */

  self->tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  g_object_unref (list_store);

  self->window = GTK_WINDOW (gtk_application_window_new (self->app));
  gtk_window_set_title (self->window, _("Unit Editor"));
  gtk_window_set_role (self->window, PLUG_IN_ROLE);
  gimp_help_connect (GTK_WIDGET (self->window), NULL,
                     gimp_standard_help_func, PLUG_IN_PROC,
                     self->window, NULL);

  /* Actions */
  g_action_map_add_action_entries (G_ACTION_MAP (self->window),
                                   ACTIONS, G_N_ELEMENTS (ACTIONS),
                                   self);
  gtk_application_set_accels_for_action (self->app, "win.new-unit",
                                         (const char*[]) { "<Ctrl>N", NULL });
  gtk_application_set_accels_for_action (self->app, "win.duplicate-unit",
                                         (const char*[]) { "<ctrl>D", NULL });
  gtk_application_set_accels_for_action (self->app, "win.refresh",
                                         (const char*[]) { "<ctrl>R", NULL });

  /* Titlebar */
  headerbar = gtk_header_bar_new ();
  gtk_header_bar_set_title (GTK_HEADER_BAR (headerbar), _("Unit Editor"));
  gtk_header_bar_set_has_subtitle (GTK_HEADER_BAR (headerbar), FALSE);
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (headerbar), TRUE);

  button = gtk_button_new_with_mnemonic (_("_Refresh"));
  gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.refresh");
  gtk_widget_show (button);
  gtk_header_bar_pack_start (GTK_HEADER_BAR (headerbar), button);

  if (gimp_show_help_button ())
    {
      button = gtk_button_new_with_mnemonic (_("_Help"));
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (unit_editor_help_clicked),
                                self->window);
      gtk_widget_show (button);
      gtk_header_bar_pack_start (GTK_HEADER_BAR (headerbar), button);
    }

  button = gtk_button_new_with_mnemonic (_("_OK"));
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gtk_widget_destroy),
                            self->window);
  gtk_widget_show (button);
  gtk_header_bar_pack_end (GTK_HEADER_BAR (headerbar), button);

  gtk_window_set_titlebar (self->window, headerbar);
  gtk_widget_show (headerbar);

  /* Content */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (self->window), vbox);

  button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_show (button_box);
  gtk_container_add (GTK_CONTAINER (vbox), button_box);

  button = gtk_button_new_from_icon_name (GIMP_ICON_DOCUMENT_NEW,
                                          GTK_ICON_SIZE_BUTTON);
  gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.new-unit");
  gtk_widget_set_tooltip_text (button, _("Create a new unit from scratch"));
  gtk_widget_show (button);
  gtk_container_add (GTK_CONTAINER (button_box), button);

  button = gtk_button_new_from_icon_name (GIMP_ICON_OBJECT_DUPLICATE,
                                          GTK_ICON_SIZE_BUTTON);
  gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.duplicate-unit");
  gtk_widget_set_tooltip_text (button, _("Create a new unit using the currently selected unit as template"));
  gtk_widget_show (button);
  gtk_container_add (GTK_CONTAINER (button_box), button);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_win, -1, 200);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (vbox), scrolled_win);
  gtk_widget_show (scrolled_win);

  gtk_widget_set_size_request (self->tv, -1, 220);
  gtk_container_add (GTK_CONTAINER (scrolled_win), self->tv);
  gtk_widget_set_vexpand (self->tv, TRUE);
  gtk_widget_show (self->tv);

  rend = gtk_cell_renderer_toggle_new ();
  col =
    gtk_tree_view_column_new_with_attributes (gettext (columns[SAVE].title),
                                              rend,
                                              "active",      SAVE,
                                              "activatable", USER_UNIT,
                                              NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (self->tv), col);

  col_widget = gtk_tree_view_column_get_widget (col);
  if (col_widget)
    {
      button = gtk_widget_get_ancestor (col_widget, GTK_TYPE_BUTTON);

      if (button)
        gimp_help_set_help_data (button,
                                 gettext (columns[SAVE].help), NULL);
    }

  g_signal_connect (rend, "toggled",
                    G_CALLBACK (saved_toggled_callback),
                    list_store);

  for (int i = 0; i < G_N_ELEMENTS (columns); i++)
    {
      if (i == SAVE)
        continue;

      col =
        gtk_tree_view_column_new_with_attributes (gettext (columns[i].title),
                                                  gtk_cell_renderer_text_new (),
                                                  "text", i,
                                                  NULL);

      gtk_tree_view_append_column (GTK_TREE_VIEW (self->tv), col);

      col_widget = gtk_tree_view_column_get_widget (col);
      if (col_widget)
        {
          button = gtk_widget_get_ancestor (col_widget, GTK_TYPE_BUTTON);

          if (button)
            gimp_help_set_help_data (button, gettext (columns[i].help), NULL);
        }
    }

  unit_list_init (GTK_TREE_VIEW (self->tv));

  g_signal_connect (self->window, "key-press-event",
                    G_CALLBACK (unit_editor_key_press_event),
                    NULL);

  gtk_widget_show (GTK_WIDGET (self->window));
}

static gboolean
unit_editor_key_press_event (GtkWidget   *window,
                             GdkEventKey *event,
                             gpointer     user_data)
{
  if (event->state == 0 &&
      event->keyval == GDK_KEY_Escape)
    gtk_widget_destroy (GTK_WIDGET (window));

  return FALSE;
}

static void
unit_editor_help_clicked (GtkWidget *window)
{
  gimp_standard_help_func (PLUG_IN_PROC, window);
}

static GimpValueArray *
editor_run (GimpProcedure        *procedure,
            GimpProcedureConfig  *config,
            gpointer              run_data)
{
  GimpUnitEditor *editor = GIMP_UNIT_EDITOR (run_data);

  gimp_ui_init (PLUG_IN_BINARY);

#if GLIB_CHECK_VERSION(2,74,0)
  editor->app = gtk_application_new (NULL, G_APPLICATION_DEFAULT_FLAGS);
#else
  editor->app = gtk_application_new (NULL, G_APPLICATION_FLAGS_NONE);
#endif
  g_signal_connect (editor->app, "activate", G_CALLBACK (on_app_activate), editor);

  g_application_run (G_APPLICATION (editor->app), 0, NULL);

  g_clear_object (&editor->app);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static GimpUnit *
new_unit_dialog (GtkWindow *main_window,
                 GimpUnit  *template)
{
  GtkWidget     *dialog;
  GtkWidget     *grid;
  GtkWidget     *entry;
  GtkWidget     *spinbutton;

  GtkWidget     *name_entry;
  GtkAdjustment *factor_adj;
  GtkAdjustment *digits_adj;
  GtkWidget     *symbol_entry;
  GtkWidget     *abbreviation_entry;

  GimpUnit      *unit = NULL;

  dialog = gimp_dialog_new (_("Add a New Unit"), PLUG_IN_ROLE,
                            GTK_WIDGET (main_window), GTK_DIALOG_MODAL,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Add"),    GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  entry = name_entry = gtk_entry_new ();
  if (template != gimp_unit_pixel ())
    {
      gtk_entry_set_text (GTK_ENTRY (entry),
                          gimp_unit_get_name (template));
    }
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("_Name:"), 0.0, 0.5,
                            entry, 1);

  gimp_help_set_help_data (entry, gettext (columns[NAME].help), NULL);

  factor_adj = gtk_adjustment_new ((template != gimp_unit_pixel ()) ?
                                   gimp_unit_get_factor (template) : 1.0,
                                   GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION,
                                   0.01, 0.1, 0.0);
  spinbutton = gimp_spin_button_new (factor_adj, 0.01, 5);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Factor:"), 0.0, 0.5,
                            spinbutton, 1);

  gimp_help_set_help_data (spinbutton, gettext (columns[FACTOR].help), NULL);

  digits_adj = gtk_adjustment_new ((template != gimp_unit_pixel ()) ?
                                   gimp_unit_get_digits (template) : 2.0,
                                   0, 5, 1, 1, 0);
  spinbutton = gimp_spin_button_new (digits_adj, 0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("_Digits:"), 0.0, 0.5,
                            spinbutton, 1);

  gimp_help_set_help_data (spinbutton, gettext (columns[DIGITS].help), NULL);

  entry = symbol_entry = gtk_entry_new ();
  if (template != gimp_unit_pixel ())
    {
      gtk_entry_set_text (GTK_ENTRY (entry),
                          gimp_unit_get_symbol (template));
    }
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                            _("_Symbol:"), 0.0, 0.5,
                            entry, 1);

  gimp_help_set_help_data (entry, gettext (columns[SYMBOL].help), NULL);

  entry = abbreviation_entry = gtk_entry_new ();
  if (template != gimp_unit_pixel ())
    {
      gtk_entry_set_text (GTK_ENTRY (entry),
                          gimp_unit_get_abbreviation (template));
    }
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            _("_Abbreviation:"), 0.0, 0.5,
                            entry, 1);

  gimp_help_set_help_data (entry, gettext (columns[ABBREVIATION].help), NULL);

  gtk_widget_show (dialog);

  while (TRUE)
    {
      gchar   *name;
      gdouble  factor;
      gint     digits;
      gchar   *symbol;
      gchar   *abbreviation;

      if (gimp_dialog_run (GIMP_DIALOG (dialog)) != GTK_RESPONSE_OK)
        break;

      name         = g_strdup (gtk_entry_get_text (GTK_ENTRY (name_entry)));
      factor       = gtk_adjustment_get_value (factor_adj);
      digits       = gtk_adjustment_get_value (digits_adj);
      symbol       = g_strdup (gtk_entry_get_text (GTK_ENTRY (symbol_entry)));
      abbreviation = g_strdup (gtk_entry_get_text (GTK_ENTRY (abbreviation_entry)));

      name         = g_strstrip (name);
      symbol       = g_strstrip (symbol);
      abbreviation = g_strstrip (abbreviation);

      if (! strlen (name)   ||
          ! strlen (symbol) ||
          ! strlen (abbreviation))
        {
          GtkWidget *msg = gtk_message_dialog_new (GTK_WINDOW (dialog), 0,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK,
                                                   _("Incomplete input"));

          gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (msg),
                                                    _("Please fill in all text fields."));
          gtk_dialog_run (GTK_DIALOG (msg));
          gtk_widget_destroy (msg);

          continue;
        }

      unit = gimp_unit_new (name, factor, digits, symbol, abbreviation);

      g_free (name);
      g_free (symbol);
      g_free (abbreviation);

      break;
    }

  gtk_widget_destroy (dialog);

  return unit;
}

static void
new_unit_action (GSimpleAction *action,
                 GVariant      *param,
                 gpointer       user_data)
{
  GimpUnitEditor   *self = GIMP_UNIT_EDITOR (user_data);
  GimpUnit         *unit;

  unit = new_unit_dialog (self->window, gimp_unit_pixel ());

  if (unit != NULL)
    {
      GtkTreeModel *model;
      GtkTreeIter   iter;

      unit_list_init (GTK_TREE_VIEW (self->tv));

      model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->tv));

      if (gtk_tree_model_get_iter_first (model, &iter) &&
          gtk_tree_model_iter_nth_child (model, &iter,
                                         NULL, gimp_unit_get_id (unit) - GIMP_UNIT_INCH))
        {
          GtkTreeSelection *selection;
          GtkAdjustment *adj;

          selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->tv));
          gtk_tree_selection_select_iter (selection, &iter);

          adj = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (self->tv));
          gtk_adjustment_set_value (adj, gtk_adjustment_get_upper (adj));
        }
    }
}

static void
duplicate_unit_action (GSimpleAction *action,
                       GVariant      *param,
                       gpointer       user_data)
{
  GimpUnitEditor   *self = GIMP_UNIT_EDITOR (user_data);
  GtkTreeModel     *model;
  GtkTreeSelection *sel;
  GtkTreeIter       iter;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->tv));
  sel   = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->tv));

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      GimpUnit *unit;

      gtk_tree_model_get (model, &iter,
                          UNIT,  &unit,
                          -1);

      unit = new_unit_dialog (self->window, unit);

      if (unit != NULL)
        {
          GtkTreeIter iter;

          unit_list_init (GTK_TREE_VIEW (self->tv));

          if (gtk_tree_model_get_iter_first (model, &iter) &&
              gtk_tree_model_iter_nth_child (model, &iter,
                                             NULL, gimp_unit_get_id (unit) - GIMP_UNIT_INCH))
            {
              GtkAdjustment *adj;

              gtk_tree_selection_select_iter (sel, &iter);

              adj = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (self->tv));
              gtk_adjustment_set_value (adj, gtk_adjustment_get_upper (adj));
            }
        }
    }
}

static void
refresh_action (GSimpleAction *action,
                GVariant      *param,
                gpointer       user_data)
{
  GimpUnitEditor *self = GIMP_UNIT_EDITOR (user_data);

  unit_list_init (GTK_TREE_VIEW (self->tv));
}

static void
saved_toggled_callback (GtkCellRendererToggle *celltoggle,
                        gchar                 *path_string,
                        GtkListStore          *list_store)
{
  GtkTreePath *path;
  GtkTreeIter  iter;
  gboolean     saved;
  GimpUnit    *unit;

  path = gtk_tree_path_new_from_string (path_string);

  if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, path))
    {
      g_warning ("%s: bad tree path?", G_STRLOC);
      return;
    }
  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter,
                      SAVE, &saved,
                      UNIT, &unit,
                      -1);

  if (! gimp_unit_is_built_in (unit))
    {
      gimp_unit_set_deletion_flag (unit, saved);
      gtk_list_store_set (GTK_LIST_STORE (list_store), &iter,
                          SAVE, ! saved,
                          -1);
    }
}

static void
unit_list_init (GtkTreeView *tv)
{
  GtkListStore *list_store;
  GtkTreeIter   iter;
  GimpUnit     *unit = gimp_unit_get_by_id (GIMP_UNIT_INCH);
  gint          i    = GIMP_UNIT_INCH + 1;

  list_store = GTK_LIST_STORE (gtk_tree_view_get_model (tv));

  gtk_list_store_clear (list_store);

  while (unit != NULL)
    {
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter,
                          SAVE,         ! gimp_unit_get_deletion_flag (unit),
                          NAME,         gimp_unit_get_name (unit),
                          FACTOR,       gimp_unit_get_factor (unit),
                          DIGITS,       gimp_unit_get_digits (unit),
                          SYMBOL,       gimp_unit_get_symbol (unit),
                          ABBREVIATION, gimp_unit_get_abbreviation (unit),
                          UNIT,         unit,
                          USER_UNIT,    ! gimp_unit_is_built_in (unit),
                          -1);

      unit = gimp_unit_get_by_id (i++);
    }

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    gtk_tree_selection_select_iter (gtk_tree_view_get_selection (tv), &iter);
}
