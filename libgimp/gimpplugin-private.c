/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpplugin-private.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimp-private.h"
#include "gimpgpparams.h"
#include "gimpplugin-private.h"
#include "gimpprocedure-private.h"


/*  local function prototpes  */

static void   gimp_plug_in_register          (GimpPlugIn      *plug_in,
                                              GList           *procedures);
static void   gimp_plug_in_loop              (GimpPlugIn      *plug_in);
static void   gimp_plug_in_process_message   (GimpPlugIn      *plug_in,
                                              GimpWireMessage *msg);
static void   gimp_plug_in_proc_run          (GimpPlugIn      *plug_in,
                                              GPProcRun       *proc_run);
static void   gimp_plug_in_temp_proc_run     (GimpPlugIn      *plug_in,
                                              GPProcRun       *proc_run);
static void   gimp_plug_in_proc_run_internal (GPProcRun       *proc_run,
                                              GimpProcedure   *procedure,
                                              GPProcReturn    *proc_return);


/*  public functions  */

void
_gimp_plug_in_query (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->query_procedures)
    {
      GList *procedures =
        GIMP_PLUG_IN_GET_CLASS (plug_in)->query_procedures (plug_in);

      gimp_plug_in_register (plug_in, procedures);
    }
}

void
_gimp_plug_in_init (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->init_procedures)
    {
      GList *procedures =
        GIMP_PLUG_IN_GET_CLASS (plug_in)->init_procedures (plug_in);

      gimp_plug_in_register (plug_in, procedures);
    }
}

void
_gimp_plug_in_run (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  gimp_plug_in_loop (plug_in);
}

void
_gimp_plug_in_quit (GimpPlugIn *plug_in)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (GIMP_PLUG_IN_GET_CLASS (plug_in)->quit)
    GIMP_PLUG_IN_GET_CLASS (plug_in)->quit (plug_in);
}

void
_gimp_plug_in_read_expect_msg (GimpPlugIn      *plug_in,
                               GimpWireMessage *msg,
                               gint             type)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  while (TRUE)
    {
      if (! gimp_wire_read_msg (_gimp_readchannel, msg, NULL))
        gimp_quit ();

      if (msg->type == type)
        return; /* up to the caller to call wire_destroy() */

      if (msg->type == GP_TEMP_PROC_RUN || msg->type == GP_QUIT)
        {
          gimp_plug_in_process_message (plug_in, msg);
        }
      else
        {
          g_error ("unexpected message: %d", msg->type);
        }

      gimp_wire_destroy (msg);
    }
}

gboolean
_gimp_plug_in_extension_read (GIOChannel  *channel,
                              GIOCondition condition,
                              gpointer     data)
{
  GimpPlugIn *plug_in = data;

  _gimp_plug_in_single_message (plug_in);

  return G_SOURCE_CONTINUE;
}


/*  private functions  */

static void
gimp_plug_in_register (GimpPlugIn *plug_in,
                       GList      *procedures)
{
  GList *list;

  for (list = procedures; list; list = g_list_next (list))
    {
      const gchar   *name = list->data;
      GimpProcedure *procedure;

      procedure = gimp_plug_in_create_procedure (plug_in, name);
      if (procedure)
        {
          _gimp_procedure_register (procedure);
          g_object_unref (procedure);
        }
      else
        {
          g_warning ("Plug-in failed to create procedure '%s'\n", name);
        }
    }

  g_list_free_full (procedures, g_free);

  if (plug_in->priv->translation_domain_name)
    {
      gchar *path = g_file_get_path (plug_in->priv->translation_domain_path);

      gimp_plugin_domain_register (plug_in->priv->translation_domain_name,
                                   path);

      g_free (path);
    }

  if (plug_in->priv->help_domain_name)
    {
      gchar *uri = g_file_get_uri (plug_in->priv->help_domain_uri);

      gimp_plugin_domain_register (plug_in->priv->help_domain_name,
                                   uri);

      g_free (uri);
    }

  for (list = plug_in->priv->menu_branches; list; list = g_list_next (list))
    {
      GimpPlugInMenuBranch *branch = list->data;

      gimp_plugin_menu_branch_register (branch->menu_path,
                                        branch->menu_label);
    }
}

