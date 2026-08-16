# Shadowkeep subclass and ability research

This note separates confirmed runtime and installed-cache evidence from hypotheses. It records
build-derived identifiers rather than assigning live-era names where the package data has not yet
proved them.

## 2026-08-16 Phoenix Dive selection

The closed Hall run archived at
`backups/deployments/tribute-hall-sobject-origin-v3-20260816-224731/site185-and-phoenix-dive.log`
captured one Warlock subclass selection:

| Field | Observed value |
| --- | --- |
| Web-service opcode | `801` |
| Subclass item definition hash | `0x6CFBA3AB` (`1828430763`) |
| Installed definition index | `1195` |
| Socket-entry list | `9` |
| Requested and selected entry | `15` |
| Competition group | `3` |
| Canonical character field | `melee` |
| Response and commit | accepted; family 4, family 0, and family 3 published; transaction committed |

The click is therefore not rejected. Entry `15` selects the four-node tree through its shared plug
source, while the character summary continues to carry the primary super entry. The missing
Phoenix Dive behavior is downstream of transaction acceptance.

### Cached selector comparison

The exact active selection is list `9`, movement `5`, grenade `7`, primary super `10`, tree
entry `15`, and class ability `2`. Cache format 35 contains exactly one matching ability row.
Comparing all three tree representatives while holding every other selection fixed gives:

| Tree entry | Selector mask | Bucket 8 kind | Bucket 8 selector | Bucket 8 hashes |
| ---: | ---: | ---: | ---: | --- |
| `11` | `0x091F` | `24` | `13` | `0xB5C4E5C9` |
| `15` | `0x081F` | absent (`255`) | `0` | none |
| `21` | `0x091F` | `24` | `22` | `0xF6F1C6C2` |

The entry-15 row is uniquely missing supplemental selector bucket 8. This strongly explains why
the tree equips and publishes successfully while its air-move behavior is unavailable. It does
not yet prove which raw package record should populate that bucket.

### Clean next step

Inspect the raw ability-pool records for list 9 entries 15 through 18 during package extraction,
including record kind, category, destination, and definition hash. The fix belongs in the generic
`assign_active_destinations` extraction path if those records describe the missing lane. Do not
hardcode Phoenix Dive, Warlock, entry 15, or bucket 8 unless the installed package proves there is
no general authored relationship to follow.

