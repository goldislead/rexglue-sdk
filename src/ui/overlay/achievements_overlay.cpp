/**
 * @file        ui/overlay/achievements_overlay.cpp
 *
 * @brief       Achievements overlay implementation. See achievements_overlay.h for details.
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#include <rex/ui/overlay/achievements_overlay.h>
#include <imgui.h>

namespace rex::ui {

AchievementsOverlayDialog::AchievementsOverlayDialog(ImGuiDrawer* imgui_drawer,
                                                     AchievementGetter getter,
                                                     UnlockChecker checker)
    : ImGuiDialog(imgui_drawer),
      achievements_getter_(std::move(getter)),
      unlock_checker_(std::move(checker)) {}

AchievementsOverlayDialog::~AchievementsOverlayDialog() {}

void AchievementsOverlayDialog::OnDraw(ImGuiIO& io) {
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, 50.0f), ImGuiCond_FirstUseEver,
                          ImVec2(0.5f, 0.0f));
  ImGui::SetNextWindowSize(ImVec2(520.0f, 420.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(0.85f);

  if (ImGui::Begin("Achievements##overlay", nullptr, ImGuiWindowFlags_NoCollapse)) {
    ImGui::TextDisabled("Session only \xe2\x80\x94 resets on restart");
    ImGui::Separator();

    const auto& achievements = achievements_getter_();

    int unlocked_count = 0;
    int total_gs = 0;
    int earned_gs = 0;
    for (const auto& a : achievements) {
      total_gs += static_cast<int>(a.gamerscore);
      if (unlock_checker_(a.id)) {
        ++unlocked_count;
        earned_gs += static_cast<int>(a.gamerscore);
      }
    }
    ImGui::Text("%d / %d unlocked   (%dG / %dG)", unlocked_count,
                static_cast<int>(achievements.size()), earned_gs, total_gs);
    ImGui::Separator();

    ImGui::BeginChild("##achlist", ImVec2(0.0f, 0.0f), false);
    for (const auto& a : achievements) {
      bool is_unlocked = unlock_checker_(a.id);
      if (is_unlocked) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.3f, 1.0f));
        ImGui::Text("[%dG] %s", static_cast<int>(a.gamerscore), a.label.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", a.description.c_str());
      } else {
        ImGui::TextDisabled("[%dG] %s  \xe2\x80\x94  %s", static_cast<int>(a.gamerscore),
                            a.label.c_str(), a.unachieved_description.c_str());
      }
    }
    ImGui::EndChild();
  }
  ImGui::End();
}

}  // namespace rex::ui
