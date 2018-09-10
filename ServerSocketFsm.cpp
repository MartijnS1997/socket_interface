#include "ServerSocketFsm.h"
//c headers
#include <sys/socket.h>
#include <fcntl.h>

//cpp headers
#include <iostream>
//own headers

//declare the used c functions to prevent name mangling in the forwarding constructor
extern "C" {
    int socket(int, int, int);
    int bind(int, const struct sockaddr*, socklen_t);
    int listen(int, int);
    int accept(int, struct sockaddr*, socklen_t* );
    int close(int);
    int fcntl(int, int , ... );
};

namespace snl{
    
    ServerSocketFsm::ServerSocketFsm() noexcept { }
        
    ServerSocketFsm::ServerSocketFsm(const SocketAddress& sockAddr, int backlog) : ServerSocketFsm(){
        //do all the state transitions in the constructor
        this->toNextState(serverBind, sockAddr); // bind the socket
        this->toNextState(serverListen, backlog); // start listening
        
    }

    ServerSocketFsm::~ServerSocketFsm(){ } //the sock fd will be automatically closed
    
    void ServerSocketFsm::toNextStateImpl(const ServerBind& , const SocketAddress& sockAddr){
//        std::cout << "start binding" << std::endl;
        //to bind the socket address with the bind function
        bindCheck(fsmState);
        //then create the file descriptor & bind the socket (will throw if something goes wrong)
        FdGuard guard = makeSockFdAndBind(sockAddr);
        //check if the blocking state is as desired
        setFdBlockingBehav(guard, getNonBlockIO());
        //save the guard
        this->servSockFd = std::move(guard);
        this->socketAddr = sockAddr;
        fsmState = ServerFsmState::BOUND;
    }
    
    void ServerSocketFsm::toNextStateImpl(const ServerListen& , int listenBacklog){
        //check if we can start the normal listening procedure (with a bound socket)
        listenCheck(fsmState);
        //then start listening, (the call will automatically check for failure)
//        std::cout << "start listening" << std::endl;
        int failure = -1;
        executeSyscall(::listen, failure, servSockFd.get(), listenBacklog);
        
        this->backlog = listenBacklog;
        
        fsmState = ServerFsmState::LISTENING;
    }
    
//    void ServerSocketFsm::toNextStateImpl(const ServDefaultListen& , TcpPort tcpPort, int listenBacklog){
//        std::cout << "start default listening" << std::endl;
//        //first create a default file descriptor, note that the default ip address is already the correct one
//        defaultListenCheck(fsmState);
//        //create the socket address for a default socket
//        SocketAddress defaultSock = SocketAddress(IpAddress{}, TcpPort{});
//        //create fd guard and bind it to the provided socket address
//        FdGuard guard = makeSockFdAndBind(defaultSock);
//        setFdBlockingBehav(guard, getNonBlockIO());
//
//        //then start listening
//        int failure = -1;
//        executeSyscall(::listen, failure, guard.get(), listenBacklog);
//        
//        //then save all the state
//        this->backlog = listenBacklog;
//        this->socketAddr = defaultSock;
//        this->servSockFd = std::move(guard);
//        fsmState = ServerFsmState::LISTENING;
//    }
//    
    //summarizing call to bind a socket to a sockaddr (can be called from both default listen and bind
    FdGuard ServerSocketFsm::makeSockFdAndBind(const SocketAddress& sockAddr){
        //create the socket file descriptor, will throw if something went wrong
        FdGuard guard = makeFdGuard(::socket, sockAddr.getAddressFamily(), SOCK_STREAM, 0);
        //then bind the file descriptor to the port descibed in the socket address
        int failure = -1;
        sockaddr_storage storageSpec = sockAddr.getSockaddrStorage();
        sockaddr* sockSpec = reinterpret_cast<sockaddr*>(&storageSpec);
        //will throw if something went wrong
        executeSyscall(::bind, failure, guard.get(), sockSpec, sockAddr.getAddrlen());
        //then return the guard after the bind
        return guard;
    }
    
