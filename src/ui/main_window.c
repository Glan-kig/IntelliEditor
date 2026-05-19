/* src/ui/main_window.c
 *
 * IntelliEditor - main_window with "page" model, page setup, margin toggle, help dialogs
 *
 * - Pages are stacked vertically (OnlyOffice-like)
 * - "Saut de page" creates a new page below the current one and focuses it
 * - Insertion de tableau : dialogue lignes/colonnes, insère un GtkGrid
 * - Mise en page : dialogue pour définir format, orientation et marges
 * - Afficher/Masquer marges : bascule l'affichage des guides visuels (comportement OnlyOffice)
 * - Panneau de conformité rétractable (GtkRevealer)
 * - Contenu de l'aide et À propos implémentés (dialogs)
 *
 * Place les fichiers CSS dans :
 *   resources/style_light.css
 *   resources/style_dark.css
 *
 * Compile:
 * gcc -Iinclude src/main.c src/ui/main_window.c src/ui/rules_panel.c src/utils/rules_stub.c -o text $(pkg-config --cflags --libs gtk+-3.0)
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "exporter.h"
#include "gap_buffer.h"
#include "search_replace.h"
#include "undo_redo.h"
#include "rules.h"

/* ---------- CSS theme loader (global provider) ---------- */
static GtkCssProvider *global_css_provider = NULL;
static gboolean theme_is_dark = FALSE;

static void load_css_file(const char *path) {
    if (global_css_provider) {
        g_object_unref(global_css_provider);
        global_css_provider = NULL;
    }
    global_css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(global_css_provider, path, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(global_css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
}

static void on_toggle_theme(GtkWidget *widget, gpointer data) {
    (void)widget;
    theme_is_dark = !theme_is_dark;
    if (theme_is_dark)
        load_css_file("resources/style_dark.css");
    else
        load_css_file("resources/style_light.css");
}

/* ---------- Helpers / état stocké dans la fenêtre ---------- */
static void set_current_file(GtkWidget *window, const char *path) {
    if (path)
        g_object_set_data_full(G_OBJECT(window), "current_file", g_strdup(path), g_free);
    else
        g_object_set_data(G_OBJECT(window), "current_file", NULL);
}

static const char* get_current_file(GtkWidget *window) {
    return (const char*)g_object_get_data(G_OBJECT(window), "current_file");
}

static void set_font_size(GtkWidget *window, int size) {
    g_object_set_data(G_OBJECT(window), "font_size", GINT_TO_POINTER(size));
}

static int get_font_size(GtkWidget *window) {
    gpointer p = g_object_get_data(G_OBJECT(window), "font_size");
    if (!p) return 12;
    return GPOINTER_TO_INT(p);
}

static Formatter *get_document_formatter(GtkWidget *window) {
    Formatter *fmt = g_object_get_data(G_OBJECT(window), "formatter");
    if (!fmt) {
        fmt = formatter_create();
        g_object_set_data_full(G_OBJECT(window), "formatter", fmt,
                               (GDestroyNotify)formatter_destroy);
    }
    return fmt;
}

static UndoRedoStack *get_document_undo_stack(GtkWidget *window) {
    UndoRedoStack *stack = g_object_get_data(G_OBJECT(window), "undo_stack");
    if (!stack) {
        stack = undo_redo_create(128);
        g_object_set_data_full(G_OBJECT(window), "undo_stack", stack,
                               (GDestroyNotify)undo_redo_destroy);
    }
    return stack;
}

static void reset_document_undo_stack(GtkWidget *window) {
    UndoRedoStack *stack = g_object_get_data(G_OBJECT(window), "undo_stack");
    if (stack) {
        undo_redo_destroy(stack);
    }
    stack = undo_redo_create(128);
    g_object_set_data_full(G_OBJECT(window), "undo_stack", stack,
                           (GDestroyNotify)undo_redo_destroy);
}

static void set_undo_suppressed(GtkWidget *window, gboolean suppressed) {
    g_object_set_data(G_OBJECT(window), "suppress_undo", GINT_TO_POINTER(suppressed));
}

static gboolean is_undo_suppressed(GtkWidget *window) {
    gpointer data = g_object_get_data(G_OBJECT(window), "suppress_undo");
    return GPOINTER_TO_INT(data) != 0;
}

static void reset_document_formatter(GtkWidget *window) {
    Formatter *fmt = g_object_get_data(G_OBJECT(window), "formatter");
    if (fmt) {
        formatter_destroy(fmt);
    }
    fmt = formatter_create();
    g_object_set_data_full(G_OBJECT(window), "formatter", fmt,
                           (GDestroyNotify)formatter_destroy);
}

static size_t get_page_offset(GtkWidget *window, GtkWidget *tv) {
    if (!window || !tv) return 0;
    GtkWidget *pages_box = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "pages_box"));
    if (!pages_box) return 0;

    GList *children = gtk_container_get_children(GTK_CONTAINER(pages_box));
    size_t offset = 0;
    for (GList *l = children; l; l = l->next) {
        GtkWidget *frame = GTK_WIDGET(l->data);
        GtkWidget *page_tv = GTK_WIDGET(g_object_get_data(G_OBJECT(frame), "page_textview"));
        if (!page_tv) page_tv = gtk_bin_get_child(GTK_BIN(frame));
        if (page_tv == tv) break;

        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(page_tv));
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);
        offset += gtk_text_iter_get_offset(&end);
        if (l->next) offset += 1;
    }
    g_list_free(children);
    return offset;
}

static void apply_formatter_range(GtkWidget *window, GtkWidget *tv,
                                  const GtkTextIter *start,
                                  const GtkTextIter *end,
                                  unsigned int style) {
    size_t page_offset = get_page_offset(window, tv);
    size_t local_start = gtk_text_iter_get_offset(start);
    size_t local_end = gtk_text_iter_get_offset(end);
    if (local_end <= local_start) return;

    Formatter *fmt = get_document_formatter(window);
    formatter_apply(fmt, page_offset + local_start, page_offset + local_end, style);
}

/* Page layout state stored on window:
 * - margins_visible : gboolean (visibility of guides)
 * - margins: int[4] left, right, top, bottom (points)
 * - page_format: string ("A4", "Letter", "Custom")
 * - orientation: string ("Portrait", "Paysage")
 */
static void set_margins(GtkWidget *window, int left, int right, int top, int bottom) {
    int *m = g_new(int, 4);
    m[0] = left; m[1] = right; m[2] = top; m[3] = bottom;
    g_object_set_data_full(G_OBJECT(window), "page_margins", m, g_free);
}

static void get_margins(GtkWidget *window, int *left, int *right, int *top, int *bottom) {
    int *m = (int*)g_object_get_data(G_OBJECT(window), "page_margins");
    if (m) {
        *left = m[0]; *right = m[1]; *top = m[2]; *bottom = m[3];
    } else {
        *left = *right = *top = *bottom = 40; /* defaults */
    }
}

static void set_margins_visible(GtkWidget *window, gboolean visible) {
    gboolean *p = g_new(gboolean, 1);
    *p = visible;
    g_object_set_data_full(G_OBJECT(window), "margins_visible", p, g_free);
}

static gboolean get_margins_visible(GtkWidget *window) {
    gboolean *p = (gboolean*)g_object_get_data(G_OBJECT(window), "margins_visible");
    if (!p) return TRUE;
    return *p;
}

static void set_page_format(GtkWidget *window, const char *fmt) {
    g_object_set_data_full(G_OBJECT(window), "page_format", g_strdup(fmt), g_free);
}

static const char* get_page_format(GtkWidget *window) {
    return (const char*)g_object_get_data(G_OBJECT(window), "page_format");
}

static void set_orientation(GtkWidget *window, const char *orient) {
    g_object_set_data_full(G_OBJECT(window), "page_orientation", g_strdup(orient), g_free);
}

static const char* get_orientation(GtkWidget *window) {
    return (const char*)g_object_get_data(G_OBJECT(window), "page_orientation");
}

/* Forward declarations */
static GtkWidget* create_page_widget(GtkWidget *window);
static GtkWidget* add_page_at_end(GtkWidget *window, GtkWidget *pages_box);
static GtkWidget* insert_page_after(GtkWidget *window, GtkWidget *pages_box, GtkWidget *after_textview);
static GtkWidget* get_current_textview(GtkWidget *window);
void apply_margins_to_all_pages(GtkWidget *window); /* defined later */

/* ---------- Page focus callback (sets current page textview) ---------- */
static gboolean on_page_focus_in(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
    GtkWidget *window = GTK_WIDGET(user_data);
    g_object_set_data(G_OBJECT(window), "current_page", widget);
    (void)event;
    return FALSE;
}

