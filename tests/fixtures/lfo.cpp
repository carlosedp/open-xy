// Exercise selection -> stored Q31 value -> audio destination with the real
// routing code. Boundary doubles fail if the audio path asks a GUI getter.
#include "lfo.hpp"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

#define require(v) do { if (!(v)) { std::fprintf(stderr, "line %d\n", __LINE__); std::abort(); } } while (0)
using namespace opxy;
using namespace opxy::platform;
static int ownership = 1;
static int ready() { return ownership; }
const LfoMachine sdk_lfo_machines[] = {
    {reinterpret_cast<const char*>(40), ready, 4},
    {reinterpret_cast<const char*>(41), ready, 5},
    {reinterpret_cast<const char*>(42), ready, 6},
    {reinterpret_cast<const char*>(43), ready, 7},
    {reinterpret_cast<const char*>(44), ready, 8},
};
const u32 sdk_lfo_machine_count = 5;
static u32 engine, kind, group, routed, gui_reads, delegated, expected_track;
static i32 amount;
static bool audio;
namespace opxy { namespace platform { namespace lfo {
u32 gui_engine(u32 track) { require(!audio && track == expected_track); ++gui_reads; return engine; }
u32 audio_engine(u32 track) { require(audio && track == expected_track); return engine; }
u32 type(u32 track) { require(!audio && track == expected_track); return kind; }
u32 group_value(u32 track) { require(!audio && track == expected_track); return group; }
i32 step(u32, u32, i32 value, const i32*) { ++delegated; return value; }
void modulate(u32 project, u32 track, u32 index, i32 value) {
    require(audio && project == 123 && track == expected_track);
    routed = index; amount = value;
}
}}}
static void route(u32 track, i32 value, u32 expected) {
    audio = true;
    const u32 before = gui_reads;
    sdk_lfo_modulate(123, track, static_cast<u32>(value) >> 29, -1234, static_cast<u32>(value) >> 8);
    require(routed == expected && amount == -1234 && gui_reads == before);
    audio = false;
}
int main() {
    const i32 up[] = {0, 1, 0}, down[] = {0, -1, 0};
    for (u32 track = 0; track < 8; ++track) {
        expected_track = track;
        for (u32 count = 5; count <= 8; ++count) {
            engine = 36 + count;
            for (u32 lfo_type : {1u, 2u, 3u}) {
                kind = lfo_type; group = 0;
                const u32 index = kind == 2 ? 31 : 27;
                require(sdk_lfo_range(track, 0, 0) == 1);
                require(sdk_lfo_range(track, 1, 1) == 0);
                i32 value = 0;
                for (u32 slot = 0; slot < count; ++slot) {
                    if (slot) value = sdk_lfo_step(track, index, value, up);
                    require(static_cast<u32>(value) >> 28 == slot);
                    route(track, value, slot);
                }
                value = sdk_lfo_step(track, index, value, up);
                require(static_cast<u32>(value) >> 28 == count - 1);
                for (u32 slot = count - 1; slot > 0; --slot) {
                    value = sdk_lfo_step(track, index, value, down);
                    require(static_cast<u32>(value) >> 28 == slot - 1);
                    route(track, value, slot - 1);
                }
                require(static_cast<u32>(sdk_lfo_step(track, index, value, down)) >> 28 == 0);
                const i32 fast[] = {0, 1, 1}, huge[] = {0, 0x7fffffff, 1}, negative[] = {0, (-2147483647 - 1), 1};
                require(static_cast<u32>(sdk_lfo_step(track, index, 0, fast)) >> 28 == 2);
                require(static_cast<u32>(sdk_lfo_step(track, index, 0, huge)) >> 28 == count - 1);
                require(static_cast<u32>(sdk_lfo_step(track, index, 0x7fffffff, negative)) >> 28 == 0);
                route(track, 0x7fffffff, count - 1); // a saved destination from a larger machine
                const u32 calls = delegated;
                sdk_lfo_step(track, index == 27 ? 31 : 27, 123, up);
                sdk_lfo_step(track, index, 123, nullptr);
                group = 0x7fffffff;
                sdk_lfo_step(track, index, 123, up);
                require(delegated == calls + 3);
            }
        }
    }
    expected_track = 0; group = 0; kind = 1;
    for (u32 id : {2u, 3u, 18u, 33u, 40u, 44u, 62u}) {
        engine = id;
        for (int state : {0, 1, 2, -1}) {
            ownership = state;
            const bool owned = id == 44 && state == 1;
            require(sdk_lfo_range(0, 0, 1) == static_cast<u32>(owned));
            const u32 calls = delegated;
            sdk_lfo_step(0, 27, 0, up);
            require(delegated == calls + (owned ? 0 : 1));
            route(0, 0x78000000, owned ? 7 : 3);
        }
    }
    engine = 44; ownership = 1;
    for (u32 other : {0u, 4u, 5u, 6u, 7u}) {
        kind = other;
        const u32 calls = delegated;
        sdk_lfo_step(0, 27, 123, up); sdk_lfo_step(0, 31, 123, up);
        require(delegated == calls + 2);
    }
    for (u32 track : {8u, 10u, 14u, 15u, 0xffffffffu}) {
        expected_track = track;
        require(sdk_lfo_range(track, 0, 0) == 0);
        const u32 calls = delegated;
        sdk_lfo_step(track, 27, 123, up);
        require(delegated == calls + 1);
        route(track, 0x78000000, 3);
    }
}
