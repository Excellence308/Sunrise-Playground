# Shadowkeep investment-root table map

This note records what the installed Shadowkeep package set actually exposes through the
investment root. It is a research aid for Sunrise development, not part of the official Sunrise
setup guide and not a claim that every unnamed table has been reverse engineered.

## Evidence rules

| Label | Meaning |
| --- | --- |
| Confirmed | Sunrise source names the slot and consumes it for that purpose. |
| Candidate | Runtime shape makes the table worth inspecting, but its identity is not established. |
| Unknown | Only the structural metadata below is known. |

The capture was made on 2026-08-15 from the locally installed Shadowkeep packages. Each root slot
begins at byte 8 and has a 16-byte stride, as implemented by `tables::slot_tag`. For ordinary
definition tables, the diagnostic resolves the array descriptor at byte 8 with Sunrise's existing
`tables::find_array_at` parser. `array = no` therefore means only that this ordinary descriptor was
not present; it does not mean that the referenced blob is empty.

The exact capture is preserved outside the repository at:

`Sunrise/backups/root-table-samples-20260815-175537/root-table-metadata.log`

## Confirmed table identities

| Slot | Sunrise identity | Table class | Rows | Element class | Evidence |
| ---: | --- | --- | ---: | --- | --- |
| 11 | Investment constants | `0x808075C0` | n/a | n/a | `kInvestmentConstantsSlot` and the constants parser |
| 17 | Inventory buckets | `0x80807C62` | n/a | n/a | `kBucketTableSlot` and the bucket descriptor parser |
| 19 | Collectibles | `0x8080306D` | 5,181 | `0x80803475` | `kCollectibleTableSlot` plus named table and row classes |
| 48 | Inventory items | `0x80807BE4` | 15,424 | `0x80807BE8` | `kItemTableSlot` plus named item-index row class |
| 51 | Plug sets | `0x80802CB6` | 607 | `0x80802DFF` | `kPlugSetTableSlot` and the plug-set parser |
| 68 | Progressions | `0x80807CD9` | 88 | `0x80807CDD` | `kProgressionTableSlot` plus named progression row class |
| 96 | Material requirement sets | `0x80807ACE` | 229 | `0x80807AD4` | `kMaterialRequirementTableSlot` plus named table and row classes |
| 97 | Socket-entry lists | `0x80807A78` | 14 | `0x80807A7E` | `kSocketEntryListTableSlot` plus named row class |

## Current Tribute Hall candidates

These are deliberately broad candidates chosen for one read-only sample, not asserted identities.
Their large row counts make them plausible definition banks that may contain acquired-state,
objective, record, quest, or similar investment data.

| Slot | Table class | Rows | Element class | Status | Next evidence |
| ---: | --- | ---: | --- | --- | --- |
| 104 | `0x80807542` | 12,506 | `0x80807480` | Candidate | Capture the first eight 32-bit words of row 0 |
| 109 | `0x80807C49` | 6,541 | `0x80807C4F` | Candidate | Capture the first eight 32-bit words of row 0 |
| 111 | `0x80807D36` | 11,923 | `0x80807D48` | Candidate | Capture the first eight 32-bit words of row 0 |
| 112 | `0x80807D49` | 21,613 | `0x80807D4F` | Candidate | Capture the first eight 32-bit words of row 0 |
| 113 | `0x80807C80` | 5,867 | `0x80807C8D` | Candidate | Capture the first eight 32-bit words of row 0 |
| 114 | `0x80807C92` | 13,231 | `0x80807C96` | Candidate | Capture the first eight 32-bit words of row 0 |

No candidate will be wired into persistent state merely because its size looks promising. The next
capture should establish whether the leading words behave like definition hashes, tags, indices,
or flags; only then should individual rows be correlated with known definitions.

## Sample 1: slots 104, 109, and 111-114

Captured on 2026-08-15. Exact hash membership was checked against the component JSON exposed by
Bungie's official live manifest endpoint. This is stronger than a name inferred from table size,
but it remains a correlation rather than a Sunrise-confirmed native table identity.

| Slot | First four word pairs or values | Official component correlation | Current conclusion |
| ---: | --- | --- | --- |
| 104 | `5FF40673 5FF40672 7F262546 7F262547 ...` | No match in the focused state-bearing components | Dense hash/value array; identity unknown |
| 109 | `73BD43E0 00000000 00000009 00000000 ...` | No match in the focused state-bearing components | Structured rows; identity unknown |
| 111 | `52A08B00:DE 6416DD91:E6 84F35AFB:E7 087FD9D8:E8` | All four hashes are `DestinyUnlockDefinition` keys | Strong candidate for the historical unlock-definition hash/index map |
| 112 | `904E7433:FFFF0000 904E7430:FFFF0000 ...` | No match in the focused state-bearing components | Hash/value map; identity unknown |
| 113 | `CD167E32:0 860A7780:1 BABE01E7:40 B9EDA137:47` | All four hashes are `DestinyUnlockValueDefinition` keys | Strong candidate for the historical unlock-value hash/index map |
| 114 | `CD167E32:1 860A7780:10001 ECC42A28:FFFF0000 D472DCD4:2` | All four hashes are `DestinyUnlockValueDefinition` keys | Strong candidate for a second unlock-value mapping or encoded selector table |

For slots 111 and 113, the second word behaves like a compact historical index: consecutive hashes
carry consecutive small values. Slot 114 uses the same definition family but different encoded
values, so it must not be treated as another plain index map.

The current Bungie manifest still contains the Tribute Hall presentation nodes and 19 record hashes,
but their retired objective and unlock links are zeroed. The Internet Archive preserved Bungie's
2020-06-09 manifest endpoint response (`84291.20.05.27.1646-1`) but not the component JSON or SQLite
files referenced by that response. Consequently, the archived endpoint proves which historical
files existed, not the removed record-to-objective or record-to-unlock relationships.

## Sample 2: neighboring slots 101-116

Captured on 2026-08-15. Slots 104, 109, and 111-114 repeated Sample 1 exactly and are not
duplicated here.

| Slot | First words | Structural observation |
| ---: | --- | --- |
| 101 | `A0B06507 A0B06506 A0B06505 A0B06504 ...` | Dense hash-like array; identity unknown |
| 102 | `6E2F4E8C 1A5F3825 268AB804 90F21F6F ...` | Dense hash-like array; identity unknown |
| 103 | `27785BD2 5180A26B 0D07754A 1EB72D4F ...` | Dense hash-like array; identity unknown |
| 105 | `7880EDEB 503A179E BDA5CA0B CADBF79E ...` | Dense hash-like array; identity unknown |
| 106 | `B8633C07:FFFF AE8AF15F:1FFFF AE8AF15E:1FFFF ...` | Compact hash/selector map; identity unknown |
| 107 | `EFCD14BE 811C9DC5 E91BB86C B44A3287 ...` | Contains Sunrise's `kNoPlugSource` sentinel; likely socket/plug-adjacent rather than Hall state |
| 108 | `B5ADFB6B 0 0 0 37A8 0 1 0` | Structured row or 64-bit value array; identity unknown |
| 110 | `0894FFFF 0 1 0 16300 0 2 0` | Structured row or 64-bit value array; 2,272 rows makes records a candidate, not a conclusion |
| 115 | `8C1D1B17 0 19 0 7C0 0 25C 0` | Structured row or 64-bit value array; identity unknown |
| 116 | `C5609C19 0 0 0 8131907E 0 41210F17 0` | Structured row containing at least one valid package tag; identity unknown |

None of the newly sampled words matched keys in the focused live Bungie components. The next
diagnostic records blob size, array-data offset, trailing byte count, and quotient/remainder by row
count. A zero remainder and plausible quotient would establish a candidate fixed row stride; it
would not by itself identify the table.

## Tribute Hall record anchors

Bungie's live `DestinyPresentationNodeDefinition` for node `1519064131` retains these 19 records.
The index column is from the current live manifest and is only an anchor. It must not be treated as
the installed Shadowkeep row index until a native row correlation proves it.

| Record hash | Current index | Name |
| ---: | ---: | --- |
| 602717866 | 1761 | Golden Offerings |
| 81793239 | 1762 | Thrifty |
| 3984906758 | 1763 | Gotta Collect 'Em All |
| 2819776107 | 1764 | Novice Curator |
| 2993512400 | 1765 | Imperial Footsoldier |
| 1166349878 | 1766 | High Roller |
| 2687773035 | 1767 | Lavish Defender |
| 964370980 | 1768 | Imperial Scoundrel |
| 3127706764 | 1769 | Imperial Assassin |
| 2801938470 | 1770 | The Scoundrel in Uniform |
| 1589566794 | 1771 | The Emperor's Gladiator |
| 3186404157 | 1772 | Into the Coliseum |
| 2379468474 | 1773 | Final Exam |
| 3035323962 | 1774 | A Succulent Wish |
| 3559225578 | 1775 | Fist of the Empire |
| 4038457128 | 1776 | Crown of Sorrow |
| 2307353670 | 1777 | Champion Triumphant |
| 3484748920 | 1778 | Completionist |
| 1662610173 | 1779 | Only the Essentials |

## Sample 3: array-size and stride evidence

Captured on 2026-08-15. `tail` is `blob size - array data offset`. Division by row count
only identifies a fixed stride when the array reaches the end of the blob or has a small understood
footer; tables with nested or appended data will not divide cleanly.

| Slot | Identity or status | Rows | Tail bytes | Division result | Interpretation |
| ---: | --- | ---: | ---: | --- | --- |
| 48 | Confirmed inventory item index | 15,424 | 370,176 | `24 x 15,424` | Exact; reproduces Sunrise's known 24-byte index-row stride |
| 97 | Confirmed socket-entry-list index | 14 | 336 | `24 x 14` | Exact 24-byte index rows |
| 101 | Unknown | 4,301 | 17,204 | `4 x 4,301` | Exact 4-byte values |
| 102 | Unknown | 27 | 108 | `4 x 27` | Exact 4-byte values |
| 103 | Unknown | 16 | 64 | `4 x 16` | Exact 4-byte values |
| 104 | Unknown | 12,506 | 50,024 | `4 x 12,506` | Exact 4-byte values |
| 105 | Unknown | 52 | 208 | `4 x 52` | Exact 4-byte values |
| 106 | Unknown | 2,481 | 19,848 | `8 x 2,481` | Exact 8-byte rows |
| 107 | Socket/plug-adjacent candidate | 403 | 3,224 | `8 x 403` | Exact 8-byte rows |
| 110 | Record-table candidate | 2,272 | 93,160 | `41 x 2,272 + 8` | Coherent packed 41-byte rows plus an 8-byte footer |
| 116 | Unknown | 511 | 12,264 | `24 x 511` | Exact 24-byte rows |

