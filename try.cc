#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <csignal>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace std;

string FLAGS_elf;
string FLAGS_gen_err = "gen_err";
string FLAGS_run_err = "run_err";
string FLAGS_cmp_err = "cmp_err";
string FLAGS_this = "a.out";
int FLAGS_cnts = 4;

#define EXIT "\e[m"
#define INFO "\e[1;36m"
#define PASS "\e[1;32m"
#define WARN "\e[1;33m"
#define ERRR "\e[1;31m"

void LOG(string msg, bool nl = true) {
  static boost::mutex mutex;
  mutex.lock();
  std::cerr << msg;
  if (nl)
    std::cerr << std::endl;
  mutex.unlock();
}

void LOG_INFO(string msg, bool nl = true) { LOG(INFO + msg + EXIT, nl); }

void LOG_PASS(string msg, bool nl = true) { LOG(PASS + msg + EXIT, nl); }

void LOG_WARN(string msg, bool nl = true) { LOG(WARN + msg + EXIT, nl); }

void LOG_ERRR(string msg, bool nl = true) { LOG(ERRR + msg + EXIT, nl); }

/*
 * Class that encapsulates promise and future object and
 * provides API to set exit signal for the thread
 */
class Stoppable {
  std::promise<void> exitSignal;
  std::future<void> futureObj;

public:
  Stoppable() : futureObj(exitSignal.get_future()) {}

  Stoppable(Stoppable &&obj)
      : exitSignal(std::move(obj.exitSignal)),
        futureObj(std::move(obj.futureObj)) {
    std::cout << "Move Constructor is called" << std::endl;
  }

  Stoppable &operator=(Stoppable &&obj) {
    std::cout << "Move Assignment is called" << std::endl;
    exitSignal = std::move(obj.exitSignal);
    futureObj = std::move(obj.futureObj);
    return *this;
  }

  // Task need to provide defination  for this function
  // It will be called by thread function
  virtual void run() = 0;

  // Thread function to be executed by thread
  void operator()() { run(); }

  // Checks if thread is requested to stop
  bool stopRequested() {
    // checks if value in future object is available
    if (futureObj.wait_for(std::chrono::milliseconds(0)) ==
        std::future_status::timeout)
      return false;
    return true;
  }

  // Request the thread to stop by setting value in promise object
  void stop() { exitSignal.set_value(); }
};

int WINDOW_WIDTH = 0;

void DrawSplit(char ch, string msg = "") {
  int tot = WINDOW_WIDTH;
  if (!msg.empty()) {
    msg = " " + msg + " ";
    int len = msg.size();
    int half = (tot - len) / 2;
    LOG_ERRR(string(half, ch), false);
    LOG_ERRR(msg, false);
    LOG_ERRR(string(half, ch));
  } else {
    LOG_ERRR(string(tot, ch));
  }
}

void AddToInFile(const string &inp, const string &out = "") {
  static boost::mutex mutex;
  mutex.lock();
  ofstream ofs(FLAGS_elf + ".in");
  if (!ofs.is_open()) {
    LOG_ERRR("File cant open!");
    exit(1);
  }
  if (!out.empty()) {
    if (std::ifstream is{inp, std::ios::binary | std::ios::ate}) {
      auto size = is.tellg();
      std::string str(size, '\0');
      is.seekg(0);
      if (is.read(&str[0], size)) {
        ofs << "\n"
            << "Input" << str;
      }
    }
    if (std::ifstream is{out, std::ios::binary | std::ios::ate}) {
      auto size = is.tellg();
      std::string str(size, '\0');
      is.seekg(0);
      if (is.read(&str[0], size)) {
        ofs << "Output" << str << "\n";
      }
    }
  } else {
    if (std::ifstream is{inp, std::ios::binary | std::ios::ate}) {
      auto size = is.tellg();
      std::string str(size, '\0');
      is.seekg(0);
      if (is.read(&str[0], size)) {
        ofs << "\n" << str << "\n";
      }
    }
  }
  ofs.close();
  mutex.unlock();
}

void DumpFile(const string &inp) {
  if (std::ifstream is{inp, std::ios::binary | std::ios::ate}) {
    auto size = is.tellg();
    std::string str(size, '\0');
    is.seekg(0);
    if (is.read(&str[0], size)) {
      LOG(str);
      DrawSplit('-');
    }
  }
}

struct ExecState {
  bool error = false;
  int stage = -1;
  int idx = -1;
};

// --------------------------------------------------------------
shared_ptr<boost::asio::io_service> io_service;
shared_ptr<boost::asio::io_service> io_service2;
shared_ptr<boost::asio::io_service::work> work;

boost::mutex exec_state_mutex;
ExecState exec_state;
boost::thread_group worker_threads;

boost::mutex kill_mutex;
pid_t PID;
// --------------------------------------------------------------

void Worker() { io_service->run(); }

void CheckSignal() { io_service2->run(); }

void HandleErrors(int sig) {
  LOG_INFO("HandleErrors...");
  if (exec_state.error) {
    int idx = exec_state.idx;
    string inp = to_string(idx) + ".gg";
    if (exec_state.stage == 0) {
      DrawSplit('-', "Generating error: " + to_string(idx));
      DumpFile(to_string(idx) + FLAGS_gen_err);
    } else if (exec_state.stage == 1) {
      DrawSplit('-', "Running error: " + to_string(idx));
      DumpFile(to_string(idx) + FLAGS_run_err);
      AddToInFile(inp);
      DumpFile(inp);
    } else if (exec_state.stage == 2) {
      DrawSplit('-', "Compring error: " + to_string(idx));
      DumpFile(to_string(idx) + FLAGS_cmp_err);
      AddToInFile(inp, to_string(idx) + ".gb");
      DumpFile(inp);
      // string diff_cmd = "diff -y -W 60 " + run_out + " " + cmp_out;
      // Executor diff(idx, diff_cmd, "/dev/null", "/dev/null");
    } else {
      DrawSplit('-', "Other error: " + to_string(idx));
    }
  }
  // system("rm -f *.gg *.ga *.gb *_err");
  string kill = "killall " + FLAGS_this;
  system(kill.c_str());
  // Exit the whole program.
  exit(sig);
}

