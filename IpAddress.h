#ifndef IPADDRESS_H
#define IPADDRESS_H

//cpp headers
#include <memory>
//c headers

//own headers

//forward declarations
struct sockaddr_storage;

namespace snl{
    
    //forward declarations
    class TcpPort;
    
    class IpAddress{
    public:
    
        friend class SocketAddress; //needed to build a sockaddr object from an ip address
        enum class IpVersion; //forward declaration of the enum that will represent the enum
        
        friend IpAddress makeIpv4Address(const std::string& ipv4String); //directly converts the address (faster than hostname)
        friend IpAddress makeIpv6Address(const std::string& ipv6String, u_int32_t flowInfo, u_int32_t scopeId); //directly converts the address (faster than hostname)
        friend IpAddress makeIpAddress(const sockaddr_storage& storage); // extracts the ip address out of the sockaddr storage
        friend void swap(IpAddress& lhs, IpAddress& rhs);
        IpAddress();
        
        IpAddress(const std::string& hostname);
        IpAddress(const char* hostname);
        
        IpAddress(const IpAddress& rhs);
        IpAddress(IpAddress&& rhs) noexcept;
        
        IpAddress& operator=(const IpAddress& rhs);
        IpAddress& operator=(IpAddress&& rhs) noexcept;
        
        ~IpAddress();
        
        std::string getIpString() const;
        
        bool isIpv4() const;
        
        bool isIpv6() const;
        
    private:
            
        //declare the implementation class
        class IpImpl; //base class
        class Ipv6Impl; // specialization for ipv6
        class Ipv4Impl; // specialization for ipv4
    
        //creates a sockaddr storage object from the curren ip address
        //can be used by socketAddress to create a new socket
        sockaddr_storage makeSockaddrStorage(TcpPort tcpPort);
        
        //factory function for an IpImpl object provided the hostname
        static std::unique_ptr<IpImpl> makeIpImpl(const std::string& hostname);
        
        //factory function for an IpImpl object provided the sockaddr storage
        static std::unique_ptr<IpImpl> makeIpImpl(const sockaddr_storage& storage);
        
        //private constructor to create an ip address from an unique ptr to impl
        IpAddress(std::unique_ptr<IpImpl> ipPtr);
        
        //declare the unique pointer
        std::unique_ptr<IpImpl> ipImplPtr;

    };
    
    /**
     * @brief creates an ip address based on an ipv4 string representation
     * @param ipv4String the ip string representing the ipv4 address
     * @return an ip address configured for the supplied ipv4 address
     */
    IpAddress makeIpv4Address(const std::string& ipv4String);
    
    /**
     * @brief creates an ip address based on the ipv6 string repesentation
     * @param ipv6String the ip string representing the ipv6 address
     * @return an ip address configured for the supplied ipv6 address
     * note: the caller may specify the flow labels that are used for creating an ipv6 header
     *       both 0 is a safe value (first lets the packages being unmarked for sorting and the scopeId to 0 follows google's and facebook's tracks)
     */
    IpAddress makeIpv6Address(const std::string& ipv6String, u_int32_t flowInfo = 0, u_int32_t scopeId = 0);
    
    /**
     * @brief creates an ip addres based on the provided sockaddrs storage
     * @param storage the storage object used to create the ip address
     * @return the created ip address (same config as the sockaddr storage)
     */
    IpAddress makeIpAddress(const sockaddr_storage& storage);
    
    /**
     * @brief swaps the contents of the lhs and the rhs
     * @param lhs the left hand side variable to swap
     * @param rhs the right hand side variable to swap
     */
    void swap(IpAddress& lhs, IpAddress& rhs);
    
}


#endif // IPADDRESS_H