Slots 19, 51, 68, 96, 108, 109, 111-115 do not divide into a trustworthy native row stride
because their blobs contain additional or nested data. Their quotient values are not row sizes.

Sample 4 scanned slots 101-116 for the 19 exact Tribute Hall record hashes. It compared
bounds-checked 32-bit values at every byte alignment and found zero matches. This rules out those
modern hashes as raw values in the sampled late-root tables, including the 41-byte rows in slot 110;
it does not rule out historical hashes, indirect indices, or records elsewhere in the root.

Sample 5 widened the same read-only scan to all 105 readable root tables (11.94 MiB total) and
again found zero matches. The installed root tables therefore do not store any of the 19 modern
Tribute Hall record hashes verbatim. This does not distinguish historical hash changes from an
indirect definition representation.

The next one-time diagnostic writes the 105 already-decompressed root blobs to a versioned binary
snapshot. Offline correlation against every saved official Record, Objective, Presentation Node,
Unlock, and Unlock Value key can then identify table families without further guessed hash scans.
The snapshot contains only package-derived diagnostic bytes and does not modify package or account
state.

The first snapshot attempt logged `tables=0 bytes=0 result=fail`: its relative output directory did
not resolve under the game's process working directory. No package bytes were captured, so this is
an output-path failure rather than a table result. The retry resolves Sunrise's artifact directory
from the loaded game module, matching the cache and log path convention, and appends the diagnostic
filename to that verified absolute path.

## Sample 6: complete root snapshot correlation

The corrected snapshot captured 105 framed tables and 12,520,742 payload bytes. Its 12,522,430
byte file size is exactly the 8-byte file header, 105 16-byte table headers, and all payload bytes.
Offline aligned-word correlation against the saved official live-manifest components identified
these table families:

| Definition family | Installed slot | Unique live-key matches | Structural result |
| --- | ---: | ---: | --- |
| Record | 72 | 48 | 2,242 primary rows of 216 bytes; definition hash at row `+0x28` |
| Record redirect/index | 73 | 28 | 66 exact 8-byte `{hash, native record index}` rows |
| Objective | 58 | 1,722 | Primary installed objective family |
| Presentation node | 63 | 690 | 924 primary rows of 168 bytes; definition hash at row `+0x28` |
| Unlock | 111 | 10,575 | Historical unlock-definition mapping |
| Unlock | 112 | 12,477 | Second historical unlock mapping |
| Unlock value | 113 | 2,533 | Historical unlock-value mapping |
| Unlock value | 114 | 3,440 | Second historical unlock-value mapping |

Presentation-node row `+0x88` is a compact record-child list. Exactly three installed nodes own 19
records: node 760 points to records 474-492, node 848 to records 1429-1447, and node 895 to records
1636-1654. The live pre-cutoff 19-record nodes are Iron Banner, Garden of Salvation, and The Tribute
Hall in the same node order and record-range order. This makes node 895 and records 1636-1654 the
strong Hall mapping. The names below are aligned by that preserved group order; the native rows,
hashes, objective references, and state slots are direct installed-byte evidence.

| Live-order name anchor | Native record row | Historical hash | Objective reference | Evaluated-state slot |
| --- | ---: | --- | --- | ---: |
| Golden Offerings | 1636 | `0xC0825913` | `0x1239` | 18877 |
| Thrifty | 1637 | `0x087B2E46` | `0x123A` | 18878 |
| Gotta Collect ’Em All | 1638 | `0x547644C1` | none | 18879 |
| Novice Curator | 1639 | `0x7BB69FF0` | none | 18880 |
| Imperial Footsoldier | 1640 | `0x17C63887` | `0x204D` | 18881 |
| High Roller | 1641 | `0x50CFE7DF` | `0x204E` | 18882 |
| Lavish Defender | 1642 | `0x63E3919E` | `0x204F` | 18883 |
| Imperial Scoundrel | 1643 | `0x69B51661` | `0x2050` | 18884 |
| Imperial Assassin | 1644 | `0x5E84ACE3` | `0x2051` | 18885 |
| The Scoundrel in Uniform | 1645 | `0x05A631C9` | `0x2052` | 18886 |
| The Emperor’s Gladiator | 1646 | `0x230C5CD1` | `0x2053` | 18887 |
| Into the Coliseum | 1647 | `0xE16C25A0` | `0x2054` | 18888 |
| Final Exam | 1648 | `0xBF06957D` | `0x2055` | 18889 |
| A Succulent Wish | 1649 | `0xC40C8175` | `0x2056` | 18890 |
| Fist of the Empire | 1650 | `0xBF213E9B` | `0x2057` | 18891 |
| Crown of Sorrow | 1651 | `0x01E380F5` | `0x2058` | 18892 |
| Champion Triumphant | 1652 | `0xF64B9535` | `0x2059` | 18893 |
| Completionist | 1653 | `0x06CF746D` | `0x205A` | 18894 |
| Only the Essentials | 1654 | `0x3026C054` | none | 18895 |

The smallest state test published all 19 slots as logical value 2 through the existing bounded
`family5_flag_overrides` configuration. Sunrise committed and armed the Family-5 object, and the
Client loaded `trophy_hall_freeroam`, but the door remained closed and no statues, dispensers,
interactables, or enemies appeared. The overrides were removed after that clean negative result.
These slots are record evaluated-state anchors; they are not sufficient evidence of the activity
object state that assembles the upgraded Hall.

## Trophy Hall activity-state evidence

The generated build-data cache exposes one bubble for `trophy_hall_freeroam`, map-global bubble 2,
and three compatible map-package spawn sets. These sets choose player arrival points; they are not
enemy or decoration groups.

| Spawn-set hash | Points | Loaded | Offered in Hall bubble | Meaning |
| --- | ---: | :---: | :---: | --- |
| `0x2EA8FB98` | 3 | yes | yes | Client-defined `default` arrival set |
| `0x64EC43A5` | 3 | yes | yes | Unnamed alternate arrival set |
| `0x9E91918E` | 3 | yes | yes | Unnamed alternate arrival set |

### Initial-slice task-manager diagnostic

The Hall's current blocker is after map precache and instantiation, in boot-flow step 36
(`activity:initial_slice_set_loading`). A read-only pass-through diagnostic observes the native
step-36 callback's return value and task-manager masks; it returns the original value unchanged and
never writes to the Client's task state. This hook is research instrumentation outside the official
Sunrise guide and must remain uncommitted until the behavior is understood.

Static analysis maps 11 task descriptors in the initial-slice state. Dependency masks use the task
id as the bit index:

| Task | Callback RVA | Dependency mask | Required earlier tasks |
| ---: | --- | --- | --- |
| 0 | `0xD48050` | `0x000` | none |
| 1 | `0xD47980` | `0x000` | none |
| 2 | `0xD46BF0` | `0x002` | 1 |
| 3 | `0xD46050` | `0x002` | 1 |
| 4 | `0xD47760` | `0x000` | none |
| 5 | `0xD48140` | `0x001` | 0 |
| 6 | `0xD46900` | `0x010` | 4 |
| 7 | `0xD46DD0` | `0x010` | 4 |
| 8 | `0xD46B50` | `0x01E` | 1, 2, 3, 4 |
| 9 | `0xD474D0` | `0x090` | 4, 7 |
| 10 | `0xD471D0` | `0x290` | 4, 7, 9 |

The four fields at state offsets `+0x198`, `+0x1A0`, `+0x1A8`, and `+0x1B0` are four 64-bit task
masks, not four eight-byte task records. Equivalently, they are task-manager offsets `+0x168`,
`+0x170`, `+0x178`, and `+0x180`. An earlier diagnostic printed only the low byte at each offset
and called bit 7 a completion flag. That output was useful for task 7 but could not show task 9,
which is bit 9 in the second byte. The corrected diagnostic uses neutral mask names until each
mask's exact scheduler meaning is proven:

| State offset | Manager offset | Correct diagnostic field | Coverage |
| --- | --- | --- | --- |
| `+0x198` | `+0x168` | `mask_198` | tasks 0-63 |
| `+0x1A0` | `+0x170` | `mask_1A0` | tasks 0-63 |
| `+0x1A8` | `+0x178` | `mask_1A8` | tasks 0-63 |
| `+0x1B0` | `+0x180` | `mask_1B0` | tasks 0-63 |

A normal Moon control run supplies spawn set `0x2EA8FB98` and reaches the observed callback's
return 2 with the following complete masks:

| Snapshot | `mask_198` | `mask_1A0` | `mask_1A8` | `mask_1B0` |
| --- | --- | --- | --- | --- |
| Moon, task-7 return 2 | `0x00000000000007FF` | `0x00000000000000FF` | `0x0000000000000000` | `0x0000000000000073` |
| Hall, task-7 return 2 | `0x00000000000007FF` | `0x00000000000000FF` | `0x0000000000000000` | `0x0000000000000033` |

Task 9 starts immediately afterward and completes in 2.421 seconds; task 10 then starts and
completes in 1.702 seconds before boot flow advances through steps 37 and 38
(`activity:in_world`). The exact native task-9 start line originates at caller RVA `0xD49EDC`;
its captured stack begins `0xD49EDC, 0xD4B4EB, 0xD4BA29, 0xE2359B, 0xE43539`.

The Hall control with explicit spawn set `0x2EA8FB98` corrects the earlier interpretation that task
9 never started. At the task-7 return-2 snapshot, only `mask_1B0` differs: Hall has `0x33` where
Moon has `0x73`, meaning task 6 has not yet reached state 3. Task 9 starts immediately anyway.
Hall tasks 2 and 6 then reach state 3 after 5.846 and 5.842 seconds, task 3 completes after 6.112
seconds, task 8 starts and completes in 29 ms, and the initial-slice transition reports completed.
Task 9 nevertheless remains pending, task 10 never starts, and boot flow stays at step 36. The
missing bit 6 is therefore a timing symptom, not the final blocker.

