#include "visu~-common.h"
#include "gui/w_dft_waterfall.h"
#include "gui/w_dft_spectrogram.h"
#include "util/unix.h"
#include "util/unix_sock.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <fftw3.h>
#include <getopt.h>
#include <string>
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

constexpr double update_interval = 10e-3;
constexpr double fftdraw_interval = 1 / 30.0;

constexpr unsigned fftsize = 2 * 1024;
std::unique_ptr<fftwf_plan_s, void(*)(fftwf_plan)> fftplan{
  nullptr, &fftwf_destroy_plan};
std::unique_ptr<float []> fftin;
std::unique_ptr<std::complex<float> []> fftout;
bool fftcandraw = false;

constexpr unsigned smemsize = fftsize;
unsigned smemindex = 0;
std::unique_ptr<float[]> smem(new float[smemsize]());

}  // namespace

static MessageHeader *receive_from_fd(SOCKET rfd) {
  uint8_t *recvbuf = ::recvbuf.get();
  MessageHeader *msg = (MessageHeader *)recvbuf;

  size_t nread = eintr_retry(recv, rfd, (char *)msg, msgmax, 0);
  if ((ssize_t)nread == -1)
    return nullptr;
  if (nread < sizeof(MessageHeader) ||
      nread != sizeof(MessageHeader) + msg->len)
    return nullptr;

  return msg;
}

static bool handle_message(const MessageHeader *msg) {
  switch (msg->tag) {
    case MessageTag_SampleRate:
      samplerate = msg->f[0]; break;

    case MessageTag_Toggle:
      if (window->shown())
        window->hide();
      else
        window->show();
      break;

    case MessageTag_Samples: {
      float *smem = ::smem.get();
      for (unsigned i = 0, n = msg->len / sizeof(float); i < n; ++i) {
        smem[smemindex] = msg->f[i];
        smemindex = (smemindex + 1) % smemsize;
      }
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
  unsigned msgcount = 0;
  const int errevents = POLLERR|POLLHUP|POLLNVAL;
  do {
    MessageHeader *msg = receive_from_fd(rfd);
    if (!msg)
      exit(1);
    if (!handle_message(msg))
      exit(1);
    ++msgcount;
    revents = eintr_retry(poll1, rfd, 0, POLLIN);
    if (revents == -1 || (revents & errevents))
      exit(1);
  } while (revents & POLLIN);
}

static float window_nutall(float r) {
  const float pi = M_PI;
  const float a0 = 0.355768, a1 = 0.487396, a2 = 0.144232, a3 = 0.012604;
  return a0 - a1 * std::cos(2 * pi * r) + a2 * std::cos(4 * pi * r)
      - a3 * std::cos(6 * pi * r);
}

static void on_update_timeout(void *) {
#ifdef _WIN32
  if (hparentprocess) {
    DWORD ec = 0;
    bool alive = GetExitCodeProcess(hparentprocess, &ec) && ec == STILL_ACTIVE;
    if (!alive)
      exit(1);
  }
#else
  if (arg_ppid != -1 && getppid() != arg_ppid)
    exit(1);
#endif

  float *fftin = ::fftin.get();
  std::complex<float> *fftout = ::fftout.get();
  unsigned fftsize = ::fftsize;

  for (unsigned i = 0; i < fftsize; ++i) {
    float w = window_nutall(i / float(fftsize-1));
    fftin[fftsize-1-i] = w * smem[(smemindex-i) % smemsize];
  }
  fftwf_execute(fftplan.get());

  for (unsigned i = 0; i < fftsize/2+1; ++i)
    fftout[i] /= fftsize;

  W_DftVisu *dftvisu = ::dftvisu;
  dftvisu->update_data(fftout, fftsize/2+1, samplerate, false);
  fftcandraw = true;

  Fl::add_timeout(update_interval, &on_update_timeout);
}

static void on_fftdraw_timeout(void *) {
  if (fftcandraw) {
    dftvisu->redraw();
    fftcandraw = false;
  }
  Fl::add_timeout(fftdraw_interval, &on_fftdraw_timeout);
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
  fftplan.reset(fftwf_plan_dft_r2c_1d(
      fftsize, fftin.get(), (fftwf_complex *)fftout.get(), FFTW_MEASURE));

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

  Fl::add_timeout(update_interval, &on_update_timeout);
  Fl::add_timeout(fftdraw_interval, &on_fftdraw_timeout);

  if (arg_fd != INVALID_SOCKET)
    Fl::add_fd(arg_fd, FL_READ, &on_fd_input);

  window->wait_for_expose();

  if (arg_fd != INVALID_SOCKET) {
    // ready
    if (eintr_retry(send, arg_fd, "!", 1, 0) == -1)
      throw std::system_error(socket_errno(), socket_category());
  }

  return Fl::run();
}
