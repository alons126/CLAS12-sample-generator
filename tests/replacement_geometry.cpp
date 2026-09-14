#include "common/TargetGeometry.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
int main() {
    try {
        samples::TargetGeometry argon("Ar"), added("replacement-only");
        TRandom3 first(17), interleaved(123), reference(17);
        for (int i=0; i<100; ++i) {
            const auto a=argon.sample(first);
            const auto b=added.sample(interleaved);
            const auto repeated=argon.sample(reference);
            if (a != repeated) throw std::runtime_error("Interleaved RNG streams changed vertices");
            if (a.Z() < -15.75 || a.Z() > -15.25 || std::abs(a.X()-0.75)>0.25 || b.Z()!=-12.)
                throw std::runtime_error("Adapter ignored replacement targets.h geometry");
        }
        std::cout << "Replacing only targets.h changes target positions, beamspot and accepted target names.\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
