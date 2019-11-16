#pragma once
#include "agent.hpp"
#include "board.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
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
      (this->*dispatch->second)();
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
  void clear_board() {
    Board b;
    std::swap(board_, b);
    history_.clear();
    gogui_turns = true;
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
  void play() {
    std::string sbw, pos;
    fin_ >> sbw >> pos;
    size_t p0 = tolower(pos[0]) - 'a';
    size_t p = (p0 - (p0 > 8)) + (8 - (pos[1] - '1')) * 9;
    if (board_.place(tolower(sbw[0]) == 'w', p)) {
      fout_ << "=\n\n";
    } else {
      fout_ << "? illegal move\n\n";
      return;
    }
    history_.push_back(board_);
    gogui_turns = !gogui_turns;
  }
  void genmove() {
    std::string sbw;
    fin_ >> sbw;
    size_t bw = tolower(sbw[0]) == 'w';
    size_t move = agent_->take_action(board_, bw);
    history_.push_back(board_);
    board_.place(bw, move);
    size_t p0 = move % 9;
    fout_ << "= " << char(p0 + (p0 >= 8) + 'A') << char((8 - move / 9) + '1')
          << "\n\n";
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
  // void time_left() { ; }

private:
  /* Regression Commands */
  // void loadsgf() { ; }
  // void reg_genmove() { ; }

private:
  /* Debug Commands */
  void showboard() const { fout_ << "=\n" << board_; }

private:
  /* GoGui Rules */
  void gogui_analyze_command() const {
    ; // TODO
  }
  void gogui_rules_game_id() const { fout_ << "= Nogo\n\n"; }
  void gogui_rules_board() const { fout_ << "=\n" << board_; }
  void gogui_rules_board_size() const { fout_ << "= 9\n\n"; }
  void gogui_rules_legal_moves() const {
    size_t bw = gogui_turns ? 0 : 1;
    fout_ << "=";
    for (size_t i = 0; i < 9; ++i) {
      for (size_t j = 0; j < 9; ++j) {
        Board b(board_);
        if (b.place(bw, (8 - j) * 9 + i)) {
          fout_ << ' ' << char(i + (i >= 8) + 'A') << char(j + '1');
        }
      }
    }
    /*for (size_t i = 0; i < 81; ++i) {
      Board b(board_);
      if (b.place(bw, i)) {
        fout_ << ' ' << char(i % 9 + 'A') << char((8 - i / 9) + '1');
      }
    }*/
    fout_ << "\n\n";
  }
  void gogui_rules_side_to_move() {
    fout_ << "= " << (gogui_turns ? "black" : "white") << "\n\n";
  }
  void gogui_rules_final_result() const {
    fout_ << "= ??? wins by ? points.\n"
          << "  Final score is B ?? and W ??.\n\n";
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
    // GoGui Commands
    // dispatcher_.emplace("gogui-analyze_commands",
    //                    (Helper)&GTPHelper::gogui_analyze_command);
    dispatcher_.emplace("gogui-rules_game_id",
                        (Helper)&GTPHelper::gogui_rules_game_id);
    dispatcher_.emplace("gogui-rules_board",
                        (Helper)&GTPHelper::gogui_rules_board);
    dispatcher_.emplace("gogui-rules_board_size",
                        (Helper)&GTPHelper::gogui_rules_board_size);
    dispatcher_.emplace("gogui-rules_legal_moves",
                        (Helper)&GTPHelper::gogui_rules_legal_moves);
    dispatcher_.emplace("gogui-rules_side_to_move",
                        (Helper)&GTPHelper::gogui_rules_side_to_move);
    dispatcher_.emplace("gogui-rules_final_result",
                        (Helper)&GTPHelper::gogui_rules_final_result);
  }

private:
  constexpr static std::istream &fin_ = std::cin;
  constexpr static std::ostream &fout_ = std::cout;
  std::map<std::string, void (GTPHelper::*)()> dispatcher_;
  Board board_;
  std::vector<Board> history_;
  std::unique_ptr<Agent> agent_;
  bool gogui_turns = true;
};
