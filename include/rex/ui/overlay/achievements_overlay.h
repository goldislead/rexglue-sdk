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
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <rex/system/achievement_store.h>
#include <rex/ui/imgui_dialog.h>

namespace rex::ui {

class ImmediateDrawer;
class ImmediateTexture;

class AchievementsOverlayDialog : public ImGuiDialog {
 public:
  using AchievementGetter =
      std::function<const std::vector<rex::system::AchievementInfo>&()>;
  using UnlockChecker = std::function<bool(uint32_t)>;

  // icons_dir: directory holding "<image_id>.png" files dumped by
  // `rexglue init achievements`. May be empty / nonexistent — icons are
  // simply omitted in that case.
  AchievementsOverlayDialog(ImGuiDrawer* imgui_drawer, ImmediateDrawer* immediate_drawer,
                            std::filesystem::path icons_dir, AchievementGetter getter,
                            UnlockChecker checker);
  ~AchievementsOverlayDialog();

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  // Returns the cached texture for image_id, lazily loading+decoding the PNG
  // from icons_dir on first request. Returns nullptr if unavailable.
  ImmediateTexture* GetIcon(uint32_t image_id);

  ImmediateDrawer* immediate_drawer_ = nullptr;
  std::filesystem::path icons_dir_;
  AchievementGetter achievements_getter_;
  UnlockChecker unlock_checker_;

  // image_id -> texture (nullptr entry = tried and failed; don't retry).
  std::unordered_map<uint32_t, std::unique_ptr<ImmediateTexture>> icon_cache_;
};

}  // namespace rex::ui
