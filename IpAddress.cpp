#include "IpAddress.h"

//cpp headers
#include <iostream>
#include <cassert>
//c headers
#include <arpa/inet.h> // for inet_ntop and inet_pton
#include <sys/types.h> // for all kinds of system related stuff (typedefs)
#include <sys/socket.h> // for sockaddr
#include <netdb.h> // for getaddrinfo
//own headers
#include "SnlException.h" //used for the exceptions thrown by ip address creation
#include "TcpPort.h"
namespace snl{
    
    enum class IpAddress::IpVersion {IPV6 = AF_INET6, IPV4 = AF_INET}; //definition of ip version
    
   /*
    * Implementation of the pimpl classes
    */ 
    
    //declation and definition of the ip impl
    class IpAddress::IpImpl{
    public:
        IpImpl(IpVersion version) : ipVersion(version) {}
        IpVersion getIpVersion() const{
            return ipVersion;
        }
        virtual std::string getIpString() const = 0;
        virtual sockaddr_storage getSockaddrStorage(TcpPort tcpPort) const = 0;
        virtual std::unique_ptr<IpImpl> copy() const = 0;
        virtual ~IpImpl() = default;
    private:    
        IpVersion ipVersion;
    };
    
    class IpAddress::Ipv6Impl : public IpAddress::IpImpl{
    public:
    
        Ipv6Impl() : Ipv6Impl(createLocalhost(), 0 , 0) { } //default constructor, creates localhost with no fancy stuff
        
        Ipv6Impl(addrinfo* info) noexcept : Ipv6Impl(reinterpret_cast<sockaddr_in6*>(info->ai_addr)) { }
        
        Ipv6Impl(const sockaddr_in6* ipv6Sockaddr) noexcept : 
           Ipv6Impl(ipv6Sockaddr->sin6_addr, ipv6Sockaddr->sin6_flowinfo, ipv6Sockaddr->sin6_scope_id) { }
            
        Ipv6Impl(const in6_addr& ipv6Address, u_int32_t flow, u_int32_t scope) noexcept : IpImpl(IpVersion::IPV6), //call base constructor (bind the first argument by reference to save a copy)
            //copy addr, flow label and scope id all is needed to create an ipv6 header (we want to be as general as possible, and these fields are ip -not tcp- specific)
            ipv6Addr(ipv6Address), flowInfo(flow), scopeId(scope) { }
            
        
        virtual ~Ipv6Impl() override= default; //just keep the old implementation
    
        virtual std::string getIpString() const override {
            char ipv6String[INET6_ADDRSTRLEN]; //allocate the space to hold the ip string
            inet_ntop(AF_INET6, &(this->ipv6Addr), ipv6String, INET6_ADDRSTRLEN);
            return std::string(ipv6String); //return the ipv6 string (trigger ROV optim)
        }    
        
        virtual sockaddr_storage getSockaddrStorage(TcpPort tcpPort) const override{
            sockaddr_storage storage{}; //create an empty stack object (initialize to null)
            storage.ss_family = AF_INET6; //fill in the address family
            sockaddr_in6* ipv6Ptr = reinterpret_cast<sockaddr_in6*>(&storage); //use semantic sledgehammer to fill in the fields
            //fill in the fields (a lot for ipv6)
            ipv6Ptr->sin6_port = tcpPort.toNetworkByteOrder(); //do not forget to do this... otherwise your call is GARBAGE
            ipv6Ptr->sin6_flowinfo = flowInfo;
            ipv6Ptr->sin6_addr = ipv6Addr;
            ipv6Ptr->sin6_scope_id = scopeId;
            
            return storage; //return the filled in sockaddr storage
        }
        
        virtual std::unique_ptr<IpImpl> copy() const override{
            //we will construct an identical object on the heap
            return std::make_unique<Ipv6Impl>(ipv6Addr, flowInfo, scopeId);
        }
        
    private:
        
        in6_addr createLocalhost(){
            in6_addr localhostAddr{};
            inet_pton(AF_INET6, "::1", &localhostAddr);
            return localhostAddr; //do not move RVO is triggered
        }
        