/* ---------- Drawing guides for margins (overlay) ---------- */
/* Draw callback for margin guides overlay */
static gboolean draw_margin_guides(GtkWidget *drawing, cairo_t *cr, gpointer user_data) {
    GtkTextView *tv = GTK_TEXT_VIEW(user_data);
    if (!GTK_IS_TEXT_VIEW(tv)) return FALSE;

    /* Retrieve margins (points) from top-level window */
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(tv));
    int left_pt = 40, right_pt = 40, top_pt = 30, bottom_pt = 30;
    get_margins(GTK_WIDGET(window), &left_pt, &right_pt, &top_pt, &bottom_pt);

    /* Convert points to pixels (approx): 1 pt = 96/72 px */
    double scale = 96.0 / 72.0;
    int left_px = (int)(left_pt * scale);
    int right_px = (int)(right_pt * scale);
    int top_px = (int)(top_pt * scale);
    int bottom_px = (int)(bottom_pt * scale);

    /* Get drawing area size */
    GtkAllocation alloc;
    gtk_widget_get_allocation(drawing, &alloc);
    int width = alloc.width;
    int height = alloc.height;

    /* Compute guide positions */
    int x_left = left_px;
    int x_right = width - right_px;
    int y_top = top_px;
    int y_bottom = height - bottom_px;

    /* Draw semi-transparent guides */
    cairo_set_source_rgba(cr, 0.2, 0.5, 0.9, 0.85);
    cairo_set_line_width(cr, 1.0);

    /* Vertical left */
    cairo_move_to(cr, x_left + 0.5, 0.0);
    cairo_line_to(cr, x_left + 0.5, height);
    cairo_stroke(cr);

    /* Vertical right */
    cairo_move_to(cr, x_right + 0.5, 0.0);
    cairo_line_to(cr, x_right + 0.5, height);
    cairo_stroke(cr);

    /* Horizontal top */
    cairo_move_to(cr, 0.0, y_top + 0.5);
    cairo_line_to(cr, width, y_top + 0.5);
    cairo_stroke(cr);

    /* Horizontal bottom */
    cairo_move_to(cr, 0.0, y_bottom + 0.5);
    cairo_line_to(cr, width, y_bottom + 0.5);
    cairo_stroke(cr);

    return FALSE;
}

/* ---------- Non-intrusive UI additions: font chooser and apply font ---------- */
/* Appliquer la police (Pango font description string) à toutes les pages */
static void apply_font_to_all_pages(GtkWidget *window, const char *font_desc_str) {
    if (!font_desc_str) return;
    GtkWidget *pages_box = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "pages_box"));
    if (!pages_box) return;

    GList *children = gtk_container_get_children(GTK_CONTAINER(pages_box));
    for (GList *l = children; l; l = l->next) {
        GtkWidget *frame = GTK_WIDGET(l->data);
        GtkWidget *tv = GTK_WIDGET(g_object_get_data(G_OBJECT(frame), "page_textview"));
        if (!tv) tv = gtk_bin_get_child(GTK_BIN(frame));
        if (!tv || !GTK_IS_TEXT_VIEW(tv)) continue;

        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);

        GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
        GtkTextTag *font_tag = gtk_text_tag_table_lookup(table, "user_font");
        if (!font_tag) {
            font_tag = gtk_text_tag_new("user_font");
            gtk_text_tag_table_add(table, font_tag);
        }
        g_object_set(font_tag, "font", font_desc_str, NULL);
        gtk_text_buffer_apply_tag(buffer, font_tag, &start, &end);
    }
    g_list_free(children);

    /* store chosen font on window for persistence during session */
    g_object_set_data_full(G_OBJECT(window), "user_font", g_strdup(font_desc_str), g_free);
}

/* Ouvre le dialogue de choix de police et applique la police choisie */
static void on_choose_font(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_font_chooser_dialog_new("Choisir la police", GTK_WINDOW(window));
    const char *cur_font = (const char*)g_object_get_data(G_OBJECT(window), "user_font");
    if (cur_font) gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dialog), cur_font);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        char *fontname = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(dialog));
        if (fontname) {
            apply_font_to_all_pages(window, fontname);
            g_free(fontname);
        }
    }
    gtk_widget_destroy(dialog);
    (void)widget;
}

/* ---------- Overflow detection: create new page when cursor reaches page bottom ---------- */

/* Forward helper used by callbacks */
static void check_cursor_overflow(GtkTextBuffer *buffer, GtkTextView *tv) {
    if (!GTK_IS_TEXT_VIEW(tv)) return;

    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(tv));
    if (!GTK_IS_WINDOW(window)) return;

    /* Get insert mark iter */
    GtkTextIter iter;
    GtkTextMark *ins_mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, ins_mark);

    /* Get iter location rectangle (in buffer coords) */
    GdkRectangle iter_rect;
    gtk_text_view_get_iter_location(tv, &iter, &iter_rect);

    /* Convert buffer coords to widget coords */
    gint wx, wy;
    gtk_text_view_buffer_to_window_coords(tv, GTK_TEXT_WINDOW_WIDGET, iter_rect.x, iter_rect.y, &wx, &wy);

    /* Get allocation of textview */
    GtkAllocation alloc;
    gtk_widget_get_allocation(GTK_WIDGET(tv), &alloc);

    /* Get bottom margin in pixels */
    int left_pt, right_pt, top_pt, bottom_pt;
    get_margins(GTK_WIDGET(window), &left_pt, &right_pt, &top_pt, &bottom_pt);
    double scale = 96.0 / 72.0;
    int bottom_px = (int)(bottom_pt * scale);

    /* If cursor Y + iter height goes beyond visible height minus bottom margin, overflow */
    if (wy + iter_rect.height > alloc.height - bottom_px) {
        /* create new page below current */
        GtkWidget *pages_box = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "pages_box"));
        if (!pages_box) return;

        insert_page_after(window, pages_box, GTK_WIDGET(tv));

        /* focus the newly created page's textview and place cursor at start */
        GList *children = gtk_container_get_children(GTK_CONTAINER(pages_box));
        GtkWidget *next_tv = NULL;
        for (GList *l = children; l; l = l->next) {
            GtkWidget *frame = GTK_WIDGET(l->data);
            GtkWidget *child_tv = GTK_WIDGET(g_object_get_data(G_OBJECT(frame), "page_textview"));
            if (!child_tv) child_tv = gtk_bin_get_child(GTK_BIN(frame));
            if (child_tv == GTK_WIDGET(tv) && l->next) {
                GtkWidget *next_frame = GTK_WIDGET(l->next->data);
                next_tv = GTK_WIDGET(g_object_get_data(G_OBJECT(next_frame), "page_textview"));
                if (!next_tv) next_tv = gtk_bin_get_child(GTK_BIN(next_frame));
                break;
            }
        }
        g_list_free(children);

        if (next_tv) {
            GtkTextBuffer *nbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(next_tv));
            GtkTextIter start;
            gtk_text_buffer_get_start_iter(nbuf, &start);
            gtk_text_buffer_place_cursor(nbuf, &start);
            gtk_widget_grab_focus(next_tv);
            g_object_set_data(G_OBJECT(window), "current_page", next_tv);
        }
    }
}

/* Buffer "changed" callback wrapper */
static void buffer_changed_cb(GtkTextBuffer *buffer, gpointer user_data) {
    GtkTextView *tv = GTK_TEXT_VIEW(user_data);
    check_cursor_overflow(buffer, tv);
}

static void buffer_delete_range_cb(GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end, gpointer user_data) {
    GtkTextView *tv = GTK_TEXT_VIEW(user_data);
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(tv));
    if (!GTK_IS_WINDOW(window)) return;
    if (is_undo_suppressed(window)) return;

    char *deleted_text = gtk_text_buffer_get_text(buffer, start, end, FALSE);
    gint offset = gtk_text_iter_get_offset(start);
    if (deleted_text) {
        UndoRedoStack *stack = get_document_undo_stack(window);
        undo_redo_push(stack, CMD_DELETE, offset, deleted_text);
        g_free(deleted_text);
    }
}

