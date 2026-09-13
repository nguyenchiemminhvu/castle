/**
 * Castle 2.0 sample: <castle/memory/storage.h>
 *
 * Scenario: reserve aligned, uninitialised bytes for an object that will be
 * constructed and destroyed explicitly by the application.
 */
#include "sample_support.hpp"
#include "castle/memory/alignment.hpp"
#include "castle/memory/storage.hpp"

#include <stdint.h>

struct RegisterSnapshot
{
    uint32_t status;
};

int main()
{
    castle::memory::aligned_storage_as_t<sizeof(RegisterSnapshot), RegisterSnapshot> storage;
    RegisterSnapshot* snapshot = storage.get_address<RegisterSnapshot>();
    (void)snapshot;
    return 0;
}
