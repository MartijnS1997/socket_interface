#ifndef STREAMSOCKET_H
#define STREAMSOCKET_H
//c headers
//cpp headers
#include <memory>
//own headeres
namespace snl{
        
    //forward declarations
    class StreamSocketFsm;
    class IpAddress;
    class TcpPort;
    class SocketAddress;
    class FdGuard;
    
    class StreamSocket
    {
    public:
        
        friend class ServerSocket;
        
        StreamSocket();
        StreamSocket(StreamSocket&& rhs);
        StreamSocket& operator=(StreamSocket&& rhs);
        ~StreamSocket();
        
        StreamSocket(const StreamSocket& rhs) = delete;
        StreamSocket& operator=(const StreamSocket& rhs) = delete;
        
        void connect(const SocketAddress& sockAddr); // pass by value, the result is always copied
        void connect(const IpAddress& address, const TcpPort& tcpPort); //pass by value, result is always copied
        
        std::size_t send(const void* buffer, std::size_t bufferSize, int flags = 0); //send primitive will return the number of bytes written
        std::size_t receive(void* buffer, std::size_t bufferSize, int flags = 0); //receive primitive (will ensure data is received)
        
        void closeUpstream(); //closes the upstream
        void closeDownstream(); //closes the downstream
        void close(); //closes the socket
        
        void reset(); //resets the socket
        void resetConnection(); //resets the connection (keeping the current config)
        
        void setNonBlockIO(bool nonBlockVal); //sets the socket to blocking/nonblocking
        bool isNonBlock() const;
        
        SocketAddress getSocketAddress()const;
        IpAddress getIpAddress()const;
        TcpPort getTcpPort()const;
        
        bool isConnected() const ;
        bool upstreamClosed() const;
        bool downStreamClosed() const;
        bool isClosed() const;
        
    private:
    
        StreamSocket(FdGuard&& guard, const SocketAddress& sockAddr); //constructor used by the server socket
        std::unique_ptr<StreamSocketFsm> fsmImpl;
    };
    
    
    
    
    /**
     * @brief Call that sends a line to the host
     * @param strSock the stream socket to use
     * @param line the line to send
     * @param eol the end of line delimiter
     * note: will always block until the message is completely sent (independent of the socket behavior)
     */
    void sendline(StreamSocket& strSock, const std::string& line, const std::string& eol = "\r\n");
    
    /**
     * @brief Call that reads a single line from the connected host
     * @param strSock the stream socket to used to receive the data with
     * @param lineBuff the buffer used to store the recieved line in
     * @param eol the end of line delimiter
     * note: will always block untill a line is completely read (independent of the socket non blocking behavior)
     */
    std::size_t readline(StreamSocket& strSock, std::string& lineBuff, const std::string& eol = "\r\n");
    
    /**
     * @brief Call that sents the complete buffer to the connected host
     * @param strSock the socket used to get the data from
     * @param buffer the buffer containing the data to send
     * @param bufferSize the size of the buffer in bytes(indicates how much must be sent)
     * note: call will always block untill the entire message is sent
     */
    void sendBuff(StreamSocket& strSock, const void* buffer, std::size_t bufferSize);
    
    /**
     * @brief Call receives data and puts it inside the buffer
     * @param strSock the stream socket that is used to receive the data
     * @param buffer pointer to the start of the buffer, the new data is stored starting at this pointer
     *        (subsequent calls should increment this counter)
     * @param bufferSize the size of the buffer in bytes
     * note: call will never block, will immediately return with the data that could be read
     */
    std::size_t receiveBuff(StreamSocket& strSock, void* buffer, std::size_t bufferSize);
}


#endif // STREAMSOCKET_H