static void buffer_insert_text_cb(GtkTextBuffer *buffer, GtkTextIter *location, gchar *text, gint len, gpointer user_data) {
    GtkTextView *tv = GTK_TEXT_VIEW(user_data);
    GtkWidget *window = gtk_widget_get_toplevel(GTK_WIDGET(tv));
    if (!GTK_IS_WINDOW(window)) return;
    if (is_undo_suppressed(window)) {
        check_cursor_overflow(buffer, tv);
        return;
    }

    if (len <= 0 || !text) {
        check_cursor_overflow(buffer, tv);
        return;
    }

    gint offset = gtk_text_iter_get_offset(location);
    char *inserted_text = g_strndup(text, len);
    if (inserted_text) {
        UndoRedoStack *stack = get_document_undo_stack(window);
        undo_redo_push(stack, CMD_INSERT, offset, inserted_text);
        g_free(inserted_text);
    }

    check_cursor_overflow(buffer, tv);
}

/* Buffer "mark-set" callback wrapper to detect cursor moves */
static void buffer_mark_set_cb(GtkTextBuffer *buffer, GtkTextIter *location, GtkTextMark *mark, gpointer user_data) {
    /* only react to insert mark */
    if (g_strcmp0(gtk_text_mark_get_name(mark), "insert") == 0) {
        GtkTextView *tv = GTK_TEXT_VIEW(user_data);
        check_cursor_overflow(buffer, tv);
    }
}

/* ---------- Page management ----------
   pages_box holds page frames stacked vertically.
   Each page is a GtkFrame containing a GtkOverlay with GtkTextView and a GtkDrawingArea for guides.
*/

/* Create a single page (frame + overlay(textview + guide)) */
static GtkWidget* create_page_widget(GtkWidget *window) {
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_set_hexpand(frame, TRUE);
    gtk_widget_set_vexpand(frame, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 8);

    gtk_style_context_add_class(gtk_widget_get_style_context(frame), "page-frame");

    /* Overlay */
    GtkWidget *overlay = gtk_overlay_new();
    gtk_widget_set_hexpand(overlay, TRUE);
    gtk_widget_set_vexpand(overlay, FALSE);

    /* TextView */
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);

    /* Apply current margins */
    int left, right, top, bottom;
    get_margins(window, &left, &right, &top, &bottom);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view), left);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_view), right);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text_view), top);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(text_view), bottom);

    gtk_widget_set_size_request(text_view, 700, 900);

    /* Drawing area for guides */
    GtkWidget *guide = gtk_drawing_area_new();
    gtk_widget_set_hexpand(guide, TRUE);
    gtk_widget_set_vexpand(guide, TRUE);

    gboolean guides_visible = get_margins_visible(window);
    gtk_widget_set_visible(guide, guides_visible);

    /* Connect draw callback; pass text_view as user_data to read allocation if needed */
    g_signal_connect(G_OBJECT(guide), "draw", G_CALLBACK(draw_margin_guides), text_view);

    /* Pack: text_view below, guide overlay above */
    gtk_container_add(GTK_CONTAINER(overlay), text_view);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), guide);

    /* Add overlay to frame */
    gtk_container_add(GTK_CONTAINER(frame), overlay);

    /* Focus handling */
    g_signal_connect(text_view, "focus-in-event", G_CALLBACK(on_page_focus_in), window);

    /* Store pointers for later access */
    g_object_set_data(G_OBJECT(frame), "page_textview", text_view);
    g_object_set_data(G_OBJECT(frame), "margin_guide", guide);

    /* Connect buffer signals to detect overflow and cursor moves */
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    g_signal_connect(buffer, "changed", G_CALLBACK(buffer_changed_cb), text_view);
    g_signal_connect(buffer, "insert-text", G_CALLBACK(buffer_insert_text_cb), text_view);
    g_signal_connect(buffer, "delete-range", G_CALLBACK(buffer_delete_range_cb), text_view);
    g_signal_connect(buffer, "mark-set", G_CALLBACK(buffer_mark_set_cb), text_view);

    return frame;
}

/* Add a new page at the end */
static GtkWidget* add_page_at_end(GtkWidget *window, GtkWidget *pages_box) {
    GtkWidget *page = create_page_widget(window);
    gtk_box_pack_end(GTK_BOX(pages_box), page, FALSE, FALSE, 12);
    gtk_widget_show_all(page);

    /* Use stored pointer if present, otherwise try to get child */
    GtkWidget *stored_tv = GTK_WIDGET(g_object_get_data(G_OBJECT(page), "page_textview"));
    GtkWidget *tv = stored_tv;
    if (!tv) {
        GtkWidget *child = gtk_bin_get_child(GTK_BIN(page)); /* overlay */
        if (child) {
            /* try to retrieve stored textview again */
            tv = GTK_WIDGET(g_object_get_data(G_OBJECT(page), "page_textview"));
        }
    }

    if (tv) gtk_widget_grab_focus(tv);
    g_object_set_data(G_OBJECT(window), "current_page", tv);
    return page;
}

/* Insert a new page after the page that contains 'after_textview' (if NULL, append)
   This version ensures the new page is placed below the page that contains after_textview.
*/
static GtkWidget* insert_page_after(GtkWidget *window, GtkWidget *pages_box, GtkWidget *after_textview) {
    GtkWidget *new_page = create_page_widget(window);

    if (!after_textview) {
        /* append at end */
        gtk_box_pack_end(GTK_BOX(pages_box), new_page, FALSE, FALSE, 12);
        gtk_widget_show_all(new_page);
        GtkWidget *tv = GTK_WIDGET(g_object_get_data(G_OBJECT(new_page), "page_textview"));
        g_object_set_data(G_OBJECT(window), "current_page", tv);
        return new_page;
    }

    /* find index of frame that contains after_textview */
    GList *children = gtk_container_get_children(GTK_CONTAINER(pages_box));
    gint index = -1;
    gint pos = 0;
    for (GList *l = children; l; l = l->next, pos++) {
        GtkWidget *frame = GTK_WIDGET(l->data);
        GtkWidget *child_tv = GTK_WIDGET(g_object_get_data(G_OBJECT(frame), "page_textview"));
        if (!child_tv) child_tv = gtk_bin_get_child(GTK_BIN(frame));
        if (child_tv == after_textview) {
            index = pos;
            break;
        }
    }

    if (index >= 0) {
        /* pack at start then reorder to index+1 to ensure correct position */
        gtk_box_pack_start(GTK_BOX(pages_box), new_page, FALSE, FALSE, 12);
        gtk_box_reorder_child(GTK_BOX(pages_box), new_page, index + 1);
    } else {
        gtk_box_pack_end(GTK_BOX(pages_box), new_page, FALSE, FALSE, 12);
    }

    gtk_widget_show_all(new_page);

    GtkWidget *tv = GTK_WIDGET(g_object_get_data(G_OBJECT(new_page), "page_textview"));
    if (!tv) tv = gtk_bin_get_child(GTK_BIN(new_page));
    if (tv) gtk_widget_grab_focus(tv);
    g_object_set_data(G_OBJECT(window), "current_page", tv);

    g_list_free(children);
    return new_page;
}

/* Get the current focused textview (page) */
static GtkWidget* get_current_textview(GtkWidget *window) {
    GtkWidget *tv = (GtkWidget*)g_object_get_data(G_OBJECT(window), "current_page");
    if (tv && GTK_IS_TEXT_VIEW(tv)) return tv;

    GtkWidget *pages_box = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "pages_box"));
    if (!pages_box) return NULL;
    GList *children = gtk_container_get_children(GTK_CONTAINER(pages_box));
    if (!children) return NULL;
    GtkWidget *first_frame = GTK_WIDGET(children->data);
    GtkWidget *first_tv = GTK_WIDGET(g_object_get_data(G_OBJECT(first_frame), "page_textview"));
    if (!first_tv) first_tv = gtk_bin_get_child(GTK_BIN(first_frame));
    g_list_free(children);
    return first_tv;
}

/* ---------- Callbacks and actions (file, edit, format, zoom) ---------- */
static void on_menu_action_stub(GtkWidget *widget, gpointer data) {
    const char *label = gtk_menu_item_get_label(GTK_MENU_ITEM(widget));
    g_print("Action menu: %s\n", label);
    (void)data;
}

static void on_quit(GtkWidget *widget, gpointer data) {
    (void)widget;
    nlp_system_cleanup();
    gtk_main_quit();
}

/* File operations (new/open/save) operate on the pages_box */
static void on_new_file(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *pages_box = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "pages_box"));
    GList *children = gtk_container_get_children(GTK_CONTAINER(pages_box));
    for (GList *l = children; l; l = l->next) gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);
    add_page_at_end(window, pages_box);
    reset_document_formatter(window);
    reset_document_undo_stack(window);
    set_current_file(window, NULL);
    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Nouveau document");
    (void)widget;
}

