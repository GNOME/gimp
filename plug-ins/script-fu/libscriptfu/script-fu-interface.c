/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifdef GDK_WINDOWING_QUARTZ
#import <Cocoa/Cocoa.h>
#elif defined (G_OS_WIN32)
#include <windows.h>
#endif

#include "scheme-wrapper.h"

#include "script-fu-lib.h"
#include "script-fu-types.h"

#include "script-fu-interface.h"
#include "script-fu-scripts.h"
#include "script-fu-script.h"
#include "script-fu-arg.h"

#include "script-fu-intl.h"


#define RESPONSE_RESET         1

#define TEXT_WIDTH           100
#define COLOR_SAMPLE_WIDTH    60
#define COLOR_SAMPLE_HEIGHT   15


typedef struct
{
  GtkWidget  *dialog;

  GtkWidget  *grid;
  GtkWidget **widgets;

  gchar      *title;

  gboolean    running;
} SFInterface;


/*
 *  Local Functions
 */

static void   script_fu_interface_quit      (SFScript      *script);

static void   script_fu_response            (GtkWidget     *widget,
                                             gint           response_id,
                                             SFScript      *script);
static void   script_fu_ok                  (SFScript      *script);
static void   script_fu_reset               (SFScript      *script);

static void   script_fu_file_callback       (GtkWidget     *widget,
                                             SFFilename    *file);
static void   script_fu_combo_callback      (GtkWidget     *widget,
                                             SFOption      *option);
static void   script_fu_color_button_update (GimpColorButton *button,
                                             SFColorType      color);

static void   script_fu_resource_set_handler (gpointer       data,
                                              gpointer       resource,
                                              gboolean       closing);

static GtkWidget * script_fu_resource_widget (const gchar    *title,
                                              SFArg          *arg,
                                              GType           resource_type);

static void   script_fu_flush_events         (void);
static void   script_fu_activate_main_dialog (void);


/*
 *  Local variables
 */

static SFInterface       *sf_interface = NULL;  /*  there can only be at most
                                                 *  one interactive interface
                                                 */

static GimpPDBStatusType  sf_status    = GIMP_PDB_SUCCESS;


/*
 *  Function definitions
 */

gboolean
script_fu_interface_is_active (void)
{
  return (sf_interface != NULL);
}


