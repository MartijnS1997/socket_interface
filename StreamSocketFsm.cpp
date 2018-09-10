#include "StreamSocketFsm.h"
//c headers
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
//cpp headers
#include <iostream>
//own headers

//declare extern c function to prevent mangled names:
extern "C" {
    int socket(int, int, int);
    int listen(int, int);
    ssize_t send(int, const void*, size_t, int);
    ssize_t recv(int, void*, size_t, int);
    int shutdown(int, int);
    int close(int);
    int fcntl(int, int , ... );
};


namespace snl{
    
    
    StreamSocketFsm::StreamSocketFsm() : fsmState(StrSoFsmState::INIT){ }
    
    StreamSocketFsm::StreamSocketFsm(FdGuard&& fdGuard, SocketAddress socketAddress_) : //pass by value is justified (will always be copied)
         socketAddress(std::move(socketAddress_)), strSoFd(std::move(fdGuard)), fsmState(StrSoFsmState::CONNECTED) { std::cout << "created socket with server" << std::endl; }

    StreamSocketFsm::~StreamSocketFsm(){}
    

    void StreamSocketFsm::toNextStateImpl(const StrSoConnect&, const SocketAddress& socketAddress){ //do not a pass by value, if the check throws, unescessary copy
        std::cout << ((&socketAddress) == nullptr) << std::endl;
        std::cout << (this == nullptr) << std::endl;
        connectCheck(fsmState); // will detect wrong order
        //create and connect the socket
        FdGuard guard = createSockAndConnect(socketAddress, isNonBlock());
        //then save the socket address && fd
        this->socketAddress = std::move(socketAddress);
        this->strSoFd = std::move(guard);
        //set the state to connected
        fsmState = StrSoFsmState::CONNECTED;
    }
    
    FdGuard StreamSocketFsm::createSockAndConnect(const SocketAddress& address, bool nonBlockVal){
        FdGuard guard = makeFdGuard(::socket, address.getAddressFamily(), SOCK_STREAM, 0);
        //then connect based on the socket address
        int failure = -1;
        sockaddr_storage hostStorage = address.getSockaddrStorage();
        sockaddr* hostSpec = reinterpret_cast<sockaddr*>(&hostStorage);
        executeSyscall(::connect, failure, guard.get(), hostSpec, address.getAddrlen());
        
        setFdBlockingBehav(guard, nonBlockVal);
        
        return guard;
    }
    
    void StreamSocketFsm::toNextStateImpl(const StrSoSend&, const void* buffer, std::size_t bufferSize, std::size_t& bytesSent, int flags){
        sendCheck(fsmState);
        int failure = -1;
        bytesSent = executeSyscall(::send, failure, strSoFd.get(), buffer, bufferSize, flags);       
        //we do not need to save anything
    }
    
    void StreamSocketFsm::toNextStateImpl(const StrSoReceive&, void* buffer, std::size_t bufferSize, std::size_t& bytesReceived, int flags){
        receiveCheck(fsmState);
        int failure = -1;
        bytesReceived = executeSyscall(::recv, failure, strSoFd.get(), buffer, bufferSize, flags);
        //checks if the end of the file has been reached
        //second check is to guarantee that the 0 bytes read is because of eof or an empty receive request
        if(bytesReceived == 0 && bufferSize != 0){
            throw SnlEofException("End of file reached");
        }
    }
    void StreamSocketFsm::toNextStateImpl(const StrSoUClose&){
        upstreamCloseCheck(fsmState);
       
        //if the socket was currently connected, goto ustream closed state
        if(fsmState == StrSoFsmState::CONNECTED){
            int failure = -1;
            //close the upstream first
            int upsd = upstreamShutdown; //local var (because the forwarding takes reference of upstr shutdown --> forwarding
            executeSyscall(::shutdown, failure, strSoFd.get(), upsd);
            fsmState = StrSoFsmState::UCLOSED;
        }else{
            //close the socket in the case that the state is DCLOSED
            //just close the whole socket (we do not need to spend another syscall
            toNextStateImpl(closeAct);
        }
    }
    void StreamSocketFsm::toNextStateImpl(const StrSoDClose&){
        downsreamCloseCheck(fsmState);
        //the action depends on the current state
        //if we are currently connected, we need to dispatch a close on the downstream
        if(fsmState == StrSoFsmState::CONNECTED){
            //execute syscall and close the downstream
            int failure = -1;
            int dssd = downstreamShutdown;
            executeSyscall(::shutdown, failure, strSoFd.get(), dssd);
            fsmState = StrSoFsmState::DCLOSED;
    }else{
        //if the upstream is already closed, no need to close the downstream first
        //clsoe the whole socket, avoid an unescessary syscall
        toNextStateImpl(closeAct);
    }
        
    }
    void StreamSocketFsm::toNextStateImpl(const StrSoClose&){
        closeCheck(fsmState);
        //close the socket with syscall
        int failure = -1;
        strSoFd.close();
        //advance state
        fsmState = StrSoFsmState::CLOSED;
        
    }
    void StreamSocketFsm::toNextStateImpl(const StrSoReset&){
        //to reset the socket goto init and set the other stuff to null
        strSoFd.close();
        socketAddress = SocketAddress();
        nonBlock = defaultNonBlock;
        fsmState = StrSoFsmState::INIT;
        //done
    }
    
