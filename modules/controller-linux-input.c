/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * controller_linux_input.c
 * Copyright (C) 2004-2007 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
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

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/input.h>

#include <glib/gstdio.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#define GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libgimpwidgets/gimpcontroller.h"

#include "gimpinputdevicestore.h"

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
  { BTN_BACK,      "button-back",      N_("Button Back")      },
  { BTN_TASK,      "button-task",      N_("Button Task")      },
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
  { REL_X,      "x-move-left",                   N_("X Move Left")               },
  { REL_X,      "x-move-right",                  N_("X Move Right")              },
  { REL_Y,      "y-move-forward",                N_("Y Move Forward")            },
  { REL_Y,      "y-move-back",                   N_("Y Move Back")               },
  { REL_Z,      "z-move-up",                     N_("Z Move Up")                 },
  { REL_Z,      "z-move-down",                   N_("Z Move Down")               },
#ifdef REL_RX
  { REL_RX,     "x-axis-tilt-forward",           N_("X Axis Tilt Forward")       },
  { REL_RX,     "x-axis-tilt-back",              N_("X Axis Tilt Back")          },
  { REL_RY,     "y-axis-tilt-right",             N_("Y Axis Tilt Right")         },
  { REL_RY,     "y-axis-tilt-left",              N_("Y Axis Tilt Left")          },
  { REL_RZ,     "z-axis-turn-left",              N_("Z Axis Turn Left")          },
  { REL_RZ,     "z-axis-turn-right",             N_("Z Axis Turn Right")         },
#endif  /* REL_RX */
  { REL_HWHEEL, "horizontal-wheel-turn-back",    N_("Horiz. Wheel Turn Back")    },
  { REL_HWHEEL, "horizontal-wheel-turn-forward", N_("Horiz. Wheel Turn Forward") },
  { REL_DIAL,   "dial-turn-left",                N_("Dial Turn Left")            },
  { REL_DIAL,   "dial-turn-right",               N_("Dial Turn Right")           },
  { REL_WHEEL,  "wheel-turn-left",               N_("Wheel Turn Left")           },
  { REL_WHEEL,  "wheel-turn-right",              N_("Wheel Turn Right")          },
};


enum
{
  PROP_0,
  PROP_DEVICE,
  PROP_DEVICE_STORE
};


#define CONTROLLER_TYPE_LINUX_INPUT            (controller_linux_input_get_type ())
#define CONTROLLER_LINUX_INPUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONTROLLER_TYPE_LINUX_INPUT, ControllerLinuxInput))
#define CONTROLLER_LINUX_INPUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CONTROLLER_TYPE_LINUX_INPUT, ControllerLinuxInputClass))
#define CONTROLLER_IS_LINUX_INPUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CONTROLLER_TYPE_LINUX_INPUT))
#define CONTROLLER_IS_LINUX_INPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CONTROLLER_TYPE_LINUX_INPUT))


typedef struct _ControllerLinuxInput      ControllerLinuxInput;
typedef struct _ControllerLinuxInputClass ControllerLinuxInputClass;

struct _ControllerLinuxInput
{
  GimpController        parent_instance;

  GimpInputDeviceStore *store;
  gchar                *device;
  GIOChannel           *io;
  guint                 io_id;
};

struct _ControllerLinuxInputClass
{
  GimpControllerClass  parent_class;
};


static GType  controller_linux_input_get_type     (void);

static void   linux_input_dispose                 (GObject        *object);
static void   linux_input_finalize                (GObject        *object);
static void   linux_input_set_property            (GObject        *object,
                                                   guint           property_id,
                                                   const GValue   *value,
                                                   GParamSpec     *pspec);
static void   linux_input_get_property            (GObject        *object,
                                                   guint           property_id,
                                                   GValue         *value,
                                                   GParamSpec     *pspec);

static gint          linux_input_get_n_events     (GimpController *controller);
static const gchar * linux_input_get_event_name   (GimpController *controller,
                                                   gint            event_id);
static const gchar * linux_input_get_event_blurb  (GimpController *controller,
                                                   gint            event_id);

static void          linux_input_device_changed   (ControllerLinuxInput *controller,
                                                   const gchar          *udi);
static gboolean      linux_input_set_device       (ControllerLinuxInput *controller,
                                                   const gchar          *device);
static gboolean      linux_input_read_event       (GIOChannel           *io,
                                                   GIOCondition          cond,
                                                   gpointer              data);


static const GimpModuleInfo linux_input_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Linux input event controller"),
  "Sven Neumann <sven@gimp.org>, Michael Natterer <mitch@gimp.org>",
  "v0.2",
  "(c) 2004-2007, released under the GPL",
  "2004-2007"
};


