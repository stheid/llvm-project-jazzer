//
// Created by sheid on 07.04.21.
//

#include "Socket.h"
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using std::string;

Socket::Socket(const char *SockPath) {
  // sending arbitrary long msgs: https://stackoverflow.com/a/2862176/13310191

  // create socket
  sockaddr_un Address;
  Sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  SocketPath = SockPath;
  if (Sockfd == -1) {
    std::cerr << "could not create unix socket" << std::endl;
    exit(EXIT_FAILURE);
  }

  Address.sun_family = AF_UNIX;
  strcpy(Address.sun_path, SocketPath);

  // connect to socket
  unlink(SocketPath);
  if (bind(Sockfd, (sockaddr *)&Address, sizeof(Address)) == -1) {
    std::cerr << "could not bind to '" << SocketPath << "'" << std::endl;
    ::close(Sockfd);
    exit(EXIT_FAILURE);
  }

  /* Prepare for accepting connections. The backlog size is set
   * to 20. So while one request is being processed other requests
   * can be waiting. */
  if (listen(Sockfd, 1) == -1) {
    std::cerr << "could not listen to '" << SocketPath << "'" << std::endl;
    exit(EXIT_FAILURE);
  }

  Conn = accept(Sockfd, NULL, NULL);
  if (Conn == -1) {
    std::cerr << "could not accept connection to '" << SocketPath << "'"
              << std::endl;
    exit(EXIT_FAILURE);
  }
  std::cout << "Client established connection";
}

bool Socket::read(dataOut *Out) {
  int_asbytes Len;
  int_asbytes MutRep;

  // recieve number of mutation repititions
  ::recv(Conn, MutRep.Byte, 4, 0);
  Out->mutRep = MutRep.Integer;

  // recieve length of the upcomming msg
  ::recv(Conn, Len.Byte, 4, 0);
  if (Len.Integer == 0) {
    return false;
  }
  int N;
  char RetValue[Len.Integer];
  // now reading file contents
  N = ::recv(Conn, RetValue, Len.Integer, 0);
  if (N < 0) {
    std::cout << "socket read failed" << std::endl;
    exit(EXIT_FAILURE);
  }

  Out->fileContents = RetValue;
  return true;
}

bool Socket::write(std::vector<std::string> Data) {
  int_asbytes Len;
  // send number of inputs
  Len.Integer = Data.size();
  if (::write(Conn, Len.Byte, 4) == -1) {
    std::cout << "Error" << std::endl;
    exit(EXIT_FAILURE);
  }
  for (auto Dat : Data) {
    // send length of the cov vector and the vector itself
    Len.Integer = Dat.size();
    if (::write(Conn, Len.Byte, 4) == -1) {
      std::cout << "Error" << std::endl;
      exit(EXIT_FAILURE);
    }

    if (::write(Conn, Dat.c_str(), Dat.size()) == -1) {
      std::cout << "Error" << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  return true;
}

bool Socket::close() {
  ::close(Conn);
  ::close(Sockfd);
  unlink(SocketPath);
  return true;
}

Socket::~Socket() { unlink(SocketPath); }
