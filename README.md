# ea_input

Portable C++17 controller input library shared by arcade_play and EasyArcade3.

Contains HID parsing, pad mapping and rapid fire, .eamacro parsing/runtime,
Switch Pro initialization/report decoding, and the shared default pad mapping.
Uses only the C++ standard library, without Pico SDK, TinyUSB, GPIO, filesystem,
Flash or UI dependencies. Runtime ownership and buffer layout are unchanged.

```cmake
add_subdirectory(external/ea_input)
target_link_libraries(your_target PRIVATE ea_input::ea_input)
```

Headers live under `include/ea_input`. Both `<ea_input/pad_state.h>` and the
existing `"pad_state.h"` includes are supported. Each product owns USB transport,
port assignment, frame timing, JAMMA output, storage and configuration UI.
The legacy arcade_play Flash format lives in its pad_config_serialization adapter.

Existing portable tests are in `tests/`. Run them explicitly with
`python3 tests/run_host_tests.py`; the default build does not execute tests.

## Provenance

Extracted from arcade_play commit `5efa8ed`, with the ongoing EasyArcade3 changes:
shared default mapping, neutral absent HID axes, configurable descriptor field
limits and bounded array usage expansion. Original notices and MIT license retained.

Repository: https://github.com/shuichitakano/ea_input

Products pin a commit through `external/ea_input` Git submodules. Initialize with
`git submodule update --init --recursive`. Updating the library requires committing
the chosen submodule revision in each product repository.
