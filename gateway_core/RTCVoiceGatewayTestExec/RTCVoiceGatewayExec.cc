#include <memory>
#include <fstream>
#include <filesystem> 
#include <vector>
#include <iostream>
#include <string>
#include <charconv>
#include <csignal>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>

#include "RTCVoiceServerCtrl.h"
#include "RTCSignalingSessionCtrl.h"

bool is_valid_ip(const std::string& ip) {
    struct in_addr addr {};
    return inet_pton(AF_INET, ip.c_str(), &addr) == 1;
}

bool get_machine_local_ipv4(std::string& out_ip) {
    struct ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) != 0) {
        return false;
    }

    bool found = false;
    for (const struct ifaddrs* it = interfaces; it != nullptr; it = it->ifa_next) {
        if (it->ifa_addr == nullptr ||
            it->ifa_addr->sa_family != AF_INET ||
            (it->ifa_flags & IFF_UP) == 0 ||
            (it->ifa_flags & IFF_LOOPBACK) != 0) {
            continue;
        }

        char ip_buffer[INET_ADDRSTRLEN] = {};
        const auto* addr = reinterpret_cast<const struct sockaddr_in*>(it->ifa_addr);
        if (inet_ntop(AF_INET, &addr->sin_addr, ip_buffer, sizeof(ip_buffer)) != nullptr) {
            out_ip = ip_buffer;
            found = true;
            break;
        }
    }

    freeifaddrs(interfaces);
    return found;
}

bool parse_port(const std::string& value, int& out_port) {
    int port = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), port);
    if (result.ec != std::errc() || result.ptr != value.data() + value.size() ||
        port < 1 || port > 65535) {
        return false;
    }

    out_port = port;
    return true;
}

bool parse_command_line(int argc, char* argv[], std::string& out_ip, int& out_port) {
    // Defaults: this machine's first active, non-loopback IPv4 address and signaling port 8080.
    if (!get_machine_local_ipv4(out_ip)) {
        std::cerr << "Failed to detect a non-loopback local IPv4 address. "
                     "Specify one with --ip." << std::endl;
        return false;
    }
    out_port = 8080;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if ((arg == "--ip" || arg == "-i") && i + 1 < argc) {
            out_ip = argv[++i];
            if (!is_valid_ip(out_ip)) {
                std::cerr << "Invalid IPv4 address: " << out_ip << std::endl;
                return false;
            }
        } else if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            const std::string port_arg = argv[++i];
            if (!parse_port(port_arg, out_port)) {
                std::cerr << "Invalid port: " << port_arg << std::endl;
                return false;
            }
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0]
                      << " [--ip <local-ipv4>] [--port <signaling-port>]" << std::endl
                      << "Defaults: machine local IPv4, signaling port 8080" << std::endl;
            return false;
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << std::endl;
            return false;
        }
    }

    return true;
}

static void display_tester_command() {
	printf("start - s\n");
	printf("stop - e\n");
}

std::unique_ptr<RTCVoiceServerCtrl> voice_server = nullptr;
std::unique_ptr<RTCSignalingSessionCtrl> signaling_session = nullptr;

bool is_closed = false;
bool is_opened = false;
bool is_error = false;

class SessionStatusObserver : public RTCSignalingSessionStatusObserver {
public:
    SessionStatusObserver() = default;
    virtual ~SessionStatusObserver() = default;

public:
	uint32_t OnSignalingSessionOpened() override {
        std::cout <<" OnSignalingSessionOpened " << std::endl;
        is_opened = true;
        voice_server = RTCVoiceServerCtrl::Create(signaling_session->GetInterface()); 
        voice_server->Start();
        return 0;
    }

	uint32_t OnSignalingSessionClosed() override {
        std::cout <<" OnSignalingSessionClosed " << std::endl;
        is_closed = true;
        return 0;
    }

	uint32_t OnSignalingSessionError() override {
        is_error = true;
        return 0;
    }   
};

void signal_handler(int signo)
{
	if (signo == SIGINT) {
        if(voice_server != nullptr) {
            voice_server->Stop();
        }
        
        if(signaling_session != nullptr) {
            signaling_session->Close();
			while (!is_closed && !is_error) {
				usleep(1000 * 10);
			}            
        }

		exit(0);
	}
}

int main(int argc, char* argv[]) {
    printf("maing+++\n");
	std::signal(SIGINT, signal_handler);

    SessionStatusObserver* observer = new SessionStatusObserver();
    signaling_session = RTCSignalingSessionCtrl::Create(observer);

    std::string ip_addr;
    int port = 8080;
    if (!parse_command_line(argc, argv, ip_addr, port)) {
        return 1;
    }

    std::cout << "signaling server info " << ip_addr
              << " port " << port << std::endl;

	display_tester_command();

	char cmd;
	bool is_end = false;
	bool is_start = false;
	
	while (1) {
		if (is_start) {
			cmd = getchar();
		}
		else {
			cmd = 's';
		}

		switch (cmd) {
		case 's':
			{
				if (is_start == false) {
                    std::string tmp = "/node";
					std::cout <<" connect signaling session " << std::endl;
					signaling_session->Open(ip_addr, port, tmp);
					is_start = true;
				}
				break;
			}
		case 'e':
			{
				if (is_start) {
					std::cout << "disconnect signaling session" << std::endl;
                    if(voice_server != nullptr) {
                        voice_server->Stop();
                    }                    
					signaling_session->Close();
					is_start = false;
				}
                is_end = true;
				break;
			}
		}

		if (is_end) {
			while (!is_closed) {
				usleep(1000 * 10);
			}
			break;
		}
	}

	return 0;
}