static void on_open_file(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Ouvrir", GTK_WINDOW(window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Annuler", GTK_RESPONSE_CANCEL,
                                                    "_Ouvrir", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gchar *content = NULL;
        gsize len = 0;
        GError *err = NULL;
        if (g_file_get_contents(filename, &content, &len, &err)) {
            reset_document_formatter(window);
            reset_document_undo_stack(window);
            set_undo_suppressed(window, TRUE);
            GapBuffer *gb = gap_buffer_create(len + 1);
            if (gb) {
                gap_buffer_insert_string(gb, content);
                load_document_from_gap_buffer(window, gb);
                gap_buffer_destroy(gb);
            } else {
                g_printerr("Impossible de créer le GapBuffer pour l'ouverture\n");
            }
            set_undo_suppressed(window, FALSE);
            set_current_file(window, filename);
            gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, filename);
            g_free(content);
        } else {
            g_printerr("Erreur lecture fichier: %s\n", err->message);
            g_error_free(err);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
    (void)widget;
}

static void on_save_file_as(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Enregistrer sous", GTK_WINDOW(window),
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "_Annuler", GTK_RESPONSE_CANCEL,
                                                    "_Enregistrer", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GapBuffer *gb = collect_pages_to_gap_buffer(window);
        if (gb) {
            char *text = gap_buffer_to_string(gb);
            if (text) {
                GError *err = NULL;
                if (!g_file_set_contents(filename, text, -1, &err)) {
                    g_printerr("Erreur écriture fichier: %s\n", err->message);
                    g_error_free(err);
                } else {
                    set_current_file(window, filename);
                    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, filename);
                }
                free(text);
            }
            gap_buffer_destroy(gb);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
    (void)widget;
}

static void on_save_file(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    const char *cur = get_current_file(window);
    if (cur) {
        GapBuffer *gb = collect_pages_to_gap_buffer(window);
        if (gb) {
            char *text = gap_buffer_to_string(gb);
            if (text) {
                GError *err = NULL;
                if (!g_file_set_contents(cur, text, -1, &err)) {
                    g_printerr("Erreur écriture fichier: %s\n", err->message);
                    g_error_free(err);
                } else {
                    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, cur);
                }
                free(text);
            }
            gap_buffer_destroy(gb);
        }
    } else {
        on_save_file_as(widget, data);
    }
}

/* ---------- Copy/Cut/Paste operate on current page textview ---------- */
static void on_export_file(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Exporter", GTK_WINDOW(window),
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "_Annuler", GTK_RESPONSE_CANCEL,
                                                    "_Exporter", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    GtkFileFilter *filter_pdf = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_pdf, "PDF document");
    gtk_file_filter_add_pattern(filter_pdf, "*.pdf");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_pdf);

    GtkFileFilter *filter_rtf = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_rtf, "RTF document");
    gtk_file_filter_add_pattern(filter_rtf, "*.rtf");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_rtf);

    GtkFileFilter *filter_ie = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_ie, "IntelliEditor file");
    gtk_file_filter_add_pattern(filter_ie, "*.ie");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_ie);

    GtkFileFilter *filter_txt = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_txt, "Texte brut");
    gtk_file_filter_add_pattern(filter_txt, "*.txt");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_txt);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GtkFileFilter *active_filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog));
        const char *dot = strrchr(filename, '.');
        if (!dot) {
            if (active_filter == filter_pdf) {
                char *new_name = g_strconcat(filename, ".pdf", NULL);
                g_free(filename);
                filename = new_name;
            } else if (active_filter == filter_rtf) {
                char *new_name = g_strconcat(filename, ".rtf", NULL);
                g_free(filename);
                filename = new_name;
            } else if (active_filter == filter_ie) {
                char *new_name = g_strconcat(filename, ".ie", NULL);
                g_free(filename);
                filename = new_name;
            } else if (active_filter == filter_txt) {
                char *new_name = g_strconcat(filename, ".txt", NULL);
                g_free(filename);
                filename = new_name;
            }
        }

        /* Collect text from all pages via GapBuffer */
        GapBuffer *gb = collect_pages_to_gap_buffer(window);
        if (gb) {
            ExportFormat fmt = exporter_detect_format(filename);
            Formatter *fmt_state = get_document_formatter(window);
            bool ok = exporter_save(gb, fmt_state, filename, fmt);
            if (!ok) g_printerr("Erreur lors de l'export vers %s\n", filename);
            else gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, filename);
            gap_buffer_destroy(gb);
        } else {
            g_printerr("Impossible de créer le GapBuffer pour l'export\n");
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
    (void)widget;
}

static void on_copy(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *tv = get_current_textview(window);
    if (!tv) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_copy_clipboard(buffer, clipboard);
    (void)widget;
}

static void on_undo(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *tv = get_current_textview(window);
    if (!tv) return;
    UndoRedoStack *stack = get_document_undo_stack(window);
    if (!undo_redo_can_undo(stack)) return;

    char *text = get_current_page_text(window);
    if (!text) return;

    GapBuffer *gb = gap_buffer_create(strlen(text) + 1);
    if (!gb) {
        g_free(text);
        return;
    }
    gap_buffer_insert_string(gb, text);
    g_free(text);

    set_undo_suppressed(window, TRUE);
    bool ok = undo_redo_undo(stack, gb);
    set_undo_suppressed(window, FALSE);

    if (ok) {
        char *updated = gap_buffer_to_string(gb);
        if (updated) {
            set_current_page_text(window, updated);
            free(updated);
        }
    }
    gap_buffer_destroy(gb);
    (void)widget;
}

static void on_redo(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *tv = get_current_textview(window);
    if (!tv) return;
    UndoRedoStack *stack = get_document_undo_stack(window);
    if (!undo_redo_can_redo(stack)) return;

    char *text = get_current_page_text(window);
    if (!text) return;

    GapBuffer *gb = gap_buffer_create(strlen(text) + 1);
    if (!gb) {
        g_free(text);
        return;
    }
    gap_buffer_insert_string(gb, text);
    g_free(text);

    set_undo_suppressed(window, TRUE);
    bool ok = undo_redo_redo(stack, gb);
    set_undo_suppressed(window, FALSE);

    if (ok) {
        char *updated = gap_buffer_to_string(gb);
        if (updated) {
            set_current_page_text(window, updated);
            free(updated);
        }
    }
    gap_buffer_destroy(gb);
    (void)widget;
}

static void on_cut(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *tv = get_current_textview(window);
    if (!tv) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_cut_clipboard(buffer, clipboard, TRUE);
    (void)widget;
}

static void on_paste(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *tv = get_current_textview(window);
    if (!tv) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_text_buffer_paste_clipboard(buffer, clipboard, NULL, TRUE);
    (void)widget;
}

static char *get_current_page_text(GtkWidget *window) {
    GtkWidget *tv = get_current_textview(window);
    if (!tv) return NULL;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

static void set_current_page_text(GtkWidget *window, const char *text) {
    GtkWidget *tv = get_current_textview(window);
    if (!tv || !text) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    set_undo_suppressed(window, TRUE);
    gtk_text_buffer_set_text(buffer, text, -1);
    set_undo_suppressed(window, FALSE);
}

static char *get_word_at_cursor_or_selection(GtkWidget *window) {
    GtkWidget *tv = get_current_textview(window);
    if (!tv) return NULL;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkTextIter start, end;

    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    }

    GtkTextMark *insert_mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &start, insert_mark);
    if (!gtk_text_iter_starts_word(&start)) {
        gtk_text_iter_backward_word_start(&start);
    }
    end = start;
    if (!gtk_text_iter_ends_word(&end)) {
        gtk_text_iter_forward_word_end(&end);
    }
    if (gtk_text_iter_compare(&start, &end) == 0) return NULL;
    return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

static gboolean ask_semantic_analysis_dialog(GtkWindow *parent, char **out_section, char **out_instruction) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Analyse sémantique",
                                                    parent,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "_Annuler", GTK_RESPONSE_CANCEL,
                                                    "_Analyser", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);

    GtkWidget *section_label = gtk_label_new("Section (optionnel) :");
    GtkWidget *instruction_label = gtk_label_new("Instruction :");
    GtkWidget *section_entry = gtk_entry_new();
    GtkWidget *instruction_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(instruction_entry), "Vérifie si le texte est conforme à la consigne donnée.");

    gtk_grid_attach(GTK_GRID(grid), section_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), section_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), instruction_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), instruction_entry, 1, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    gboolean ok = FALSE;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *section_text = gtk_entry_get_text(GTK_ENTRY(section_entry));
        const char *instruction_text = gtk_entry_get_text(GTK_ENTRY(instruction_entry));
        *out_section = (section_text && *section_text) ? g_strdup(section_text) : NULL;
        *out_instruction = g_strdup(instruction_text ? instruction_text : "Analyse sémantique");
        ok = TRUE;
    }

    gtk_widget_destroy(dialog);
    return ok;
}