Static analysis of task 9's callback at RVA `0xD474D0` shows that it checks state 3 for tasks 0, 2,
5, and 6, requires task 8 to be past state 0, and also evaluates four non-task readiness gates. The
observed Hall sequence eventually satisfies every task-state predicate, so the next bounded probe
should report the remaining gates without changing them:

| Task-9 gate | Static location | Required result | Meaning |
| --- | --- | --- | --- |
| Global byte | `0x27F66F4` | zero | Unknown; nonzero returns pending early |
| Selector state | calls at `0x4AFE80` and `0x4FFDA0` | derived Boolean true | Unknown |
| Selected object byte | `0xA55FF0(id)` result `+0x08` | nonzero | Unknown |
| Tasks 0, 2, 5, 6 | four task masks | state 3 | Satisfied after Hall transition work |
| Task 8 | four task masks | decoded state nonzero | State 0 fails; states 1-3 pass, and native `-1` also passes this test |
| Final global predicate | call at `0xC240F0` | false | Unknown |

The follow-up observer is isolated in `task_nine_gate_observer.cpp` rather than expanding the
production world-step implementation. Its task-9 signature resolves uniquely at RVA `0xD474D0`
in the saved Shadowkeep text image. It resolves the selector, selected-object lookup, final
predicate, and early global byte from task 9's own checked instructions, then installs all four
detours in one transaction. Each replacement calls the native target exactly once and returns that
unchanged result. Thread-local bracketing plus exact return-address checks ensure helper calls are
recorded only when task 9 itself made them; the diagnostic never invokes a helper a second time.
It reports only changed snapshots and labels still-unknown values neutrally. This remains
uncommitted research instrumentation outside the official Sunrise guide.

The successful Moon gate control reaches the following final task-9 snapshot immediately before
the callback returns 3:

| Gate | Moon value | Direct conclusion |
| --- | --- | --- |
| Early global byte | readable, `0` | Required zero state is present |
| Tasks 0, 2, 5, 6, 8 | `3,3,3,3,3` | Every observed task is complete |
| Selector | called, returned true, value `3` | Selector gate is satisfied |
| Selected object | id `0x3CFEC000`, present, byte `+0x08 = 1` | Selected-object gate is satisfied |
| Final predicate | called, returned false | Required false state is present |

Task 9 completes after 1.650 seconds, task 10 completes after 1.705 seconds, and boot flow enters
steps 37 and 38. During earlier pending polls, the observer read the selector output slot even when
the selector returned false. The native callback does not read that slot on the false path, so its
alternating `0`/uninitialized values are diagnostic noise rather than game state. The observer now
records the output only on a true selector result before the Hall comparison.

A fresh launch from the Hall's own node with Activity Override disabled confirms that its client
selection names activity index 143, hash `0xE5FBF1EF`, and package `trophy_hall_freeroam`. Both
the optional arrival-bubble hash and spawn-set hash are absent (`0x811C9DC5`). The host derives
runtime region 0 and slice set 0 while preserving the absent spawn set. At world step 36, every
observed prerequisite task is in state 3 and the early byte is zero, but task 9 returns 1 with the
following non-task values:

| Gate | Natural Hall-node value | Moon control difference |
| --- | --- | --- |
| Selector | returned false; output not read | Moon returns true and writes `3` |
| Selected object | id `0x1CFEC000`; null | Moon uses `0x3CFEC000`, present, byte `+0x08 = 1` |
| Final predicate | returned false | Same as Moon |

The bounded selector experiment still calls the native selector first. It substitutes value `3`
and a true result only when the direct task-9 call returned false, tasks 0, 2, 5, 6, and 8 are all
state 3, and the early byte is zero. Task 9's selected-object and final-predicate checks remain
native. The override fires, but task 9 remains pending: the selector reports true with value `3`,
while the selected-object lookup is still null. This is a useful negative result, not a production
fix.

The captured unpacked callback corrects one interpretation of the cross-run object ids. The first
helper at task offset `+0x5A` writes the object id to a separate local, and the selected-object
lookup consumes that local unconditionally. The selector at `+0x64` writes another local and only
controls a Boolean gate. Therefore the Hall ids `0x1CFEC000`, `0x39FEC000`, and `0x74FEC000`
from separate launches must not be treated as a value transformed by the selector; their changing
high byte is run-specific. The stable conclusions are that Hall's selector is false and its
selected-object lookup is null, while both gates are satisfied in the Moon control.

A separate bounded completion experiment leaves every native helper unchanged. Only after the
exact observed Hall-stall shape is complete does it replace task 9's native return 1 with return 3.
That makes task 9 complete immediately, task 10 complete normally after 1.919 seconds, physics join
finish after 6.012 seconds, and boot flow enter step 38 (`activity:in_world`). It does not create a
valid arrival: the natural node still carries absent spawn hash `0x811C9DC5`, spawn state remains
zero, no teleport target appears, the Client repeatedly fades, and its in-world job fiber reports a
continuous hitch. Task 9 is therefore guarding real arrival readiness; bypassing it is diagnostic
evidence, not the fix.

The full `0x290`-byte runtime capture resolves task 9's final native decision exactly. Its default
return is 1. It changes that return to 3 only when all of the following are true: the selector's
derived Boolean is true, the selected-object lookup is non-null with byte `+0x08` nonzero, decoded
tasks 0, 2, 5, and 6 all equal state 3, decoded task 8 is nonzero, and the final predicate returns
false. The task-8 check is literally a zero test; its decoded absent value `-1` would also pass.
This confirms that the completion experiment bypasses two native Hall failures at once: its
selector remains false and its selected-object lookup remains null.

A follow-up test supplied each concrete spawn-set choice exposed by Activity Override while
retaining the same bounded task-9 return experiment:

| Forced spawn set | Boot-flow result | Published arrival state | Visible result |
| --- | --- | --- | --- |
| `0x2EA8FB98` | Reached step 38, `activity:in_world` | `spawn_state=0`, `teleport_state=0`, `teleport_slice=-1` | Solid black screen |
| `0x64EC43A5` | Reached step 38, `activity:in_world` | `spawn_state=0`, `teleport_state=0`, `teleport_slice=-1` | Solid black screen |
| `0x9E91918E` | Reached step 38, `activity:in_world` | `spawn_state=0`, `teleport_state=0`, `teleport_slice=-1` | Solid black screen |

Changing the roster's spawn-set field is therefore insufficient to create or acknowledge the
player. The three values change the selected launch data, but none repairs either native task-9
object gate or produces a camera. Further blind spawn-set iteration is not justified by this
evidence; the next bounded observer should follow the native selector/object creation path instead.

The next pass-through build captured `0x400` runtime bytes from each helper while the Client was
only in orbit. The resolved addresses below are specific to this installed Shadowkeep executable;
the observer derives them from task 9's checked call instructions instead of hard-coding them.

| Helper | Disassembled native behavior | Direct consequence for Hall |
| --- | --- | --- |
| Object-ID source, `0x4AFE80` | Initializes output to `-1`, obtains and validates a current 16-bit registry index, then composes a 32-bit datum ID from the registry row and its generation/configuration fields | Hall's non-`-1` IDs show this stage succeeds; the changing high byte is generation-like while the low 13-bit row index remains stable |
| Selector, `0x4FFDA0` | Calls `0x4FF9F0`; returns false if that source is null or its first byte is `0xFF`, otherwise writes that signed byte and returns true | Hall's false result is now narrowed to exactly those two source states; Moon's true value `3` is the source byte itself |
| Selected-object lookup, `0xA55FF0` | Rejects ID `-1`, resolves its low 13-bit primary row, follows a linked handle at primary-row offset `+0x50`, rejects linked ID `-1`, then calls `0xA56460` and returns that result | Hall supplies a valid primary ID, so its null result is narrowed to a missing `+0x50` linked handle or a null result from `0xA56460` |

A paired native pass-through diagnostic then observed `0x4FF9F0`'s pointer/first byte and whether
the `0xA56460` resolver was reached. The Moon and natural Hall node were loaded in the same process,
with no task return or helper value changed:

| Native witness | Moon control | Natural Trophy Hall |
| --- | --- | --- |
| Type-17 selector source | null, then present with byte `0xFF`, then present with byte `3` | present and readable, but remains byte `0xFF` |
| Selector result | false until the byte becomes `3`, then true with value `3` | false |
| Task-9 primary object ID | `0x6BFEC000` in this run | `0x27FEC000` in this run |
| Linked-object resolver `0xA56460` | initially not reached, then reached and non-null | never reached |
| Selected object | present, readable, byte `+0x08 = 1` | null |
| Native task-9 result | `3`, followed by steps 37 and 38 | `1`, remaining at step 36 |

Offline disassembly of `0x4FF9F0` shows that it calls `0x4EA280` with constant selector type 17.
When the returned handle is valid, it decodes that handle and returns the runtime component record
at offset `+0x180`; `0x4FFDA0` reads that record's first byte. Hall's `0xFF` is therefore a
present-but-uncommitted type-17 runtime state, rather than a missing selector source.

The linked-object resolver at `0xA56460` reads a handle at its input row's offset `+0x1D0`, rejects
`-1`, decodes it, and returns the decoded component address relative to the base at `+0x1D8`.
Because Hall's primary lookup never calls this resolver, the earlier `0xA55FF0` walk stops first:
the task-9 primary row's linked handle at `+0x50` is still `-1`. Moon eventually satisfies the
complete chain.

These two failures move together: Hall neither commits its type-17 runtime byte nor creates the
primary row's linked object, while Moon does both. The strongest current hypothesis is that Hall's
167-object, two-group roster transaction is not committing as a unit. This is not proven merely by
the accepted wire frames. The next clean control therefore withholds only the exact 146-object
ambient supplement at snapshot time and publishes the unchanged 21-object primary group. Task 9
and all of its helpers remain native; the extracted build-data cache is left unchanged.

