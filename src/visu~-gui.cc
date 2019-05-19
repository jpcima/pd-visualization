#include "visu~-common.h"
#include "gui/w_dft_waterfall.h"
#include "gui/w_dft_spectrogram.h"
#include "gui/w_dft_transfer.h"
#include "gui/w_ts_oscillogram.h"
#include "gui/s_smem.h"
#include "gui/s_math.h"
#include "util/unix.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#ifdef USE_FFTW
# include <fftw3.h>
#else
# include <kfr/dft.hpp>
#endif
#include <getopt.h>
#include <algorithm>
#include <string>
#include <array>
#include <memory>
#include <complex>
#include <cmath>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef _WIN32
# include <winsock2.h>
#endif

namespace {

SOCKET arg_fd = INVALID_SOCKET;
#ifdef _WIN32
DWORD arg_ppid = -1;
HANDLE hparentprocess = nullptr;
#else
pid_t arg_ppid = -1;
#endif
VisuType arg_visu = Visu_Default;
std::string arg_title;

std::unique_ptr<uint8_t[]> recvbuf(new uint8_t[msgmax]);

float samplerate = 44100;

Fl_Double_Window *window = nullptr;
W_Visu *visu = nullptr;

constexpr double redraw_interval = 1 / 24.0;

VisuDftResolution fftres = (VisuDftResolution)-1;
unsigned fftsize = 2 * 1024;
#ifdef USE_FFTW
std::unique_ptr<fftwf_plan_s, void(*)(fftwf_plan)> fftplan{
  nullptr, &fftwf_destroy_plan};
#else
std::unique_ptr<kfr::dft_plan_real<float>> fftplan;
std::unique_ptr<kfr::u8[]> ffttemp;
#endif
std::unique_ptr<float[]> fftin;
std::unique_ptr<std::complex<float>[]> fftout[channelmax];
std::unique_ptr<float[]> fftwindow;

constexpr float updatedelta = 10e-3f;

sample_memory<> smem;
float smemtime = 0;
unsigned schannels = 1;

bool enabled = false;
bool visucandraw = false;
void (*initfn)() = nullptr;
void (*updatefn)() = nullptr;

}  // namespace

static void on_redraw_timeout(void *);
static void on_fd_input(FL_SOCKET, void *);

///
static void enable() {
  if (!::enabled) {
    Fl::add_timeout(redraw_interval, &on_redraw_timeout);
    if (arg_fd != INVALID_SOCKET)
      Fl::add_fd(arg_fd, FL_READ, &on_fd_input);
    ::enabled = true;
  }
}

static void disable() {
  if (::enabled) {
    Fl::remove_timeout(&on_redraw_timeout);
    if (arg_fd != INVALID_SOCKET)
      Fl::remove_fd(arg_fd);
    ::enabled = false;
  }
}

static bool check_alive_parent_process() {
#ifdef _WIN32
  if (!hparentprocess)
    return true;
  DWORD ec = 0;
  return GetExitCodeProcess(hparentprocess, &ec) && ec == STILL_ACTIVE;
#else
  if (arg_ppid == -1)
    return true;
  return getppid() == arg_ppid;
#endif
}

static int receive_from_fd(SOCKET rfd, MessageHeader **pmsg) {
  if (!check_alive_parent_process())
    exit(1);

  uint8_t *recvbuf = ::recvbuf.get();
  MessageHeader *msg = (MessageHeader *)recvbuf;

  size_t nread = socket_retry(recv, rfd, (char *)msg, msgmax, 0);
  if ((ssize_t)nread == -1) {
    if (socket_errno() == SOCK_ERR(EWOULDBLOCK))
      return 0;
    return -1;
  }
  if (nread == 0)
    return 0;
  if (nread < sizeof(MessageHeader) ||
      nread != sizeof(MessageHeader) + msg->len)
    return -1;

  if (pmsg) *pmsg = msg;
  return 1;
}

