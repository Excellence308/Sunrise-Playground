7117, 3904, 4913 are all tied to entity spawning from what i can glean

## 2026-08-16 upstream-6f54354 Hall run

This run used research build `57a3e8a` on top of upstream `6f54354`. The complete run is archived
at:

`Sunrise/backups/tribute-hall-upstream-6f54354-entity-errors-20260816-215825`

### Activity selection

| Field | Observed value |
| --- | --- |
| Activity index | `143` |
| Destination | `trophy_hall_freeroam` |
| Bubble hash | `0x811C9DC5` |
| Spawn-set hash | `0x811C9DC5` |
| Initial slice | `PRV00.00` |
| Initial slice hash | `0xF18B720F` |
| Loading roster | 1 group, 21 objects |
| In-world roster | 2 groups, 167 objects |

The world transition completed through steps 36, 37, and 38. This establishes that activity,
bubble, spawn-set, and initial-slice selection reached the intended Hall. Those selectors are not
the present blocker.

### Entity creation evidence

| Observation | Count or timing | What it establishes |
| --- | --- | --- |
| `failed to create 'sobject' entity` | 138 | Static/simulation objects are requested but cannot be instantiated. |
| First `sobject` failure | Immediately after opcode 2100 selected `0xF18B720F` | Failure begins as the initial slice becomes active. |
| Full two-group roster | About 11 seconds after the first `sobject` failures | The earliest `sobject` failures do not require the later 167-object roster expansion. |
| `failed to create 'player_broadcast' entity` | 89,771 | The local-player simulation entity is also retrying continuously; keep this separate from Hall props. |

The three captured sense payloads for group `0xF18B720F` contained 50, 10, and 12 parsed objects.
Their diagnostic classification was:

| Payload | Framed | Exact | Partial | Mismatch | Unframed tail |
| --- | ---: | ---: | ---: | ---: | ---: |
| ordinal 4 | 49 | 5 | 28 | 16 | 1 |
| ordinal 5 | 9 | 4 | 0 | 5 | 1 |
| ordinal 6 | 11 | 5 | 0 | 6 | 1 |

Type 1 records remain schema mismatches, type 4 records are only partially understood as
`partial_kind22`, and type 23 records match the ambiguous grouped form with map-gap mask `0x0A`.
The next investigation should therefore stay on the entity/state record schemas and the lifecycle
needed to instantiate `sobject`; it should not return to guessing activity, bubble, slice, or spawn
selectors without contradictory evidence.

## Runtime-object census lead

A later Discord post described a global Tiger object array, generic handle-table resolution,
per-class reflected field offsets, and a path to Havok rigid bodies. Its signatures do not match
the supported Shadowkeep executable, but the object-census and reflection concepts may help
separate missing objects from failed class/component activation. The build-specific assessment,
existing Sunrise overlap, correctness defects, and safe validation plan are preserved in
[`runtime-object-reflection-reference.md`](runtime-object-reflection-reference.md).
