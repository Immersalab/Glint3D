#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use tauri::{CustomMenuItem, Manager, Menu, Submenu};

fn main() {
    let open_item = CustomMenuItem::new("open", "Open Model…").accelerator("Ctrl+O");
    let file_menu = Submenu::new("File", Menu::new().add_item(open_item));
    let menu = Menu::new().add_submenu(file_menu);

    tauri::Builder::default()
        .menu(menu)
        .on_menu_event(|event| {
            match event.menu_item_id() {
                "open" => { let _ = event.window().emit("open-model", ()); }
                _ => {}
            }
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