GimpPDBStatusType
script_fu_interface_dialog (SFScript  *script,
                            gint       start_arg)
{
  GtkWidget    *dialog;
  GtkWidget    *vbox;
  GtkSizeGroup *group;
  GSList       *list;
  gchar        *title;
  gint          i;

  /* Requires gimp_ui_init called previously. */

  g_debug ("%s", G_STRFUNC);

  /* Simply return if there is already an interface. This is an
   * ugly workaround for the fact that we can not process two
   * scripts at a time.
   */
  if (sf_interface != NULL)
    {
      gchar *message =
        g_strdup_printf ("%s\n\n%s",
                         _("Script-Fu cannot process two scripts "
                           "at the same time."),
                         _("You are already running the \"%s\" script."));

      g_message (message, sf_interface->title);
      g_free (message);

      return GIMP_PDB_CANCEL;
    }

  g_return_val_if_fail (script != NULL, FALSE);

  sf_status = GIMP_PDB_SUCCESS;

  sf_interface = g_slice_new0 (SFInterface);

  sf_interface->widgets = g_new0 (GtkWidget *, script->n_args);
  sf_interface->title   = script_fu_script_get_title (script);

  title = g_strdup_printf ("%s", sf_interface->title);

  sf_interface->dialog = dialog =
    gimp_dialog_new (title, "gimp-script-fu",
                     NULL, 0,
                     gimp_standard_help_func, script->name,

                     _("_Reset"),  RESPONSE_RESET,
                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                     _("_OK"),     GTK_RESPONSE_OK,

                     NULL);
  g_free (title);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  g_signal_connect (dialog, "response",
                    G_CALLBACK (script_fu_response),
                    script);

  g_signal_connect_swapped (dialog, "destroy",
                            G_CALLBACK (script_fu_interface_quit),
                            script);

  gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /*  The argument table  */
  sf_interface->grid = gtk_grid_new ();

  gtk_grid_set_row_spacing (GTK_GRID (sf_interface->grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (sf_interface->grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), sf_interface->grid, FALSE, FALSE, 0);
  gtk_widget_show (sf_interface->grid);

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  for (i = start_arg; i < script->n_args; i++)
    {
      GtkWidget *widget       = NULL;
      gchar     *label_text;
      gfloat     label_yalign = 0.5;
      gint      *ID_ptr       = NULL;
      gint       row          = i;
      gboolean   left_align   = FALSE;
      SFArg     *arg          = &script->args[i];

      row -= start_arg;

      /*  we add a colon after the label;
       *  some languages want an extra space here
       */
      label_text = g_strdup_printf (_("%s:"), arg->label);

      switch (arg->type)
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
        case SF_VECTORS:
          switch (arg->type)
            {
            case SF_IMAGE:
              widget = gimp_image_combo_box_new (NULL, NULL, NULL);
              ID_ptr = &arg->value.sfa_image;
              break;

            case SF_DRAWABLE:
              widget = gimp_drawable_combo_box_new (NULL, NULL, NULL);
              ID_ptr = &arg->value.sfa_drawable;
              break;

            case SF_LAYER:
              widget = gimp_layer_combo_box_new (NULL, NULL, NULL);
              ID_ptr = &arg->value.sfa_layer;
              break;

            case SF_CHANNEL:
              widget = gimp_channel_combo_box_new (NULL, NULL, NULL);
              ID_ptr = &arg->value.sfa_channel;
              break;

            case SF_VECTORS:
              widget = gimp_path_combo_box_new (NULL, NULL, NULL);
              ID_ptr = &arg->value.sfa_vectors;
              break;

            default:
              break;
            }

          gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (widget), *ID_ptr,
                                      G_CALLBACK (gimp_int_combo_box_get_active),
                                      ID_ptr, NULL);
          break;

        case SF_COLOR:
          {
            GimpColorConfig *config;
            GeglColor       *color = sf_color_get_gegl_color (arg->value.sfa_color);

            left_align = TRUE;
            widget = gimp_color_button_new (_("Script-Fu Color Selection"),
                                            COLOR_SAMPLE_WIDTH, COLOR_SAMPLE_HEIGHT,
                                            color, GIMP_COLOR_AREA_FLAT);

            gimp_color_button_set_update (GIMP_COLOR_BUTTON (widget), TRUE);

            config = gimp_get_color_configuration ();
            gimp_color_button_set_color_config (GIMP_COLOR_BUTTON (widget),
                                                config);
            g_object_unref (config);
            g_object_unref (color);

            g_signal_connect (widget, "color-changed",
                              G_CALLBACK (script_fu_color_button_update),
                              arg->value.sfa_color);
          }
          break;

        case SF_TOGGLE:
          g_free (label_text);
          label_text = NULL;
          widget = gtk_check_button_new_with_mnemonic (arg->label);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                        arg->value.sfa_toggle);

          g_signal_connect (widget, "toggled",
                            G_CALLBACK (gimp_toggle_button_update),
                            &arg->value.sfa_toggle);
          break;

        case SF_STRING:
          widget = gtk_entry_new ();
          gtk_widget_set_size_request (widget, TEXT_WIDTH, -1);
          gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);

          gtk_entry_set_text (GTK_ENTRY (widget), arg->value.sfa_value);
          break;

        case SF_TEXT:
          {
            GtkWidget     *view;
            GtkTextBuffer *buffer;

            widget = gtk_scrolled_window_new (NULL, NULL);
            gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget),
                                                 GTK_SHADOW_IN);
            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget),
                                            GTK_POLICY_AUTOMATIC,
                                            GTK_POLICY_AUTOMATIC);
            gtk_widget_set_size_request (widget, TEXT_WIDTH, -1);

            view = gtk_text_view_new ();
            gtk_container_add (GTK_CONTAINER (widget), view);
            gtk_widget_show (view);

            buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
            gtk_text_view_set_editable (GTK_TEXT_VIEW (view), TRUE);

            gtk_text_buffer_set_text (buffer, arg->value.sfa_value, -1);

            label_yalign = 0.0;
          }
          break;

        case SF_ADJUSTMENT:
          {
            GtkAdjustment *adj_widget;

            switch (arg->default_value.sfa_adjustment.type)
              {
              case SF_SLIDER:
                {
                  GtkWidget *spinbutton;

                  widget = gimp_scale_entry_new ("",
                                                 arg->value.sfa_adjustment.value,
                                                 arg->default_value.sfa_adjustment.lower,
                                                 arg->default_value.sfa_adjustment.upper,
                                                 arg->default_value.sfa_adjustment.digits);

                  /* #12157 We should not need to set value, since we just passed it.
                   * But seems to be a flaw in the widget.
                   * The comments in gimp_label_spin_init, says it inits to bogus.
                   */
                  gimp_label_spin_set_value ((GimpLabelSpin*)widget, arg->value.sfa_adjustment.value);

                  gimp_label_spin_set_increments (GIMP_LABEL_SPIN (widget),
                                                  arg->default_value.sfa_adjustment.step,
                                                  arg->default_value.sfa_adjustment.page);
                  spinbutton = gimp_label_spin_get_spin_button (GIMP_LABEL_SPIN (widget));
                  adj_widget = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbutton));

                  gtk_entry_set_activates_default (GTK_ENTRY (spinbutton), TRUE);
                }
                break;

              default:
                g_warning ("unexpected adjustment type: %d",
                           arg->default_value.sfa_adjustment.type);
                /* fallthrough */

              case SF_SPINNER:
                left_align = TRUE;
                adj_widget =
                  gtk_adjustment_new (arg->value.sfa_adjustment.value,
                                      arg->default_value.sfa_adjustment.lower,
                                      arg->default_value.sfa_adjustment.upper,
                                      arg->default_value.sfa_adjustment.step,
                                      arg->default_value.sfa_adjustment.page,
                                      0);
                widget = gimp_spin_button_new (adj_widget,
                                               arg->default_value.sfa_adjustment.step,
                                               arg->default_value.sfa_adjustment.digits);
                gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (widget), TRUE);
                gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);

                break;
              } /* end switch on adjustment type. */

            /* Each case set adj_widget. */
            g_assert (adj_widget != NULL);

            g_signal_connect (adj_widget,
                              "value-changed",
                              G_CALLBACK (gimp_double_adjustment_update),
                              &arg->value.sfa_adjustment.value);
          } /* end case SF_ADJUSTMENT */
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          if (arg->type == SF_FILENAME)
            widget = gtk_file_chooser_button_new (_("Script-Fu File Selection"),
                                                  GTK_FILE_CHOOSER_ACTION_OPEN);
          else
            widget = gtk_file_chooser_button_new (_("Script-Fu Folder Selection"),
                                                  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

          if (arg->value.sfa_file.filename)
            gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (widget),
                                           arg->value.sfa_file.filename);

          g_signal_connect (widget, "selection-changed",
                            G_CALLBACK (script_fu_file_callback),
                            &arg->value.sfa_file);
          break;

        case SF_FONT:
          widget = script_fu_resource_widget (label_text,
                                              arg,
                                              GIMP_TYPE_FONT);
          break;

        case SF_PALETTE:
          widget = script_fu_resource_widget (label_text,
                                              arg,
                                              GIMP_TYPE_PALETTE);
          break;

        case SF_PATTERN:
          left_align = TRUE;
          widget = script_fu_resource_widget (label_text,
                                              arg,
                                              GIMP_TYPE_PATTERN);
          break;

        case SF_GRADIENT:
          left_align = TRUE;
          widget = script_fu_resource_widget (label_text,
                                              arg,
                                              GIMP_TYPE_GRADIENT);
          break;

        case SF_BRUSH:
          left_align = TRUE;
          widget = script_fu_resource_widget (label_text,
                                              arg,
                                              GIMP_TYPE_BRUSH);
          break;

        case SF_OPTION:
          widget = gtk_combo_box_text_new ();
          for (list = arg->default_value.sfa_option.list;
               list;
               list = g_slist_next (list))
            {
              gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget),
                                              list->data);
            }

          gtk_combo_box_set_active (GTK_COMBO_BOX (widget),
                                    arg->value.sfa_option.history);

          g_signal_connect (widget, "changed",
                            G_CALLBACK (script_fu_combo_callback),
                            &arg->value.sfa_option);
          break;

        case SF_ENUM:
          widget = gimp_enum_combo_box_new (g_type_from_name (arg->default_value.sfa_enum.type_name));

          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (widget),
                                         arg->value.sfa_enum.history);

          g_signal_connect (widget, "changed",
                            G_CALLBACK (gimp_int_combo_box_get_active),
                            &arg->value.sfa_enum.history);
          break;

        case SF_DISPLAY:
          break;
        }

      if (widget)
        {
          if (label_text)
            {
              gimp_grid_attach_aligned (GTK_GRID (sf_interface->grid),
                                        0, row,
                                        label_text, 0.0, label_yalign,
                                        widget, 2);
              g_free (label_text);
            }
          else
            {
              gtk_grid_attach (GTK_GRID (sf_interface->grid),
                               widget, 0, row, 3, 1);
              gtk_widget_show (widget);
            }

          if (left_align)
            gtk_size_group_add_widget (group, widget);
        }

      sf_interface->widgets[i] = widget;
    }

  g_object_unref (group);

