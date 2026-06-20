# Achievements

This document is both a reference and a how-to for the achievement subsystem. It
covers the architecture, the authoring workflow, and the three extension points
recomp developers care about most:

- **Replacing metadata** — edit the text/icons/score of extracted achievements.
- **Custom achievements** — define your own achievements and unlock them from a
  hook on any recompiled function.
- **Custom UI** — replace the built-in overlay/toast, or build a fully custom
  ImGui window driven by your title.

The feature gives recomp developers authored control over achievement metadata
(extracted from the original title at build time) and surfaces it at runtime as
an in-game overlay (F7) and unlock toasts, with live unlock tracking driven by
the guest's own XGI/XAM calls.

## Architecture overview

- **Authoring (CLI):** `rexglue init achievements` extracts achievement metadata
  and icons from a title's XEX/XDBF into a `metadata/` directory. Recomp
  projects can ship that directory as loose files during development, or embed
  the icon PNGs into the executable for release builds.
- **Core (rexsystem):** an `AchievementManager` owns the loaded metadata, unlock
  state, persistence, and notification callbacks. `KernelState` loads the data
  and delegates to it; `Runtime` exposes it to the app layer.
- **Guest hooks (xam):** the XGI/XAM kernel paths report unlocks to the manager
  and report current state back to the game.
- **UI (rexui):** an overlay (toggled with F7) lists achievements with per-state
  descriptions and icons, and a toast renderer shows unlock notifications. Icons
  are decoded from PNG via `stb_image` and cached.
- **App wiring (ReXApp):** the base app constructs the overlay/toast, registers
  the keybind, and bridges the manager's notification callback to the toast.

## Where data lives

At module load, `KernelState::LoadAchievementsData()` runs:

1. Reads the title's embedded XDBF achievement table (labels, descriptions,
   gamerscore, image ids) as the baseline.
2. Merges `achievements.toml` if found, **overriding** any XDBF entry with the
   same `id`. This is how authored edits win over extracted defaults.
3. Sets the unlock save path to
   `<user_data_root>/achievements/<TITLEID>.toml` and restores persisted
   unlocks.

`achievements.toml` and loose icon files are resolved by
`Runtime::FindMetadataPath()`:

- If the `metadata_root` cvar (`--metadata_root`) is set, **only**
  `<metadata_root>/<relative>` is used.
- Otherwise, in order:
  1. `<game_data_root>/metadata/<relative>`
  2. `<game_data_root>/../metadata/<relative>`
  3. `<game_data_root>/<relative>`

So the default layout a recomp project ships is:

```
<game_data_root>/
  metadata/
    achievements.toml
    icons/
      <image_id>.png   # one per achievement
      title.png        # the game's own icon (dumped from the XDBF)
```

Icon PNGs can also be embedded into the host executable at build time. Loose
files still win, so `metadata_root` remains useful for development overrides and
mods, but release builds can display custom icons without sidecar PNGs.

## Authoring & replacing metadata

Generate the starting files from the original title:

```
rexglue init --project-name <name> --xex-path <path/to/default.xex> achievements <path/to/default.xex> <path/to/output/>
```

For example:

```
rexglue init --project-name rewos --xex-path assets/default.xex achievements assets/default.xex assets/metadata
```

The XEX path is intentionally present twice: the parent `init` command requires
`--xex-path`, and the `achievements` subcommand also takes the XEX path it should
extract from. The final positional argument is the output directory. For the
default runtime lookup rules, write the output to `metadata/` next to the game
data, or to `<game_data_root>/metadata`.

This writes `achievements.toml` and `icons/<image_id>.png` for every achievement
in the title, plus `icons/title.png` — the game's own icon, dumped from the same
XDBF. Each entry looks like:

```toml
[[achievements]]
id                    = 1
label                 = "First Blood"
description           = "Defeat your first enemy."
unachieved_description = "Defeat an enemy."
gamerscore            = 10
image_id              = 1
icon_path             = "icons/1.png"
flags                 = 0
```

To **replace text**, edit `label`, `description`, or `unachieved_description`.
To **replace an icon**, overwrite the PNG at `icon_path` (path is relative to the
metadata root) or point `icon_path` at a different file. Icons resolve in this
order at render time:

