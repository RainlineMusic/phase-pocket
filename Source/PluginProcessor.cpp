#include "PluginProcessor.h"
#include "PluginEditor.h"
DuckPocketAudioProcessor::DuckPocketAudioProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withInput("Sidechain",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),parameters(*this,nullptr,"PARAMETERS",layout()) {
    amount=parameters.getRawParameterValue("amount");duration=parameters.getRawParameterValue("duration");low=parameters.getRawParameterValue("scLow");high=parameters.getRawParameterValue("scHigh");bypass=parameters.getRawParameterValue("bypass");balance=parameters.getRawParameterValue("msBalance");processLow=parameters.getRawParameterValue("processLow");processHigh=parameters.getRawParameterValue("processHigh");outputGain=parameters.getRawParameterValue("outputGain");setLatencySamples(engine.latency());
}
static juce::NormalisableRange<float> logHzRange(){
    return {20.f,20000.f,
        [](float start,float end,float proportion){return start*std::pow(end/start,proportion);},
        [](float start,float end,float value){return std::log(juce::jlimit(start,end,value)/start)/std::log(end/start);},
        [](float start,float end,float value){return juce::jlimit(start,end,value);}};
}
juce::AudioProcessorValueTreeState::ParameterLayout DuckPocketAudioProcessor::layout(){
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>("amount","Influence",juce::NormalisableRange<float>(0,150,.1f),100.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("tolerance","Legacy Tolerance",0.f,6.f,1.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("low","Legacy Low",20.f,150.f,25.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("high","Legacy High",80.f,500.f,220.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("maxReduction","Legacy Reduction",0.f,48.f,24.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("release","Legacy Smoothing (fixed 40 ms)",juce::NormalisableRange<float>(0,500,.1f,.4f),40.f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("phaseAware","Legacy Phase",true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("mode","Legacy Mode (amplitude only)",juce::StringArray{"Legacy","Amplitude"},1));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("scLow","Sidechain Low",logHzRange(),20.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("scHigh","Sidechain High",logHzRange(),20000.f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("bypass","Bypass",false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("msBalance","M/S balance",juce::NormalisableRange<float>(-1,1,.001f),0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("duration","Duration",juce::NormalisableRange<float>(1,2000,1,.35f),2000.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("sustain","Legacy Sustain (fixed zero)",0.f,100.f,0.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("processLow","Processing Low",logHzRange(),20.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("processHigh","Processing High",logHzRange(),20000.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain","Output Gain",juce::NormalisableRange<float>(-100.f,6.f,.01f,4.94f),0.f));
    return {p.begin(),p.end()};
}
void DuckPocketAudioProcessor::prepareToPlay(double sr,int){engine.reset(sr,amount->load()*.01f);setLatencySamples(engine.latency());decimation=juce::jmax(1,int(sr/400));captured=0;capture={};}
void DuckPocketAudioProcessor::reset(){engine.reset(juce::jmax(1.,getSampleRate()),amount->load()*.01f);captured=0;capture={};}
bool DuckPocketAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {if(l.inputBuses.size()!=2||l.outputBuses.size()!=1)return false;auto in=l.getMainInputChannelSet(),out=l.getMainOutputChannelSet(),key=l.getChannelSet(true,1);return in==out&&(out==juce::AudioChannelSet::mono()||out==juce::AudioChannelSet::stereo())&&(key.isDisabled()||key==juce::AudioChannelSet::mono()||key==juce::AudioChannelSet::stereo());}
void DuckPocketAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m){processAudio(b,m,false);}
void DuckPocketAudioProcessor::processAudio(juce::AudioBuffer<float>& b,juce::MidiBuffer&,bool hostBypass){
    juce::ScopedNoDenormals noDenormals;auto main=getBusBuffer(b,true,0),key=getBusBuffer(b,true,1);if(!main.getNumChannels())return;
    displayBypass.store(hostBypass,std::memory_order_relaxed);engine.configure(amount->load()*.01f,duration->load(),low->load(),high->load(),hostBypass||bypass->load()>.5f,balance->load(),processLow->load(),processHigh->load(),outputGain->load());
    bool show=editorOpen.load(std::memory_order_relaxed);float peak=0,minimum=1;double step=1./juce::jmax(1.,getSampleRate());
    for(int n=0;n<b.getNumSamples();++n){std::array<float,2> in{},sc{};for(int c=0;c<2;++c){in[size_t(c)]=main.getSample(juce::jmin(c,main.getNumChannels()-1),n);if(key.getNumChannels())sc[size_t(c)]=key.getSample(juce::jmin(c,key.getNumChannels()-1),n);}auto v=engine.process(in,sc);for(int c=0;c<main.getNumChannels();++c)main.setSample(c,n,v.out[size_t(c)]);minimum=juce::jmin(minimum,v.gain);peak=juce::jmax(peak,std::abs(sc[0]),std::abs(sc[1]));traceTime+=step;if(!show){captured=0;continue;}if(!captured)capture={v.dry[0],v.dry[0],v.key[0],v.key[0],v.out[0],v.out[0],v.gain,traceTime};else{capture.inLo=juce::jmin(capture.inLo,v.dry[0]);capture.inHi=juce::jmax(capture.inHi,v.dry[0]);capture.keyLo=juce::jmin(capture.keyLo,v.key[0]);capture.keyHi=juce::jmax(capture.keyHi,v.key[0]);capture.outLo=juce::jmin(capture.outLo,v.out[0]);capture.outHi=juce::jmax(capture.outHi,v.out[0]);capture.gain=juce::jmin(capture.gain,v.gain);capture.time=traceTime;}if(++captured>=decimation){int a,s,c,d;fifo.prepareToWrite(1,a,s,c,d);if(s){traces[size_t(a)]=capture;fifo.finishedWrite(1);}captured=0;}}
    keyPeak.store(peak);gainMeter.store(minimum);
}
bool DuckPocketAudioProcessor::popTrace(PocketTrace& v){int a,s,b,c;fifo.prepareToRead(1,a,s,b,c);if(!s)return false;v=traces[size_t(a)];fifo.finishedRead(1);return true;}
void DuckPocketAudioProcessor::getStateInformation(juce::MemoryBlock& d){auto state=parameters.copyState();state.setProperty("uiWidth",editorWidth.load(),nullptr);if(auto x=state.createXml())copyXmlToBinary(*x,d);}
void DuckPocketAudioProcessor::setStateInformation(const void* d,int n){if(auto x=getXmlFromBinary(d,n))if(x->hasTagName(parameters.state.getType())){auto state=juce::ValueTree::fromXml(*x);editorWidth.store(int(state.getProperty("uiWidth",0)));parameters.replaceState(state);}}
juce::AudioProcessorEditor* DuckPocketAudioProcessor::createEditor(){return new DuckPocketAudioProcessorEditor(*this);}juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DuckPocketAudioProcessor();}