static void on_check_spelling(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    char *word = get_word_at_cursor_or_selection(window);
    if (!word || *word == '\0') {
        gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Aucun mot sélectionné ou détecté.");
        g_free(word);
        return;
    }

    int correct = is_word_correct(word);
    char *message = g_strdup_printf("%s : %s", word, correct ? "Correct" : "Incorrect");
    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, message);
    g_free(message);
    g_free(word);
    (void)widget;
}

static void on_semantic_analysis(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    char *text = get_current_page_text(window);
    if (!text || *text == '\0') {
        gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, "Aucun texte disponible pour l'analyse.");
        g_free(text);
        return;
    }

    char *section_name = NULL;
    char *instruction = NULL;
    if (!ask_semantic_analysis_dialog(GTK_WINDOW(window), &section_name, &instruction)) {
        g_free(text);
        g_free(section_name);
        g_free(instruction);
        return;
    }

    nlp_process_check(text, section_name, instruction);
    char *status_msg = g_strdup_printf("Analyse NLP lancée%s", section_name ? " sur section" : "");
    gtk_statusbar_push(GTK_STATUSBAR(g_object_get_data(G_OBJECT(window), "statusbar")), 0, status_msg);
    g_free(status_msg);
    g_free(text);
    g_free(section_name);
    g_free(instruction);
    (void)widget;
}
    GtkWidget *tv = get_current_textview(window);
    if (!tv) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkTextIter iter_start, iter_end;
    gtk_text_buffer_get_iter_at_offset(buffer, &iter_start, start);
    gtk_text_buffer_get_iter_at_offset(buffer, &iter_end, start + length);
    gtk_text_buffer_select_range(buffer, &iter_start, &iter_end);
    gtk_widget_grab_focus(tv);
}

static void search_replace_dialog_show(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Rechercher / Remplacer",
                                                    GTK_WINDOW(window),
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "_Fermer", GTK_RESPONSE_CLOSE,
                                                    "_Trouver suivant", GTK_RESPONSE_YES,
                                                    "_Remplacer", GTK_RESPONSE_APPLY,
                                                    "_Remplacer tout", GTK_RESPONSE_OK,
                                                    NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);

    GtkWidget *find_label = gtk_label_new("Rechercher :");
    GtkWidget *replace_label = gtk_label_new("Remplacer par :");
    GtkWidget *find_entry = gtk_entry_new();
    GtkWidget *replace_entry = gtk_entry_new();
    GtkWidget *case_check = gtk_check_button_new_with_label("Respecter la casse");
    GtkWidget *word_check = gtk_check_button_new_with_label("Mot entier seulement");

    gtk_grid_attach(GTK_GRID(grid), find_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), find_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), replace_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), replace_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), case_check, 0, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), word_check, 0, 3, 2, 1);
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    size_t last_search_pos = 0;
    gboolean keep_running = TRUE;
    while (keep_running) {
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response == GTK_RESPONSE_CLOSE) {
            keep_running = FALSE;
            break;
        }

        const char *pattern = gtk_entry_get_text(GTK_ENTRY(find_entry));
        const char *replacement = gtk_entry_get_text(GTK_ENTRY(replace_entry));
        SearchOptions options = {
            .case_sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(case_check)),
            .whole_word = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(word_check)),
            .use_regex = false
        };

        char *text = get_current_page_text(window);
        if (!text || strlen(pattern) == 0) {
            g_printerr("Texte ou motif invalide\n");
            if (text) {
                g_free(text);
            }
            continue;
        }

        SearchResult result;
        if (response == GTK_RESPONSE_YES) {
            SearchContext *ctx = search_create(text, pattern, &options);
            if (!ctx) {
                g_free(text);
                continue;
            }
            if (last_search_pos == 0) {
                result = search_find_first(ctx);
            } else {
                result = search_find_next(ctx, last_search_pos);
            }
            if (!result.found) {
                result = search_find_first(ctx);
                last_search_pos = 0;
            }
            search_destroy(ctx);
            if (result.found) {
                select_text_range_in_current_page(window, result.start, result.length);
                last_search_pos = result.start + result.length;
            } else {
                g_printerr("Motif non trouvé\n");
            }
        } else if (response == GTK_RESPONSE_APPLY) {
            ReplaceResult replace_res = search_replace_first(text, pattern, replacement, &options);
            if (replace_res.result) {
                set_current_page_text(window, replace_res.result);
                g_print("Replaced %zu instance(s)\n", replace_res.replacements);
            } else {
                g_printerr("Aucun remplacement effectué\n");
            }
            replace_result_free(&replace_res);
            last_search_pos = 0;
        } else if (response == GTK_RESPONSE_OK) {
            ReplaceResult replace_res = search_replace_all(text, pattern, replacement, &options);
            if (replace_res.result) {
                set_current_page_text(window, replace_res.result);
                g_print("Remplacé %zu occurrences\n", replace_res.replacements);
            } else {
                g_printerr("Aucun remplacement effectué\n");
            }
            replace_result_free(&replace_res);
            last_search_pos = 0;
        }

        g_free(text);
    }

    gtk_widget_destroy(dialog);
    (void)widget;
}

/* ---------- Formatting (apply to selection in current page) ---------- */
static void on_bold(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *tv = get_current_textview(window);
    if (!tv) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
        GtkTextTag *bold_tag = gtk_text_tag_table_lookup(table, "bold");
        if (!bold_tag) {
            bold_tag = gtk_text_tag_new("bold");
            g_object_set(bold_tag, "weight", PANGO_WEIGHT_BOLD, NULL);
            gtk_text_tag_table_add(table, bold_tag);
        }
        gtk_text_buffer_apply_tag(buffer, bold_tag, &start, &end);
        apply_formatter_range(window, tv, &start, &end, STYLE_BOLD);
    }
    (void)widget;
}

static void on_italic(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *tv = get_current_textview(window);
    if (!tv) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
        GtkTextTag *italic_tag = gtk_text_tag_table_lookup(table, "italic");
        if (!italic_tag) {
            italic_tag = gtk_text_tag_new("italic");
            g_object_set(italic_tag, "style", PANGO_STYLE_ITALIC, NULL);
            gtk_text_tag_table_add(table, italic_tag);
        }
        gtk_text_buffer_apply_tag(buffer, italic_tag, &start, &end);
        apply_formatter_range(window, tv, &start, &end, STYLE_ITALIC);
    }
    (void)widget;
}

static void on_underlined(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *tv = get_current_textview(window);
    if (!tv) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
        GtkTextTag *under_tag = gtk_text_tag_table_lookup(table, "underline");
        if (!under_tag) {
            under_tag = gtk_text_tag_new("underline");
            g_object_set(under_tag, "underline", PANGO_UNDERLINE_SINGLE, NULL);
            gtk_text_tag_table_add(table, under_tag);
        }
        gtk_text_buffer_apply_tag(buffer, under_tag, &start, &end);
        apply_formatter_range(window, tv, &start, &end, STYLE_UNDERLINE);
    }
    (void)widget;
}

/* ---------- Zoom helpers (apply to all pages) ---------- */
static void apply_font_size_to_all_pages(GtkWidget *window, int size) {
    GtkWidget *pages_box = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "pages_box"));
    if (!pages_box) return;
    GList *children = gtk_container_get_children(GTK_CONTAINER(pages_box));
    for (GList *l = children; l; l = l->next) {
        GtkWidget *frame = GTK_WIDGET(l->data);
        GtkWidget *tv = GTK_WIDGET(g_object_get_data(G_OBJECT(frame), "page_textview"));
        if (!tv) tv = gtk_bin_get_child(GTK_BIN(frame));
        if (!tv) continue;
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);
        GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
        GtkTextTag *size_tag = gtk_text_tag_table_lookup(table, "font_size");
        if (!size_tag) {
            size_tag = gtk_text_tag_new("font_size");
            g_object_set(size_tag, "size-points", size, NULL);
            gtk_text_tag_table_add(table, size_tag);
        } else {
            g_object_set(size_tag, "size-points", size, NULL);
        }
        gtk_text_buffer_apply_tag(buffer, size_tag, &start, &end);
    }
    g_list_free(children);
}

static void on_zoom_in(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    int size = get_font_size(window);
    if (size < 48) size += 2;
    set_font_size(window, size);
    apply_font_size_to_all_pages(window, size);
    (void)widget;
}

