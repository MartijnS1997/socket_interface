#ifndef SOCKETADDRESS_H
#define SOCKETADDRESS_H

//c headers

//cpp headers
#include <string>
#include <memory>
//own headers

//forward declarations
struct sockaddr_storage;

namespace snl{
    //some forward declarations
    class IpAddress;
    
    //type alias
    class TcpPort;
    
    class SocketAddress
    {
    public:
        
        friend SocketAddress makeIpv4SockAddr(const std::string& ipv4String, TcpPort port);
        friend SocketAddress makeIpv6SockAddr(const std::string& ipv6String, TcpPort port, u_int32_t flowInfo, u_int32_t scopeId);
        friend SocketAddress makeSockAddr(const sockaddr_storage& storage);
        friend void swap(SocketAddress& lhs, SocketAddress& rhs);
        
        SocketAddress(); //creates a localhost address bound to port 0 (no valid port) --> note needed for server impl
        
        //note pass by value: ipAddress: always copied, cheap to move
        SocketAddress(IpAddress ipAddress, TcpPort tcpPort); // by creation of an ip address
       // SocketAddress(const std::string& hostname, TcpPort tcpPort); // by creation of an hostname (slower)
        
        SocketAddress(const SocketAddress& other);
        SocketAddress(SocketAddress&& other);
        
        SocketAddress& operator=(const SocketAddress& rhs);
        SocketAddress& operator=(SocketAddress&& rhs);
        
        ~SocketAddress();
        
        /**
         * @brief getter for the ip address of the socket addr
         * @return the ip address representing the 
         */
        IpAddress getIpAddress() const noexcept;
        
        /**
         * @brief getter for the tcp port of the socket addr
         * @return the tcp port
         */
        TcpPort getTcpPort() const noexcept;
        
        /**
         * @brief returns the size of the underlying address
         */
        std::size_t getAddrlen() const noexcept;
        
        /**
         * @brief getter for a sockaddr_storage object that represents the socket address
         * @return a sockaddr_storage with the ip address and the port filled in (in case of ipv6 the flow and scope are also filled in)
         */
        sockaddr_storage getSockaddrStorage() const;
        
        /**
         * getter for the address family corresponding to the stored ip address
         */ 
        std::size_t getAddressFamily() const;
        
        
    private:
    
        struct SockAddrImpl;
        std::unique_ptr<SockAddrImpl> implPtr;
    };
    
    /**
     * @brief creates an ipv4 socket address from a string representing an ipv4 address
     * @param ipv4String the string to convert into an ipv4 address
     * @param tcpPort the tcp port to create a socket address for
     * @return a socket address created form the ip string and the tcp port
     * @thows SnlException if the ipv4 string is malformed
     */
    SocketAddress makeIpv4SockAddr(const std::string& ipv4String, TcpPort tcpPort);
    
    /**
     * @brief creates an ipv6 socket address
     * @param ipv6String the string to convert
     * @param tcpPort the tcp port as end point of the socket
     * @param flowInfo the flow label for the ipv6 header (is default 0 for no special treatment)
     * @param scopeId the scope id for the ipv6 header (is default 0 for no special cases)
     * @return a socket address made from an ipv6 address with the specified config
     * @throws SnlException if the ipv6 string is malformed
     */
    SocketAddress makeIpv6SockAddr(const std::string& ipv6String, TcpPort tcpPort, u_int32_t flowInfo = 0, u_int32_t scopeId = 0);
    /**
     * @brief swaps the contents of the lhs and the rhs
     */
    void swap(SocketAddress& lhs, SocketAddress& rhs);
    
    /**
     * @brief makes a socket address from the provided sockaddr storage
     * @param storage the sockaddr storage to create the sockaddr from
     * @return a socket address corresponding to the provided sockaddr storage
     */
    SocketAddress makeSockAddr(const sockaddr_storage& storage);

}

#endif // SOCKETADDRESS_H
