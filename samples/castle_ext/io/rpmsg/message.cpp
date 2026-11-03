#include "sample_support.hpp"
#include "castle_ext/io/rpmsg/message.hpp"

using namespace castle::io::rpmsg;

int main()
{
    const uint8_t payload[] = {0x10U, 0x20U, 0x30U};
    message_view view(1500U, 1100U, payload, sizeof(payload));
    CASTLE_SAMPLE_CHECK(!view.empty());
    CASTLE_SAMPLE_CHECK(view.source == 1500U && view.destination == 1100U);
    CASTLE_SAMPLE_CHECK(view.size == 3U && view.data[1] == 0x20U);
    message_view empty;
    CASTLE_SAMPLE_CHECK(empty.empty());
    tx_buffer tx;
    tx.data = nullptr;
    tx.capacity = 12U;
    tx.endpoint_token = &view;
    tx.active = true;
    tx.reset();
    CASTLE_SAMPLE_CHECK(!tx.active && tx.data == nullptr && tx.capacity == 0U);
    return 0;
}
