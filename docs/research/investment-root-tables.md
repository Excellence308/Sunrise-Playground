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

The Hall publishes roster key `0x4786C0E0` from placed object `0x80FEB3DC`. Its 21 slots are:

| Slot type | Count | Component class | Sense schema | Auth schema | Current Sunrise body |
| ---: | ---: | --- | --- | --- | --- |
| 13 | 16 | `0x80804F2D` | `0x80804F2F` | `0x80804F30` | Player participation; one body binds the selected character |
| 16 | 1 | `0x808099F1` | absent | `0x808099F9` | Package-owned configuration; neutral body implemented |
| 17 | 1 | `0x80809915` | absent | `0x8080991A` | Activity lifetime and optional arrival override; implemented |
| 18 | 1 | `0x80809917` | absent | `0x80809919` | Native-derived neutral body implemented in source |
| 35 | 1 | `0x808099BD` | absent | `0x808099BF` | Native-derived neutral body implemented in source |
| 41 | 1 | `0x80804EE6` | `0x80804EE8` | `0x80804EE9` | Queue state; neutral body implemented |

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
| `0x808099C4` | bool, five unsigned 64-bit values, unsigned 32-bit value | 353 bits | all zero |
| `0x80809919` | `0x808099C4`, bool, signed 32-bit value | 386 bits | shared record zero, bool zero, signed-zero bias `0x80000000` |
| `0x808099BF` | two bools, two biased 2-bit values, `0x808099C4` | 359 bits | bools zero, 2-bit values one, shared record zero |

The source now emits these exact neutral bodies through the general slot-type encoder. It does not
add a Hall-specific client hook. The build succeeds, but it remains source-only until the current
game session closes and the DLL can be deployed safely.

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