static void on_zoom_out(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    int size = get_font_size(window);
    if (size > 6) size -= 2;
    set_font_size(window, size);
    apply_font_size_to_all_pages(window, size);
    (void)widget;
}

static void on_zoom_reset(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    int size = 12;
    set_font_size(window, size);
    apply_font_size_to_all_pages(window, size);
    (void)widget;
}

/* ---------- Insert page break: create a new page below current page ---------- */
static void on_insert_page_break(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *pages_box = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "pages_box"));
    GtkWidget *current_tv = get_current_textview(window);
    insert_page_after(window, pages_box, current_tv);
    g_print("Action menu: Saut de page (inséré sous la page courante)\n");
    (void)widget;
}

/* ---------- Table insertion: create GtkGrid and insert as child widget ---------- */

/* Create a widget representing the table (frame + grid) */
static GtkWidget* create_table_widget(int rows, int cols) {
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_style_context_add_class(gtk_widget_get_style_context(frame), "embedded-table-frame");
    gtk_container_set_border_width(GTK_CONTAINER(frame), 6);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
    gtk_container_add(GTK_CONTAINER(frame), grid);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            GtkWidget *cell = gtk_entry_new();
            gtk_entry_set_text(GTK_ENTRY(cell), "");
            gtk_widget_set_size_request(cell, 100, 30);
            gtk_widget_set_hexpand(cell, TRUE);
            gtk_widget_set_vexpand(cell, FALSE);
            gtk_widget_set_halign(cell, GTK_ALIGN_FILL);
            gtk_widget_set_valign(cell, GTK_ALIGN_FILL);
            gtk_grid_attach(GTK_GRID(grid), cell, c, r, 1, 1);
            gtk_widget_show(cell);
        }
    }

    gtk_widget_show_all(frame);
    return frame;
}

/* Dialog to ask for rows/cols */
static gboolean ask_table_size_dialog(GtkWindow *parent, int *out_rows, int *out_cols) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Insérer un tableau",
                                                    parent,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "_Annuler", GTK_RESPONSE_CANCEL,
                                                    "_Insérer", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 8);

    GtkWidget *rows_label = gtk_label_new("Lignes :");
    GtkWidget *cols_label = gtk_label_new("Colonnes :");
    GtkWidget *rows_spin = gtk_spin_button_new_with_range(1, 50, 1);
    GtkWidget *cols_spin = gtk_spin_button_new_with_range(1, 50, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(rows_spin), 3);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(cols_spin), 3);

    gtk_grid_attach(GTK_GRID(grid), rows_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), rows_spin, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cols_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cols_spin, 1, 1, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    gboolean ok = FALSE;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        *out_rows = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(rows_spin));
        *out_cols = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(cols_spin));
        ok = TRUE;
    }

    gtk_widget_destroy(dialog);
    return ok;
}

/* Insert table at cursor position in current page */
static void on_insert_table(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *tv = get_current_textview(window);
    if (!tv) {
        g_print("Aucune page active pour insérer un tableau.\n");
        return;
    }

    int rows = 0, cols = 0;
    if (!ask_table_size_dialog(GTK_WINDOW(window), &rows, &cols)) {
        return;
    }

    GtkWidget *table_widget = create_table_widget(rows, cols);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkTextIter iter;
    if (gtk_text_buffer_get_has_selection(buffer)) {
        GtkTextIter start, end;
        gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
        iter = end;
    } else {
        GtkTextMark *ins = gtk_text_buffer_get_insert(buffer);
        gtk_text_buffer_get_iter_at_mark(buffer, &iter, ins);
    }

    gtk_text_buffer_insert(buffer, &iter, "\n", 1);

    GtkTextChildAnchor *anchor = gtk_text_buffer_create_child_anchor(buffer, &iter);

    gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(tv), table_widget, anchor);
    gtk_widget_show_all(table_widget);

    GtkTextIter after_iter = iter;
    gtk_text_buffer_insert(buffer, &after_iter, "\n\n", -1);

    g_print("Action menu: Tableau... (%dx%d) inséré\n", rows, cols);
    (void)widget;
}

/* ---------- Page setup dialog (format, orientation, margins) ---------- */
static gboolean ask_page_setup_dialog(GtkWindow *parent, int *out_left, int *out_right, int *out_top, int *out_bottom, char **out_format, char **out_orientation) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Mise en page",
                                                    parent,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "_Annuler", GTK_RESPONSE_CANCEL,
                                                    "_Appliquer", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);

    /* Page format */
    GtkWidget *format_label = gtk_label_new("Format :");
    GtkWidget *format_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), "A4");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), "Letter");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format_combo), "Personnalisé");
    gtk_combo_box_set_active(GTK_COMBO_BOX(format_combo), 0);

    /* Orientation */
    GtkWidget *orient_label = gtk_label_new("Orientation :");
    GtkWidget *orient_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(orient_combo), "Portrait");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(orient_combo), "Paysage");
    gtk_combo_box_set_active(GTK_COMBO_BOX(orient_combo), 0);

    /* Margins */
    GtkWidget *left_label = gtk_label_new("Gauche (pt) :");
    GtkWidget *right_label = gtk_label_new("Droite (pt) :");
    GtkWidget *top_label = gtk_label_new("Haut (pt) :");
    GtkWidget *bottom_label = gtk_label_new("Bas (pt) :");

    GtkWidget *left_spin = gtk_spin_button_new_with_range(0, 200, 1);
    GtkWidget *right_spin = gtk_spin_button_new_with_range(0, 200, 1);
    GtkWidget *top_spin = gtk_spin_button_new_with_range(0, 200, 1);
    GtkWidget *bottom_spin = gtk_spin_button_new_with_range(0, 200, 1);

    /* Pre-fill with current margins if available (cast parent to GtkWidget*) */
    int cur_left, cur_right, cur_top, cur_bottom;
    get_margins(GTK_WIDGET(parent), &cur_left, &cur_right, &cur_top, &cur_bottom);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(left_spin), cur_left);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(right_spin), cur_right);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(top_spin), cur_top);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(bottom_spin), cur_bottom);

    /* Layout */
    gtk_grid_attach(GTK_GRID(grid), format_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), format_combo, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), orient_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), orient_combo, 1, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), left_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), left_spin, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), right_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), right_spin, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), top_label, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), top_spin, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), bottom_label, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), bottom_spin, 1, 5, 1, 1);

    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    gboolean ok = FALSE;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        *out_left = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(left_spin));
        *out_right = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(right_spin));
        *out_top = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(top_spin));
        *out_bottom = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(bottom_spin));
        const char *fmt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(format_combo));
        const char *orient = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(orient_combo));
        *out_format = g_strdup(fmt ? fmt : "A4");
        *out_orientation = g_strdup(orient ? orient : "Portrait");
        ok = TRUE;
    }

    gtk_widget_destroy(dialog);
    return ok;
}

/* Apply page setup to all pages */
static void on_page_setup_apply(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    int left, right, top, bottom;
    char *format = NULL;
    char *orientation = NULL;
    if (!ask_page_setup_dialog(GTK_WINDOW(window), &left, &right, &top, &bottom, &format, &orientation)) {
        return;
    }

    set_margins(window, left, right, top, bottom);
    set_page_format(window, format ? format : "A4");
    set_orientation(window, orientation ? orientation : "Portrait");
    /* ensure guides visible when user applies page setup (like OnlyOffice) */
    set_margins_visible(window, TRUE);

    apply_margins_to_all_pages(window);

    g_free(format);
    g_free(orientation);
    g_print("Mise en page appliquée : %d %d %d %d, format=%s, orientation=%s\n", left, right, top, bottom, get_page_format(window), get_orientation(window));
    (void)widget;
}

/* Toggle margins visibility (Afficher/Masquer marges) */
static void on_toggle_margins(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    gboolean visible = get_margins_visible(window);
    visible = !visible;
    set_margins_visible(window, visible);

    apply_margins_to_all_pages(window);

    g_print("Afficher/Masquer marges : %s\n", visible ? "Afficher" : "Masquer");
    (void)widget;
}

