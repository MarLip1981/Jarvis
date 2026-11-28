# Jarvis T058 — global RAM audit and optimization plan

Base: T057i commit `d04799e00de0dfbc32b92341c46c96792f564ec9`.
Stable `main` remains `ede43d08901dd2be7f8c18373cc2bb32b74a25ed` and is not modified.

## Current inventory (T058 candidate)
- The candidate switches the test package back to the Arduino framework (matching stable main) to isolate the ESP-IDF runtime/framework cost. This is a measurement candidate, not an optimization claim. T058 preserves the modular packages and local radio-cover feature.
- 46 Home Assistant platform receivers in `packages/entities.yaml`.
- 39 globals in `packages/logic.yaml`.
- 5 periodic interval sections in `packages/runtime.yaml`.
- 13 font definitions in `packages/hardware.yaml`.
- 152 unique LVGL IDs across the UI package files (IDs/reference regex audit; includes non-widget IDs and requires YAML-level confirmation).
- Cover pipeline is proven on-device: local HA URL, JPEG 120x120, static RGB565 buffer (28,800 bytes), online_image download/decode. Do not change the cover pipeline/buffer without separate evidence and explicit test.

## Findings
0. The user's on-device T057/T056 results on ESP-IDF were: heap before cover 11,948 B, largest block 3,584 B; after successful cover heap 5,144 B, largest block 3,072 B; minimum heap 908 B. User reports memory is worse than stable main. T058 restores Arduino as a controlled comparison before further RAM work.
1. The T056/T057 changes primarily reduce update frequency and some text allocation, not the footprint of ESPHome HA receiver components. Heap on the user's test was 11,948 B before cover, largest block 3,584 B; after successful cover 5,144 B, largest block 3,072 B; minimum heap 908 B. This is not a successful RAM optimization against stable main.
2. There are seven HA light raw text receivers and separate brightness receivers for four lights; two HA switches also command lights. The state manager can unify the UI-facing state and avoid repeated conversions/text creation, but cannot remove the underlying HA receiver objects merely by introducing globals.
3. Gate integration attributes are received through multiple ESPHome HA text_sensor components (gate_status, direction, entity IDs, lock/lighting states, button events). A single ESPHome receiver cannot subscribe to multiple arbitrary HA attributes. Consolidation into one receiver requires an upstream combined HA entity/attribute or a custom integration change, which is not assumed in this repo-only pass.
4. Radio station/title callbacks each rebuild the dashboard string; playback callback rebuilds it again. This is a clear central-state and one-render-per-change candidate. Keep radio-cover logic unchanged.
5. The UI contains many LVGL objects across pages. Need confirm object counts by YAML structure (not regex count) and identify objects/page elements that can be removed or made conditional without changing UX. Do not remove visible functionality, fonts, touch handlers, or gate animations without a behavior test.
6. The test branch uses ESP-IDF, whereas stable main is Arduino. Framework/runtime differences may account for memory changes. Treat the framework as a measured variable; don't assume the ESP-IDF branch is better. Preserve cover support and test branch as current rollback.

## Implemented on T058 (source changes only; not compiled yet)
- Restored Arduino framework in the test branch to match the stable framework for comparison. T058 still retains the split package structure and the tested local radio-cover feature. This is a controlled candidate; it is not yet a measured RAM win.
- Added a packed 16-bit light-state manager: 2 bits per each of the 7 HA lights (unavailable / off / on). Receiver callbacks update only the packed state, then dashboard/selected lamp UI reads the manager rather than repeatedly comparing/copying HA strings.
- Replaced selected-lamp status/icon/toggle rendering with one LVGL lambda using static strings, reducing repeated string-selection lambdas and dynamic string returns. Existing brightness-slider updates and SOS feature are preserved.
- Centralized radio station/title/playback UI rendering into one script with fixed buffers; radio cover path and its 120x120 RGB565 buffer are unchanged.
- Added compact boiler-state manager and static label rendering for heating/standby/unavailable.
- Removed the previous repeated temporary-string composition for dashboard radio text and selected-lamp UI. This aims to reduce allocation churn/fragmentation; receiver component objects and their stored HA text states remain.

## Intended implementation phases
- Phase 0: establish measurements from stable main and current T057i under the same boot/page sequence. Record free heap, minimum heap, largest block and cover success. If no comparable stable-main logs, ask for them; don't invent a baseline.
- Phase 1: extend the compact state manager to gate status/position and review temperature caches. The light, radio, boiler states are now centralized; door already uses a state-sync script and timer globals. Gate attributes remain separate receivers unless an upstream aggregate entity is designed.
- Phase 2: centralize dashboard rendering and page-specific state refresh. Static buffers with lifetime guarantee; do not make LVGL label text point at temporary buffers.
- Phase 2: consolidate duplicated dashboard rendering (especially radio and lamps) into one event-driven renderer; static buffers with lifetime guarantee; do not make LVGL label text point at temporary buffers.
- Phase 3: inventory LVGL object and font costs, remove only demonstrably redundant objects/glyph/font definitions, preserving Polish glyphs and all visual controls.
- Phase 4: inspect receiver count reduction opportunities. Any aggregation requiring HA templates/integration changes must be separately scoped and documented; don't claim globals alone reduce receiver component RAM.
- Phase 5: static review, compile on user's ESPHome, then one authorized device test. Keep main unchanged; no OTA action without explicit approval.

## Acceptance criteria
- All features/pages/controls retain behavior, including door initial state, gate, touch, radio playback and cover retrieval.
- The cover download/decode path and 120x120 RGB565 buffer are unchanged.
- Under a repeatable comparison, new version has demonstrably more free heap and a larger contiguous block than baseline; stability and no regression must be confirmed on-device.
- If the candidate doesn't improve both memory metrics, do not call it successful or promote it.
