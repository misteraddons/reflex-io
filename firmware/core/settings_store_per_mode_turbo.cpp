#include "../product_config.h"
#include "button_remap.h"
#include "remap_pair_store.h"
#include "remap_pairs.h"
#include "../output/output_runtime_state.h"
#include "settings_store_per_mode_internal.h"
#include "turbo.h"

namespace {

PerModeSettingsRecord loadSettingsOrDefault(DeviceEnum mode, bool* found = nullptr) {
  PerModeSettingsRecord settings{};
  const bool hasSettings = readPerModeSettings(mode, settings);
  if (found != nullptr) {
    *found = hasSettings;
  }
  return settings;
}

}  // namespace

bool sanitizeTurboRatesForMode(DeviceEnum mode, uint8_t* turbo_rates) {
  bool allowed[TURBO_BTN_COUNT] = {false};
  const TurboButtonConfig& config = getTurboButtonConfig(getTurboInputModeForDeviceMode(mode));
  for (uint8_t i = 0; i < config.count; ++i) {
    if (config.indices[i] < TURBO_BTN_COUNT) {
      allowed[config.indices[i]] = true;
    }
  }

  bool changed = false;
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    if (turbo_rates[i] >= TURBO_RATE_LAST) {
      turbo_rates[i] = TURBO_OFF;
      changed = true;
    }
    if (!allowed[i] && turbo_rates[i] != TURBO_OFF) {
      turbo_rates[i] = TURBO_OFF;
      changed = true;
    }
  }
  return changed;
}

void selectPerModeRemapSlot(DeviceEnum mode) {
  g_eeprom_remap_index = getPerModeRemapAddress(mode);
}

void saveTurboSettingsForMode(DeviceEnum mode, const uint8_t* rates) {
  if (!isConcreteModeForPerSettings(mode)) {
    return;
  }

  PerModeSettingsRecord settings = loadSettingsOrDefault(mode);
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    settings.turbo_rates[i] = (rates[i] < TURBO_RATE_LAST) ? rates[i] : (uint8_t)TURBO_OFF;
  }
  sanitizePerModeSettings(mode, settings);
  writePerModeSettings(mode, settings);
}

void loadStoredTurboRatesForMode(DeviceEnum mode, uint8_t* turbo_rates) {
  PerModeSettingsRecord settings = loadSettingsOrDefault(mode);
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    turbo_rates[i] = settings.turbo_rates[i];
  }
  sanitizeTurboRatesForMode(mode, turbo_rates);
}

bool loadTurboRatesForMode(DeviceEnum mode, uint8_t* turbo_rates) {
  bool found = false;
  PerModeSettingsRecord settings = loadSettingsOrDefault(mode, &found);

  bool changed = !found;
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    turbo_rates[i] = settings.turbo_rates[i];
  }
  if (sanitizeTurboRatesForMode(mode, turbo_rates)) {
    changed = true;
  }
  if (changed && isConcreteModeForPerSettings(mode)) {
    for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
      settings.turbo_rates[i] = turbo_rates[i];
    }
    sanitizePerModeSettings(mode, settings);
    writePerModeSettings(mode, settings);
  }
  return changed;
}

bool loadRemapsForMode(DeviceEnum mode) {
  bool found = false;
  PerModeSettingsRecord settings = loadSettingsOrDefault(mode, &found);

  bool changed = !found;
  // The record holds the 19 button entries; its directions stay themselves.
  uint8_t legacy[REMAP_MAX_BUTTONS];
  remapTableFromRecord(legacy, settings.remaps);
  if (sanitizeRemapValuesForMode(mode, legacy)) {
    changed = true;
  }
  bool legacyIsIdentity = true;
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; ++i) {
    if (legacy[i] != i) legacyIsIdentity = false;
  }

  // Remaps belong to the input+output pair. A non-identity record copy is
  // newer than any pair (every pair save retires it): pre-pair firmware's
  // per-input set, or a direct record write. It applies, and once the output
  // is resolved it moves into the output in use and the copy is retired.
  const outputMode_t output = remapPairOutputFor(configuredOutputMode);
  const bool concrete = ADAPT_REMAPS_PER_OUTPUT && isConcreteModeForPerSettings(mode);
  if (!legacyIsIdentity || !concrete || !remapPairRead(mode, output, active_remaps)) {
    for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; ++i) {
      active_remaps[i] = legacy[i];
    }
  } else {
    sanitizeRemapValuesForMode(mode, active_remaps);
  }
#ifdef ENABLE_INPUT_DREAMCAST
  (void)applyDreamcastTwinStickDefaults(mode, output, active_remaps);
#endif
  const bool moveLegacy = concrete && !legacyIsIdentity &&
                          remapPairOutputResolved(configuredOutputMode) &&
                          remapPairWrite(mode, output, legacy);
  for (uint8_t i = 0; i < REMAP_RECORD_BUTTONS; ++i) {
    settings.remaps[i] = moveLegacy ? i : legacy[i];
  }
  changed = changed || moveLegacy;
  noteActiveRemapPair(mode, output);

  if (changed && isConcreteModeForPerSettings(mode)) {
    sanitizePerModeSettings(mode, settings);
    writePerModeSettings(mode, settings);
  }
  return changed;
}
