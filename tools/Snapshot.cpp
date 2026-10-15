// Offline harness for Mini Bloberto. No DAW needed.
//   MiniBlobertoSnapshot --selftest
//       pass-through null test: audio out must equal audio in exactly
//   MiniBlobertoSnapshot --png out.png [--ppq 3.25] [--stopped] [--set id=index ...]
//       the plugin window at that beat position (ids: mode moves pick speed bars colour flip
//       size backdrop, and 0/1 for reverse shake trails reflect rainbow). --dancer: just him
//   MiniBlobertoSnapshot --wav song.wav [--seconds N] [--set ...]
//       no host clock (like OBS): feeds the song through the plugin and prints, every 5 s,
//       the tempo it hears, the beats it found and the moves he danced
//   MiniBlobertoSnapshot --strip out.png [--set ...]
//       one frame per 16th across 2 beats, side by side
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

static void apply (MiniBlobertoProcessor& p, const juce::StringArray& args)
{
    for (int i = 0; i < args.size(); ++i)
        if (args[i] == "--set" && i + 1 < args.size())
        {
            auto id = args[i + 1].upToFirstOccurrenceOf ("=", false, false);
            auto v = args[i + 1].fromFirstOccurrenceOf ("=", false, false).getIntValue();
            if (auto* prm = p.params.getParameter (id))
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) v));
            else
                std::printf ("unknown parameter %s\n", id.toRawUTF8());
        }
}

static bool save (const juce::Image& img, const juce::String& path)
{
    juce::File f (juce::File::getCurrentWorkingDirectory().getChildFile (path));
    f.deleteFile();
    juce::FileOutputStream out (f);
    juce::PNGImageFormat png;
    return out.openedOk() && png.writeImageToStream (img, out);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::StringArray args;
    for (int i = 1; i < argc; ++i)
        args.add (argv[i]);

    MiniBlobertoProcessor proc;
    proc.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    apply (proc, args);

    if (args.contains ("--selftest"))
    {
        juce::Random r (42);
        juce::AudioBuffer<float> buf (2, 512), ref (2, 512);
        juce::MidiBuffer midi;
        double worst = 0.0;
        for (int block = 0; block < 200; ++block)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    buf.setSample (ch, i, r.nextFloat() * 2.0f - 1.0f);
            ref.makeCopyOf (buf);
            proc.processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    worst = juce::jmax (worst, (double) std::abs (buf.getSample (ch, i) - ref.getSample (ch, i)));
        }
        std::printf ("pass-through null test: max difference %g (%s)\n", worst, worst == 0.0 ? "PASS" : "FAIL");
        return worst == 0.0 ? 0 : 1;
    }

    const auto wavIdx = args.indexOf ("--wav");
    if (wavIdx >= 0)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (juce::File (args[wavIdx + 1])));
        if (r == nullptr)
        {
            std::printf ("cannot read %s\n", args[wavIdx + 1].toRawUTF8());
            return 1;
        }
        const double sr = r->sampleRate;
        proc.prepareToPlay (sr, 512);
        std::unique_ptr<MiniBlobertoEditor> ed (dynamic_cast<MiniBlobertoEditor*> (proc.createEditor()));
        const auto secIdx = args.indexOf ("--seconds");
        const auto len = juce::jmin ((juce::int64) ((secIdx >= 0 ? args[secIdx + 1].getDoubleValue() : 60.0) * sr),
                                     r->lengthInSamples);
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        double lastBeat = 0.0, next = 5.0, uiClock = 0.0;
        juce::String beatTimes;
        juce::StringArray moves;
        int frameChanges = 0, lastFrame = -1;
        for (juce::int64 pos = 0; pos < len; pos += 512)
        {
            const int n = (int) juce::jmin ((juce::int64) 512, len - pos);
            buf.clear();
            r->read (&buf, 0, n, pos, true, true);
            proc.processBlock (buf, midi);
            const double t = (double) (pos + n) / sr;
            const double b = proc.tracker().beatsAtLastHop();
            if (proc.tracker().active() && std::floor (b) > std::floor (lastBeat))
                beatTimes << juce::String (t, 2) << " ";
            lastBeat = b;
            while (uiClock < t)   // the editor's 30 fps timer, in song time
            {
                uiClock += 1.0 / 30.0;
                ed->advance (1.0 / 30.0);
                auto c = ed->currentCell();
                if (c.frame != lastFrame) { ++frameChanges; lastFrame = c.frame; }
                moves.addIfNotAlreadyThere (mb::rowNames[c.row]);
            }
            if (t >= next)
            {
                std::printf ("t=%5.1fs bpm=%6.1f active=%d frames=%d moves=%s\n  beats: %s\n", t,
                             proc.tracker().bpm(), (int) proc.tracker().active(), frameChanges,
                             moves.joinIntoString (",").toRawUTF8(), beatTimes.toRawUTF8());
                beatTimes.clear();
                moves.clear();
                frameChanges = 0;
                next += 5.0;
            }
        }
        return 0;
    }

    const bool playing = ! args.contains ("--stopped");
    const auto ppqIdx = args.indexOf ("--ppq");
    const double ppq = ppqIdx >= 0 ? args[ppqIdx + 1].getDoubleValue() : 0.0;

    std::unique_ptr<MiniBlobertoEditor> ed (dynamic_cast<MiniBlobertoEditor*> (proc.createEditor()));
    ed->showDancer (args.contains ("--dancer"));

    const auto stripIdx = args.indexOf ("--strip");
    if (stripIdx >= 0)
    {
        const int n = 8;
        proc.setFakeTransport (true, ppq, 120.0);
        ed->advance (0.0);
        const int cw = 330, ch = ed->composeNative().getHeight() * 3;
        juce::Image strip (juce::Image::ARGB, cw * n, ch, true);
        juce::Graphics g (strip);
        g.fillAll (juce::Colour (0xff1f1f26));
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
        for (int k = 0; k < n; ++k)
        {
            proc.setFakeTransport (true, ppq + k * 0.25, 120.0);
            ed->advance (0.0);
            auto him = ed->composeNative();
            g.drawImage (him, k * cw, 0, cw, ch, 0, 0, him.getWidth(), him.getHeight());
            auto c = ed->currentCell();
            std::printf ("ppq %.2f -> %s frame %d\n", ppq + k * 0.25, mb::rowNames[c.row].toRawUTF8(), c.frame);
        }
        return save (strip, args[stripIdx + 1]) ? 0 : 1;
    }

    proc.setFakeTransport (playing, ppq, 120.0);
    ed->advance (0.0);
    auto c = ed->currentCell();
    std::printf ("%s at ppq %.2f -> %s frame %d\n", playing ? "playing" : "stopped", ppq,
                 mb::rowNames[c.row].toRawUTF8(), c.frame);

    const auto pngIdx = args.indexOf ("--png");
    if (pngIdx >= 0)
        return save (ed->createComponentSnapshot (ed->getLocalBounds()), args[pngIdx + 1]) ? 0 : 1;
    return 0;
}
