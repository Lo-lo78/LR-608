// SPDX-License-Identifier: AGPL-3.0-or-later
#include <iostream>
#include "SlotGridNavigation.h"
#define CHECK(x) do {if(!(x)) {std::cerr<<"FAILED "<<__LINE__<<": "#x"\n";return 1;}} while(false)
int main()
{
    using namespace lr608;
    CHECK(slotUiNavigate(0,1,0)==1); // Slot -> Engine
    CHECK(slotUiNavigate(1,1,0)==2); // Engine -> Layer
    CHECK(slotUiNavigate(2,1,0)==2); // stop at bottom
    CHECK(slotUiNavigate(2,-1,0)==1);
    CHECK(slotUiNavigate(0,-1,0)==0);
    CHECK(slotUiNavigate(2,0,1)==5); // Layer -> Choke Target, same row
    CHECK(slotUiNavigate(5,0,1)==8); // Choke Target -> Roll Reduction
    CHECK(slotUiNavigate(8,0,1)==9); // only Output in fourth column
    CHECK(slotUiNavigate(9,0,-1)==6); // Output -> top Roll entry
    CHECK(slotUiNavigate(7,0,-1)==4); // Roll Shuffle -> Choke Trigger
    CHECK(slotUiNavigate(4,0,1)==7);
    CHECK(slotUiNavigate(7,-5,0)==6); // Page Up clamps within column
    CHECK(slotUiNavigate(7,5,0)==8); // Page Down clamps within column
    CHECK(slotUiNavigate(9,5,0)==9); // last column has one entry
    CHECK(slotUiReverseValueOrder(0));
    CHECK(slotUiReverseValueOrder(9));
    CHECK(!slotUiReverseValueOrder(6)); // Roll Division increases with Alt+Up
    CHECK(!slotUiReverseValueOrder(8)); // Roll Reduction increases with Alt+Up
    std::cout<<"PASS LR608 Alt+D vertical grid geometry and value direction\n";
}
