
#include "visu~.h"
#include "visu~-remote.h"
#include "util/scope_guard.h"
#include <jack/jack.h>
#include <getopt.h>
#include <memory>
#include <cstring>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

static jack_client_t *g_client = nullptr;
static jack_port_t *g_input[channelmax] = {};
static std::unique_ptr<t_visu> g_visu;
static float g_samplerate = 44100;
static unsigned g_channels = 2;

// substitutions of pd standard routines
extern "C" float sys_getsr() { return g_samplerate; }
extern "C" void dsp_addv(t_perfroutine, int, t_int *) { /* don't need */ }
extern "C" t_inlet *inlet_new(t_object *, t_pd *, t_symbol *, t_symbol *) { return nullptr; /* don't need */ }
extern "C" t_symbol *gensym(const char *) { return nullptr; /* don't need */ }

//------------------------------------------------------------------------------
static int cb_jack_process(jack_nframes_t nframes, void *) {
  t_int w[3 + channelmax];
  t_int *wp = w;
  *wp++ = (t_int)&visu_perform;
  *wp++ = (t_int)g_visu.get();
  for (unsigned i = 0, n = g_channels; i < n; ++i)
      *wp++ = (t_int)jack_port_get_buffer(g_input[i], nframes);
  *wp++ = (t_int)nframes;
  visu_perform(w);
  return 0;
}

//------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  const char *visuname = nullptr;
  for (int c; (c = getopt(argc, argv, "t:c:h")) != -1;) {
    switch (c) {
      case 't':
        visuname = optarg; break;
      case 'c': {
        unsigned nch = ::atoi(optarg);
        if (nch < 1 || nch > channelmax) {
            fprintf(stderr, "invalid number of channels (1-%u)\n", channelmax);
            return 1;
        }
        g_channels = nch; break;
      }
      case 'h':
      default:
        fprintf(stderr, "Usage: %s [-t type] [-c channels] [title]\n", argv[0]);
        return 1;
    }
  }

  if (!visuname) {
    const char *progname = argv[0];
    size_t pos;
    for (pos = strlen(progname); pos > 0; --pos) {
      if (progname[pos - 1] == '/') break;
#ifdef _WIN32
      if (progname[pos - 1] == '\\') break;
#endif
    }
    visuname = progname + pos;
  }

  //
  static const char *visu_names[] =
      {"wfvisu~", "sgvisu~", "ogvisu~", "tfvisu~"};
  static const VisuType visu_types[] = {
    Visu_Waterfall, Visu_Spectrogram, Visu_Oscillogram, Visu_Transfer};

  //
  const char *name;
  if (optind == argc)
    name = "untitled";
  else if (optind == argc - 1)
    name = argv[optind];
  else
    return 1;

  //
  unsigned visu_index = (unsigned)-1;
  for (unsigned i = 0; i < Visu_Count && visu_index == (unsigned)-1; ++i)
    if (!std::strcmp(visuname, visu_names[i]))
      visu_index = i;
  if (visu_index == (unsigned)-1)
    return 1;

  //
  {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGCHLD);
    pthread_sigmask(SIG_BLOCK, &mask, nullptr);
  }

  //
  VisuType type = visu_types[visu_index];
  t_visu *visu = new t_visu;
  g_visu.reset(visu);
  visu_init(visu, type);
  visu->x_title = name;
  visu->x_channels = g_channels;

  //
  char *client_name;
  if (asprintf(&client_name, "%s %s", visuname, name) == -1)
    throw std::bad_alloc();
  scope(exit) { free(client_name); };

  g_client = jack_client_open(client_name, JackNoStartServer, nullptr);
  if (!g_client)
    throw std::runtime_error("cannot open jack client");
  scope(exit) { jack_client_close(g_client); };

  //
  for (unsigned i = 0, n = g_channels; i < n; ++i) {
      char portname[32];
      sprintf(portname, "channel %u", i + 1);
      jack_port_t *port = jack_port_register(
          g_client, portname, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput|JackPortIsTerminal, 0);
      if (!port)
          throw std::runtime_error("cannot register jack ports");
      g_input[i] = port;
  }

  //
  g_samplerate = jack_get_sample_rate(g_client);
  jack_set_process_callback(g_client, &cb_jack_process, nullptr);
  if (jack_activate(g_client))
    throw std::runtime_error("cannot activate jack client");

  //
  visu_bang(visu);

  //
  bool signaled = false;
  while (!signaled) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGCHLD);
    int sig;
    if (!sigwait(&mask, &sig)) {
      if (sig != SIGCHLD) {
        signaled = true;
      } else {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        signaled = pid != -1 && (WIFEXITED(status) || WIFSIGNALED(status));
      }
    }
    if (signaled) {
      jack_client_close(g_client);
      _exit(1);
    }
  }

  //
  return 0;
}