        in6_addr ipv6Addr; //struct holding the ipv6 address
        u_int32_t flowInfo; //used to hold the flow info of ipv6
        u_int32_t scopeId; //used to hold the scope id of ipv6
    };
    
    class IpAddress::Ipv4Impl : public IpAddress::IpImpl{
    public:
        Ipv4Impl() noexcept : Ipv4Impl(createLocalhost()) {  } //default is localhost
        
        //constructor for an ipv4 addr out of an addrinfo
        Ipv4Impl(addrinfo* info) noexcept : Ipv4Impl(reinterpret_cast<sockaddr_in*>(info->ai_addr)) { }
        //constructor for an ipv4 out of an sockaddr_in
        
        Ipv4Impl(const sockaddr_in* ipv4Sockaddr) noexcept:
            Ipv4Impl(ipv4Sockaddr->sin_addr) { } //copy the address (default copy constructor)
            
        //base constructor, used for constructing a new ipv4 address, all the other constructors lead to this one
        Ipv4Impl(const in_addr& ipv4Address) noexcept : 
            IpImpl(IpVersion::IPV4), //instantiate the base class
            ipv4Addr(ipv4Address) { }
        
        virtual ~Ipv4Impl() override = default;

        
        virtual std::string getIpString() const override {
            char ipv4String[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(this->ipv4Addr), ipv4String, INET_ADDRSTRLEN);
            return std::string(ipv4String);
        }
        
        virtual sockaddr_storage getSockaddrStorage(TcpPort tcpPort) const override{
            sockaddr_storage storage{}; //create empty sockaddr storage
            storage.ss_family = AF_INET;
            sockaddr_in* ipv4Ptr = reinterpret_cast<sockaddr_in*>(&storage); //use semantic sledgehammer to fill in the fields
            //fill in the fields
            ipv4Ptr->sin_port = tcpPort.toNetworkByteOrder();//convert to network byte order, otherwise GARBAGE results
            ipv4Ptr->sin_addr = ipv4Addr;
            //return the filled in sockaddr storage
            return storage;
        }
        
        virtual std::unique_ptr<IpImpl> copy() const override{
            //create an identical copy by invoking the constructor with the same parameters
            return std::make_unique<Ipv4Impl>(ipv4Addr);
        }
        
    private:
        static in_addr createLocalhost() noexcept {
            in_addr localhostAddr{};
            inet_pton(AF_INET, "127.0.0.1", &localhostAddr);
            return localhostAddr; //do not move, RVO is triggered
        }
    
        in_addr ipv4Addr; //struct holding the ipv4 address
    };
    
    IpAddress::IpAddress(const char* hostname) : IpAddress(std::string(hostname)) { }
    
    IpAddress::IpAddress(const std::string& hostname) : IpAddress(std::move(makeIpImpl(hostname))) { }; 
    
    std::unique_ptr<IpAddress::IpImpl> IpAddress::makeIpImpl(const std::string& hostname){
        //create the hints to find the ip address
        addrinfo hints{}; //empty init
        addrinfo* searchResult;//pointer for the search results
        
        hints.ai_family = AF_UNSPEC; //ip version agnostic
        hints.ai_socktype = SOCK_STREAM; //stream sockets
        
        int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &searchResult); //get addr info
        
        if(status != 0){ //check for failure
            throw SnlException(std::string("IpAddress error: ") + std::string(gai_strerror(status)));
        }
        
        std::unique_ptr<addrinfo, void (*)(addrinfo*)> info(searchResult, freeaddrinfo); //custom deleter to make shure the addrinfo gets freed

        
        //then create an ip impl obj depending on the address family
        switch(searchResult->ai_family){
            case AF_INET:
                return std::unique_ptr<IpImpl>(new Ipv4Impl(info.get())); //safe to use handle, constructor is noexcept
            case AF_INET6:
                return std::unique_ptr<IpImpl>(new Ipv6Impl(info.get())); //safe to use handle, constructor is noexcept
            default:
                throw SnlException("IpAddress error: unknown address family");
        }
        
