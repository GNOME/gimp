/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * controller_midi.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gtk/gtk.h>

#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimpwidgets/gimpcontroller.h"

#include "libgimp/libgimp-intl.h"


typedef struct
{
  gchar *name;
  gchar *blurb;
} MidiEvent;


enum
{
  PROP_0,
  PROP_DEVICE
};


#define CONTROLLER_TYPE_MIDI            (controller_type)
#define CONTROLLER_MIDI(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONTROLLER_TYPE_MIDI, ControllerMidi))
#define CONTROLLER_MIDI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CONTROLLER_TYPE_MIDI, ControllerMidiClass))
#define CONTROLLER_IS_MIDI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CONTROLLER_TYPE_MIDI))
#define CONTROLLER_IS_MIDI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CONTROLLER_TYPE_MIDI))


typedef struct _ControllerMidi      ControllerMidi;
typedef struct _ControllerMidiClass ControllerMidiClass;

struct _ControllerMidi
{
  GimpController  parent_instance;

  gchar          *device;
  GIOChannel     *io;

  /* midi status */
  gboolean        swallow;
  gint            command;
  gint            channel;
  gint            key;
  gint            velocity;
  gint            msb;
  gint            lsb;
};

struct _ControllerMidiClass
{
  GimpControllerClass  parent_class;
};


GType                midi_get_type        (GTypeModule    *module);

static void          midi_class_init      (ControllerMidiClass *klass);
static void          midi_init            (ControllerMidi      *midi);

static void          midi_dispose         (GObject        *object);
static void          midi_set_property    (GObject        *object,
                                           guint           property_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
static void          midi_get_property    (GObject        *object,
                                           guint           property_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);

static gint          midi_get_n_events    (GimpController *controller);
static const gchar * midi_get_event_name  (GimpController *controller,
                                           gint            event_id);
static const gchar * midi_get_event_blurb (GimpController *controller,
                                           gint            event_id);

static gboolean      midi_set_device      (ControllerMidi *controller,
                                           const gchar    *device);
static gboolean      midi_read_event      (GIOChannel     *io,
                                           GIOCondition    cond,
                                           gpointer        data);


static const GimpModuleInfo midi_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Midi event controller"),
  "Michael Natterer <mitch@gimp.org>",
  "v0.1",
  "(c) 2004, released under the GPL",
  "June 2004"
};


static GType                controller_type = 0;
static GimpControllerClass *parent_class    = NULL;

static MidiEvent            midi_events[128 + 128 + 128];


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &midi_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  midi_get_type (module);

  return TRUE;
}


GType
midi_get_type (GTypeModule *module)
{
  if (! controller_type)
    {
      static const GTypeInfo controller_info =
      {
        sizeof (ControllerMidiClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) midi_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (ControllerMidi),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) midi_init
      };

      controller_type = g_type_module_register_type (module,
                                                     GIMP_TYPE_CONTROLLER,
                                                     "ControllerMidi",
                                                     &controller_info, 0);
    }

  return controller_type;
}

static void
midi_class_init (ControllerMidiClass *klass)
{
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->dispose            = midi_dispose;
  object_class->get_property       = midi_get_property;
  object_class->set_property       = midi_set_property;

  g_object_class_install_property (object_class, PROP_DEVICE,
                                   g_param_spec_string ("device", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        GIMP_CONTROLLER_PARAM_SERIALIZE));

  controller_class->name            = _("Midi Events");

  controller_class->get_n_events    = midi_get_n_events;
  controller_class->get_event_name  = midi_get_event_name;
  controller_class->get_event_blurb = midi_get_event_blurb;
}

static void
midi_init (ControllerMidi *midi)
{
  midi->device   = NULL;
  midi->io       = NULL;

  midi->swallow  = TRUE; /* get rid of data bytes at start of stream */
  midi->command  = 0x0;
  midi->channel  = 0x0;
  midi->key      = -1;
  midi->velocity = -1;
  midi->msb      = -1;
  midi->lsb      = -1;
}

