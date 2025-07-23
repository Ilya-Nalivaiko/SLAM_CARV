// Ilya Nalivaiko 2025

#include "Notifier.h"

static zmq::context_t ctx(1);

// Notifies the server at [notifyAddress] over ZMQ
// to download [chunkID] from the HTTP server at [ownAddress]
// ownAddress technically does not have to be the same IP as this program but that is the easiest
// for simplicity both adress strings should already include the : sockets
// IPV4 adresses ONLY. IPV6 does not work rn
bool notifyUpdate(int chunkId, const std::string& notifyAddress, const std::string& ownAddress) {
    try {
        zmq::socket_t sock(ctx, zmq::socket_type::push);
        sock.setsockopt(ZMQ_SNDTIMEO, 500); // set 500ms timeout

        std::cout << "[ZMQ] Connecting to ZMQ socket\n";
        sock.connect("tcp://" + notifyAddress);
        std::cout << "[ZMQ] Connection to ZMQ socket established\n";

        std::string msg = "UPDATE:http://" + ownAddress + "/chunk/" + std::to_string(chunkId);
 
        msg += "?_=" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

        std::cout << "[ZMQ] Sending update announcement to ZMQ socket:\n"
                  << "      " << notifyAddress << "\n"
                  << "      Content:\n"
                  << "      " << msg << "\n";
        auto result = sock.send(zmq::buffer(msg), zmq::send_flags::none);

        if (!result.has_value()) {
            std::cerr << "[ZMQ] Failed to send update (no result timeout)\n";
            return false;
        }

        std::cout << "[ZMQ] Send returned: " << *result << " bytes\n";

        return true;

    } catch (const zmq::error_t& e) {
        std::cerr << "[ZMQ] ZMQ Error: " << e.what() << "\n";
        return false;
    }
}
