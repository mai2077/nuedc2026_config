# Code Architecture Documentation Plan

## Goal

Create a current, source-backed Chinese Markdown reference for every file and
public interface under `bsp/`, `control/`, and `app/`.

## Phases

- [complete] Inventory files, headers, public types, macros, and functions.
- [complete] Read implementations and record runtime relationships/status.
- [complete] Write the architecture and API reference document.
- [complete] Verify coverage against the source tree and public headers.

## Constraints

- Do not modify firmware source, `empty.syscfg`, or generated `Debug/` files.
- Describe current runtime behavior, including disabled Bluetooth and turn paths.
- Keep preserved diagnostic/test modules distinct from the active main flow.

## Errors Encountered

| Error | Attempt | Resolution |
| --- | --- | --- |
| PowerShell did not expand `bsp/*.h` passed directly to `rg` | 1 | Search directories with `-g "*.h"` instead. |
