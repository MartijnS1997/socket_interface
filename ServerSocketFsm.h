#ifndef SERVERSOCKETFSM_H
#define SERVERSOCKETFSM_H

#include "IpAddress.h"
#include "TcpPort.h"
#include "SocketAddress.h"
#include "FdGuard.h"

namespace snl{
    
   /*
    * action classes, used for the tag dispatch
    */
    struct ServerBind { ServerBind() noexcept = default; };
    struct ServerListen { ServerListen () noexcept = default; };
//    struct ServDefaultListen { ServDefaultListen() noexcept = default; };
    struct ServerAccept {ServerAccept() noexcept = default; };
    struct ServerClose {ServerClose() noexcept = default; };
    struct ServerReset {ServerReset() noexcept = default; };
    
    //instantiations
    constexpr ServerBind serverBind;
    constexpr ServerListen serverListen;
//    constexpr ServDefaultListen defaultListenAction;
    constexpr ServerAccept serverAccept;
    constexpr ServerClose serverClose;
    constexpr ServerReset serverReset;
    
    
    class ServerSocketFsm
    {
    public:
        //enum that contains the states of the fsm
        enum class ServerFsmState:uint8_t{INIT = 0, BOUND = 1, LISTENING = 2, ACCEPTING = 3, CLOSED = 4};
        
        ServerSocketFsm() noexcept; //default constructor == ok
        ServerSocketFsm(const SocketAddress& address, int backlog);
        ~ServerSocketFsm();
        
        template<typename Action, typename ...Args>
        void toNextState(const Action& action, Args&& ... args){
            //first do a static check
//            ServerFsmState state = fsmState;
//            TransitionCheck<state, Action>();
            //use tag dispatch for the next state
            toNextStateImpl(action, std::forward<Args>(args)...);
        }
        
        void setNonBlockIO(bool nonBlockVal); 
        bool getNonBlockIO();
        
        SocketAddress getSockAddr();
        IpAddress getIpAddress();
        TcpPort getTcpPort();
        
        bool isBound();
        bool isListening();
        bool isAccepting();
        bool isClosed();
            
    private:
        //todo try to move the correct order of execution to compile time by filling in the previous 
        //state during to next state (static asserts on the previous and current state) ==> need to fill in a template parameter with the previous type and the next action
        //toNextState Implementations
        void toNextStateImpl(const ServerBind& , const SocketAddress& socketAddress); // action is bind
        void toNextStateImpl(const ServerListen& , int listenBacklog); //action is listen
//        void toNextStateImpl(const ServDefaultListen& , TcpPort tcpPort, int listenBacklog); //action is default listen
        void toNextStateImpl(const ServerAccept&, FdGuard& clientFd, sockaddr_storage& clientSockaddr); //accept, put new guard and addr info in the references
        void toNextStateImpl(const ServerClose& ); //action is close
        void toNextStateImpl(const ServerReset& ); //action is reset
        
        //checks to do
        static void bindCheck(ServerFsmState current);
        static void listenCheck(ServerFsmState current);
//        static void defaultListenCheck(ServerFsmState current);
        static void acceptCheck(ServerFsmState current);
        static void closeCheck(ServerFsmState current);
        static void resetCheck(ServerFsmState current);
        
        
        //general functions to make life easier
        //creates a socket fd for the given sockaddr and binds it to the created fd
        static FdGuard makeSockFdAndBind(const SocketAddress& sockAddr);
        //used to change fd
        void setFdBlockingBehav(FdGuard& guard, bool nonBlockVal);
        
        //datamembers
        SocketAddress socketAddr;
        FdGuard servSockFd;
        int backlog;
        bool nonBlockingIo = defaultIOBehav;
        //the state of the fsm
        ServerFsmState fsmState;
        
        static constexpr bool defaultIOBehav = false;
    };
    
    bool operator<(ServerSocketFsm::ServerFsmState lhs, ServerSocketFsm::ServerFsmState rhs);
    bool operator>(ServerSocketFsm::ServerFsmState lhs, ServerSocketFsm::ServerFsmState rhs);
    bool operator<=(ServerSocketFsm::ServerFsmState lhs, ServerSocketFsm::ServerFsmState rhs);
    bool operator>=(ServerSocketFsm::ServerFsmState lhs, ServerSocketFsm::ServerFsmState rhs);


   
}


#endif // SERVERSOCKETFSM_H


