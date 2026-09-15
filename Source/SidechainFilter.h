#pragma once
#include <array>
#include <cmath>
#include <algorithm>
namespace pocket {
struct StereoSVF {
    double s1=0,s2=0;
    double tick(double x,double g,bool high) noexcept {
        const double a=1/(1+g*(g+1.4142135623730951));
        const double v1=a*(s1+g*(x-s2)),v2=s2+g*v1;
        s1=2*v1-s1;s2=2*v2-s2;
        return high?x-1.4142135623730951*v1-v2:v2;
    }
    void clear() noexcept {s1=s2=0;}
};
struct LinkwitzRiley {
    StereoSVF a{},b{};
    void clear() noexcept {a={};b={};}
    double tick(double x,double g,bool high) noexcept {return b.tick(a.tick(x,g,high),g,high);}
};
class SidechainFilter {
    std::array<StereoSVF,2> hp{},lp{};
    double sr=48000,lowLog=std::log(20.),highLog=std::log(20000.),targetLow=20,targetHigh=20000;
    double hpMix=0,lpMix=0,gh=0,gl=0,step=0;int count=0;
public:
    void reset(double rate) noexcept {sr=std::max(100.,rate);hp={};lp={};targetLow=20;targetHigh=20000;lowLog=std::log(20.);highLog=std::log(20000.);hpMix=lpMix=0;count=0;step=std::exp(-16/(sr*.01));update();}
    void set(float low,float high) noexcept {targetLow=std::clamp(double(std::min(low,high)),20.,20000.);targetHigh=std::clamp(double(std::max(low,high)),20.,20000.);}
    void update() noexcept {lowLog=std::log(targetLow)+step*(lowLog-std::log(targetLow));highLog=std::log(targetHigh)+step*(highLog-std::log(targetHigh));const double useHp=targetLow>20.01?1:0,useLp=targetHigh<19999.?1:0;hpMix=useHp+step*(hpMix-useHp);lpMix=useLp+step*(lpMix-useLp);if(std::abs(hpMix-useHp)<1e-8)hpMix=useHp;if(std::abs(lpMix-useLp)<1e-8)lpMix=useLp;gh=std::tan(3.141592653589793*std::min(std::exp(lowLog),sr*.45)/sr);gl=std::tan(3.141592653589793*std::min(std::exp(highLog),sr*.45)/sr);}
    std::array<float,2> process(std::array<float,2> x) noexcept {if(count++==0)update();count%=16;for(int c=0;c<2;++c){double v=x[size_t(c)],h=hp[size_t(c)].tick(v,gh,true);v+=hpMix*(h-v);double l=lp[size_t(c)].tick(v,gl,false);x[size_t(c)]=float(v+lpMix*(l-v));}return x;}
};
/*
    Dynamic band selector (v0.9).

    v0.8 used a Linkwitz-Riley three way splitter: one branch was scaled and the
    branches were summed back together. That put a permanent phase step and a
    magnitude ripple (up to +0.9 dB, measured) around both crossover points and
    cost 3-6 ms of group delay at bass crossover frequencies, even when nothing
    was being ducked.

    v0.9 never filters the dry path. The filter only extracts the selection S(x)
    and the engine applies

        out = dry - (1 - g) * S(dry)

    which is exactly a dynamic bell / shelf filter:
      - at g == 1 (no ducking) the output is bit-identical to the input, so there
        is no filter in the path at rest;
      - the magnitude moves smoothly with no overshoot anywhere (measured: never
        above unity);
      - the phase deviation scales with the ducking depth instead of being
        permanent, and the group delay is that of a single second order section
        (< 1 ms at 60 Hz instead of 3-6 ms).

    Three shapes, chosen from where the processing range handles sit:
      - both handles moved  -> normalised SVF bandpass -> dynamic bell
      - only the low handle -> first order highpass     -> dynamic high shelf
      - only the high handle-> first order lowpass      -> dynamic low shelf

    blend() reports engagement and stays at zero while both handles sit at
    20 Hz / 20 kHz, so the plugin keeps its plain wideband duck until the user
    starts moving one of the processing range handles.
*/
struct OnePoleTPT {
    double s=0;
    void clear() noexcept {s=0;}
    double lp(double x,double g) noexcept {const double v=(x-s)*g/(1+g),y=v+s;s=y+v;return y;}
};
struct SvfBandpass {
    double ic1=0,ic2=0;
    void clear() noexcept {ic1=ic2=0;}
    /* Bandpass normalised to unity magnitude at the centre frequency. */
    double normalised(double x,double g,double k) noexcept {
        const double a1=1/(1+g*(g+k)),a2=g*a1,a3=g*a2;
        const double v3=x-ic2,v1=a1*ic1+a2*v3,v2=ic2+a2*ic1+a3*v3;
        ic1=2*v1-ic1;ic2=2*v2-ic2;
        return k*v1;
    }
};
class DynamicBandFilter {
    std::array<SvfBandpass,2> bell{};
    std::array<OnePoleTPT,2> lowShelf{},highShelf{};
    double sr=48000,lowLog=std::log(20.),highLog=std::log(20000.),targetLow=20,targetHigh=20000;
    double gc=0,gl=0,gh=0,k=1,wHp=0,wLp=0,wBell=0,wHigh=0,wLow=0;
    double step=0,mix=0,targetMix=0,mixStep=0;int count=0;
public:
    void reset(double rate) noexcept {
        sr=std::max(100.,rate);
        for(size_t c=0;c<2;++c){bell[c].clear();lowShelf[c].clear();highShelf[c].clear();}
        targetLow=20;targetHigh=20000;lowLog=std::log(20.);highLog=std::log(20000.);
        wHp=wLp=wBell=wHigh=wLow=0;mix=targetMix=0;count=0;
        step=std::exp(-16/(sr*.01));mixStep=std::exp(-16/(sr*.02));update();
    }
    void set(float low,float high) noexcept {
        targetLow=std::clamp(double(std::min(low,high)),20.,20000.);
        targetHigh=std::clamp(double(std::max(low,high)),20.,20000.);
        targetMix=(targetLow>20.5||targetHigh<19500.)?1.:0.;
    }
    bool engaged() const noexcept {return mix>0||targetMix>0;}
    float blend() const noexcept {return float(mix);}
    double lowHz() const noexcept {return std::exp(lowLog);}
    double highHz() const noexcept {return std::exp(highLog);}
    void update() noexcept {
        lowLog=std::log(targetLow)+step*(lowLog-std::log(targetLow));
        highLog=std::log(targetHigh)+step*(highLog-std::log(targetHigh));
        mix=targetMix+mixStep*(mix-targetMix);if(std::abs(mix-targetMix)<1e-6)mix=targetMix;
        const double useHp=targetLow>20.5?1:0,useLp=targetHigh<19500.?1:0;
        wHp=useHp+step*(wHp-useHp);wLp=useLp+step*(wLp-useLp);
        if(std::abs(wHp-useHp)<1e-8)wHp=useHp;if(std::abs(wLp-useLp)<1e-8)wLp=useLp;
        wBell=wHp*wLp;wHigh=wHp*(1-wLp);wLow=wLp*(1-wHp);
        const double low=std::exp(lowLog),high=std::exp(highLog);
        const double centre=std::sqrt(std::max(20.,low)*std::max(20.,high));
        const double q=std::clamp(centre/std::max(1.,high-low),.9,8.);
        k=1/q;
        const double nyquist=sr*.45;
        gc=std::tan(3.141592653589793*std::min(centre,nyquist)/sr);
        gl=std::tan(3.141592653589793*std::min(high,nyquist)/sr);
        gh=std::tan(3.141592653589793*std::min(low,nyquist)/sr);
    }
    /* Returns the selection that should be ducked. With both handles parked every
       weight is zero and the engine falls back to its wideband path. */
    std::array<float,2> process(std::array<float,2> x) noexcept {
        if(count++==0)update();count%=16;
        for(size_t c=0;c<2;++c){
            const double v=x[c];
            const double band=bell[c].normalised(v,gc,k);
            const double low=lowShelf[c].lp(v,gl);
            const double high=v-highShelf[c].lp(v,gh);
            x[c]=float(wBell*band+wLow*low+wHigh*high);
        }
        return x;
    }
};
inline float effectiveDepth(float amount) noexcept {amount=std::clamp(amount,0.f,1.5f);return amount<=1?amount:std::exp2(6.f*(amount-1.f));}
inline float depthGain(float baseGain,float influence) noexcept {return std::clamp(1.f-effectiveDepth(influence)*(1.f-baseGain),0.f,1.f);}
}
