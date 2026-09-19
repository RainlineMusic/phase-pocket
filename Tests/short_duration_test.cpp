#include "../Source/PocketDSP.h"
#include <cmath>
#include <iostream>
#include <cstdlib>
static void check(bool x,const char* m){if(!x){std::cerr<<"FAIL "<<m<<"\n";std::exit(1);}}
int main(){
 for(float duration:{1.f,2.f,5.f,10.f,20.f}){
  pocket::Engine e;e.reset(48000);e.configure(1.f,duration);
  int clusters=0;bool cutting=false;
  for(int n=0;n<4800;++n){
   const double t=n/48000.;
   const float tail=n<1800?float(std::sin(6.283185307179586*900*t)*std::exp(-t/.009)):0.f;
   const auto v=e.process({1,1},{tail,tail});
   const bool now=v.gain<.985f;
   if(now&&!cutting)++clusters;
   cutting=now;
  }
  std::cout<<duration<<" ms clusters="<<clusters<<"\n";
  check(clusters==1,"short key tail must create exactly one duck event");
 }
 std::cout<<"PASS short-duration single-event regression\n";
}
