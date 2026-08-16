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
3. Treat release tags as stable bases. Review untagged `upstream/master` commits individually; do
   not assume the branch tip is release-ready.
4. Create an integration branch whose starting tree is the chosen official upstream commit. If
   published Playground ancestry must be retained, record it with a history-only merge whose tree
   remains byte-for-byte identical to that upstream commit.
5. Replay the small, ordered Playground overlay on top. If upstream now provides the same feature,
   omit the local implementation. Resolve every genuine overlap in favor of upstream's architecture
   and behavior, then adapt only the still-missing Playground behavior to the new interfaces.
6. Review the final upstream-to-Playground diff so generated files, research probes, and duplicate
   implementations cannot enter `main` accidentally.
7. Build the Windows DLL from a clean directory with the official CMake Linux cross-compilation
   workflow.
8. Test the resulting DLL offline before advancing `main`, tagging, or publishing a Playground
   release.
9. Rebuild `research/tribute-hall` from the validated `main` tree and replay only its research
   commits. Never merge the research branch back into `main` as a shortcut.

## Upstream-first invariant

The selected official commit is the source of truth, not merely another side of a conventional
merge. Playground code must remain a removable overlay. A local implementation is deleted as soon
as upstream supplies an equivalent, even if the local version landed first or has more history.
Release tags and the `backup/pre-upstream-*` branches preserve old states; they are not reasons to
carry obsolete implementations into the next build.

## Cache-format invariant

Every structural change to `build_data.bin` must produce a format number greater than every format
that has existed on either side of an upstream merge. Do not resolve concurrent version bumps by
choosing one side's number. The post-`0.2.1` vendor domain changed the header from 21 to 25 32-bit
count fields, while the historical Playground cache had already reached format 34. Replaying the
Playground commits left the structurally different merged format at 34 as well, so an existing
format-34 cache was parsed with the wrong header and failed during State initialization. Format 35
is the first merged format and makes those older files stale, allowing Sunrise to rebuild them
through its normal cache lifecycle.

Never force-push `main` or erase the historical branch. Experimental probes should remain isolated
until they are understood, reproducible, and suitable for review.