void AsioHandler(const boost::system::error_code &error, int sig) {
  LOG_INFO("asio handling...");
  system("rm -f *.gg *.ga *.gb *_err");
  string kill = "killall " + FLAGS_this;
  system(kill.c_str());
  exit(1);
}

struct Executor {
  string cmd_;
  int idx_;
  string std_out_;
  string std_err_;

  Executor(int idx, const string &cmd, const string &out, const string &err)
      : idx_(idx), cmd_(cmd), std_out_(out), std_err_(err) {}

  bool Run() {
    if (!std_out_.empty())
      cmd_ += " >" + std_out_;
    if (!std_err_.empty())
      cmd_ += " 2>" + std_err_;
    // cmd_ = "bash -c \"" + cmd_ + "\" 1>&2 2>/dev/null";
    // LOG_INFO(cmd_);
    int ret = system(cmd_.c_str());
    exec_state_mutex.lock();
    if (cmd_.find("_ge") != -1) {
      // LOG_INFO("gen");
      exec_state.stage = 0;
    } else if (cmd_.find("_mp") != -1) {
      // LOG_INFO("mp");
      exec_state.stage = 2;
    } else {
      // LOG_INFO("run");
      exec_state.stage = 1;
    }
    exec_state.idx = idx_;
    if (WIFSIGNALED(ret) &&
        (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT)) {
      exec_state.error = false;
      exec_state_mutex.unlock();
      return true;
    }
    int status = WEXITSTATUS(ret);
    LOG_INFO(cmd_ + "---" + to_string(status));
    if (status == 0 || status == 1) {
      // LOG_INFO("AAAAAAAAAAA");
      exec_state.error = false;
      exec_state_mutex.unlock();
      return true;
    } else {
      // If any run fails, raise a SIGINT signal to exiting.
      // LOG_INFO("BBBBBBB");
      // usleep(100000);
      exec_state.error = true;
      kill_mutex.lock();
      LOG_INFO("kill mutex...");
      HandleErrors(SIGINT);
      return false;
    }
  }
};

func FileExist(const std::string &name) bool {
  ifstream f(name.c_str());
  return f.good();
}

struct Xuyi {
  // The command that will be run.
  string generator_;
  string runner_;
  string comparer_;

  Xuyi() {
    runner_ = FLAGS_elf;
    generator_ = FLAGS_elf + "_ge";
    comparer_ = FLAGS_elf + "_mp";
  }

  void Start(int idx) {
    string gen_out = to_string(idx) + ".gg";
    Executor gen(idx, generator_, gen_out, to_string(idx) + FLAGS_gen_err);

    string run_out = to_string(idx) + ".ga";
    string run_cmd = runner_ + "< " + gen_out;
    Executor run(idx, run_cmd, run_out, to_string(idx) + FLAGS_run_err);

    string cmp_out = to_string(idx) + ".gb";
    string cmp_cmd = comparer_ + "< " + gen_out;
    Executor cmp(idx, cmp_cmd, cmp_out, to_string(idx) + FLAGS_cmp_err);

    string diff_cmd = "diff -y -W 60 " + run_out + " " + cmp_out;
    Executor diff(idx, diff_cmd, "", "");

    gen.Run();
    if (FileExist(gen_out)) {
      run.Run();
    } else {
      kill_mutex.lock();
      HandleErrors(SIGINT);
    }
    if (FileExist(gen_out)) {
      cmp.Run();
    } else {
      kill_mutex.lock();
      HandleErrors(SIGINT);
    }
    if (FileExist(run_out) && FileExist(cmp_out)) {
      diff.Run();
    } else {
      kill_mutex.lock();
      HandleErrors(SIGINT);
    }

    LOG("test... " + to_string(idx), false);
    LOG_PASS("\t\t\t\tPASS");
  }
};

void Job(int idx) {
  Xuyi xy;
  xy.Start(idx);
}

void SignalHandle(int sig) {
  LOG_INFO("signal handling...");
  kill_mutex.lock();
  // system("rm -f *.gg *.ga *.gb *_err");
  string kill = "killall " + FLAGS_this;
  system(kill.c_str());
}

int main(int argc, char *argv[]) {
  signal(SIGINT, SignalHandle);

  if (argc == 2) {
    FLAGS_elf = string(argv[1]);
  } else if (argc == 3) {
    FLAGS_elf = string(argv[1]);
    FLAGS_cnts = atoi(argv[2]);
  }

  struct winsize win;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
  WINDOW_WIDTH = win.ws_col;

  io_service.reset(new boost::asio::io_service);
  work.reset(new boost::asio::io_service::work(*io_service));

  // io_service2.reset(new boost::asio::io_service);
  // boost::asio::signal_set signals(*io_service2, SIGINT, SIGTERM);
  // signals.async_wait(AsioHandler);
  // worker_threads.create_thread(boost::bind(&CheckSignal));

  const int kThreads = 4;
  // Create kThreads threads and wait for jobs to work on.
  for (int x = 0; x < kThreads; ++x) {
    worker_threads.create_thread(boost::bind(&Worker));
  }

  for (int i = 0; i < FLAGS_cnts; i++) {
    io_service->post(boost::bind(Job, i));
  }

  work.reset();
  worker_threads.join_all();

  return 0;
}
