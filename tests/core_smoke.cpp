#include "baojiaozi/version.hpp"

#include <cassert>

int main() {
    assert(baojiaozi::Version() == "0.1.0-dev");
    return 0;
}
