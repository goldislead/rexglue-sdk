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

namespace {
// Palette — kept ASCII-only; the bundled overlay font has no em dash / check glyphs.
constexpr ImVec4 kUnlockedTitle{0.45f, 1.00f, 0.55f, 1.00f};  // bright green
constexpr ImVec4 kUnlockedDesc{0.70f, 0.85f, 0.72f, 1.00f};   // soft green
constexpr ImVec4 kLockedTitle{0.78f, 0.80f, 0.84f, 1.00f};    // light grey
constexpr ImVec4 kLockedDesc{0.50f, 0.52f, 0.56f, 1.00f};     // dim grey
constexpr ImVec4 kBadgeGS{1.00f, 0.82f, 0.30f, 1.00f};        // gamerscore gold
constexpr ImVec4 kRowUnlockedBg{0.16f, 0.30f, 0.18f, 0.55f};  // green tint
constexpr ImVec4 kHeaderText{0.60f, 0.85f, 1.00f, 1.00f};     // accent blue
}  // namespace

void AchievementsOverlayDialog::OnDraw(ImGuiIO& io) {
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, 40.0f), ImGuiCond_FirstUseEver,
                          ImVec2(0.5f, 0.0f));
  ImGui::SetNextWindowSize(ImVec2(620.0f, 540.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(0.92f);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 12.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));

  if (ImGui::Begin("Achievements##overlay", nullptr, ImGuiWindowFlags_NoCollapse)) {
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
    const int total_count = static_cast<int>(achievements.size());

    // ---- Header: summary line + progress bar -------------------------------
    ImGui::PushStyleColor(ImGuiCol_Text, kHeaderText);
    ImGui::Text("%d / %d unlocked", unlocked_count, total_count);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextColored(kBadgeGS, "%dG / %dG", earned_gs, total_gs);

    float frac = total_count > 0 ? static_cast<float>(unlocked_count) / total_count : 0.0f;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.30f, 0.80f, 0.40f, 1.0f));
    ImGui::ProgressBar(frac, ImVec2(-1.0f, 6.0f), "");
    ImGui::PopStyleColor();

    ImGui::TextDisabled("Session only - resets on restart");
    ImGui::Separator();
    ImGui::Spacing();

    // ---- List --------------------------------------------------------------
    ImGui::BeginChild("##achlist", ImVec2(0.0f, 0.0f), false);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    for (const auto& a : achievements) {
      const bool is_unlocked = unlock_checker_(a.id);
      const std::string& desc =
          is_unlocked ? a.description : a.unachieved_description;

      ImGui::PushID(static_cast<int>(a.id));

      // Split the draw list so the row band (channel 0) renders behind the
      // text (channel 1); we paint the rect after measuring, then merge.
      draw_list->ChannelsSplit(2);
      draw_list->ChannelsSetCurrent(1);

      const float row_start_y = ImGui::GetCursorScreenPos().y;
      const float pad = 4.0f;
      ImGui::Dummy(ImVec2(0.0f, pad * 0.5f));

      // Title line: state marker + gamerscore badge + label.
      const char* marker = is_unlocked ? "[*]" : "[ ]";
      ImGui::TextColored(is_unlocked ? kUnlockedTitle : kLockedTitle, "%s", marker);
      ImGui::SameLine();
      ImGui::TextColored(kBadgeGS, "%dG", static_cast<int>(a.gamerscore));
      ImGui::SameLine();
      ImGui::TextColored(is_unlocked ? kUnlockedTitle : kLockedTitle, "%s", a.label.c_str());

      // Description line, indented under the label.
      ImGui::Indent(28.0f);
      ImGui::PushStyleColor(ImGuiCol_Text, is_unlocked ? kUnlockedDesc : kLockedDesc);
      ImGui::TextWrapped("%s", desc.c_str());
      ImGui::PopStyleColor();
      ImGui::Unindent(28.0f);

      ImGui::Dummy(ImVec2(0.0f, pad * 0.5f));
      const float row_end_y = ImGui::GetCursorScreenPos().y;

      // Paint the band behind unlocked rows on the lower channel.
      if (is_unlocked) {
        draw_list->ChannelsSetCurrent(0);
        const float x0 = ImGui::GetWindowPos().x + 2.0f;
        const float x1 = x0 + ImGui::GetWindowSize().x - 4.0f;
        draw_list->AddRectFilled(ImVec2(x0, row_start_y), ImVec2(x1, row_end_y),
                                 ImGui::GetColorU32(kRowUnlockedBg), 3.0f);
      }
      draw_list->ChannelsMerge();

      ImGui::Separator();
      ImGui::PopID();
    }
    ImGui::EndChild();
  }
  ImGui::End();

  ImGui::PopStyleVar(2);
}

}  // namespace rex::ui