static void
midi_dispose (GObject *object)
{
  ControllerMidi *midi = CONTROLLER_MIDI (object);

  midi_set_device (midi, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
midi_set_property (GObject      *object,
                   guint         property_id,
                   const GValue *value,
                   GParamSpec   *pspec)
{
  ControllerMidi *midi = CONTROLLER_MIDI (object);

  switch (property_id)
    {
    case PROP_DEVICE:
      midi_set_device (midi, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
midi_get_property (GObject    *object,
                   guint       property_id,
                   GValue     *value,
                   GParamSpec *pspec)
{
  ControllerMidi *midi = CONTROLLER_MIDI (object);

  switch (property_id)
    {
    case PROP_DEVICE:
      g_value_set_string (value, midi->device);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint
midi_get_n_events (GimpController *controller)
{
  return 128 + 128 + 128;
}

static const gchar *
midi_get_event_name (GimpController *controller,
                     gint            event_id)
{
  if (event_id < (128 + 128 + 128))
    {
      if (! midi_events[event_id].name)
        {
          if (event_id < 128)
            midi_events[event_id].name = g_strdup_printf ("note-on-%02x",
                                                          event_id);
          else if (event_id < (128 + 128))
            midi_events[event_id].name = g_strdup_printf ("note-off-%02x",
                                                          event_id - 128);
          else if (event_id < (128 + 128 + 128))
            midi_events[event_id].name = g_strdup_printf ("controller-%03d",
                                                          event_id - 256);
        }

      return midi_events[event_id].name;
    }

  return NULL;
}

static const gchar *
midi_get_event_blurb (GimpController *controller,
                      gint            event_id)
{
  if (event_id <= 383)
    {
      if (! midi_events[event_id].blurb)
        {
          if (event_id <= 127)
            midi_events[event_id].blurb = g_strdup_printf ("Note %02x on",
                                                           event_id);
          else if (event_id <= 255)
            midi_events[event_id].blurb = g_strdup_printf ("Note %02x off",
                                                           event_id - 128);
          else if (event_id <= 383)
            midi_events[event_id].blurb = g_strdup_printf ("Controller %03d",
                                                           event_id - 256);
        }

      return midi_events[event_id].blurb;
    }

  return NULL;
}

static gboolean
midi_set_device (ControllerMidi *midi,
                 const gchar    *device)
{
  midi->swallow  = TRUE;
  midi->command  = 0x0;
  midi->channel  = 0x0;
  midi->key      = -1;
  midi->velocity = -1;
  midi->msb      = -1;
  midi->lsb      = -1;

  if (midi->io)
    {
      g_io_channel_unref (midi->io);
      midi->io = NULL;
    }

  if (midi->device)
    g_free (midi->device);

  midi->device = g_strdup (device);

  if (device)
    {
      gint fd;

#ifdef G_OS_WIN32
      fd = open (midi->device, O_RDONLY);
#else
      fd = open (midi->device, O_RDONLY | O_NONBLOCK);
#endif

      if (fd >= 0)
        {
          g_object_set (midi, "name", device, NULL);

          midi->io = g_io_channel_unix_new (fd);
          g_io_channel_set_close_on_unref (midi->io, TRUE);
          g_io_channel_set_encoding (midi->io, NULL, NULL);

          g_io_add_watch (midi->io,
                          G_IO_IN,
                          midi_read_event,
                          midi);
          return TRUE;
        }
      else
        {
          gchar *name = g_strdup_printf (_("Device not available: %s"),
                                         g_strerror (errno));
          g_object_set (midi, "name", name, NULL);
          g_free (name);
        }
    }
  else
    {
      g_object_set (midi, "name", _("No device configured"), NULL);
    }

  return FALSE;
}


#define D(stmnt) stmnt;

gboolean
midi_read_event (GIOChannel   *io,
                 GIOCondition  cond,
                 gpointer      data)
{
  ControllerMidi *midi = CONTROLLER_MIDI (data);
  guchar          buf[0xf];
  gsize           size;

  if (g_io_channel_read_chars (io,
                               (gchar *) buf,
                               sizeof (buf), &size,
                               NULL) == G_IO_STATUS_NORMAL)
    {
      gint pos = 0;

      while (pos < size)
        {
#if 0
          gint i;

          g_print ("MIDI IN (%d bytes), command 0x%02x: ", size, midi->command);

          for (i = 0; i < size; i++)
            g_print ("%02x ", buf[i]);
          g_print ("\n");
#endif

          if (buf[pos] & 0x80)  /* status byte */
            {
              if (buf[pos] >= 0xf8)  /* realtime messages */
                {
                  switch (buf[pos])
                    {
                    case 0xf8:  /* timing clock   */
                    case 0xf9:  /* (undefined)    */
                    case 0xfa:  /* start          */
                    case 0xfb:  /* continue       */
                    case 0xfc:  /* stop           */
                    case 0xfd:  /* (undefined)    */
                    case 0xfe:  /* active sensing */
                    case 0xff:  /* system reset   */
                      /* nop */
#if 0
                      g_print ("MIDI: realtime message (%02x)\n", buf[pos]);
#endif
                      break;
                    }
                }
              else
                {
                  midi->swallow = FALSE;  /* any status bytes ends swallowing */

                  if (buf[pos] >= 0xf0)  /* system messages */
                    {
                      switch (buf[pos])
                        {
                        case 0xf0:  /* sysex start */
                          midi->swallow = TRUE;

                          D (g_print ("MIDI: sysex start\n"));
                          break;

                        case 0xf1:              /* time code   */
                          midi->swallow = TRUE; /* type + data */

                          D (g_print ("MIDI: time code\n"));
                          break;

                        case 0xf2:              /* song position */
                          midi->swallow = TRUE; /* lsb + msb     */

                          D (g_print ("MIDI: song position\n"));
                          break;

                        case 0xf3:              /* song select */
                          midi->swallow = TRUE; /* song number */

                          D (g_print ("MIDI: song select\n"));
                          break;

                        case 0xf4:  /* (undefined) */
                        case 0xf5:  /* (undefined) */
                          D (g_print ("MIDI: undefined system message\n"));
                          break;

                        case 0xf6:  /* tune request */
                          D (g_print ("MIDI: tune request\n"));
                          break;

                        case 0xf7:  /* sysex end */
                          D (g_print ("MIDI: sysex end\n"));
                          break;
                        }
                    }
                  else  /* channel messages */
                    {
                      midi->command = buf[pos] >> 4;
                      midi->channel = buf[pos] & 0xf;

                      /* reset running status */
                      midi->key      = -1;
                      midi->velocity = -1;
                      midi->msb      = -1;
                      midi->lsb      = -1;
                    }
                }

              pos++;  /* status byte consumed */
              continue;
            }

          if (midi->swallow)
            {
              pos++;  /* consume any data byte */
              continue;
            }

          switch (midi->command)
            {
            case 0x8:  /* note off   */
            case 0x9:  /* note on    */
            case 0xa:  /* aftertouch */

              if (midi->key == -1)
                {
                  midi->key = buf[pos++];  /* key byte consumed */
                  continue;
                }

              if (midi->velocity == -1)
                midi->velocity = buf[pos++];  /* velocity byte consumed */

              /* note on with velocity = 0 means note off */
              if (midi->command == 0x9 && midi->velocity == 0x0)
                midi->command = 0x8;

              if (midi->command == 0x9)
                {
                  GimpControllerEvent event = { 0, };

                  D (g_print ("MIDI: note on (%02x vel %02x)\n",
                              midi->key, midi->velocity));

                  event.any.type     = GIMP_CONTROLLER_EVENT_VALUE;
                  event.any.source   = GIMP_CONTROLLER (data);
                  event.any.event_id = midi->key;

                  g_value_init (&event.value.value, G_TYPE_DOUBLE);
                  g_value_set_double (&event.value.value,
                                      (gdouble) midi->velocity / 127.0);

                  gimp_controller_event (GIMP_CONTROLLER (midi), &event);

                  g_value_unset (&event.value.value);
                }
              else if (midi->command == 0x8)
                {
                  GimpControllerEvent event = { 0, };

                  D (g_print ("MIDI: note off (%02x vel %02x)\n",
                              midi->key, midi->velocity));

                  event.any.type     = GIMP_CONTROLLER_EVENT_VALUE;
                  event.any.source   = GIMP_CONTROLLER (data);
                  event.any.event_id = midi->key + 128;

                  g_value_init (&event.value.value, G_TYPE_DOUBLE);
                  g_value_set_double (&event.value.value,
                                      (gdouble) midi->velocity / 127.0);

                  gimp_controller_event (GIMP_CONTROLLER (midi), &event);

                  g_value_unset (&event.value.value);
                }
              else
                {
                  D (g_print ("MIDI: polyphonic aftertouch (%02x pressure %02x)\n",
                              midi->key, midi->velocity));
                }

              midi->key      = -1;
              midi->velocity = -1;
              break;

            case 0xb:  /* controllers, sustain */
              if (midi->key == -1)
                {
                  midi->key = buf[pos++];
                  continue;
                }

              if (midi->velocity == -1)
                midi->velocity = buf[pos++];

              D (g_print ("MIDI: controller %d (value %d)\n",
                          midi->key, midi->velocity));

              {
                GimpControllerEvent event = { 0, };

                event.any.type     = GIMP_CONTROLLER_EVENT_VALUE;
                event.any.source   = GIMP_CONTROLLER (data);
                event.any.event_id = midi->key + 128 + 128;

                g_value_init (&event.value.value, G_TYPE_DOUBLE);
                g_value_set_double (&event.value.value,
                                    (gdouble) midi->velocity / 127.0);

                gimp_controller_event (GIMP_CONTROLLER (midi), &event);

                g_value_unset (&event.value.value);
              }

              midi->key      = -1;
              midi->velocity = -1;
              break;

            case 0xc:  /* program change */
              midi->key = buf[pos++];

              D (g_print ("MIDI: program change (%d)\n", midi->key));

              midi->key = -1;
              break;

            case 0xd:  /* channel key pressure */
              midi->velocity = buf[pos++];

              D (g_print ("MIDI: channel aftertouch (%d)\n", midi->velocity));

              midi->velocity = -1;
              break;

            case 0xe:  /* pitch bend */
              if (midi->lsb == -1)
                {
                  midi->lsb = buf[pos++];
                  continue;
                }

              if (midi->msb == -1)
                midi->msb = buf[pos++];

              midi->velocity = midi->lsb | (midi->msb << 7);

              D (g_print ("MIDI: pitch (%d)\n", midi->velocity));

              midi->msb      = -1;
              midi->lsb      = -1;
              midi->velocity = -1;
              break;
            }
        }
    }

  return TRUE;
}
