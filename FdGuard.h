#ifndef FDGUARD_H
#define FDGUARD_H
#include <functional>
#include "SnlException.h"
namespace snl{
    
    using Fd = int;
    
    class FdGuard
    {
    public:
        template<typename FdConstructor, typename ...Args>
        friend FdGuard makeFdGuard(FdConstructor& fdConstructor, Args&&... args);
        
        friend void swap(FdGuard& lhs, FdGuard& rhs);
    
        FdGuard() noexcept;
        FdGuard(Fd fd) noexcept;
        
        FdGuard(FdGuard&& rhs) noexcept;
        FdGuard& operator=(FdGuard&& rhs);
        
        FdGuard(const FdGuard& rhs) = delete;
        FdGuard& operator=(const FdGuard& rhs) = delete;
        
        ~FdGuard();
        
        /**
         * @brief returns a handle to the file descriptor
         * @return the file descriptor that is currently owned,
         *         if there is no such fd, returns -1
         */
        const Fd get() const noexcept;
        
        /**
         * @brief releases ownership over the file descriptor
         * @return the file descriptor previously owned, if the
         *         fd guard did not own any fd, returns -1
         */
        Fd release() noexcept;
        
        /**
         * @brief closes the previously owned file descriptor and takes ownership over the provided fd
         *        if the fd did not previously own a fd only new ownership is taken
         * @param fd the file descriptor to take ownership off
         * @throws 
         */
        void reset(Fd fd);
        
        /**
         * @brief checks if the fd guard is currently owning a file descriptor
         * @return true if and only if a call to get() >= 0;
         */
        bool ownsFd() const noexcept;
        
        /**
         * @brief closes the file descriptor is the fd guard does own a fd
         *        if the fd guard does not own any fd, then this function has no effect
         * @throws SnlException if the fd could not be properly closed
         */
        void close();
        
    private:
        /**
         * The file descriptor currently guarded
         */ 
        Fd fd;
        
        /**
         * @brief general close function, closes the provided fd if the fd is not -1, else this method does nothing
         * @param fd the file descriptor to close
         * @throws SnlException if the fd could not be properly closed
         */
        static void closeFd(Fd fd);
        
        /**
         * @brief checks if the provided file descriptor is a valid fd
         * @param fd the filedescriptor to check
         * @return true if and only if fd != NO_FD
         */
        static bool isOwnableFd(Fd fd);
        
        //static indicating the value for the FD if the guard does not own any fd
        static constexpr int NO_FD = -1;
    };
    
    template<typename FdConstructor, typename ...Args>
    FdGuard makeFdGuard(FdConstructor& fdConstructor, Args&&... args){
        Fd fd = fdConstructor(std::forward<Args>(args)...); //create the fd by perfect forwarding
        
        //check for errors
        if(fd == -1){
            throw SnlException("FdGuard Error: ", errno);
        }
        
        return FdGuard(fd); //store the fd in a guard
    }
    
    /**
     * @brief executes a system call
     * @param failure the status code returned by the system call in case of failure
     * @return the statuscode if it is not the failure code (the needed type gets deducted and depends on the syscall
     * @throws if the status code of the syscall is failure, throws an snl exception
     */
    template<typename Syscall, typename ...Args>
    auto executeSyscall(const Syscall& sCall, int failure, Args&&...args){
        auto status = sCall(std::forward<Args>(args)...);
        
        if(status == failure){
            throw SnlException("System call error: ", errno);
        }
        
        return status;
    }
    
}

#endif // FDGUARD_H
