/**
 * @file        rex/ui/overlay/achievements_overlay.h
 *
 * @brief       ImGui achievements overlay dialog for in-session unlock tracking.
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#pragma once
#include <functional>
#include <vector>

#include <rex/system/achievement_store.h>
#include <rex/ui/imgui_dialog.h>

namespace rex::ui {

class AchievementsOverlayDialog : public ImGuiDialog {
 public:
  using AchievementGetter =
      std::function<const std::vector<rex::system::AchievementInfo>&()>;
  using UnlockChecker = std::function<bool(uint32_t)>;

  AchievementsOverlayDialog(ImGuiDrawer* imgui_drawer, AchievementGetter getter,
                             UnlockChecker checker);
  ~AchievementsOverlayDialog();

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  AchievementGetter achievements_getter_;
  UnlockChecker unlock_checker_;
};

}  // namespace rex::ui
