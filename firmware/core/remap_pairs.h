#pragma once

#include <stdint.h>

#include "device_mode.h"
#include "remap_table.h"
#include "../output/output_mode.h"

// Products with a single fixed output (USB2DB15, USB2JVS) have nothing to
// pair with and keep remaps in the per-mode record.
#if defined(ADAPT_OUTPUT_USB_DEVICE)
#define ADAPT_REMAPS_PER_OUTPUT 1
#else
#define ADAPT_REMAPS_PER_OUTPUT 0
#endif

// Button remaps belong to an input+output pair (see remap_pair_store.h).
// Firmware before 2026-09-26 kept one set per input in PerModeSettingsRecord;
// that copy is now "legacy": it is used until the input first loads with a
// resolved output, then moves to that pair and the record copy becomes the
// identity map.

// The output a remap belongs to: the selected output after the runtime's own
// fallbacks. For Auto it is the output Auto resolved to (the probe output
// while still probing); a MiSTer peripheral promotion under Auto stays MiSTer.
outputMode_t remapPairOutputFor(outputMode_t selected);
// False while Auto is still probing, so legacy remaps are not moved yet.
bool remapPairOutputResolved(outputMode_t selected);

// Values outside the remap range become identity (N64 assignments and
// USB2DB15 combined bindings are kept). True when anything changed.
bool sanitizeRemapValuesForMode(DeviceEnum mode, uint8_t* remaps);
bool applyDreamcastTwinStickDefaults(DeviceEnum input, outputMode_t selected,
                                     uint8_t* remaps);

// The input's unmoved legacy remaps if any (they are newer than any pair),
// else the pair's stored remaps, else identity. Never writes.
// The tables are the full remap width; the 19-entry per-mode record goes
// through remapTableFromRecord()/remapTableToRecord() instead.
void readRemapsForPair(DeviceEnum input, outputMode_t selected, uint8_t (&remaps)[REMAP_MAX_BUTTONS]);
// Stores the pair (identity removes it) and retires the input's legacy copy.
// False when the pair store is full.
// True when writeRemapsForPair() would find room: the pair store is shared by
// every input and output, and it can fill.
bool remapsFitForPair(DeviceEnum input, outputMode_t selected,
                      const uint8_t (&remaps)[REMAP_MAX_BUTTONS]);
bool writeRemapsForPair(DeviceEnum input, outputMode_t selected,
                        const uint8_t (&remaps)[REMAP_MAX_BUTTONS]);

// Records which pair active_remaps holds, for refreshRemapsForOutputChange().
void noteActiveRemapPair(DeviceEnum input, outputMode_t output);
// True when the output in use no longer matches the loaded remaps.
bool activeRemapPairIsStale(DeviceEnum input);
