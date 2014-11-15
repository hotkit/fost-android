/*
    Copyright 2014 Felspar Co Ltd. http://support.felspar.com/
    Distributed under the Boost Software License, Version 1.0.
    See accompanying file LICENSE_1_0.txt or copy at
        http://www.boost.org/LICENSE_1_0.txt
*/


#include "fost-android.hpp"
#include <fost/internet>
#include <fost/http-cache.hpp>
#include <fost/http.server.hpp>
#include <fost/log>
#include <fost/urlhandler>


namespace {
    fostlib::worker g_server;
    boost::shared_ptr< fostlib::detail::future_result< void > > g_running;

    boost::mutex g_terminate_lock;
    bool g_terminate = false;

    std::unique_ptr< fostlib::setting<fostlib::string> > g_new_root;
}


extern "C" JNIEXPORT void JNICALL
Java_com_felspar_android_WebServer_start(
    JNIEnv *env, jobject self, jstring jdataroot
) {
    boost::filesystem::wpath dataroot(
        fostlib::jni_cast<boost::filesystem::wpath>(env, jdataroot));
    g_new_root.reset(new fostlib::setting<fostlib::string>(
        "Java_com_felspar_android_WebServer_start",
        fostlib::c_cache_dir, fostlib::coerce<fostlib::string>(dataroot / "rproxy")));
    // Start the web server and set the termination condition
    g_running = g_server([]() {
        fostlib::http::server server(fostlib::host(0), 2555);
        server(fostlib::urlhandler::service, []() {
            boost::mutex::scoped_lock lock(g_terminate_lock);
            return g_terminate;
        });
    });
    fostlib::log::info().module("JNI.com.felspar.android.WebServer")
        ("dataroot", dataroot),
        ("rproxy", g_new_root->value());
}


extern "C" JNIEXPORT void JNICALL
Java_com_felspar_android_WebServer_stop(
    JNIEnv *env, jobject self
) {
    { // Tell the server to stop
        boost::mutex::scoped_lock lock(g_terminate_lock);
        g_terminate = true;
    }
    // Tickle the port so it notices
    fostlib::network_connection tickle(fostlib::host("localhost"), 2555);
    // Clear the root setting
    g_new_root.reset(nullptr);
    g_running->wait();
}