static void
gimp_plug_in_loop (GimpPlugIn *plug_in)
{
  while (TRUE)
    {
      GimpWireMessage msg;

      if (! gimp_wire_read_msg (_gimp_readchannel, &msg, NULL))
        return;

      switch (msg.type)
        {
        case GP_QUIT:
          gimp_wire_destroy (&msg);
          return;

        case GP_CONFIG:
          _gimp_config (msg.data);
          break;

        case GP_TILE_REQ:
        case GP_TILE_ACK:
        case GP_TILE_DATA:
          g_warning ("unexpected tile message received (should not happen)");
          break;

        case GP_PROC_RUN:
          gimp_plug_in_proc_run (plug_in, msg.data);
          gimp_wire_destroy (&msg);
          return;

        case GP_PROC_RETURN:
          g_warning ("unexpected proc return message received (should not happen)");
          break;

        case GP_TEMP_PROC_RUN:
          g_warning ("unexpected temp proc run message received (should not happen");
          break;

        case GP_TEMP_PROC_RETURN:
          g_warning ("unexpected temp proc return message received (should not happen");
          break;

        case GP_PROC_INSTALL:
          g_warning ("unexpected proc install message received (should not happen)");
          break;

        case GP_HAS_INIT:
          g_warning ("unexpected has init message received (should not happen)");
          break;
        }

      gimp_wire_destroy (&msg);
    }
}

void
_gimp_plug_in_single_message (GimpPlugIn *plug_in)
{
  GimpWireMessage msg;

  /* Run a temp function */
  if (! gimp_wire_read_msg (_gimp_readchannel, &msg, NULL))
    gimp_quit ();

  gimp_plug_in_process_message (plug_in, &msg);

  gimp_wire_destroy (&msg);
}

static void
gimp_plug_in_process_message (GimpPlugIn      *plug_in,
                              GimpWireMessage *msg)
{
  switch (msg->type)
    {
    case GP_QUIT:
      gimp_quit ();
      break;
    case GP_CONFIG:
      _gimp_config (msg->data);
      break;
    case GP_TILE_REQ:
    case GP_TILE_ACK:
    case GP_TILE_DATA:
      g_warning ("unexpected tile message received (should not happen)");
      break;
    case GP_PROC_RUN:
      g_warning ("unexpected proc run message received (should not happen)");
      break;
    case GP_PROC_RETURN:
      g_warning ("unexpected proc return message received (should not happen)");
      break;
    case GP_TEMP_PROC_RUN:
      gimp_plug_in_temp_proc_run (plug_in, msg->data);
      break;
    case GP_TEMP_PROC_RETURN:
      g_warning ("unexpected temp proc return message received (should not happen)");
      break;
    case GP_PROC_INSTALL:
      g_warning ("unexpected proc install message received (should not happen)");
      break;
    case GP_HAS_INIT:
      g_warning ("unexpected has init message received (should not happen)");
      break;
    }
}

static void
gimp_plug_in_proc_run (GimpPlugIn *plug_in,
                       GPProcRun  *proc_run)
{
  GPProcReturn   proc_return;
  GimpProcedure *procedure;

  procedure = gimp_plug_in_create_procedure (plug_in, proc_run->name);

  if (procedure)
    {
      gimp_plug_in_proc_run_internal (proc_run, procedure,
                                      &proc_return);
      g_object_unref (procedure);
    }

  if (! gp_proc_return_write (_gimp_writechannel, &proc_return, NULL))
    gimp_quit ();
}

static void
gimp_plug_in_temp_proc_run (GimpPlugIn *plug_in,
                            GPProcRun  *proc_run)
{
  GPProcReturn   proc_return;
  GimpProcedure *procedure;

  procedure = gimp_plug_in_get_temp_procedure (plug_in, proc_run->name);

  if (procedure)
    {
      gimp_plug_in_proc_run_internal (proc_run, procedure,
                                      &proc_return);
    }

  if (! gp_temp_proc_return_write (_gimp_writechannel, &proc_return, NULL))
    gimp_quit ();
}

static void
gimp_plug_in_proc_run_internal (GPProcRun     *proc_run,
                                GimpProcedure *procedure,
                                GPProcReturn  *proc_return)
{
  GimpValueArray *arguments;
  GimpValueArray *return_values = NULL;

  arguments = _gimp_gp_params_to_value_array (NULL, 0,
                                              proc_run->params,
                                              proc_run->nparams,
                                              FALSE, FALSE);

  return_values = gimp_procedure_run (procedure, arguments);
  gimp_value_array_unref (arguments);

  proc_return->name    = proc_run->name;
  proc_return->nparams = gimp_value_array_length (return_values);
  proc_return->params  = _gimp_value_array_to_gp_params (return_values, TRUE);

  gimp_value_array_unref (return_values);
}
