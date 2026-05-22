#include "gconfig/gconfig.hpp"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        std::exit(1);
    }
}

gconfig::Schema make_schema() {
    gconfig::Schema schema;
    schema.add_bool("video.vsync", true).set_label("V-Sync").add_category("Video");
    schema.add_float("audio.music_volume", 1.0f).set_range(0.0f, 1.0f, 0.01f);
    schema.add_string("video.window_mode", "windowed")
        .add_option("windowed", "Windowed")
        .add_option("fullscreen", "Fullscreen");
    schema.add_int("video.frame_cap", 60).set_range(0.0f, 240.0f, 1.0f);
    schema.add_vec2("ui.safe_area", gconfig::Vec2{0.0f, 0.0f});
    return schema;
}

void test_document_reconcile() {
    gconfig::Schema schema = make_schema();
    gconfig::Document doc;
    doc.set_int("video.vsync", 1);
    doc.set_int("audio.music_volume", 2);
    doc.set_string("video.window_mode", "bad");
    doc.set_int("video.frame_cap", 999);
    doc.set_int("unknown", 7);

    gconfig::ReconcileReport report = doc.reconcile(schema, gconfig::UnknownKeyMode::Drop);

    require(report.changed(), "reconcile reports changes");
    require(doc.get_bool("video.vsync", false), "0/1 int coerces to bool");
    require(doc.get_float("audio.music_volume", 0.0f) == 1.0f, "int coerces to float and clamps");
    require(doc.get_string("video.window_mode", "") == "windowed", "invalid option resets");
    require(doc.get_int("video.frame_cap", 0) == 240, "int range clamps");
    require(doc.get_vec2("ui.safe_area").x == 0.0f, "missing vec2 default added");
    require(!doc.contains("unknown"), "unknown key dropped");
}

void test_preserved_unknown_is_reported_but_not_changed() {
    gconfig::Schema schema;
    schema.add_bool("known", true);

    gconfig::Document doc;
    doc.set_bool("known", false);
    doc.set_int("unknown", 9);

    gconfig::ReconcileReport report = doc.reconcile(schema, gconfig::UnknownKeyMode::Preserve);

    require(!report.changed(), "preserved unknown does not dirty document");
    require(report.changes.size() == 1, "preserved unknown is reported");
    require(report.changes[0].kind == gconfig::ReconcileChangeKind::PreservedUnknown,
            "preserved unknown has specific kind");
    require(doc.contains("unknown"), "unknown key preserved");
}

void test_document_round_trip() {
    gconfig::Document doc;
    doc.set_bool("flag", true);
    doc.set_int("count", 42);
    doc.set_float("scale", 1.5f);
    doc.set_string("name", "demo");
    doc.set_vec2("pos", gconfig::Vec2{3.0f, 4.0f});

    std::string text;
    require(gconfig::save_document_string(doc, text), "save document string");
    gconfig::LoadDocumentResult loaded = gconfig::load_document_string(text);

    require(loaded.ok, "load saved document string");
    require(loaded.document.get_bool("flag", false), "round-trip bool");
    require(loaded.document.get_int("count", 0) == 42, "round-trip int");
    require(loaded.document.get_float("scale", 0.0f) == 1.5f, "round-trip float");
    require(loaded.document.get_string("name", "") == "demo", "round-trip string");
    gconfig::Vec2 pos = loaded.document.get_vec2("pos");
    require(pos.x == 3.0f && pos.y == 4.0f, "round-trip vec2");
}

void test_schema_aware_load_reconciles() {
    gconfig::Schema schema = make_schema();
    gconfig::LoadDocumentResult loaded = gconfig::load_document_string(
        R"((config
  (values
    (key "video.vsync" 0)
    (key "video.window_mode" "fullscreen"))))",
        schema);

    require(loaded.ok, "schema-aware load succeeds");
    require(loaded.reconcile_report.changed(), "schema-aware load reports default additions");
    require(!loaded.document.get_bool("video.vsync", true), "schema-aware load coerces bool");
    require(loaded.document.get_float("audio.music_volume", 0.0f) == 1.0f,
            "schema-aware load adds missing default");
}

void test_schema_ordered_save() {
    gconfig::Schema schema;
    schema.add_string("b", "").set_order(20);
    schema.add_string("a", "").set_order(10);

    gconfig::Document doc;
    doc.set_string("z_unknown", "last");
    doc.set_string("b", "second");
    doc.set_string("a", "first");

    std::string text;
    require(gconfig::save_document_string(doc, schema, text), "save schema ordered document");

    std::size_t a_pos = text.find("\"a\"");
    std::size_t b_pos = text.find("\"b\"");
    std::size_t z_pos = text.find("\"z_unknown\"");
    require(a_pos != std::string::npos, "ordered save includes a");
    require(b_pos != std::string::npos, "ordered save includes b");
    require(z_pos != std::string::npos, "ordered save includes unknown");
    require(a_pos < b_pos && b_pos < z_pos, "ordered save uses schema order then unknown keys");
}

void test_document_list() {
    gconfig::DocumentRecord record;
    record.id = 100;
    record.name = "Default";
    record.document.set_float("mouse.sensitivity", 0.75f);

    gconfig::DocumentList list;
    list.add(record);

    std::string text;
    require(gconfig::save_document_list_string(list, text), "save document list string");
    gconfig::LoadDocumentListResult loaded = gconfig::load_document_list_string(text);

    require(loaded.ok, "load document list string");
    const gconfig::DocumentRecord* found = loaded.documents.find(100);
    require(found != nullptr, "find document by id");
    require(found->name == "Default", "round-trip record name");
    require(found->document.get_float("mouse.sensitivity", 0.0f) == 0.75f,
            "round-trip record value");
}

void test_gubsy_shaped_document_lists() {
    gconfig::LoadDocumentListResult game_settings = gconfig::load_document_list_string(
        R"((game_settings_list
  (settings
    (id 49195645)
    (name "GMans Settings")
    (values
      (key "gubsy.input.mouse_sensitivity" 1)
      (key "gubsy.input.invert_y_axis" 0)))))",
        "game_settings_list", "settings");

    require(game_settings.ok, "load gubsy game settings shape");
    const gconfig::DocumentRecord* game = game_settings.documents.find(49195645);
    require(game != nullptr, "find gubsy settings record");
    require(game->name == "GMans Settings", "read gubsy settings name");
    require(game->document.get_int("gubsy.input.invert_y_axis", -1) == 0, "read gubsy values node");

    gconfig::LoadDocumentListResult user_profiles = gconfig::load_document_list_string(
        R"((user_profiles
  (profile
    (id 66933936)
    (name "GMan")
    (last_binds_profile 1)
    (last_game_settings 35883033))))",
        "user_profiles", "profile");

    require(user_profiles.ok, "load gubsy user profile shape");
    const gconfig::DocumentRecord* user = user_profiles.documents.find(66933936);
    require(user != nullptr, "find gubsy user profile");
    require(user->document.get_int("last_binds_profile", -1) == 1, "read direct record int");
    require(user->document.get_int("last_game_settings", -1) == 35883033,
            "read direct record settings ref");
}

} // namespace

int main() {
    test_document_reconcile();
    test_preserved_unknown_is_reported_but_not_changed();
    test_document_round_trip();
    test_schema_aware_load_reconciles();
    test_schema_ordered_save();
    test_document_list();
    test_gubsy_shaped_document_lists();

    std::cout << "gconfig_tests passed\n";
    return 0;
}