#ifdef G_OS_WIN32
    {
      HWND foreground = GetForegroundWindow ();
#endif

  gtk_widget_show (dialog);

  gtk_main ();
  /* The script ran, or was canceled, and called gtk_main_quit */

#ifdef GDK_WINDOWING_QUARTZ
  /* Make dock icon go away. */
  [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
#endif

#ifdef G_OS_WIN32
      if (! GetForegroundWindow ())
        SetForegroundWindow (foreground);
    }
#endif
  return sf_status;
}

/* Return a widget for choosing a resource.
 * Dispatch on resource type.
 *
 * On first show, widget will show initial choice : declared by script.
 * Thereafter, the choice from prior use of filter.
 *
 * Widget will update model if user touches widget.
 */
static GtkWidget *
script_fu_resource_widget (const gchar    *title,
                           SFArg          *arg,
                           GType           resource_type)
{
  GtkWidget    *result_widget = NULL;
  GimpResource *initial_value;

  g_debug ("%s type: %" G_GSIZE_FORMAT, G_STRFUNC, resource_type);

  /* Init the arg's value (ID).
   * On first run, initialize to default.
   * On second run, init to prior choice.
   * Can't do it at registration time, resources are not loaded.
   */
  sf_resource_arg_init_current_value (arg);

  /* Passing empty string for outer widget label,
   * since this old-style interface makes the label.
   */
  /* Create a widget with initial value of the arg. */
  initial_value = sf_resource_arg_get_value (arg);
  if (g_type_is_a (resource_type, GIMP_TYPE_FONT))
    {
      result_widget = gimp_font_chooser_new (title, "", GIMP_FONT (initial_value));
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_BRUSH))
    {
      result_widget = gimp_brush_chooser_new (title, "", GIMP_BRUSH (initial_value));
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_GRADIENT))
    {
      result_widget = gimp_gradient_chooser_new (title, "", GIMP_GRADIENT (initial_value));
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PALETTE))
    {
      result_widget = gimp_palette_chooser_new (title, "", GIMP_PALETTE (initial_value));
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PATTERN))
    {
      result_widget = gimp_pattern_chooser_new (title, "", GIMP_PATTERN (initial_value));
    }
  else
    {
      g_warning ("%s: Unhandled resource type", G_STRFUNC);
    }

  if (result_widget != NULL)
  {
    /* All resource widgets emit signal resource-set
     * Connect to our handler which will be passed
     * a Resource* and int* to the arg's value,
     * the model of the new choice of resource.
     */
    g_signal_connect_swapped (result_widget, "resource-set",
                              G_CALLBACK (script_fu_resource_set_handler),
                              &arg->value.sfa_resource.history);  /* data */
  }

  return result_widget;
}

