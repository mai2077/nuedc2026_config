# Progress

## 2026-07-23

- Started current-source inventory for `bsp/`, `control/`, and `app/`.
- Confirmed old root planning files belong to the earlier ICM task and created
  an isolated plan directory for this documentation task.
- Recorded the first failed `rg` glob attempt and changed to directory filters.
- Completed the source-backed inventory of all BSP, control, and app headers
  and implementations, including active/disabled runtime paths and ISR roles.
- Added `项目代码结构与接口说明.md` with the active call chain, every source
  module, public interface, key type/macro, algorithm, polarity, timing, and
  retained-but-disabled path.
- Verification passed for 52 source/header files and 122 public API tokens;
  UTF-8 decoding, code-fence pairing, trailing whitespace, and zero firmware
  source/config diffs were also checked.