        //the resources will be freed automatically after finishing the construction
    }
    
     std::unique_ptr<IpAddress::IpImpl> IpAddress::makeIpImpl(const sockaddr_storage& storage){
        switch(storage.ss_family){
            case AF_INET:
                return std::make_unique<Ipv4Impl>(reinterpret_cast<const sockaddr_in*>(&storage)); //call the constructor with sockaddr in
            case AF_INET6:
                return std::make_unique<Ipv6Impl>(reinterpret_cast<const sockaddr_in6*>(&storage)); //call the concstructor with sockaddr in6
            default:
                throw SnlException("IpAddress error: unknown address family");
        }
        
    }
    
    
    IpAddress::IpAddress() : IpAddress(std::move(std::make_unique<Ipv4Impl>())) { } //default constructor uses the default constructor of ipv4 addresses
    
    IpAddress::IpAddress(std::unique_ptr<IpImpl> ipPtr) : ipImplPtr(std::move(ipPtr)) { }
    
    IpAddress::IpAddress(const IpAddress& rhs) : ipImplPtr(rhs.ipImplPtr->copy()) { }
    
    IpAddress::IpAddress(IpAddress&& rhs) noexcept : ipImplPtr(std::move(rhs.ipImplPtr)){ }
    
    IpAddress& IpAddress::operator =(const IpAddress& rhs){
        IpAddress temp = rhs; //make a copy
        //swap the contents of the copy with this -> guard against self reference (case this & other are the same)
        using std::swap;
        swap(*this, temp); 
        return *this;
    }
    
    IpAddress& IpAddress::operator =(IpAddress&& rhs) noexcept {
        assert(this != &rhs);
        this->ipImplPtr = std::move(rhs.ipImplPtr);
        return *this;
    }
    
    IpAddress::~IpAddress(){ }

   /*
    * general functionality
    */ 
    
    std::string IpAddress::getIpString() const {
        return ipImplPtr->getIpString();
    }
    
    bool IpAddress::isIpv4() const{
        return ipImplPtr->getIpVersion() == IpVersion::IPV4;
    }
    
    bool IpAddress::isIpv6() const{
        return ipImplPtr->getIpVersion() == IpVersion::IPV6;
    }
    
    sockaddr_storage IpAddress::makeSockaddrStorage(TcpPort tcpPort){
        return ipImplPtr->getSockaddrStorage(tcpPort);
    }
    
   
    
   /*
    * implementation of friend functions
    */

    IpAddress makeIpv4Address(const std::string& ipv4String){
        in_addr ipv4Addr{}; //target
        int status = inet_pton(AF_INET, ipv4String.c_str(), &ipv4Addr); //convert to addr
        if(status == 0){
            throw SnlException("IpAddress error: malformed ipv4 address supplied to ipv4 string");
        }
        
        //create an unique ptr to ipv4impl and store it inside a base impl pointer
        std::unique_ptr<IpAddress::IpImpl> implPtr = std::make_unique<IpAddress::Ipv4Impl>(std::move(ipv4Addr));
        return IpAddress(std::move(implPtr)); //return the ip addr created by the private constructor
    }
    
    IpAddress makeIpv6Address(const std::string& ipv6String, u_int32_t flowInfo, u_int32_t scopeId){
        in6_addr ipv6Addr{}; //target
        int status = inet_pton(AF_INET6, ipv6String.c_str(), &ipv6Addr);
        if(status == 0){
            throw SnlException("IpAddress error: malformed ipv6 address supplied to ipv6 string");
        }
        
        //create an unique ptr to ipv6 impl and store it inside a base impl pointer
        std::unique_ptr<IpAddress::IpImpl> implPtr = std::make_unique<IpAddress::Ipv6Impl>(ipv6Addr, flowInfo, scopeId);
        return IpAddress(std::move(implPtr)); //return the ip addr created by the private constructor
    }
    
    IpAddress makeIpAddress(const sockaddr_storage& storage){
        return IpAddress(std::move(IpAddress::makeIpImpl(storage)));
    }
    
    void swap(IpAddress& lhs, IpAddress& rhs){
        //we only need to swap in implementation pointers
        using std::swap;
        //swap the two implementation pointers
        (lhs.ipImplPtr).swap(rhs.ipImplPtr);
    }
    
}