The paired trace is preserved as
`backups/deployments/tribute-hall-task9-nested-observer-20260816/sunrise-moon-hall-nested-observer.log`
with SHA-256 `af1426aa61fbdc571ea16a3266bcedbffbb9b932dbb8ac56f9dd462ecd82391d`.
That deployment's pass-through DLL has SHA-256
`280ab9ef9ef7664bcf1a2cfe1cb601edc5c9800fcaefe3ca8ca362ed41d487ed`.
Its selector-source capture has SHA-256
`a5456a1f423670b92f0ac82e2f25517bc391485c3bf5d18792ff69f51c372b67`; its object-resolver
capture has SHA-256 `162626b810474016e5e26a8b38a637bbdb407ba25d771912747168ab89a34006`.
The cache and settings remained unchanged at
`bead2c68e79cc0facf93527c9d29c190dfe9e552d27f37f041444f39a7966a27` and
`746ca57fdaa3882b5a79e52eb846091485c22af80be9e01b27c775618af6ba10`, respectively.

The exact one-group control is archived under
`backups/deployments/tribute-hall-one-group-control-20260816/`. Its deployed DLL has SHA-256
`f056011a373dba796a16040ef7534dc50a6151ddf4b586a0a9711bd2340051bc`; the native nested-observer
rollback DLL has SHA-256 `280ab9ef9ef7664bcf1a2cfe1cb601edc5c9800fcaefe3ca8ca362ed41d487ed`.
The source copy has SHA-256 `91cc6ed744074347b483cd51709822373a8f30c573c804ef15777190538cd96d`.
The cached build data and settings were copied without modification at the hashes above.

The natural-node test cleanly separated arrival from ambient content. The control published one
group with 21 objects; its steady roster body was 501 bytes and its state sequence reached 3.
Task 9 then returned native value 3 with no override: the type-17 selector source was byte 3, the
linked-object resolver was reached, and the selected object at `0x75FEC000` had byte `+0x08 = 1`.
Task 10 completed after 1.920 seconds, physics join completed after 6.013 seconds, and the Client
entered step 38 with full player, camera, and movement control. The door remained closed and no
statues, dispensers, enemies, or other ambient Hall entities appeared.

This establishes that the 146-object supplemental group is causal to the initial two-group commit
failure, although it does not yet identify whether size, body semantics, or publication timing is
the underlying reason. The successful trace is preserved as
`backups/deployments/tribute-hall-one-group-control-20260816/sunrise-hall-one-group-success.log`
with SHA-256 `0ef5e457952a7c0962cefedb01919148c58698f4564ec273eba5267c709b250c`.
The installed DLL, cache, and settings remained at the archived hashes.

The next bounded control uses Sunrise's existing `WorldPhase::arrived` signal. While the Client is
below step 38 it publishes only the primary group. After step 38, the next ordinary keepalive
publishes both groups; the existing folded-group comparison advances the roster state byte and the
existing encoder rollback restores all counters if staging fails. No task return, native helper,
or extracted cache row is changed. Because a changed roster state rebuilds all roster-owned
objects, the live test must watch both ambient activation and continued player control.

The staged build is archived under
`backups/deployments/tribute-hall-staged-ambient-20260816/`. Its deployed DLL has SHA-256
`998bdf0d4cd117b6227847da1908faae374f6f70e7875c6ece78a463705e6dfa`; the successful one-group
rollback DLL has SHA-256 `f056011a373dba796a16040ef7534dc50a6151ddf4b586a0a9711bd2340051bc`.
The exact staged source has SHA-256
`a4a80665896699ec0989ed692d17712232cb0efd941fc6545a44636e5e32ffdc`. Cache and settings remain
unchanged at their archived hashes.

The staged live test reached step 38 on the 21-object primary group, then the next ordinary
keepalive advanced the roster state from 3 to 4 and published both groups with all 167 objects in a
4,120-byte body. The Client remained controllable and accepted stable repeated state-4 updates.
Immediately after the handoff it emitted three type-6 `sensor_sense_update` messages with payload
sizes 1,297, 235, and 276 bytes. No decoder or activity error followed.

The ambient group produced visible but mostly non-functional content: all trophy pedestals appeared,
green smoke appeared at the Bad Juju shrine, and a purple effect appeared at the door-side location.
None of those observed elements offered an interaction prompt. The door stayed closed, and the
vendor, enemies, ammo dispensers, and target-range functionality remained absent. This separates
package-local arrangement/effect activation from the still-neutral gameplay authority carried by
slot types 1, 2, 4, 5, 23, and 70.
The exact trace is preserved as
`backups/deployments/tribute-hall-staged-ambient-20260816/sunrise-hall-staged-partial-activation.log`
with SHA-256 `79a518e8f8645d0877e411484fb07a7f390d706d326ce8878898bc08d2e82ef7`.

A later staged-observer run found one hidden urn with an `[E] Inspect` prompt, proving that at least
one ambient object retained a working proximity and interaction component. Activating it once made
the urn disappear in a transmat-like effect and immediately emitted one validated type-19 activity
incident: primary target `3539`, zero extra targets, no compressed selector, and an 82-byte declared
incident payload. Sunrise currently validates this message but does not relay or act on it, and no
explicit collectible, lore, triumph, or progression mutation followed in the log. The local visual
completion is therefore confirmed; an intended lore or account reward remains a hypothesis until
target row 3539 or the incident payload schema is resolved.

The same run tested whether type-6 `sensor_sense_update` reused type 5's immediate
`group key -> biased slot type -> biased slot index -> block length` object-reference shape. The
three messages contained one exact primary-key occurrence and 76 exact ambient-key occurrences,
but zero candidates passed that full layout and the installed roster bounds. Type 6 therefore uses
a different field order, grouping, or reference encoding; the failed shape is not suitable for a
state responder. The exact closed-run trace is preserved as
`backups/deployments/tribute-hall-sense-shape-observer-20260816/sunrise-hall-sense-shape-urn.log`
with SHA-256 `bbb83ae0ca5730c2c1548ad585a3c9937a3dd22d0379361e6b4f1283e3348dd2`.

Upstream marks decrypted activity payloads as sensitive and explicitly forbids logging, capturing,
caching, hashing, or retaining them. The refined observer follows that rule: it scans at most 16
small type-6 payloads in borrowed memory for only the two already-known Hall roster keys. It reports
distances between those key matches and accepts a nearby biased type/index candidate only when both
fields name the same exact slot in Sunrise's immutable extracted Hall roster. It records no payload
bits or bytes and keeps no payload-derived storage after the call. The observer is isolated in
`activity_sensor_sense_observer.cpp` and remains temporary research instrumentation.

The observer build is archived under
`backups/deployments/tribute-hall-sense-shape-observer-20260816/`. The build, installed DLL, and
archived observer DLL are byte-identical at SHA-256
`525ecce9c4fb86122e8062ab04823738fbd5e3390f81f323abf05df3ed795eac`. The rollback copy is the
verified staged-ambient DLL at SHA-256
`998bdf0d4cd117b6227847da1908faae374f6f70e7875c6ece78a463705e6dfa`. The installed build-data
cache and settings were not modified and remain at SHA-256
`bead2c68e79cc0facf93527c9d29c190dfe9e552d27f37f041444f39a7966a27` and
`746ca57fdaa3882b5a79e52eb846091485c22af80be9e01b27c775618af6ba10`, respectively.

The refined coordinate observer is archived under
`backups/deployments/tribute-hall-sense-coordinate-observer-20260816/`. The build, installed DLL,
and archived deployment are byte-identical at SHA-256
`6e7f440b4ce24c097b6be19e2af4a88518dc3a20e9bcfdda26eb81fdfa1ae478`. Its immediate rollback is
the tested shape-and-urn observer at SHA-256
`525ecce9c4fb86122e8062ab04823738fbd5e3390f81f323abf05df3ed795eac`; the earlier staged-only
rollback remains archived separately. Cache and settings remain unchanged at the hashes above.

The refined live run reached step 38 naturally, staged both groups at state 4, retained player
control, and reproduced the pedestals plus green and purple effects. Its three type-6 messages were
1,326, 235, and 235 bytes. Exact-roster validation found 51, 10, and 10 object coordinates,
respectively; 61 slots were unique across the run: all 16 type-1 slots, 29 of 47 type-4 slots, and
16 of 20 type-23 slots. No type-2 or type-70 coordinate appeared.

Every genuine object coordinate put the biased type at exactly `group_key_bit + 32` and the biased
index immediately after it. The prior parser therefore had the correct identity prefix and failed
only because type 6 does not carry type 5's 32-bit block length there. A group header key was 65
bits before its first object key, matching `key + 32-bit filler + one continuation bit`. For
non-final records, subtracting the 55-bit key/type/index identity and the next continuation bit from
the next-key distance gives observed sense-body lengths: type 4 was 165 bits, type 23 was 167 bits,
and type 1 used 56-, 85-, and 92-bit forms. These are observed variants, not yet promoted to
complete schema rules. The broader nearby scan also produced one `+97` duplicate for each first
object by seeing the next record's identity; only `+32` coordinates are genuine.

The exact successful trace is preserved as
`backups/deployments/tribute-hall-sense-coordinate-observer-20260816/sunrise-hall-sense-coordinate-success.log`
with SHA-256 `eecaa8b2c129249f4efba27cb85e5b4cdf0c2461866fb1814f0d6d11924b175c`.

An upstream-first check found official Sunrise still at tag `0.2.1` / commit
`b12a9dab780f47c89f1c147d4a8ef3ddbc839734`; its type-6 path remains an accepted one-way no-op.
The current Linux-fork head adds no later sensor or entity-authority implementation, and exact
public-source searches for the three sense schema ids found no protocol definition. The next local
step therefore remains temporary framing research rather than a guessed responder.

The exact record framer removes the broad neighboring-bit search. It accepts an object only when
the fields exactly 32 bits after a known group key name the same slot in the immutable installed
roster. A sense-body endpoint is reported only when the immediately following known key is another
validated object in the same group; one continuation bit is removed from that gap. Final objects
remain explicitly unframed. It classifies the previously observed type-1, type-4, and type-23 body
widths but does not decode, retain, or report body data and does not change activity state.

