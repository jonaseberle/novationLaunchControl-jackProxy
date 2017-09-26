// gcc -o jack_midi_dump -Wall midi_dump.c -ljack -pthread

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>

#include <signal.h>
#include <pthread.h>
#include <sys/mman.h>

static jack_port_t* portIn;
static jack_port_t* portMidiControlIn;
static jack_port_t* portMmcIn;
static jack_port_t* portOut;
static jack_port_t* portMidiControlOut;

static pthread_mutex_t msg_thread_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t data_ready = PTHREAD_COND_INITIALIZER;
size_t message_size = 3;

void* jackBufferIn;
void* jackBufferOut;
unsigned char* eventBufferOut;

static int keeprunning = 1;

int state = 0;
int mutePressed = 0;
int soloPressed = 0;
int recArmPressed = 0;

const int trackFocusNotesPreset1[8] = { 41, 42, 43, 44, 57, 58, 59, 60 };
const int trackControlNotesPreset1[8] = { 73, 74, 75, 76, 89, 90, 91, 92 };

static void
describe (const jack_midi_event_t* event)
{
	if (event->size == 0) {
        printf ("empty event\n");
        return;
	}

	uint8_t type = event->buffer[0] & 0xf0;
    uint8_t channel = event->buffer[0] & 0xf;

	switch (type) {
		case 0x90:
			assert (event->size == 3);
            printf ("+  ");
			break;
		case 0x80:
			assert (event->size == 3);
            printf ("-  ");
            break;
        case 0xb0:
            assert (event->size == 3);
            printf ("CC ");
            break;
        case 0xf0:
            printf ("SX ");
            break;
        default:
            printf ("0x%x:", type);
            break;
    }
    printf (
        "{%#x} [c%2d] 0x%02x[%3d] 0x%02x[%3d]",
        event->buffer[0],
        channel + 1,
        event->buffer[1],event->buffer[1],
        event->buffer[2],event->buffer[2]
    );
    for (size_t i = 3; i < event->size; i++) {
        printf (
            " %#x[%d]",
            event->buffer[i], event->buffer[i]
        );
    }
    printf ("\n");
}

int
send (jack_midi_event_t event, jack_nframes_t frames, jack_nframes_t frame) {
    printf("‹—M  ");
    describe (&event);
    jackBufferOut = jack_port_get_buffer(portOut, frames);
    assert (jackBufferOut);
    eventBufferOut = jack_midi_event_reserve(jackBufferOut, frame, message_size);
    memcpy(eventBufferOut, event.buffer, event.size * sizeof(jack_midi_data_t));
}

int
process (jack_nframes_t frames, void* arg)
{
    jack_midi_clear_buffer(jack_port_get_buffer(portOut, frames));

    jackBufferIn = jack_port_get_buffer (portIn, frames);
    assert (jackBufferIn);

    for (jack_nframes_t frame = 0; frame < jack_midi_get_event_count (jackBufferIn); ++frame) {
        jack_midi_event_t event;

        // read
        jack_midi_event_get (&event, jackBufferIn, frame);

        printf("  ——›");
        describe (&event);

        // register state

        // https://de.wikipedia.org/wiki/Musical_Instrument_Digital_Interface

        // Ardour PORT: MMC Out (SX: manuf.=0x7f)
        // https://www.midi.org/specifications/item/table-1-summary-of-midi-message
        // play:      ——›SX [c 0] 0x7f[127] 0x00[  0] 0x6[6] 0x3[3] 0xf7[247]
        // stop:      ——›SX [c 0] 0x7f[127] 0x00[  0] 0x6[6] 0x1[1] 0xf7[247]
        // rec arm    ——›SX [c 0] 0x7f[127] 0x00[  0] 0x6[6] 0x8[8] 0xf7[247]
        // rec unarm  ——›SX [c 0] 0x7f[127] 0x00[  0] 0x6[6] 0x7[7] 0xf7[247]
        // rec start  ——›SX [c 0] 0x7f[127] 0x00[  0] 0x6[6] 0x6[6] 0xf7[247]
        //            ——›SX [c 0] 0x7f[127] 0x00[  0] 0x6[6] 0x3[3] 0xf7[247]
        // rec sto    ——›SX [c 0] 0x7f[127] 0x00[  0] 0x6[6] 0x7[7] 0xf7[247]
        //            ——›SX [c 0] 0x7f[127] 0x00[  0] 0x6[6] 0x1[1] 0xf7[247]



        if (event.buffer[1] == 0x45) {
            state++;
            printf("new state: %d\n", state);
        }

        // manipulate event
        // Factory preset 1 = channel 9,
        // Factory preset 2 = channel 10, ...

        // mute
        if (event.buffer[1] == 0x6a) {
            mutePressed = event.buffer[0] == 0x98;
        }

        // solo
        if (event.buffer[1] == 0x6b) {
            soloPressed = event.buffer[0] == 0x98;
        }

        // rec arm
        if (event.buffer[1] == 0x6c) {
            recArmPressed = event.buffer[0] == 0x98;
        }

        // Track Focus
        if (event.buffer[0] == 0x98 && event.buffer[2] == 0x7f) {
            for (int i = 0; i < 8; i++) {
                if (event.buffer[1] == trackFocusNotesPreset1[i]) {
                    if (mutePressed) {
                        send(event, frames, frame);
                        event.buffer[1] += 4;
                    }
                    if (soloPressed) {
                        send(event, frames, frame);
                        event.buffer[1] += 4;
                    }
                    if (recArmPressed) {
                        send(event, frames, frame);
                        event.buffer[1] += 4;
                    }
                }
            }
        }

        if (soloPressed && event.buffer[0] == 0x98 && event.buffer[1] == 73 && event.buffer[2] == 0x7f) {
            send(event, frames, frame);
            event.buffer[1] = 0;
printf("MUTED track \n");
        }


        // write
        if (1) {
            // change channel
//            event.buffer[0] &= 0xf0 | 9;
            send(event, frames, frame);
        }
    }


	if (pthread_mutex_trylock (&msg_thread_lock) == 0) {
		pthread_cond_signal (&data_ready);
		pthread_mutex_unlock (&msg_thread_lock);
	}

	return 0;
}

static void wearedone(int sig) {
	keeprunning = 0;
}

int
main (int argc, char* argv[])
{
	jack_client_t* client;

    client = jack_client_open ("Launch Control XL proxy", JackNullOption, NULL);
	if (client == NULL) {
		fprintf (stderr, "Could not create JACK client.\n");
		exit (EXIT_FAILURE);
	}
    portIn = jack_port_register (client, "L C XL in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
    portMmcIn = jack_port_register (client, "MMC in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
    portMidiControlIn = jack_port_register (client, "Midi Control in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsTerminal, 0);
    portOut = jack_port_register (client, "L C XL out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);
    portMidiControlOut = jack_port_register (client, "Midi Control out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput | JackPortIsTerminal, 0);

    jack_set_process_callback (client, process, 0);

	if (mlockall (MCL_CURRENT | MCL_FUTURE)) {
		fprintf (stderr, "Warning: Can not lock memory.\n");
	}

    if (jack_activate (client) != 0) {
		fprintf (stderr, "Could not activate client.\n");
		exit (EXIT_FAILURE);
	}

	signal(SIGHUP, wearedone);
	signal(SIGINT, wearedone);

	pthread_mutex_lock (&msg_thread_lock);

    while (keeprunning) {
//		fflush (stdout);
        pthread_cond_wait (&data_ready, &msg_thread_lock);
	}
	pthread_mutex_unlock (&msg_thread_lock);

	jack_deactivate (client);
	jack_client_close (client);

	return 0;
}
