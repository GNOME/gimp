/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>

#include "gimpbasetypes.h"

#include "gimpparamspecs.h"
#include "gimpparasite.h"
#include "gimpprotocol.h"
#include "gimpwire.h"


static void _gp_quit_read                (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_quit_write               (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_quit_destroy             (GimpWireMessage  *msg);

static void _gp_config_read              (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_config_write             (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_config_destroy           (GimpWireMessage  *msg);

static void _gp_tile_req_read            (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_tile_req_write           (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_tile_req_destroy         (GimpWireMessage  *msg);

static void _gp_tile_ack_read            (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_tile_ack_write           (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_tile_ack_destroy         (GimpWireMessage  *msg);

static void _gp_tile_data_read           (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_tile_data_write          (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_tile_data_destroy        (GimpWireMessage  *msg);

static void _gp_proc_run_read            (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_proc_run_write           (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_proc_run_destroy         (GimpWireMessage  *msg);

static void _gp_proc_return_read         (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_proc_return_write        (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_proc_return_destroy      (GimpWireMessage  *msg);

static void _gp_temp_proc_run_read       (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_temp_proc_run_write      (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_temp_proc_run_destroy    (GimpWireMessage  *msg);

static void _gp_temp_proc_return_read    (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_temp_proc_return_write   (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_temp_proc_return_destroy (GimpWireMessage  *msg);

static void _gp_proc_install_read        (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_proc_install_write       (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_proc_install_destroy     (GimpWireMessage  *msg);

static void _gp_proc_uninstall_read      (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_proc_uninstall_write     (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_proc_uninstall_destroy   (GimpWireMessage  *msg);

static void _gp_extension_ack_read       (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_extension_ack_write      (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_extension_ack_destroy    (GimpWireMessage  *msg);

static void _gp_params_read              (GIOChannel       *channel,
                                          GPParam         **params,
                                          guint            *nparams,
                                          gpointer          user_data);
static void _gp_params_write             (GIOChannel       *channel,
                                          GPParam          *params,
                                          gint              nparams,
                                          gpointer          user_data);
static void _gp_params_destroy           (GPParam          *params,
                                          gint              nparams);


static void _gp_has_init_read            (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_has_init_write           (GIOChannel       *channel,
                                          GimpWireMessage  *msg,
                                          gpointer          user_data);
static void _gp_has_init_destroy         (GimpWireMessage  *msg);



void
gp_init (void)
{
  gimp_wire_register (GP_QUIT,
                      _gp_quit_read,
                      _gp_quit_write,
                      _gp_quit_destroy);
  gimp_wire_register (GP_CONFIG,
                      _gp_config_read,
                      _gp_config_write,
                      _gp_config_destroy);
  gimp_wire_register (GP_TILE_REQ,
                      _gp_tile_req_read,
                      _gp_tile_req_write,
                      _gp_tile_req_destroy);
  gimp_wire_register (GP_TILE_ACK,
                      _gp_tile_ack_read,
                      _gp_tile_ack_write,
                      _gp_tile_ack_destroy);
  gimp_wire_register (GP_TILE_DATA,
                      _gp_tile_data_read,
                      _gp_tile_data_write,
                      _gp_tile_data_destroy);
  gimp_wire_register (GP_PROC_RUN,
                      _gp_proc_run_read,
                      _gp_proc_run_write,
                      _gp_proc_run_destroy);
  gimp_wire_register (GP_PROC_RETURN,
                      _gp_proc_return_read,
                      _gp_proc_return_write,
                      _gp_proc_return_destroy);
  gimp_wire_register (GP_TEMP_PROC_RUN,
                      _gp_temp_proc_run_read,
                      _gp_temp_proc_run_write,
                      _gp_temp_proc_run_destroy);
  gimp_wire_register (GP_TEMP_PROC_RETURN,
                      _gp_temp_proc_return_read,
                      _gp_temp_proc_return_write,
                      _gp_temp_proc_return_destroy);
  gimp_wire_register (GP_PROC_INSTALL,
                      _gp_proc_install_read,
                      _gp_proc_install_write,
                      _gp_proc_install_destroy);
  gimp_wire_register (GP_PROC_UNINSTALL,
                      _gp_proc_uninstall_read,
                      _gp_proc_uninstall_write,
                      _gp_proc_uninstall_destroy);
  gimp_wire_register (GP_EXTENSION_ACK,
                      _gp_extension_ack_read,
                      _gp_extension_ack_write,
                      _gp_extension_ack_destroy);
  gimp_wire_register (GP_HAS_INIT,
                      _gp_has_init_read,
                      _gp_has_init_write,
                      _gp_has_init_destroy);
}

/* public writing API */

gboolean
gp_quit_write (GIOChannel *channel,
               gpointer    user_data)
{
  GimpWireMessage msg;

  msg.type = GP_QUIT;
  msg.data = NULL;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_config_write (GIOChannel *channel,
                 GPConfig   *config,
                 gpointer    user_data)
{
  GimpWireMessage msg;

  msg.type = GP_CONFIG;
  msg.data = config;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_tile_req_write (GIOChannel *channel,
                   GPTileReq  *tile_req,
                   gpointer    user_data)
{
  GimpWireMessage msg;

  msg.type = GP_TILE_REQ;
  msg.data = tile_req;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_tile_ack_write (GIOChannel *channel,
                   gpointer    user_data)
{
  GimpWireMessage msg;

  msg.type = GP_TILE_ACK;
  msg.data = NULL;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_tile_data_write (GIOChannel *channel,
                    GPTileData *tile_data,
                    gpointer    user_data)
{
  GimpWireMessage msg;

  msg.type = GP_TILE_DATA;
  msg.data = tile_data;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_proc_run_write (GIOChannel *channel,
                   GPProcRun  *proc_run,
                   gpointer    user_data)
{
  GimpWireMessage msg;

  msg.type = GP_PROC_RUN;
  msg.data = proc_run;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_proc_return_write (GIOChannel   *channel,
                      GPProcReturn *proc_return,
                      gpointer      user_data)
{
  GimpWireMessage msg;

  msg.type = GP_PROC_RETURN;
  msg.data = proc_return;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_temp_proc_run_write (GIOChannel *channel,
                        GPProcRun  *proc_run,
                        gpointer    user_data)
{
  GimpWireMessage msg;

  msg.type = GP_TEMP_PROC_RUN;
  msg.data = proc_run;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_temp_proc_return_write (GIOChannel   *channel,
                           GPProcReturn *proc_return,
                           gpointer      user_data)
{
  GimpWireMessage msg;

  msg.type = GP_TEMP_PROC_RETURN;
  msg.data = proc_return;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_proc_install_write (GIOChannel    *channel,
                       GPProcInstall *proc_install,
                       gpointer       user_data)
{
  GimpWireMessage msg;

  msg.type = GP_PROC_INSTALL;
  msg.data = proc_install;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_proc_uninstall_write (GIOChannel      *channel,
                         GPProcUninstall *proc_uninstall,
                         gpointer         user_data)
{
  GimpWireMessage msg;

  msg.type = GP_PROC_UNINSTALL;
  msg.data = proc_uninstall;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_extension_ack_write (GIOChannel *channel,
                        gpointer    user_data)
{
  GimpWireMessage msg;

  msg.type = GP_EXTENSION_ACK;
  msg.data = NULL;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

gboolean
gp_has_init_write (GIOChannel *channel,
                   gpointer    user_data)
{
  GimpWireMessage msg;

  msg.type = GP_HAS_INIT;
  msg.data = NULL;

  if (! gimp_wire_write_msg (channel, &msg, user_data))
    return FALSE;

  if (! gimp_wire_flush (channel, user_data))
    return FALSE;

  return TRUE;
}

/*  quit  */

static void
_gp_quit_read (GIOChannel      *channel,
               GimpWireMessage *msg,
               gpointer         user_data)
{
}

static void
_gp_quit_write (GIOChannel      *channel,
                GimpWireMessage *msg,
                gpointer         user_data)
{
}

static void
_gp_quit_destroy (GimpWireMessage *msg)
{
}

/*  config  */

static void
_gp_config_read (GIOChannel      *channel,
                 GimpWireMessage *msg,
                 gpointer         user_data)
{
  GPConfig *config = g_slice_new0 (GPConfig);

  if (! _gimp_wire_read_int32 (channel,
                               &config->tile_width, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &config->tile_height, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               (guint32 *) &config->shm_ID, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int8 (channel,
                              (guint8 *) &config->check_size, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int8 (channel,
                              (guint8 *) &config->check_type, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int8 (channel,
                              (guint8 *) &config->show_help_button, 1,
                              user_data))
    goto cleanup;
  if (! _gimp_wire_read_int8 (channel,
                              (guint8 *) &config->use_cpu_accel, 1,
                              user_data))
    goto cleanup;
  if (! _gimp_wire_read_int8 (channel,
                              (guint8 *) &config->use_opencl, 1,
                              user_data))
    goto cleanup;
  if (! _gimp_wire_read_int8 (channel,
                              (guint8 *) &config->export_profile, 1,
                              user_data))
    goto cleanup;
  if (! _gimp_wire_read_int8 (channel,
                              (guint8 *) &config->export_exif, 1,
                              user_data))
    goto cleanup;
  if (! _gimp_wire_read_int8 (channel,
                              (guint8 *) &config->export_xmp, 1,
                              user_data))
    goto cleanup;
  if (! _gimp_wire_read_int8 (channel,
                              (guint8 *) &config->export_iptc, 1,
                              user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               (guint32 *) &config->gdisp_ID, 1, user_data))
    goto cleanup;

  if (! _gimp_wire_read_string (channel,
                                &config->app_name, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &config->wm_class, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &config->display_name, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               (guint32 *) &config->monitor_number, 1,
                               user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &config->timestamp, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &config->icon_theme_dir, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int64 (channel,
                               &config->tile_cache_size, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &config->swap_path, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               (guint32 *) &config->num_processors, 1,
                               user_data))
    goto cleanup;

  msg->data = config;
  return;

 cleanup:
  g_free (config->app_name);
  g_free (config->wm_class);
  g_free (config->display_name);
  g_free (config->icon_theme_dir);
  g_free (config->swap_path);
  g_slice_free (GPConfig, config);
}

static void
_gp_config_write (GIOChannel      *channel,
                  GimpWireMessage *msg,
                  gpointer         user_data)
{
  GPConfig *config = msg->data;

  if (! _gimp_wire_write_int32 (channel,
                                &config->tile_width, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                &config->tile_height, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                (const guint32 *) &config->shm_ID, 1,
                                user_data))
    return;
  if (! _gimp_wire_write_int8 (channel,
                               (const guint8 *) &config->check_size, 1,
                               user_data))
    return;
  if (! _gimp_wire_write_int8 (channel,
                               (const guint8 *) &config->check_type, 1,
                               user_data))
    return;
  if (! _gimp_wire_write_int8 (channel,
                               (const guint8 *) &config->show_help_button, 1,
                               user_data))
    return;
  if (! _gimp_wire_write_int8 (channel,
                               (const guint8 *) &config->use_cpu_accel, 1,
                               user_data))
    return;
  if (! _gimp_wire_write_int8 (channel,
                               (const guint8 *) &config->use_opencl, 1,
                               user_data))
    return;
  if (! _gimp_wire_write_int8 (channel,
                               (const guint8 *) &config->export_profile, 1,
                               user_data))
    return;
  if (! _gimp_wire_write_int8 (channel,
                               (const guint8 *) &config->export_exif, 1,
                               user_data))
    return;
  if (! _gimp_wire_write_int8 (channel,
                               (const guint8 *) &config->export_xmp, 1,
                               user_data))
    return;
  if (! _gimp_wire_write_int8 (channel,
                               (const guint8 *) &config->export_iptc, 1,
                               user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                (const guint32 *) &config->gdisp_ID, 1,
                                user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &config->app_name, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &config->wm_class, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &config->display_name, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                (const guint32 *) &config->monitor_number, 1,
                                user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                (const guint32 *) &config->timestamp, 1,
                                user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &config->icon_theme_dir, 1, user_data))
    return;
  if (! _gimp_wire_write_int64 (channel,
                                &config->tile_cache_size, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &config->swap_path, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                (const guint32 *) &config->num_processors, 1,
                                user_data))
    return;
}

static void
_gp_config_destroy (GimpWireMessage *msg)
{
  GPConfig *config = msg->data;

  if (config)
    {
      g_free (config->app_name);
      g_free (config->wm_class);
      g_free (config->display_name);
      g_free (config->icon_theme_dir);
      g_free (config->swap_path);
      g_slice_free (GPConfig, config);
    }
}

/*  tile_req  */

static void
_gp_tile_req_read (GIOChannel      *channel,
                   GimpWireMessage *msg,
                   gpointer         user_data)
{
  GPTileReq *tile_req = g_slice_new0 (GPTileReq);

  if (! _gimp_wire_read_int32 (channel,
                               (guint32 *) &tile_req->drawable_ID, 1,
                               user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &tile_req->tile_num, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &tile_req->shadow, 1, user_data))
    goto cleanup;

  msg->data = tile_req;
  return;

 cleanup:
  g_slice_free (GPTileReq, tile_req);
  msg->data = NULL;
}

static void
_gp_tile_req_write (GIOChannel      *channel,
                    GimpWireMessage *msg,
                    gpointer         user_data)
{
  GPTileReq *tile_req = msg->data;

  if (! _gimp_wire_write_int32 (channel,
                                (const guint32 *) &tile_req->drawable_ID, 1,
                                user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                &tile_req->tile_num, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                &tile_req->shadow, 1, user_data))
    return;
}

static void
_gp_tile_req_destroy (GimpWireMessage *msg)
{
  GPTileReq *tile_req = msg->data;

  if (tile_req)
    g_slice_free (GPTileReq, msg->data);
}

/*  tile_ack  */

static void
_gp_tile_ack_read (GIOChannel      *channel,
                   GimpWireMessage *msg,
                   gpointer         user_data)
{
}

static void
_gp_tile_ack_write (GIOChannel      *channel,
                    GimpWireMessage *msg,
                    gpointer         user_data)
{
}

static void
_gp_tile_ack_destroy (GimpWireMessage *msg)
{
}

/*  tile_data  */

static void
_gp_tile_data_read (GIOChannel      *channel,
                    GimpWireMessage *msg,
                    gpointer         user_data)
{
  GPTileData *tile_data = g_slice_new0 (GPTileData);

  if (! _gimp_wire_read_int32 (channel,
                               (guint32 *) &tile_data->drawable_ID, 1,
                               user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &tile_data->tile_num, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &tile_data->shadow, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &tile_data->bpp, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &tile_data->width, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &tile_data->height, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &tile_data->use_shm, 1, user_data))
    goto cleanup;

  if (!tile_data->use_shm)
    {
      guint length = tile_data->width * tile_data->height * tile_data->bpp;

      tile_data->data = g_new (guchar, length);

      if (! _gimp_wire_read_int8 (channel,
                                  (guint8 *) tile_data->data, length,
                                  user_data))
        goto cleanup;
    }

  msg->data = tile_data;
  return;

 cleanup:
  g_free (tile_data->data);
  g_slice_free (GPTileData, tile_data);
  msg->data = NULL;
}

static void
_gp_tile_data_write (GIOChannel      *channel,
                     GimpWireMessage *msg,
                     gpointer         user_data)
{
  GPTileData *tile_data = msg->data;

  if (! _gimp_wire_write_int32 (channel,
                                (const guint32 *) &tile_data->drawable_ID, 1,
                                user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                &tile_data->tile_num, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                &tile_data->shadow, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                &tile_data->bpp, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                &tile_data->width, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                &tile_data->height, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                &tile_data->use_shm, 1, user_data))
    return;

  if (!tile_data->use_shm)
    {
      guint length = tile_data->width * tile_data->height * tile_data->bpp;

      if (! _gimp_wire_write_int8 (channel,
                                   (const guint8 *) tile_data->data, length,
                                   user_data))
        return;
    }
}

static void
_gp_tile_data_destroy (GimpWireMessage *msg)
{
  GPTileData *tile_data = msg->data;

  if  (tile_data)
    {
      if (tile_data->data)
        {
          g_free (tile_data->data);
          tile_data->data = NULL;
        }

      g_slice_free (GPTileData, tile_data);
    }
}

/*  proc_run  */

static void
_gp_proc_run_read (GIOChannel      *channel,
                   GimpWireMessage *msg,
                   gpointer         user_data)
{
  GPProcRun *proc_run = g_slice_new0 (GPProcRun);

  if (! _gimp_wire_read_string (channel, &proc_run->name, 1, user_data))
    goto cleanup;

  _gp_params_read (channel,
                   &proc_run->params, (guint *) &proc_run->nparams,
                   user_data);

  msg->data = proc_run;
  return;

 cleanup:
  g_slice_free (GPProcRun, proc_run);
  msg->data = NULL;
}

static void
_gp_proc_run_write (GIOChannel      *channel,
                    GimpWireMessage *msg,
                    gpointer         user_data)
{
  GPProcRun *proc_run = msg->data;

  if (! _gimp_wire_write_string (channel, &proc_run->name, 1, user_data))
    return;

  _gp_params_write (channel, proc_run->params, proc_run->nparams, user_data);
}

static void
_gp_proc_run_destroy (GimpWireMessage *msg)
{
  GPProcRun *proc_run = msg->data;

  if (proc_run)
    {
      _gp_params_destroy (proc_run->params, proc_run->nparams);

      g_free (proc_run->name);
      g_slice_free (GPProcRun, proc_run);
    }
}

/*  proc_return  */

static void
_gp_proc_return_read (GIOChannel      *channel,
                      GimpWireMessage *msg,
                      gpointer         user_data)
{
  GPProcReturn *proc_return = g_slice_new0 (GPProcReturn);

  if (! _gimp_wire_read_string (channel, &proc_return->name, 1, user_data))
    goto cleanup;

  _gp_params_read (channel,
                   &proc_return->params, (guint *) &proc_return->nparams,
                   user_data);

  msg->data = proc_return;
  return;

 cleanup:
  g_slice_free (GPProcReturn, proc_return);
  msg->data = NULL;
}

static void
_gp_proc_return_write (GIOChannel      *channel,
                       GimpWireMessage *msg,
                       gpointer         user_data)
{
  GPProcReturn *proc_return = msg->data;

  if (! _gimp_wire_write_string (channel, &proc_return->name, 1, user_data))
    return;

  _gp_params_write (channel,
                    proc_return->params, proc_return->nparams, user_data);
}

static void
_gp_proc_return_destroy (GimpWireMessage *msg)
{
  GPProcReturn *proc_return = msg->data;

  if (proc_return)
    {
      _gp_params_destroy (proc_return->params, proc_return->nparams);

      g_free (proc_return->name);
      g_slice_free (GPProcReturn, proc_return);
    }
}

/*  temp_proc_run  */

static void
_gp_temp_proc_run_read (GIOChannel      *channel,
                        GimpWireMessage *msg,
                        gpointer         user_data)
{
  _gp_proc_run_read (channel, msg, user_data);
}

static void
_gp_temp_proc_run_write (GIOChannel      *channel,
                         GimpWireMessage *msg,
                         gpointer         user_data)
{
  _gp_proc_run_write (channel, msg, user_data);
}

static void
_gp_temp_proc_run_destroy (GimpWireMessage *msg)
{
  _gp_proc_run_destroy (msg);
}

/*  temp_proc_return  */

static void
_gp_temp_proc_return_read (GIOChannel      *channel,
                           GimpWireMessage *msg,
                           gpointer         user_data)
{
  _gp_proc_return_read (channel, msg, user_data);
}

static void
_gp_temp_proc_return_write (GIOChannel      *channel,
                            GimpWireMessage *msg,
                            gpointer         user_data)
{
  _gp_proc_return_write (channel, msg, user_data);
}

static void
_gp_temp_proc_return_destroy (GimpWireMessage *msg)
{
  _gp_proc_return_destroy (msg);
}

/*  proc_install  */

static gboolean
_gp_param_def_read (GIOChannel *channel,
                    GPParamDef *param_def,
                    gpointer    user_data)
{
  if (! _gimp_wire_read_int32 (channel,
                               &param_def->param_def_type, 1,
                               user_data))
    return FALSE;

  if (! _gimp_wire_read_string (channel,
                                &param_def->type_name, 1,
                                user_data))
    return FALSE;

  if (! _gimp_wire_read_string (channel,
                                &param_def->name, 1,
                                user_data))
    return FALSE;

  if (! _gimp_wire_read_string (channel,
                                &param_def->nick, 1,
                                user_data))
    return FALSE;

  if (! _gimp_wire_read_string (channel,
                                &param_def->blurb, 1,
                                user_data))
    return FALSE;

  switch (param_def->param_def_type)
    {
    case GP_PARAM_DEF_TYPE_DEFAULT:
      break;

    case GP_PARAM_DEF_TYPE_INT:
      if (! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_int.min_val, 1,
                                   user_data) ||
          ! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_int.max_val, 1,
                                   user_data) ||
          ! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_int.default_val, 1,
                                   user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_UNIT:
      if (! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_unit.allow_pixels, 1,
                                   user_data) ||
          ! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_unit.allow_percent, 1,
                                   user_data) ||
          ! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_unit.default_val, 1,
                                   user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_ENUM:
      if (! _gimp_wire_read_string (channel,
                                    &param_def->meta.m_enum.type_name, 1,
                                    user_data) ||
          ! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_enum.default_val, 1,
                                   user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_BOOLEAN:
      if (! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_boolean.default_val, 1,
                                   user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_FLOAT:
      if (! _gimp_wire_read_double (channel,
                                    &param_def->meta.m_float.min_val, 1,
                                    user_data) ||
          ! _gimp_wire_read_double (channel,
                                    &param_def->meta.m_float.max_val, 1,
                                    user_data) ||
          ! _gimp_wire_read_double (channel,
                                    &param_def->meta.m_float.default_val, 1,
                                    user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_STRING:
      if (! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_string.allow_non_utf8, 1,
                                   user_data) ||
          ! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_string.null_ok, 1,
                                   user_data) ||
          ! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_string.non_empty, 1,
                                   user_data) ||
          ! _gimp_wire_read_string (channel,
                                    &param_def->meta.m_string.default_val, 1,
                                    user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_COLOR:
      if (! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_color.has_alpha, 1,
                                   user_data) ||
          ! _gimp_wire_read_color (channel,
                                   &param_def->meta.m_color.default_val, 1,
                                   user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_ID:
      if (! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &param_def->meta.m_id.none_ok, 1,
                                   user_data))
        return FALSE;
      break;
    }

  return TRUE;
}

static void
_gp_param_def_destroy (GPParamDef *param_def)
{
  g_free (param_def->type_name);
  g_free (param_def->name);
  g_free (param_def->nick);
  g_free (param_def->blurb);

  switch (param_def->param_def_type)
    {
    case GP_PARAM_DEF_TYPE_DEFAULT:
    case GP_PARAM_DEF_TYPE_INT:
    case GP_PARAM_DEF_TYPE_UNIT:
      break;

    case GP_PARAM_DEF_TYPE_ENUM:
      g_free (param_def->meta.m_enum.type_name);
      break;

    case GP_PARAM_DEF_TYPE_BOOLEAN:
    case GP_PARAM_DEF_TYPE_FLOAT:
      break;

    case GP_PARAM_DEF_TYPE_STRING:
      g_free (param_def->meta.m_string.default_val);
      break;

    case GP_PARAM_DEF_TYPE_COLOR:
    case GP_PARAM_DEF_TYPE_ID:
      break;
    }
}

static void
_gp_proc_install_read (GIOChannel      *channel,
                       GimpWireMessage *msg,
                       gpointer         user_data)
{
  GPProcInstall *proc_install = g_slice_new0 (GPProcInstall);
  gint           i;

  if (! _gimp_wire_read_string (channel,
                                &proc_install->name, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &proc_install->blurb, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &proc_install->help, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &proc_install->authors, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &proc_install->copyright, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &proc_install->date, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &proc_install->menu_label, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &proc_install->image_types, 1, user_data))
    goto cleanup;

  if (! _gimp_wire_read_int32 (channel,
                               &proc_install->type, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &proc_install->nparams, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               &proc_install->nreturn_vals, 1, user_data))
    goto cleanup;

  proc_install->params = g_new0 (GPParamDef, proc_install->nparams);

  for (i = 0; i < proc_install->nparams; i++)
    {
      if (! _gp_param_def_read (channel,
                                &proc_install->params[i],
                                user_data))
        goto cleanup;
    }

  proc_install->return_vals = g_new0 (GPParamDef, proc_install->nreturn_vals);

  for (i = 0; i < proc_install->nreturn_vals; i++)
    {
      if (! _gp_param_def_read (channel,
                                &proc_install->return_vals[i],
                                user_data))
        goto cleanup;
    }

  msg->data = proc_install;
  return;

 cleanup:
  g_free (proc_install->name);
  g_free (proc_install->blurb);
  g_free (proc_install->help);
  g_free (proc_install->authors);
  g_free (proc_install->copyright);
  g_free (proc_install->date);
  g_free (proc_install->menu_label);
  g_free (proc_install->image_types);

  if (proc_install->params)
    {
      for (i = 0; i < proc_install->nparams; i++)
        {
          if (! proc_install->params[i].name)
            break;

          _gp_param_def_destroy (&proc_install->params[i]);
        }

      g_free (proc_install->params);
    }

  if (proc_install->return_vals)
    {
      for (i = 0; i < proc_install->nreturn_vals; i++)
        {
          if (! proc_install->return_vals[i].name)
            break;

          _gp_param_def_destroy (&proc_install->return_vals[i]);
        }

      g_free (proc_install->return_vals);
    }

  g_slice_free (GPProcInstall, proc_install);
  msg->data = NULL;
}

static gboolean
_gp_param_def_write (GIOChannel *channel,
                     GPParamDef *param_def,
                     gpointer    user_data)
{
  if (! _gimp_wire_write_int32 (channel,
                                &param_def->param_def_type, 1,
                                user_data))
    return FALSE;

  if (! _gimp_wire_write_string (channel,
                                 &param_def->type_name, 1,
                                 user_data))
    return FALSE;

  if (! _gimp_wire_write_string (channel,
                                 &param_def->name, 1,
                                 user_data))
    return FALSE;

  if (! _gimp_wire_write_string (channel,
                                 &param_def->nick, 1,
                                 user_data))
    return FALSE;

  if (! _gimp_wire_write_string (channel,
                                 &param_def->blurb, 1,
                                 user_data))
    return FALSE;

  switch (param_def->param_def_type)
    {
    case GP_PARAM_DEF_TYPE_DEFAULT:
      break;

    case GP_PARAM_DEF_TYPE_INT:
      if (! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_int.min_val, 1,
                                    user_data) ||
          ! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_int.max_val, 1,
                                    user_data) ||
          ! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_int.default_val, 1,
                                    user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_UNIT:
      if (! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_unit.allow_pixels, 1,
                                    user_data) ||
          ! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_unit.allow_percent, 1,
                                    user_data) ||
          ! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_unit.default_val, 1,
                                    user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_ENUM:
      if (! _gimp_wire_write_string (channel,
                                     &param_def->meta.m_enum.type_name, 1,
                                     user_data) ||
          ! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_enum.default_val, 1,
                                    user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_BOOLEAN:
      if (! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_boolean.default_val, 1,
                                    user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_FLOAT:
      if (! _gimp_wire_write_double (channel,
                                     &param_def->meta.m_float.min_val, 1,
                                     user_data) ||
          ! _gimp_wire_write_double (channel,
                                     &param_def->meta.m_float.max_val, 1,
                                     user_data) ||
          ! _gimp_wire_write_double (channel,
                                     &param_def->meta.m_float.default_val, 1,
                                     user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_STRING:
      if (! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_string.allow_non_utf8, 1,
                                    user_data) ||
          ! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_string.null_ok, 1,
                                    user_data) ||
          ! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_string.non_empty, 1,
                                    user_data) ||
          ! _gimp_wire_write_string (channel,
                                     &param_def->meta.m_string.default_val, 1,
                                     user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_COLOR:
      if (! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_color.has_alpha, 1,
                                    user_data) ||
          ! _gimp_wire_write_color (channel,
                                    &param_def->meta.m_color.default_val, 1,
                                    user_data))
        return FALSE;
      break;

    case GP_PARAM_DEF_TYPE_ID:
      if (! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &param_def->meta.m_id.none_ok, 1,
                                    user_data))
        return FALSE;
      break;
    }

  return TRUE;
}

  static void
_gp_proc_install_write (GIOChannel      *channel,
                        GimpWireMessage *msg,
                        gpointer         user_data)
{
  GPProcInstall *proc_install = msg->data;
  gint           i;

  if (! _gimp_wire_write_string (channel,
                                 &proc_install->name, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &proc_install->blurb, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &proc_install->help, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &proc_install->authors, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &proc_install->copyright, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &proc_install->date, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &proc_install->menu_label, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &proc_install->image_types, 1, user_data))
    return;

  if (! _gimp_wire_write_int32 (channel,
                                &proc_install->type, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                &proc_install->nparams, 1, user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                &proc_install->nreturn_vals, 1, user_data))
    return;

  for (i = 0; i < proc_install->nparams; i++)
    {
      if (! _gp_param_def_write (channel,
                                 &proc_install->params[i],
                                 user_data))
        return;
    }

  for (i = 0; i < proc_install->nreturn_vals; i++)
    {
      if (! _gp_param_def_write (channel,
                                 &proc_install->return_vals[i],
                                 user_data))
        return;
    }
}

static void
_gp_proc_install_destroy (GimpWireMessage *msg)
{
  GPProcInstall *proc_install = msg->data;

  if (proc_install)
    {
      gint i;

      g_free (proc_install->name);
      g_free (proc_install->blurb);
      g_free (proc_install->help);
      g_free (proc_install->authors);
      g_free (proc_install->copyright);
      g_free (proc_install->date);
      g_free (proc_install->menu_label);
      g_free (proc_install->image_types);

      for (i = 0; i < proc_install->nparams; i++)
        {
          _gp_param_def_destroy (&proc_install->params[i]);
        }

      for (i = 0; i < proc_install->nreturn_vals; i++)
        {
          _gp_param_def_destroy (&proc_install->return_vals[i]);
        }

      g_free (proc_install->params);
      g_free (proc_install->return_vals);
      g_slice_free (GPProcInstall, proc_install);
    }
}

/*  proc_uninstall  */

static void
_gp_proc_uninstall_read (GIOChannel      *channel,
                         GimpWireMessage *msg,
                         gpointer         user_data)
{
  GPProcUninstall *proc_uninstall = g_slice_new0 (GPProcUninstall);

  if (! _gimp_wire_read_string (channel, &proc_uninstall->name, 1, user_data))
    goto cleanup;

  msg->data = proc_uninstall;
  return;

 cleanup:
  g_slice_free (GPProcUninstall, proc_uninstall);
}

static void
_gp_proc_uninstall_write (GIOChannel      *channel,
                          GimpWireMessage *msg,
                          gpointer         user_data)
{
  GPProcUninstall *proc_uninstall = msg->data;

  if (! _gimp_wire_write_string (channel, &proc_uninstall->name, 1, user_data))
    return;
}

static void
_gp_proc_uninstall_destroy (GimpWireMessage *msg)
{
  GPProcUninstall *proc_uninstall = msg->data;

  if (proc_uninstall)
    {
      g_free (proc_uninstall->name);
      g_slice_free (GPProcUninstall, proc_uninstall);
    }
}

/*  extension_ack  */

static void
_gp_extension_ack_read (GIOChannel      *channel,
                        GimpWireMessage *msg,
                        gpointer         user_data)
{
}

static void
_gp_extension_ack_write (GIOChannel      *channel,
                         GimpWireMessage *msg,
                         gpointer         user_data)
{
}

static void
_gp_extension_ack_destroy (GimpWireMessage *msg)
{
}

/*  params  */

static void
_gp_params_read (GIOChannel  *channel,
                 GPParam    **params,
                 guint       *nparams,
                 gpointer     user_data)
{
  gint i;

  if (! _gimp_wire_read_int32 (channel, (guint32 *) nparams, 1, user_data))
    return;

  if (*nparams == 0)
    {
      *params = NULL;
      return;
    }

  *params = g_try_new0 (GPParam, *nparams);

  /* We may read crap on the wire (and as a consequence try to allocate
   * far too much), which would be a plug-in error.
   */
  if (*params == NULL)
    {
      /* Output on stderr but no WARNING/CRITICAL. This is likely a
       * plug-in bug sending bogus data, not a core bug.
       */
      g_printerr ("%s: failed to allocate %u parameters\n",
                  G_STRFUNC, *nparams);
      *nparams = 0;
      return;
    }

  for (i = 0; i < *nparams; i++)
    {
      if (! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &(*params)[i].param_type, 1,
                                   user_data))
        goto cleanup;

      if (! _gimp_wire_read_string (channel,
                                    &(*params)[i].type_name, 1,
                                    user_data))
        return;

      switch ((*params)[i].param_type)
        {
        case GP_PARAM_TYPE_INT:
          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) &(*params)[i].data.d_int, 1,
                                       user_data))
            goto cleanup;
          break;

        case GP_PARAM_TYPE_FLOAT:
          if (! _gimp_wire_read_double (channel,
                                        &(*params)[i].data.d_float, 1,
                                        user_data))
            goto cleanup;
          break;

        case GP_PARAM_TYPE_STRING:
          if (! _gimp_wire_read_string (channel,
                                        &(*params)[i].data.d_string, 1,
                                        user_data))
            goto cleanup;
          break;

        case GP_PARAM_TYPE_COLOR:
          if (! _gimp_wire_read_color (channel,
                                       &(*params)[i].data.d_color, 1,
                                       user_data))
            goto cleanup;
          break;

        case GP_PARAM_TYPE_ARRAY:
          if (! _gimp_wire_read_int32 (channel,
                                       &(*params)[i].data.d_array.size, 1,
                                       user_data))
            goto cleanup;

          (*params)[i].data.d_array.data = g_new0 (guint8,
                                                   (*params)[i].data.d_array.size);

          if (! _gimp_wire_read_int8 (channel,
                                      (*params)[i].data.d_array.data,
                                      (*params)[i].data.d_array.size,
                                      user_data))
            {
              g_free ((*params)[i].data.d_array.data);
              (*params)[i].data.d_array.data = NULL;
              goto cleanup;
            }
          break;

        case GP_PARAM_TYPE_STRING_ARRAY:
          if (! _gimp_wire_read_int32 (channel,
                                       &(*params)[i].data.d_string_array.size, 1,
                                       user_data))
            goto cleanup;

          (*params)[i].data.d_string_array.data = g_new0 (gchar *,
                                                          (*params)[i].data.d_array.size);

          if (! _gimp_wire_read_string (channel,
                                        (*params)[i].data.d_string_array.data,
                                        (*params)[i].data.d_array.size,
                                        user_data))
            {
              gint j;

              for (j = 0; j < (*params)[i].data.d_array.size; j++)
                g_free ((*params)[i].data.d_string_array.data[j]);
              g_free ((*params)[i].data.d_string_array.data);
              (*params)[i].data.d_string_array.data = NULL;
              goto cleanup;
            }
          break;

        case GP_PARAM_TYPE_PARASITE:
          if (! _gimp_wire_read_string (channel,
                                        &(*params)[i].data.d_parasite.name, 1,
                                        user_data))
            goto cleanup;
          if ((*params)[i].data.d_parasite.name == NULL)
            {
              /* we have a null parasite */
              (*params)[i].data.d_parasite.data = NULL;
              break;
            }
          if (! _gimp_wire_read_int32 (channel,
                                       &((*params)[i].data.d_parasite.flags), 1,
                                       user_data))
            goto cleanup;
          if (! _gimp_wire_read_int32 (channel,
                                       &((*params)[i].data.d_parasite.size), 1,
                                       user_data))
            goto cleanup;
          if ((*params)[i].data.d_parasite.size > 0)
            {
              (*params)[i].data.d_parasite.data =
                g_malloc ((*params)[i].data.d_parasite.size);
              if (! _gimp_wire_read_int8 (channel,
                                          (*params)[i].data.d_parasite.data,
                                          (*params)[i].data.d_parasite.size,
                                          user_data))
                {
                  g_free ((*params)[i].data.d_parasite.data);
                  goto cleanup;
                }
            }
          else
            (*params)[i].data.d_parasite.data = NULL;
          break;
        }
    }

  return;

 cleanup:
  *nparams = 0;
  g_free (*params);
  *params = NULL;
}

static void
_gp_params_write (GIOChannel *channel,
                  GPParam    *params,
                  gint        nparams,
                  gpointer    user_data)
{
  gint i;

  if (! _gimp_wire_write_int32 (channel,
                                (const guint32 *) &nparams, 1, user_data))
    return;

  for (i = 0; i < nparams; i++)
    {
      if (! _gimp_wire_write_int32 (channel,
                                    (const guint32 *) &params[i].param_type, 1,
                                    user_data))
        return;

      if (! _gimp_wire_write_string (channel,
                                     &params[i].type_name, 1,
                                     user_data))
        return;

      switch (params[i].param_type)
        {
        case GP_PARAM_TYPE_INT:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_int, 1,
                                        user_data))
            return;
          break;

        case GP_PARAM_TYPE_FLOAT:
          if (! _gimp_wire_write_double (channel,
                                         (const gdouble *) &params[i].data.d_float, 1,
                                         user_data))
            return;
          break;

        case GP_PARAM_TYPE_STRING:
          if (! _gimp_wire_write_string (channel,
                                         &params[i].data.d_string, 1,
                                         user_data))
            return;
          break;

        case GP_PARAM_TYPE_COLOR:
          if (! _gimp_wire_write_color (channel,
                                        &params[i].data.d_color, 1,
                                        user_data))
            return;
          break;

        case GP_PARAM_TYPE_ARRAY:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_array.size, 1,
                                        user_data) ||
              ! _gimp_wire_write_int8 (channel,
                                       (const guint8 *) params[i].data.d_array.data,
                                       params[i].data.d_array.size,
                                       user_data))
            return;
          break;

        case GP_PARAM_TYPE_STRING_ARRAY:
          if (! _gimp_wire_write_int32 (channel,
                                        &params[i].data.d_string_array.size, 1,
                                        user_data) ||
              ! _gimp_wire_write_string (channel,
                                         params[i].data.d_string_array.data,
                                         params[i].data.d_string_array.size,
                                         user_data))
            return;
          break;

        case GP_PARAM_TYPE_PARASITE:
          {
            GimpParasite *p = &params[i].data.d_parasite;

            if (p->name == NULL)
              {
                /* write a null string to signal a null parasite */
                _gimp_wire_write_string (channel,  &p->name, 1, user_data);
                break;
              }

            if (! _gimp_wire_write_string (channel, &p->name, 1, user_data))
              return;
            if (! _gimp_wire_write_int32 (channel, &p->flags, 1, user_data))
              return;
            if (! _gimp_wire_write_int32 (channel, &p->size, 1, user_data))
              return;
            if (p->size > 0)
              {
                if (! _gimp_wire_write_int8 (channel,
                                             p->data, p->size, user_data))
                  return;
              }
          }
          break;
        }
    }
}

static void
_gp_params_destroy (GPParam *params,
                    gint     nparams)
{
  gint i;

  for (i = 0; i < nparams; i++)
    {
      g_free (params[i].type_name);

      switch (params[i].param_type)
        {
        case GP_PARAM_TYPE_INT:
        case GP_PARAM_TYPE_FLOAT:
          break;

        case GP_PARAM_TYPE_STRING:
          g_free (params[i].data.d_string);
          break;

        case GP_PARAM_TYPE_COLOR:
          break;

        case GP_PARAM_TYPE_ARRAY:
          g_free (params[i].data.d_array.data);
          break;

        case GP_PARAM_TYPE_STRING_ARRAY:
          if (params[i].data.d_string_array.size > 0 &&
              params[i].data.d_string_array.data)
            {
              gint j;

              for (j = 0; j < params[i].data.d_string_array.size; j++)
                g_free (params[i].data.d_string_array.data[j]);

              g_free (params[i].data.d_string_array.data);
            }
          break;

        case GP_PARAM_TYPE_PARASITE:
          if (params[i].data.d_parasite.name)
            g_free (params[i].data.d_parasite.name);
          if (params[i].data.d_parasite.data)
            g_free (params[i].data.d_parasite.data);
          break;
        }
    }

  g_free (params);
}

/* has_init */

static void
_gp_has_init_read (GIOChannel      *channel,
                   GimpWireMessage *msg,
                   gpointer         user_data)
{
}

static void
_gp_has_init_write (GIOChannel      *channel,
                    GimpWireMessage *msg,
                    gpointer         user_data)
{
}

static void
_gp_has_init_destroy (GimpWireMessage *msg)
{
}