static bool handle_message(const MessageHeader *msg) {
  switch (msg->tag) {
    case MessageTag_SampleRate: {
      float sr = msg->f[0];
      if (!std::isfinite(sr) || sr <= 0)
        exit(1);
      ::samplerate = sr;
      break;
    }

    case MessageTag_Toggle:
      if (window->visible())
        window->hide();
      else
        window->show();
      break;

    case MessageTag_Frames: {
      const float *data = &msg->f[0];
      unsigned datalen = msg->len;

      if (datalen < sizeof(uint32_t))
          exit(1);
      uint32_t nchannels = *(uint32_t *)data++;
      datalen -= sizeof(*data);
      if (nchannels == 0 || nchannels > channelmax)
          exit(1);
      uint32_t nframes = datalen / (nchannels * sizeof(float));

      sample_memory<> &smem = ::smem;
      const float sr = ::samplerate;
      float t = ::smemtime;
      ::schannels = nchannels;

      for (unsigned i = 0; i < nframes; ++i) {
        smem.append(&data[i * nchannels], nchannels);
        t += 1 / sr;
        if (t >= updatedelta) {
          if (::enabled && ::updatefn)
            ::updatefn();
          t -= updatedelta;
        }
      }
      ::smemtime = t;
      break;
    }

    default:
      return false;
  }
  return true;
}

static void on_fd_input(FL_SOCKET, void *) {
  SOCKET rfd = ::arg_fd;
  MessageHeader *msg = nullptr;
  for (int ret; (ret = receive_from_fd(rfd, &msg)) != 0;) {
    if (ret == -1)
      exit(1);
    if (!handle_message(msg))
      exit(1);
  }
}

static void dft_prepare() {
  const unsigned fftsize = ::fftsize;
  smem.resize(fftsize);

  fftin.reset(new float[fftsize]());
  for (unsigned c = 0; c < channelmax; ++c)
      fftout[c].reset(new std::complex<float>[fftsize/2+1]());

#ifdef USE_FFTW
  fftplan.reset(fftwf_plan_dft_r2c_1d(
      fftsize, fftin.get(), (fftwf_complex *)fftout[0].get(), FFTW_MEASURE));
#else
  fftplan.reset(new kfr::dft_plan_real<float>(fftsize));
  ffttemp.reset(new kfr::u8[fftplan->temp_size]());
#endif

  fftwindow.reset(new float[fftsize]());
  for (unsigned i = 0; i < fftsize; ++i)
    fftwindow[i] = window_nutall(i / float(fftsize-1));
}

static void dft_initres(VisuDftResolution res) {
  if (res == ::fftres)
    return;

  ::fftres = res;
  switch (res) {
    case VisuDftResolution::Medium:
      ::fftsize = 2048; break;
    case VisuDftResolution::High:
      ::fftsize = 8192; break;
  }
  dft_prepare();
}

static void dft_init() {
  dft_initres(VisuDftResolution::Medium);
}

static void dft_update() {
  W_DftVisu *dftvisu = static_cast<W_DftVisu *>(::visu);
  dft_initres(dftvisu->desired_resolution());

  float *fftin = ::fftin.get();
  const unsigned fftsize = ::fftsize;
  const unsigned nchannels = ::schannels;
  const frame<> *smem = ::smem.data();

  for (unsigned c = 0; c < nchannels; ++c) {
      std::complex<float> *fftout = ::fftout[c].get();
      for (unsigned i = 0; i < fftsize; ++i)
          fftin[i] = fftwindow[i] * smem[i].samples[c];
#ifdef USE_FFTW
      fftwf_execute_dft_r2c(fftplan.get(), fftin, (fftwf_complex *)fftout);
#else
      fftplan->execute((kfr::complex<float> *)fftout, fftin, ::ffttemp.get());
#endif
      for (unsigned i = 0; i < fftsize/2+1; ++i)
          fftout[i] /= fftsize;
  }

  const std::complex<float> *spec[channelmax];
  for (unsigned c = 0; c < nchannels; ++c)
      spec[c] = ::fftout[c].get();

  dftvisu->update_dft_data(spec, fftsize/2+1, samplerate, nchannels);
  visucandraw = true;
}

static void ts_init() {
  constexpr float sampleratemax = 192000;
  constexpr float timebasemax = 1.0;
  unsigned memsize = std::ceil(timebasemax * sampleratemax);
  smem.resize(memsize);
}

static void ts_update() {
  const sample_memory<> &smem = ::smem;
  const unsigned nchannels = ::schannels;
  W_TsVisu *tsvisu = static_cast<W_TsVisu *>(::visu);
  tsvisu->update_ts_data(samplerate, smem.data(), smem.size(), nchannels);
  visucandraw = true;
}

static void on_redraw_timeout(void *) {
  if (!check_alive_parent_process())
    exit(1);

  if (visucandraw) {
    visu->redraw();
    visucandraw = false;
  }

  Fl::repeat_timeout(redraw_interval, &on_redraw_timeout);
}