The framer is archived under
`backups/deployments/tribute-hall-sense-record-framer-20260816/`. The build, installed DLL, and
archived deployment are byte-identical at SHA-256
`21f4eb8eee69a8ddbaa2d534c69f8bf00957e2b4eb83f7b4205868b52d970363`. Its immediate rollback is
the successful coordinate observer at SHA-256
`6e7f440b4ce24c097b6be19e2af4a88518dc3a20e9bcfdda26eb81fdfa1ae478`. Cache and settings remain
unchanged at the hashes above.

The bounded live test reached task 9's native return 3 and world step 38 without forcing either
result. After arrival, the normal staged publication advanced to state 4 with both groups and all
167 objects. The Client accepted all three type-6 messages, normal keepalives continued, and the
user confirmed full player control plus the previously established ambient presentation: trophy
pedestals, green Bad Juju-shrine smoke, and the purple door-side effect. No new Hall behavior was
introduced by the observer.

| Message | Bytes | Exact objects | Framed | Known body variants | Group headers | Deliberately unframed tail |
| ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 1 | 1,297 | 50 | 49 | 49 | 1 | type 4, index 88; 172 bits remain |
| 2 | 276 | 12 | 11 | 11 | 1 | type 23, index 72; 170 bits remain |
| 3 | 235 | 10 | 9 | 9 | 1 | type 23, index 64; 177 bits remain |

All 69 records with a following exact object boundary matched an already observed body width. Each
message contained exactly one validated group header and exactly one final object that the framer
left unresolved by design. The final type-4 remainder is seven bits longer than its observed
165-bit body; the final type-23 remainders are three and ten bits longer than its observed 167-bit
body. That variation is consistent with message-footer and byte-alignment effects, but it does not
identify their schema. Final-record endpoints must therefore remain unresolved until the footer is
understood.

The exact closed-run trace is preserved as
`backups/deployments/tribute-hall-sense-record-framer-20260816/sunrise-hall-sense-record-framer-success.log`
with SHA-256 `e36cf5a86270fa4ea3d262c6690f7d083ae2c511fd29bfcfe32400893285e9c8`.
The installed build-data cache and settings remained unchanged at SHA-256
`bead2c68e79cc0facf93527c9d29c190dfe9e552d27f37f041444f39a7966a27` and
`746ca57fdaa3882b5a79e52eb846091485c22af80be9e01b27c775618af6ba10`, respectively.

### Sensor-sense native schema registry

The next orbit-only inspection read the Client's native schema registry directly from borrowed
process memory. It did not attach a debugger, pause or write to the process, and it did not read,
copy, hash, log, or retain any activity-message payload. These structures are reverse-engineering
evidence outside the official Sunrise guide. Runtime metadata addresses below identify this one
ASLR-dependent run and are not stable integration constants.

| Slot type | Sense schema | Runtime metadata | Size | Fields | Recovered status |
| ---: | ---: | ---: | ---: | ---: | --- |
| 1 | `0x80807ECC` | `0x6FFFF8A6C9D0` | `0x288` | 13 | Complete outer and nested field shapes |
| 4 | `0x8080992E` | `0x6FFFF8977E70` | `0x170` | 6 | Complete through the outer `kind 0x22` boundary |
| 23 | `0x80804F47` | `0x6FFFF899F550` | `0x170` | 6 | Complete outer field shape |

Ordinary metadata stores the field count at `+0x60`, schema id at `+0x68`, and its first
`0x28`-byte field descriptor at `+0x80`. Two wrapper schemas instead use `+0x68`, `+0x70`, and
`+0x88`. Descriptor word 4 is the field kind and word 5 is the nested schema id when present;
adding `0x100` to a kind marks the field optional. Bias and encoded width were read from the later
descriptor words and cross-checked against the already recovered auth encoders.

| Type | Native field order |
| ---: | --- |
| 1 | optional widths `31, 31, 31, 6, 7, 31`; fixed widths `2, 3, 1, 1, 1`; optional `0x80807ECF`; optional `0x80807ECD` |
| 4 | signed 32; bool; bool; signed 32; nested `0x80809E1B`; nested wrapper `0x80809AEA` |
| 23 | six optional 32-bit fields, alternating `kind 0x0B` and signed `kind 0x05` |

| Nested schema | Runtime metadata | Recovered shape |
| ---: | ---: | --- |
| `0x80807ECF` | `0x6FFFF8A71668` | wrapper: fixed width 4, then `0x80809491` |
| `0x80809491` | `0x6FFFF8A1F708` | one signed 32-bit field |
| `0x80807ECD` | `0x6FFFF8A1E8D0` | one optional width-7 field |
| `0x80809E1B` | `0x6FFFF8A20060` | one 32-bit `kind 0x09` field |
| `0x80809AEA` | `0x6FFFF8A63550` | wrapper: fixed width 2, then `0x80809AE8` |
| `0x80809AE8` | `0x6FFFF8A59240` | one polymorphic `kind 0x22` field |

The isolated validator in `activity_sensor_sense_schema_validator.cpp` only advances a bounded
MSB-first cursor over borrowed storage. It emits a structural verdict and no field values or
presence pattern. Type 1 and type 23 are eligible for `exact`; type 4 is eligible only for
`partial_kind22` when its present dynamic branch has the observed 63 remaining bits. An absent
type-4 dynamic branch may validate exactly, but the observed 165-bit form cannot be promoted beyond
partial until the branch schema is recovered. The one-bit record wrapper and the resulting live
alignment still require a Hall trace; a zero-mismatch result has not yet been claimed.

Synthetic-only checks exercised all three observed type-1 widths, the 167-bit type-23 shape, both
type-4 outer branches, truncated-body rejection, unsupported types, and a non-byte-aligned record
start. They passed before deployment and used invented bit patterns rather than captured activity
data. The validator build is archived under
`backups/deployments/tribute-hall-sense-schema-validator-20260816/`. The build, installed DLL, and
archived deployment are byte-identical at SHA-256
`0cd07f5c22848f4a89e5653d3142b527ba6908673d6d46ba69c6ebd97ce45177`. Its immediate rollback is
the successful record framer at SHA-256
`21f4eb8eee69a8ddbaa2d534c69f8bf00957e2b4eb83f7b4205868b52d970363`. The installed build-data
cache and settings were not modified and remain at SHA-256
`bead2c68e79cc0facf93527c9d29c190dfe9e552d27f37f041444f39a7966a27` and
`746ca57fdaa3882b5a79e52eb846091485c22af80be9e01b27c775618af6ba10`, respectively.

The first live validator run preserved normal Hall behavior and all 69 known record lengths, but
reported 69 schema mismatches: 49 in message 1, 9 in message 2, and 11 in message 3. The failure was
uniform across types 1, 4, and 23, while Sunrise's existing parser confirmed that a set presence
bit means present. The common leading-wrapper hypothesis was therefore rejected. The payload-safe
trace is archived as
`backups/deployments/tribute-hall-sense-schema-validator-20260816/sunrise-hall-leading-wrapper-mismatch.log`
with SHA-256 `3b990bd6110117a14b51244b80b318373d508632ce68528b65d3962ced4512eb`.

Each observed body remains exactly one bit longer than its recovered native schema: type 23 is
`6 + (5 * 32) + 1 = 167` bits, and the same placement makes the three type-1 variants 56, 85, and
92 bits. The corrected validator now parses the native schema first and consumes the shared opaque
record bit last. Type 4 likewise treats its 63-bit unresolved dynamic branch as preceding that
trailer. The corrected synthetic suite passed before deployment. This build is archived under
`backups/deployments/tribute-hall-sense-schema-validator-trailer-20260816/`; the build, installed
DLL, and archived DLL are byte-identical at SHA-256
`f1c85ba0313f8a811cac31351d850a9f16d8b8443be603e2f2f7007312aa88f3`. The failed leading-wrapper
build remains available at SHA-256
`0cd07f5c22848f4a89e5653d3142b527ba6908673d6d46ba69c6ebd97ce45177`, and the known-good framer
rollback remains `21f4eb8eee69a8ddbaa2d534c69f8bf00957e2b4eb83f7b4205868b52d970363`.

The corrected trailing-bit run also preserved normal behavior and all known widths, but all 68
framed records again mismatched: 50, 9, and 9 across its three messages. That rejects both simple
inline layouts. Its payload-safe trace is archived as
`backups/deployments/tribute-hall-sense-schema-validator-trailer-20260816/sunrise-hall-trailing-wrapper-mismatch.log`
with SHA-256 `41a012aeb6ccb89a463f314d363e1b96837a52151f31c87ca5af61585528adf2`.

The next bounded hypothesis groups a schema's optional-field markers before its present values.
Type 23 gives the strongest arithmetic evidence: one opaque record bit, six grouped markers, and
five 32-bit values total exactly 167 bits. The new validator independently tests the record bit
before the map and after the mapped values. It reports only `exact_prefix_map`,
`exact_trailer_map`, `exact_ambiguous_map`, or `mismatch`; neither the map nor any value leaves the
call. Type 1 applies the same grouping to its eight optional top-level fields and keeps the nested
single optional marker inside its nested value. Type 4 is now deliberately only partial: it checks
the 100 recovered fixed-width bits leave 65 opaque bits and makes no ordering or value claim about
the record bit and kind-`0x22` state.

Both map placements, ambiguity, truncation, arbitrary starting offsets, and all observed widths
passed synthetic-only tests before deployment. The experiment is archived under
`backups/deployments/tribute-hall-sense-schema-presence-map-20260816/`. The build, installed DLL,
and archived DLL are byte-identical at SHA-256
`76b0ac3b51f37b0506eb629abd1ed61d38bf797d6631d36d5f75b1a1bac826dd`; cache and settings remain
unchanged at their hashes above. The immediate rollback is the trailing-wrapper build at SHA-256
`f1c85ba0313f8a811cac31351d850a9f16d8b8443be603e2f2f7007312aa88f3`, with the record-framer
rollback retained separately.

