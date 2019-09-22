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
                               &config->version, 1, user_data))
    goto cleanup;
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
  if (! _gimp_wire_read_int8 (channel,
                              (guint8 *) &config->export_profile, 1,
                              user_data))
    goto cleanup;
  if (! _gimp_wire_read_int8 (channel,
                              (guint8 *) &config->show_tooltips, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_int32 (channel,
                               (guint32 *) &config->min_colors, 1, user_data))
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

  if (config->version < 0x0017)
    goto end;

  if (! _gimp_wire_read_string (channel,
                                &config->icon_theme_dir, 1, user_data))
    goto cleanup;

  if (config->version < 0x0019)
    goto end;

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

  if (config->version < 0x001A)
    goto end;

  if (! _gimp_wire_read_string (channel,
                                &config->swap_compression, 1, user_data))
    goto cleanup;

 end:
  msg->data = config;
  return;

 cleanup:
  g_free (config->app_name);
  g_free (config->wm_class);
  g_free (config->display_name);
  g_free (config->icon_theme_dir);
  g_free (config->swap_path);
  g_free (config->swap_compression);
  g_slice_free (GPConfig, config);
}

static void
_gp_config_write (GIOChannel      *channel,
                  GimpWireMessage *msg,
                  gpointer         user_data)
{
  GPConfig *config = msg->data;

  if (! _gimp_wire_write_int32 (channel,
                                &config->version, 1, user_data))
    return;
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
  if (! _gimp_wire_write_int8 (channel,
                               (const guint8 *) &config->export_profile, 1,
                               user_data))
    return;
  if (! _gimp_wire_write_int8 (channel,
                               (const guint8 *) &config->show_tooltips, 1,
                               user_data))
    return;
  if (! _gimp_wire_write_int32 (channel,
                                (const guint32 *) &config->min_colors, 1,
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

  if (config->version < 0x0017)
    return;

  if (! _gimp_wire_write_string (channel,
                                 &config->icon_theme_dir, 1, user_data))
    return;

  if (config->version < 0x0019)
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

  if (config->version < 0x001A)
    return;

  if (! _gimp_wire_write_string (channel,
                                 &config->swap_compression, 1, user_data))
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
      g_free (config->swap_compression);
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
      gp_params_destroy (proc_run->params, proc_run->nparams);

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
      gp_params_destroy (proc_return->params, proc_return->nparams);

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
                                &proc_install->author, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &proc_install->copyright, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &proc_install->date, 1, user_data))
    goto cleanup;
  if (! _gimp_wire_read_string (channel,
                                &proc_install->menu_path, 1, user_data))
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
      if (! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &proc_install->params[i].type, 1,
                                   user_data))
        goto cleanup;
      if (! _gimp_wire_read_string (channel,
                                    &proc_install->params[i].name, 1,
                                    user_data))
        goto cleanup;
      if (! _gimp_wire_read_string (channel,
                                    &proc_install->params[i].description, 1,
                                    user_data))
        goto cleanup;
    }

  proc_install->return_vals = g_new0 (GPParamDef, proc_install->nreturn_vals);

  for (i = 0; i < proc_install->nreturn_vals; i++)
    {
      if (! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &proc_install->return_vals[i].type, 1,
                                   user_data))
        goto cleanup;
      if (! _gimp_wire_read_string (channel,
                                    &proc_install->return_vals[i].name, 1,
                                    user_data))
        goto cleanup;
      if (! _gimp_wire_read_string (channel,
                                    &proc_install->return_vals[i].description, 1,
                                    user_data))
        goto cleanup;
    }

  msg->data = proc_install;
  return;

 cleanup:
  g_free (proc_install->name);
  g_free (proc_install->blurb);
  g_free (proc_install->help);
  g_free (proc_install->author);
  g_free (proc_install->copyright);
  g_free (proc_install->date);
  g_free (proc_install->menu_path);
  g_free (proc_install->image_types);

  if (proc_install->params)
    {
      for (i = 0; i < proc_install->nparams; i++)
        {
          if (!proc_install->params[i].name)
            break;

          g_free (proc_install->params[i].name);
          g_free (proc_install->params[i].description);
        }

      g_free (proc_install->params);
    }

  if (proc_install->return_vals)
    {
      for (i = 0; i < proc_install->nreturn_vals; i++)
        {
          if (!proc_install->return_vals[i].name)
            break;

          g_free (proc_install->return_vals[i].name);
          g_free (proc_install->return_vals[i].description);
        }

      g_free (proc_install->return_vals);
    }

  g_slice_free (GPProcInstall, proc_install);
  msg->data = NULL;
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
                                 &proc_install->author, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &proc_install->copyright, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &proc_install->date, 1, user_data))
    return;
  if (! _gimp_wire_write_string (channel,
                                 &proc_install->menu_path, 1, user_data))
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
      if (! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &proc_install->params[i].type, 1,
                                    user_data))
        return;
      if (! _gimp_wire_write_string (channel,
                                     &proc_install->params[i].name, 1,
                                     user_data))
        return;
      if (! _gimp_wire_write_string (channel,
                                     &proc_install->params[i].description, 1,
                                     user_data))
        return;
    }

  for (i = 0; i < proc_install->nreturn_vals; i++)
    {
      if (! _gimp_wire_write_int32 (channel,
                                    (guint32 *) &proc_install->return_vals[i].type, 1,
                                    user_data))
        return;
      if (! _gimp_wire_write_string (channel,
                                     &proc_install->return_vals[i].name, 1,
                                     user_data))
        return;
      if (! _gimp_wire_write_string (channel,
                                     &proc_install->return_vals[i].description, 1,
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
      g_free (proc_install->author);
      g_free (proc_install->copyright);
      g_free (proc_install->date);
      g_free (proc_install->menu_path);
      g_free (proc_install->image_types);

      for (i = 0; i < proc_install->nparams; i++)
        {
          g_free (proc_install->params[i].name);
          g_free (proc_install->params[i].description);
        }

      for (i = 0; i < proc_install->nreturn_vals; i++)
        {
          g_free (proc_install->return_vals[i].name);
          g_free (proc_install->return_vals[i].description);
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
  gint i, j;

  if (! _gimp_wire_read_int32 (channel, (guint32 *) nparams, 1, user_data))
    return;

  if (*nparams == 0)
    {
      *params = NULL;
      return;
    }

  *params = g_new0 (GPParam, *nparams);

  for (i = 0; i < *nparams; i++)
    {
      if (! _gimp_wire_read_int32 (channel,
                                   (guint32 *) &(*params)[i].type, 1,
                                   user_data))
        goto cleanup;

      switch ((*params)[i].type)
        {
        case GIMP_PDB_INT32:
          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) &(*params)[i].data.d_int32, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_INT16:
          if (! _gimp_wire_read_int16 (channel,
                                       (guint16 *) &(*params)[i].data.d_int16, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_INT8:
          if (! _gimp_wire_read_int8 (channel,
                                      &(*params)[i].data.d_int8, 1,
                                      user_data))
            goto cleanup;
          break;

        case GIMP_PDB_FLOAT:
          if (! _gimp_wire_read_double (channel,
                                        &(*params)[i].data.d_float, 1,
                                        user_data))
            goto cleanup;
          break;

        case GIMP_PDB_STRING:
          if (! _gimp_wire_read_string (channel,
                                        &(*params)[i].data.d_string, 1,
                                        user_data))
            goto cleanup;
          break;

        case GIMP_PDB_INT32ARRAY:
          (*params)[i-1].data.d_int32 = MAX (0, (*params)[i-1].data.d_int32);
          (*params)[i].data.d_int32array = g_new (gint32,
                                                  (*params)[i-1].data.d_int32);

          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) (*params)[i].data.d_int32array,
                                       (*params)[i-1].data.d_int32,
                                       user_data))
            {
              g_free ((*params)[i].data.d_int32array);
              goto cleanup;
            }
          break;

        case GIMP_PDB_INT16ARRAY:
          (*params)[i-1].data.d_int32 = MAX (0, (*params)[i-1].data.d_int32);
          (*params)[i].data.d_int16array = g_new (gint16,
                                                  (*params)[i-1].data.d_int32);
          if (! _gimp_wire_read_int16 (channel,
                                       (guint16 *) (*params)[i].data.d_int16array,
                                       (*params)[i-1].data.d_int32,
                                       user_data))
            {
              g_free ((*params)[i].data.d_int16array);
              goto cleanup;
            }
          break;

        case GIMP_PDB_INT8ARRAY:
          (*params)[i-1].data.d_int32 = MAX (0, (*params)[i-1].data.d_int32);
          (*params)[i].data.d_int8array = g_new (guint8,
                                                 (*params)[i-1].data.d_int32);
          if (! _gimp_wire_read_int8 (channel,
                                      (*params)[i].data.d_int8array,
                                      (*params)[i-1].data.d_int32,
                                      user_data))
            {
              g_free ((*params)[i].data.d_int8array);
              goto cleanup;
            }
          break;

        case GIMP_PDB_FLOATARRAY:
          (*params)[i-1].data.d_int32 = MAX (0, (*params)[i-1].data.d_int32);
          (*params)[i].data.d_floatarray = g_new (gdouble,
                                                  (*params)[i-1].data.d_int32);
          if (! _gimp_wire_read_double (channel,
                                        (*params)[i].data.d_floatarray,
                                        (*params)[i-1].data.d_int32,
                                        user_data))
            {
              g_free ((*params)[i].data.d_floatarray);
              goto cleanup;
            }
          break;

        case GIMP_PDB_STRINGARRAY:
          (*params)[i-1].data.d_int32 = MAX (0, (*params)[i-1].data.d_int32);
          (*params)[i].data.d_stringarray = g_new0 (gchar *,
                                                    (*params)[i-1].data.d_int32);
          if (! _gimp_wire_read_string (channel,
                                        (*params)[i].data.d_stringarray,
                                        (*params)[i-1].data.d_int32,
                                        user_data))
            {
              for (j = 0; j < (*params)[i-1].data.d_int32; j++)
                g_free (((*params)[i].data.d_stringarray)[j]);
              g_free ((*params)[i].data.d_stringarray);
              goto cleanup;
            }
          break;

        case GIMP_PDB_COLOR:
          if (! _gimp_wire_read_color (channel,
                                       &(*params)[i].data.d_color, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_ITEM:
          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) &(*params)[i].data.d_item, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_DISPLAY:
          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) &(*params)[i].data.d_display, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_IMAGE:
          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) &(*params)[i].data.d_image, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_LAYER:
          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) &(*params)[i].data.d_layer, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_CHANNEL:
          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) &(*params)[i].data.d_channel, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_DRAWABLE:
          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) &(*params)[i].data.d_drawable, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_SELECTION:
          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) &(*params)[i].data.d_selection, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_COLORARRAY:
          (*params)[i].data.d_colorarray = g_new (GimpRGB,
                                                  (*params)[i-1].data.d_int32);
          if (! _gimp_wire_read_color (channel,
                                       (*params)[i].data.d_colorarray,
                                       (*params)[i-1].data.d_int32,
                                       user_data))
            {
              g_free ((*params)[i].data.d_colorarray);
              goto cleanup;
            }
          break;

        case GIMP_PDB_VECTORS:
          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) &(*params)[i].data.d_vectors, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_PARASITE:
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

        case GIMP_PDB_STATUS:
          if (! _gimp_wire_read_int32 (channel,
                                       (guint32 *) &(*params)[i].data.d_status, 1,
                                       user_data))
            goto cleanup;
          break;

        case GIMP_PDB_END:
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
                                    (const guint32 *) &params[i].type, 1,
                                    user_data))
        return;

      switch (params[i].type)
        {
        case GIMP_PDB_INT32:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_int32, 1,
                                        user_data))
            return;
          break;

        case GIMP_PDB_INT16:
          if (! _gimp_wire_write_int16 (channel,
                                        (const guint16 *) &params[i].data.d_int16, 1,
                                        user_data))
            return;
          break;

        case GIMP_PDB_INT8:
          if (! _gimp_wire_write_int8 (channel,
                                       (const guint8 *) &params[i].data.d_int8, 1,
                                       user_data))
            return;
          break;

        case GIMP_PDB_FLOAT:
          if (! _gimp_wire_write_double (channel,
                                         &params[i].data.d_float, 1,
                                         user_data))
            return;
          break;

        case GIMP_PDB_STRING:
          if (! _gimp_wire_write_string (channel,
                                         &params[i].data.d_string, 1,
                                         user_data))
            return;
          break;

        case GIMP_PDB_INT32ARRAY:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) params[i].data.d_int32array,
                                        params[i-1].data.d_int32,
                                        user_data))
            return;
          break;

        case GIMP_PDB_INT16ARRAY:
          if (! _gimp_wire_write_int16 (channel,
                                        (const guint16 *) params[i].data.d_int16array,
                                        params[i-1].data.d_int32,
                                        user_data))
            return;
          break;

        case GIMP_PDB_INT8ARRAY:
          if (! _gimp_wire_write_int8 (channel,
                                       (const guint8 *) params[i].data.d_int8array,
                                       params[i-1].data.d_int32,
                                       user_data))
            return;
          break;

        case GIMP_PDB_FLOATARRAY:
          if (! _gimp_wire_write_double (channel,
                                         params[i].data.d_floatarray,
                                         params[i-1].data.d_int32,
                                         user_data))
            return;
          break;

        case GIMP_PDB_STRINGARRAY:
          if (! _gimp_wire_write_string (channel,
                                         params[i].data.d_stringarray,
                                         params[i-1].data.d_int32,
                                         user_data))
            return;
          break;

        case GIMP_PDB_COLOR:
          if (! _gimp_wire_write_color (channel,
                                        &params[i].data.d_color, 1, user_data))
            return;
          break;

        case GIMP_PDB_ITEM:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_item, 1,
                                        user_data))
            return;
          break;

        case GIMP_PDB_DISPLAY:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_display, 1,
                                        user_data))
            return;
          break;

        case GIMP_PDB_IMAGE:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_image, 1,
                                        user_data))
            return;
          break;

        case GIMP_PDB_LAYER:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_layer, 1,
                                        user_data))
            return;
          break;

        case GIMP_PDB_CHANNEL:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_channel, 1,
                                        user_data))
            return;
          break;

        case GIMP_PDB_DRAWABLE:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_drawable, 1,
                                        user_data))
            return;
          break;

        case GIMP_PDB_SELECTION:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_selection, 1,
                                        user_data))
            return;
          break;

        case GIMP_PDB_COLORARRAY:
          if (! _gimp_wire_write_color (channel,
                                        params[i].data.d_colorarray,
                                        params[i-1].data.d_int32,
                                        user_data))
            return;
          break;

        case GIMP_PDB_VECTORS:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_vectors, 1,
                                        user_data))
            return;
          break;

        case GIMP_PDB_PARASITE:
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

        case GIMP_PDB_STATUS:
          if (! _gimp_wire_write_int32 (channel,
                                        (const guint32 *) &params[i].data.d_status, 1,
                                        user_data))
            return;
          break;

        case GIMP_PDB_END:
          break;
        }
    }
}

