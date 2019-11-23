#pragma once
#include "agent.hpp"
#include "board.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <vector>

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
      dispatch->second();
    } else {
      std::getline(fin_, cmd);
      fout_ << "? unknown command\n\n";
    }
    return true;
  }

  void registerAgent(std::unique_ptr<Agent> agent) {
    agent_ = std::move(agent);
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
    std::transform(dispatcher_.begin(), dispatcher_.end(),
                   std::ostream_iterator<std::string>(fout_, "\n"),
                   [](const auto &p) { return p.first; });
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
  void clear_board() {
    Board b;
    std::swap(board_, b);
    history_.clear();
    gogui_turns_ = true;
    fout_ << "=\n\n";
  }
  void komi() const {
    // not used
    float komi;
    fin_ >> komi;
    fout_ << "=\n\n";
  }

private:
  /* Core Play Commands */
  void play() {
    std::string sbw;
    Position pos;
    fin_ >> sbw >> pos;
    size_t bw = tolower(sbw[0]) == 'w' ? 1 : 0;
    if (board_.place(bw, static_cast<size_t>(pos))) {
      fout_ << "=\n\n";
    } else {
      fout_ << "? illegal move\n\n";
      return;
    }
    history_.push_back(board_);
    gogui_turns_ = !gogui_turns_;
  }
  void genmove() {
    std::string sbw;
    fin_ >> sbw;
    size_t bw = tolower(sbw[0]) == 'w' ? 1 : 0;
    size_t move = agent_->take_action(board_, bw);
    history_.push_back(board_);
    board_.place(bw, move);
    fout_ << "= " << Position(move) << "\n\n";
  }
  void undo() {
    if (history_.empty()) {
      fout_ << "? cannot undo\n\n";
      return;
    }
    board_ = history_.back();
    history_.pop_back();
    fout_ << "=\n\n";
  }

private:
  /* Tournament Commands */
  // void time_settings() { ; }
  void final_score() const {
    fout_ << "= " << (!gogui_turns_ ? "B" : "W") << "+1\n\n";
  }

private:
  /* Regression Commands */
  // void loadsgf() { ; }
  // void reg_genmove() { ; }

private:
  /* Debug Commands */
  void showboard() const { fout_ << "=\n" << board_; }

private:
  /* GoGui Rules */
  // void gogui_analyze_command() const { ; }
  void gogui_rules_game_id() const { fout_ << "= Nogo\n\n"; }
  void gogui_rules_board() const { fout_ << "=\n" << board_; }
  void gogui_rules_board_size() const { fout_ << "= 9\n\n"; }
  void gogui_rules_legal_moves() const {
    size_t bw = gogui_turns_ ? 0 : 1;
    fout_ << "= ";
    auto moves = board_.get_legal_moves(bw);
    // std::sort(moves.begin(), moves.end(), [](size_t lhs, size_t rhs) {
    //   Position lp(lhs), rp(rhs);
    //   return (8 - lp.p1) * 9 + lp.p0 < (8 - rp.p1) * 9 + rp.p0;
    // });
    std::transform(moves.begin(), moves.end(),
                   std::ostream_iterator<Position>(fout_, " "),
                   [](const auto &p) { return Position(p); });
    fout_ << "\n\n";
  }
  void gogui_rules_side_to_move() {
    fout_ << "= " << (gogui_turns_ ? "black" : "white") << "\n\n";
  }
  void gogui_rules_final_result() const {
    fout_ << "= " << (!gogui_turns_ ? "Black" : "White") << " wins.\n\n";
  }

private:
  template <class F> constexpr void register_command(std::string &&cmd, F &&f) {
    dispatcher_.emplace(cmd, std::bind(f, this));
  }

  void register_commands() {
    // Adminstrative Commands
    dispatcher_.emplace("quit", nullptr);
    register_command("protocol_version", &GTPHelper::protocol_version);
    register_command("name", &GTPHelper::name);
    register_command("version", &GTPHelper::version);
    register_command("known_command", &GTPHelper::known_command);
    register_command("list_commands", &GTPHelper::list_commands);
    // Setup Commands
    register_command("boardsize", &GTPHelper::boardsize);
    register_command("clear_board", &GTPHelper::clear_board);
    register_command("komi", &GTPHelper::komi);
    // Core Play Commands
    register_command("play", &GTPHelper::play);
    register_command("genmove", &GTPHelper::genmove);
    register_command("undo", &GTPHelper::undo);
    // Tournament Commands
    register_command("final_score", &GTPHelper::final_score);
    // Debug Commands
    register_command("showboard", &GTPHelper::showboard);
    // GoGui Commands
    register_command("gogui-rules_game_id", &GTPHelper::gogui_rules_game_id);
    register_command("gogui-rules_board", &GTPHelper::gogui_rules_board);
    register_command("gogui-rules_board_size",
                     &GTPHelper::gogui_rules_board_size);
    register_command("gogui-rules_legal_moves",
                     &GTPHelper::gogui_rules_legal_moves);
    register_command("gogui-rules_side_to_move",
                     &GTPHelper::gogui_rules_side_to_move);
    register_command("gogui-rules_final_result",
                     &GTPHelper::gogui_rules_final_result);
  }

private:
  constexpr static std::istream &fin_ = std::cin;
  constexpr static std::ostream &fout_ = std::cout;
  std::map<std::string, std::function<void()>> dispatcher_;
  Board board_;
  std::vector<Board> history_;
  std::unique_ptr<Agent> agent_;
  bool gogui_turns_ = true;
};
