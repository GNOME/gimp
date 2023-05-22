/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * controller_midi.c
 * Copyright (C) 2004-2007 Michael Natterer <mitch@gimp.org>
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

#define _GNU_SOURCE  /* the ALSA headers need this */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <glib/gstdio.h>

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#endif

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#define GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
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
  PROP_DEVICE,
  PROP_CHANNEL
};


#define CONTROLLER_TYPE_MIDI            (controller_midi_get_type ())
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
  gint            midi_channel;

  GIOChannel     *io;
  guint           io_id;

#ifdef HAVE_ALSA
  snd_seq_t      *sequencer;
  guint           seq_id;
#endif

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


GType                controller_midi_get_type (void);

static void          midi_dispose             (GObject        *object);
static void          midi_set_property        (GObject        *object,
                                               guint           property_id,
                                               const GValue   *value,
                                               GParamSpec     *pspec);
static void          midi_get_property        (GObject        *object,
                                               guint           property_id,
                                               GValue         *value,
                                               GParamSpec     *pspec);

static gint          midi_get_n_events        (GimpController *controller);
static const gchar * midi_get_event_name      (GimpController *controller,
                                               gint            event_id);
static const gchar * midi_get_event_blurb     (GimpController *controller,
                                               gint            event_id);

static gboolean      midi_set_device          (ControllerMidi *controller,
                                               const gchar    *device);
static void          midi_event               (ControllerMidi *midi,
                                               gint            channel,
                                               gint            event_id,
                                               gdouble         value);

static gboolean      midi_read_event          (GIOChannel     *io,
                                               GIOCondition    cond,
                                               gpointer        data);

#ifdef HAVE_ALSA
static gboolean      midi_alsa_prepare        (GSource        *source,
                                               gint           *timeout);
static gboolean      midi_alsa_check          (GSource        *source);
static gboolean      midi_alsa_dispatch       (GSource        *source,
                                               GSourceFunc     callback,
                                               gpointer        user_data);

static GSourceFuncs alsa_source_funcs =
{
  midi_alsa_prepare,
  midi_alsa_check,
  midi_alsa_dispatch,
  NULL
};

typedef struct _GAlsaSource GAlsaSource;

struct _GAlsaSource
{
  GSource         source;
  ControllerMidi *controller;
};
#endif /* HAVE_ALSA */

static const GimpModuleInfo midi_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("MIDI event controller"),
  "Michael Natterer <mitch@gimp.org>",
  "v0.2",
  "(c) 2004-2007, released under the GPL",
  "2004-2007"
};


G_DEFINE_DYNAMIC_TYPE (ControllerMidi, controller_midi,
                       GIMP_TYPE_CONTROLLER)

static MidiEvent midi_events[128 + 128 + 128];


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &midi_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  controller_midi_register_type (module);

  return TRUE;
}

static void
controller_midi_class_init (ControllerMidiClass *klass)
{
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);
  gchar               *blurb;

  object_class->dispose            = midi_dispose;
  object_class->get_property       = midi_get_property;
  object_class->set_property       = midi_set_property;

  blurb = g_strconcat (_("The name of the device to read MIDI events from."),
#ifdef HAVE_ALSA
                       "\n",
                       _("Enter 'alsa' to use the ALSA sequencer."),
#endif
                       NULL);

  g_object_class_install_property (object_class, PROP_DEVICE,
                                   g_param_spec_string ("device",
                                                        _("Device:"),
                                                        blurb,
                                                        NULL,
                                                        GIMP_CONFIG_PARAM_FLAGS));

  g_free (blurb);

  g_object_class_install_property (object_class, PROP_CHANNEL,
                                   g_param_spec_int ("channel",
                                                     _("Channel:"),
                                                     _("The MIDI channel to read events from. Set to -1 for reading from all MIDI channels."),
                                                     -1, 15, -1,
                                                     GIMP_CONFIG_PARAM_FLAGS));

  controller_class->name            = _("MIDI");
  controller_class->help_id         = "gimp-controller-midi";
  controller_class->icon_name       = GIMP_ICON_CONTROLLER_MIDI;

  controller_class->get_n_events    = midi_get_n_events;
  controller_class->get_event_name  = midi_get_event_name;
  controller_class->get_event_blurb = midi_get_event_blurb;
}

