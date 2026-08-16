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

## 2026-08-16 sobject-creation origin diagnostic

The current upstream-integrated baseline reaches `activity:in_world` with the complete two-group,
167-object roster and 4,120-byte type-5 roster body. Its remaining Hall failure is later: the
Client reports 138 `failed to create 'sobject' entity` lines as region `0xF18B720F` becomes
active. The earlier roster-framing and prologue-loading failures are therefore not the current
boundary.

The next diagnostic reuses Sunrise's existing retail-log detour. On the first exact site-193 line

`networking:simulation:entity: failed to create 'sobject' entity`

it records the native caller RVA and up to 16 game-image stack-frame RVAs in one
`ev=retail_origin stage=sobject_create_failure` event. It then remains inert for the rest of the
process. It does not read or retain activity payloads, object memory, entity identifiers, or field
values; it does not alter the native call or its return. This is local research instrumentation,
not behavior specified by an upstream guide.

A clean official CMake cross-build completed successfully. The unchanged vendor code emitted its
usual three portability warnings; Sunrise source emitted no warnings.

| Artifact | SHA-256 |
| --- | --- |
| deployed and archived diagnostic DLL | `d6657794c476bf90db3346aaef39ea2b907fb102f69d2ac30c77c07a3d8b2aca` |
| immediate rollback DLL | `35ce5fbc3ff7f9f4cd97a0a8f8fc0b5719dc92b250a470ea5c6d8c5dd3e90576` |
| unchanged build-data cache | `562d6d9974bc05b35ff3883d4fb30a369e7c7e654f052b15fd78fbe725470a95` |
| unchanged settings | `2a679c1e94ceba991dd6c51b747a83d1c4bd23bf835eadec783098f1ddaf7b7c` |

The candidate, rollback DLL, pre-deployment log, cache, settings, and exact observer source are
preserved under
`backups/deployments/tribute-hall-sobject-origin-20260816-222714/`. The diagnostic is useful as
soon as one ordinary Hall load reaches the first `sobject` failure; no interaction with the empty
Hall is required.
