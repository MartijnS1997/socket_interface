#ifndef STREAMSOCKETFSM_H
#define STREAMSOCKETFSM_H

#include "SocketAddress.h"
#include "FdGuard.h"
namespace snl{
    
    struct StrSoConnect {StrSoConnect() = default;};
    struct StrSoSend { StrSoSend() = default; };
    struct StrSoReceive { StrSoReceive() = default; };
    struct StrSoUClose {StrSoUClose() = default;};
    struct StrSoDClose {StrSoDClose() = default;};
    struct StrSoClose {StrSoClose() = default; };
    struct StrSoReset {StrSoReset() = default; };
    struct StrSoReConnect {StrSoReConnect() = default; };
    
    constexpr StrSoConnect connectAct{};
    constexpr StrSoSend sendAct{};
    constexpr StrSoReceive receiveAct{};
    constexpr StrSoUClose upstrCloseAct{};
    constexpr StrSoDClose downstrCloseAct{};
    constexpr StrSoClose closeAct{};
    constexpr StrSoReset resetAct{};
    constexpr StrSoReConnect resetConnectAct{};
    
    
    
    class StreamSocketFsm
    {
    public:
    
        enum class StrSoFsmState:uint8_t {INIT = 0, CONNECTED = 1, UCLOSED = 2, DCLOSED = 3, CLOSED = 4};
        StreamSocketFsm();
        StreamSocketFsm(FdGuard&& fdGuard, SocketAddress address);
        
        ~StreamSocketFsm();
        template<typename Action, typename ...Args>
        void toNextState(Action& action, Args&&... args){
            toNextStateImpl(action, std::forward<Args>(args)...);
        }
        
        void setNonBlock(bool nonBlockVal);
        
        SocketAddress getSockAddress();
        bool isNonBlock();
        bool isConnected();
        bool upstreamClosed();
        bool downStreamClosed();
        bool isClosed();
        
        static constexpr int upstreamShutdown = 1;
        static constexpr int downstreamShutdown = 0;
        static constexpr bool defaultNonBlock = false;
        
    private:
    
        
        
        void toNextStateImpl(const StrSoConnect&, const SocketAddress& socketAddress);
        void toNextStateImpl(const StrSoSend&, const void* buffer, std::size_t bufferSize, std::size_t& bytesSent, int flags); //sets the buffer size to the size of the unread portion
        void toNextStateImpl(const StrSoReceive&, void* buffer, std::size_t bufferSize, std::size_t& bytesReceived, int flags);
        void toNextStateImpl(const StrSoUClose&);
        void toNextStateImpl(const StrSoDClose&);
        void toNextStateImpl(const StrSoClose&);
        void toNextStateImpl(const StrSoReset&);
        void toNextStateImpl(const StrSoReConnect&);
        
        static void connectCheck(StrSoFsmState current);
        static void sendCheck(StrSoFsmState current);
        static void receiveCheck(StrSoFsmState current);
        static void upstreamCloseCheck(StrSoFsmState current);
        static void downsreamCloseCheck(StrSoFsmState current);
        static void closeCheck(StrSoFsmState current);
        static void resetConnectCheck(StrSoFsmState current);
        
        //creates a socket fd and connects it to the specified address
        //common call for reset connection and connect, the non block val is to indicate if the socket is blocking or not
        //-->saved blocking behavior will also be set here
        static FdGuard createSockAndConnect(const SocketAddress& address, bool nonBlockVal);
        
        //setter for the blocking behavior of the blocking call
        //if nonBlockVal == true, the fd will be set to nonblock
        static void setFdBlockingBehav(FdGuard& guard, bool nonBlockVal);
        
        
        SocketAddress socketAddress;
        FdGuard strSoFd;
        bool nonBlock = defaultNonBlock;
        StrSoFsmState fsmState;
        
       
    };
    
}



#endif // STREAMSOCKETFSM_H