1. loose metadata file at explicit `icon_path`
2. embedded metadata asset at explicit `icon_path`
3. loose metadata file at `icons/<image_id>.png`
4. embedded metadata asset at `icons/<image_id>.png`
5. the title's embedded XDBF image `<image_id>`

### Embedding icon PNGs into the executable

To avoid shipping custom achievement icons as loose files, embed the metadata
icon directory into the host executable:

```cmake
rexglue_embed_metadata(your_target
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/metadata/icons"
    PREFIX "icons")
```

With new or migrated `rexglue.cmake` scaffolding, `rexglue_setup_target()` calls
this automatically when `metadata/icons` exists. Existing projects can add the
call above after `add_executable()` and before or after `rexglue_setup_target()`.

The `PREFIX "icons"` part is important: a file at
`metadata/icons/custom/67.png` becomes the embedded metadata path
`icons/custom/67.png`, which is the same path you put in `AchievementInfo` or
`achievements.toml`:

```cpp
rex::system::RegisterAchievement({
    .id = 0x10067,
    .label = "67",
    .description = "Set every volume slider to 67.",
    .unachieved_description = "Set Master, Music, SFX, and Voice all to 67.",
    .icon_path = "icons/custom/67.png",
    .gamerscore = 67,
});
```

During development, a loose file at `metadata/icons/custom/67.png` overrides the
embedded bytes. For a release build, the PNG can be omitted from the shipped
folder because the executable already contains it.

Fields:

| Field | Meaning |
|---|---|
| `id` | Unique non-zero achievement id (matches the XDBF id for title achievements). |
| `label` | Display name. |
| `description` | Shown once unlocked. |
| `unachieved_description` | Shown while locked. |
| `gamerscore` | Points value. |
| `image_id` | XDBF image id used for the fallback icon. |
| `icon_path` | Icon file relative to the metadata root. |
| `flags` | XDBF achievement flags (passthrough). |

## Custom achievements

Custom (recomp-authored) achievements are achievements that don't exist in the
original title. There are two steps: **define** the achievement, then **unlock**
it from wherever the trigger lives.

Use the convenience facade in `<rex/system/achievements.h>`. Every call is
null-safe (no-op before the runtime is live) and thread-safe:

```cpp
namespace rex::system {
  bool RegisterAchievement(AchievementInfo info);
  bool UnlockAchievement(uint32_t id, bool show_toast = true);
  bool IsAchievementUnlocked(uint32_t id);
}
```

### 1. Define it

Register custom achievements once, after the image loads. `OnPostLoadXexImage()`
is the intended hook.

```cpp
#include <rex/system/achievements.h>

void MyApp::OnPostLoadXexImage() {
  rex::system::RegisterAchievement({
      .id = 0x10001,                       // see ID conventions below
      .label = "Speedrunner",
      .description = "Finished the first level in under a minute.",
      .unachieved_description = "Finish the first level quickly.",
      .icon_path = "icons/custom/speedrunner.png",
      .gamerscore = 25,
  });
}
```

> **ID conventions.** Title achievements use the game's id space (typically
> `1..N`). Pick custom ids in a high range (e.g. `0x10000+`) so they never
> collide. Registering an id that already exists overrides that entry instead of
> adding a new one.

Authored custom achievements can also live in `achievements.toml` alongside
extracted ones — just add `[[achievements]]` blocks with high ids. Use
`RegisterAchievement()` when the definition needs to be computed at runtime.

### Creating a custom achievement

The `rewos` commit `c529752ac1dab36ab80bf5b4c68783066fec35ea` is a minimal
example of adding a recomp-authored achievement. It added a "67" achievement
that unlocks when the Master, Music, SFX, and Voice sliders all round to 67.

Use the same pattern:

1. Pick a stable high-range id and keep it in one shared place:

   ```cpp
   // Custom achievement: unlocks when every volume slider reads 67.
   inline constexpr std::uint32_t kVolume67AchievementId = 0x10067;
   ```

2. Put the PNG at `metadata/icons/custom/67.png`, then register the
   achievement once after the XEX image loads:

   ```cpp
   #include <rex/system/achievements.h>

   void RewosApp::OnPostLoadXexImage() {
     rex::system::RegisterAchievement({
         .id = wos::audio::kVolume67AchievementId,
         .label = "67",
         .description = "Set every volume slider to 67.",
         .unachieved_description =
             "Set Master, Music, SFX, and Voice all to 67.",
         .icon_path = "icons/custom/67.png",
         .gamerscore = 67,
     });
   }
   ```