The live grouped-presence run preserved normal behavior but rejected that hypothesis. Across 69
framed records it produced no exact schema matches: type 1 supplied 27 mismatches, type 23 supplied
14 mismatches, and all 28 type-4 records retained the deliberately partial `kind22` result. By
message, ordinal 1 contained 49 framed objects (`0` exact, `28` partial, `21` mismatch), ordinal 2
contained 9 (`0`, `0`, `9`), and ordinal 3 contained 11 (`0`, `0`, `11`). Three final records were
unframed and therefore not validated. The payload-safe trace is archived as
`backups/deployments/tribute-hall-sense-schema-presence-map-20260816/presence-map-run.log` with
SHA-256 `43308baa26ca7aa2dda9580a6194d0f589dee2d932bfc53ebc5943b7e1626109`.

The next diagnostic changes only the type-23 interpretation. It tests the seven possible
locations of the single unexplained bit around six inline optional 32-bit fields: before field 0,
between each adjacent pair, and after field 5. Each framed type-23 record reports one seven-bit
structural-match mask. The skipped bit, the six presence bits, and all field values remain borrowed
and are never logged, copied, hashed, or retained. Type 1 and the deliberately partial type-4
validator remain unchanged so the experiment isolates one hypothesis.

All seven intended positions, arbitrary non-byte-aligned starts, truncation rejection, deliberate
ambiguity, and the unchanged type-1/type-4 paths passed synthetic-only tests. The deployment is
archived under
`backups/deployments/tribute-hall-sense-type23-gap-positions-20260816/`. The build, archived DLL,
and installed DLL are byte-identical at SHA-256
`8534ce71b82a519ba7ed84ebca534f548116edd217a55e636c2e3a31578eb3b9`. The immediate rollback is
the grouped-presence build at SHA-256
`76b0ac3b51f37b0506eb629abd1ed61d38bf797d6631d36d5f75b1a1bac826dd`. Settings and build data
were not changed; their SHA-256 values remain
`746ca57fdaa3882b5a79e52eb846091485c22af80be9e01b27c775618af6ba10` and
`bead2c68e79cc0facf93527c9d29c190dfe9e552d27f37f041444f39a7966a27`, respectively.

That first clean post-migration build stopped after 15 ms at `initialize stage=state`, before the
sense observer could run. The installed cache and both source histories all named their format 34,
but the pre-migration header had 21 32-bit count fields while the merged header had 25 plus four
vendor record banks. Upstream's direct gameplay commit had correctly advanced its format from 23
to 24; replaying the Playground's later numeric bumps made the two incompatible histories collide
again at 34. None of the five intervening PR merges changed the cache layout. The four-line failure
trace is archived as
`backups/deployments/tribute-hall-sense-type23-gap-cache35-20260816/bootstrap-state-failure.log`.

The merged format is now 35, causing the old format-34 cache to take Sunrise's ordinary stale-cache
rebuild path. The corrected clean build, archived DLL, and installed DLL are byte-identical at
SHA-256 `60b064681df09488aeb5472af797699faced62243eb67f0d984201d8b4a7a25a`. Its archive is
`backups/deployments/tribute-hall-sense-type23-gap-cache35-20260816/`, which also preserves the
failed format-34 DLL and the last known-working pre-migration DLL. Deployment did not edit settings
or the old cache; their hashes remain the values above so the next launch itself will exercise the
versioned rebuild path.

That launch completed initialization in 378 ms and rebuilt the stale cache as format 35. The
resulting cache is archived as `build_data-format35-after.bin` with SHA-256
`8547197d6cedbff30cc9c0e6700be3dada03630d344f046f032bc709a99489b9`; post-run settings remained
byte-identical at SHA-256
`746ca57fdaa3882b5a79e52eb846091485c22af80be9e01b27c775618af6ba10`. Six Hall messages supplied
28 framed type-23 records, and every record returned `type23_gap_mask=0x00`. This rejects all seven
placements of one opaque bit around the recovered six inline optional fields. The complete
payload-safe trace is archived as `type23-inline-gap-run.log` with SHA-256
`0bbdbf13b0c09478b22f4b04bb6baf3f7cf534e1e392cb202d58cf8c51b270e2` in the same deployment
folder.

The next bounded hypothesis retains the recovered field count and widths but groups the six
markers before their present 32-bit values. It tests eight placements for the unexplained bit:
before the first marker, at each of the five inter-marker boundaries, between the marker map and
values, and after the values. Each framed type-23 record reports only an eight-bit structural-match
mask named `type23_map_gap_mask`; marker bits, field values, and payload bytes remain borrowed and
are never logged, copied, hashed, or retained. Type 1 and the deliberately partial type-4 path are
unchanged.

All eight grouped-map gap positions, arbitrary non-byte-aligned starts, truncation rejection,
deliberate ambiguity, and the unchanged type-1/type-4 paths passed the isolated synthetic suite.
The official CMake cross-build then completed from a fresh build directory. The candidate is
archived under
`backups/deployments/tribute-hall-sense-type23-grouped-map-gap-20260816/`; the clean-build and
archived DLL are byte-identical at SHA-256
`e003dd458f361472f1efad56586e428d57e94a7f040a976bc8a215d0c4e45902`. Its immediate rollback is
the successful cache-35 inline-gap build at SHA-256
`60b064681df09488aeb5472af797699faced62243eb67f0d984201d8b4a7a25a`. The archive also preserves
the pre-deployment format-35 cache and settings at their unchanged hashes above.

The successful Moon control trace is preserved as
`backups/deployments/tribute-hall-task9-origin-20260816/sunrise-moon-task9-origin.log` with SHA-256
`b3351bc22a27fede6037e069887e705a94e3aa23f0e71bc960eed8cf229f91db`.
The corrected full-mask control is preserved as
`backups/deployments/tribute-hall-full-task-masks-20260816/sunrise-moon-full-task-masks.log` with
SHA-256 `bb4d007ac459200b3c89dbd9500a31e7b9fd740bc95b398606b385c10b826655`.
The paired Hall trace is preserved as
`backups/deployments/tribute-hall-full-task-masks-20260816/sunrise-hall-full-task-masks.log` with
SHA-256 `ddb7f3af0c64ab0e99e96159d4804e5b3f076e4c5fa4e2c269111538738e516f`.
The successful task-9 gate control is preserved as
`backups/deployments/tribute-hall-task9-gates-20260816/sunrise-moon-task9-gates.log` with SHA-256
`6b9815992d6c24271ab898df4c1f42dfda16b1a3f1c43fa4240da25e2776fdaf`.
The false-selector noise correction was deployed from DLL SHA-256
`d9e24561daaa7af7936fcd30643414691611957be1eb96a46bd117ae07cbea39`; its rollback set is
`backups/deployments/tribute-hall-task9-gates-v2-20260816/`. The cache and settings were not
changed by that deployment.
The natural Hall-node trace is preserved as
`backups/deployments/tribute-hall-task9-selector3-20260816/sunrise-hall-natural-node.log` with
SHA-256 `40e2c88f28c3564910c42bb6a4ebc3eba442f581a97da7c7bd781e188555a45e`.
The temporary selector experiment has DLL SHA-256
`5fbbb5e12f43da5ccf0197388418f88931c35e035c4915755aae5e31a7ce19ac`; its complete rollback
set and exact experimental source are in
`backups/deployments/tribute-hall-task9-selector3-20260816/`.
Its completed Hall trace is preserved there as `sunrise-hall-selector3.log` with SHA-256
`fa160c4bc6f7cefed08d634a997cabf24cbc7a1e5ab8b6c79c921ea7c9122c3b`.
The first bounded task-9 completion experiment has DLL SHA-256
`d2adeade44e7bab4af1971c1f1e73761ff0a85747d3104576fd8a8eb23971278`. Its completed trace is
`backups/deployments/tribute-hall-task9-return3-20260816/sunrise-hall-return3.log` with SHA-256
`cb9e88652edd158e39af8e675f824ef5bc073cf6bf92fd37d073d3fe7f234f12`. The same folder retains
the exact source, rollback DLL, and the first `0x240` bytes of the unpacked callback; that binary has
SHA-256 `06c195618add8f59861c1918a3d4f256eea8f94e00bd90948e4ad8a105c9aa46`.
The follow-up build changes only the startup capture length to the callback's full `0x290` bytes;
its DLL SHA-256 is `009c90a8355b53c1c2291618b70569726a9e5a8f704b942db4370fa45655311a`.
The three-spawn black-screen trace is preserved as
`backups/deployments/tribute-hall-task9-return3-20260816/sunrise-hall-return3-multi-spawn-black.log`
with SHA-256 `2e06e484f7c18b95494800eaabcedd8a4d7f139d75f58de674982b26e998d1e1`.
The reconstructed full callback is preserved alongside it as
`task_nine_unpacked-full-0x290.bin`; it is exactly 656 bytes and has SHA-256
`7aa0aaa1f44b591f8c13390742271cb79080d58b524c31e937ca3d622f96eb42`.
The native pass-through helper-capture DLL has SHA-256
`95bce94dbdb23a30bd939c20959ee9e7cafe3e7a9217b65428b065fdd44a2ec4`; its exact source,
rollback DLL, and evidence are in
`backups/deployments/tribute-hall-task9-helper-capture-20260816/`. The orbit trace has SHA-256
`11809354e5883dabc12b070c3506ebe211cfc2878319dd414b7df959130a15cd`. The three reconstructed
`0x400`-byte binaries have SHA-256 values
`5a6b56bc09db5805aaf3ab13fa3fc52a3cd52363ce131ff38d09f37aabc51eaf` (object-ID source),
`cee3d32f65eb0115355f0767f6a5f3245d49ac6616451494cf7785b0b81a6930` (selector), and
`6aaa0e09bd0cd3549bc2bd4d0efab60018d91af072f19af8f04d2605a4f8e215` (object lookup).

The Hall publishes roster key `0x4786C0E0` from placed object `0x80FEB3DC`. Its 21 slots are:

| Slot type | Count | Component class | Sense schema | Auth schema | Current Sunrise body |
| ---: | ---: | --- | --- | --- | --- |
| 13 | 16 | `0x80804F2D` | `0x80804F2F` | `0x80804F30` | Player participation; one body binds the selected character |
| 16 | 1 | `0x808099F1` | absent | `0x808099F9` | Package-owned configuration; neutral body implemented |
| 17 | 1 | `0x80809915` | absent | `0x8080991A` | Activity lifetime and optional arrival override; implemented |
| 18 | 1 | `0x80809917` | absent | `0x80809919` | Native-derived neutral body implemented in source |
| 35 | 1 | `0x808099BD` | absent | `0x808099BF` | Native-derived neutral body implemented in source |
| 41 | 1 | `0x80804EE6` | `0x80804EE8` | `0x80804EE9` | Queue state; neutral body implemented |

