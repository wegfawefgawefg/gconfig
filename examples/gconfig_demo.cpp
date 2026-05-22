#include "gconfig/gconfig.hpp"

#include <iostream>

namespace {

gconfig::Schema make_global_schema() {
    gconfig::Schema schema;
    schema.add_string("video.window_mode", "windowed")
        .set_label("Display Mode")
        .add_category("Video")
        .add_option("fullscreen", "Fullscreen")
        .add_option("borderless", "Borderless")
        .add_option("windowed", "Windowed");
    schema.add_string("video.render_resolution", "1920x1080")
        .set_label("Render Resolution")
        .add_category("Video");
    schema.add_bool("video.vsync", true).set_label("V-Sync").add_category("Video");
    schema.add_float("audio.master_volume", 1.0f)
        .set_label("Master Volume")
        .add_category("Audio")
        .set_range(0.0f, 1.0f, 0.01f);
    return schema;
}

gconfig::Schema make_game_schema() {
    gconfig::Schema schema;
    schema.add_float("input.mouse_sensitivity", 1.0f)
        .set_label("Mouse Sensitivity")
        .add_category("Controls")
        .set_range(0.1f, 10.0f, 0.1f);
    schema.add_bool("input.invert_y_axis", false)
        .set_label("Invert Y-Axis")
        .add_category("Controls");
    schema.add_float("hud.scale", 1.0f)
        .set_label("HUD Scale")
        .add_category("HUD")
        .set_range(0.5f, 2.0f, 0.05f);
    return schema;
}

gconfig::Schema make_user_schema() {
    gconfig::Schema schema;
    schema.add_string("profile.name", "Player");
    schema.add_int("last_binds_profile", -1);
    schema.add_int("last_game_settings", -1);
    schema.add_int("last_save_slot", -1);
    return schema;
}

gconfig::Schema make_binds_schema() {
    gconfig::Schema schema;
    schema.add_string("bind.jump", "space");
    schema.add_string("bind.confirm", "enter");
    schema.add_string("bind.cancel", "escape");
    return schema;
}

void print_report(std::string_view label, const gconfig::ReconcileReport& report) {
    std::cout << label << ": " << report.changes.size() << " reconcile changes\n";
    for (const gconfig::ReconcileChange& change : report.changes)
        std::cout << "  " << change.key << ": " << change.message << "\n";
}

} // namespace

int main() {
    gconfig::Schema global_schema = make_global_schema();
    gconfig::Schema user_schema = make_user_schema();
    gconfig::Schema game_schema = make_game_schema();
    gconfig::Schema binds_schema = make_binds_schema();

    gconfig::Document global;
    global.set_string("video.window_mode", "windowed");
    global.set_float("audio.master_volume", 0.8f);
    print_report("global", global.reconcile(global_schema));

    gconfig::DocumentList game_profiles;
    gconfig::DocumentRecord game_profile;
    game_profile.id = 100;
    game_profile.name = "Default Game Settings";
    game_profile.document.set_float("input.mouse_sensitivity", 1.25f);
    game_profiles.add(game_profile);
    print_report("game profiles", game_profiles.reconcile_all(game_schema));

    gconfig::DocumentList bind_profiles;
    gconfig::DocumentRecord binds;
    binds.id = 1;
    binds.name = "Keyboard";
    binds.document.set_string("bind.jump", "space");
    bind_profiles.add(binds);
    print_report("bind profiles", bind_profiles.reconcile_all(binds_schema));

    gconfig::DocumentList user_profiles;
    gconfig::DocumentRecord user;
    user.id = 42;
    user.name = "GMan";
    user.document.set_string("profile.name", "GMan");
    user.document.set_int("last_binds_profile", 1);
    user.document.set_int("last_game_settings", 100);
    user.document.set_int("last_save_slot", 3);
    user_profiles.add(user);
    print_report("user profiles", user_profiles.reconcile_all(user_schema));

    const gconfig::DocumentRecord* active_user = user_profiles.find(42);
    if (!active_user)
        return 1;

    int game_settings_id = active_user->document.get_int("last_game_settings", -1);
    int binds_id = active_user->document.get_int("last_binds_profile", -1);
    const gconfig::DocumentRecord* active_game = game_profiles.find(game_settings_id);
    const gconfig::DocumentRecord* active_binds = bind_profiles.find(binds_id);

    std::cout << "active user: " << active_user->name << "\n";
    std::cout << "window mode: " << global.get_string("video.window_mode", "unknown") << "\n";
    if (active_game) {
        std::cout << "mouse sensitivity: "
                  << active_game->document.get_float("input.mouse_sensitivity", 1.0f) << "\n";
    }
    if (active_binds) {
        std::cout << "jump bind: " << active_binds->document.get_string("bind.jump", "unset")
                  << "\n";
    }

    std::string text;
    gconfig::save_document_list_string(user_profiles, user_schema, text, "user_profiles");
    std::cout << "\nuser profile file:\n" << text;
    return 0;
}
