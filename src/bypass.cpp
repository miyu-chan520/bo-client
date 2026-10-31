#include "bypass/bypass.hpp"

extern "C" void init_bypass() {
    bypass::init_combined_bypass();
}
