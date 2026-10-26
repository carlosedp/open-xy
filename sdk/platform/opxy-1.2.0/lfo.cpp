#include "lfo.hpp"

namespace {
using namespace opxy;
using namespace opxy::platform;
u32 extended(u32 engine) {
    for (u32 i = 0; i < sdk_lfo_machine_count; ++i) {
        const LfoMachine& m = sdk_lfo_machines[i];
        if (static_cast<u32>(reinterpret_cast<__UINTPTR_TYPE__>(m.id)) == engine)
            return m.parameters > 4 && m.parameters <= parameter_slots && m.ready() == 1 ? m.parameters : 0;
    }
    return 0;
}
bool extra_icon(u32 group, u32 slot) {
    if (group != 0 || slot < page_controls || slot >= parameter_slots) return false;
    const u32 track = lfo::selected_track();
    return track < 8 && slot < extended(lfo::gui_engine(track));
}
}

// All four stock display callers use this result to choose eight positions
// instead of dividing the decoded selector by two. Stock's function always
// returns zero; keep that result outside an owned engine's destination group.
u32 sdk_lfo_range(u32 track, u32 group, u32) {
    return track < 8 && group == 0 && extended(lfo::gui_engine(track)) ? 1 : 0;
}

// Stock scrolls a strip containing four knob icons by -spacing * slot.
// The three callers pass their already-decoded group and full destination;
// only the icon scroll repeats. Labels, cached selection and audio keep slot.
u32 sdk_lfo_icon_slide(u32 controller, u32 position, i32 target, u32 duration, u32 group, u32 slot) {
    if (extra_icon(group, slot))
        target = (target / static_cast<i32>(slot)) * static_cast<i32>(slot % page_controls);
    return lfo::animate(controller, position, target, duration);
}

// Element additionally hides its strip when slot > 3, before naming the
// destination. Restore visibility for a declared extra slot; name it as-is.
const char* sdk_lfo_element_label(u32 group, u32 slot, u32 flag, u32 widget) {
    if (extra_icon(group, slot)) lfo::show(widget);
    return lfo::name(group, slot, flag);
}

i32 sdk_lfo_step(u32 track, u32 index, i32 value, const i32* event) {
    if (track < 8 && event && (index == 27 || index == 31)) {
        const u32 kind = lfo::type(track);
        const bool target = ((kind == 1 || kind == 3) && index == 27) || (kind == 2 && index == 31);
        const u32 count = target ? extended(lfo::gui_engine(track)) : 0;
        if (count) {
            const u32 group_value = lfo::group_value(track);
            const u32 group = kind == 3 ? group_value >> 29 : ((group_value >> 8) * 6) >> 24;
            if (group == 0) {
                i32 slot = value < 0 ? 0 : static_cast<u32>(value) >> 28;
                if (slot >= static_cast<i32>(count)) slot = count - 1;
                // Match stock's discrete detents and accelerated turns. Bound
                // before multiplying so even a bad event cannot overflow.
                i32 delta = event[1];
                if (delta > 8) delta = 8;
                if (delta < -8) delta = -8;
                slot += delta * (event[2] > 0 ? 2 : 1);
                if (slot < 0) slot = 0;
                if (slot >= static_cast<i32>(count)) slot = count - 1;
                return static_cast<i32>((static_cast<u32>(slot) << 28) + 0x08000000u);
            }
        }
    }
    return lfo::step(track, index, value, event);
}

// Only the engine-group modulation call is hooked. The caller keeps the
// original selector >> 8 in R4; its R2 has already lost the third slot bit.
// Read the audio engine table, never the GUI/project getters, on this path.
void sdk_lfo_modulate(u32 project, u32 track, u32 index, i32 amount, u32 selector_q23) {
    const u32 count = track < 8 ? extended(lfo::audio_engine(track)) : 0;
    if (count) {
        index = selector_q23 >> 20;
        if (index >= count) index = count - 1;
    }
    lfo::modulate(project, track, index, amount);
}
