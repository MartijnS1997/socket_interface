#include "SocketAddress.h"

//c headers
#include <sys/select.h>
#include <netinet/in.h>
//cpp headers
#include <cassert>
//own headers
#include "IpAddress.h"
#include "SnlException.h"
#include "TcpPort.h"

namespace snl{
    
    constexpr std::size_t ipv4Addrlen = sizeof(sockaddr_in);
    constexpr std::size_t ipv6Addrlen = sizeof(sockaddr_in6);
    
    struct SocketAddress::SockAddrImpl{
        //use of pass by value: cheap to move ip addresses around and they MUST be copied in the first place
        SockAddrImpl(IpAddress ipAddress, TcpPort port) noexcept : ipAddr(std::move(ipAddress)), tcpPort(port), addrlen(ipAddr.isIpv4() ? ipv4Addrlen : ipv6Addrlen)  { }
        
        IpAddress ipAddr;
        TcpPort tcpPort;
        std::size_t addrlen; //the size (in bytes) of the sockaddr that is built from the socket address
    };
    
    SocketAddress::SocketAddress() : SocketAddress(std::move(IpAddress{}), 0) {} //create empty ip with no valid port
    
    SocketAddress::SocketAddress(IpAddress ipAddress, TcpPort tcpPort) : implPtr(std::make_unique<SockAddrImpl>(std::move(ipAddress), std::move(tcpPort))) { }
    
    //SocketAddress::SocketAddress(const std::string& hostname, TcpPort tcpPort) : implPtr(std::make_unique<SockAddrImpl>(IpAddress(hostname), tcpPort)) { } // a new ip addr must be created
    //note: we need to copy all the fields (note that the ipAddress must be copied, so pass by value is justified again)
    SocketAddress::SocketAddress(const SocketAddress& rhs) : implPtr(std::make_unique<SockAddrImpl>(rhs.implPtr->ipAddr, rhs.implPtr->tcpPort)){ }
    
    SocketAddress::SocketAddress(SocketAddress&& rhs) : implPtr(std::move(rhs.implPtr)) { }
    
    SocketAddress& SocketAddress::operator =(const SocketAddress& rhs){
        SocketAddress temp = rhs;
        using std::swap; //make available all the possible swaps
        swap(*this, temp);
        
        return *this;
    }
    
    SocketAddress& SocketAddress::operator =(SocketAddress&& rhs){
        assert(this != &rhs);
        this->implPtr = std::move(rhs.implPtr);
        
        return *this;
    }
    
    SocketAddress::~SocketAddress() = default;
    
    /*
     * getters and setters
     */ 
    IpAddress SocketAddress::getIpAddress() const noexcept{
        return implPtr->ipAddr;
    }
    
    TcpPort SocketAddress::getTcpPort() const noexcept{
        return implPtr->tcpPort;
    }
    
    std::size_t SocketAddress::getAddrlen() const noexcept{
        return implPtr->addrlen;
    }
    
    sockaddr_storage SocketAddress::getSockaddrStorage() const{
        return implPtr->ipAddr.makeSockaddrStorage(implPtr->tcpPort);
    }
        
   std::size_t SocketAddress::getAddressFamily() const{
        return implPtr->ipAddr.isIpv4() ? AF_INET : AF_INET6;
    }
    
    /*
     * friend functions
     */ 
    
    void swap(SocketAddress& lhs, SocketAddress& rhs){
        using std::swap;
        (lhs.implPtr).swap(rhs.implPtr); //swap the unique ptrs
    }
    
    SocketAddress makeIpv4SockAddr(const std::string& ipv4String, TcpPort tcpPort){
        //create an ipv4 address based on the string, and add the tcp port
        return SocketAddress(makeIpv4Address(ipv4String), tcpPort);
    }
    
    SocketAddress makeIpv6SockAddr(const std::string& ipv6String, TcpPort tcpPort, u_int32_t flowInfo, u_int32_t scopeId){
        return SocketAddress(makeIpv6Address(ipv6String, flowInfo, scopeId), tcpPort);
    }
    
    SocketAddress makeSockAddr(const sockaddr_storage& storage){
        return SocketAddress(makeIpAddress(storage), makeTcpPort(storage));
    }
    
}
