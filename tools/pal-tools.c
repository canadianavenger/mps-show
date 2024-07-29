#include "pal-tools.h"

void pal_to_pal(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries, int in_max, int out_max) {
    for(int i = 0; i < entries; i++) {
        uint16_t comp = in_pal[i].r;
        comp *= out_max;
        comp /= in_max;
        out_pal[i].r = comp;

        comp = in_pal[i].g;
        comp *= out_max;
        comp /= in_max;
        out_pal[i].g = comp;

        comp = in_pal[i].b;
        comp *= out_max;
        comp /= in_max;
        out_pal[i].b = comp;
    }
}

void pal4_to_pal6(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries) {
    return pal_to_pal(in_pal, out_pal, entries, ((1 << 4) - 1), ((1 << 6) -1));
}

void pal4_to_pal8(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries) {
    return pal_to_pal(in_pal, out_pal, entries, ((1 << 4) - 1), ((1 << 8) -1));
}

void pal6_to_pal8(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries) {
    return pal_to_pal(in_pal, out_pal, entries, ((1 << 6) - 1), ((1 << 8) -1));
}

void pal6_to_pal4(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries) {
    return pal_to_pal(in_pal, out_pal, entries, ((1 << 6) - 1), ((1 << 4) -1));
}

void pal8_to_pal4(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries) {
    return pal_to_pal(in_pal, out_pal, entries, ((1 << 8) - 1), ((1 << 4) -1));
}

void pal8_to_pal6(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries) {
    return pal_to_pal(in_pal, out_pal, entries, ((1 << 8) - 1), ((1 << 6) -1));
}