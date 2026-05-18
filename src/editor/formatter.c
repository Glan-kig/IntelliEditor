/**
 * @file formatter.c
 * @brief Moteur de formatage inline.
 *
 * Les styles sont stockés dans une liste chaînée de ranges [start, end[
 * avec un masque de flags FormatStyle. Plusieurs ranges peuvent se
 * chevaucher (un caractère peut être gras ET italique simultanément).
 *
 * @author DEV-A
 */
#include "formatter.h"
#include "memory.h"
#include <string.h>

/** Un range de formatage. */
typedef struct Range {
    size_t        start;
    size_t        end;     /**< Exclusif */
    unsigned int  style;
    struct Range *next;
} Range;

struct Formatter {
    Range *head;
};

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

Formatter *formatter_create(void) {
    return MEM_CALLOC(1, sizeof(Formatter));
}

void formatter_destroy(Formatter *fmt) {
    if (!fmt) return;
    Range *r = fmt->head;
    while (r) {
        Range *next = r->next;
        MEM_FREE(r);
        r = next;
    }
    MEM_FREE(fmt);
}

bool formatter_apply(Formatter *fmt, size_t start, size_t end,
                     unsigned int style) {
    if (!fmt || start >= end) return false;

    Range *r = MEM_ALLOC(sizeof(Range));
    if (!r) return false;
    r->start = start;
    r->end   = end;
    r->style = style;
    r->next  = fmt->head;
    fmt->head = r;
    return true;
}

bool formatter_remove(Formatter *fmt, size_t start, size_t end,
                      unsigned int style) {
    if (!fmt || start >= end) return false;

    Range **cur = &fmt->head;
    while (*cur) {
        Range *r = *cur;
        /* Ne traiter que les ranges qui contiennent le style ciblé */
        if (!(r->style & style) || r->end <= start || r->start >= end) {
            cur = &r->next;
            continue;
        }

        /* Cas 1 : range entièrement inclus → on retire le style */
        if (r->start >= start && r->end <= end) {
            r->style &= ~style;
            if (r->style == 0) {
                *cur = r->next;
                MEM_FREE(r);
                continue;
            }
        }
        /* Cas 2 : range déborde à gauche → tronquer ou splitter */
        else if (r->start < start && r->end <= end) {
            /* Créer un nouveau range pour la partie sans le style */
            Range *right = MEM_ALLOC(sizeof(Range));
            if (!right) return false;
            right->start = start;
            right->end   = r->end;
            right->style = r->style & ~style;
            right->next  = r->next;
            r->end       = start;
            if (right->style != 0) {
                r->next = right;
            } else {
                MEM_FREE(right);
            }
        }
        /* Cas 3 : range déborde à droite */
        else if (r->start >= start && r->end > end) {
            Range *left = MEM_ALLOC(sizeof(Range));
            if (!left) return false;
            left->start = r->start;
            left->end   = end;
            left->style = r->style & ~style;
            left->next  = r;
            r->start    = end;
            *cur        = left->style != 0 ? left : r;
            if (left->style == 0) MEM_FREE(left);
        }
        /* Cas 4 : range englobe complètement → split en 3 */
        else {
            Range *middle = MEM_ALLOC(sizeof(Range));
            Range *right  = MEM_ALLOC(sizeof(Range));
            if (!middle || !right) {
                MEM_FREE(middle); MEM_FREE(right);
                return false;
            }
            middle->start = start;
            middle->end   = end;
            middle->style = r->style & ~style;
            right->start  = end;
            right->end    = r->end;
            right->style  = r->style;
            right->next   = r->next;
            middle->next  = right;
            r->end        = start;
            r->next       = (middle->style != 0) ? middle : right;
            if (middle->style == 0) MEM_FREE(middle);
        }
        cur = &(*cur)->next;
    }
    return true;
}

unsigned int formatter_get_style_at(const Formatter *fmt, size_t pos) {
    if (!fmt) return STYLE_NONE;
    unsigned int combined = STYLE_NONE;
    for (const Range *r = fmt->head; r; r = r->next) {
        if (pos >= r->start && pos < r->end) {
            combined |= r->style;
        }
    }
    return combined;
}

bool formatter_has_style(const Formatter *fmt, size_t pos,
                         unsigned int style) {
    if (!fmt) return false;
    return (formatter_get_style_at(fmt, pos) & style) == style;
}

size_t formatter_range_count(const Formatter *fmt) {
    if (!fmt) return 0;
    size_t n = 0;
    for (const Range *r = fmt->head; r; r = r->next) n++;
    return n;
}

void formatter_iterate(const Formatter *fmt,
                       FormatterIterFn fn, void *user_data) {
    if (!fmt || !fn) return;
    for (const Range *r = fmt->head; r; r = r->next) {
        fn(r->start, r->end, r->style, user_data);
    }
}