The scenario registry also contains the package-owned ambient Hall group at placed object
`0x815BA505`, key `0xF18B720F`. Its 146-slot layout is distinct from the 21-slot global activity
group:

| Slot type | Count | Component class | Sense schema | Auth schema | Published flags/body |
| ---: | ---: | --- | --- | --- | --- |
| 1 | 16 | `0x80809A3B` | `0x80807ECC` | `0x80807EC9` | sense + auth; neutral body implemented |
| 2 | 16 | `0x8080834E` | `0x80807DA2` | `0x80807DA1` | sense + auth; neutral body implemented |
| 4 | 47 | `0x80809927` | `0x8080992E` | `0x8080992F` | sense + auth; native-derived neutral body implemented |
| 5 | 1 | `0x80804F01` | absent | `0x80804F04` | auth; neutral body implemented |
| 23 | 20 | `0x80804F45` | `0x80804F47` | `0x80804F48` | sense + auth; neutral body implemented |
| 47 | 1 | no descriptor | absent | absent | local; no network block |
| 60 | 1 | no descriptor | absent | absent | local; no network block |
| 61 | 26 | no descriptor | absent | absent | local; no network block |
| 66 | 16 | `0x808094CF` | absent | absent | local; no network block |
| 70 | 1 | `0x808094EE` | `0x808094F0` | `0x808094F1` | sense + auth; neutral body implemented |
| 72 | 1 | no descriptor | absent | absent | local; no network block |

The source-only Hall supplement preserves the original general-purpose roster filter. It admits
this additional group only when all of the following still match: destination
`trophy_hall_freeroam`, registry key `0xF18B720F`, total count 146, and every per-type count in the
table above. The exact presence flags are then applied without walking the group's 120 descriptor
handles during cache extraction. This keeps the operation within the ordinary extraction budget;
any package-layout drift drops the supplemental group instead of guessing.

Only the ambient key receives this allowance. The other secondary registry groups, including the
Bad Juju mission groups, remain excluded. The encoder emits native-derived neutral bodies for slot
types 1, 2, 4, 5, 23, and 70. Type 4 was added only after tracing its kind-`0x0D` and
kind-`0x22` runtime decoders; it no longer relies on a guessed payload.

The first live test proved cache extraction and wire framing, but not a usable ambient state. The
Client accepted repeated two-group updates containing all 167 objects, then completed the Hall
region's precache and instantiation passes. Immediately after opcode 2100 reported region/key
`0xF18B720F`, the world-controller job remained stalled in `activity:initial_slice_set_loading` and
hit the diagnostic assertion cap of 200. The process stayed alive but made no further log progress.
The failed DLL, cache, and log were preserved, and the installed DLL/cache were restored to their
previous working hashes. The supplemental source remains uncommitted until the required auth state
is understood.

The second live test added the five fully decoded neutral bodies for slot types 1, 2, 5, 23, and
70. The steady roster message grew from 2,158 to 2,633 bytes, and the Client continued accepting
all 167 objects without a framing or decoder error. It again completed Hall precache and
instantiation, received opcode 2100 for `0xF18B720F`, then remained in
`activity:initial_slice_set_loading` until the five-second prologue-filler timeout. This proves the
five bodies are wire-valid but not sufficient to release the Hall. Type 4 is the only ambient slot
type that both requests auth state and still lacks a body; its 47 instances are therefore the next
bounded research target. The failed DLL, cache, and log were preserved separately, and no live
behavior from this test is committed as working functionality.

The third live test added the exact 253-bit type-4 body to all 47 instances. The steady roster
message grew from 2,633 to 4,120 bytes: the 1,487-byte increase exactly matches 47 times 253 bits
after whole-message byte rounding. The Client accepted every update with no framing or decoder
error. It completed the Hall region's precache and instantiation passes and reported opcode 2100
for `0xF18B720F`, but it again hit the five-second prologue-filler timeout and remained in
`activity:initial_slice_set_loading` while normal keepalives continued. This proves the type-4
body is wire-valid but also proves that complete neutral auth-body coverage does not release the
Hall. The next target is the separate activity/prologue readiness state, not additional guessed
type-4 bits.

The tested DLL (`3baa9a6f0200215b71fc9e33406ece858e2b9d47d3e6f7a74b065d3f7317ed53`)
and live log (`e51dfa3aead997c632c843098365595f138092653bcf6bf80b0383f4a2404e9b`)
are preserved under `backups/deployments/tribute-hall-type4-v3-20260816`. The installed DLL was
restored to the prior hash
`3fe02b5d94c71c4b1784656f4ff824d4e37712fab6db535da381c40926552e81`; the cache remained
unchanged at `bead2c68e79cc0facf93527c9d29c190dfe9e552d27f37f041444f39a7966a27`.

A bounded fresh-boot diagnostic recorded the first component/sense/auth tuple for every observed
slot type and at most one conflict per type. It found no conflicts anywhere in the scanned package
set, so the type-18 and type-35 schemas above are not Hall-only guesses. All 21 Hall slots declare
auth state; the 16 type-13 slots and the one type-41 slot also declare sense state. The diagnostic
was removed after capture.

Read-only inspection of the running Shadowkeep schema registry established the missing auth layouts.
Native field descriptors have a 40-byte stride; field kind `0x100` marks an optional field. This
rule independently reproduces type 16's existing seven-bit neutral body because all seven of its
top-level fields are optional and absent.

| Auth schema | Native field order | Exact width | Neutral wire values |
| --- | --- | ---: | --- |
| `0x80807EC9` | 18 optional fields, biased 2-bit value, biased 3-bit value, optional u32 | 24 bits | optionals absent; biased values one |
| `0x80807DA1` | optional i32, biased 2-bit value, biased 3-bit value, bool, four optional records | 11 bits | optionals absent; biased values one; bool zero |
| `0x80809C42` | u32, biased 7-bit value, signed i16 | 55 bits | zero, one, signed-zero bias `0x8000` |
| `0x80804F04` | two u64s, u8, nested u32 + `0x80809C42`, `0x80809C42` | 278 bits | unsigned values zero; nested biased values as above |
| `0x80804F48` | three u32/signed-i16/bool tuples | 147 bits | u32 and bool zero; i16 bias `0x8000` |
| `0x808094F1` | u5, optional record, signed i16, optional record | 23 bits | u5 zero; optionals absent; i16 bias `0x8000` |
| `0x8080992F` | three signed i32s, two bools, `0x80809C42`, kind `0x0D`, bool, `0x80809AEA` | 253 bits | signed-zero biases; bools zero; neutral nested record; raw quaternion xyz zero; unbiased u2 zero; polymorphic tail absent |
| `0x808099C4` | bool, five unsigned 64-bit values, unsigned 32-bit value | 353 bits | all zero |
| `0x80809919` | `0x808099C4`, bool, signed 32-bit value | 386 bits | shared record zero, bool zero, signed-zero bias `0x80000000` |
| `0x808099BF` | two bools, two biased 2-bit values, `0x808099C4` | 359 bits | bools zero, 2-bit values one, shared record zero |

The runtime field-kind dispatch table proves the two formerly unresolved type-4 encodings.
Kind `0x0D` checks a global compression-mode byte; that byte is zero in this Shadowkeep runtime,
so the decoder reads three raw 32-bit float values for quaternion x/y/z and synthesizes w=1.
Kind `0x22` first decodes schema `0x80800046`, whose sole kind-`0x17` field is a presence bit:
zero stores `-1`, after which kind `0x22` omits its second, polymorphic-object decode. The
neutral kind-`0x22` tail is therefore exactly one clear bit. Descriptor inspection also proves
that `0x80809AEA`'s preceding two-bit value has bias zero, so its neutral wire value is zero.

The source now emits these exact neutral bodies through the general slot-type encoder. It does not
add a Hall-specific client hook. All six ambient auth bodies, including the 253-bit type-4 body,
passed bounded live wire tests. The complete ambient-Hall change remains experimental and
uncommitted because valid neutral roster state still does not release the Hall's prologue gate.

## Complete observed slot inventory

Zero-valued slots are omitted. `n/a` means the ordinary table-array descriptor at byte 8 did not
resolve.

