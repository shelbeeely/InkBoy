#!/usr/bin/env python3
"""The FreeInk SDK Ssd1681Driver over the fake panel: profile pins, call order, fast masks and session recycling."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from host_build import ROOT, FAKES

SDK = ROOT / 'freeink-sdk/libs'
CODE = r'''
#include <assert.h>
#include <string.h>
#include "driver/Ssd1681Driver.h"
#include <BoardConfig.h>
extern "C" {
#include "host_fakes.h"
#include "host_panel.h"
}
using namespace freeink;

static uint8_t frame[5000];
// Never touched by an external-bus driver; raw storage avoids linking EpdBus.cpp.
alignas(EpdBus) static unsigned char bus_storage[sizeof(EpdBus)];
static EpdBus *const bus = reinterpret_cast<EpdBus *>(bus_storage);

int main() {
    host_fakes_reset(); host_panel_reset();
    PanelDriver &d = ssd1681Driver();
    assert(d.usesExternalBus());
    const PanelGeometry g = d.geometry();
    assert(g.width == 200 && g.height == 200 && g.widthBytes == 25 && g.bufferSize == 5000);
    d.begin(*bus);

    // Full: pins come from the profile, then one stock full refresh.
    memset(frame, 0xff, sizeof(frame));
    d.display(*bus, frame, nullptr, RefreshMode::Full, false);
    assert(d.displayCommitted());
    assert(host_panel.pin_configs == 1 && host_panel.pins.sclk == 12 && host_panel.pins.mosi == 13 &&
           host_panel.pins.cs == 11 && host_panel.pins.dc == 10 && host_panel.pins.rst == 9 &&
           host_panel.pins.busy == 8 && host_panel.pins.power == 6);
    assert(host_panel.fulls == 1 && host_panel.scans == 0 && host_panel.powered);

    // Fast: one black square, 8 pulses + an all-HOLD ending, analog off afterwards.
    for (unsigned y = 50; y < 60; ++y) frame[y * 25 + 5] = 0x00;
    d.display(*bus, frame, nullptr, RefreshMode::Fast, false);
    assert(d.displayCommitted());
    assert(host_panel.fulls == 1 && host_panel.scans == Ssd1681Driver::FAST_PULSES + 1);
    for (unsigned i = 0; i < Ssd1681Driver::FAST_PULSES; ++i) assert(!host_panel.scan_hold[i]);
    assert(host_panel.scan_hold[Ssd1681Driver::FAST_PULSES]);
    assert(!host_panel.analog && !memcmp(host_panel.physical, frame, sizeof(frame)));

    // Second fast update resumes analog power and drives white back.
    memset(frame, 0xff, sizeof(frame));
    d.display(*bus, frame, nullptr, RefreshMode::Fast, false);
    assert(host_panel.resumes == 1 && !memcmp(host_panel.physical, frame, sizeof(frame)));

    // Unchanged frame: no scan at all.
    const unsigned scans = host_panel.scans;
    d.display(*bus, frame, nullptr, RefreshMode::Fast, false);
    assert(host_panel.scans == scans && d.displayCommitted());

    // turnOff powers the panel down; the next Fast becomes a Full.
    d.display(*bus, frame, nullptr, RefreshMode::Half, true);
    assert(host_panel.fulls == 2 && host_panel.destroys == 1 && !host_panel.powered);
    frame[0] = 0;
    d.display(*bus, frame, nullptr, RefreshMode::Fast, false);
    assert(host_panel.fulls == 3 && host_panel.scans == scans);

    // Many fast updates never hit the per-session cap: the session is recycled with a Full.
    const unsigned destroys = host_panel.destroys;
    for (unsigned n = 0; n < 400; ++n) {
        frame[1] = (uint8_t)(n & 1 ? 0x00 : 0xff);
        d.display(*bus, frame, nullptr, RefreshMode::Fast, false);
        assert(d.displayCommitted());
        assert(host_panel.updates < SSD1681_MAX_UPDATES);
    }
    assert(host_panel.destroys > destroys && !memcmp(host_panel.physical, frame, sizeof(frame)));

    // A failed scan reports an uncommitted frame and closes the session.
    host_panel.fail_scan_at = host_panel.scans + 1;
    frame[2] ^= 0xff;
    d.display(*bus, frame, nullptr, RefreshMode::Fast, false);
    assert(!d.displayCommitted() && !host_panel.powered);
    host_panel.fail_scan_at = 0;
    d.display(*bus, frame, nullptr, RefreshMode::Fast, false);  // recovers with a Full
    assert(d.displayCommitted() && !memcmp(host_panel.physical, frame, sizeof(frame)));
    d.deepSleep(*bus);
    assert(!host_panel.powered);
    return 0;
}
'''

C_FAKES = ['host_panel.c', 'host_gpio.c', 'host_heap.c', 'host_log.c', 'host_nvs.c', 'host_reset.c', 'host_rtos.c',
           'host_spi.c', 'host_board.c']
with tempfile.TemporaryDirectory(prefix='host-test-') as directory:
    work = Path(directory)
    includes = [f'-I{p}' for p in (FAKES / 'arduino', FAKES, ROOT / 'main', ROOT / 'components/ssd1681_fast/include',
                                   SDK / 'hardware/BoardConfig/include', SDK / 'display/FreeInkDisplay/include',
                                   SDK / 'display/FreeInkDisplay/src')]
    flags = ['-fsanitize=address,undefined', '-O1', '-DFREEINK_DEVICE_WS154=1']
    objects = []
    for fake in C_FAKES:
        objects.append(str(work / f'{fake}.o'))
        if subprocess.run(['cc', '-std=c11', '-w', *flags, *includes[1:], '-c', str(FAKES / fake), '-o', objects[-1]]).returncode:
            sys.exit('host test failed to compile')
    (work / 'test.cpp').write_text(CODE)
    sources = [work / 'test.cpp', SDK / 'display/FreeInkDisplay/src/driver/Ssd1681Driver.cpp']
    if subprocess.run(['c++', '-std=gnu++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', *flags, *includes,
                       *map(str, sources), *objects, '-o', str(work / 'test'), '-lm']).returncode:
        sys.exit('host test failed to compile')
    result = subprocess.run([str(work / 'test')], timeout=120, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
    if result.returncode: sys.exit(f'host test failed ({result.returncode})')
print('PASS: SDK Ssd1681Driver: profile pins, stock full, 8-pulse fast masks with HOLD ending, resume, no-op, turn-off, session recycling, failure recovery')