    void ServerSocketFsm::toNextStateImpl(const ServerAccept& , FdGuard& clientFd, sockaddr_storage& clientSockaddr){
//        std::cout << "accepting new connections" << std::endl;
        //we are accepting a new connection, the sockaddr storage passed will receive the sockaddr storage of the new connection
        acceptCheck(fsmState);
        sockaddr* clientAddr = reinterpret_cast<sockaddr*>(&clientSockaddr);
        socklen_t clientAddrlen =  sizeof(sockaddr_storage);
        //then start accepting new connections (via syscall)
        int failure = -1;
        int acceptedFd = executeSyscall(::accept, failure, servSockFd.get(),  clientAddr, &clientAddrlen);
        //the call will be succesfull(otherwise an exception would have been thrown)
        clientFd.reset(acceptedFd);
        //the clientSockaddr will also have been set so ok
        fsmState = ServerFsmState::ACCEPTING;
    }
    
    void ServerSocketFsm::toNextStateImpl(const ServerClose&){
//        std::cout << "closing socket" << std::endl;
        closeCheck(fsmState);
        //just close the server
        servSockFd.close();
        //we're done
        fsmState = ServerFsmState::CLOSED;
    }
    
    void ServerSocketFsm::toNextStateImpl(const ServerReset&){
        
//        std::cout << "resetting socket" << std::endl;
        //close the socket (if there is no owned fd, nothing will happen)
        servSockFd.close();
        //reset the blocking behavior to default
        nonBlockingIo = defaultIOBehav;
        //no need to reset the sockaddr and the backlog (cannot be read, will throw error) so reset to init state
        fsmState = ServerFsmState::INIT;
    }
    
    
    bool ServerSocketFsm::getNonBlockIO(){
        return nonBlockingIo;
    }
    
    void ServerSocketFsm::setNonBlockIO(bool nonBlockVal){
        //first check if the current flag == new flag
        if(nonBlockVal == this->nonBlockingIo){
            return;
        }
        
        //if we do not yet own an fd, just set the flag
        if(!servSockFd.ownsFd()){
            nonBlockingIo = nonBlockVal;
            return;
        }
        
        //then if we have a fd, set the blocking behavior
        setFdBlockingBehav(servSockFd, nonBlockVal);
        
        //save the flag value (we have to do this after the syscall, otherwise no exception safety)
        nonBlockingIo = nonBlockVal;
    } 
    
    void ServerSocketFsm::setFdBlockingBehav(FdGuard& guard, bool nonBlockVal){
        int failure = -1;
//        std::cout << guard.get() << std::endl;
        //first get the flags
        int flags = executeSyscall(::fcntl, failure, guard.get(), F_GETFL, 0);
        
        if(nonBlockVal){
            flags |= O_NONBLOCK;
        }else{
            flags &= (~O_NONBLOCK);
        }
        //then set the new flags
        executeSyscall(::fcntl, failure,guard.get(),  F_SETFL, flags);
    }

    SocketAddress ServerSocketFsm::getSockAddr(){
        if(!isBound()){
            throw SnlException("ServerSocket exception: trying to get the socket address of an unbound socket");
        }
        return socketAddr;
    }
    
    IpAddress ServerSocketFsm::getIpAddress(){
        return getSockAddr().getIpAddress();
    }
    
    TcpPort ServerSocketFsm::getTcpPort(){
        return getSockAddr().getTcpPort();
    }

    bool ServerSocketFsm::isBound(){
        return ServerFsmState::BOUND <= fsmState;
    }
    bool ServerSocketFsm::isListening(){
        return ServerFsmState::LISTENING <= fsmState && fsmState < ServerFsmState::CLOSED;
    }
    bool ServerSocketFsm::isAccepting(){
        return ServerFsmState::ACCEPTING == fsmState;
    }
    bool ServerSocketFsm::isClosed(){
        return ServerFsmState::CLOSED == fsmState;
    }
    
    /*
     * Checks on the order of the fsm
     */ 
    
    void ServerSocketFsm::bindCheck(ServerFsmState current) { 
        if(current != ServerFsmState::INIT){ 
            throw SnlException("ServerSocket error: binding to already bound socket"); 
        }
    }
    void ServerSocketFsm::listenCheck(ServerFsmState current) { 
        if(current != ServerFsmState::BOUND) {
            if(current < ServerFsmState::BOUND){
                throw SnlException("ServerSocket error: trying to listen with unbound socket");
            }else{
                throw SnlException("ServerSocker error: trying to listen with already listening socket");
            }
        }
    }
    
//    void ServerSocketFsm::defaultListenCheck(ServerFsmState current){
//        if(current != ServerFsmState::INIT){
//            throw SnlException("ServerSocket error: trying to default listen with already initialized socket");
//        }
//    }
    
