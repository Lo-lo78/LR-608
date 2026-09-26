// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Kick808Voice.h"
#include "KickOtherVoices.h"
#include "SnareVoice.h"
#include "ClapRimVoices.h"
#include "TomVoice.h"
#include "HatCymbalVoices.h"
#include "MaracasVoice.h"
#include "CowbellVoice.h"
#include "ZapVoice.h"
#include "OutputStage.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>

int main()
{
    lr608::Kick808Parameters p {
        10.0, 0.26, 0.46, 0.061, 0.2, 0.12, 0.305, 0.79, 0.0, 0.0, 1.0,
        15.0, 0.1, 0.0, 4.0, 0, 0.0, 200.0, 40.0, 0.5, 0.0, 0.0, 0.5,
        3.0, 0.5, -18.0, 0.0, 8.0, 80.0, 0.0, 1, 112.0, 1.0, 0.0
    };
    lr608::Kick808Voice voice;
    voice.prepare (48000.0); voice.trigger (127, p);
    double energy = 0.0;
    for (int i = 0; i < 48000; ++i)
    {
        const auto sample = voice.render (p, 120.0);
        if (! std::isfinite (sample)) { std::cerr << "Non-finite Kick sample\n"; return 1; }
        energy += sample * sample;
    }
    if (energy <= 0.01) { std::cerr << "Kick is silent\n"; return 1; }
    lr608::KickOtherVoices other;
    for (int engine = 1; engine <= 6; ++engine)
    {
        other.prepare (48000.0); other.trigger (engine, 127, p);
        double engineEnergy = 0.0;
        for (int i = 0; i < 48000; ++i)
        {
            const auto sample = other.render (p);
            if (! std::isfinite (sample)) { std::cerr << "Non-finite Kick engine " << engine << '\n'; return 1; }
            engineEnergy += sample * sample;
        }
        if (engineEnergy <= 0.0001) { std::cerr << "Silent Kick engine " << engine << '\n'; return 1; }
        std::cout << "Kick engine " << engine << " energy=" << engineEnergy << '\n';
    }
    const double snareFactory[9][31] {
        {20.2,.8,.014,.05,.6,.25,0,0,.94,.03,.66,.55,.2,0,.1,.2,1,50,.7,120,.1,0,0,0,.5,-18,1,0,8,90,10},
        {12.45,1.1,.04,-1,.52,.37,.3,1,1.25,.06,.75,.55,.35,0,.5,.32,1,7,.25,500,.2,1,.62,.04,.5,-18,1,0,8,90,0},
        {43,1,.25,.05,.5,.5,.5,.4,.1,.06,.1,.4,6.9,.4,.5,.05,.5,500,.5,100,0,1,.35,.2,.31,-18,1,14,4,20,80},
        {40,1.5,.11,-1.3,1,.79,.8,0,1,0,0,1,1.6,.003,1,.7,.65,0,.5,1,1,1,0,.08,.55,-18,1,7,8,90,40},
        {50,1.5,.4,-2,1,.8,.6,1,.8,1,.74,.4,1.6,.5,1,1,0,100,.5,80,-.95,0,0,.08,.51,-18,1,7,8,90,50},
        {30.5,1.5,.4,-1,1,.8,.6,1,.8,1,.74,.4,1.6,.5,1,1,0,100,.5,80,-.8,0,0,.08,.51,-18,1,7,8,90,22},
        {19.4,2,.12,-1.3,1,0,.03,.1,.5,.85,1,.48,.14,.1,1,0,.6,180,.46,80,-.95,0,0,0,.1,-18,1,7,8,90,50},
        {50,1.5,.6,-2.4,.9,.8,.6,1,.7,.7,.7,.4,1.6,.5,1,1,0,100,.5,80,-.95,0,0,.08,.51,-18,1,7,8,90,50},
        {18,1.6,.06,0,.95,.56,0,1,1.61,0,.34,.34,2.2,.56,.54,.18,.74,55,.28,120,0,0,0,0,.52,-18,1,0,8,90,0}
    };
    for (int slot = 0; slot < 2; ++slot)
        for (int engine = 0; engine < 9; ++engine)
        {
            lr608::SnareParameters sp;
            std::copy(std::begin(snareFactory[engine]), std::end(snareFactory[engine]), sp.v.begin());
            lr608::SnareVoice snare; snare.prepare(48000.0); snare.trigger(engine, 127, sp);
            double snareEnergy = 0.0;
            for (int i = 0; i < 48000; ++i)
            {
                const auto sample = snare.render(sp);
                if (!std::isfinite(sample)) { std::cerr << "Non-finite Snare slot " << slot << " engine " << engine << '\n'; return 1; }
                snareEnergy += sample * sample;
            }
            if (snareEnergy <= 0.0001) { std::cerr << "Silent Snare slot " << slot << " engine " << engine << '\n'; return 1; }
            std::cout << "Snare slot " << slot << " engine " << engine << " energy=" << snareEnergy << '\n';
        }
    const double clapFactory[7][11]{{4,.12,.5,1,1.21,.15,0,.4,.9,1,.4},{4,.07,.62,.8,.16,.34,.38,.18,.72,.34,.34},{5,.02,.78,.7,.35,.52,.78,.3,.9,.6,.3},{2.4,.165,.5,1,0,0,.5,.5,.5,.5,0},{2.9,.165,.5,1,0,0,.5,.5,.5,.5,0},{4,.165,.5,1,0,0,.5,.5,.5,.5,0},{3.35,.02,.92,1,1.66,.70,2,1,1,1,.18}};
    for(int engine=0;engine<7;++engine){lr608::ClapParameters cp;std::copy(std::begin(clapFactory[engine]),std::end(clapFactory[engine]),cp.v.begin());lr608::ClapVoice clap;clap.prepare(48000);clap.trigger(engine,127,cp);double e=0;for(int i=0;i<96000;++i){const auto s=clap.render(cp);if(!std::isfinite(s.left)||!std::isfinite(s.right)){std::cerr<<"Non-finite Clap "<<engine<<'\n';return 1;}e+=s.left*s.left+s.right*s.right;}if(e<.0001){std::cerr<<"Silent Clap "<<engine<<'\n';return 1;}std::cout<<"Clap engine "<<engine<<" energy="<<e<<'\n';}
    const double rimFactory[6][18]{{7,.004,0,2,0,1.1,.05,0,.5,.7,.35,0,-16,1,0,3,60,50},{4,.5,.5,0,1.7,2.323,.05,0,.5,.7,.35,0,-16,1,0,3,60,50},{0,.5,.5,0,5,2.629,.05,0,.5,.7,.35,0,-16,1,0,3,60,50},{6,.5,.5,0,.5,2.736,.05,0,.5,.7,.35,0,-16,1,0,3,60,50},{8.4,.14,-.08,1.83,1.15,1.72,0,.05,.42,.7,.24,0,-16,1,0,3,60,0},{10.29,.16,.87,2.373333333333333,4.56,4,.08,.05,.48,.7,.35,0,-16,1,0,3,60,0}};
    for(int engine=0;engine<6;++engine){lr608::RimParameters rp;std::copy(std::begin(rimFactory[engine]),std::end(rimFactory[engine]),rp.v.begin());lr608::RimVoice rim;rim.prepare(48000);rim.trigger(engine,127,rp);double e=0;for(int i=0;i<96000;++i){const auto s=rim.render(rp);if(!std::isfinite(s.left)||!std::isfinite(s.right)){std::cerr<<"Non-finite Rim "<<engine<<'\n';return 1;}e+=s.left*s.left+s.right*s.right;}if(e<.0001){std::cerr<<"Silent Rim "<<engine<<'\n';return 1;}std::cout<<"Rim engine "<<engine<<" energy="<<e<<'\n';}
    const double tomFactory[5][54] {
        {7,7,7,-.5,0,.5,.57,.65,.82,0,.06,0,.041,.041,.016,.3,.16,.15,.1,.1,.1,0,0,0,0,0,0,.3,.3,.3,.15,.15,.15,.4,.4,.4,-18,1,0,6,100,0,-18,1,0,6,100,0,-18,1,0,5,90,0},
        {13,8.4,10,-.5,0,.5,1,1,1,0,-.06,-.38,.12,.09,.15,.51,.35,.32,.04,.03,.025,.6,.7,.4,100,100,100,1.3,1.3,1.3,1,1,1,.75,.76,.9,-18,1,7,6,100,50,-18,1,7,6,100,50,-18,1,7,6,100,50},
        {4.2,4.4,4.3,-.5,0,.5,.45,.65,1,.92,1,2,.028,.1,.01,.55,.3,.25,.1,.1,.1,.5,.5,.5,0,0,0,.2,.2,.2,.4,.4,.4,.5,.5,.5,-18,1,0,6,100,0,-18,1,0,6,100,0,-18,1,0,5,90,0},
        {2.7,2.6,2.5,-.5,0,.5,1,1,1,0,.08,2,.15,.15,.15,.2,.2,.2,.42,.4,.36,.5,.5,.5,0,0,0,.2,.2,.2,.4,.4,.4,.5,.5,.5,-18,1,0,6,100,0,-18,1,0,6,100,0,-18,1,0,5,90,0},
        {3.8,3.7,3.6,-.5,0,.5,1,1,1,-.16,-.04,1.08,.15,.15,.15,.2,.2,.2,.48,.45,.4,.5,.5,.5,0,0,0,.2,.2,.2,.4,.4,.4,.5,.5,.5,-18,1,0,6,100,0,-18,1,0,6,100,0,-18,1,0,5,90,0}
    };
    for(int engine=0;engine<5;++engine)for(int tom=0;tom<3;++tom){lr608::TomParameters tp;const int map[3][18]={{0,3,6,9,12,15,18,21,24,27,30,33,36,37,38,39,40,41},{1,4,7,10,13,16,19,22,25,28,31,34,42,43,44,45,46,47},{2,5,8,11,14,17,20,23,26,29,32,35,48,49,50,51,52,53}};for(int p=0;p<18;++p)tp.v[p]=tomFactory[engine][map[tom][p]];lr608::TomVoice voice(2-tom);voice.prepare(48000);voice.trigger(engine,127,tp);double e=0;for(int i=0;i<96000;++i){const auto s=voice.render(tp);if(!std::isfinite(s.left)||!std::isfinite(s.right)){std::cerr<<"Non-finite Tom "<<tom<<" engine "<<engine<<'\n';return 1;}e+=s.left*s.left+s.right*s.right;}if(e<.0001){std::cerr<<"Silent Tom "<<tom<<" engine "<<engine<<'\n';return 1;}std::cout<<"Tom "<<tom<<" engine "<<engine<<" energy="<<e<<'\n';}
    const double hatFactory[7][10]{{.72,.038,1,0,.72,0,.5,.5,0,80},{.68,.036,1,.03,.7,.1,.62,.5,.1,32},{1.02,.038,1,.13,.7,0,.26,.5,0,80},{.72,.038,1,0,.72,0,.5,.5,0,80},{1,.105,1.5,.65,1,.1,0,.541,.3,80},{1,.055,1.5,.5,.72,0,.42,.5,0,80},{1,.102,1.5,.6,.72,0,0,.43,.5,80}};
    for(int engine=0;engine<7;++engine)for(int open=0;open<2;++open){lr608::HatParameters hp;std::copy(std::begin(hatFactory[engine]),std::end(hatFactory[engine]),hp.v.begin());lr608::HatVoice hat;hat.prepare(48000);hat.trigger(engine,open!=0,127,hp);double e=0;for(int i=0;i<144000;++i){const auto s=hat.render(hp);if(!std::isfinite(s)){std::cerr<<"Non-finite HiHat "<<engine<<'\n';return 1;}e+=s*s;}if(e<.000001){std::cerr<<"Silent HiHat "<<engine<<" open "<<open<<'\n';return 1;}std::cout<<"HiHat engine "<<engine<<" open "<<open<<" energy="<<e<<'\n';}
    const double cymFactory[4][7]{{3.5,4,.3,.1,1,0,0},{1.4,2,.7,.8,.55,.48,.35},{2,3,1,.1,.55,.48,.35},{1.5,2.4,1,.1,1,0,0}};
    for(int engine=0;engine<4;++engine)for(int isCrash=0;isCrash<2;++isCrash){lr608::CymbalParameters cp;std::copy(std::begin(cymFactory[engine]),std::end(cymFactory[engine]),cp.v.begin());lr608::CymbalVoice cym(isCrash!=0);cym.prepare(48000);cym.trigger(engine,127,cp);double e=0;for(int i=0;i<144000;++i){const auto s=cym.render(cp);if(!std::isfinite(s)){std::cerr<<"Non-finite Cymbal "<<engine<<'\n';return 1;}e+=s*s;}if(e<.000001){std::cerr<<"Silent Cymbal "<<engine<<" crash "<<isCrash<<'\n';return 1;}std::cout<<"Cymbal engine "<<engine<<" crash "<<isCrash<<" energy="<<e<<'\n';}
    const double maracasFactory[4][8]{{5,.003,.005,.45,180,.55,.3,.2},{6,.01,.18,.5,180,.55,.2,.2},{4,.01,.18,.5,180,.55,.2,.2},{5,.01,.18,.5,180,.55,.2,.2}};
    for(int engine=0;engine<4;++engine){lr608::MaracasParameters mp;std::copy(std::begin(maracasFactory[engine]),std::end(maracasFactory[engine]),mp.v.begin());lr608::MaracasVoice maracas;maracas.prepare(48000);maracas.trigger(engine,127,mp);double e=0;for(int i=0;i<192000;++i){const auto s=maracas.render(mp);if(!std::isfinite(s)){std::cerr<<"Non-finite Maracas "<<engine<<'\n';return 1;}e+=s*s;}if(e<.000001){std::cerr<<"Silent Maracas "<<engine<<'\n';return 1;}std::cout<<"Maracas engine "<<engine<<" energy="<<e<<'\n';}
    const double cowFactory[8][11]{{8,.32,.45,.55,1,.4,.35,.3,10,.25,1},{6,.5,2,.5,0,.5,.5,0,0,0,1},{6,.1,2,.5,10,5,.5,0,0,0,1},{9,0,2.3,0,1,.4,.35,.3,5,.25,1},{6,.5,2,.5,0,.5,.5,0,0,0,1},{5,.8,2.75,1,4.5,5,.7,.3,3.5,.18,.16},{5,.8,2.75,1,4.5,5,.7,.3,3.5,.18,.16},{1,.50,2.0,.50,5.0,2.5,.50,.50,6.5,.54,55.0}};
    for(int engine=0;engine<8;++engine){lr608::CowbellParameters cp;std::copy(std::begin(cowFactory[engine]),std::end(cowFactory[engine]),cp.v.begin());lr608::CowbellVoice cow;cow.prepare(48000);cow.trigger(engine,127,cp);double e=0;for(int i=0;i<240000;++i){const auto s=cow.render(cp);if(!std::isfinite(s)){std::cerr<<"Non-finite Cowbell "<<engine<<'\n';return 1;}e+=s*s;}if(e<.000001){std::cerr<<"Silent Cowbell "<<engine<<'\n';return 1;}std::cout<<"Cowbell engine "<<engine<<" energy="<<e<<'\n';}
    const double zapFactory[11][22]{{1.6,.02,1.5,10,-.06,3,128,.1,3,0,.2,.5,0,180,0,0,-16,1,0,2,70,0},{1.5,.05,.7,6.5,.8,.8,64,4,5,.15,0,.32,.25,920,4,.8,-16,1,2,2,70,20},{1.3,.08,.75,4,.2,.4,256,2,5,.7,0,.58,.15,640,5,.35,-18,1,3,3,120,18},{2,.12,.9,7,-.4,.8,8,3,0,.4,0,.5,.35,480,0,.6,-18,1,4,4,160,25},{1.4,.32,.5,10,-1,2,32,1,5,.2,.06,.81,.2,350,4,.25,-18,1,2,2,120,18},{2,.2,1.2,8.5,-.2,.3,4,8,3,.05,.2,.7,.4,1200,0,.9,-20,2,3,1,180,35},{1.4,.04,.65,5,1.2,.7,512,5,5,.25,0,.65,.3,2400,5,.5,-18,1,2,1,80,22},{1.8,.06,.8,9,0,1.2,128,12,4,.02,0,.23,.5,666,4,1,-22,3,5,1,140,40},{2,.08,1.55,7.5,-.3,1.7,8,.35,5,.7,.1,.46,.15,220,0,.35,-18,1,3,4,180,22},{1.2,.06,1.25,7.2,.15,1.55,32,8,0,.18,.2,.58,.08,410,0,.25,-18,1,3,2,90,20},{1.4,.03,2,2.5,.5,1.5,6,1,1,.2,0,.42,.2,350,4,.25,-18,1,2,2,120,18}};
    for(int engine=0;engine<11;++engine){lr608::ZapParameters zp;std::copy(std::begin(zapFactory[engine]),std::end(zapFactory[engine]),zp.v.begin());lr608::ZapVoice zap;zap.prepare(48000);zap.trigger(engine,127,zp);double e=0;for(int i=0;i<240000;++i){const auto s=zap.render(zp);if(!std::isfinite(s)){std::cerr<<"Non-finite Zap "<<engine<<'\n';return 1;}e+=s*s;}if(e<.000001){std::cerr<<"Silent Zap "<<engine<<'\n';return 1;}std::cout<<"Zap engine "<<engine<<" energy="<<e<<'\n';}
    // A voice that has finished audibly must release its active flag, otherwise
    // the processor can never re-enter its true-audio-sleep branch.
    const auto terminationLimit = 30 * 48000;
    const auto requireStopped = [] (bool active, const char* family, int engine)
    {
        if (active) { std::cerr << "Voice does not terminate: " << family << " engine " << engine << '\n'; return false; }
        return true;
    };
    for(int engine=0;engine<8;++engine){if(engine==0){auto v=std::make_unique<lr608::Kick808Voice>();v->prepare(48000);v->trigger(127,p);for(int i=0;i<terminationLimit&&v->isActive();++i)v->render(p,120);if(!requireStopped(v->isActive(),"Kick",engine))return 1;}else{auto v=std::make_unique<lr608::KickOtherVoices>();v->prepare(48000);v->trigger(engine,127,p);for(int i=0;i<terminationLimit&&v->isActive();++i)v->render(p);if(!requireStopped(v->isActive(),"Kick",engine))return 1;}}
    for(int engine=0;engine<9;++engine){lr608::SnareParameters q;std::copy(std::begin(snareFactory[engine]),std::end(snareFactory[engine]),q.v.begin());auto v=std::make_unique<lr608::SnareVoice>();v->prepare(48000);v->trigger(engine,127,q);for(int i=0;i<terminationLimit&&v->isActive();++i)v->render(q);if(!requireStopped(v->isActive(),"Snare",engine))return 1;}
    for(int engine=0;engine<7;++engine){lr608::ClapParameters q;std::copy(std::begin(clapFactory[engine]),std::end(clapFactory[engine]),q.v.begin());auto v=std::make_unique<lr608::ClapVoice>();v->prepare(48000);v->trigger(engine,127,q);for(int i=0;i<terminationLimit&&v->isActive();++i)v->render(q);if(!requireStopped(v->isActive(),"Clap",engine))return 1;}
    for(int engine=0;engine<6;++engine){lr608::RimParameters q;std::copy(std::begin(rimFactory[engine]),std::end(rimFactory[engine]),q.v.begin());auto v=std::make_unique<lr608::RimVoice>();v->prepare(48000);v->trigger(engine,127,q);for(int i=0;i<terminationLimit&&v->isActive();++i)v->render(q);if(!requireStopped(v->isActive(),"Rim",engine))return 1;}
    for(int engine=0;engine<5;++engine)for(int tom=0;tom<3;++tom){lr608::TomParameters q;const int map[3][18]={{0,3,6,9,12,15,18,21,24,27,30,33,36,37,38,39,40,41},{1,4,7,10,13,16,19,22,25,28,31,34,42,43,44,45,46,47},{2,5,8,11,14,17,20,23,26,29,32,35,48,49,50,51,52,53}};for(int n=0;n<18;++n)q.v[n]=tomFactory[engine][map[tom][n]];auto v=std::make_unique<lr608::TomVoice>(2-tom);v->prepare(48000);v->trigger(engine,127,q);for(int i=0;i<terminationLimit&&v->isActive();++i)v->render(q);if(!requireStopped(v->isActive(),"Tom",engine))return 1;}
    for(int engine=0;engine<7;++engine)for(int open=0;open<2;++open){lr608::HatParameters q;std::copy(std::begin(hatFactory[engine]),std::end(hatFactory[engine]),q.v.begin());auto v=std::make_unique<lr608::HatVoice>();v->prepare(48000);v->trigger(engine,open!=0,127,q);for(int i=0;i<terminationLimit&&v->isActive();++i)v->render(q);if(!requireStopped(v->isActive(),open?"Open Hat":"Closed Hat",engine))return 1;}
    for(int engine=0;engine<4;++engine)for(int isCrash=0;isCrash<2;++isCrash){lr608::CymbalParameters q;std::copy(std::begin(cymFactory[engine]),std::end(cymFactory[engine]),q.v.begin());auto v=std::make_unique<lr608::CymbalVoice>(isCrash!=0);v->prepare(48000);v->trigger(engine,127,q);for(int i=0;i<terminationLimit&&v->isActive();++i)v->render(q);if(!requireStopped(v->isActive(),isCrash?"Crash":"Ride",engine))return 1;}
    for(int engine=0;engine<4;++engine){lr608::MaracasParameters q;std::copy(std::begin(maracasFactory[engine]),std::end(maracasFactory[engine]),q.v.begin());auto v=std::make_unique<lr608::MaracasVoice>();v->prepare(48000);v->trigger(engine,127,q);for(int i=0;i<terminationLimit&&v->isActive();++i)v->render(q);if(!requireStopped(v->isActive(),"Maracas",engine))return 1;}
    for(int engine=0;engine<8;++engine){lr608::CowbellParameters q;std::copy(std::begin(cowFactory[engine]),std::end(cowFactory[engine]),q.v.begin());auto v=std::make_unique<lr608::CowbellVoice>();v->prepare(48000);v->trigger(engine,127,q);double last=0;for(int i=0;i<terminationLimit&&v->isActive();++i)last=v->render(q);if(v->isActive())std::cerr<<"Cowbell residual="<<last<<'\n';if(!requireStopped(v->isActive(),"Cowbell",engine))return 1;}
    for(int engine=0;engine<11;++engine){lr608::ZapParameters q;std::copy(std::begin(zapFactory[engine]),std::end(zapFactory[engine]),q.v.begin());auto v=std::make_unique<lr608::ZapVoice>();v->prepare(48000);v->trigger(engine,127,q);for(int i=0;i<terminationLimit&&v->isActive();++i)v->render(q);if(!requireStopped(v->isActive(),"Zap",engine))return 1;}

    lr608::OutputStage output;
    output.prepare (48000.0);
    std::array<lr608::StereoSample, lr608::OutputStage::stemCount> stems {};
    stems[0] = { std::numeric_limits<double>::infinity(), 1.0e300 };
    output.process (stems, false, 0.0, 0);
    if (! std::isfinite (stems[0].left) || ! std::isfinite (stems[0].right)
        || std::abs (stems[0].left) > 8.0 || std::abs (stems[0].right) > 8.0)
    { std::cerr << "Output safety boundary failed\n"; return 1; }
    std::cout << "LR-608 Kick and both Snare engine audits passed; 808 energy=" << energy << '\n';
}
