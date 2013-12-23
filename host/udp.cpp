#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <string>
using namespace std;

int main(int, char *[])
{
    int s;
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
        cerr << "Cannot create socket" << endl;
        return 1;
    }

    sockaddr_in addr, clientAddr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);

    if (bind(s, (const struct sockaddr*)(&addr), sizeof(addr)) == -1) {
        cerr << "Cannot bound to port" << endl;
        return 1;
    }
    
    socklen_t len = sizeof(addr);
    if (getsockname(s, (struct sockaddr*)(&addr), &len) == -1) {
        cerr << "Get sockname failed" << endl;
        return 1;
    }

    cout << "Addr: " << ntohl(addr.sin_addr.s_addr) << endl
         << "Port: " << ntohs(addr.sin_port) << endl;

    ssize_t size = 0;
    char buffer[256] = {0};
    len = sizeof(clientAddr);

    while(true)
    {
        string command;
        cout << "> ";
        cin >> command;

        if (command.compare("exit") == 0) break;
        if (command.compare("read") == 0) {
            size = recvfrom(s, buffer, 256, 0, (struct sockaddr*) &clientAddr,
                            &len);
            if (size == -1)
                cout << "Error(" << errno << "): " << strerror(errno) << endl;
            else {
                cout << "Got " << size << " B from "
                     << inet_ntoa(clientAddr.sin_addr) << ":"
                     << ntohs(clientAddr.sin_port) << endl;

                buffer[size] = '\0';
                cout << "Msg: " << buffer << endl;
            }
        }
        if (command.compare("send") == 0) {
            string ip;
            string msg;
            unsigned int port = 0;
            cout << "ip> ";
            cin >> ip;
            cout << "port> ";
            cin >> port;
            cout << "msg>";
            cin >> msg;

            clientAddr.sin_family = AF_INET; 
            clientAddr.sin_addr.s_addr = inet_addr(ip.c_str());
            clientAddr.sin_port = htons(port);
            size = sendto(s, msg.c_str(), msg.length() , 0, 
                          (const struct sockaddr*)&clientAddr, len);

            if (size == -1)
                cout << "Error(" << errno << "): " << strerror(errno) << endl;
            else
                cout << "Sent " << size << " B" << endl;
        }
    }

    close(s);

    return 0;
}