static void
script_fu_interface_quit (SFScript *script)
{
  g_return_if_fail (script != NULL);
  g_return_if_fail (sf_interface != NULL);

  g_free (sf_interface->title);
  g_free (sf_interface->widgets);

  g_slice_free (SFInterface, sf_interface);
  sf_interface = NULL;

  /*  We do not call gtk_main_quit() earlier to reduce the possibility
   *  that script_fu_script_proc() is called from gimp_extension_process()
   *  while we are not finished with the current script. This sucks!
   */

  gtk_main_quit ();
}

static void
script_fu_file_callback (GtkWidget  *widget,
                         SFFilename *file)
{
  if (file->filename)
    g_free (file->filename);

  file->filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
  script_fu_activate_main_dialog ();
}

static void
script_fu_combo_callback (GtkWidget *widget,
                          SFOption  *option)
{
  option->history = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
}

static void
script_fu_color_button_update (GimpColorButton *button,
                               SFColorType      arg_value)
{
  GeglColor *color = gimp_color_button_get_color (button);

  sf_color_set_from_gegl_color (&arg_value, color);

  g_object_unref (color);
}

/* Handle resource-set signal.
 * Store id of newly chosen resource in SF local cache of args,
 * at the integer location passed by pointer in "data"
 *
 * Note the callback is the same for all resource types.
 * The callback passes the resource, and no attributes of the resource.
 */