3. Call `UnlockAchievement()` from the code path that observes the trigger:

   ```cpp
   void MaybeUnlockVolume67() {
     for (VolumeChannel channel : {VolumeChannel::kMaster, VolumeChannel::kMusic,
                                   VolumeChannel::kSfx, VolumeChannel::kVoice}) {
       const std::optional<float> percent = ReadVolumePercent(channel);
       if (!percent || std::lround(*percent) != 67) {
         return;
       }
     }
     rex::system::UnlockAchievement(kVolume67AchievementId, /*show_toast=*/true);
   }
   ```

4. Invoke that check only after the relevant state changes. In `rewos`, the
   volume setter calls `MaybeUnlockVolume67()` only when a slider update
   succeeds. Repeated checks are fine because `UnlockAchievement()` is a no-op
   after the first successful unlock.

### 2. Unlock it from a hook

Hook the recompiled function that represents the trigger (see `rex/hook.h` for
the hook macros) and call the API. The unlock is persisted automatically, and
the toast shows because the notification callback is already wired in
`ReXApp::LaunchModule()`.

```cpp
#include <rex/hook.h>
#include <rex/system/achievements.h>

// Fires whenever the recompiled function sub_82012345 runs.
void OnLevelComplete() {
  rex::system::UnlockAchievement(0x10001);   // toast shown by default
}
REX_HOOK(sub_82012345, OnLevelComplete)
```

For conditional unlocks, read guest state inside the hook (via the raw
`REX_HOOK_RAW` form for direct `ctx`/`base` access) and call
`UnlockAchievement()` only when your condition holds. Pass `show_toast = false`
to record the unlock silently.

`UnlockAchievement()` returns `true` only on a first-time unlock; repeat calls
are no-ops, so it's safe to call from a hot path.

## Custom UI

There are three levels of UI customization, from least to most involved.

### Replace the F7 overlay

Override `ReXApp::CreateAchievementsOverlay()`. Return your own
`ui::ImGuiDialog`; returning `nullptr` disables the overlay. The F7 keybind and
its toggle lifecycle are handled for you.

```cpp
std::unique_ptr<rex::ui::ImGuiDialog> MyApp::CreateAchievementsOverlay() {
  return std::make_unique<MyAchievementsWindow>(
      imgui_drawer(), immediate_drawer(), runtime(), &achievements());
}
```

### Replace the toast

Override `ReXApp::CreateAchievementNotificationDialog()` to return your own
`ui::AchievementNotificationDialog`. Implement `Push(const AchievementEvent&)`
(must be thread-safe — events can arrive from guest threads). Returning
`nullptr` disables notifications.

### Build a fully custom window

For a bespoke window — e.g. a "Trophies" page opened from a button in your
title — subclass `ui::ImGuiDialog`, query the manager for data, and reuse
`ui::AchievementIconCache` for icons:

```cpp
#include <rex/system/achievement_manager.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/overlay/achievement_icon_cache.h>

class MyAchievementsWindow : public rex::ui::ImGuiDialog {
 public:
  MyAchievementsWindow(rex::ui::ImGuiDrawer* drawer, rex::ui::ImmediateDrawer* id,
                       rex::Runtime* rt, rex::system::AchievementManager* ach)
      : ImGuiDialog(drawer), icons_(id, rt), ach_(ach) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    ImGui::Begin("Trophies");
    for (const auto& a : ach_->ListAchievements()) {
      const bool unlocked = ach_->IsUnlocked(a.id);
      // icons_.GetIcon(a) returns an ImmediateTexture* you can bind for
      // ImGui::Image(); see src/ui/overlay/achievements_overlay.cpp for the
      // exact texture-binding and layout pattern.
      ImGui::TextColored(unlocked ? ImVec4(0, 1, 0, 1) : ImVec4(.6f, .6f, .6f, 1),
                         "%s (%u G)", a.label.c_str(), a.gamerscore);
      ImGui::TextWrapped("%s", (unlocked ? a.description : a.unachieved_description).c_str());
    }
    ImGui::End();
  }

 private:
  rex::ui::AchievementIconCache icons_;
  rex::system::AchievementManager* ach_;
};
```

The built-in `AchievementsOverlayDialog`
(`src/ui/overlay/achievements_overlay.cpp`) is the canonical, copy-able example
for icon rendering, two-line rows, and locked/unlocked styling.

