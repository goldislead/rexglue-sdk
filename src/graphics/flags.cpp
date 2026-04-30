/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/graphics/flags.h>
#include <rex/logging.h>
#include <rex/ui/renderdoc_api.h>

REXCVAR_DEFINE_BOOL(gpu_allow_invalid_fetch_constants, true, "GPU",
                    "Allow invalid fetch constants");
REXCVAR_DEFINE_BOOL(native_2x_msaa, true, "GPU", "Enable native 2x MSAA");
REXCVAR_DEFINE_BOOL(depth_float24_round, false, "GPU", "Round float24 depth values");
REXCVAR_DEFINE_BOOL(depth_float24_convert_in_pixel_shader, false, "GPU",
                    "Convert float24 depth in pixel shader");
REXCVAR_DEFINE_BOOL(depth_transfer_not_equal_test, true, "GPU",
                    "Use not-equal test for depth transfer");
REXCVAR_DEFINE_BOOL(gamma_render_target_as_unorm16, true, "GPU",
                    "Use R16G16B16A16_UNORM for gamma render targets (more accurate than sRGB)")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);
REXCVAR_DEFINE_STRING(dump_shaders, "", "GPU", "Path to dump shaders to");
REXCVAR_DEFINE_BOOL(use_fuzzy_alpha_epsilon, false, "GPU",
                    "Use approximate compare for alpha test values to prevent "
                    "flickering on NVIDIA graphics cards");
REXCVAR_DEFINE_BOOL(gpu_debug_markers, false, "GPU",
                    "Insert debug markers into GPU command streams for tools "
                    "like PIX and RenderDoc. Automatically enabled when "
                    "RenderDoc is detected.");

REXCVAR_DEFINE_STRING(spirv_version_override, "1.0", "GPU/Vulkan",
                      "Override the SPIR-V version used in Vulkan shader translation.\n"
                      "Use: 1.0, 1.3, 1.4, 1.5, auto")
    .allowed({"1.0", "1.3", "1.4", "1.5", "auto"})
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_BOOL(spirv_disable_rounding_mode_rte, false, "GPU/Vulkan",
                    "Disable RoundingModeRTE in SPIR-V shaders for tools that "
                    "do not support the capability")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_STRING(occlusion_query, "fast", "GPU",
                      "Occlusion query mode.\n"
                      " fake: Always write fake sample counts (safe default)\n"
                      " fast: Real GPU queries with speculative cached writes (default)\n"
                      " strict: Real GPU queries, waits for writeback (may stall)")
    .allowed({"fake", "fast", "strict"})
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_BOOL(occlusion_query_log, false, "GPU",
                    "Log ZPD occlusion query events for debugging")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_INT32(occlusion_query_fake_lower_threshold, 0, "GPU",
                     "When >= 0, oscillate fake sample counts between this lower "
                     "bound and occlusion_query_fake_upper_threshold. When < 0, "
                     "disable writing fake results in the fallback path.")
    .range(-1, 100000)
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_INT32(occlusion_query_fake_upper_threshold, 1000, "GPU",
                     "Upper bound for oscillating fake occlusion sample counts")
    .range(0, 100000)
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_DOUBLE(occlusion_query_sample_count_saturation, 1.0, "GPU",
                      "Saturate real occlusion sample counts to compress high values. "
                      "1.0 = no saturation, 0.0 = clamp everything to 1.")
    .range(0.0, 1.0)
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

bool IsGpuDebugMarkersEnabled() {
  static bool cached = false;
  static bool result = false;
  if (!cached) {
    cached = true;
    if (REXCVAR_GET(gpu_debug_markers)) {
      result = true;
      REXLOG_INFO("GPU debug markers enabled via CVar");
    } else {
      auto renderdoc_api = rex::ui::RenderDocAPI::CreateIfConnected();
      if (renderdoc_api) {
        result = true;
        REXLOG_INFO("GPU debug markers auto-enabled (RenderDoc detected)");
      }
    }
  }
  return result;
}
