/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * controller_linux_input.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/input.h>

#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#define GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libgimpwidgets/gimpcontroller.h"

#include "libgimp/libgimp-intl.h"


typedef struct
{
  guint16       code;
  const gchar  *name;
  const gchar  *blurb;
} LinuxInputEvent;

static const LinuxInputEvent key_events[] =
{
  { BTN_0,         "button-0",         N_("Button 0")         },
  { BTN_1,         "button-1",         N_("Button 1")         },
  { BTN_2,         "button-2",         N_("Button 2")         },
  { BTN_3,         "button-3",         N_("Button 3")         },
  { BTN_4,         "button-4",         N_("Button 4")         },
  { BTN_5,         "button-5",         N_("Button 5")         },
  { BTN_6,         "button-6",         N_("Button 6")         },
  { BTN_7,         "button-7",         N_("Button 7")         },
  { BTN_8,         "button-8",         N_("Button 8")         },
  { BTN_9,         "button-9",         N_("Button 9")         },
  { BTN_MOUSE,     "button-mouse",     N_("Button Mouse")     },
  { BTN_LEFT,      "button-left",      N_("Button Left")      },
  { BTN_RIGHT,     "button-right",     N_("Button Right")     },
  { BTN_MIDDLE,    "button-middle",    N_("Button Middle")    },
  { BTN_SIDE,      "button-side",      N_("Button Side")      },
  { BTN_EXTRA,     "button-extra",     N_("Button Extra")     },
  { BTN_FORWARD,   "button-forward",   N_("Button Forward")   },
  { BTN_BACK,      "button-back",      N_("Button Forward")   },
#ifdef BTN_WHEEL
  { BTN_WHEEL,     "button-wheel",     N_("Button Wheel")     },
#endif
#ifdef BTN_GEAR_DOWN
  { BTN_GEAR_DOWN, "button-gear-down", N_("Button Gear Down") },
#endif
#ifdef BTN_GEAR_UP
  { BTN_GEAR_UP,   "button-gear-up",   N_("Button Gear Up")   }
#endif
};

static const LinuxInputEvent rel_events[] =
{
  { REL_WHEEL,     "wheel-turn-left",  N_("Wheel Turn Left")  },
  { REL_WHEEL,     "wheel-turn-right", N_("Wheel Turn Right") },
  { REL_DIAL,      "dial-turn-left",   N_("Dial Turn Left")   },
  { REL_DIAL,      "dial-turn-right",  N_("Dial Turn Right")  },
};


enum
{
  PROP_0,
  PROP_DEVICE
};


#define CONTROLLER_TYPE_LINUX_INPUT            (controller_type)
#define CONTROLLER_LINUX_INPUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONTROLLER_TYPE_LINUX_INPUT, ControllerLinuxInput))
#define CONTROLLER_LINUX_INPUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CONTROLLER_TYPE_LINUX_INPUT, ControllerLinuxInputClass))
#define CONTROLLER_IS_LINUX_INPUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CONTROLLER_TYPE_LINUX_INPUT))
#define CONTROLLER_IS_LINUX_INPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CONTROLLER_TYPE_LINUX_INPUT))


typedef struct _ControllerLinuxInput      ControllerLinuxInput;
typedef struct _ControllerLinuxInputClass ControllerLinuxInputClass;

struct _ControllerLinuxInput
{
  GimpController  parent_instance;

  gchar          *device;
  GIOChannel     *io;
  guint           io_id;
};

struct _ControllerLinuxInputClass
{
  GimpControllerClass  parent_class;
};


G_MODULE_EXPORT const GimpModuleInfo * gimp_module_query    (GTypeModule *module);
G_MODULE_EXPORT gboolean               gimp_module_register (GTypeModule *module);

GType         linux_input_get_type     (GTypeModule    *module);
static void   linux_input_class_init   (ControllerLinuxInputClass *klass);
static void   linux_input_dispose      (GObject        *object);
static void   linux_input_set_property (GObject        *object,
                                        guint           property_id,
                                        const GValue   *value,
                                        GParamSpec     *pspec);
static void   linux_input_get_property (GObject        *object,
                                        guint           property_id,
                                        GValue         *value,
                                        GParamSpec     *pspec);

