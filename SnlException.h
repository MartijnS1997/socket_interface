#ifndef SNLEXCEPTION_H
#define SNLEXCEPTION_H
#include <stdexcept>
#include <system_error>
#include <string>

namespace snl{
    class SnlException : public std::runtime_error{
    public:
        /**
         * @brief constructor for an snl exception, will take a string as input
         * will set the error code to zero (default value)
         */ 
        SnlException(const std::string& errorMsg);
        /**
         * @brief constructor for an error message taking an error message and
         *        and an errno to create an error code
         * @param errorMsg the error message
         * @param errno the errno to generate the errcode from
         */
        SnlException(const std::string& errorMsg, int errorNo);
        
        virtual ~SnlException() override;
        
        /**
         * @brief getter for the error no that was used to produce the snl exception
         * @return an integer representing the error number
         * note is wrapper for the error_code.value() call
         */
        int getErrorNo() const noexcept;
        
        /**
         * @brief getter for the error code that corresponds with the snl exception
         * @return the error code
         */
        std::error_code getErrorCode() const noexcept;
        
    private:
        std::error_code errorCode;
        
        //some local static functions to aid the process
        static std::string generateErrorMessage(const std::string& errorMsg, int errorNo);
    };
    
    class SnlEofException : public SnlException{
        public: SnlEofException(const std::string& errorMsg);
    };
}


#endif // SNLEXCEPTION_H