G_DEFINE_DYNAMIC_TYPE (ControllerLinuxInput, controller_linux_input,
                       GIMP_TYPE_CONTROLLER)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &linux_input_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  gimp_input_device_store_register_types (module);
  controller_linux_input_register_type (module);

  return TRUE;
}

static void
controller_linux_input_class_init (ControllerLinuxInputClass *klass)
{
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);

  object_class->dispose            = linux_input_dispose;
  object_class->finalize           = linux_input_finalize;
  object_class->get_property       = linux_input_get_property;
  object_class->set_property       = linux_input_set_property;

  g_object_class_install_property (object_class, PROP_DEVICE,
                                   g_param_spec_string ("device",
                                                        _("Device:"),
                                                        _("The name of the device to read Linux Input events from."),
                                                        NULL,
                                                        GIMP_CONFIG_PARAM_FLAGS));
#ifdef HAVE_LIBGUDEV
  g_object_class_install_property (object_class, PROP_DEVICE_STORE,
                                   g_param_spec_object ("device-values",
                                                        NULL, NULL,
                                                        GIMP_TYPE_INPUT_DEVICE_STORE,
                                                        G_PARAM_READABLE));
#endif

  controller_class->name            = _("Linux Input");
  controller_class->help_id         = "gimp-controller-linux-input";
  controller_class->icon_name       = GIMP_ICON_CONTROLLER_LINUX_INPUT;

  controller_class->get_n_events    = linux_input_get_n_events;
  controller_class->get_event_name  = linux_input_get_event_name;
  controller_class->get_event_blurb = linux_input_get_event_blurb;
}

static void
controller_linux_input_class_finalize (ControllerLinuxInputClass *klass)
{
}

static void
controller_linux_input_init (ControllerLinuxInput *controller)
{
  controller->store = gimp_input_device_store_new ();

  if (controller->store)
    {
      g_signal_connect_swapped (controller->store, "device-added",
                                G_CALLBACK (linux_input_device_changed),
                                controller);
      g_signal_connect_swapped (controller->store, "device-removed",
                                G_CALLBACK (linux_input_device_changed),
                                controller);
    }
}

static void
linux_input_dispose (GObject *object)
{
  ControllerLinuxInput *controller = CONTROLLER_LINUX_INPUT (object);

  linux_input_set_device (controller, NULL);

  G_OBJECT_CLASS (controller_linux_input_parent_class)->dispose (object);
}

