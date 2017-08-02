#include "visu~-common.h"
#include "gui/w_dft_waterfall.h"
#include "gui/w_dft_spectrogram.h"
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

SOCKET arg_fd = -1;
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
W_DftVisu *dftvisu = nullptr;

constexpr double fftdraw_interval = 1 / 24.0;

constexpr unsigned fftsize = 2 * 1024;
#ifdef USE_FFTW
std::unique_ptr<fftwf_plan_s, void(*)(fftwf_plan)> fftplan{
  nullptr, &fftwf_destroy_plan};
#else
std::unique_ptr<kfr::dft_plan_real<float>> fftplan;
std::unique_ptr<kfr::u8[]> ffttemp;
#endif
std::unique_ptr<float[]> fftin;
std::unique_ptr<std::complex<float>[]> fftout;
std::unique_ptr<float[]> fftwindow;
bool fftcandraw = false;

constexpr float updatedelta = 10e-3f;

constexpr unsigned smemsize = fftsize;
unsigned smemindex = 0;
std::unique_ptr<float[]> smembuf(new float[2 * smemsize]());
float smemtime = 0;

bool enabled = false;

}  // namespace

static void update_dft_data();
static void on_fftdraw_timeout(void *);
static void on_fd_input(FL_SOCKET, void *);

///
static void enable() {
  if (!::enabled) {
    Fl::add_timeout(fftdraw_interval, &on_fftdraw_timeout);
    if (arg_fd != INVALID_SOCKET)
      Fl::add_fd(arg_fd, FL_READ, &on_fd_input);
    ::enabled = true;
  }
}

static void disable() {
  if (::enabled) {
    Fl::remove_timeout(&on_fftdraw_timeout);
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
      if (window->shown()) {
        disable();
        window->hide();
        ::dftvisu->reset_data();
      } else {
        enable();
        window->show();
      }
      break;

    case MessageTag_Samples: {
      float *smembuf = ::smembuf.get();
      const float sr = ::samplerate;
      float t = ::smemtime;
      for (unsigned i = 0, n = msg->len / sizeof(float); i < n; ++i) {
        smembuf[smemindex] = smembuf[smemindex + smemsize] = msg->f[i];
        smemindex = (smemindex + 1) % smemsize;
        t += 1 / sr;
        if (t >= updatedelta) {
          if (::enabled)
            update_dft_data();
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
  int revents {};

  MessageHeader *msg = nullptr;
  for (int ret; (ret = receive_from_fd(rfd, &msg)) != 0;) {
    if (ret == -1)
      exit(1);
    if (!handle_message(msg))
      exit(1);
  }
}

static float window_nutall(float r) {
  const std::array<float, 4> a {0.355768, -0.487396, 0.144232, -0.012604};
  float p = r * 2 * float(M_PI);
  float w = a[0];
  for (unsigned i = 1; i < a.size(); ++i)
    w += a[i] * std::cos(i * p);
  return w;
}

static void update_dft_data() {
  float *fftin = ::fftin.get();
  std::complex<float> *fftout = ::fftout.get();
  unsigned fftsize = ::fftsize;

  float *smem = ::smembuf.get() + (smemindex - smemsize) % smemsize;
  for (unsigned i = 0; i < fftsize; ++i)
    fftin[i] = fftwindow[i] * smem[i];

#ifdef USE_FFTW
  fftwf_execute(fftplan.get());
#else
  fftplan->execute((kfr::complex<float> *)fftout, fftin, ffttemp.get());
#endif

  for (unsigned i = 0; i < fftsize/2+1; ++i)
    fftout[i] /= fftsize;

  W_DftVisu *dftvisu = ::dftvisu;
  dftvisu->update_data(fftout, fftsize/2+1, samplerate);
  fftcandraw = true;
}

static void on_fftdraw_timeout(void *) {
  if (!check_alive_parent_process())
    exit(1);

  if (fftcandraw) {
    dftvisu->redraw();
    fftcandraw = false;
  }

  Fl::repeat_timeout(fftdraw_interval, &on_fftdraw_timeout);
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
#ifdef WIN32_
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

int main(int argc, char *argv[]) {
  if (!handle_cmdline(argc, argv))
    return 1;

#ifdef _WIN32
  if (arg_ppid != (DWORD)-1) {
    hparentprocess = OpenProcess(PROCESS_QUERY_INFORMATION, false, arg_ppid);
    if (!hparentprocess)
      throw std::system_error(GetLastError(), std::system_category());
  }
#endif

#ifdef _WIN32
  WSADATA wsadata {};
  if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
    throw std::system_error(socket_errno(), socket_category());
#endif

  fftin.reset(new float[fftsize]());
  fftout.reset(new std::complex<float>[fftsize/2+1]());
#ifdef USE_FFTW
  fftplan.reset(fftwf_plan_dft_r2c_1d(
      fftsize, fftin.get(), (fftwf_complex *)fftout.get(), FFTW_MEASURE));
#else
  fftplan.reset(new kfr::dft_plan_real<float>(fftsize));
  ffttemp.reset(new kfr::u8[fftplan->temp_size]());
#endif

  fftwindow.reset(new float[fftsize]());
  for (unsigned i = 0; i < fftsize; ++i)
    fftwindow[i] = window_nutall(i / float(fftsize-1));

  Fl::visual(FL_RGB);

  Fl_Double_Window *window = new Fl_Double_Window(1, 1, ::arg_title.c_str());
  ::window = window;

  switch (::arg_visu) {
    case Visu_Waterfall: {
      window->resize(window->x(), window->y(), 1000, 400);
      ::dftvisu = new W_DftWaterfall(0, 0, window->w(), window->h());
      break;
    }
    case Visu_Spectrogram: {
      window->resize(window->x(), window->y(), 1000, 200);
      ::dftvisu = new W_DftSpectrogram(0, 0, window->w(), window->h());
      break;
    }
    default:
      return 1;
  }

  window->end();

  window->show();
  enable();
  window->wait_for_expose();

  if (arg_fd != INVALID_SOCKET) {
    // ready
    if (socket_retry(send, arg_fd, "!", 1, 0) == -1)
      throw std::system_error(socket_errno(), socket_category());
    if (socksetblocking(arg_fd, false) == -1)
      throw std::system_error(socket_errno(), socket_category());
  }

  return Fl::run();
}
