#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H
//c headers

//cpp headers
#include <memory>
//own headers

namespace snl{
    
    //snl forward declarations
    class IpAddress;
    class SocketAddress;
    class TcpPort;
    class ServerSocketFsm;
    class StreamSocket;
    class ServerSocket{
    public:
    
        ServerSocket();
        ServerSocket(const TcpPort& port, int backlog = 5);
        ServerSocket(const SocketAddress& socketAddress, int backlog = 5);
        ServerSocket(const IpAddress& ipAddress, const TcpPort& port, int backlog = 5);
        ServerSocket(ServerSocket&& other);
        ServerSocket& operator=(ServerSocket&& other);
        ~ServerSocket();

        
        
        ServerSocket(const ServerSocket& rhs) = delete;
        ServerSocket operator=(const ServerSocket& rhs) = delete;
        
        //fsm calls
        void bind(IpAddress address, TcpPort tcpPort); //pass by value, ip address is copied
        void bind(SocketAddress sockAddr); //pass by value sockaddr is copied to local sockaddr
        
        void listen(int backlog = defaultBacklog);
        
        StreamSocket accept();
        
        void close();
        void closeUpstream();
        void closeDownstream();
        
        void reset();
        
        //inspecting calls
        SocketAddress getSockAddr();
        IpAddress getIpAddress();
        TcpPort getTcpAddress();
        
        bool isBound();
        bool isListening();
        bool isAccepting();
        bool isClosed();
        
    private:
    
        std::unique_ptr<ServerSocketFsm> fsmPtr;
        
        static constexpr int defaultBacklog = 5;
    }; 
}


#endif // SERVERSOCKET_H
