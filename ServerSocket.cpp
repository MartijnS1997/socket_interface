#include "ServerSocket.h"
//c headers
#include <sys/socket.h>
//cpp headers
#include <iostream>
#include <cassert>
//own headers
#include "SocketAddress.h"
#include "SnlException.h"
#include "ServerSocketFsm.h"
#include "StreamSocket.h"

namespace snl{
    
    ServerSocket::ServerSocket() : fsmPtr(std::make_unique<ServerSocketFsm>()) { }
    
    ServerSocket::ServerSocket(const TcpPort& tcpPort, int backlog) : ServerSocket(makeIpv4Address("127.0.0.1"), tcpPort, backlog) {}
    
    ServerSocket::ServerSocket(const SocketAddress& socketAddress, int backlog) : fsmPtr(std::make_unique<ServerSocketFsm>(socketAddress , backlog)) {}
    
    ServerSocket::ServerSocket(const IpAddress& ipAddress, const TcpPort& tcpPort, int backlog): ServerSocket(SocketAddress(ipAddress,tcpPort), backlog) {}
        
    ServerSocket::ServerSocket(ServerSocket&& rhs) : fsmPtr(std::move(rhs.fsmPtr)) {};
    ServerSocket& ServerSocket::operator=(ServerSocket&& rhs){
        assert(this != &rhs);
        this->fsmPtr = std::move(rhs.fsmPtr);
        return *this;
    }
    ServerSocket::~ServerSocket() = default;
    
    void ServerSocket::bind(SocketAddress sockAddr){
        //bind the socket
        fsmPtr->toNextState(serverBind, std::move(sockAddr));
    }
    
    void ServerSocket::listen(int backlog){
        //listen
        fsmPtr->toNextState(serverListen, backlog);
    }
//    
//    void ServerSocket::listen(TcpPort tcpPort, int backlog){
//        //default listen
//        fsmPtr->toNextState(defaultListenAction, tcpPort, backlog);
//    }
//    
    StreamSocket ServerSocket::accept(){
        sockaddr_storage storage{};
        FdGuard guard{};
        fsmPtr->toNextState(serverAccept, guard, storage);
        return StreamSocket(std::move(guard), makeSockAddr(storage));
    }
    
    void ServerSocket::close(){
        fsmPtr->toNextState(serverClose);
    }
    
    void ServerSocket::reset(){
        fsmPtr->toNextState(serverReset);
    }
    
    SocketAddress ServerSocket::getSockAddr(){
        return fsmPtr->getSockAddr();
    }
    
    IpAddress ServerSocket::getIpAddress(){
        return fsmPtr->getIpAddress();
    }
    
    TcpPort ServerSocket::getTcpAddress(){
        return fsmPtr->getTcpPort();
    }
    
    bool ServerSocket::isBound(){
        return fsmPtr->isBound();
    }
    
    bool ServerSocket::isListening(){
        return fsmPtr->isListening();
    }
    
    bool ServerSocket::isAccepting(){
        return fsmPtr->isAccepting();
    }
    
    bool ServerSocket::isClosed(){
        return fsmPtr->isClosed();
    }
    
}

