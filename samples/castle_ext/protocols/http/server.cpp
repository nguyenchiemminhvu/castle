#include "sample_support.hpp"
#include "castle_ext/protocols/http/server.hpp"
#include "castle/chrono/chrono.hpp"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

namespace
{
volatile sig_atomic_t stop_requested = 0;

void handle_signal(int)
{
    stop_requested = 1;
}

void sleep_for_ms(castle::chrono::milliseconds duration)
{
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

    struct timespec deadline{};
    clock_gettime(CLOCK_REALTIME, &deadline);
    const long total_ns = deadline.tv_nsec + static_cast<long>(duration.count()) * 1000000L;
    deadline.tv_sec += total_ns / 1000000000L;
    deadline.tv_nsec = total_ns % 1000000000L;

    pthread_mutex_lock(&mutex);
    while (pthread_cond_timedwait(&condition, &mutex, &deadline) != ETIMEDOUT)
    {
    }
    pthread_mutex_unlock(&mutex);
}

castle::protocols::http::string_view append_text(char* output, castle::size_type capacity,
                                                  castle::size_type& used,
                                                  castle::protocols::http::string_view value)
{
    if (value.size() > capacity - used)
    {
        return castle::protocols::http::string_view();
    }
    for (castle::size_type i = 0U; i < value.size(); ++i)
    {
        output[used++] = value[i];
    }
    return castle::protocols::http::string_view(output, used);
}
}

int main(int argc, char* argv[])
{
    using namespace castle;
    using namespace castle::protocols::http;

    HttpServer server;
    server.register_handler(HttpMethod::GET, "/health",
        [](HttpRequest const&, HttpResponse& response)
        {
            response.set_header("Content-Type", "text/plain; charset=utf-8");
            response.set_body("ok");
        });
    CASTLE_SAMPLE_CHECK(server.last_registration_status() == status::ok);

    server.register_handler(HttpMethod::GET, "/api/status",
        [](HttpRequest const&, HttpResponse& response)
        {
            response.set_json("{\"status\":\"ok\",\"service\":\"CASTLE\"}");
        });
    CASTLE_SAMPLE_CHECK(server.last_registration_status() == status::ok);

    server.register_handler(HttpMethod::GET, "/devices/{id}/state",
        [](HttpRequest const& request, HttpResponse& response)
        {
            char body[96U]{};
            size_type used = 0U;
            append_text(body, sizeof(body), used, string_view("device="));
            append_text(body, sizeof(body), used, request.path_param("id"));
            append_text(body, sizeof(body), used, string_view(";refresh="));
            append_text(body, sizeof(body), used, request.query_param("refresh"));
            response.set_header("Content-Type", "text/plain");
            response.set_body(string_view(body, used));
        });
    CASTLE_SAMPLE_CHECK(server.last_registration_status() == status::ok);

    server.register_handler(HttpMethod::POST, "/api/devices",
        [](HttpRequest const& request, HttpResponse& response)
        {
            response.set_status(status_code::created);
            response.set_header("Location", "/devices/created");
            response.set_json(request.body());
        });
    CASTLE_SAMPLE_CHECK(server.last_registration_status() == status::ok);

    server.register_handler(HttpMethod::PUT, "/devices/{id}/state",
        [](HttpRequest const& request, HttpResponse& response)
        {
            response.set_header("Content-Type", "text/plain");
            response.set_body(request.body_size() == 0U ? string_view("updated") : request.body());
        });
    CASTLE_SAMPLE_CHECK(server.last_registration_status() == status::ok);

    server.register_handler(HttpMethod::DELETE, "/devices/{id}",
        [](HttpRequest const&, HttpResponse& response)
        {
            response.set_status(status_code::no_content);
        });
    CASTLE_SAMPLE_CHECK(server.last_registration_status() == status::ok);

    server.register_handler(HttpMethod::HEAD, "/health",
        [](HttpRequest const&, HttpResponse& response)
        {
            response.set_header("Content-Type", "text/plain");
            response.set_body("ok"); // The server suppresses the body for HEAD.
        });
    CASTLE_SAMPLE_CHECK(server.last_registration_status() == status::ok);

    server.register_handler(HttpMethod::OPTIONS, "/api/status",
        [](HttpRequest const&, HttpResponse& response)
        {
            response.set_header("Allow", "GET, HEAD, OPTIONS");
            response.set_status(status_code::no_content);
        });
    CASTLE_SAMPLE_CHECK(server.last_registration_status() == status::ok);

    server.register_prefix_handler(HttpMethod::GET, "/api/v1",
        [](HttpRequest const& request, HttpResponse& response)
        {
            response.set_body(request.path());
        });
    CASTLE_SAMPLE_CHECK(server.last_registration_status() == status::ok);

    server.register_handler(HttpMethod::GET, "/static",
        [](HttpRequest const&, HttpResponse& response)
        {
            response.set_file("text/html; charset=utf-8", "<html><body>CASTLE</body></html>");
        });
    CASTLE_SAMPLE_CHECK(server.last_registration_status() == status::ok);

    server.set_not_found_handler(
        [](HttpRequest const&, HttpResponse& response)
        {
            response.set_status(status_code::not_found);
            response.set_body("resource not registered");
        });
    CASTLE_SAMPLE_CHECK(server.last_registration_status() == status::ok);

    uint16_t port = 8080U;
    if (argc > 1)
    {
        char* end = nullptr;
        unsigned long parsed = strtoul(argv[1], &end, 10);
        CASTLE_SAMPLE_CHECK(end != argv[1] && *end == '\0' && parsed > 0U && parsed <= 65535U);
        port = static_cast<uint16_t>(parsed);
    }
    const net::endpoint listen_address = net::endpoint::loopback_ipv4(port);
    CASTLE_SAMPLE_CHECK(server.start(listen_address) == status::ok);
    signal(SIGINT, handle_signal);

    // Run the bounded server loop on this thread until Ctrl+C is received.
    while (stop_requested == 0)
    {
        static_cast<void>(server.poll(0));
        sleep_for_ms(castle::chrono::milliseconds(1));
    }

    CASTLE_SAMPLE_CHECK(server.stop() == status::ok);
    return 0;
}