static void
script_fu_resource_set_handler (gpointer      data,  /* callback "data" */
                                gpointer      resource,
                                gboolean      closing)
{
  gint32 *id_pointer = data;

  /* Store integer ID, not pointer to Resource.  We bind to integer ID. */
  *id_pointer = gimp_resource_get_id (resource);

  if (closing) script_fu_activate_main_dialog ();
}

static void
unset_transient_for (GtkWidget *dialog)
{
  GdkWindow *window = gtk_widget_get_window (dialog);

  if (window)
    gdk_property_delete (window,
                         gdk_atom_intern_static_string ("WM_TRANSIENT_FOR"));
}

static void
script_fu_response (GtkWidget *widget,
                    gint       response_id,
                    SFScript  *script)
{
  if (sf_interface->running)
    return;

  switch (response_id)
    {
    case RESPONSE_RESET:
      script_fu_reset (script);
      break;

    case GTK_RESPONSE_OK:
      sf_interface->running = TRUE;

      gtk_widget_set_sensitive (sf_interface->grid, FALSE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (sf_interface->dialog),
                                         GTK_RESPONSE_HELP,
                                         FALSE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (sf_interface->dialog),
                                         RESPONSE_RESET,
                                         FALSE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (sf_interface->dialog),
                                         GTK_RESPONSE_CANCEL,
                                         FALSE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (sf_interface->dialog),
                                         GTK_RESPONSE_OK,
                                         FALSE);

      script_fu_ok (script);

      script_fu_flush_events ();
      /*
       * The script could have created a new GimpImageWindow, so
       * unset the transient-for property not to focus the
       * ImageWindow from which the script was started
       */
      unset_transient_for (sf_interface->dialog);

      gtk_widget_destroy (sf_interface->dialog);
      break;

    default:
      sf_status = GIMP_PDB_CANCEL;

      script_fu_flush_events ();
      gtk_widget_destroy (sf_interface->dialog);
      break;
    }

  script_fu_flush_events ();
}

/* Update model for all widgets that
 * don't have callbacks that update the model.
 * Only the text and string widgets don't.
 *
 * The model is the arg.value field.
 */
