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

### Live v1 result and corrected v2

The first live Hall load reached `activity:in_world` and emitted 112 exact
`failed to create 'sobject' entity` lines. Every one used site `188`; no
`ev=retail_origin stage=sobject_create_failure` event appeared because v1 intentionally required
site `193`. This confirms the diagnostic stayed inert when its full guard did not match. The
closed trace is preserved as `site188-missed-capture.log`, SHA-256
`c6938669073477980abe20f7170a1477d472470f87f6d40769d5c8cd53de2540`.

V2 changes only `kEntityFailureSite` from `193` to the live upstream-integrated value `188`; the
exact native message guard, one-shot behavior, captured fields, and read-only handling are
unchanged. An incremental official CMake build completed successfully with no Sunrise-source
warnings.

| V2 artifact | SHA-256 |
| --- | --- |
| deployed and archived site-188 diagnostic DLL | `db2f525b4e42ad8acb9931229a2a98380ae238925205f3f47931a5e228c6e40e` |
| immediate rollback site-193 DLL | `d6657794c476bf90db3346aaef39ea2b907fb102f69d2ac30c77c07a3d8b2aca` |
| unchanged build-data cache | `562d6d9974bc05b35ff3883d4fb30a369e7c7e654f052b15fd78fbe725470a95` |
| unchanged settings | `2a679c1e94ceba991dd6c51b747a83d1c4bd23bf835eadec783098f1ddaf7b7c` |

The v2 DLL, immediate rollback, corrected source, closed v1 trace, cache, and settings are
preserved under
`backups/deployments/tribute-hall-sobject-origin-v2-20260816-223757/`. One ordinary Hall load is
again sufficient; no in-world interaction is required.

### Live v2 result and exact-text v3

The next ordinary Hall load again reached `activity:in_world` and emitted 112 exact
`failed to create 'sobject' entity` lines, but every line used site `185`. V2 intentionally
required site `188`, so it emitted no origin event and remained inert.

The same process recorded the exact task-9 text under sites `49`, `76`, and `177`. Site `185`
also carried both `player_broadcast` and `sobject` failures. These observations establish that
the registered site value is not a stable executable address or safe matching key; it varies with
the runtime log origin/registration path. The closed trace is preserved as
`site185-and-phoenix-dive.log`, SHA-256
`4214df37dd5f5a5d201bb86a1d27ff009094a3c18e9f868298551aaa2a518f6c`.

V3 removes the expected-site parameter from both one-shot origin probes. Each probe now matches
the complete native text only, while the observed site remains in the emitted evidence. Null and
exception guards, caller and stack capture, and one-shot publication are unchanged. The official
CMake build completed successfully with no Sunrise-source warnings.

| V3 artifact | SHA-256 |
| --- | --- |
| deployed and archived exact-text diagnostic DLL | `daeb24dd51bf77f9e274a078281d21bdf1bd4f3bbcdb1df82629c2eea846dd0c` |
| immediate rollback site-188 DLL | `db2f525b4e42ad8acb9931229a2a98380ae238925205f3f47931a5e228c6e40e` |
| unchanged build-data cache | `562d6d9974bc05b35ff3883d4fb30a369e7c7e654f052b15fd78fbe725470a95` |
| unchanged settings | `2a679c1e94ceba991dd6c51b747a83d1c4bd23bf835eadec783098f1ddaf7b7c` |

The v3 DLL, immediate rollback, corrected and previous source, closed trace, cache, and settings are
preserved under
`backups/deployments/tribute-hall-sobject-origin-v3-20260816-224731/`. One ordinary Hall load is
sufficient; no in-world interaction is required.

## Live v3 result and native entity-creation boundary

The exact-text v3 probe succeeded on the next ordinary Hall load. The first matching line used
site `186` and produced this origin:

`caller_rva=0x16EE3F7 stack=0x16EE3F7,0xB428E3,0x3BD462,0x4D721C,0x3F860D,0x403295,0x56C72C,0x56DD82,0x575AAE,0x559F03,0x434A39,0x435733,0x3CD6FF,0x3C82EB`

The changing site values `188`, `185`, and `186` confirm that exact text is the stable match key.
The native caller stack is stable enough to identify the entity path.

The executable on disk is packed, so its bytes at these RVAs are not usable code. With the game
running, small ranges were copied read-only from the already decrypted main-image mapping through
`/proc/<pid>/mem`. No debugger was attached, no thread was suspended, and no game memory was
written. Those snapshots and the closed-run log are preserved under
`backups/deployments/tribute-hall-sobject-native-origin-20260816-231812/`.

### Resolved native path

| RVA | Role | Failure evidence |
| --- | --- | --- |
| `0x16EE3F2` | Emits the exact `failed to create 'sobject' entity` line | Downstream symptom after the returned output remains `-1` |
| `0x170F190` | Owns the top-level simulation-entity output | Calls handle allocation, then commit; commit failure resets the output to `-1` |
| `0x1711D10` | Claims one of `0x2000` simulation handles | Leaves the output at `-1` if no handle is available |
| `0x170B0F0` | Allocates buffers and commits the entity record | Returns false for record-pool, buffer-setup, or registration failure |
| `0x170B2B0` | Claims one of `0x400` entity records | Returns null when the record pool cannot supply a slot |
| `0x171C280` | Applies the final native registration mode | Returns false only when its required registration allocation fails |

The game therefore is not merely missing a Hall presentation flag. It requests the static
objects, enters the native simulation-entity creator, and receives a real `-1` result before the
error line is emitted. Forcing the final result would skip substantial handle, record, buffer,
and registration bookkeeping and is not a safe next experiment.

### Stage diagnostic

The next fork-local build adds five signature-resolved, read-only detours in one all-or-nothing
transaction. A miss disables only this optional trace and does not demote normal Sunrise
activation. Results are correlated per thread and capped at 256 failed creates per process.

| Logged reason | Proven boundary |
| --- | --- |
| `handle_pool` | The `0x2000` simulation-handle allocator returned `-1` |
| `record_pool` | A handle existed, but the `0x400` record allocator returned null |
| `buffer_setup` | A record existed, but commit stopped before final registration |
| `registration` | Final native registration was reached and returned false |
| `commit_unknown` or `create_unknown` | The known nested stages succeeded but the enclosing routine still rejected the create |

The general game allocator is deliberately not hooked. That keeps the diagnostic off a hot,
unrelated path and preserves normal performance outside these entity-create calls. The official
CMake cross-build completed successfully with no Sunrise-source warnings. All five signatures
match their archived decrypted native bytes exactly. The candidate, exact v3 rollback, unchanged runtime state, documentation, and source are preserved under `backups/deployments/tribute-hall-sobject-stage-diagnostic-20260816-233147/`.

| Stage-diagnostic artifact | SHA-256 |
| --- | --- |
| deployed stage-diagnostic DLL | `9324aca2296ff1359686611383325c29a869dd98d5cb18184f1f752f7858154b` |
| unchanged build-data cache | `562d6d9974bc05b35ff3883d4fb30a369e7c7e654f052b15fd78fbe725470a95` |
| unchanged settings | `2a679c1e94ceba991dd6c51b747a83d1c4bd23bf835eadec783098f1ddaf7b7c` |