static gint          linux_input_get_n_events     (GimpController *controller);
static const gchar * linux_input_get_event_name   (GimpController *controller,
                                                   gint            event_id);
static const gchar * linux_input_get_event_blurb  (GimpController *controller,
                                                   gint            event_id);

static gboolean      linux_input_set_device (ControllerLinuxInput *controller,
                                             const gchar          *device);
static gboolean      linux_input_read_event (GIOChannel           *io,
                                             GIOCondition          cond,
                                             gpointer              data);


static const GimpModuleInfo linux_input_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Linux input event controller"),
  "Sven Neumann <sven@gimp.org>",
  "v0.1",
  "(c) 2004, released under the GPL",
  "June 2004"
};


static GType                controller_type = 0;
static GimpControllerClass *parent_class    = NULL;


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &linux_input_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  linux_input_get_type (module);

  return TRUE;
}


GType
linux_input_get_type (GTypeModule *module)
{
  if (! controller_type)
    {
      const GTypeInfo controller_info =
      {
        sizeof (ControllerLinuxInputClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) linux_input_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (ControllerLinuxInput),
        0,              /* n_preallocs    */
        NULL            /* instance_init  */
      };

      controller_type = g_type_module_register_type (module,
                                                     GIMP_TYPE_CONTROLLER,
                                                     "ControllerLinuxInput",
                                                     &controller_info, 0);
    }

  return controller_type;
}

static void
linux_input_class_init (ControllerLinuxInputClass *klass)
{
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->dispose            = linux_input_dispose;
  object_class->get_property       = linux_input_get_property;
  object_class->set_property       = linux_input_set_property;

  g_object_class_install_property (object_class, PROP_DEVICE,
                                   g_param_spec_string ("device",
                                                        _("Device:"),
                                                        _("The name of the device to read Linux Input events from."),
                                                        NULL,
                                                        GIMP_CONFIG_PARAM_FLAGS));

  controller_class->name            = _("Linux Input");
  controller_class->help_id         = "gimp-controller-linux-input";
  controller_class->stock_id        = GIMP_STOCK_CONTROLLER_LINUX_INPUT;

  controller_class->get_n_events    = linux_input_get_n_events;
  controller_class->get_event_name  = linux_input_get_event_name;
  controller_class->get_event_blurb = linux_input_get_event_blurb;
}

