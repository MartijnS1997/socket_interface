#include "SnlException.h"
#include <cerrno>    
#include <cstring>    

namespace snl{
    
    SnlException::SnlException(const std::string& errorMsg) : std::runtime_error(errorMsg), errorCode(0, std::generic_category()){ }
    
    SnlException::SnlException(const std::string& errorMsg, int errorNo) : 
        std::runtime_error(generateErrorMessage(errorMsg, errorNo)), errorCode(errorNo, std::generic_category()) {}
    
    std::string SnlException::generateErrorMessage(const std::string& errorMsg, int errorNo){
        //concatinate the error message and the error number message
        std::string errorNoString = std::strerror(errorNo);
        return errorMsg + errorNoString;
    }
    
    SnlException::~SnlException() { } // we do not need to add anyting to the destructor
    
    int SnlException::getErrorNo() const noexcept{
        return errorCode.value();
    }
    
    std::error_code SnlException::getErrorCode() const noexcept{
        return errorCode;
    }
    
    SnlEofException::SnlEofException(const std::string& errorMsg) : SnlException(errorMsg) {}
    
    
}

