# TODO - UI menu options improvements

- [x] Add dedicated callbacks in `src/ui/main_window.c`:
  - [x] on_settings_dialog
  - [x] on_plugins_dialog
  - [x] on_add_reference_dialog
  - [x] on_manage_references_dialog

- [x] Implement "Paramètres" dialog (V1):
  - [x] dictionary path entry
  - [x] language entry
  - [x] autosave toggle (session only)
  - [x] statusbar feedback

- [x] Implement "Gestion des plugins" dialog (V1):
  - [x] show plugin list placeholders
  - [x] activate/deactivate toggles (session only)
  - [x] statusbar feedback

- [x] Implement "Ajouter référence..." dialog (V1):
  - [x] form (title, author, year, type)
  - [x] insert formatted reference into current page text
  - [x] statusbar feedback

- [x] Implement "Gérer références" dialog (V1):
  - [x] list current references (session)
  - [x] copy selected reference
  - [x] remove selected reference
  - [x] statusbar feedback

- [x] Wire menu items to new callbacks (replace stubs).

- [ ] Improve image insertion (V1):
  - [x] add `on_insert_image` callback
  - [x] file chooser + image filters
  - [x] proportional resize before insertion
  - [x] inline insertion at cursor via child anchor
  - [x] statusbar feedback + error handling

- [ ] Build check:
  - [ ] `cmake --build build -j4`
