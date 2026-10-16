// Drives reSIDfp directly: noise sanity check and raw CPU cost per chip.
#include "SID.h"
#include <chrono>
#include <cstdio>
#include <vector>
#include <cmath>

static double bench (reSIDfp::ChipModel model, reSIDfp::SamplingMethod m, int voices, bool filter)
{
    reSIDfp::SID sid;
    sid.setChipModel (model);
    sid.setSamplingParameters (985248.0, m, 48000.0);
    sid.write (0x18, 0x1f);
    sid.write (0x17, filter ? 0xf7 : 0x00);
    sid.write (0x16, 0x40);
    for (int v = 0; v < voices; ++v)
    {
        sid.write (v * 7 + 0, 0x00); sid.write (v * 7 + 1, (unsigned char) (0x10 + v * 3));
        sid.write (v * 7 + 5, 0x00); sid.write (v * 7 + 6, 0xf0);
        sid.write (v * 7 + 4, 0x21);
    }
    std::vector<short> buf (4096);
    const auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i)           // 1 second in 1 ms chunks
        sid.clock (985, buf.data());
    const auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double> (t1 - t0).count() * 100.0;   // % of one core
}

int main()
{
    for (int model = 0; model < 2; ++model)
    for (int freq : { 0x1000, 0xffff })
    {
        reSIDfp::SID sid;
        sid.setChipModel (model == 0 ? reSIDfp::MOS6581 : reSIDfp::MOS8580);
        sid.setSamplingParameters (985248.0, reSIDfp::RESAMPLE, 48000.0);
        sid.write (0x18, 0x0f);
        sid.write (0x00, freq & 0xff); sid.write (0x01, freq >> 8);
        sid.write (0x05, 0x00); sid.write (0x06, 0xf0);
        std::vector<short> buf (100000);
        sid.write (0x04, 0x81);
        int n = sid.clock (985248 / 4, buf.data());
        double mean = 0; for (int i = 4800; i < n; ++i) mean += buf[i]; mean /= (n - 4800);
        int zc = 0;
        for (int i = 4801; i < n; ++i) if ((buf[i-1] <= mean) != (buf[i] <= mean)) ++zc;
        std::printf ("noise %s freq %04x: crossings/s %.0f\n", model ? "8580" : "6581", freq, zc / ((n - 4801) / 48000.0));
    }

    for (auto model : { reSIDfp::MOS6581, reSIDfp::MOS8580 })
        for (auto m : { reSIDfp::RESAMPLE, reSIDfp::DECIMATE })
            for (int voices : { 1, 3 })
                for (bool f : { false, true })
                    std::printf ("cpu %s %s voices %d filter %d: %.2f%%\n", model == reSIDfp::MOS6581 ? "6581" : "8580",
                                 m == reSIDfp::RESAMPLE ? "sinc" : "deci", voices, f, bench (model, m, voices, f));
}
