#include "PluginProcessor.h"
#include "PluginEditor.h"

DuckPocketAudioProcessor::DuckPocketAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input",juce::AudioChannelSet::stereo(),true)
        .withInput("Sidechain",juce::AudioChannelSet::stereo(),true)
        .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      parameters(*this,nullptr,"PARAMETERS",layout())
{
    amount=parameters.getRawParameterValue("amount");
    duration=parameters.getRawParameterValue("duration");
    low=parameters.getRawParameterValue("scLow");
    high=parameters.getRawParameterValue("scHigh");
    bypass=parameters.getRawParameterValue("bypass");
    balance=parameters.getRawParameterValue("msBalance");
    processLow=parameters.getRawParameterValue("processLow");
    processHigh=parameters.getRawParameterValue("processHigh");
    outputGain=parameters.getRawParameterValue("outputGain");
    setLatencySamples(engine.latency());
}

static juce::NormalisableRange<float> logHzRange()
{
    return {20.f,20000.f,
        [](float start,float end,float proportion){return start*std::pow(end/start,proportion);},
        [](float start,float end,float value){return std::log(juce::jlimit(start,end,value)/start)/std::log(end/start);},
        [](float start,float end,float value){return juce::jlimit(start,end,value);}};
}

juce::AudioProcessorValueTreeState::ParameterLayout DuckPocketAudioProcessor::layout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>("amount","Influence",juce::NormalisableRange<float>(0,150,.1f),100.f));

    // Keep old IDs loadable, but mark them as non-automatable metadata so new
    // sessions do not present dead controls as normal automation destinations.
    const auto legacyFloat=[&p](const char* id,const char* name,float lo,float hi,float def){
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi),def,
            juce::AudioParameterFloatAttributes().withAutomatable(false).withMeta(true)));
    };
    legacyFloat("tolerance","Legacy Tolerance",0.f,6.f,1.f);
    legacyFloat("low","Legacy Low",20.f,150.f,25.f);
    legacyFloat("high","Legacy High",80.f,500.f,220.f);
    legacyFloat("maxReduction","Legacy Reduction",0.f,48.f,24.f);
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"release",1},"Legacy Smoothing (fixed 40 ms)",juce::NormalisableRange<float>(0,500,.1f,.4f),40.f,
        juce::AudioParameterFloatAttributes().withAutomatable(false).withMeta(true)));
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"phaseAware",1},"Legacy Phase",true,
        juce::AudioParameterBoolAttributes().withAutomatable(false).withMeta(true)));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"mode",1},"Legacy Mode (amplitude only)",juce::StringArray{"Legacy","Amplitude"},1,
        juce::AudioParameterChoiceAttributes().withAutomatable(false).withMeta(true)));

    p.push_back(std::make_unique<juce::AudioParameterFloat>("scLow","Sidechain Low",logHzRange(),20.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("scHigh","Sidechain High",logHzRange(),20000.f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("bypass","Bypass",false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("msBalance","M/S balance",juce::NormalisableRange<float>(-1,1,.001f),0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("duration","Duration",juce::NormalisableRange<float>(1,2000,1,.35f),2000.f));
    legacyFloat("sustain","Legacy Sustain (fixed zero)",0.f,100.f,0.f);
    p.push_back(std::make_unique<juce::AudioParameterFloat>("processLow","Processing Low",logHzRange(),20.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("processHigh","Processing High",logHzRange(),20000.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain","Output Gain",juce::NormalisableRange<float>(-12.f,6.f,.01f),0.f));
    return {p.begin(),p.end()};
}

void DuckPocketAudioProcessor::prepareToPlay(double sr,int)
{
    engine.reset(sr,amount->load()*.01f);
    setLatencySamples(engine.latency());
    decimation=juce::jmax(1,int(sr/2400.0));
    captured=0;
    capture={};
}

void DuckPocketAudioProcessor::reset()
{
    engine.reset(juce::jmax(1.,getSampleRate()),amount->load()*.01f);
    captured=0;
    capture={};
}

bool DuckPocketAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    if(l.inputBuses.size()!=2||l.outputBuses.size()!=1)return false;
    auto in=l.getMainInputChannelSet(),out=l.getMainOutputChannelSet(),key=l.getChannelSet(true,1);
    return in==out&&(out==juce::AudioChannelSet::mono()||out==juce::AudioChannelSet::stereo())
        &&(key.isDisabled()||key==juce::AudioChannelSet::mono()||key==juce::AudioChannelSet::stereo());
}

void DuckPocketAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m)
{
    processAudio(b,m,false);
}

void DuckPocketAudioProcessor::processAudio(juce::AudioBuffer<float>& b,juce::MidiBuffer&,bool hostBypass)
{
    juce::ScopedNoDenormals noDenormals;
    auto main=getBusBuffer(b,true,0);
    auto key=getBusBuffer(b,true,1);
    const int mainChannels=main.getNumChannels();
    const int keyChannels=key.getNumChannels();
    if(mainChannels==0)return;

    displayBypass.store(hostBypass,std::memory_order_relaxed);
    engine.configure(amount->load()*.01f,duration->load(),low->load(),high->load(),
                     hostBypass||bypass->load()>.5f,balance->load(),
                     processLow->load(),processHigh->load(),outputGain->load());

    auto* mainL=main.getWritePointer(0);
    auto* mainR=mainChannels>1?main.getWritePointer(1):nullptr;
    const auto* keyL=keyChannels>0?key.getReadPointer(0):nullptr;
    const auto* keyR=keyChannels>1?key.getReadPointer(1):keyL;
    const bool show=editorOpen.load(std::memory_order_relaxed);
    const double step=1./juce::jmax(1.,getSampleRate());

    for(int n=0;n<b.getNumSamples();++n)
    {
        const float l=mainL[n],r=mainR?mainR[n]:l;
        const float kl=keyL?keyL[n]:0.f,kr=keyR?keyR[n]:kl;
        auto v=engine.process({l,r},{kl,kr});
        mainL[n]=v.out[0];
        if(mainR)mainR[n]=v.out[1];
        traceTime+=step;

        if(!show){captured=0;continue;}
        const float inLo=juce::jmin(v.dry[0],v.dry[1]),inHi=juce::jmax(v.dry[0],v.dry[1]);
        const float keyLo=juce::jmin(v.key[0],v.key[1]),keyHi=juce::jmax(v.key[0],v.key[1]);
        const float outLo=juce::jmin(v.out[0],v.out[1]),outHi=juce::jmax(v.out[0],v.out[1]);
        if(!captured)capture={inLo,inHi,keyLo,keyHi,outLo,outHi,v.gain,traceTime};
        else {
            capture.inLo=juce::jmin(capture.inLo,inLo); capture.inHi=juce::jmax(capture.inHi,inHi);
            capture.keyLo=juce::jmin(capture.keyLo,keyLo); capture.keyHi=juce::jmax(capture.keyHi,keyHi);
            capture.outLo=juce::jmin(capture.outLo,outLo); capture.outHi=juce::jmax(capture.outHi,outHi);
            capture.gain=juce::jmin(capture.gain,v.gain); capture.time=traceTime;
        }
        if(++captured>=decimation){
            int a,s,c,d;fifo.prepareToWrite(1,a,s,c,d);
            if(s){traces[size_t(a)]=capture;fifo.finishedWrite(1);}
            captured=0;
        }
    }
}

bool DuckPocketAudioProcessor::popTrace(PocketTrace& v)
{
    int a,s,b,c;fifo.prepareToRead(1,a,s,b,c);if(!s)return false;
    v=traces[size_t(a)];fifo.finishedRead(1);return true;
}

void DuckPocketAudioProcessor::getStateInformation(juce::MemoryBlock& d)
{
    auto state=parameters.copyState();
    state.setProperty("uiWidth",editorWidth.load(),nullptr);
    state.setProperty("uiExpanded",editorExpanded.load(),nullptr);
    if(auto x=state.createXml())copyXmlToBinary(*x,d);
}

void DuckPocketAudioProcessor::setStateInformation(const void* d,int n)
{
    if(auto x=getXmlFromBinary(d,n))if(x->hasTagName(parameters.state.getType())){
        auto state=juce::ValueTree::fromXml(*x);
        editorWidth.store(int(state.getProperty("uiWidth",0)));
        editorExpanded.store(bool(state.getProperty("uiExpanded",false)));
        parameters.replaceState(state);
    }
}

juce::AudioProcessorEditor* DuckPocketAudioProcessor::createEditor(){return new DuckPocketAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DuckPocketAudioProcessor();}
