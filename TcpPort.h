#ifndef TCPPORT_H
#define TCPPORT_H

#include <string>
#include <cstdint>

//forward declaration
struct sockaddr_storage;
namespace snl{
    
    using TcpPortNbType = std::uint16_t;
    
    class TcpPort
    {
    public:
        friend TcpPort makeTcpPort(const sockaddr_storage& storage);
    
        TcpPort() noexcept;
        TcpPort(TcpPortNbType port) noexcept;
//        TcpPort(const std::string& portString);
//        TcpPort(const char* portString);
        
        TcpPort(const TcpPort& rhs) noexcept = default;
        TcpPort(TcpPort&& rhs) noexcept = default;
        TcpPort& operator= (const TcpPort& rhs) noexcept = default;
        TcpPort& operator= (TcpPort&& rhs) noexcept = default;
        
        ~TcpPort();
        
        TcpPortNbType toNetworkByteOrder();
        TcpPortNbType getPortNumber();
        
        constexpr TcpPortNbType getMaxPortNumber(){
            return MAX_PORTNB;
        }
        
    private:
        //function to convert the port string
        static std::uint16_t convertPortString(const std::string& portString);
        
        //member that stores the port number
        TcpPortNbType portNb;
        
        //
        static constexpr std::uint16_t MAX_PORTNB= ~static_cast<std::uint16_t>(0); //set all the bits to 1;
    };
    
    /**
     * @brief create a tcp port based on the  sockaddr storage
     * @param storage the storage object
     * @return the tcp port stored in the sockaddr storage
     */
    TcpPort makeTcpPort(const sockaddr_storage& storage);

}

#endif // TCPPORT_H
