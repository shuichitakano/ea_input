#include "macro_profile.h"
#include "rapid_timing.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace
{
void append16(std::vector<uint8_t> &out, uint16_t value)
{
    out.push_back(value & 0xff);
    out.push_back(value >> 8);
}

void append24(std::vector<uint8_t> &out, uint32_t value)
{
    out.push_back(value & 0xff);
    out.push_back((value >> 8) & 0xff);
    out.push_back((value >> 16) & 0xff);
}

void appendString(std::vector<uint8_t> &out, const char *value)
{
    out.insert(out.end(), value, value + std::strlen(value));
}

void addSection(std::vector<uint8_t> &out, uint8_t type,
                const std::vector<uint8_t> &payload)
{
    out.push_back(type);
    out.push_back(0);
    append16(out, payload.size());
    out.insert(out.end(), payload.begin(), payload.end());
}

void write32(std::vector<uint8_t> &out, size_t offset, uint32_t value)
{
    out[offset] = value;
    out[offset + 1] = value >> 8;
    out[offset + 2] = value >> 16;
    out[offset + 3] = value >> 24;
}

std::vector<uint8_t> makeProfileBytes()
{
    std::vector<uint8_t> bytes{'A', 'M', 'A', 'P', 1, 0, 16, 0,
                               0, 0, 0, 0, 0, 0, 0, 0};
    std::vector<uint8_t> direct;
    for (int i = 0; i < 32; ++i)
        append24(direct, i < 12 ? 1u << i : 0);
    addSection(bytes, 1, direct);

    std::vector<uint8_t> bindings;
    append16(bindings, 1);
    bindings.insert(bindings.end(), {6, 3, 0, 0});
    addSection(bytes, 2, bindings);

    std::vector<uint8_t> definitions{1, 3, 2, 0, 0};
    append24(definitions, 1u << 2);
    append16(definitions, 1);
    append24(definitions, 1u << 5);
    append16(definitions, 2);
    addSection(bytes, 3, definitions);

    std::vector<uint8_t> selectors{1, 0, 7, 8, 0, 1, 0, 1, 1, 2, 0};
    append24(selectors, 1u << 6);
    append24(selectors, 1u << 7);
    addSection(bytes, 4, selectors);

    std::vector<uint8_t> rapid;
    for (int i = 0; i < 32; ++i)
        rapid.insert(rapid.end(), {0, 0, 2});
    addSection(bytes, 5, rapid);
    addSection(bytes, 6, {2, 0});
    addSection(bytes, 7, {2, 0});

    constexpr const char *profileName = "Parser Fixture";
    constexpr const char *description = "Full parser coverage";
    std::vector<uint8_t> metadata{1, 0};
    append16(metadata, std::strlen(profileName));
    append16(metadata, std::strlen(description));
    metadata.insert(metadata.end(), {1, 2, 1, 0});
    appendString(metadata, profileName);
    appendString(metadata, description);
    metadata.push_back(3);
    append16(metadata, 4);
    appendString(metadata, "Fire");
    append16(metadata, 8);
    appendString(metadata, "Set Zero");
    append16(metadata, 7);
    appendString(metadata, "Set One");
    metadata.push_back(0);
    append16(metadata, 4);
    metadata.push_back(2);
    appendString(metadata, "Gear");
    append16(metadata, 3);
    appendString(metadata, "Low");
    append16(metadata, 4);
    appendString(metadata, "High");
    addSection(bytes, 8, metadata);

    write32(bytes, 8, bytes.size());
    write32(bytes, 12, Macro::crc32(bytes.data() + 16, bytes.size() - 16));
    return bytes;
}

std::vector<uint8_t> makeV11ProfileBytes()
{
    std::vector<uint8_t> bytes{'A', 'M', 'A', 'P', 1, 1, 16, 0,
                               0, 0, 0, 0, 0, 0, 0, 0};
    std::vector<uint8_t> direct;
    for (int i = 0; i < 32; ++i)
        append24(direct, i == 6 ? 1u << 4 : 0);
    addSection(bytes, 1, direct);

    std::vector<uint8_t> bindings;
    append16(bindings, 1);
    bindings.insert(bindings.end(), {6, 0, 0, 0x11}); // loop + loop sync
    addSection(bytes, 2, bindings);

    constexpr uint32_t directions = 0x3cu;
    std::vector<uint8_t> definitions{1, 0, 2, 0, 1};
    append24(definitions, directions);
    definitions.push_back(0);
    append24(definitions, 1u << 2);
    append16(definitions, 1);
    append24(definitions, 1u << 5);
    append16(definitions, 3);
    addSection(bytes, 3, definitions);

    std::vector<uint8_t> selectors{1, 0, 7, 8, 0, 0, 0, 0, 0, 1, 0};
    append24(selectors, 1u << 6);
    append24(selectors, 1u << 7);
    addSection(bytes, 4, selectors);

    std::vector<uint8_t> rapid;
    for (int i = 0; i < 32; ++i)
        rapid.insert(rapid.end(), {0, 0, 2});
    addSection(bytes, 5, rapid);
    addSection(bytes, 6, {1, 0});
    addSection(bytes, 7, {1, 0});

    write32(bytes, 8, bytes.size());
    write32(bytes, 12, Macro::crc32(bytes.data() + 16, bytes.size() - 16));
    return bytes;
}

void parserTests()
{
    auto bytes = makeProfileBytes();
    Macro::Profile profile;
    assert(Macro::parse(bytes.data(), bytes.size(), profile) == Macro::Result::OK);
    assert(profile.setCount == 2);
    assert(profile.frameStep == 2);
    assert(profile.name == "Parser Fixture");
    assert(profile.mappings[6] == (1u << 6));
    assert(profile.sequences.size() == 1);
    assert(profile.sequences[0].name == "Fire");
    assert(profile.sequences[0].composition == 1);
    assert(profile.sequences[0].suppression == 0x3c);
    assert(profile.bindings.size() == 1);
    assert(profile.selectors.size() == 1);
    assert(profile.selectors[0].stateNames[1] == "High");

    auto damaged = bytes;
    damaged.back() ^= 1;
    assert(Macro::parse(damaged.data(), damaged.size(), profile) == Macro::Result::BAD_CRC);

    damaged = bytes;
    damaged[4] = 2;
    assert(Macro::parse(damaged.data(), damaged.size(), profile) == Macro::Result::BAD_HEADER);

    auto v11 = makeV11ProfileBytes();
    assert(Macro::parse(v11.data(), v11.size(), profile) == Macro::Result::OK);
    assert(profile.sequences[0].composition == 1);
    assert(profile.sequences[0].suppression == 0x3c);
    assert(profile.bindings[0].flags == 0x11);
    assert(profile.selectors[0].occupancy == (1u << 6));

    damaged = v11;
    damaged[5] = 2; // unknown minor versions are never inferred from section sizes
    write32(damaged, 12, Macro::crc32(damaged.data() + 16, damaged.size() - 16));
    assert(Macro::parse(damaged.data(), damaged.size(), profile) == Macro::Result::BAD_HEADER);

    // Exercise malformed TLV lengths, counts, IDs and masks under sanitizers.
    uint32_t random = 0x13579bdf;
    for (int iteration = 0; iteration < 2000; ++iteration)
    {
        damaged = bytes;
        random = random * 1664525u + 1013904223u;
        const size_t offset = 16 + random % (damaged.size() - 16);
        random = random * 1664525u + 1013904223u;
        damaged[offset] ^= static_cast<uint8_t>(random | 1);
        write32(damaged, 12, Macro::crc32(damaged.data() + 16, damaged.size() - 16));
        (void)Macro::parse(damaged.data(), damaged.size(), profile);
    }
}

void runtimeTests()
{
    Macro::Profile profile;
    profile.setCount = 2;
    profile.frameStep = 2;
    profile.setNames = {"One", "Two"};
    profile.mappings[6] = 1u << 6;
    for (auto &rapid : profile.rapidFire)
        rapid = {false, 0, 2};
    profile.sequences.push_back({3, 0, {{1u << 2, 1}, {1u << 5, 2}}, "Test"});
    profile.bindings.push_back({6, 3, 1, 0});

    Macro::PlayerRuntime player;
    player.attach(&profile);
    // Set 0 has no sequence binding; inherited direct mapping still applies.
    assert(player.processFrame(1u << 6, 1u << 6, 0) == (1u << 6));
    player.processFrame(0, 0, 1);

    player.changeSet(1);
    assert(player.currentSet() == 1);
    assert(player.processFrame(1u << 6, 1u << 6, 2) == ((1u << 6) | (1u << 2)));
    assert(player.processFrame(1u << 6, 1u << 6, 3) == ((1u << 6) | (1u << 2)));
    assert(player.processFrame(0, 0, 4) == (1u << 5));
    assert(player.processFrame(0, 0, 5) == (1u << 5));
    assert(player.processFrame(0, 0, 6) == (1u << 5));
    assert(player.processFrame(0, 0, 7) == (1u << 5));
    assert(player.processFrame(0, 0, 8) == 0);

    player.changeSet(1);
    assert(player.currentSet() == 0); // wraps
    player.changeSet(-1);
    assert(player.currentSet() == 1); // wraps backwards
}

void runtimeModeTests()
{
    Macro::Profile profile;
    profile.setCount = 1;
    profile.frameStep = 1;
    profile.setNames = {"Set"};
    for (auto &rapid : profile.rapidFire)
        rapid = {false, 0, 2};

    // Horizontal transform applies independently to both output halves.
    profile.sequences = {{0, 0, {{(1u << 4) | (1u << 16), 1}}, "Flip"}};
    profile.bindings = {{6, 0, 0, 4}};
    Macro::PlayerRuntime player;
    player.attach(&profile);
    assert(player.processFrame(1u << 6, 0, 0) == ((1u << 5) | (1u << 17)));
    assert(player.processFrame(0, 0, 1) == 0);

    // A held-loop binding finishes its current loop after release.
    profile.sequences = {{0, 0, {{1u << 6, 2}}, "Loop"}};
    profile.bindings = {{6, 0, 0, 1}};
    player.attach(&profile);
    assert(player.processFrame(1u << 6, 0, 0) == (1u << 6));
    assert(player.processFrame(1u << 6, 0, 1) == (1u << 6));
    assert(player.processFrame(1u << 6, 0, 2) == (1u << 6));
    assert(player.processFrame(0, 0, 3) == (1u << 6));
    assert(player.processFrame(0, 0, 4) == 0);

    // Immediate cancellation removes output on the release frame.
    profile.bindings = {{6, 0, 0, 3}};
    player.attach(&profile);
    assert(player.processFrame(1u << 6, 0, 0) == (1u << 6));
    assert(player.processFrame(0, 0, 1) == 0);

    // Selector transitions are edge-based and honor their neutral gap.
    profile.bindings.clear();
    profile.sequences.clear();
    profile.selectors = {{0, 7, 8, 0, 1, 0, true, 1,
                          {1u << 6, 1u << 7}, "Gear", {"Low", "High"}}};
    player.attach(&profile);
    assert(player.processFrame(0, 0, 0) == (1u << 6));
    assert(player.processFrame(1u << 7, 0, 1) == 0);
    assert(player.processFrame(1u << 7, 0, 2) == (1u << 7));
    player.processFrame(0, 0, 3);
    assert(player.processFrame(1u << 7, 0, 4) == 0); // wraps to state 0
    assert(player.processFrame(1u << 7, 0, 5) == (1u << 6));

    // Override-disabled means raw passthrough even when inherited rapid is off.
    profile.selectors.clear();
    profile.mappings[6] = 1u << 6;
    profile.rapidFire[6] = {true, 0, 2};
    player.attach(&profile);
    assert(player.processFrame(1u << 6, 0, 0) == (1u << 6));
    profile.rapidFire[6] = {true, 1, 2};
    player.attach(&profile);
    assert(player.processFrame(0, 0, 0) == 0);
    assert(player.processFrame(1u << 6, 0, 1) == (1u << 6));
    assert(player.processFrame(1u << 6, 0, 2) == 0);
    assert(player.processFrame(1u << 6, 0, 3) == (1u << 6));
    assert(player.processFrame(0, 0, 4) == 0);
    assert(player.processFrame(1u << 6, 0, 5) == (1u << 6)); // re-press resets to ON

    assert(RapidTiming::globalPhaseOn(0, 2, false));
    assert(!RapidTiming::globalPhaseOn(1, 2, false));
    assert(RapidTiming::globalPhaseOn(2, 2, false));
    assert(!RapidTiming::globalPhaseOn(2, 2, true));
    assert(RapidTiming::synchronizedOn(7, 7, 2));
    assert(!RapidTiming::synchronizedOn(8, 7, 2));
    assert(RapidTiming::synchronizedOn(9, 7, 2));
}

void rapidPeriodTests()
{
    // Exercise the actual runtime for every supported period and phase.
    // Count rising edges over 60 cycles: a doubled period produces only 30.
    for (uint8_t period = 2; period <= 60; ++period)
        for (uint8_t type = 1; type <= 3; ++type)
        {
            Macro::Profile profile;
            profile.setCount = 1;
            profile.frameStep = 1;
            profile.mappings[6] = 1u << 6;
            profile.rapidFire[6] = {true, type, period};
            Macro::PlayerRuntime player;
            player.attach(&profile);
            const uint32_t start = 7;
            player.processFrame(0, 0, start - 1);
            unsigned edges = 0;
            bool previous = false;
            for (uint32_t offset = 0; offset < 61u * period; ++offset)
            {
                const uint32_t frame = start + offset;
                const bool on = player.processFrame(1u << 6, 0, frame) != 0;
                const uint32_t phase = (type == 1 ? offset : frame) % period;
                const bool front = phase < (period + 1u) / 2;
                assert(on == (type == 3 ? !front : front));
                if (offset >= period && on && !previous)
                    ++edges;
                previous = on;
            }
            assert(edges == 60);
        }

    // Legacy divisors still represent ON and OFF durations separately.
    for (int divisor = 1; divisor <= 4; ++divisor)
        for (uint32_t elapsed = 0; elapsed < 32; ++elapsed)
            assert(RapidTiming::synchronizedOn(7 + elapsed, 7, 2 * divisor) ==
                   ((elapsed / divisor) % 2 == 0));

    // Synchronized elapsed time also works across the frame counter wrap.
    assert(RapidTiming::synchronizedOn(0, UINT32_MAX, 3));
    assert(!RapidTiming::synchronizedOn(1, UINT32_MAX, 3));
    assert(RapidTiming::synchronizedOn(2, UINT32_MAX, 3));
}

void synchronizedStopTests()
{
    for (uint8_t scale : {1, 3, 255})
        for (uint16_t ticks : {1, 4})
        {
            Macro::Profile profile;
            profile.setCount = 1;
            profile.frameStep = scale;
            profile.sequences = {{0, 0, {{4, ticks}}, "Sync"}};
            profile.bindings = {{6, 0, 0, 0x11}};
            const uint32_t period = scale * ticks;
            // Release both at and inside a cycle, including tick zero of
            // multi-frame ticks. Playback must finish exactly that cycle.
            for (uint32_t release : {1u, period, period + 1})
            {
                Macro::PlayerRuntime player;
                player.attach(&profile);
                const uint32_t stop = ((release + period - 1) / period) * period;
                for (uint32_t frame = 0; frame <= stop + period; ++frame)
                    assert(player.processFrame(frame < release ? 64 : 0, 0, frame) ==
                           (frame < stop ? 4u : 0u));
                // Re-press starts playback again after a completed stop.
                assert(player.processFrame(64, 0, stop + period + 1) == 4);
            }
        }

    Macro::Profile profile;
    profile.setCount = 1;
    profile.frameStep = 1;
    profile.sequences = {{0, 0, {{4, 1}, {8, 3}}, "Sync"}};
    profile.bindings = {{6, 0, 0, 0x11}};
    for (uint32_t next : {5u, 7u, 11u})
    {
        Macro::PlayerRuntime player;
        player.attach(&profile);
        assert(player.processFrame(64, 0, 2) == 8);
        assert(player.processFrame(0, 0, 3) == 8);
        assert(player.processFrame(0, 0, next) == 0);
    }
    Macro::PlayerRuntime player;
    player.attach(&profile);
    assert(player.processFrame(64, 0, UINT32_MAX - 1) == 8);
    assert(player.processFrame(0, 0, UINT32_MAX) == 8);
    assert(player.processFrame(0, 0, 0) == 0);

    // A held input must continue playing through missed frames.
    player.attach(&profile);
    assert(player.processFrame(64, 0, 2) == 8);
    assert(player.processFrame(64, 0, 9) == 8);
    assert(player.processFrame(64, 0, 12) == 4);
}

void v11RuntimeTests()
{
    Macro::Profile profile;
    profile.setCount = 1;
    profile.frameStep = 1;
    for (auto &rapid : profile.rapidFire)
        rapid = {false, 0, 2};

    // Automatic/custom suppression replaces only its owned outputs.
    profile.mappings[6] = (1u << 4) | (1u << 6);
    profile.sequences = {{0, 0, {{1u << 2, 3}}, "Suppress", 1, 0x3c}};
    profile.bindings = {{6, 0, 0, 0}};
    Macro::PlayerRuntime player;
    player.attach(&profile);
    assert(player.processFrame(1u << 6, 1u << 6, 0) == ((1u << 2) | (1u << 6)));

    // Sequences started on one frame are one composition rank, independent of
    // binding order. A later rank overrides directions from the older rank.
    profile.mappings = {};
    profile.sequences = {
        {0, 0, {{1u << 4, 4}}, "Left", 2, 0x3c},
        {1, 0, {{1u << 5, 4}}, "Right", 2, 0x3c},
        {2, 0, {{1u << 2, 4}}, "Down", 2, 0x3c},
    };
    profile.bindings = {{6, 0, 0, 0}, {6, 1, 0, 0}, {7, 2, 0, 0}};
    player.attach(&profile);
    assert(player.processFrame(1u << 6, 0, 10) == ((1u << 4) | (1u << 5)));
    assert(player.processFrame((1u << 6) | (1u << 7), 0, 11) == (1u << 2));
    std::reverse(profile.bindings.begin(), profile.bindings.end());
    player.attach(&profile);
    assert(player.processFrame(1u << 6, 0, 10) == ((1u << 4) | (1u << 5)));
    assert(player.processFrame((1u << 6) | (1u << 7), 0, 11) == (1u << 2));

    // Selector occupancy clears direct/sequence output before state output.
    profile.sequences.clear();
    profile.bindings.clear();
    profile.mappings[6] = 1u << 6;
    profile.selectors = {{0, 7, 8, 0, 0, 0, false, 0,
                          {1u << 7}, "Mode", {"Only"}, 1u << 6}};
    player.attach(&profile);
    assert(player.processFrame(1u << 6, 1u << 6, 0) == (1u << 7));

    // Loop Sync uses the global VSync phase and finishes before the next tick 0.
    profile.selectors.clear();
    profile.mappings = {};
    profile.sequences = {{0, 0, {{1u << 6, 1}, {0, 3}}, "Sync"}};
    profile.bindings = {{6, 0, 0, 0x11}};
    player.attach(&profile);
    assert(player.processFrame(1u << 6, 0, 2) == 0);
    assert(player.processFrame(0, 0, 3) == 0);
    assert(player.processFrame(0, 0, 4) == 0); // stopped before synchronized tick 0

    player.attach(&profile);
    assert(player.processFrame(1u << 6, 0, 4) == (1u << 6));
}
}

int main(int argc, char **argv)
{
    parserTests();
    runtimeTests();
    runtimeModeTests();
    rapidPeriodTests();
    synchronizedStopTests();
    v11RuntimeTests();
    if (argc > 1)
    {
        std::ifstream input(argv[1], std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                   std::istreambuf_iterator<char>());
        Macro::Profile profile;
        assert(Macro::parse(bytes.data(), bytes.size(), profile) == Macro::Result::OK);
        assert(profile.name == "Firmware Fixture");
        assert(profile.setCount == 2);
        assert(profile.twoPlayerOutputs);
        assert(profile.sequences.size() == 1);
        assert(profile.selectors.size() == 1);
    }
    std::cout << "macro profile tests passed\n";
}