/* Implementation of apply_margins_to_all_pages (actual function body) */
void apply_margins_to_all_pages(GtkWidget *window) {
    GtkWidget *pages_box = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "pages_box"));
    if (!pages_box) return;

    int left, right, top, bottom;
    get_margins(window, &left, &right, &top, &bottom);
    gboolean guides_visible = get_margins_visible(window);

    GList *children = gtk_container_get_children(GTK_CONTAINER(pages_box));
    for (GList *l = children; l; l = l->next) {
        GtkWidget *frame = GTK_WIDGET(l->data);
        GtkWidget *tv = GTK_WIDGET(g_object_get_data(G_OBJECT(frame), "page_textview"));
        GtkWidget *guide = GTK_WIDGET(g_object_get_data(G_OBJECT(frame), "margin_guide"));

        if (!tv) tv = gtk_bin_get_child(GTK_BIN(frame));

        if (tv && GTK_IS_TEXT_VIEW(tv)) {
            gtk_text_view_set_left_margin(GTK_TEXT_VIEW(tv), left);
            gtk_text_view_set_right_margin(GTK_TEXT_VIEW(tv), right);
            gtk_text_view_set_top_margin(GTK_TEXT_VIEW(tv), top);
            gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(tv), bottom);
        }

        if (guide) {
            gtk_widget_set_visible(guide, guides_visible);
            gtk_widget_queue_draw(guide);
        }

        if (guides_visible) {
            gtk_style_context_remove_class(gtk_widget_get_style_context(frame), "page-frame-no-border");
            gtk_style_context_add_class(gtk_widget_get_style_context(frame), "page-frame");
        } else {
            gtk_style_context_remove_class(gtk_widget_get_style_context(frame), "page-frame");
            gtk_style_context_add_class(gtk_widget_get_style_context(frame), "page-frame-no-border");
        }
    }
    g_list_free(children);
}

/* ---------- Panneau de conformité rétractable (angle) ---------- */

/* Lance le moteur de règles sur le texte courant et met à jour le panneau */
static void run_rules_and_update_panel(GtkWidget *window) {
    if (!window) return;

    GtkWidget *rules_panel = g_object_get_data(G_OBJECT(window), "rules_panel");
    if (!rules_panel) return;

    GtkWidget *pages_box = GTK_WIDGET(g_object_get_data(G_OBJECT(window), "pages_box"));
    if (!pages_box) return;

    /* 1) récupérer le texte complet */
    GapBuffer *gb = collect_pages_to_gap_buffer(window);
    if (!gb) {
        rules_panel_update_from_report(rules_panel, NULL);
        return;
    }

    char *text = gap_buffer_to_string(gb);
    gap_buffer_destroy(gb);

    if (!text || *text == '\0') {
        rules_panel_update_from_report(rules_panel, NULL);
        if (text) free(text);
        return;
    }

    /* 2) charger les règles */
    const char *json_paths[] = { "data/rules.json", "test_rules.json" };
    RuleReport *report = NULL;

    for (size_t i = 0; i < (sizeof(json_paths) / sizeof(json_paths[0])); i++) {
        report = load_rules(json_paths[i]);
        if (report) break;
    }

    if (!report) {
        rules_panel_update_from_report(rules_panel, NULL);
        free(text);
        return;
    }

    /* 3) exécuter + afficher */
    run_full_diagnostic(report, text);
    rules_panel_update_from_report(rules_panel, report);

    free_rule_report(report);
    free(text);
}

static void on_toggle_rules_panel(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *revealer = g_object_get_data(G_OBJECT(window), "rules_revealer");

    if (!window || !revealer) {
        (void)widget;
        return;
    }

    gboolean visible = gtk_revealer_get_reveal_child(GTK_REVEALER(revealer));
    gboolean new_visible = !visible;
    gtk_revealer_set_reveal_child(GTK_REVEALER(revealer), new_visible);

    /* Exécuter les règles uniquement quand on affiche le panneau */
    if (new_visible) {
        run_rules_and_update_panel(window);
    }

    (void)widget;
}

/* ---------- Help content and About dialogs ---------- */

/* Show a simple help content dialog (scrollable text) */
static void on_show_help_contents(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Contenu de l'aide",
                                                    GTK_WINDOW(window),
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "_Fermer", GTK_RESPONSE_CLOSE,
                                                    NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled, 600, 400);

    GtkWidget *textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD_CHAR);

    const char *help_text =
        "IntelliEditor - Aide\n\n"
        "Principales fonctionnalités:\n"
        "- Fichier: Nouveau, Ouvrir, Enregistrer\n"
        "- Accueil: Copier, Couper, Coller, Gras, Italique, Souligné\n"
        "- Insertion: Saut de page, Image, Tableau\n"
        "- Mise en page: définir marges, format et orientation\n"
        "- Afficher/Masquer marges: bascule l'affichage des guides de marge\n\n"
        "Utilisation rapide:\n"
        "- Pour insérer un tableau: Insertion -> Tableau... puis choisissez lignes/colonnes.\n"
        "- Pour créer un saut de page: Insertion -> Saut de page (crée une nouvelle page sous la page courante).\n"
        "- Pour modifier les marges: Mise en page -> Mise en page... puis Appliquer.\n\n"
        "Raccourcis utiles:\n"
        "- Ctrl+N: Nouveau\n"
        "- Ctrl+O: Ouvrir\n"
        "- Ctrl+S: Enregistrer\n\n"
        "Pour plus d'informations, consultez la documentation du projet ou contactez l'équipe.";

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_set_text(buffer, help_text, -1);

    gtk_container_add(GTK_CONTAINER(scrolled), textview);
    gtk_container_add(GTK_CONTAINER(content), scrolled);
    gtk_widget_show_all(dialog);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    (void)widget;
}

/* Show About dialog using GtkAboutDialog */
static void on_show_about(GtkWidget *widget, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *about = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), "IntelliEditor");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), "0.1");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about), "IntelliEditor is a lightweight GTK-based editor prototype with page model, table insertion and page setup features.");
    const char *authors[] = { "IntelliEditor Team", NULL };
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about), "https://example.org/IntelliEditor");
    gtk_window_set_transient_for(GTK_WINDOW(about), GTK_WINDOW(window));
    gtk_dialog_run(GTK_DIALOG(about));
    gtk_widget_destroy(about);
    (void)widget;
}

