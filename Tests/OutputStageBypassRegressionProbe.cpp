// SPDX-License-Identifier: AGPL-3.0-or-later
// For bypass comparison, compile this probe against OutputStage.cpp/h from
// original LR-608 0.99.33 and TEST34 with the SAME compiler and flags.
// Expected local digest for both (g++ -O2 on this test system):
// baa46810d0a66e0c. Other toolchains may differ in last floating bits.
#include "OutputStage.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
int main()
{
  lr608::OutputStage stage;
  stage.prepare(48000.0);
  std::uint64_t digest=1469598103934665603ull;
  auto mix=[&](double v) {const auto* p = reinterpret_cast<const unsigned char*>(&v);for (unsigned i=0;i<sizeof(v);++i)digest=(digest^p[i])*1099511628211ull;};
  for(int i=0;i<60000;++i)
  {
    std::array<lr608::StereoSample,lr608::OutputStage::stemCount> stems{};
    stems[0]={std::sin(i*0.055)*0.6+std::sin(i*0.11)*0.25,std::sin(i*0.057)*0.6};
    stems[3]={std::sin(i*0.07)*1.8,std::sin(i*0.071)*1.8};
    stems[31]={std::sin(i*0.13)*0.32,std::sin(i*0.13)*0.32};
    stage.process(stems,true,i<30000?-3.0:1.0,0);
    for(auto& s:stems){mix(s.left);mix(s.right);}
  }
  std::printf("%016llx\n",static_cast<unsigned long long>(digest));
}
