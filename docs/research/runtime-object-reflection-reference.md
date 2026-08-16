# Runtime object and reflection lead

This note preserves two Discord code samples supplied on 2026-08-16. They were presented as a
way to enumerate every live Tiger object, resolve an object's class through the global handle
tables, enumerate reflected class fields by name hash, and eventually reach its Havok rigid body.
The post did not identify its authoring branch, Destiny build, executable hash, or validation
method. Treat it as an unverified research lead, not as a supported Sunrise layout.

## Assessment

| Part of the lead | Value to Sunrise | Current status |
| --- | --- | --- |
| Global `TigerArray` object census | High diagnostic value | Concept is new to this branch; posted signature does not match the supported executable. |
| `GetComponent` handle lookup | Little new implementation value | Sunrise already implements the same table layout and selector math with bounded reads and overflow checks. |
| Per-class hash-to-field-offset table | Medium-to-high diagnostic value | Potentially useful, but its layout, hash namespace, and descriptor semantics are unverified. |
| Physics-component lookup | Low value for Tribute Hall spawning | Could identify components on objects that already exist; it cannot instantiate missing authoritative objects. |
| Havok rigid-body pointer chase | Low immediate value | Sunrise already reaches the player rigid body through Havok simulation islands without this unverified pointer chain. |

The useful near-term outcome would be a **read-only object census**. Comparing class tags and object
types between a working destination and the Tribute Hall could distinguish these cases:

1. an expected object never enters the client object array;
2. an object exists but its class handle cannot be resolved;
3. a class resolves but lacks an expected reflected component; or
4. the object and component exist but a later lifecycle step does not activate them.

This does not recover the missing server-authored entity/state messages by itself. It observes
objects that the client successfully created after world state arrived.

## Validation against the supported executable

The read-only comparison used repository commit `b2a57a2469041d615bbae8afe1dd730b74c7d96d` and:

| Artifact | SHA-256 |
| --- | --- |
| `destiny2.exe` | `81964380664e7fcee3c620085a157fdeaf91fefacf7214907820f188bbeb4ced` |

Neither posted signature occurs in that executable as written:

| Claimed target | Posted signature | Exact matches | Prefix-only matches |
| --- | --- | ---: | ---: |
| global object array | `4C 03 15 ? ? ? ? BA` | 0 | 3 for `4C 03 15` |
| global handle table | `4C 8B 0D ? ? ? ? 8B 48` | 0 | 3 for `4C 8B 0D` |

The three prefix-only hits in each case have unrelated following bytes. They are not usable target
matches. This establishes only that the supplied byte patterns do not identify these globals in
our installed Shadowkeep executable; it does not disprove the underlying object-array design on a
different build.

The call `Scan(pattern, 7)` is also ambiguous without the author's scanner implementation. Both
instructions shown use a signed 32-bit RIP-relative displacement beginning at byte `+3`; the
absolute target is `instruction + 7 + displacement`. Sunrise already performs that calculation
through its checked relative-target helper and requires a unique image match.

## What Sunrise already has

The posted `HandleBucket` matches Sunrise's
`content::handles::layout::TableDescriptor` field for field:

| Offset | Discord name | Sunrise name |
| ---: | --- | --- |
| `+0x00` | `pad0` | `opaque00` |
| `+0x08` | `entries` | `recordArray` |
| `+0x10..+0x2F` | `pad1` | `opaque16` |
| `+0x30` | `stride` | `recordStride` |
| `+0x34` | `fixupMask` | `correctionMask` |
| `+0x38..+0x3F` | `pad2` | `opaque56` |

Sunrise's resolver also performs the same 13-bit record selection, descriptor-table selection, and
`record - (fixup & mask)` correction. The local implementation adds bounded reads, pointer and
stride checks, checked address arithmetic, explicit sign extension, and descriptor-table bounds.
It deliberately accepts the loaded content/schema table namespace beginning at descriptor 1024.

The Discord resolver has no such lower bound and may be intended for generic runtime class handles.
If that difference proves necessary, add a separately bounded generic resolver or factor a shared
primitive. Do not silently weaken the existing content-handle contract.

The Havok note has one independently confirmed overlap: current Sunrise noclip uses rigid-body
position `+0x1C0`. It uses velocity `+0x230`, not merely "right under" position, and finds the
player rigid body through simulation islands plus its motion vtable. The Discord-only component
offset `+0x1E0`, handler-to-body pointer `+0x20`, object/class layouts, and encrypted transform
layout remain unverified for this build.