static bool handle_cmdline(int argc, char *argv[]) {
  const struct option opts[] = {
    {"fd", required_argument, nullptr, 256 + 'f'},
    {"ppid", required_argument, nullptr, 256 + 'p'},
    {"visu", required_argument, nullptr, 256 + 'v'},
    {"title", required_argument, nullptr, 256 + 't'},
    {},
  };

  for (int c; (c = getopt_long(argc, argv, "", opts, nullptr)) != -1;) {
    switch (c) {
      case 256 + 'f': {
        unsigned len = 0;
        if (sscanf(optarg, "%" SCNdSOCKET "%n", &::arg_fd, &len) != 1 ||
            len != strlen(optarg))
          return false;
        break;
      }
      case 256 + 'p': {
        unsigned len = 0;
#ifdef _WIN32
        unsigned count = sscanf(optarg, "%lu%n", &::arg_ppid, &len);
#else
        unsigned count = sscanf(optarg, "%d%n", &::arg_ppid, &len);
#endif
        if (count != 1 || len != strlen(optarg))
          return false;
        break;
      }
      case 256 + 'v': {
        unsigned len = 0;
        int v = 0;
        if (sscanf(optarg, "%d%n", &v, &len) != 1 || len != strlen(optarg))
          return false;
        ::arg_visu = (VisuType)v;
        break;
      }
      case 256 + 't': {
        ::arg_title.assign(optarg);
        break;
      }
      default:
        return false;
    }
  }

  if (argc != optind)
    return false;

  return true;
}

class W_MainWindow : public Fl_Double_Window {
 public:
  W_MainWindow(int w, int h, const char *l = nullptr)
      : Fl_Double_Window(w, h, l) {}
  // note: superclass destructor is non-virtual
  int handle(int event) override;
};

int W_MainWindow::handle(int event) {
  if (event == FL_SHOW) {
    enable();
  } else if (event == FL_HIDE) {
    disable();
    ::visu->reset_data();
  }
  return Fl_Double_Window::handle(event);
}

int main(int argc, char *argv[]) {
  if (!handle_cmdline(argc, argv))
    return 1;

#ifdef _WIN32
  if (arg_ppid != (DWORD)-1) {
    hparentprocess = OpenProcess(PROCESS_QUERY_INFORMATION, false, arg_ppid);
    if (!hparentprocess)
      throw std::system_error(GetLastError(), std::system_category(), "OpenProcess");
  }
#endif

#ifdef _WIN32
  WSADATA wsadata {};
  if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
    throw std::system_error(socket_errno(), socket_category(), "WSAStartup");
#endif

  Fl::visual(FL_RGB);

  int w = 1, h = 1;

  Fl_Double_Window *window = new W_MainWindow(w, h, ::arg_title.c_str());
  ::window = window;

  switch (::arg_visu) {
    case Visu_Waterfall: {
      w = 1000;
      h = 400;
      window->size(w, h);
      ::visu = new W_DftWaterfall(0, 0, w, h);
      ::initfn = &dft_init;
      ::updatefn = &dft_update;
      break;
    }
    case Visu_Spectrogram: {
      w = 1000;
      h = 250;
      window->size(w, h);
      ::visu = new W_DftSpectrogram(0, 0, w, h);
      ::initfn = &dft_init;
      ::updatefn = &dft_update;
      break;
    }
    case Visu_Oscillogram: {
      w = 1000;
      h = 300;
      window->size(w, h);
      ::visu = new W_TsOscillogram(0, 0, w, h);
      ::initfn = &ts_init;
      ::updatefn = &ts_update;
      break;
    }
    case Visu_Transfer: {
      w = 1000;
      h = 400;
      window->size(w, h);
      ::visu = new W_DftTransfer(0, 0, w, h);
      ::initfn = &dft_init;
      ::updatefn = &dft_update;
      break;
    }
    default:
      return 1;
  }

  window->resizable(::visu);
  window->size_range(w / 10, h / 10);

  if (::initfn)
    ::initfn();

  window->end();

  window->show();
  window->wait_for_expose();

  if (arg_fd != INVALID_SOCKET) {
    // ready
    if (socket_retry(send, arg_fd, "!", 1, 0) == -1)
      throw std::system_error(socket_errno(), socket_category(), "send");
    if (socksetblocking(arg_fd, false) == -1)
      throw std::system_error(socket_errno(), socket_category(), "socksetblocking");
  }

  return Fl::run();
}
