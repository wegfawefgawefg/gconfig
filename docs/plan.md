# gconfig Plan

`gconfig` is a small C++20 library for schema-backed, human-editable config
documents and document lists.

It exists because Gubsy currently hand-rolls the same pieces in several places:
typed settings values, default reconciliation, config document lists, profile
references, sexpr read/write, and menu metadata.

## Goal

- Store typed config values.
- Define schemas with defaults, validation, and passive metadata.
- Reconcile old or partial documents with schemas.
- Load and save human-editable sexpr files through `gsexp`.
- Support single documents and lists of named/id documents.
- Make global settings, user profiles, game settings profiles, save-slot config,
  and binds profiles easy to model without hardcoding those concepts.

## Non-Goals

- No menu system.
- No ImGui or SDL dependency.
- No user account/profile policy in the core.
- No graphics/audio/input application logic.
- No game-save serializer.
- No scripting or config evaluation.
- No file search policy beyond direct paths passed by the caller.

## Core Concepts

- `Value`: bool, int, float, string, and vec2.
- `Schema`: fields keyed by string, with defaults and metadata.
- `Document`: one map of config values.
- `DocumentList`: many named/id documents in one file.
- `ReconcileReport`: changes made while bringing a document up to schema.
- Reconciliation safely coerces common legacy config shapes, including `0/1`
  ints for bools and ints for float fields.
- Schema-aware load helpers can load and reconcile in one call.
- Save helpers can write values in schema order, then unknown keys in stable
  alphabetical order.

## Gubsy Mapping

- `top_level_game_settings.lisp` becomes one `Document`.
- `game_settings.lisp` becomes a `DocumentList`.
- `user_profiles.lisp` can become a `DocumentList` where each document stores
  identity data and ids of the last-used settings/binds/save documents.
- `binds_profiles/default.lisp` may become a specialized `DocumentList`, or it
  may keep a custom parser if bindings need a richer shape.
- Existing Gubsy-style root and record names can be read by passing custom names,
  for example `game_settings_list/settings` or `user_profiles/profile`.

## First Implementation

- CMake target: `gconfig::gconfig`.
- Dependency: `gsexp::gsexp`.
- Build options match other `g-*` libraries.
- Provide a demo that models:
  - global settings
  - user profiles
  - game settings profiles
  - save-slot references
  - bind profiles
- Provide tests for:
  - typed values
  - schema reconciliation
  - single document round trip
  - document list round trip
  - Gubsy-shaped document list parsing

## Remaining Future Work

- Add schema version/migration hooks.
- Add list/table values only if a real consumer needs them.
- Migrate Gubsy settings code to this library once the API feels right.