    void StreamSocketFsm::toNextStateImpl(const StrSoReConnect&){
        resetConnectCheck(fsmState);
        strSoFd.close(); //will close the socket in case of owning a socket
        strSoFd = std::move(createSockAndConnect(getSockAddress(), isNonBlock()));
        fsmState = StrSoFsmState::CONNECTED;
    }
    
    void StreamSocketFsm::setNonBlock(bool nonBlockVal){
        std::cout << "setting non block to " << std::boolalpha << nonBlockVal << std::noboolalpha << std::endl;

        //first check if the current blocking value is already the desired one
        if(nonBlock == nonBlockVal){
            return; //already desired, skip this
        }
        
        //then check if we already own a fd, if not, save flag for later (upon connection the socket will be set to nonblock)
        if(fsmState == StrSoFsmState::INIT){
            return;
        }
        //else we set the blocking/nonblocking behav
        setFdBlockingBehav(strSoFd, nonBlockVal);
    }
    
    void StreamSocketFsm::setFdBlockingBehav(FdGuard& guard, bool nonBlockVal){
        int failure = -1;
        //first get the flags
        int flags = executeSyscall(::fcntl, failure, guard.get(), F_GETFL, 0);
        
        if(nonBlockVal){
            flags |= O_NONBLOCK;
        }else{
            flags &= (~O_NONBLOCK);
        }
        //then set the new flags
        executeSyscall(::fcntl, failure,guard.get(),  F_SETFL, flags);
        std::cout << "changed blocking behavior" << std::endl;
    }
    
    
    /*
     * inspection functionality
     */ 

    SocketAddress StreamSocketFsm::getSockAddress(){
        if(!isConnected()){
            throw SnlException("ServerSocket error: trying to get the socket address from unconnected socket");
        }
        
        return socketAddress;
    }
    
    bool StreamSocketFsm::isNonBlock(){
        return nonBlock;
    }
    bool StreamSocketFsm::isConnected(){
        return StrSoFsmState::CONNECTED <= fsmState && fsmState < StrSoFsmState::CLOSED;
    }
    
    bool StreamSocketFsm::upstreamClosed(){
        return StrSoFsmState::UCLOSED == fsmState || StrSoFsmState::CLOSED == fsmState;
    }
    
    bool StreamSocketFsm::downStreamClosed(){
        return StrSoFsmState::DCLOSED == fsmState || StrSoFsmState::CLOSED == fsmState;
    }
    
    bool StreamSocketFsm::isClosed(){
        return fsmState == StrSoFsmState::CLOSED;
    }
    
    /*
     * state transition control
     */ 
    
    void StreamSocketFsm::connectCheck(StrSoFsmState current){
        if(current != StrSoFsmState::INIT){
            throw SnlException("StreamSocket error: connect fail");
        }
    }
    void StreamSocketFsm::sendCheck(StrSoFsmState current){
        if(current != StrSoFsmState::CONNECTED && current != StrSoFsmState::DCLOSED){
            throw SnlException("StreamSocket error: Send fail");
        }
    }
    void StreamSocketFsm::receiveCheck(StrSoFsmState current){
        if(current != StrSoFsmState::CONNECTED && current != StrSoFsmState::UCLOSED){
            throw SnlException("StreamSocket error: Receive fail");
        }
    }
    void StreamSocketFsm::upstreamCloseCheck(StrSoFsmState current){
        if(current != StrSoFsmState::DCLOSED && current != StrSoFsmState::CONNECTED){
            throw SnlException("StreamSocket error: upstream close fail");
        }
    }
    void StreamSocketFsm::downsreamCloseCheck(StrSoFsmState current){
        if(current != StrSoFsmState::UCLOSED && current != StrSoFsmState::CONNECTED){
            throw SnlException("StreamSocket error: downstream close fail");
        }
    }
    void StreamSocketFsm::closeCheck(StrSoFsmState current){
        if(current == StrSoFsmState::CLOSED){
            throw SnlException("StreamSocket error: close fail, the socket is already closed");
        }
    }
    void StreamSocketFsm::resetConnectCheck(StrSoFsmState current){
        if(current == StrSoFsmState::INIT){
            throw SnlException("StreamSocket error: connection reset fail, the socket has no specified host");
        }
    }
    
//    bool operator<(cons tStreamSocketFsm::StrSoFsmState& lhs, const StreamSocketFsm::StrSoFsmState& rhs){
//        using fsm = StreamSocketFsm::StrSoFsmState;
//        return static_cast<std::underlying_type_t<fsm>>(lhs) < static_cast<std::underlying_type_t<fsm>>(rhs);
//    }
////    bool operator>(const StreamSocketFsm::StrSoFsmState& lhs, const StreamSocketFsm::StrSoFsmState& rhs){
////        return rhs < lhs;
////    }
//    
//    bool operator<=(const StreamSocketFsm::StrSoFsmState& lhs, const StreamSocketFsm::StrSoFsmState& rhs){
//        return static_cast<std::underlying_type_t<fsm>>(lhs) < static_cast<std::underlying_type_t<fsm>>(rhs);
//    }
////    
//    bool operator>=(const StreamSocketFsm::StrSoFsmState& lhs, const StreamSocketFsm::StrSoFsmState& rhs){
//        return rhs <= lhs;
//    }


}



