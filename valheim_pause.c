/** @file valheim_pause.c
 *
 * @brief This simple client controls pausing and unpausing Valheim.
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>

#include <jack/jack.h>
#include <jack/midiport.h>

jack_port_t *midi_port;
jack_client_t *client;

pid_t getPID() {
    char path[40], line[100], *p;
    DIR *proc = opendir("/proc");
    FILE *statusf;
    struct dirent *ent;
    pid_t tgid;

    if (proc == NULL) {
        perror("opendir(/proc)");
        return 1;
    }

    while ((ent = readdir(proc))) {
        if (!isdigit(*ent->d_name))
            continue;

        tgid = strtol(ent->d_name, NULL, 10);
        snprintf(path, 40, "/proc/%d/comm", tgid);
        statusf = fopen(path, "r");
        if (!statusf)
            return 0;

        while (fgets(line, 100, statusf)) {
            if (strncmp(line, "valheim", 7) == 0) {
                return tgid;
            }
        }

        fclose(statusf);
    }

    closedir(proc);
    return 0;
}

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client does nothing more than copy data from its input
 * port to its output port. It will exit when stopped by 
 * the user (e.g. using Ctrl-C on a unix-ish operating system)
 */
int process(jack_nframes_t nframes, void *arg) {
    int i;
    void *midi_buf = jack_port_get_buffer(midi_port, nframes);
    jack_midi_event_t midi_event;
    jack_nframes_t event_count = jack_midi_get_event_count(midi_buf);

    if (event_count > 0) {
        for (i = 0; i < event_count; i++) {
            jack_midi_event_get(&midi_event, midi_buf, i);
            if ((*(midi_event.buffer)) == 0xb0) {
                if ((midi_event.buffer[1] == 42) && (midi_event.buffer[2] == 127)) {
                    pid_t pid = getPID();
                    kill(pid, SIGSTOP);
                }
                if ((midi_event.buffer[1] == 41) && (midi_event.buffer[2] == 127)) {
                    pid_t pid = getPID();
                    kill(pid, SIGCONT);
                }
            }
        }
    }

    return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown(void *arg) {
    exit(1);
}

int main(int argc, char *argv[]) {
    const char **ports;
    const char *client_name = "Valheim Pause";
    const char *server_name = NULL;
    jack_options_t options = JackNullOption;
    jack_status_t status;

    daemon(0, 0);

    /* open a client connection to the JACK server */

    client = jack_client_open(client_name, options, &status, server_name);
    if (client == NULL) {
        fprintf(stderr, "jack_client_open() failed, "
                        "status = 0x%2.0x\n", status);
        if (status & JackServerFailed) {
            fprintf(stderr, "Unable to connect to JACK server\n");
        }
        exit(1);
    }
    if (status & JackServerStarted) {
        fprintf(stderr, "JACK server started\n");
    }
    if (status & JackNameNotUnique) {
        client_name = jack_get_client_name(client);
        fprintf(stderr, "unique name `%s' assigned\n", client_name);
    }

    /* tell the JACK server to call `process()' whenever
       there is work to be done.
    */

    jack_set_process_callback(client, process, 0);

    /* tell the JACK server to call `jack_shutdown()' if
       it ever shuts down, either entirely, or if it
       just decides to stop calling us.
    */

    jack_on_shutdown(client, jack_shutdown, 0);

    /* display the current sample rate.
     */

    printf("engine sample rate: %"
    PRIu32
    "\n",
            jack_get_sample_rate(client));

    /* create two ports */

    midi_port = jack_port_register(client, "input",
                                   JACK_DEFAULT_MIDI_TYPE,
                                   (JackPortIsInput | JackPortIsTerminal), 0);
    if (midi_port == NULL) {
        fprintf(stderr, "no MIDI ports available\n");
        exit(1);
    }

    /* Tell the JACK server that we are ready to roll.  Our
     * process() callback will start running now. */

    if (jack_activate(client)) {
        fprintf(stderr, "cannot activate client");
        exit(1);
    }

    /* keep running until stopped by the user */

    sleep(-1);

    /* this is never reached but if the program
       had some other way to exit besides being killed,
       they would be important to call.
    */

    jack_client_close(client);
    exit(0);
}