| Slot | Root tag | Table class | Array | Rows | Element class | Identity |
| ---: | --- | --- | :---: | ---: | --- | --- |
| 1 | `0x81327D10` | `0x808075CE` | yes | 122 | `0x80807521` | Unknown |
| 2 | `0x81327D0A` | `0x808076FE` | no | n/a | n/a | Unknown |
| 3 | `0x81327D04` | `0x808075CA` | yes | 26 | `0x80807510` | Unknown |
| 4 | `0x81327CF0` | `0x808076F0` | yes | 1,170 | `0x808076FC` | Unknown |
| 5 | `0x81327CE8` | `0x80807773` | yes | 54 | `0x80807778` | Unknown |
| 6 | `0x81327CE7` | `0x808075C6` | yes | 211 | `0x80807460` | Unknown |
| 9 | `0x81327CE6` | `0x80803F49` | no | n/a | n/a | Unknown |
| 10 | `0x81327CE5` | `0x8080792F` | yes | 4 | `0x80807933` | Unknown |
| 11 | `0x81327CE4` | `0x808075C0` | no | n/a | n/a | Investment constants |
| 12 | `0x81327CE3` | `0x808075BE` | yes | 3 | `0x808074FA` | Unknown |
| 13 | `0x8132068A` | `0x80807C32` | yes | 2 | `0x80807C38` | Unknown |
| 14 | `0x81320689` | `0x80807C2A` | yes | 3 | `0x80807C30` | Unknown |
| 15 | `0x81327CE2` | `0x808075BA` | yes | 9 | `0x808074E8` | Unknown |
| 16 | `0x81327CE1` | `0x80807C16` | yes | 6 | `0x80807C1C` | Unknown |
| 17 | `0x81327CE0` | `0x80807C62` | no | n/a | n/a | Inventory buckets |
| 18 | `0x81327CDF` | `0x808075B5` | no | n/a | n/a | Unknown |
| 19 | `0x81327CDE` | `0x8080306D` | yes | 5,181 | `0x80803475` | Collectibles |
| 20 | `0x81327CDD` | `0x80804F13` | no | n/a | n/a | Unknown |
| 21 | `0x81327CDC` | `0x80807C50` | yes | 59 | `0x80807C5C` | Unknown |
| 22 | `0x81320681` | `0x808075B0` | no | n/a | n/a | Unknown |
| 23 | `0x81320680` | `0x808076D9` | no | n/a | n/a | Unknown |
| 24 | `0x81327CDB` | `0x808076D5` | yes | 4 | `0x808076EB` | Unknown |
| 25 | `0x81327CDA` | `0x80807768` | yes | 48 | `0x8080776C` | Unknown |
| 26 | `0x8132067D` | `0x80807BBB` | yes | 66 | `0x80807BC0` | Unknown |
| 27 | `0x8132067C` | `0x80807BC2` | yes | 7 | `0x80807BC8` | Unknown |
| 28 | `0x81327CD9` | `0x808075A8` | yes | 927 | `0x80807459` | Unknown |
| 29 | `0x81327CD8` | `0x80807A30` | yes | 953 | `0x80807A35` | Unknown |
| 30 | `0x81327CD7` | `0x80807A2B` | yes | 489 | `0x80807A2F` | Unknown |
| 31 | `0x81320678` | `0x808075A4` | yes | 27 | `0x808074D7` | Unknown |
| 32 | `0x81320677` | `0x808075A1` | yes | 5 | `0x808074D2` | Unknown |
| 33 | `0x81327CD6` | `0x80807D1E` | yes | 19 | `0x80807D23` | Unknown |
| 35 | `0x81327CD5` | `0x8080759B` | yes | 47 | `0x808074CE` | Unknown |
| 36 | `0x81327CD4` | `0x80807C9B` | no | n/a | n/a | Unknown |
| 37 | `0x81327CD3` | `0x80807A18` | yes | 3 | `0x80807A1E` | Unknown |
| 38 | `0x81320670` | `0x80807A0E` | yes | 1 | `0x80807A17` | Unknown |
| 40 | `0x81327CD2` | `0x808078A6` | yes | 5 | `0x808078AC` | Unknown |
| 41 | `0x81327CD1` | `0x808079F8` | yes | 344 | `0x80807A01` | Unknown |
| 42 | `0x81327CD0` | `0x80807A02` | yes | 400 | `0x80807A08` | Unknown |
| 43 | `0x81327CCF` | `0x8080789C` | yes | 36 | `0x808078A2` | Unknown |
| 44 | `0x81327CCE` | `0x80807C6C` | yes | 52 | `0x80807C70` | Unknown |
| 45 | `0x8132066A` | `0x80802C83` | yes | 6 | `0x80802C8A` | Unknown |
| 46 | `0x81327CCD` | `0x80802C82` | no | n/a | n/a | Unknown |
| 47 | `0x81327CCC` | `0x8080294D` | no | n/a | n/a | Unknown |
| 48 | `0x81327CCB` | `0x80807BE4` | yes | 15,424 | `0x80807BE8` | Inventory items |
| 49 | `0x813191F4` | `0x808077F6` | yes | 7 | `0x808077FB` | Unknown |
| 50 | `0x8131934A` | `0x80807BD9` | yes | 700 | `0x80807BE3` | Unknown |
| 51 | `0x81319349` | `0x80802CB6` | yes | 607 | `0x80802DFF` | Plug sets |
| 52 | `0x81319348` | `0x808077EE` | yes | 1,425 | `0x808077F4` | Unknown |
| 53 | `0x81319347` | `0x80807583` | no | n/a | n/a | Unknown |
| 54 | `0x81319346` | `0x80807581` | yes | 12 | `0x808074BF` | Unknown |
| 55 | `0x81319345` | `0x80805408` | yes | 215 | `0x808055B8` | Unknown |
| 56 | `0x813191EE` | `0x8080757E` | yes | 3 | `0x808074DA` | Unknown |
| 57 | `0x813191ED` | `0x8080757C` | yes | 3 | `0x808074DD` | Unknown |
| 58 | `0x81319344` | `0x8080775B` | yes | 8,483 | `0x8080775F` | Unknown |
| 59 | `0x81319343` | `0x80807879` | yes | 1 | `0x8080787C` | Unknown |
| 60 | `0x81319342` | `0x80807576` | yes | 38 | `0x808074B9` | Unknown |
| 61 | `0x81319341` | `0x80807574` | yes | 358 | `0x808074B5` | Unknown |
| 62 | `0x81319340` | `0x8080385E` | yes | 6 | `0x8080386A` | Unknown |
| 63 | `0x8131933F` | `0x80803041` | yes | 924 | `0x80803056` | Unknown |
| 64 | `0x8131933E` | `0x80807571` | yes | 2 | `0x808074A3` | Unknown |
| 65 | `0x813191E5` | `0x80807D6B` | yes | 82 | `0x80807D71` | Unknown |
| 66 | `0x8131914C` | `0x80807C0B` | yes | 1 | `0x80807C11` | Unknown |
| 67 | `0x8131933D` | `0x80807797` | yes | 16 | `0x80807801` | Unknown |
| 68 | `0x8131933C` | `0x80807CD9` | yes | 88 | `0x80807CDD` | Progressions |
| 69 | `0x8131933B` | `0x80807CCA` | yes | 33 | `0x80807CD7` | Unknown |
| 70 | `0x81319149` | `0x80807CC1` | yes | 12 | `0x80807CC7` | Unknown |
| 71 | `0x8131933A` | `0x80807AE0` | yes | 44 | `0x80807AE6` | Unknown |
| 72 | `0x81319339` | `0x80807567` | yes | 2,242 | `0x80807452` | Unknown |
| 73 | `0x81319338` | `0x80802F0E` | yes | 66 | `0x80802F07` | Unknown |
| 74 | `0x81319145` | `0x808077D1` | no | n/a | n/a | Unknown |
| 75 | `0x81319337` | `0x80804193` | no | n/a | n/a | Unknown |
| 77 | `0x81319143` | `0x80807561` | no | n/a | n/a | Unknown |
| 78 | `0x81319336` | `0x80807B62` | yes | 16 | `0x80807B70` | Unknown |
| 87 | `0x81327D16` | `0x8080769F` | no | n/a | n/a | Unknown |
| 88 | `0x81319335` | `0x80807553` | yes | 979 | `0x8080748C` | Unknown |
| 89 | `0x81319334` | `0x80803145` | yes | 12 | `0x80803142` | Unknown |
| 90 | `0x81319333` | `0x80802C2E` | yes | 5 | `0x80802C15` | Unknown |
| 91 | `0x81319332` | `0x80802C78` | no | n/a | n/a | Unknown |
| 92 | `0x81319331` | `0x80802C2B` | no | n/a | n/a | Unknown |
| 93 | `0x81319330` | `0x80807AC3` | yes | 40 | `0x80807AC9` | Unknown |
| 94 | `0x8131932F` | `0x80807AB5` | yes | 765 | `0x80807ABB` | Unknown |
| 95 | `0x8131932E` | `0x80807D05` | yes | 61 | `0x80807D09` | Unknown |
| 96 | `0x8131932D` | `0x80807ACE` | yes | 229 | `0x80807AD4` | Material requirement sets |
| 97 | `0x8131932C` | `0x80807A78` | yes | 14 | `0x80807A7E` | Socket-entry lists |
| 98 | `0x8131932B` | `0x80802C3E` | yes | 75 | `0x80802C45` | Unknown |
| 99 | `0x8131932A` | `0x80802C3C` | yes | 9 | `0x80802C42` | Unknown |
| 100 | `0x81319135` | `0x80807548` | no | n/a | n/a | Unknown |
| 101 | `0x81319329` | `0x80807546` | yes | 4,301 | `0x80807484` | Unknown |
| 102 | `0x81319133` | `0x80807A6B` | yes | 27 | `0x80807A71` | Unknown |
| 103 | `0x81319132` | `0x80807A66` | yes | 16 | `0x80807A6A` | Unknown |
| 104 | `0x81319328` | `0x80807542` | yes | 12,506 | `0x80807480` | Candidate |
| 105 | `0x81319130` | `0x80807AAF` | yes | 52 | `0x80807AB4` | Unknown |
| 106 | `0x81319327` | `0x80807AAA` | yes | 2,481 | `0x80807AAE` | Unknown |
| 107 | `0x81319326` | `0x80807A5F` | yes | 403 | `0x80807A65` | Unknown |
| 108 | `0x81319325` | `0x8080753C` | yes | 198 | `0x80807438` | Unknown |
| 109 | `0x81319324` | `0x80807C49` | yes | 6,541 | `0x80807C4F` | Candidate |
| 110 | `0x81319323` | `0x80807539` | yes | 2,272 | `0x80807476` | Unknown |
| 111 | `0x81319322` | `0x80807D36` | yes | 11,923 | `0x80807D48` | Candidate |
| 112 | `0x81319321` | `0x80807D49` | yes | 21,613 | `0x80807D4F` | Candidate |
| 113 | `0x81319320` | `0x80807C80` | yes | 5,867 | `0x80807C8D` | Candidate |
| 114 | `0x8131931F` | `0x80807C92` | yes | 13,231 | `0x80807C96` | Candidate |
| 115 | `0x8131931E` | `0x80807533` | yes | 62 | `0x8080747C` | Unknown |
| 116 | `0x8131931D` | `0x8080784A` | yes | 511 | `0x8080784E` | Unknown |
| 117 | `0x81327D1C` | `0x80807981` | no | n/a | n/a | Unknown |
| 118 | `0x81320712` | `0x80807CA4` | no | n/a | n/a | Unknown |

## Maintenance rule

Treat the slot numbers and native class ids as snapshot-specific until verified against a newer
package build. When a candidate becomes confirmed, record the parser or runtime correlation that
proved it; do not promote it based solely on a community manifest label.