    void ServerSocketFsm::acceptCheck(ServerFsmState current){
        if(current != ServerFsmState::LISTENING && current != ServerFsmState::ACCEPTING){
            throw SnlException("ServerSocket error: trying to accept with socket that is either unbound or already closed");
        }
    }
    
    void ServerSocketFsm::closeCheck(ServerFsmState current){
        if(current == ServerFsmState::CLOSED){
            throw SnlException("ServerSocket error: trying to close an already closed socket");
        }
    }
    
    bool operator <(ServerSocketFsm::ServerFsmState lhs, ServerSocketFsm::ServerFsmState rhs){
        using StateType = ServerSocketFsm::ServerFsmState;
        //cast the scoped enum to the underlying type and compare
        return static_cast<std::underlying_type_t<StateType>>(lhs) < static_cast<std::underlying_type_t<StateType>>(rhs);
    }
    
    bool operator>(ServerSocketFsm::ServerFsmState lhs, ServerSocketFsm::ServerFsmState rhs){
        return rhs < lhs;
    }
    bool operator<=(ServerSocketFsm::ServerFsmState lhs, ServerSocketFsm::ServerFsmState rhs){
        using StateType = ServerSocketFsm::ServerFsmState;
        //cast the scoped enum to the underlying type and compare
        return static_cast<std::underlying_type_t<StateType>>(lhs) <= static_cast<std::underlying_type_t<StateType>>(rhs);
    }
    bool operator>=(ServerSocketFsm::ServerFsmState lhs, ServerSocketFsm::ServerFsmState rhs){
        return rhs <= lhs;
    }
    
}

//
// //first param is the current state, the second is the action
//    template<ServerSocketFsm::ServerFsmState CurrentState, typename ActionName>
//    struct TransitionCheck{
//        TransitionCheck() noexcept = default; //default constructor
//        //all the transition checks that are not valid will trigger the assertion
//        //static_assert(std::false_type(), "invalid state transition");
//        static constexpr bool value = false; 
//    };
//    
//    
//    template<> //current: init, action: bind
//    struct TransitionCheck <ServerSocketFsm::ServerFsmState::INIT, ServBind> { TransitionCheck() noexcept = default;
//                                                                         static constexpr bool value = true; };
//    
//    template<> //current: init, action: default listen
//    struct TransitionCheck <ServerSocketFsm::ServerFsmState::INIT, ServDefaultListen> { TransitionCheck() noexcept = default; };
//
//    template<> //current: bound, action: listen
//    struct TransitionCheck <ServerSocketFsm::ServerFsmState::BOUND, ServListen> { TransitionCheck() noexcept = default; };
//    
//    template<> //current: listening, action: accept
//    struct TransitionCheck <ServerSocketFsm::ServerFsmState::LISTENING, ServAccept> { TransitionCheck() noexcept = default; };
//    
//    template<>//current: accepting, action: accept (new connection)
//    struct TransitionCheck <ServerSocketFsm::ServerFsmState::ACCEPTING, ServAccept> { TransitionCheck() noexcept = default; };
//    
//    template<>//current: accepting, action: close
//    struct TransitionCheck <ServerSocketFsm::ServerFsmState::ACCEPTING, ServClose> { TransitionCheck() noexcept = default; };
//    
//    template<ServerSocketFsm::ServerFsmState CurrentState>//current: anything, action: reset
//    struct TransitionCheck <CurrentState, ServBind> { TransitionCheck() noexcept = default; };
//    
//    
//    
//        
//    
//    void checker(){
//        using Fsm = ServerSocketFsm::ServerFsmState;
//        TransitionCheck<Fsm::INIT, ServBind> base{};
//        static_assert(TransitionCheck<Fsm::INIT, ServBind>::value);
//        //std::cout << TransitionCheck<Fsm::INIT, ServBind>::value << std::endl;
////        TransitionCheck<Fsm::BOUND, ServListen>();
////        TransitionCheck<Fsm::LISTENING, ServAccept>();
////        TransitionCheck<Fsm::ACCEPTING, ServAccept>();
////        TransitionCheck<Fsm::ACCEPTING, ServClose>();
////        TransitionCheck<Fsm::CLOSED, ServReset>();
//    }

