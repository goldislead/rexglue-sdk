/**
 * @file        ui/overlay/achievement_toast.cpp
 *
 * @brief       Achievement toast implementation. See achievement_toast.h for details.
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */
#include <rex/ui/overlay/achievement_toast.h>
#include <imgui.h>

namespace rex::ui {

AchievementToastDialog::AchievementToastDialog(ImGuiDrawer* drawer)
    : AchievementNotificationDialog(drawer) {}

AchievementToastDialog::~AchievementToastDialog() {}

void AchievementToastDialog::Push(const rex::system::AchievementEvent& event) {
  std::lock_guard<std::mutex> lock(mutex_);
  queue_.push_back({event, std::chrono::steady_clock::now()});
}

void AchievementToastDialog::OnDraw(ImGuiIO& io) {
  auto now = std::chrono::steady_clock::now();

  {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
      float age = std::chrono::duration<float>(now - queue_.front().arrived).count();
      if (age >= kDisplaySeconds)
        queue_.pop_front();
      else
        break;
    }
    if (queue_.empty())
      return;
  }

  PendingToast toast;
  float age;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    toast = queue_.front();
    age = std::chrono::duration<float>(now - toast.arrived).count();
  }

  float alpha = 1.0f;
  if (age < 0.3f)
    alpha = age / 0.3f;
  else if (age > kDisplaySeconds - 0.5f)
    alpha = (kDisplaySeconds - age) / 0.5f;
  alpha = std::max(0.0f, std::min(1.0f, alpha));

  const float pad = 16.0f;
  const float w = 280.0f;
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - w - pad, io.DisplaySize.y - 80.0f - pad),
                          ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(w, 70.0f), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.88f * alpha);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav |
                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                           ImGuiWindowFlags_NoInputs;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
  if (ImGui::Begin("##ach_toast", nullptr, flags)) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.2f, alpha));
    ImGui::TextUnformatted("Achievement Unlocked");
    ImGui::PopStyleColor();
    ImGui::TextUnformatted(toast.event.achievement.label.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled(" (%dG)", static_cast<int>(toast.event.achievement.gamerscore));
  }
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}

}  // namespace rex::ui
