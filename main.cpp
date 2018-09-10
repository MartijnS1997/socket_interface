#include <iostream>
#include <cassert>
#include <fstream>
#include <thread>
//own headers
#include "SnlException.h"
#include "IpAddress.h"
#include "SocketAddress.h"
#include "ServerSocket.h"
#include "TcpPort.h"
#include "StreamSocket.h"

//test function declaration
void snlExceptionTest();
void ipStringTest();
void servSockFsmTest();
void strSockTest(const std::string&);
void serverFunc(std::size_t);

int main(int argc, char **argv)
{
    std::string msg("hello there! i hope this message comes trough... otherwise i would be sad :(");
    //servSockFsmTest();
    //serverFunc(msg.size());

    strSockTest(msg);

}

void serverFunc(std::size_t dataToRead){
    std::cout << "Server started" << std::endl;
    //open a server socket
    snl::ServerSocket sock{"localhost", 8000};
//    sock.bind(snl::SocketAddress("localhost", "8000"));
//    sock.listen();
    snl::StreamSocket strSock = sock.accept();
    
    char buffer[dataToRead +1];
    std::size_t dataRead = 0;
    do{
        dataRead += snl::receiveBuff(strSock, (&buffer[dataRead]), dataToRead - dataRead);

    }while(dataRead != dataToRead);
    buffer[dataToRead] = '\0';
    //print out the result 
    std::cout << buffer << std::endl;
    strSock.close();
    sock.close();
}

void strSockTest(const std::string& message){
    std::cout << "Client started" << std::endl;
    //then launch our own client sock
    snl::StreamSocket client;
    client.connect("localhost", 8000);
    snl::sendBuff(client, message.c_str(), message.size());
    //close the client 
    client.close();
}

void servSockFsmTest(){
    snl::IpAddress ipAddr; // no errors here
    snl::IpAddress newAddr = std::move(ipAddr); //move works fine
    snl::SocketAddress sockAddress;
    snl::ServerSocket servSock(8000);
    snl::StreamSocket strSock = servSock.accept();
    strSock.setNonBlockIO(false);
    
    snl::sendline(strSock, "hello, this is test 101 what do you require?");
    snl::sendline(strSock, "escape character is |quit|");
    
    std::cout << "str sock host ip address: " << strSock.getIpAddress().getIpString() << std::endl;
    
    servSock.close();
    std::string line;
    do{
        readline(strSock, line);
        std::cout << line << std::endl;
    }
    while(line.find("|quit|") == std::string::npos);
    
    snl::sendline(strSock, "response registered");
    
    std::cout << "closing " << std::endl;
    strSock.close();

}    

void ipStringTest(){
    std::cout << "____ip string test____" << std::endl;
    
    //first create some ip addresses
    //and print them
    snl::IpAddress addr1("www.google.com");
    snl::IpAddress addr2("www.facebook.com");
    snl::IpAddress addr3("localhost");
    snl::IpAddress addr4 = snl::makeIpv4Address("192.63.2.6");
    
    assert(addr4.isIpv4());
    
    try{
        snl::IpAddress addr5 = snl::makeIpv6Address("15456::558:445::0");
        assert(true==false);
        
    }catch(snl::SnlException &e){
        //ok must happen
    }
    
    snl::IpAddress addr5 = addr4;
    
    std::cout << addr1.getIpString() << std::endl;
    std::cout << addr2.getIpString() << std::endl;
    std::cout << addr3.getIpString() << std::endl;
    std::cout << addr4.getIpString() << std::endl;
    std::cout << addr5.getIpString() << std::endl;
}

void snlExceptionTest(){
    
    std::cout << "____snl exception test____" << std::endl;
    
    int eCode = 5;
    
    try{
        throw snl::SnlException("testing error message: ", eCode);
    }catch(snl::SnlException &e){
        assert(e.getErrorNo() == eCode);
    }
}

