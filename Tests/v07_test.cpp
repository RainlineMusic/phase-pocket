#include "../Source/PocketDSP.h"
#include <iostream>
#include <memory>
#include <random>
#include <cstdlib>
static void check(bool ok,const char* name){if(!ok){std::cerr<<"FAIL "<<name<<'\n';std::exit(1);}}
int main(){
    check(pocket::Engine::latencyForRate(48000)==48,"1 ms = 48 samples");
    check(pocket::Engine::latencyForRate(44100)==45,"44.1k latency rounds up");
    for(float d:{5.f,10.f,100.f,200.f}){
        check(pocket::Engine::durationGain(d*.5,d)>.9999f,"full hold at 50 percent");
        check(std::abs(pocket::Engine::durationGain(d*.75,d)-.5f)<1e-5f,"half fade at 75 percent");
        check(pocket::Engine::durationGain(d,d)<1e-6f,"zero at requested duration");
    }
    for(double sr:{44100.,48000.,96000.,192000.}){
        const int latency=pocket::Engine::latencyForRate(sr);
        std::mt19937 random(17);std::uniform_real_distribution<float> noise(-.8f,.8f);
        std::vector<std::array<float,2>> input(16000);for(auto& s:input)s={noise(random),noise(random)};
        for(bool noKey:{true,false}){
            auto e=std::make_unique<pocket::Engine>();e->reset(sr,noKey?1.f:0.f);e->configure(noKey?1.f:0.f,2000);
            for(size_t i=0;i<input.size();++i){auto v=e->process(input[i],noKey?std::array<float,2>{0,0}:std::array<float,2>{noise(random),noise(random)});auto wanted=i>=size_t(latency)?input[i-size_t(latency)]:std::array<float,2>{0,0};check(v.out==wanted,"delayed dry identity");}
        }
        {auto e=std::make_unique<pocket::Engine>();e->reset(sr);e->configure(1,2000);for(int n=0;n<latency+8;++n){float x=n==0?1.f:0.f;auto v=e->process({x,x},{x,x});if(n==latency)check(std::abs(v.out[0])<1e-6f,"first impulse fully caught");}}
        for(float duration:{5.f,10.f,100.f,200.f}){
            auto e=std::make_unique<pocket::Engine>();e->reset(sr);e->configure(1,duration);
            float beforeFade=1,halfFade=0,afterEnd=0;
            for(int n=0;n<int(sr*.27)+latency;++n){auto v=e->process({1,1},{.8f,.8f});int aligned=n-latency;if(aligned<0)continue;double ms=aligned*1000./sr;
                if(aligned==int(std::floor(duration*.45*sr*.001)))beforeFade=v.gain;
                if(aligned==int(std::ceil(duration*.75*sr*.001)))halfFade=v.gain;
                if(ms>=duration+.1){afterEnd=v.gain;check(std::abs(v.gain-1)<1e-5f,"no smoothing extension or event retrigger");}
            }
            check(beforeFade<.21f,"duration keeps full depth before fade");check(halfFade>.3f&&halfFade<.9f,"audible proportional fade");check(afterEnd>.9999f,"requested length is real");
        }
        {auto e=std::make_unique<pocket::Engine>();e->reset(sr);e->configure(1,2000);float g=1;for(int n=0;n<int(sr*.3);++n)g=e->process({1,1},{.8f,.8f}).gain;check(g<.21f,"infinity does not shorten");}
        {auto e=std::make_unique<pocket::Engine>();e->reset(sr);e->configure(1,2000);float g=0;for(int n=0;n<int(sr*.12);++n)g=e->process({1,1},{n==0?1.f:0.f,0}).gain;check(g>.9999f,"short natural key-end release");}
        for(float balance:{-1.f,1.f}){auto e=std::make_unique<pocket::Engine>();e->reset(sr);e->configure(1,2000,20,20000,false,balance);pocket::Sample v;for(int n=0;n<int(sr*.2);++n)v=e->process(balance<0?std::array<float,2>{.5f,-.5f}:std::array<float,2>{.5f,.5f},{1,1});check(std::abs(v.out[0]-.5f)<1e-5f,"M/S leaves excluded component dry");}
        {auto e=std::make_unique<pocket::Engine>();e->reset(sr);e->configure(1,2000,20,20000,true);pocket::Sample v;for(int n=0;n<int(sr*.2);++n)v=e->process({.3f,-.5f},{1,1});check(std::abs(v.out[0]-.3f)<1e-5f&&std::abs(v.out[1]+.5f)<1e-5f,"bypass is delayed dry");}
    }
    std::cout<<"PASS v0.7: identity, first attack, 1ms lookahead, 1/2/5/10/100/200ms duration, 20% fade, infinity, M/S and bypass\n";
}