static void
script_fu_update_models (SFScript *script)
{
  for (gint i = 0; i < script->n_args; i++)
    {
      SFArgValue *arg_value = &script->args[i].value;
      GtkWidget  *widget    = sf_interface->widgets[i];

      switch (script->args[i].type)
        {
        case SF_STRING:
          g_free (arg_value->sfa_value);
          arg_value->sfa_value =
            g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
          break;

        case SF_TEXT:
          {
            GtkWidget     *view;
            GtkTextBuffer *buffer;
            GtkTextIter    start, end;

            view = gtk_bin_get_child (GTK_BIN (widget));
            buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

            gtk_text_buffer_get_start_iter (buffer, &start);
            gtk_text_buffer_get_end_iter (buffer, &end);

            g_free (arg_value->sfa_value);
            arg_value->sfa_value = gtk_text_buffer_get_text (buffer,
                                                             &start, &end,
                                                             FALSE);
          }
          break;

        default:
          /* Silence warnings about unhandled enumeration values */
          break;
        }
    }
}


/* Handler for event: OK button clicked.
 *
 * Run the scripts with values from the dialog.
 *
 * Sets a global status of the PDB call to this plugin,
 * which is returned later by interface_dialog.
 */
static void
script_fu_ok (SFScript *script)
{
  gchar   *command;

  script_fu_update_models (script);

  command = script_fu_script_get_command (script);

  /*  run the command through the interpreter  */

  gimp_plug_in_set_pdb_error_handler (gimp_get_plug_in (),
                                      GIMP_PDB_ERROR_HANDLER_PLUGIN);

  script_fu_redirect_output_to_stdout ();

  /* Returns non-zero error code on failure. */
  if (ts_interpret_string (command))
    {
      gchar *message;

      /* Log to stdout.  Later to Gimp. */
      message = g_strdup_printf (_("Error while executing %s:"), script->name);
      g_message ("%s\n", message);
      g_free (message);

      /* Set global to be returned by script-fu-interface-dialog. */
      sf_status = GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_plug_in_set_pdb_error_handler (gimp_get_plug_in (),
                                      GIMP_PDB_ERROR_HANDLER_INTERNAL);

  g_free (command);
}

/* On user clicking "Reset" button. */
static void
script_fu_reset (SFScript *script)
{
  gint i;

  /* Reset the model to defaults. */
  script_fu_script_reset (script, FALSE);

  /* Reset the view to the model values. */
  for (i = 0; i < script->n_args; i++)
    {
      SFArg      *arg    = &script->args[i];
      SFArgValue *value  = &arg->value;
      GtkWidget  *widget = sf_interface->widgets[i];

      switch (script->args[i].type)
        {
        case SF_IMAGE:
        case SF_DRAWABLE:
        case SF_LAYER:
        case SF_CHANNEL:
        case SF_VECTORS:
        case SF_DISPLAY:
          break;

        case SF_COLOR:
          {
            GeglColor *color = sf_color_get_gegl_color (value->sfa_color);

            gimp_color_button_set_color (GIMP_COLOR_BUTTON (widget), color);
            g_object_unref (color);
          }
          break;

        case SF_TOGGLE:
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                        value->sfa_toggle);
          break;

        case SF_STRING:
          gtk_entry_set_text (GTK_ENTRY (widget), value->sfa_value);
          break;

        case SF_TEXT:
          {
            GtkWidget     *view;
            GtkTextBuffer *buffer;

            view = gtk_bin_get_child (GTK_BIN (widget));
            buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

            gtk_text_buffer_set_text (buffer, value->sfa_value, -1);
          }
          break;

        case SF_ADJUSTMENT:
          {
            /* Reset the widget's underlying GtkAdjustment.
             * The widget knows its own GtkAdjustment.
             * The widget is a GimpScaleEntry or a GimpSpinButton.
             */
            GtkAdjustment *adj_widget = NULL;

            switch (script->args[i].default_value.sfa_adjustment.type)
              {
              case SF_SLIDER:  /* GimpScaleEntry */
                {
                  /* Widget knows its range which knows its adjustment. */
                  /* Unfortunately, gimp_scale_entry_get_range returns GtkWidget*, we must downcast.*/
                  GtkRange *range = GTK_RANGE (gimp_scale_entry_get_range (GIMP_SCALE_ENTRY (widget)));

                  adj_widget = gtk_range_get_adjustment (range);
                }
                break;

              case SF_SPINNER:  /* GimpSpinButton */
                adj_widget = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
                break;
              }
            g_assert (adj_widget != NULL);

            gtk_adjustment_set_value (adj_widget, value->sfa_adjustment.value);
          }
          break;

        case SF_FILENAME:
        case SF_DIRNAME:
          gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (widget),
                                         value->sfa_file.filename);
          break;

        case SF_FONT:
        case SF_PALETTE:
        case SF_PATTERN:
        case SF_GRADIENT:
        case SF_BRUSH:
          gimp_resource_chooser_set_resource (GIMP_RESOURCE_CHOOSER (widget),
                                              sf_resource_arg_get_default (arg));
          break;

        case SF_OPTION:
          gtk_combo_box_set_active (GTK_COMBO_BOX (widget),
                                    value->sfa_option.history);
          break;

        case SF_ENUM:
          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (widget),
                                         value->sfa_enum.history);
          break;
        }
    }
}


