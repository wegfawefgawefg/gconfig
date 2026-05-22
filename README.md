# gconfig

<p>
  <img src="assets/logo.svg" alt="gconfig logo" width="96" height="96">
</p>

`gconfig` is a small C++20 library for schema-backed config documents and named
document lists. It is intended for vendored game libraries and tools that need
human-editable config files without owning game policy.

The library stores typed values, validates them against caller-defined schemas,
reconciles old files with defaults, and reads/writes small S-expression files
through `gsexp`.

`gconfig` is not a settings framework. It does not own profiles, menus, save
systems, rendering, input, platform paths, or game policy. The host decides what
each document means.

## Target

- `gconfig::gconfig`: values, schemas, documents, document lists, reconciliation,
  and sexpr file IO.

## What It Does

- Typed values: bool, int, float, string, vec2.
- Schema defaults and validation.
- Schema-aware load helpers that reconcile immediately.
- Single config documents.
- Named/id document lists.
- Reconcile reports for missing defaults, wrong types, clamped values, and
  invalid options, preserved unknown keys, and dropped unknown keys.
- Safe reconciliation coercions for common old config shapes, such as `0/1`
  ints for bool fields and ints for float fields.
- Passive metadata for caller-owned menus or debug UI.

## What It Does Not Do

- No menu system.
- No SDL, ImGui, graphics, audio, or input backend.
- No user-profile policy.
- No game-save serialization.
- No scripting.

## Add To A Project

The intended integration path is vendored source with CMake `add_subdirectory`.
Put `gsexp` and `gconfig` somewhere under your project, for example:

```text
third_party/
  gsexp/
  gconfig/
```

Then wire them into CMake:

```cmake
set(GSEXP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GSEXP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/gsexp)

set(GCONFIG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GCONFIG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/gconfig)

target_link_libraries(my_game PRIVATE gconfig::gconfig)
```

For sibling development checkouts, `gconfig` defaults
`GCONFIG_GSEXP_SOURCE_DIR` to `../gsexp`. If your layout is different, set it
before adding `gconfig`:

```cmake
set(GCONFIG_GSEXP_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/gsexp" CACHE PATH "" FORCE)
add_subdirectory(third_party/gconfig)
```

## Basic Use

```cpp
gconfig::Schema schema;
schema.add_bool("video.vsync", true);
schema.add_float("audio.master_volume", 1.0f).set_range(0.0f, 1.0f, 0.01f);

gconfig::LoadDocumentResult loaded = gconfig::load_document_file("settings.lisp", schema);
gconfig::Document document = loaded.document;

bool vsync = document.get_bool("video.vsync", true);
document.set_float("audio.master_volume", 0.75f);
gconfig::save_document_file("settings.lisp", document, schema);
```

Use `DocumentList` when one file contains many named/id documents, such as user
profiles, settings profiles, save slots, or bind profiles.

```cpp
gconfig::LoadDocumentListResult profiles =
    gconfig::load_document_list_file("user_profiles.lisp", user_schema,
                                     gconfig::UnknownKeyMode::Preserve,
                                     "user_profiles", "profile");

gconfig::DocumentRecord* user = profiles.documents.find(42);
int last_binds = user ? user->document.get_int("last_binds_profile", -1) : -1;
```

The `root_name` and `record_name` arguments let callers read existing shapes
such as `(game_settings_list (settings ...))` or `(user_profiles (profile ...))`
without making `gconfig` own profile policy.

When saving with a schema, known values are written in schema order and unknown
values are written afterward in stable alphabetical order.

## File Format

Single documents are stored as key/value forms:

```lisp
(config
  (values
    (key "video.vsync" true)
    (key "audio.master_volume" 0.75)))
```

Document lists store named/id'd documents in one file:

```lisp
(user_profiles
  (profile
    (id 42)
    (name "GMan")
    (values
      (key "last_binds_profile" 1)
      (key "last_game_settings" 100))))
```

Existing shapes can be loaded by passing custom root and record names.

## Build

```sh
./scripts/build.sh
```

## Run Demo

```sh
./scripts/run.sh
```

See [docs/plan.md](docs/plan.md) for the extraction plan and boundaries.