## Reflected field table proposed by the post

The second sample describes a hash table rooted at `class metadata + 0x38`:

| Field | Claimed location or rule | Status |
| --- | --- | --- |
| bucket count | schema base `+0x08` | Unverified |
| buckets relative offset | schema base `+0x10` | Unverified |
| entries relative offset | schema base `+0x20` | Unverified |
| bucket array | schema base + buckets offset + `0x20` | Unverified |
| descriptor array | schema base + entries offset + `0x30` | Unverified |
| bucket entry | 8 bytes: 32-bit name hash, 16-bit descriptor index | Unverified/incomplete |
| field descriptor | `0x28` bytes; signed field offset at `+0x04` | Descriptor stride overlaps our metadata research; meaning of `+0x04` is not yet verified. |
| physics field hash | `0x7F6AD2A1` | Unverified; source calls it FNV-1a. |

Sunrise has separately observed `0x28`-byte native schema field descriptors while recovering
sensor-sense schemas, but those metadata roots and known descriptor words do not yet establish the
per-class hash table above. Sunrise's content names use 32-bit FNV-1, while other local identifiers
use FNV-1a. Therefore the statement that `0x7F6AD2A1` is FNV-1a may describe a distinct reflection
namespace; it must be tested rather than normalized to either existing hash routine.

## Correctness problems in the posted samples

These are transcription or implementation issues, independent of whether the reverse-engineered
layout is correct:

| Issue | Consequence | Required treatment |
| --- | --- | --- |
| A function returning `const unordered_map&` uses `return {};`. | Returns a dangling reference. | Return a pointer/optional reference, or return a stable static empty map. |
| `objects`, `sObjects`, and `tigerArray` are mixed. | Sample does not compile as written. | Use one validated snapshot object. |
| Several declarations lack semicolons; one end check is duplicated. | Sample does not compile as written. | Treat it as pseudocode only. |
| Direct pointer dereferences are used throughout. | A stale or malformed game pointer can fault the client. | Use Sunrise's bounded process-memory reader. |
| Array count, stride, mask, and address products are not bounded. | Corrupt/transient data can cause an arbitrary walk. | Check minimum layout, maximum count, multiplication, addition, and readable ranges. |
| Descriptor index has no upper bound. | Can index beyond the reflected descriptor array. | Recover and validate the descriptor count before reading. |
| A signed field offset is added to an unsigned address unchecked. | Negative or corrupt offsets can wrap. | Use checked signed address arithmetic and an owning-class extent when known. |
| `ObjectType` size and structure packing are unspecified. | Claimed `sObject` offsets may shift. | Use fixed-width storage and `static_assert` every known offset and size. |
| Filtering immediately to `Living` objects | Hides most Hall props, pedestals, dispensers, and interactables. | First collect a histogram of every type and class tag. |
| Cache access has no synchronization policy. | Concurrent discovery can race. | Build once at a controlled point or guard publication. |
| Writing transforms/rigid bodies is suggested before validation. | Adds crash and state-corruption risk without helping entity creation. | Keep the first implementation strictly read-only. |

The post's claim that this yields "everything" is too broad. At most it yields fields present in
the reflected table for a class; the author also notes that some fields are absent.

## Upstream-friendly implementation path

If this lead is pursued, keep it as an optional research layer over upstream:

1. Locate the supported-build object-array reference with a unique, wildcarded signature and the
   existing checked RIP-relative resolver. Do not ship either Discord signature unchanged.
2. Snapshot only the array header first. Bound count and stride, validate the complete readable
   range, and record only aggregate type/class-tag counts.
3. Reuse the existing handle descriptor layout. Factor a generic resolver only if captured class
   handles prove that the current content-table lower bound excludes them.
4. Validate the class metadata root and reflection bucket layout against at least two known classes.
   Establish the exact name spelling and hash algorithm for `0x7F6AD2A1` before naming it physics.
5. Compare the same census in a working Moon destination and the Tribute Hall. Correlate changes
   with the archived `failed to create 'sobject' entity` events and visible Hall pedestals/urn.
6. Add component summaries only after the census is stable. Leave Havok and transform writes out
   of the Tribute Hall diagnostic.

No runtime code or installed build was changed while recording this lead.
