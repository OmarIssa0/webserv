#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string>

std::string hex_encode(const unsigned char* data) {
    const char* hex = "0123456789abcdef";
    std::string out;
    for (int i = 0; i < len; i++) {
        out.push_back(hex[data[i] >> 4]);
        out.push_back(hex[data[i] & 0x0F]);
    }
    return out;
}

std::string generate_session_id() {
    unsigned char buffer[32];

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
        return "";

    int total = 0;
    while (total < 32) {
        int r = read(fd, buffer + total, 32 - total);
        if (r <= 0)
            break;
        total += r;
    }
    close(fd);

    if (total != 32)
        return "";

    return hex_encode(buffer, 32);
}

int main() {
    std::cout << generate_session_id() << std::endl;
    return 0;
}
