#include <fost/internet>
#include <fost/http.server.hpp>
#include <fost/log>
#include <proxy/cache.hpp>
#include <proxy/webserver.hpp>
#include <proxy/views.hpp>


namespace {
    fostlib::worker g_server;
    boost::shared_ptr< fostlib::detail::future_result< void > > g_running;

    boost::mutex g_terminate_lock;
    bool g_terminate = false;

    std::unique_ptr< fostlib::setting<fostlib::string> > g_new_root;
}


void proxy::start(const boost::filesystem::wpath &root) {
    fostlib::log::debug()
        ("saving-root", root);
    g_new_root.reset(new fostlib::setting<fostlib::string>(
        "proxy::start", c_cache_dir, fostlib::coerce<fostlib::string>(root)));
    // Start the web server and set the termination condition
    g_running = g_server([]() {
        fostlib::http::server server(fostlib::host(0), 2555);
        server(fostlib::urlhandler::service, []() {
            boost::mutex::scoped_lock lock(g_terminate_lock);
            return g_terminate;
        });
    });
}


void proxy::wait() {
    g_running->wait();
}


void proxy::stop() {
    { // Tell the server to stop
        boost::mutex::scoped_lock lock(g_terminate_lock);
        g_terminate = true;
    }
    // Tickle the port so it notices
    fostlib::network_connection tickle(fostlib::host("localhost"), 2555);
    // Clear the root setting
    g_new_root.reset(nullptr);
}
