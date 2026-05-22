# Gubsy-Style Example

This example shows the kind of data `gconfig` is meant to handle for Gubsy-like
projects.

The important idea is that `gconfig` does not know what a user profile, game
settings profile, save slot, or bind profile means. It only knows how to load,
save, validate, and reconcile named config documents. The game owns the policy
that connects those documents.

## Why This Is Not Just JSON With Extra Steps

A plain JSON loader gives the caller a tree. The caller still has to write:

- default insertion when a key is missing
- type checks and safe fallbacks
- range clamping
- option validation
- schema-ordered output
- old-file reconciliation
- document-list handling for profile collections
- diagnostics for preserved or dropped unknown keys
- UI/debug metadata such as labels, categories, order, ranges, and options

`gconfig` is useful when those behaviors matter. If a project only needs to read
one free-form data tree with no schema and no reconciliation, `gconfig` is the
wrong tool.

## Standard Relationship Model

For a Gubsy-style game shell, the common relationship looks like this:

```text
Global settings
  machine/install-wide document
  not owned by a user profile

User profiles
  one record per local player identity
  stores profile preferences and references to preferred shared documents

Input profiles
  shared document list
  any user profile can point at the same input profile

Game settings profiles
  usually shared presets
  any user profile can point at one, duplicate one, or make a personal one

Save slots
  owned by one user profile
  not shared across user profiles unless the game explicitly supports that
```

`gconfig` can store all of these as one `Document` plus several
`DocumentList`s. It does not enforce ownership rules. A game can enforce those
rules by storing ids in documents and resolving them in game code.

### Ownership Rules

- Global settings are loaded once and applied to the install or machine.
- User profiles are the root of player-local preferences.
- Input profiles are shared because multiple users may prefer the same keyboard
  or gamepad layout.
- Game settings profiles may be shared when they are treated as presets.
- Save slots are normally user-owned because save progress belongs to a player
  identity.
- Save metadata can live in `gconfig`; save payloads should live elsewhere.

### Reference Fields

A user profile can store ids that point into other document lists:

```lisp
(profile
  (id 42)
  (name "GMan")
  (values
    (key "profile.name" "GMan")
    (key "preferred_input_profile" 1)
    (key "preferred_game_settings" 100)
    (key "last_save_slot" 3)))
```

Those fields are just config values. The game decides what to do when a
referenced document is missing. Common repair policies are:

- choose the first available shared input profile
- create a default game settings profile
- clear `last_save_slot` if the save slot no longer exists
- warn the user before dropping a broken reference

## Example Files

### Global Settings

One document can store install-wide settings. These are not tied to a player.

```lisp
(global_settings
  (values
    (key "video.window_mode" "windowed")
    (key "video.render_resolution" "1920x1080")
    (key "video.vsync" true)
    (key "video.frame_cap" 60)
    (key "audio.master_volume" 0.80)
    (key "audio.music_volume" 0.65)
    (key "audio.sfx_volume" 1.00)
    (key "audio.output_device" "default")))
```

Typical use:

- Load once on startup.
- Reconcile with the global schema.
- Apply values to graphics/audio systems in game code.
- Save after settings menu edits.

### User Profiles

A user profile is a named document that points at the user's preferred config
documents.

```lisp
(user_profiles
  (profile
    (id 42)
    (name "GMan")
    (values
      (key "profile.name" "GMan")
      (key "preferred_input_profile" 1)
      (key "preferred_game_settings" 100)
      (key "last_save_slot" 3)))
  (profile
    (id 43)
    (name "Nees")
    (values
      (key "profile.name" "Nees")
      (key "preferred_input_profile" 2)
      (key "preferred_game_settings" 101)
      (key "last_save_slot" 7))))
```

Typical use:

- Load as a `DocumentList` with root `user_profiles` and record name `profile`.
- Find the active user by id.
- Read ids such as `preferred_input_profile`, `preferred_game_settings`, and
  `last_save_slot`.
- Use those ids to select documents from other lists.

`gconfig` does not enforce that a referenced document exists. The game decides
whether to create a replacement, warn the user, or fall back to defaults.

### Game Settings Profiles

Game settings profiles are reusable presets. A user can point at one, duplicate
one, or switch between them.

```lisp
(game_settings_list
  (settings
    (id 100)
    (name "Default Game Settings")
    (values
      (key "input.mouse_sensitivity" 1.25)
      (key "input.invert_y_axis" false)
      (key "input.vibration_strength" 0.80)
      (key "hud.scale" 1.00)
      (key "hud.opacity" 0.85)
      (key "hud.minimap_mode" "corner")
      (key "accessibility.camera_shake" true)))
  (settings
    (id 101)
    (name "Low Motion")
    (values
      (key "input.mouse_sensitivity" 0.90)
      (key "input.invert_y_axis" false)
      (key "input.vibration_strength" 0.30)
      (key "hud.scale" 1.10)
      (key "hud.opacity" 1.00)
      (key "hud.minimap_mode" "large")
      (key "accessibility.camera_shake" false))))
```

Typical use:

- Load as a `DocumentList` with root `game_settings_list` and record name
  `settings`.
- Reconcile every document against the game settings schema.
- Preserve unknown keys if mods or app-specific settings may exist.
- Drop unknown keys if the file must exactly match the current schema.

### Input Profiles

Simple input bindings can be modeled as a shared document list.

