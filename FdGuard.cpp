#include "FdGuard.h"
//c headers
#include <cassert>
#include <unistd.h>
//cpp headers
#include <iostream>
//own headers
namespace snl{
    
    FdGuard::FdGuard() noexcept : FdGuard(NO_FD){ }
    
    FdGuard::FdGuard(Fd fd) noexcept: fd(fd) { }
    
    FdGuard::FdGuard(FdGuard&& rhs) noexcept : fd(rhs.fd) { rhs.fd = NO_FD; } //transfer owenership
    
    FdGuard& FdGuard::operator= (FdGuard&& rhs){
        assert(this != &rhs);
        //1. close the current fd
        closeFd(this->fd);
        //2. assign the next fd
        this->fd = rhs.fd;
        //3. reset the rhs fd (transfer the ownership)
        rhs.fd = NO_FD;
        //4. return reference to this
        return *this;
    }
    
    FdGuard::~FdGuard(){
        close();
    }
    
    const Fd FdGuard::get() const noexcept{
        return fd;
    }
    
    Fd FdGuard::release() noexcept{
        //1. save the current fd
        Fd temp = fd;
        //2. then clear the owned fd
        fd = NO_FD;
        //3. then return the temporary
        return temp;
    }
    
    void FdGuard::reset(Fd fd_){
        //1. first check if the provided fd is equal to the local fd, if so, return
        if(fd == fd_){
            return;
        }
        
        //2. else close the current fd and set the new fd, but check for error
        closeFd(this->fd);
        
        //3. then transfer the ownership
        this->fd = fd_;
    }
    
    void FdGuard::close(){
        closeFd(this->fd); //forward the call
        fd = NO_FD;
    }
    
    bool FdGuard::ownsFd() const noexcept{
        return fd != NO_FD; //just check if the fd has the value of NO_FD
    }
    
    void FdGuard::closeFd(Fd fd_){
        //check first if the provided fd is owned
        if(!isOwnableFd(fd_)){
            return;
        }
        
        //if it is owned, close it
        int status = ::close(fd_); //close the file descriptor(with general close, use global scope operator)
        
        if(status != 0){
            throw SnlException("FdGuard error: ", errno);
        }
        
    }
    
    bool FdGuard::isOwnableFd(Fd fd){
        return fd != NO_FD; //just check if the fd can be an owned fd
    }
    
    

}