static void
linux_input_finalize (GObject *object)
{
  ControllerLinuxInput *controller = CONTROLLER_LINUX_INPUT (object);

  if (controller->store)
    {
      g_object_unref (controller->store);
      controller->store = NULL;
    }

  G_OBJECT_CLASS (controller_linux_input_parent_class)->finalize (object);
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
    case PROP_DEVICE_STORE:
      g_value_set_object (value, controller->store);
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

#define BITS_PER_LONG        (sizeof(long) * 8)
#define NBITS(x)             ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)               ((x)%BITS_PER_LONG)
#define BIT(x)               (1UL<<OFF(x))
#define LONG(x)              ((x)/BITS_PER_LONG)
#define test_bit(bit, array) ((array[LONG(bit)] >> OFF(bit)) & 1)

static void
linux_input_get_device_info (ControllerLinuxInput *controller,
                             int                   fd)
{
  unsigned long evbit[NBITS (EV_MAX)];
  unsigned long keybit[NBITS (KEY_MAX)];
  unsigned long relbit[NBITS (REL_MAX)];
  unsigned long absbit[NBITS (ABS_MAX)];

  gint num_keys     = 0;
  gint num_ext_keys = 0;
  gint num_buttons  = 0;
  gint num_rels     = 0;
  gint num_abs      = 0;

  /* get event type bits */
  ioctl (fd, EVIOCGBIT (0, EV_MAX), evbit);

  if (test_bit (EV_KEY, evbit))
    {
      gint i;

      /* get keyboard bits */
      ioctl (fd, EVIOCGBIT (EV_KEY, KEY_MAX), keybit);

      /**  count typical keyboard keys only */
      for (i = KEY_Q; i < KEY_M; i++)
        if (test_bit (i, keybit))
          {
            num_keys++;

            g_print ("%s: key 0x%02x present\n", G_STRFUNC, i);
          }

      g_print ("%s: #keys = %d\n", G_STRFUNC, num_keys);

      for (i = KEY_OK; i < KEY_MAX; i++)
        if (test_bit (i, keybit))
          {
            num_ext_keys++;

            g_print ("%s: ext key 0x%02x present\n", G_STRFUNC, i);
          }

      g_print ("%s: #ext_keys = %d\n", G_STRFUNC, num_ext_keys);

      for (i = BTN_MISC; i < KEY_OK; i++)
        if (test_bit (i, keybit))
          {
            num_buttons++;

            g_print ("%s: button 0x%02x present\n", G_STRFUNC, i);
          }

      g_print ("%s: #buttons = %d\n", G_STRFUNC, num_buttons);
    }

  if (test_bit (EV_REL, evbit))
    {
      gint i;

      /* get bits for relative axes */
      ioctl (fd, EVIOCGBIT (EV_REL, REL_MAX), relbit);

      for (i = 0; i < REL_MAX; i++)
        if (test_bit (i, relbit))
          {
            num_rels++;

            g_print ("%s: rel 0x%02x present\n", G_STRFUNC, i);
          }

      g_print ("%s: #rels = %d\n", G_STRFUNC, num_rels);
    }

  if (test_bit (EV_ABS, evbit))
    {
      gint i;

      /* get bits for absolute axes */
      ioctl (fd, EVIOCGBIT (EV_ABS, ABS_MAX), absbit);

      for (i = 0; i < ABS_MAX; i++)
        if (test_bit (i, absbit))
          {
            struct input_absinfo absinfo;

            num_abs++;

            /* get info for the absolute axis */
            ioctl (fd, EVIOCGABS (i), &absinfo);

            g_print ("%s: abs 0x%02x present [%d..%d]\n", G_STRFUNC, i,
                     absinfo.minimum, absinfo.maximum);
          }

      g_print ("%s: #abs = %d\n", G_STRFUNC, num_abs);
    }
}

static void
linux_input_device_changed (ControllerLinuxInput *controller,
                            const gchar          *identifier)
{
  if (controller->device && strcmp (identifier, controller->device) == 0)
    {
      linux_input_set_device (controller, identifier);
      g_object_notify (G_OBJECT (controller), "device");
    }
}

static gboolean
linux_input_set_device (ControllerLinuxInput *controller,
                        const gchar          *device)
{
  gchar *filename;

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
      if (controller->store)
        filename = gimp_input_device_store_get_device_file (controller->store,
                                                            controller->device);
      else
        filename = g_strdup (controller->device);
    }
  else
    {
      g_object_set (controller, "state", _("No device configured"), NULL);

      return FALSE;
    }

  if (filename)
    {
      gchar *state;
      gint   fd;

      fd = g_open (filename, O_RDONLY, 0);

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

          linux_input_get_device_info (controller, fd);

          state = g_strdup_printf (_("Reading from %s"), filename);
          g_object_set (controller, "state", state, NULL);
          g_free (state);

          g_free (filename);

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

      g_free (filename);
    }
  else if (controller->store)
    {
      GError *error = gimp_input_device_store_get_error (controller->store);

      if (error)
        {
          g_object_set (controller, "state", error->message, NULL);
          g_error_free (error);
        }
      else
        {
          g_object_set (controller, "state", _("Device not available"), NULL);
        }
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
      GimpController      *controller = GIMP_CONTROLLER (data);
      GimpControllerEvent  cevent     = { 0, };
      gint                 i;

      switch (ev.type)
        {
        case EV_KEY:
          g_print ("%s: EV_KEY code = 0x%02x\n", G_STRFUNC, ev.code);

          for (i = 0; i < G_N_ELEMENTS (key_events); i++)
            if (ev.code == key_events[i].code)
              {
                cevent.any.type     = GIMP_CONTROLLER_EVENT_TRIGGER;
                cevent.any.source   = controller;
                cevent.any.event_id = i;

                gimp_controller_event (controller, &cevent);

                break;
              }
          break;

        case EV_REL:
          g_print ("%s: EV_REL code = 0x%02x (value = %d)\n", G_STRFUNC,
                   ev.code, ev.value);

          for (i = 0; i < G_N_ELEMENTS (rel_events); i++)
            if (ev.code == rel_events[i].code)
              {
                cevent.any.type     = GIMP_CONTROLLER_EVENT_VALUE;
                cevent.any.source   = controller;
                cevent.any.event_id = G_N_ELEMENTS (key_events) + i;

                g_value_init (&cevent.value.value, G_TYPE_DOUBLE);

                if (ev.value < 0)
                  {
                    g_value_set_double (&cevent.value.value, -ev.value);
                  }
                else
                  {
                    cevent.any.event_id++;

                    g_value_set_double (&cevent.value.value, ev.value);
                  }

                gimp_controller_event (controller, &cevent);

                g_value_unset (&cevent.value.value);

               break;
              }
          break;

        case EV_ABS:
          g_print ("%s: EV_ABS code = 0x%02x (value = %d)\n", G_STRFUNC,
                   ev.code, ev.value);
          break;

        default:
          break;
        }
    }

  return TRUE;
}
