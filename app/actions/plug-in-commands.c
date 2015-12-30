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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimp-filter-history.h"
#include "core/gimp-utils.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginmanager-data.h"

#include "pdb/gimpprocedure.h"

#include "widgets/gimpbufferview.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdatafactoryview.h"
#include "widgets/gimpfontview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpimageeditor.h"
#include "widgets/gimpitemtreeview.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"

#include "display/gimpdisplay.h"

#include "actions.h"
#include "plug-in-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

#if 0
static void  plug_in_procedure_execute     (GimpPlugInProcedure *procedure,
                                            Gimp                *gimp,
                                            GimpDisplay         *display,
                                            GimpValueArray      *args,
                                            gint                 n_args);
#endif

static gint  plug_in_collect_data_args     (GtkAction       *action,
                                            GimpObject      *object,
                                            GParamSpec     **pspecs,
                                            GimpValueArray  *args,
                                            gint             n_args);
static gint  plug_in_collect_image_args    (GtkAction       *action,
                                            GimpImage       *image,
                                            GParamSpec     **pspecs,
                                            GimpValueArray  *args,
                                            gint             n_args);
static gint  plug_in_collect_item_args     (GtkAction       *action,
                                            GimpImage       *image,
                                            GimpItem        *item,
                                            GParamSpec     **pspecs,
                                            GimpValueArray  *args,
                                            gint             n_args);
#if 0
static gint  plug_in_collect_display_args  (GtkAction       *action,
                                            GimpDisplay     *display,
                                            GParamSpec     **pspecs,
                                            GimpValueArray  *args,
                                            gint             n_args);
#endif
static void  plug_in_reset_all_response    (GtkWidget       *dialog,
                                            gint             response_id,
                                            Gimp            *gimp);


/*  public functions  */

void
plug_in_run_cmd_callback (GtkAction           *action,
                          GimpPlugInProcedure *proc,
                          gpointer             data)
{
  GimpProcedure  *procedure = GIMP_PROCEDURE (proc);
  Gimp           *gimp;
  GimpValueArray *args;
  gint            n_args    = 0;
  GimpDisplay    *display   = NULL;
  return_if_no_gimp (gimp, data);

  args = gimp_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  g_value_set_int (gimp_value_array_index (args, n_args),
                   GIMP_RUN_INTERACTIVE);
  n_args++;

  switch (procedure->proc_type)
    {
    case GIMP_EXTENSION:
      break;

    case GIMP_PLUGIN:
    case GIMP_TEMPORARY:
      if (GIMP_IS_DATA_FACTORY_VIEW (data) ||
          GIMP_IS_FONT_VIEW (data)         ||
          GIMP_IS_BUFFER_VIEW (data))
        {
          GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
          GimpContainer       *container;
          GimpContext         *context;
          GimpObject          *object;

          container = gimp_container_view_get_container (editor->view);
          context   = gimp_container_view_get_context (editor->view);

          object = gimp_context_get_by_type (context,
                                             gimp_container_get_children_type (container));

          n_args = plug_in_collect_data_args (action, object,
                                              procedure->args,
                                              args, n_args);
        }
      else if (GIMP_IS_IMAGE_EDITOR (data))
        {
          GimpImageEditor *editor = GIMP_IMAGE_EDITOR (data);
          GimpImage       *image;

          image = gimp_image_editor_get_image (editor);

          n_args = plug_in_collect_image_args (action, image,
                                               procedure->args,
                                               args, n_args);
        }
      else if (GIMP_IS_ITEM_TREE_VIEW (data))
        {
          GimpItemTreeView *view = GIMP_ITEM_TREE_VIEW (data);
          GimpImage        *image;
          GimpItem         *item;

          image = gimp_item_tree_view_get_image (view);

          if (image)
            item = GIMP_ITEM_TREE_VIEW_GET_CLASS (view)->get_active_item (image);
          else
            item = NULL;

          n_args = plug_in_collect_item_args (action, image, item,
                                              procedure->args,
                                              args, n_args);
        }
      else
        {
          display = action_data_get_display (data);

          n_args = plug_in_collect_display_args (action,
                                                 display,
                                                 procedure->args,
                                                 args, n_args);
        }
      break;

    case GIMP_INTERNAL:
      g_warning ("Unhandled procedure type.");
      n_args = -1;
      break;
    }

  if (n_args >= 1)
    plug_in_procedure_execute (proc, gimp, display, args, n_args);

  gimp_value_array_unref (args);
}

void
plug_in_reset_all_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  Gimp      *gimp;
  GtkWidget *dialog;
  return_if_no_gimp (gimp, data);

  dialog = gimp_message_dialog_new (_("Reset all Filters"), GIMP_STOCK_QUESTION,
                                    NULL, 0,
                                    gimp_standard_help_func, NULL,

                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GIMP_STOCK_RESET, GTK_RESPONSE_OK,

                                    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (plug_in_reset_all_response),
                    gimp);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("Do you really want to reset all "
                                       "filters to default values?"));

  gtk_widget_show (dialog);
}


