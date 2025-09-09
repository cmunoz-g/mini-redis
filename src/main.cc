#include "server.hh"

int main(void) {
    HMap db = HMap{};
    return run_server(db, "0.0.0.0", 1234);
}