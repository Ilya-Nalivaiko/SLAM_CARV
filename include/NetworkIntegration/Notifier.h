// Ilya Nalivaiko 2025

#pragma once
#include <iostream>
#include <external/zmq.hpp>
#include <ctime>
bool notifyUpdate(int chunkId, const std::string& notifyAddress, const std::string& ownAddress);

