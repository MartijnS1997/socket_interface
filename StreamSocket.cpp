#include "StreamSocket.h"
//c headers
#include <cassert>
#include <sys/socket.h>
//cpp headers
#include <sstream>
//own headers
#include "StreamSocketFsm.h"
#include "IpAddress.h"
#include "TcpPort.h"
namespace snl{
    
    constexpr int messageDontWaitFlag = MSG_DONTWAIT; //flag that makes the socket non blocking for one call
    constexpr int MessageWaitFlag = MSG_WAITALL; //flag that makes the socket wait on a receive untill the message is sent
    
    StreamSocket::StreamSocket():fsmImpl(std::make_unique<StreamSocketFsm>()){ }

    StreamSocket::~StreamSocket(){ }
    
    StreamSocket::StreamSocket(StreamSocket&& rhs) : fsmImpl(std::move(rhs.fsmImpl)){ }
    
    StreamSocket::StreamSocket(FdGuard&& guard, const SocketAddress& sockAddr) : fsmImpl(std::make_unique<StreamSocketFsm>(std::move(guard), sockAddr)) { }
    
    StreamSocket& StreamSocket::operator=(StreamSocket&& rhs){
        assert(this != &rhs);
        this->fsmImpl = std::move(rhs.fsmImpl);
        return *this;
    }
    
    
    void StreamSocket::connect(const SocketAddress& sockAddr){ fsmImpl->toNextState(connectAct, sockAddr); }
    
    void StreamSocket::connect(const IpAddress& address, const TcpPort& tcpPort){ fsmImpl->toNextState(connectAct, SocketAddress(address, tcpPort));}
    
    std::size_t StreamSocket::send(const void* buffer, std::size_t bufferSize, int flags){
        std::size_t bytesSent = 0;
        fsmImpl->toNextState(sendAct, buffer, bufferSize, bytesSent, flags);
        return bytesSent;
    }
    
    std::size_t StreamSocket::receive(void* buffer, std::size_t bufferSize, int flags){
        std::size_t bytesReceived = 0;
        fsmImpl->toNextState(receiveAct, buffer, bufferSize, bytesReceived, flags);
        return bytesReceived;
    }
    
    void StreamSocket::closeUpstream(){ fsmImpl->toNextState(upstrCloseAct);}
    
    void StreamSocket::closeDownstream(){ fsmImpl->toNextState(downstrCloseAct); }
    
    void StreamSocket::close(){fsmImpl->toNextState(closeAct); }
    
    void StreamSocket::reset(){ fsmImpl->toNextState(resetAct); }
    
    void StreamSocket::resetConnection(){ fsmImpl->toNextState(resetConnectAct); }
    
    void StreamSocket::setNonBlockIO(bool nonBlockVal){ fsmImpl->setNonBlock(nonBlockVal); }
    
    bool StreamSocket::isNonBlock() const { return fsmImpl->isNonBlock(); }
    
    SocketAddress StreamSocket::getSocketAddress() const { return fsmImpl->getSockAddress(); }
    
    IpAddress StreamSocket::getIpAddress() const { return getSocketAddress().getIpAddress(); }
    
    TcpPort StreamSocket::getTcpPort() const{ return getSocketAddress().getTcpPort(); }
    
    bool StreamSocket::isConnected() const { return fsmImpl->isConnected(); }
    
    bool StreamSocket::upstreamClosed() const { return fsmImpl->upstreamClosed(); }
    
    bool StreamSocket::downStreamClosed() const { return fsmImpl->downStreamClosed(); }
        
    bool StreamSocket::isClosed() const { return fsmImpl->isClosed(); }
    
    std::size_t readline(StreamSocket& strSock, std::string& lineBuff, const std::string& eol){
        lineBuff.clear();
        char buff;
        do{
            strSock.receive(&buff, sizeof(char), MessageWaitFlag);
            lineBuff.push_back(buff);
        }while(lineBuff.rfind(eol)==std::string::npos);
        
        for(std::size_t i = 0; i != eol.size(); i++){
            lineBuff.pop_back();
        }
        
        return lineBuff.size();
    }
    
    void sendline(StreamSocket& strSock, const std::string& line, const std::string& eol){
        //convert the line into a c-string
        
        std::ostringstream strBldr;
        strBldr << line << eol;
        std::string message = strBldr.str();
        const char* msgBuff = message.c_str();
        
        std::size_t charsSent = 0;
        std::size_t charsToSend = message.size();
        
        do{
            charsSent += strSock.send((msgBuff + charsSent), charsToSend - charsSent);
            
        }while(charsSent  != charsToSend);
    }
    
    void sendBuff(StreamSocket& strSock, const void* buffer, std::size_t bufferSize){
        //we need to loop until the entire message is sent to the other side
        //note that pointer arithmetic is not possible on a void pointer, so we need to cast the buffer
        const char* bufferHead = reinterpret_cast<const char*>(buffer);
        std::size_t bytesSent = 0;
        
        //keep sending the buffer (with modified head) untill all the bytes are sent
        do{
            bytesSent += strSock.send(bufferHead + bytesSent, bufferSize - bytesSent);
        }while(bytesSent != bufferSize);
        
        //done(the send will throw errors if something goes wrong)
    }
    
    
    std::size_t receiveBuff(StreamSocket& strSock, void* buffer, std::size_t bufferSize){
        //just do one call the receive, will not block the caller (good for multithreaded implementations)
        return strSock.receive(buffer, bufferSize, messageDontWaitFlag);
    }
}