**Opening it from a button in the title.** Two common patterns:

- *Host keybind:* register your own bind in `OnCreateDialogs(drawer)` (the same
  mechanism the F7 overlay uses) to toggle the window.
- *Guest-driven:* set a flag from a hook on the guest's "open menu" function
  (an `std::atomic<bool>`), and show/hide your dialog based on that flag. This
  lets an in-title button open the host ImGui window.

## API reference

### Convenience facade — `<rex/system/achievements.h>`

`rex::system::RegisterAchievement`, `UnlockAchievement`, `IsAchievementUnlocked`.
Null-safe, thread-safe, ideal for hooks. Thin wrappers over the manager below.

### Manager — `kernel_state()->achievements()`

`rex::system::AchievementManager` is the full surface for advanced use:

- Catalog: `RegisterAchievement`, `RegisterAchievements`, `ReplaceAchievements`,
  `LoadMetadataFile`, `ListAchievements`, `FindAchievement`.
- Unlocks: `UnlockAchievement`, `ShowAchievementNotification`, `IsUnlocked`,
  `GetUnlockTime`.
- Callbacks: `RegisterUnlockCallback`, `RegisterNotificationCallback`,
  `UnregisterCallback`. Unlock callbacks fire on every unlock; notification
  callbacks fire when a toast should show.
- Persistence: `SetUnlockSavePath`, `LoadUnlockState`, `SaveUnlockState`.

Reach it from anywhere via the global
`rex::system::kernel_state()->achievements()`, or from a `ReXApp` subclass via
the `achievements()` accessor.

### ReXApp hooks

- `OnPostLoadXexImage()` — register custom achievements here.
- `CreateAchievementsOverlay()` — replace/disable the F7 overlay.
- `CreateAchievementNotificationDialog()` — replace/disable the toast.
- `achievements()` — the live `AchievementManager&`.
- `metadata_root()` — the resolved metadata root path.

## Persistence

Unlock state is stored at `<user_data_root>/achievements/<TITLEID>.toml` as
`[unlocked.<id>] filetime = <ms>` entries, written atomically on each unlock. A
legacy array format is auto-migrated on load. Custom achievements persist the
same way as title achievements.

## Commit series

The feature was assembled on top of `upstream/development` in this order, each
commit building on its own:

1. **feat(cvar): defer unknown config flags until their flag registers** —
   deferred-config so late-registered achievement cvars still pick up TOML
   values; includes a unit test.
2. **feat(system): add achievement metadata store struct** — `AchievementInfo`
   shared by the CLI and the runtime.
3. **feat(cli): add rexglue init achievements XEX-to-TOML extractor** — extracts
   metadata, per-achievement icons, and the game's own icon (`title.png`) from
   XDBF into a `metadata/` directory.
4. **feat(system): add runtime achievement manager** — `AchievementManager`:
   store, unlock timestamps, persistence, notifications.
5. **feat(system): integrate achievement manager into KernelState** — loads
   metadata (TOML with XDBF fallback) and owns the manager.
6. **feat(system): expose achievement manager via Runtime** — threads
   `metadata_root` through `Runtime` and exposes the manager to the app.
7. **feat(xam): track achievement unlocks and report state to the game** — routes
   guest XGI/XAM unlock writes into the manager and reports state back.
8. **feat(ui): add PNG decoding via stb_image** — vendored stb + `image_decode`
   helper for icon textures.
9. **feat(ui): add achievement icon cache** — lazy icon decode/upload as GPU
   textures.
10. **feat(ui): add achievements overlay** — the F7 overlay; makes
    `ImGuiDialog`'s destructor virtual.
11. **feat(ui): add achievement toast notifications** — toast renderer.
12. **feat(ui): wire achievement overlays into ReXApp** — constructs overlay/toast,
    registers the F7 bind in `SetupOverlays()`, bridges the notification
    callback, tears down on destroy.
13. **test(system): add achievement manager unit tests** — manager coverage plus
    a `ReXApp` consumer-compilation test target.
14. **feat(system): add convenience achievement hook API** — null-safe
    `rex::system::{RegisterAchievement,UnlockAchievement,IsAchievementUnlocked}`
    facade for hooks and app code.
15. **docs: document the achievements feature and its commit series** — this file.