static void
controller_midi_class_finalize (ControllerMidiClass *klass)
{
}

static void
controller_midi_init (ControllerMidi *midi)
{
  midi->device       = NULL;
  midi->midi_channel = -1;
  midi->io           = NULL;
  midi->io_id        = 0;
#ifdef HAVE_ALSA
  midi->sequencer    = NULL;
  midi->seq_id       = 0;
#endif

  midi->swallow      = TRUE; /* get rid of data bytes at start of stream */
  midi->command      = 0x0;
  midi->channel      = 0x0;
  midi->key          = -1;
  midi->velocity     = -1;
  midi->msb          = -1;
  midi->lsb          = -1;
}

static void
midi_dispose (GObject *object)
{
  ControllerMidi *midi = CONTROLLER_MIDI (object);

  midi_set_device (midi, NULL);

  G_OBJECT_CLASS (controller_midi_parent_class)->dispose (object);
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
    case PROP_CHANNEL:
      midi->midi_channel = g_value_get_int (value);
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
    case PROP_CHANNEL:
      g_value_set_int (value, midi->midi_channel);
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
            midi_events[event_id].blurb = g_strdup_printf (_("Note %02x on"),
                                                           event_id);
          else if (event_id <= 255)
            midi_events[event_id].blurb = g_strdup_printf (_("Note %02x off"),
                                                           event_id - 128);
          else if (event_id <= 383)
            midi_events[event_id].blurb = g_strdup_printf (_("Controller %03d"),
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
      g_source_remove (midi->io_id);
      midi->io_id = 0;

      g_io_channel_unref (midi->io);
      midi->io = NULL;
    }

#ifdef HAVE_ALSA
  if (midi->seq_id)
    {
      g_source_remove (midi->seq_id);
      midi->seq_id = 0;

      snd_seq_close (midi->sequencer);
      midi->sequencer = NULL;
    }
#endif /* HAVE_ALSA */

  if (midi->device)
    g_free (midi->device);

  midi->device = g_strdup (device);

  g_object_set (midi, "name", _("MIDI Events"), NULL);

  if (midi->device && strlen (midi->device))
    {
      gint fd;

#ifdef HAVE_ALSA
      if (! g_ascii_strcasecmp (midi->device, "alsa"))
        {
          GSource *event_source;
          gchar   *alsa;
          gchar   *state;
          gint     ret;

          ret = snd_seq_open (&midi->sequencer, "default",
                              SND_SEQ_OPEN_INPUT, 0);
          if (ret >= 0)
            {
              snd_seq_set_client_name (midi->sequencer, _("GIMP"));
              ret = snd_seq_create_simple_port (midi->sequencer,
                                                _("GIMP MIDI Input Controller"),
                                                SND_SEQ_PORT_CAP_WRITE |
                                                SND_SEQ_PORT_CAP_SUBS_WRITE,
                                                SND_SEQ_PORT_TYPE_APPLICATION);
            }

          if (ret < 0)
            {
              state = g_strdup_printf (_("Device not available: %s"),
                                       snd_strerror (ret));
              g_object_set (midi, "state", state, NULL);
              g_free (state);

              if (midi->sequencer)
                {
                  snd_seq_close (midi->sequencer);
                  midi->sequencer = NULL;
                }

              return FALSE;
            }

          /* hack to avoid new message to translate */
          alsa = g_strdup_printf ("ALSA (%d:%d)",
                                  snd_seq_client_id (midi->sequencer),
                                  ret);
          state = g_strdup_printf (_("Reading from %s"), alsa);
          g_free (alsa);

          g_object_set (midi, "state", state, NULL);
          g_free (state);

          event_source = g_source_new (&alsa_source_funcs,
                                       sizeof (GAlsaSource));

          ((GAlsaSource *) event_source)->controller = midi;

          midi->seq_id = g_source_attach (event_source, NULL);
          g_source_unref (event_source);

          return TRUE;
        }
#endif /* HAVE_ALSA */

#ifdef G_OS_WIN32
      fd = g_open (midi->device, O_RDONLY, 0);
#else
      fd = g_open (midi->device, O_RDONLY | O_NONBLOCK, 0);
#endif

      if (fd >= 0)
        {
          gchar *state = g_strdup_printf (_("Reading from %s"), midi->device);
          g_object_set (midi, "state", state, NULL);
          g_free (state);

          midi->io = g_io_channel_unix_new (fd);
          g_io_channel_set_close_on_unref (midi->io, TRUE);
          g_io_channel_set_encoding (midi->io, NULL, NULL);

          midi->io_id = g_io_add_watch (midi->io,
                                        G_IO_IN  | G_IO_ERR |
                                        G_IO_HUP | G_IO_NVAL,
                                        midi_read_event,
                                        midi);
          return TRUE;
        }
      else
        {
          gchar *state = g_strdup_printf (_("Device not available: %s"),
                                          g_strerror (errno));
          g_object_set (midi, "state", state, NULL);
          g_free (state);
        }
    }
  else
    {
      g_object_set (midi, "state", _("No device configured"), NULL);
    }

  return FALSE;
}