/*
 * Functions for window front/back management.
 * These might only be necessary for MacOS.
 *
 * One problem is that the GIMP and the scriptfu extension process are separate "apps".
 * Closing a main scriptfu dialog does not terminate the scriptfu extension process,
 * and MacOS does not then activate some other app.
 * On other platforms, the select dialogs are transient to the GIMP app's progress bar,
 * but that doesn't seem to work on MacOS.
 *
 * Also some of the GIMP "select" widgets (for brush, pattern, gradient, font, palette)
 * for plugins are independent and tool like:
 * their popup dialog window is not a child of the button which pops it up.
 * The button "owns" the popup but GDK is not aware of that relation,
 * and so does not handle closing automatically.
 * There are PDB callbacks in each direction:
 * from the select dialog to the scriptfu extension on user's new selection
 * from the scriptfu extension to the select dialog on closing
 *
 * This might change in the future.
 * 1) in GIMP 3, scriptfu and python plugins should use a common API for a control dialog,
 * (script-fu-interface.c is obsoleted?)
 * 2) the code for select dialogs is significantly changed in GIMP 3
 * 3) Gtk3 might solve this (now using Gtk2.)
 */

/* On MacOS, without calls to this, scriptfu dialog gets spinning ball of doom,
 * meaning MacOS thinks app is not responding to events,
 * and dialog stays visible even after destroyed.
 */
static void
script_fu_flush_events (void)
{
  /* Ensure all GLib events have been processed. */

  /* Former code also hid GUI of the script-fu extension process using: [NSApp hide: nil];
   * (In Objective-C, get instance of NSApplication and send it a "hide" message with nil sender.)
   * Hiding is not necessary, since this is only called when
   * scriptfu is done interpreting the current script and is destroying its widgets.
   */
#ifdef GDK_WINDOWING_QUARTZ
  /* Alternative code might be a call to gtk_main()?
   * This is not an infinite loop since there are finite events, and iteration reduces them.
   * Somehow, this lets MacOS think the app is responsive.
   */
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, TRUE);
#else
  /* empty function, optimized out. */
#endif
}


/* On MacOS, without calls to this,
 * when user closes GIMP "select" dialogs (child of main dialog)
 * the main scriptfu dialog can be obscured by GIMP main window.
 * The main scriptfu dialog must be visible so user can choose the OK button,
 * and it contains a progress bar.
 *
 * Note the color "select" dialog has no callback specialized for scriptfu.
 * And the file "chooser" dialog is also different.
 */
static void
script_fu_activate_main_dialog (void)
{
  /* Ensure the main dialog of the script-fu extension process is not obscured. */
#ifdef GDK_WINDOWING_QUARTZ
  /* Make Dock icon appear. */
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  /* Bring to front. */
  [NSApp activateIgnoringOtherApps: YES];
#else
  /* empty function, optimized out. */
#endif
}