static void
linux_input_dispose (GObject *object)
{
  ControllerLinuxInput *controller = CONTROLLER_LINUX_INPUT (object);

  linux_input_set_device (controller, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
linux_input_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  ControllerLinuxInput *controller = CONTROLLER_LINUX_INPUT (object);

  switch (property_id)
    {
    case PROP_DEVICE:
      linux_input_set_device (controller, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
linux_input_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  ControllerLinuxInput *controller = CONTROLLER_LINUX_INPUT (object);

  switch (property_id)
    {
    case PROP_DEVICE:
      g_value_set_string (value, controller->device);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint
linux_input_get_n_events (GimpController *controller)
{
  return G_N_ELEMENTS (key_events) + G_N_ELEMENTS (rel_events);
}

static const gchar *
linux_input_get_event_name (GimpController *controller,
                            gint            event_id)
{
  if (event_id < 0)
    {
      return NULL;
    }
  else if (event_id < G_N_ELEMENTS (key_events))
    {
      return key_events[event_id].name;
    }
  else if (event_id < linux_input_get_n_events (controller))
    {
      return rel_events[event_id - G_N_ELEMENTS (key_events)].name;
    }
  else
    {
      return NULL;
    }
}

static const gchar *
linux_input_get_event_blurb (GimpController *controller,
                             gint            event_id)
{
  if (event_id < 0)
    {
      return NULL;
    }
  else if (event_id < G_N_ELEMENTS (key_events))
    {
      return gettext (key_events[event_id].blurb);
    }
  else if (event_id < linux_input_get_n_events (controller))
    {
      return gettext (rel_events[event_id - G_N_ELEMENTS (key_events)].blurb);
    }
  else
    {
      return NULL;
    }
}

static gboolean
linux_input_set_device (ControllerLinuxInput *controller,
                        const gchar          *device)
{
  if (controller->io)
    {
      g_source_remove (controller->io_id);
      controller->io_id = 0;

      g_io_channel_unref (controller->io);
      controller->io = NULL;
    }

  if (controller->device)
    g_free (controller->device);

  controller->device = g_strdup (device);

  g_object_set (controller, "name", _("Linux Input Events"), NULL);

  if (controller->device && strlen (controller->device))
    {
      gchar *state;
      gint   fd;

      fd = g_open (controller->device, O_RDONLY, 0);

      if (fd >= 0)
        {
          gchar name[256];

          name[0] = '\0';
          if (ioctl (fd, EVIOCGNAME (sizeof (name)), name) >= 0 &&
              strlen (name) > 0                                 &&
              g_utf8_validate (name, -1, NULL))
            {
              g_object_set (controller, "name", name, NULL);
            }

          state = g_strdup_printf (_("Reading from %s"), controller->device);
          g_object_set (controller, "state", state, NULL);
          g_free (state);

          controller->io = g_io_channel_unix_new (fd);
          g_io_channel_set_close_on_unref (controller->io, TRUE);
          g_io_channel_set_encoding (controller->io, NULL, NULL);

          controller->io_id = g_io_add_watch (controller->io,
                                              G_IO_IN,
                                              linux_input_read_event,
                                              controller);
          return TRUE;
        }
      else
        {
          state = g_strdup_printf (_("Device not available: %s"),
                                   g_strerror (errno));
          g_object_set (controller, "state", state, NULL);
          g_free (state);
        }
    }
  else
    {
      g_object_set (controller, "state", _("No device configured"), NULL);
    }

  return FALSE;
}

static gboolean
linux_input_read_event (GIOChannel   *io,
                        GIOCondition  cond,
                        gpointer      data)
{
  ControllerLinuxInput *input = CONTROLLER_LINUX_INPUT (data);
  GIOStatus             status;
  GError               *error = NULL;
  struct input_event    ev;
  gsize                 n_bytes;
  gint                  i;

  status = g_io_channel_read_chars (io,
                                    (gchar *) &ev,
                                    sizeof (struct input_event), &n_bytes,
                                    &error);

  switch (status)
    {
    case G_IO_STATUS_ERROR:
    case G_IO_STATUS_EOF:
      g_source_remove (input->io_id);
      input->io_id = 0;

      g_io_channel_unref (input->io);
      input->io = NULL;

      if (error)
        {
          gchar *state = g_strdup_printf (_("Device not available: %s"),
                                          error->message);
          g_object_set (input, "state", state, NULL);
          g_free (state);

          g_clear_error (&error);
        }
      else
        {
          g_object_set (input, "state", _("End of file"), NULL);
        }
      return FALSE;
      break;

    case G_IO_STATUS_AGAIN:
      return TRUE;

    default:
      break;
    }

  if (n_bytes == sizeof (struct input_event))
    {
      switch (ev.type)
        {
        case EV_KEY:
          for (i = 0; i < G_N_ELEMENTS (key_events); i++)
            if (ev.code == key_events[i].code)
              {
                GimpController      *controller = GIMP_CONTROLLER (data);
                GimpControllerEvent  cevent;

                cevent.any.type     = GIMP_CONTROLLER_EVENT_TRIGGER;
                cevent.any.source   = GIMP_CONTROLLER (data);
                cevent.any.event_id = i;

                gimp_controller_event (controller, &cevent);

                break;
              }
          break;

        case EV_REL:
          for (i = 0; i < G_N_ELEMENTS (rel_events); i++)
            if (ev.code == rel_events[i].code)
              {
                GimpController      *controller = GIMP_CONTROLLER (data);
                GimpControllerEvent  cevent;
                gint                 count;

                cevent.any.type     = GIMP_CONTROLLER_EVENT_TRIGGER;
                cevent.any.source   = controller;
                cevent.any.event_id = G_N_ELEMENTS (key_events) + i;

                for (count = ev.value; count < 0; count++)
                  gimp_controller_event (controller, &cevent);

                cevent.any.event_id++;

                for (count = ev.value; count > 0; count--)
                  gimp_controller_event (controller, &cevent);

                break;
              }
          break;

        default:
          break;
        }
    }

  return TRUE;
}
