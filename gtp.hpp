#pragma once
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>

class GTPHelper {
public:
  static GTPHelper &getInstance() {
    static GTPHelper instance;
    return instance;
  }
  GTPHelper() { register_commands(); }
  ~GTPHelper() = default;
  GTPHelper(GTPHelper const &) = delete;
  GTPHelper(GTPHelper &&) = delete;
  GTPHelper &operator=(GTPHelper const &) = delete;
  GTPHelper &operator=(GTPHelper &&) = delete;

public:
  bool execute() {
    std::string cmd;
    fin_ >> cmd;
    if (cmd == "quit") {
      fout_ << "=\n\n";
      return false;
    }
    const auto &dispatch = dispatcher_.find(cmd);
    if (dispatch != dispatcher_.end()) {
      (this->*dispatch->second)();
    } else {
      std::getline(fin_, cmd);
      fout_ << "? unknown command\n\n";
    }
    return true;
  }

private:
  /* Adminstrative Commands */
  void protocol_version() const { fout_ << "= 2\n\n"; }
  void name() const { fout_ << "= GuaGua\n\n"; }
  void version() const { fout_ << "= 1.0\n\n"; }
  void known_command() const {
    std::string cmd;
    fin_ >> cmd;
    if (dispatcher_.find(cmd) != dispatcher_.end()) {
      fout_ << "= true\n\n";
    } else {
      fout_ << "= false\n\n";
    }
  }
  void list_commands() const {
    fout_ << "=\n";
    std::for_each(dispatcher_.begin(), dispatcher_.end(),
                  [](const auto &p) { fout_ << p.first << '\n'; });
    fout_ << "\n";
  }

private:
  /* Setup Commands */
  void boardsize() const {
    int size;
    fin_ >> size;
    if (size == 9) {
      fout_ << "=\n\n";
    } else {
      fout_ << "? unacceptable size\n\n";
    }
  }
  void clear_board() const {
    ; // TODO
    fout_ << "=\n\n";
  }
  void komi() const {
    float komi;
    fin_ >> komi;
    ; // TODO
    fout_ << "=\n\n";
  }

private:
  /* Core Play Commands */
  void play() const {
    std::string bw;
    char p[2];
    fin_ >> bw >> p;
    ; // TODO
    fout_ << "=\n\n";
  }
  void genmove() const {
    std::string bw;
    fin_ >> bw;
    ; // TODO
    fout_ << "=\n\n";
  }
  void undo() const {
    ; // TODO
    fout_ << "=\n\n";
  }

private:
  /* Tournament Commands */
  // void time_left() { ; }

private:
  /* Regression Commands */
  // void loadsgf() { ; }
  // void reg_genmove() { ; }

private:
  /* Debug Commands */
  void showboard() const {
    ; // TODO
    fout_ << "=\n\n";
  }

private:
  void register_commands() {
    using Helper = void (GTPHelper::*)();
    // Adminstrative Commands
    dispatcher_.emplace("protocol_version",
                        (Helper)&GTPHelper::protocol_version);
    dispatcher_.emplace("name", (Helper)&GTPHelper::name);
    dispatcher_.emplace("version", (Helper)&GTPHelper::version);
    dispatcher_.emplace("known_command", (Helper)&GTPHelper::known_command);
    dispatcher_.emplace("list_commands", (Helper)&GTPHelper::list_commands);

    // Setup Commands
    dispatcher_.emplace("boardsize", (Helper)&GTPHelper::boardsize);
    dispatcher_.emplace("clear_board", (Helper)&GTPHelper::clear_board);
    dispatcher_.emplace("komi", (Helper)&GTPHelper::komi);
    // Core Play Commands
    dispatcher_.emplace("play", (Helper)&GTPHelper::play);
    dispatcher_.emplace("genmove", (Helper)&GTPHelper::genmove);
    dispatcher_.emplace("undo", (Helper)&GTPHelper::undo);
    // Debug Commands
    dispatcher_.emplace("showboard", (Helper)&GTPHelper::showboard);
  }

private:
  constexpr static std::istream &fin_ = std::cin;
  constexpr static std::ostream &fout_ = std::cout;
  std::map<std::string, void (GTPHelper::*)()> dispatcher_;
};
