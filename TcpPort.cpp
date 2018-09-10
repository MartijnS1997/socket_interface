#include "TcpPort.h"

//c headers
#include <arpa/inet.h>
//cpp headers
#include <stdexcept>

//own headers
#include "SnlException.h"
namespace snl{
    
    TcpPort::TcpPort() noexcept : portNb(0) { }
    TcpPort::TcpPort(TcpPortNbType portNumber) noexcept : portNb(portNumber) {}

    
//    TcpPort::TcpPort(const char* portString) : TcpPort(std::string(portString)) { }
//    
//    TcpPort::TcpPort(const std::string& portString) : portNb(convertPortString(portString)) { }
    
    
//    TcpPortNbType TcpPort::convertPortString(const std::string& portString){
//        //convert the string to unsigned long
//        unsigned long converted;
//        bool conversionError = false;
//        
//        try{
//            converted = stoul(portString);
//        }catch(std::invalid_argument& e){
//            conversionError = true;
//        }
//        
//        //then check if it can fit, if not faulty port number
//        if(conversionError || converted > TcpPort::MAX_PORTNB){
//            throw SnlException("TcpPort exception: invalid port number");
//        }
//        
//        return converted;
//    }
    
    TcpPort::~TcpPort() = default;
    
    /*
     * member functions
     */ 
     
    TcpPortNbType TcpPort::getPortNumber(){
        return this->portNb;
    } 
    
    TcpPortNbType TcpPort::toNetworkByteOrder(){
        return htons(this->portNb);
    }
    
    /*
     * friend functions
     */ 
    
    TcpPort makeTcpPort(const sockaddr_storage& storage){
        switch(storage.ss_family){
            case AF_INET:{
                const sockaddr_in* ipv4Sock = reinterpret_cast<const sockaddr_in*>(&storage);
                return TcpPort(ntohs(ipv4Sock->sin_port)); }
            case AF_INET6:{
                const sockaddr_in6* ipv6Sock = reinterpret_cast<const sockaddr_in6*>(&storage);
                return TcpPort (ntohs(ipv6Sock->sin6_port)); }
            default:
                throw SnlException("TcpPort error: error extracting tcp port from sockaddr storage: unkown address family");
        }
    }
}