static void
midi_event (ControllerMidi *midi,
            gint            channel,
            gint            event_id,
            gdouble         value)
{
  if (channel == -1            ||
      midi->midi_channel == -1 ||
      channel == midi->midi_channel)
    {
      GimpControllerEvent event = { 0, };

      event.any.type     = GIMP_CONTROLLER_EVENT_VALUE;
      event.any.source   = GIMP_CONTROLLER (midi);
      event.any.event_id = event_id;

      g_value_init (&event.value.value, G_TYPE_DOUBLE);
      g_value_set_double (&event.value.value, value);

      gimp_controller_event (GIMP_CONTROLLER (midi), &event);

      g_value_unset (&event.value.value);
    }
}


#define D(stmnt) stmnt;

gboolean
midi_read_event (GIOChannel   *io,
                 GIOCondition  cond,
                 gpointer      data)
{
  ControllerMidi *midi = CONTROLLER_MIDI (data);
  GIOStatus       status;
  GError         *error = NULL;
  guchar          buf[0xf];
  gsize           size;
  gint            pos = 0;

  status = g_io_channel_read_chars (io,
                                    (gchar *) buf,
                                    sizeof (buf), &size,
                                    &error);

  switch (status)
    {
    case G_IO_STATUS_ERROR:
    case G_IO_STATUS_EOF:
      g_source_remove (midi->io_id);
      midi->io_id = 0;

      g_io_channel_unref (midi->io);
      midi->io = NULL;

      if (error)
        {
          gchar *state = g_strdup_printf (_("Device not available: %s"),
                                          error->message);
          g_object_set (midi, "state", state, NULL);
          g_free (state);

          g_clear_error (&error);
        }
      else
        {
          g_object_set (midi, "state", _("End of file"), NULL);
        }
      return FALSE;
      break;

    case G_IO_STATUS_AGAIN:
      return TRUE;

    default:
      break;
    }

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
              D (g_print ("MIDI (ch %02d): note on  (%02x vel %02x)\n",
                          midi->channel,
                          midi->key, midi->velocity));

              midi_event (midi, midi->channel, midi->key,
                          (gdouble) midi->velocity / 127.0);
            }
          else if (midi->command == 0x8)
            {
              D (g_print ("MIDI (ch %02d): note off (%02x vel %02x)\n",
                          midi->channel, midi->key, midi->velocity));

              midi_event (midi, midi->channel, midi->key + 128,
                          (gdouble) midi->velocity / 127.0);
            }
          else
            {
              D (g_print ("MIDI (ch %02d): polyphonic aftertouch (%02x pressure %02x)\n",
                          midi->channel, midi->key, midi->velocity));
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

          D (g_print ("MIDI (ch %02d): controller %d (value %d)\n",
                      midi->channel, midi->key, midi->velocity));

          midi_event (midi, midi->channel, midi->key + 128 + 128,
                      (gdouble) midi->velocity / 127.0);

          midi->key      = -1;
          midi->velocity = -1;
          break;

        case 0xc:  /* program change */
          midi->key = buf[pos++];

          D (g_print ("MIDI (ch %02d): program change (%d)\n",
                      midi->channel, midi->key));

          midi->key = -1;
          break;

        case 0xd:  /* channel key pressure */
          midi->velocity = buf[pos++];

          D (g_print ("MIDI (ch %02d): channel aftertouch (%d)\n",
                      midi->channel, midi->velocity));

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

          D (g_print ("MIDI (ch %02d): pitch (%d)\n",
                      midi->channel, midi->velocity));

          midi->msb      = -1;
          midi->lsb      = -1;
          midi->velocity = -1;
          break;
        }
    }

  return TRUE;
}