/* ---------- Create main window ---------- */
GtkWidget* create_main_window(void) {
    GtkWidget *window, *vbox, *menubar;
    GtkWidget *statusbar, *hbox, *rules_panel;
    GtkWidget *revealer;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "IntelliEditor");
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 700);
    g_signal_connect(window, "destroy", G_CALLBACK(on_quit), window);

    load_css_file("resources/style_light.css");

    set_current_file(window, NULL);
    set_font_size(window, 12);
    set_page_format(window, "A4");
    set_orientation(window, "Portrait");
    set_margins(window, 40, 40, 30, 30); /* default margins in points */
    set_margins_visible(window, TRUE);
    reset_document_formatter(window);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    menubar = gtk_menu_bar_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menubar), "menu-bar");

    GtkWidget *fileMi   = gtk_menu_item_new_with_label("Fichier");
    GtkWidget *homeMi   = gtk_menu_item_new_with_label("Accueil");
    GtkWidget *insertMi = gtk_menu_item_new_with_label("Insertion");
    GtkWidget *layoutMi = gtk_menu_item_new_with_label("Mise en page");
    GtkWidget *refsMi   = gtk_menu_item_new_with_label("Références");
    GtkWidget *viewMi   = gtk_menu_item_new_with_label("Affichage");
    GtkWidget *toolsMi  = gtk_menu_item_new_with_label("Outils");
    GtkWidget *helpMi   = gtk_menu_item_new_with_label("Aide");

    GtkWidget *fileMenu   = gtk_menu_new();
    GtkWidget *homeMenu   = gtk_menu_new();
    GtkWidget *insertMenu = gtk_menu_new();
    GtkWidget *layoutMenu = gtk_menu_new();
    GtkWidget *refsMenu   = gtk_menu_new();
    GtkWidget *viewMenu   = gtk_menu_new();
    GtkWidget *toolsMenu  = gtk_menu_new();
    GtkWidget *helpMenu   = gtk_menu_new();

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(homeMi), homeMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(insertMi), insertMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(layoutMi), layoutMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(refsMi), refsMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(viewMi), viewMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(toolsMi), toolsMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpMi), helpMenu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), homeMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), insertMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), layoutMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), refsMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), viewMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), toolsMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), helpMi);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    GtkWidget *copyMi      = gtk_menu_item_new_with_label("Copier");
    GtkWidget *pasteMi     = gtk_menu_item_new_with_label("Coller");
    GtkWidget *cutMi       = gtk_menu_item_new_with_label("Couper");
    GtkWidget *findMi      = gtk_menu_item_new_with_label("Rechercher...");
    GtkWidget *replaceMi   = gtk_menu_item_new_with_label("Remplacer...");
    GtkWidget *boldMi      = gtk_menu_item_new_with_label("Gras");
    GtkWidget *italicMi    = gtk_menu_item_new_with_label("Italique");
    GtkWidget *underMi     = gtk_menu_item_new_with_label("Souligné");
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), copyMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), pasteMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), cutMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), gtk_separator_menu_item_new());
    GtkWidget *undoMi = gtk_menu_item_new_with_label("Annuler");
    GtkWidget *redoMi = gtk_menu_item_new_with_label("Rétablir");
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), undoMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), redoMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), findMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), replaceMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), boldMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), italicMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), underMi);

    /* Add "Choisir police..." non-intrusif to Accueil */
    GtkWidget *chooseFontMi = gtk_menu_item_new_with_label("Choisir police...");
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(homeMenu), chooseFontMi);

    GtkWidget *newMi  = gtk_menu_item_new_with_label("Nouveau");
    GtkWidget *openMi = gtk_menu_item_new_with_label("Ouvrir...");
    GtkWidget *saveMi = gtk_menu_item_new_with_label("Enregistrer");
    GtkWidget *saveAsMi = gtk_menu_item_new_with_label("Enregistrer sous...");
    GtkWidget *exportMi = gtk_menu_item_new_with_label("Exporter...");
    GtkWidget *quitMi = gtk_menu_item_new_with_label("Quitter");
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), newMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), openMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveAsMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), exportMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);

    GtkWidget *insertPageBreakMi = gtk_menu_item_new_with_label("Saut de page");
    GtkWidget *insertImageMi     = gtk_menu_item_new_with_label("Image...");
    GtkWidget *insertTableMi     = gtk_menu_item_new_with_label("Tableau...");
    gtk_menu_shell_append(GTK_MENU_SHELL(insertMenu), insertPageBreakMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(insertMenu), insertImageMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(insertMenu), insertTableMi);

    GtkWidget *pageSetupMi = gtk_menu_item_new_with_label("Mise en page...");
    gtk_menu_shell_append(GTK_MENU_SHELL(layoutMenu), pageSetupMi);

    GtkWidget *addRefMi = gtk_menu_item_new_with_label("Ajouter référence...");
    GtkWidget *manageRefMi = gtk_menu_item_new_with_label("Gérer références");
    gtk_menu_shell_append(GTK_MENU_SHELL(refsMenu), addRefMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(refsMenu), manageRefMi);

    GtkWidget *toggleMarginsMi = gtk_menu_item_new_with_label("Afficher/Masquer marges");
    GtkWidget *zoomInMi    = gtk_menu_item_new_with_label("Zoom avant");
    GtkWidget *zoomOutMi   = gtk_menu_item_new_with_label("Zoom arrière");
    GtkWidget *zoomResetMi = gtk_menu_item_new_with_label("Zoom par défaut");
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), toggleMarginsMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), zoomInMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), zoomOutMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), zoomResetMi);

    GtkWidget *settingsMi = gtk_menu_item_new_with_label("Paramètres");
    GtkWidget *spellCheckMi = gtk_menu_item_new_with_label("Vérification orthographique");
    GtkWidget *pluginsMi = gtk_menu_item_new_with_label("Gestion des plugins");
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), settingsMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), spellCheckMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), semanticAnalysisMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), pluginsMi);

    GtkWidget *toggleThemeMi = gtk_menu_item_new_with_label("Basculer thème");
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), toggleThemeMi);

    GtkWidget *helpContentsMi = gtk_menu_item_new_with_label("Contenu de l'aide");
    GtkWidget *aboutMi = gtk_menu_item_new_with_label("À propos de IntelliEditor");
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), helpContentsMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), aboutMi);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
    gtk_widget_set_hexpand(hbox, TRUE);
    gtk_widget_set_vexpand(hbox, TRUE);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_widget_set_vexpand(scrolled, TRUE);

    GtkWidget *pages_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_add(GTK_CONTAINER(scrolled), pages_box);

    add_page_at_end(window, pages_box);

    g_object_set_data(G_OBJECT(window), "pages_box", pages_box);

    revealer = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
    gtk_revealer_set_reveal_child(GTK_REVEALER(revealer), FALSE);

    rules_panel = create_rules_panel();
    gtk_container_add(GTK_CONTAINER(revealer), rules_panel);
    gtk_widget_set_size_request(revealer, 320, -1);

    g_object_set_data(G_OBJECT(window), "rules_panel", rules_panel);
    g_object_set_data(G_OBJECT(window), "rules_revealer", revealer);

    gtk_box_pack_start(GTK_BOX(hbox), scrolled, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), revealer, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    statusbar = gtk_statusbar_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(statusbar), "statusbar");
    g_object_set_data(G_OBJECT(window), "statusbar", statusbar);
    gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

    /* Signals */
    g_signal_connect(boldMi, "activate", G_CALLBACK(on_bold), window);
    g_signal_connect(italicMi, "activate", G_CALLBACK(on_italic), window);
    g_signal_connect(underMi, "activate", G_CALLBACK(on_underlined), window);

    g_signal_connect(copyMi, "activate", G_CALLBACK(on_copy), window);
    g_signal_connect(cutMi, "activate", G_CALLBACK(on_cut), window);
    g_signal_connect(pasteMi, "activate", G_CALLBACK(on_paste), window);
    g_signal_connect(undoMi, "activate", G_CALLBACK(on_undo), window);
    g_signal_connect(redoMi, "activate", G_CALLBACK(on_redo), window);
    g_signal_connect(findMi, "activate", G_CALLBACK(search_replace_dialog_show), window);
    g_signal_connect(replaceMi, "activate", G_CALLBACK(search_replace_dialog_show), window);

    g_signal_connect(newMi, "activate", G_CALLBACK(on_new_file), window);
    g_signal_connect(openMi, "activate", G_CALLBACK(on_open_file), window);
    g_signal_connect(saveMi, "activate", G_CALLBACK(on_save_file), window);
    g_signal_connect(saveAsMi, "activate", G_CALLBACK(on_save_file_as), window);
    g_signal_connect(exportMi, "activate", G_CALLBACK(on_export_file), window);
    g_signal_connect(quitMi, "activate", G_CALLBACK(on_quit), NULL);

    g_signal_connect(insertPageBreakMi, "activate", G_CALLBACK(on_insert_page_break), window);
    g_signal_connect(insertImageMi, "activate", G_CALLBACK(on_menu_action_stub), NULL);
    g_signal_connect(insertTableMi, "activate", G_CALLBACK(on_insert_table), window);

    g_signal_connect(pageSetupMi, "activate", G_CALLBACK(on_page_setup_apply), window);

    g_signal_connect(toggleMarginsMi, "activate", G_CALLBACK(on_toggle_margins), window);
    g_signal_connect(zoomInMi, "activate", G_CALLBACK(on_zoom_in), window);
    g_signal_connect(zoomOutMi, "activate", G_CALLBACK(on_zoom_out), window);
    g_signal_connect(zoomResetMi, "activate", G_CALLBACK(on_zoom_reset), window);

    g_signal_connect(addRefMi, "activate", G_CALLBACK(on_menu_action_stub), NULL);
    g_signal_connect(manageRefMi, "activate", G_CALLBACK(on_menu_action_stub), NULL);
    g_signal_connect(settingsMi, "activate", G_CALLBACK(on_menu_action_stub), NULL);
    g_signal_connect(spellCheckMi, "activate", G_CALLBACK(on_check_spelling), window);
    g_signal_connect(semanticAnalysisMi, "activate", G_CALLBACK(on_semantic_analysis), window);
    g_signal_connect(pluginsMi, "activate", G_CALLBACK(on_menu_action_stub), NULL);

    GtkWidget *toggleRulesMi = gtk_menu_item_new_with_label("Afficher panneau de conformité");
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(toolsMenu), toggleRulesMi);
    g_signal_connect(toggleRulesMi, "activate", G_CALLBACK(on_toggle_rules_panel), window);

    g_signal_connect(toggleThemeMi, "activate", G_CALLBACK(on_toggle_theme), NULL);

    g_signal_connect(helpContentsMi, "activate", G_CALLBACK(on_show_help_contents), window);
    g_signal_connect(aboutMi, "activate", G_CALLBACK(on_show_about), window);

    /* Connect choose font menu item (non-intrusive) */
    g_signal_connect(chooseFontMi, "activate", G_CALLBACK(on_choose_font), window);

    GtkAccelGroup *accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel);
    gtk_widget_add_accelerator(newMi, "activate", accel, GDK_KEY_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(openMi, "activate", accel, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(saveMi, "activate", accel, GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(undoMi, "activate", accel, GDK_KEY_z, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(redoMi, "activate", accel, GDK_KEY_y, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(quitMi, "activate", accel, GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    nlp_system_init();
    apply_margins_to_all_pages(window);

    return window;
}
