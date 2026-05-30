/**
 * @file        rex/ui/overlay/achievement_toast.h
 *
 * @brief       Thread-safe achievement unlock toast — bottom-right corner,
 *              auto-dismisses after a few seconds.
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include <string>

#include <rex/ui/imgui_dialog.h>

namespace rex::ui {

class AchievementToastDialog : public ImGuiDialog {
 public:
  explicit AchievementToastDialog(ImGuiDrawer* drawer);
  ~AchievementToastDialog();

  // Thread-safe: safe to call from any thread, including guest threads.
  void Push(std::string label, uint32_t gamerscore);

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  static constexpr float kDisplaySeconds = 4.5f;

  struct PendingToast {
    std::string label;
    uint32_t gamerscore;
    std::chrono::steady_clock::time_point arrived;
  };

  std::mutex mutex_;
  std::deque<PendingToast> queue_;
};

}  // namespace rex::ui
