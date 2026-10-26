#include "lfo.hpp"
#include "stock.hpp"

namespace opxy { namespace platform { namespace lfo {
u32 gui_engine(u32 track) { return track_engine(track); }
u32 audio_engine(u32 track) { return word(0x11b01c48 + track * 4); }
u32 type(u32 track) { return stock<u32(u32)>(0x8d585fb8)(track); }
u32 group_value(u32 track) {
    return stock<u32(u32, u32, u32)>(0x8d5aac40)(0x86bd9f38, track, 26);
}
i32 step(u32 track, u32 index, i32 value, const i32* event) {
    return stock<i32(u32, u32, i32, const i32*)>(0x8d5ab618)(track, index, value, event);
}
void modulate(u32 project, u32 track, u32 index, i32 amount) {
    stock<void(u32, u32, u32, i32)>(0x8d5182d4)(project, track, index, amount);
}
}}}
