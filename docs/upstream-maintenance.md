# Upstream maintenance

Sunrise Playground develops directly on the official Sunrise history. The Linux fork remains a
reference for the small Wine/Proton compatibility layer, but it is no longer the base of `main`.

## Remote layout

| Remote | Repository | Purpose |
| --- | --- | --- |
| `origin` | `Excellence308/Sunrise-Playground` | Published Playground branches and releases |
| `upstream` | `stanuwu/Sunrise` | Official source of truth and daily changes |
| `linux-reference` | `TotalTaxAmount/Sunrise` | Reference for Linux-specific work not yet upstream |

## Branch boundaries

| Branch | Contents | Release eligible |
| --- | --- | --- |
| `main` | Official upstream plus reviewed Linux, inventory, settings, and subclass changes | Yes, after runtime testing |
| `research/tribute-hall` | `main` plus incomplete Tribute Hall protocol and entity-spawning probes | No |
| `sunrise-linux-0.2.1` | Historical Linux-based Playground development and checkpoints | Historical only |

## Updating from official Sunrise

1. Close Destiny 2 and any Sunrise launcher processes.
2. Fetch `upstream` and inspect the new commits.
3. Merge `upstream/master` into `main` and resolve conflicts by preserving official behavior first.
4. Build the Windows DLL with the official CMake Linux cross-compilation workflow.
5. Test the resulting DLL offline before tagging or publishing a Playground release.
6. Merge the validated `main` into `research/tribute-hall`; do not merge the research branch back
   into `main` as a shortcut.

Never force-push `main` or erase the historical branch. Experimental probes should remain isolated
until they are understood, reproducible, and suitable for review.
