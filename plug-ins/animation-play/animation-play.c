/*
 * Animation Playback plug-in version 0.99.1
 *
 * (c) Adam D. Moss : 1997-2000 : adam@gimp.org : adam@foxbox.org
 * (c) Mircea Purdea : 2009 : someone_else@exhalus.net
 * (c) Jehan : 2012-2015 : jehan at girinstud.io
 *
 * GIMP - The GNU Image Manipulation Program
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

#include <libgimp/gimp.h>
#undef GDK_DISABLE_DEPRECATED
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "animation-utils.h"
#include "animation.h"
#include "animation-dialog.h"

static void        query                     (void);
static void        run                       (const gchar      *name,
                                              gint              nparams,
                                              const GimpParam  *param,
                                              gint             *nreturn_vals,
                                              GimpParam       **return_vals);

/* Utils */
static void        save_settings             (Animation        *animation,
                                              gint32            image_id);

/* Settings we cache assuming they may be the user's
 * favorite, like a framerate, or a type of frame disposal. */
typedef struct
{
  DisposeType disposal;
  gdouble     framerate;
}
CachedSettings;

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable (unused)"      }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Preview an animation"),
                          "",
                          "Adam D. Moss <adam@gimp.org>",
                          "Adam D. Moss <adam@gimp.org>",
                          "1997, 1998...",
                          N_("Animation _Playback..."),
                          "RGB*, INDEXED*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Animation");
  gimp_plugin_icon_register (PLUG_IN_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) "media-playback-start");
}

static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status;
  GimpRunMode        run_mode;
  GeglConfig        *config;

  INIT_I18N ();

  gegl_init (NULL, NULL);
  config = gegl_config ();
  /* For preview, we want fast (0.0) over high quality (1.0). */
  g_object_set (config, "quality", 0.0, NULL);

  status        = GIMP_PDB_SUCCESS;
  *nreturn_vals = 1;
  *return_vals  = values;

  run_mode = param[0].data.d_int32;

  if (run_mode == GIMP_RUN_NONINTERACTIVE && n_params != 3)
    {
      /* This plugin is meaningless right now other than interactive. */
      status = GIMP_PDB_CALLING_ERROR;
    }
  else
    {
      Animation       *animation;
      GtkWidget       *dialog;
      GimpParasite    *parasite;
      CachedSettings   cached_settings;
      gint32           image_id;

      gimp_ui_init (PLUG_IN_BINARY, TRUE);

      /***********************/
      /* init the animation. */
      image_id  = param[1].data.d_image;
      animation = animation_new (image_id);
      dialog    = animation_dialog_new (animation);

      gtk_widget_show_now (GTK_WIDGET (dialog));

      gimp_help_connect (GTK_WIDGET (dialog), gimp_standard_help_func, PLUG_IN_PROC, NULL);

      /* Acceptable default for cached settings. */
      cached_settings.disposal  = DISPOSE_COMBINE;
      cached_settings.framerate = 24.0;

      /* If we saved any settings globally, use the one from the last run. */
      gimp_get_data (PLUG_IN_PROC, &cached_settings);

      /* If this animation has specific settings already, override the global ones. */
      parasite = gimp_image_get_parasite (image_id, PLUG_IN_PROC "/frame-disposal");
      if (parasite)
        {
          const DisposeType *mode = gimp_parasite_data (parasite);

          cached_settings.disposal = *mode;
          gimp_parasite_free (parasite);
        }
      parasite = gimp_image_get_parasite (image_id,
                                          PLUG_IN_PROC "/framerate");
      if (parasite)
        {
          const gdouble *rate = gimp_parasite_data (parasite);

          cached_settings.framerate = *rate;
          gimp_parasite_free (parasite);
        }

      animation_set_framerate (animation, cached_settings.framerate);
      animation_load (animation, cached_settings.disposal, 1.0);

      gtk_main ();

      /* Save the current settings. */
      save_settings (animation, image_id);

      /* Frames are associated to an unused GimpImage.
       * We have to make sure the animation is freed properly so that
       * we don't get leaked GEGL buffers. */
      g_object_unref (animation);

      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  gegl_exit ();
  gimp_quit ();
}

/************ UTILS ****************/

static void
save_settings (Animation       *animation,
               gint32           image_id)
{
  GimpParasite   *old_parasite;
  gboolean        undo_step_started = FALSE;
  CachedSettings  cached_settings;

  /* First saving in cache for any image. */
  cached_settings.disposal  = animation_get_disposal (animation);
  cached_settings.framerate = animation_get_framerate (animation);

  gimp_set_data (PLUG_IN_PROC, &cached_settings, sizeof (&cached_settings));

  /* Then as a parasite for the specific image.
   * If there was already parasites and they were all the same as the
   * current state, do not resave them.
   * This prevents setting the image in a dirty state while it stayed
   * the same. */
  old_parasite = gimp_image_get_parasite (image_id, PLUG_IN_PROC "/frame-disposal");
  if (! old_parasite ||
      *(DisposeType *) gimp_parasite_data (old_parasite) != animation_get_disposal (animation))
    {
      GimpParasite *parasite;
      DisposeType   disposal = animation_get_disposal (animation);

      gimp_image_undo_group_start (image_id);
      undo_step_started = TRUE;

      parasite = gimp_parasite_new (PLUG_IN_PROC "/frame-disposal",
                                    GIMP_PARASITE_PERSISTENT | GIMP_PARASITE_UNDOABLE,
                                    sizeof (disposal),
                                    &disposal);
      gimp_image_attach_parasite (image_id, parasite);
      gimp_parasite_free (parasite);
    }
  gimp_parasite_free (old_parasite);

  old_parasite = gimp_image_get_parasite (image_id, PLUG_IN_PROC "/framerate");
  if (! old_parasite ||
      *(gdouble *) gimp_parasite_data (old_parasite) != cached_settings.framerate)
    {
      GimpParasite *parasite;
      if (! undo_step_started)
        {
          gimp_image_undo_group_start (image_id);
          undo_step_started = TRUE;
        }
      parasite = gimp_parasite_new (PLUG_IN_PROC "/frame-rate",
                                    GIMP_PARASITE_PERSISTENT | GIMP_PARASITE_UNDOABLE,
                                    sizeof (cached_settings.framerate),
                                    &cached_settings.framerate);
      gimp_image_attach_parasite (image_id, parasite);
      gimp_parasite_free (parasite);
    }
  gimp_parasite_free (old_parasite);

  if (undo_step_started)
    {
      gimp_image_undo_group_end (image_id);
    }
}