#ifdef HAVE_ALSA
static gboolean
midi_alsa_prepare (GSource *source,
                   gint    *timeout)
{
  ControllerMidi *midi = CONTROLLER_MIDI (((GAlsaSource *) source)->controller);
  gboolean        ready;

  ready = snd_seq_event_input_pending (midi->sequencer, 1) > 0;
  *timeout = ready ? 1 : 10;

  return ready;
}

static gboolean
midi_alsa_check (GSource *source)
{
  ControllerMidi *midi = CONTROLLER_MIDI (((GAlsaSource *) source)->controller);

  return snd_seq_event_input_pending (midi->sequencer, 1) > 0;
}

static gboolean
midi_alsa_dispatch (GSource     *source,
                    GSourceFunc  callback,
                    gpointer     user_data)
{
  ControllerMidi *midi = CONTROLLER_MIDI (((GAlsaSource *) source)->controller);

  snd_seq_event_t       *event;
  snd_seq_client_info_t *client_info;
  snd_seq_port_info_t   *port_info;

  if (snd_seq_event_input_pending (midi->sequencer, 1) > 0)
    {
      snd_seq_event_input (midi->sequencer, &event);

      if (event->type == SND_SEQ_EVENT_NOTEON &&
          event->data.note.velocity == 0)
        event->type = SND_SEQ_EVENT_NOTEOFF;

      switch (event->type)
        {
        case SND_SEQ_EVENT_NOTEON:
          midi_event (midi, event->data.note.channel,
                      event->data.note.note,
                      (gdouble) event->data.note.velocity / 127.0);
          break;

        case SND_SEQ_EVENT_NOTEOFF:
          midi_event (midi, event->data.note.channel,
                      event->data.note.note + 128,
                      (gdouble) event->data.note.velocity / 127.0);
          break;

        case SND_SEQ_EVENT_CONTROLLER:
          midi_event (midi, event->data.control.channel,
                      event->data.control.param + 256,
                      (gdouble) event->data.control.value / 127.0);
          break;

        case SND_SEQ_EVENT_PORT_SUBSCRIBED:
          snd_seq_client_info_alloca (&client_info);
          snd_seq_port_info_alloca (&port_info);
          snd_seq_get_any_client_info (midi->sequencer,
                                       event->data.connect.sender.client,
                                       client_info);
          snd_seq_get_any_port_info (midi->sequencer,
                                     event->data.connect.sender.client,
                                     event->data.connect.sender.port,
                                     port_info);
          /*
           * g_printerr ("subscribed to \"%s:%s\"\n",
           *             snd_seq_client_info_get_name (client_info),
           *             snd_seq_port_info_get_name (port_info));
           */
          break;

        case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
          /* g_printerr ("unsubscribed\n");
           */
          break;

        default:
          break;
        }

      return TRUE;
    }

  return FALSE;
}
#endif /* HAVE_ALSA */
