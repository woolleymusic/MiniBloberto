#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    using C = juce::AudioParameterChoice;
    using B = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<C> (juce::ParameterID { "mode", 1 }, "Mode", juce::StringArray { "Still", "Dance" }, 1));
    l.add (std::make_unique<C> (juce::ParameterID { "moves", 1 }, "Moves", juce::StringArray { "Pick", "Random", "Input", "Cycle" }, 1));
    l.add (std::make_unique<C> (juce::ParameterID { "pick", 1 }, "Pick", mb::rowNames, 0));
    l.add (std::make_unique<C> (juce::ParameterID { "speed", 1 }, "Speed", juce::StringArray { "8n", "16n", "32n" }, 1));
    l.add (std::make_unique<C> (juce::ParameterID { "bars", 1 }, "Bars", juce::StringArray { "1", "2", "4", "8" }, 1));
    l.add (std::make_unique<C> (juce::ParameterID { "colour", 1 }, "Colour", mb::colourNames, 0));

    // v2: the window and the tricks (FAOSDance's extensions, kept on the pixel grid)
    l.add (std::make_unique<C> (juce::ParameterID { "size", 2 }, "Size", juce::StringArray { "2x", "3x", "4x" }, 1));
    l.add (std::make_unique<C> (juce::ParameterID { "backdrop", 2 }, "Backdrop",
                                juce::StringArray { "Dark", "Green", "Magenta", "Blue" }, 0));
    l.add (std::make_unique<C> (juce::ParameterID { "flip", 2 }, "Flip", juce::StringArray { "Off", "Each bar", "2 beats" }, 0));
    l.add (std::make_unique<B> (juce::ParameterID { "reverse", 2 }, "Reverse", false));
    l.add (std::make_unique<B> (juce::ParameterID { "shake", 2 }, "Shake", false));
    l.add (std::make_unique<B> (juce::ParameterID { "trails", 2 }, "Trails", false));
    l.add (std::make_unique<B> (juce::ParameterID { "reflect", 2 }, "Reflection", false));
    l.add (std::make_unique<B> (juce::ParameterID { "rainbow", 2 }, "Rainbow", false));
    return l;
}
} // namespace

MiniBlobertoProcessor::MiniBlobertoProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      params (*this, nullptr, "MiniBloberto", makeLayout())
{
}

void MiniBlobertoProcessor::prepareToPlay (double, int) {}

bool MiniBlobertoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void MiniBlobertoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // pass-through: leave the buffer alone, just clear any extra outputs
    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    float pk = 0.0f;
    for (int ch = 0; ch < getTotalNumInputChannels(); ++ch)
        pk = juce::jmax (pk, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));
    auto prev = peakSinceRead.load();
    while (pk > prev && ! peakSinceRead.compare_exchange_weak (prev, pk)) {}

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition())
                blockPpq = *q;
            if (auto b = pos->getBpm())
                bpm = *b;
            if (auto ts = pos->getTimeSignature())
            {
                num = ts->numerator;
                den = ts->denominator;
            }
            blockTimeMs = juce::Time::getMillisecondCounterHiRes();
        }
    }
}

void MiniBlobertoProcessor::setFakeTransport (bool isPlaying, double ppq, double tempo)
{
    playing = isPlaying;
    blockPpq = ppq;
    bpm = tempo;
    blockTimeMs = juce::Time::getMillisecondCounterHiRes();
}

mb::Transport MiniBlobertoProcessor::transportNow() const
{
    mb::Transport t;
    t.playing = playing.load();
    t.num = num.load();
    t.den = den.load();
    t.ppq = blockPpq.load();
    if (t.playing)
    {
        // blocks arrive every few ms; fill the gap so frames change exactly on the grid.
        // Capped so a host that stops calling processBlock doesn't run away.
        const auto ms = juce::jlimit (0.0, 250.0, juce::Time::getMillisecondCounterHiRes() - blockTimeMs.load());
        t.ppq += ms / 60000.0 * bpm.load();
    }
    return t;
}

mb::Settings MiniBlobertoProcessor::settings() const
{
    auto v = [this] (const char* id) { return (int) params.getRawParameterValue (id)->load(); };
    mb::Settings s;
    s.mode = v ("mode");
    s.moves = v ("moves");
    s.pick = v ("pick");
    s.speed = v ("speed");
    s.bars = v ("bars");
    s.flip = v ("flip");
    s.reverse = v ("reverse") != 0;
    return s;
}

void MiniBlobertoProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = params.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void MiniBlobertoProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (params.state.getType()))
            params.replaceState (juce::ValueTree::fromXml (*xml));
}

bool MiniBlobertoProcessor::dancerView() const
{
    return (bool) params.state.getProperty ("dancerView", false);
}

void MiniBlobertoProcessor::setDancerView (bool on)
{
    params.state.setProperty ("dancerView", on, nullptr);
}

juce::AudioProcessorEditor* MiniBlobertoProcessor::createEditor()
{
    return new MiniBlobertoEditor (*this);
}

#if ! MB_HEADLESS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MiniBlobertoProcessor();
}
#endif