//
////action structs
//    struct Bind {Bind() noexcept = default; };
//    struct Listen {Listen() noexcept = default; };
//    struct DefaultListen {DefaultListen() noexcept = default; };
//    struct Accept {Accept() noexcept = default; };
//    struct Close {Close() noexcept = default; };
//    struct Reset {Reset() noexcept = default; };
//    
//    //action constexpr objects
//    static constexpr Bind bindAct{};
//    static constexpr Listen listenAct{};
//    static constexpr DefaultListen defaultListenAct{};
//    static constexpr Accept acceptAct{};
//    static constexpr Close closeAct{};
//    static constexpr Reset resetAct{};
//    
//    struct ServerSocket::ServSockFsm{
//        //the states of the server socket fsm
//        enum class ServSockState: std::uint8_t {INIT=0, BOUND=1, LISTENING = 2, ACCEPTING = 3, CLOSED=5}; 
//        
//        
//        template<typename Action, typename... Args>
//        void toNextState(const Action& action, Args&&...args){
//            //use tag dispatch, with the dispatch tags the actions
//            toNextStateImpl(this->state, action, std::forward<Args>(args)...);
//        }
//        
//        //test implementation to see if the fsm works
//        
//        void toNextStateImpl(ServSockState currentState, const Bind& b, SocketAddress socketAddr){
//            if(currentState != ServSockState::INIT){
//                throw SnlException("ServerSocket error: trying to bind an already initialized socket");
//            }
//            
//            std::cout << "binding socket" << std::endl;
//            
//            state = ServSockState::BOUND;
//        }
//        
//        void toNextStateImpl(ServSockState currentState, const Listen& l, int backlog){
//            if(currentState != ServSockState::BOUND){
//                throw SnlException("ServerSocket error: conditions for listening not met");
//            }
//            
//            std::cout << "starting listening with already bound socket" <<std::endl;
//            
//            state = ServSockState::LISTENING;
//        }
//        
//        void toNextStateImpl(ServSockState currentState, const DefaultListen& dl, unsigned int tcpPort, int backlog){
//            if(currentState != ServSockState::INIT){
//                throw SnlException("ServerSocket error: invoking default listen on non init socket");
//            }
//            
//            std::cout << "starting default listening" << std::endl;
//            

//            state = ServSockState::LISTENING;
//        }
//        
//        void toNextStateImpl(ServSockState currentState, const Accept& a){
//            if(currentState != ServSockState::LISTENING){
//                throw SnlException("ServerSocket error: accepting new connections on socket that is not listening");
//            }
//            
//            std::cout << "started accepting new connections" << std::endl;
//            
//            state = ServSockState::ACCEPTING;
//        }
//        
//        
//        void toNextStateImpl(ServSockState currentState, const Close& c){
//            if(currentState != ServSockState::ACCEPTING){
//                throw SnlException("ServerSocket error: closing socket that does not meet the condtions for closing");
//            }
//            
//            std::cout << "closing the socket" << std::endl;
//            
//            state = ServSockState::CLOSED;
//        }
//        
//        //the current state of the fsm
//        ServSockState state = ServSockState::INIT;
//        
//        //fields that are filled in during the traversal of the fsm
//        SocketAddress sockAddr; //the sockaddr also
//        int backlog; //the backlog is unspecified
//        bool nonBlocking = false; //flag that can be set during any point of the execution 
//    };
//    
////    struct ServerSocket::ServSockImpl{
////        
////        //fields that are filled in during the traversal of the fsm
////        SocketAddress sockAddr; //the sockaddr also
////        int backlog; //the backlog is unspecified
////        bool nonBlocking = false; //flag that can be set during any point of the execution 
////        
////        //the finite automata describing the server
////        ServerSocket::ServSockFsm servFsm;
////    }
//    
//    ServerSocket::ServerSocket() : implPtr(std::make_unique<ServSockFsm>()){
//        
//    }
//
//    ServerSocket::~ServerSocket(){
//        
//    }
//    
//    void ServerSocket::bind(SocketAddress socketAddress){
//        implPtr->toNextState(bindAct, std::move(socketAddress));
//    }
//    
//    void ServerSocket::listen(int backlog){
//        implPtr->toNextState(listenAct, backlog);
//    }
//    
//    void ServerSocket::listen(unsigned short tcpPort, int backlog){
//        implPtr->toNextState(defaultListenAct, tcpPort, backlog);
//    }
//    
//    
//    void ServerSocket::accept(){
//        implPtr->toNextState(acceptAct);
//    }
//    
//    void ServerSocket::close(){
//        implPtr->toNextState(closeAct);
//    }