```lisp
(input_profiles
  (input_profile
    (id 1)
    (name "Keyboard")
    (values
      (key "bind.jump" "space")
      (key "bind.confirm" "enter")
      (key "bind.cancel" "escape")
      (key "bind.left" "a")
      (key "bind.right" "d")))
  (input_profile
    (id 2)
    (name "Gamepad")
    (values
      (key "bind.jump" "pad_a")
      (key "bind.confirm" "pad_a")
      (key "bind.cancel" "pad_b")
      (key "bind.left" "pad_left")
      (key "bind.right" "pad_right"))))
```

This works for simple key/value bindings. If a project's input system needs
multiple buttons per action, analog axes, chords, or device-specific structs, it
may be better to keep a custom binding file format and only use `gconfig` for
the user profile reference to the active binds profile.

### Save Slots

`gconfig` is not a save-game serializer, but it can store user-owned save-slot
metadata.

```lisp
(save_slots
  (slot
    (id 3)
    (name "Caves Run")
    (values
      (key "display_name" "Caves Run")
      (key "owner_user_profile" 42)
      (key "world" "caves")
      (key "level" 2)
      (key "play_seconds" 540)
      (key "payload_path" "saves/slot_3.bin")))
  (slot
    (id 7)
    (name "Daily")
    (values
      (key "display_name" "Daily")
      (key "owner_user_profile" 43)
      (key "world" "jungle")
      (key "level" 4)
      (key "play_seconds" 1220)
      (key "payload_path" "saves/slot_7.bin"))))
```

The actual serialized game state should live somewhere else. The config document
stores searchable/editable metadata and a path or id for the real save payload.

Typical use:

- Load all save metadata as a `DocumentList`.
- Filter slots where `owner_user_profile` matches the active user id.
- Use `last_save_slot` from the user profile to select the last-used owned slot.
- Store the real game-state payload in the path referenced by `payload_path`.

## Example Schemas

```cpp
gconfig::Schema global_schema;
global_schema.add_string("video.window_mode", "windowed")
    .set_label("Display Mode")
    .add_category("Video")
    .add_option("fullscreen", "Fullscreen")
    .add_option("borderless", "Borderless")
    .add_option("windowed", "Windowed");
global_schema.add_string("video.render_resolution", "1920x1080")
    .set_label("Render Resolution")
    .add_category("Video");
global_schema.add_bool("video.vsync", true)
    .set_label("V-Sync")
    .add_category("Video");
global_schema.add_int("video.frame_cap", 60)
    .set_label("Frame Cap")
    .add_category("Video")
    .set_range(0.0f, 240.0f, 1.0f);
global_schema.add_float("audio.master_volume", 1.0f)
    .set_label("Master Volume")
    .add_category("Audio")
    .set_range(0.0f, 1.0f, 0.01f);

gconfig::Schema user_schema;
user_schema.add_string("profile.name", "Player");
user_schema.add_int("preferred_input_profile", -1);
user_schema.add_int("preferred_game_settings", -1);
user_schema.add_int("last_save_slot", -1);

gconfig::Schema game_schema;
game_schema.add_float("input.mouse_sensitivity", 1.0f)
    .set_label("Mouse Sensitivity")
    .add_category("Controls")
    .set_range(0.1f, 10.0f, 0.1f);
game_schema.add_bool("input.invert_y_axis", false)
    .set_label("Invert Y-Axis")
    .add_category("Controls");
game_schema.add_float("hud.opacity", 1.0f)
    .set_label("HUD Opacity")
    .add_category("HUD")
    .set_range(0.0f, 1.0f, 0.01f);
game_schema.add_string("hud.minimap_mode", "corner")
    .set_label("Minimap")
    .add_category("HUD")
    .add_option("off", "Off")
    .add_option("corner", "Corner")
    .add_option("large", "Large");
```

## Example Load Flow

```cpp
gconfig::LoadDocumentResult global =
    gconfig::load_document_file("global_settings.lisp", global_schema,
                                gconfig::UnknownKeyMode::Preserve,
                                "global_settings");

gconfig::LoadDocumentListResult users =
    gconfig::load_document_list_file("user_profiles.lisp", user_schema,
                                     gconfig::UnknownKeyMode::Preserve,
                                     "user_profiles", "profile");

gconfig::LoadDocumentListResult games =
    gconfig::load_document_list_file("game_settings.lisp", game_schema,
                                     gconfig::UnknownKeyMode::Preserve,
                                     "game_settings_list", "settings");

gconfig::DocumentRecord* active_user = users.documents.find(42);
if (!active_user) {
    // Game policy: create a user, choose a default, or show an error.
}

int game_id = active_user->document.get_int("preferred_game_settings", -1);
int input_id = active_user->document.get_int("preferred_input_profile", -1);
gconfig::DocumentRecord* active_game = games.documents.find(game_id);
if (!active_game) {
    // Game policy: create a replacement settings profile or fall back.
}

bool vsync = global.document.get_bool("video.vsync", true);
float mouse_sensitivity =
    active_game ? active_game->document.get_float("input.mouse_sensitivity", 1.0f) : 1.0f;
```

## Example Reconciliation

Given an old file:

```lisp
(global_settings
  (values
    (key "video.vsync" 1)
    (key "audio.master_volume" 2.0)
    (key "old.debug.setting" true)))
```

Loading with the schema can:

- coerce `video.vsync` from `1` to `true`
- clamp `audio.master_volume` to `1.0`
- add missing schema defaults
- report `old.debug.setting` as preserved unknown, or drop it if requested

This is the main reason the library exists.
