// Ilya Nalivaiko 2025

#pragma once
#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>
#include "ChunkCache.h"
#include <httplib.h>
#include <thread>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <iostream>

class HttpService {
public:
    HttpService(int port, ChunkCache& cache);
    void start();
    void stop();
private:
    httplib::Server server_;
    ChunkCache& cache_;
    int port_;
    std::thread server_thread_;
};