void
gp_params_destroy (GPParam *params,
                   gint     nparams)
{
  gint i;

  for (i = 0; i < nparams; i++)
    {
      switch (params[i].type)
        {
        case GIMP_PDB_INT32:
        case GIMP_PDB_INT16:
        case GIMP_PDB_INT8:
        case GIMP_PDB_FLOAT:
        case GIMP_PDB_COLOR:
        case GIMP_PDB_ITEM:
        case GIMP_PDB_DISPLAY:
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_VECTORS:
        case GIMP_PDB_STATUS:
          break;

        case GIMP_PDB_STRING:
          g_free (params[i].data.d_string);
          break;

        case GIMP_PDB_INT32ARRAY:
          g_free (params[i].data.d_int32array);
          break;

        case GIMP_PDB_INT16ARRAY:
          g_free (params[i].data.d_int16array);
          break;

        case GIMP_PDB_INT8ARRAY:
          g_free (params[i].data.d_int8array);
          break;

        case GIMP_PDB_FLOATARRAY:
          g_free (params[i].data.d_floatarray);
          break;

        case GIMP_PDB_STRINGARRAY:
          if ((i > 0) && (params[i-1].type == GIMP_PDB_INT32))
            {
              gint count = params[i-1].data.d_int32;
              gint j;

              for (j = 0; j < count; j++)
                g_free (params[i].data.d_stringarray[j]);

              g_free (params[i].data.d_stringarray);
            }
          break;

        case GIMP_PDB_COLORARRAY:
          g_free (params[i].data.d_colorarray);
          break;

        case GIMP_PDB_PARASITE:
          if (params[i].data.d_parasite.name)
            g_free (params[i].data.d_parasite.name);
          if (params[i].data.d_parasite.data)
            g_free (params[i].data.d_parasite.data);
          break;

        case GIMP_PDB_END:
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