/*  private functions  */

/* FIXME history */
void
plug_in_procedure_execute (GimpPlugInProcedure *procedure,
                           Gimp                *gimp,
                           GimpDisplay         *display,
                           GimpValueArray      *args,
                           gint                 n_args)
{
  GError *error = NULL;

  gimp_value_array_truncate (args, n_args);

  /* run the plug-in procedure */
  gimp_procedure_execute_async (GIMP_PROCEDURE (procedure), gimp,
                                gimp_get_user_context (gimp),
                                GIMP_PROGRESS (display), args,
                                GIMP_OBJECT (display), &error);

  if (error)
    {
      gimp_message_literal (gimp,
                            G_OBJECT (display), GIMP_MESSAGE_ERROR,
                            error->message);
      g_error_free (error);
    }
  else
    {
      /* remember only image plug-ins */
      if (GIMP_PROCEDURE (procedure)->num_args  >= 2  &&
          GIMP_IS_PARAM_SPEC_IMAGE_ID (GIMP_PROCEDURE (procedure)->args[1]))
        {
          gimp_filter_history_add (gimp, procedure);
        }
    }
}

static gint
plug_in_collect_data_args (GtkAction       *action,
                           GimpObject      *object,
                           GParamSpec     **pspecs,
                           GimpValueArray  *args,
                           gint             n_args)
{
  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_STRING (pspecs[n_args]))
    {
      if (object)
        {
          g_value_set_string (gimp_value_array_index (args, n_args),
                              gimp_object_get_name (object));
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active data object for the plug-in!");
          return -1;
        }
    }

  return n_args;
}

static gint
plug_in_collect_image_args (GtkAction       *action,
                            GimpImage       *image,
                            GParamSpec     **pspecs,
                            GimpValueArray  *args,
                            gint             n_args)
{
  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_IMAGE_ID (pspecs[n_args]))
    {
      if (image)
        {
          gimp_value_set_image (gimp_value_array_index (args, n_args), image);
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active image for the plug-in!");
          return -1;
        }
    }

  return n_args;
}

static gint
plug_in_collect_item_args (GtkAction       *action,
                           GimpImage       *image,
                           GimpItem        *item,
                           GParamSpec     **pspecs,
                           GimpValueArray  *args,
                           gint             n_args)
{
  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_IMAGE_ID (pspecs[n_args]))
    {
      if (image)
        {
          gimp_value_set_image (gimp_value_array_index (args, n_args), image);
          n_args++;

          if (gimp_value_array_length (args) > n_args &&
              GIMP_IS_PARAM_SPEC_ITEM_ID (pspecs[n_args]))
            {
              if (item &&
                  g_type_is_a (G_TYPE_FROM_INSTANCE (item),
                               GIMP_PARAM_SPEC_ITEM_ID (pspecs[n_args])->item_type))
                {
                  gimp_value_set_item (gimp_value_array_index (args, n_args),
                                       item);
                  n_args++;
                }
              else
                {
                  g_warning ("Uh-oh, no active item for the plug-in!");
                  return -1;
                }
            }
        }
    }

  return n_args;
}

/* FIXME history */
gint
plug_in_collect_display_args (GtkAction       *action,
                              GimpDisplay     *display,
                              GParamSpec     **pspecs,
                              GimpValueArray  *args,
                              gint             n_args)
{
  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_DISPLAY_ID (pspecs[n_args]))
    {
      if (display)
        {
          gimp_value_set_display (gimp_value_array_index (args, n_args),
                                  GIMP_OBJECT (display));
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active display for the plug-in!");
          return -1;
        }
    }

  if (gimp_value_array_length (args) > n_args &&
      GIMP_IS_PARAM_SPEC_IMAGE_ID (pspecs[n_args]))
    {
      GimpImage *image = display ? gimp_display_get_image (display) : NULL;

      if (image)
        {
          gimp_value_set_image (gimp_value_array_index (args, n_args),
                                image);
          n_args++;

          if (gimp_value_array_length (args) > n_args &&
              GIMP_IS_PARAM_SPEC_DRAWABLE_ID (pspecs[n_args]))
            {
              GimpDrawable *drawable = gimp_image_get_active_drawable (image);

              if (drawable)
                {
                  gimp_value_set_drawable (gimp_value_array_index (args, n_args),
                                           drawable);
                  n_args++;
                }
              else
                {
                  g_warning ("Uh-oh, no active drawable for the plug-in!");
                  return -1;
                }
            }
        }
    }

  return n_args;
}

static void
plug_in_reset_all_response (GtkWidget *dialog,
                            gint       response_id,
                            Gimp      *gimp)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    gimp_plug_in_manager_data_free (gimp->plug_in_manager);
}
