#pragma once
#include "SidechainFilter.h"
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace pocket {
struct Sample { std::array<float,2> dry{},key{},out{}; float gain=1; };
class Engine {
    struct Delayed { std::array<float,2> dry{},key{}; };
    struct Peak { std::uint64_t index=0; float value=0; };
    SidechainFilter filter;
    DynamicBandFilter processingFilter;
    std::vector<Delayed> delay;
    std::vector<Peak> peaks;
    size_t write=0,head=0,tail=0;
    std::uint64_t clock=0,age=0;
    int lookahead=240,refractory=0,quiet=0,rearmQuiet=0;
    double rate=48000;
    float amount=1,targetAmount=1,ms=0,targetMs=0,targetBypass=0,bypassMix=0,outputGain=1,targetOutputGain=1;
    float duration=2000,targetDuration=2000,envelope=0,fast=0,slow=0,eventPeak=0,smoothedControl=0;
    float fastC=0,slowC=0,releaseC=0,endC=0,slew=0,attackC=0,bypassC=0;
    bool active=false,onsetHighPreviously=false,triggerArmed=true,durationInit=false,strongPreviously=false;
    static float clean(float v) noexcept { return std::isfinite(v)?v:0.f; }
public:
    // 5 ms of lookahead: the gain starts moving before the transient arrives so the
    // duck fades in instead of cutting the waveform, which is what caused clicks.
    static int latencyForRate(double sr) noexcept { return std::max(1,int(std::ceil(std::max(1.,sr)*.005))); }
    static float durationGain(double elapsedMs,float lengthMs) noexcept {
        if(lengthMs>=1999.5f)return 1;
        const double length=std::clamp(double(lengthMs),1.,2000.);
        // Duration is the total length: half hold, then a half-cosine fade over the other half.
        const double t=std::clamp((elapsedMs-length*.5)/(length*.5),0.,1.);
        return float(.5+.5*std::cos(3.141592653589793*t));
    }
    Engine(){reset(48000,1);}
    int latency() const noexcept {return lookahead;}
    void reset(double sr,float influence=1) {
        rate=std::max(1.,sr);lookahead=latencyForRate(rate);
        delay.assign(size_t(lookahead+1),{});peaks.assign(size_t(lookahead+2),{});
        write=head=tail=0;clock=age=0;refractory=quiet=rearmQuiet=0;
        filter.reset(rate);processingFilter.reset(rate);
        amount=targetAmount=std::clamp(influence,0.f,1.5f);
        ms=targetMs=targetBypass=bypassMix=0;outputGain=targetOutputGain=1;envelope=fast=slow=eventPeak=0;smoothedControl=0;
        active=onsetHighPreviously=strongPreviously=false;triggerArmed=true;
        fastC=float(std::exp(-1/(rate*.0015)));slowC=float(std::exp(-1/(rate*.035)));
        releaseC=float(std::exp(-1/(rate*.04)));endC=float(std::exp(-1/(rate*.002)));
        attackC=float(std::exp(-1/(rate*.0016)));
        bypassC=float(std::exp(-1/(rate*.0025)));
        slew=float(std::exp(-1/(rate*.005)));duration=targetDuration=2000;durationInit=false;
    }
    void configure(float influence,float durationMs,float low=20,float high=20000,bool bypassed=false,float balance=0,float processLow=20,float processHigh=20000,float outputDb=0) noexcept {
        targetAmount=std::clamp(clean(influence),0.f,1.5f);targetDuration=std::clamp(clean(durationMs),5.f,2000.f);if(!durationInit){duration=targetDuration;durationInit=true;}
        filter.set(clean(low),clean(high));processingFilter.set(clean(processLow),clean(processHigh));
        targetMs=std::clamp(clean(balance),-1.f,1.f);targetBypass=bypassed?1.f:0.f;
        const float safeDb=std::clamp(clean(outputDb),-12.f,6.f);targetOutputGain=std::pow(10.f,safeDb/20.f);
    }
    Sample process(std::array<float,2> input,std::array<float,2> key) noexcept {
        for(auto& v:input)v=clean(v);
        for(auto& v:key)v=clean(v);
        key=filter.process(key);
        const float level=std::clamp(std::max(std::abs(key[0]),std::abs(key[1])),0.f,1.f);
        fast=level+fastC*(fast-level);slow=level+slowC*(slow-level);
        if(refractory>0)--refractory;
        const bool onsetHigh=level>1e-7f&&fast>std::max(1e-7f,slow*1.8f);
        const bool onset=onsetHigh&&!onsetHighPreviously&&refractory==0;
        onsetHighPreviously=onsetHigh;
        if(active){
            eventPeak=std::max(eventPeak,level);
            quiet=fast<std::max(1e-7f,eventPeak*.001f)?quiet+1:0;
            if(quiet>int(rate*.002)){active=false;rearmQuiet=0;}
        }
        // A short key tail used to start a second event as soon as `active`
        // dropped, producing two 1-20 ms notches. Require a real low-level gap
        // before arming the next event; a rising onset then starts it normally.
        if(!active&&!triggerArmed){
            const float resetLevel=std::max(1e-7f,eventPeak*.001f);
            rearmQuiet=level<resetLevel?rearmQuiet+1:0;
            if(rearmQuiet>int(rate*.004)){triggerArmed=true;eventPeak=0;rearmQuiet=0;onsetHighPreviously=false;}
        }
        // A clear new transient always starts a new event, even if the previous key
        // is still ringing and the trigger was never re-armed by a quiet gap. Without
        // this a finite Duration ducked once and then stayed silent for any key that
        // does not fall 60 dB between hits. The stricter ratio keeps slow ripple of a
        // sustained low tone from restarting the event.
        const bool armedStart=triggerArmed&&refractory==0&&(onset||(!active&&level>1e-5f));
        // Hysteresis (2.5x to enter, 1.2x to leave) so ripple inside one hit is not mistaken for a new hit.
        const bool strongHigh=level>1e-7f&&fast>std::max(1e-7f,slow*(strongPreviously?1.2f:2.5f));
        const bool hitStart=!triggerArmed&&strongHigh&&!strongPreviously&&refractory==0;
        strongPreviously=strongHigh;
        if(armedStart||hitStart){active=true;triggerArmed=false;age=0;quiet=0;rearmQuiet=0;eventPeak=level;refractory=std::max(1,int(rate*.012));}
        // Duration is read live (lightly slewed), so turning the knob takes effect on
        // the sound that is already playing instead of waiting for the next event.
        duration=targetDuration+slew*(duration-targetDuration);if(std::abs(duration-targetDuration)<.01f)duration=targetDuration;
        envelope=std::max(level,envelope*(active?releaseC:endC));if(envelope<1e-7f)envelope=0;
        const float gate=duration>=1999.5f?1.f:(active?durationGain(double(age)*1000/rate,duration):0.f);
        const float control=envelope*gate;if(active)++age;
        while(head!=tail&&clock>std::uint64_t(lookahead)&&peaks[head].index<clock-std::uint64_t(lookahead))head=(head+1)%peaks.size();
        while(head!=tail){size_t last=(tail+peaks.size()-1)%peaks.size();if(peaks[last].value>control)break;tail=last;}
        peaks[tail]={clock,control};tail=(tail+1)%peaks.size();
        const float predicted=peaks[head].value;
        // Soft attack inside the lookahead window: the duck ramps in ahead of the hit.
        smoothedControl=predicted>smoothedControl?predicted+attackC*(smoothedControl-predicted):predicted;
        delay[write]={input,{key[0]*gate,key[1]*gate}};const size_t read=(write+1)%delay.size();
        Sample result;result.dry=delay[read].dry;result.key=delay[read].key;
        amount=targetAmount+slew*(amount-targetAmount);ms=targetMs+slew*(ms-targetMs);outputGain=targetOutputGain+slew*(outputGain-targetOutputGain);
        if(std::abs(amount-targetAmount)<1e-4f)amount=targetAmount;
        if(std::abs(ms-targetMs)<1e-4f)ms=targetMs;
        if(std::abs(outputGain-targetOutputGain)<1e-6f)outputGain=targetOutputGain;
        const float reduction=std::clamp(effectiveDepth(amount)*smoothedControl,0.f,1.f);
        const float gm=1-reduction*(1-std::max(0.f,ms)),gs=1-reduction*(1+std::min(0.f,ms));
        const float mid=(result.dry[0]+result.dry[1])*.5f,side=(result.dry[0]-result.dry[1])*.5f;
        // v0.9: the dry path is never filtered. The band filter only extracts the
        // selection, which is then subtracted by the amount of ducking:
        //     out = dry - (1 - g) * selection
        // With the handles parked the selection weights are zero and mix is zero, so
        // this collapses back to the plain wideband duck, bit for bit.
        std::array<float,2> band{};
        float mix=0.f;
        if(processingFilter.engaged()){
            band=processingFilter.process({mid,side});
            mix=processingFilter.blend();
        }
        const float selectMid=mid+(band[0]-mid)*mix,selectSide=side+(band[1]-side)*mix;
        const float processedMid=mid-(1-gm)*selectMid,processedSide=side-(1-gs)*selectSide;
        result.out={processedMid+processedSide,processedMid-processedSide};
        // The meter follows the gain the engine asked for, so the history stays the
        // same whether the reduction runs wideband or inside the selected band.
        result.gain=std::clamp((gm+gs)*.5f,0.f,1.f);
        if(reduction==0){result.out=result.dry;result.gain=1;}
        result.out[0]*=outputGain;result.out[1]*=outputGain;

        // Click-free but perceptually immediate bypass. Both sides are already
        // aligned to the same 5 ms lookahead, so this crossfade adds no latency.
        const float bypassTarget=targetBypass>.5f?1.f:0.f;
        bypassMix=bypassTarget+bypassC*(bypassMix-bypassTarget);
        if(std::abs(bypassMix-bypassTarget)<1e-5f)bypassMix=bypassTarget;
        if(bypassMix>0.f){
            result.out[0]+=bypassMix*(result.dry[0]-result.out[0]);
            result.out[1]+=bypassMix*(result.dry[1]-result.out[1]);
            result.gain+=bypassMix*(1.f-result.gain);
        }
        write=read;++clock;return result;
    }
};
}
