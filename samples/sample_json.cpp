#include "castle/serialization/json.hpp"

int main()
{
    using document = castle::serialization::json::document<32U, 512U, 8U>;
    using node_id = document::node_id;
    using string = castle::container::string<512U>;

    document doc;
    node_id root = document::npos;
    node_id device = document::npos;
    node_id name = document::npos;
    node_id enabled = document::npos;
    node_id channels = document::npos;
    node_id channel = document::npos;

    (void)doc.make_object(root);
    (void)doc.make_object(device, root, "device");
    (void)doc.set_string(name, "castle", device, "name");
    (void)doc.set_bool(enabled, true, device, "enabled");
    (void)doc.make_array(channels, root, "channels");
    (void)doc.set_integer(channel, 115200, channels);

    bool is_enabled = false;
    (void)doc.get_bool(enabled, is_enabled);

    string compact;
    (void)castle::serialization::json::serialize(doc, compact);

    string pretty;
    (void)castle::serialization::json::serialize(doc, pretty, true, 2U);

    // compact.c_str() / pretty.c_str() can now be sent to a transport.
    (void)is_enabled;
    (void)compact;
    (void)pretty;

    return 0;
}
