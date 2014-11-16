#ifndef _H_NET_
#define _H_NET_

#include <iostream>
#include <vector>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "decr.h"


using namespace std;
KMQ_DECLARATION_START

enum OpenMode {
    O_ACTIVE = 0x01,
    O_PASSIVE = 0x02,
};

enum SockOpt {
    SO_TIMEOUT = 0x01,
    SO_WRITECACHE = 0x02,
    SO_READCACHE = 0x03,
    SO_NONBLOCK = 0x04,
    SO_NODELAY = 0x05,
    SO_QUICKACK = 0x06,
};


class Conn {
 public:
    virtual ~Conn() {

    }
    
    virtual int Fd() = 0;
    virtual int OpenMode() = 0;
    virtual int64_t Read(char *buf, int64_t len) = 0;
    virtual int64_t ReadCache(char *buf, int64_t len) = 0;
    virtual int64_t Write(const char *buf, int64_t len) = 0;
    virtual int64_t CacheSize(int op) = 0;
    virtual int Flush() = 0;
    virtual int Close() = 0;
    virtual int Reconnect() = 0;
    virtual int LocalAddr(string &addr) = 0;
    virtual int RemoteAddr(string &addr) = 0;
    virtual int SetSockOpt(int op, ...) = 0;
};


class Listener {
 public:
    virtual ~Listener() {

    }
    virtual int Fd() = 0;
    virtual Conn *Accept() = 0;
    virtual int Close() = 0;
    virtual int Addr(string &addr) = 0;
    virtual int SetNonBlock(bool nonblock) = 0;
};



class UnixConn: public Conn {
 public:
    UnixConn();
    ~UnixConn();

    inline int Fd() {
	return sockfd;
    }
    inline int OpenMode() {
	return openmode;
    }
    int64_t Read(char *buf, int64_t len);
    int64_t ReadCache(char *buf, int64_t len) {
	return 0;
    }
    int64_t Write(const char *buf, int64_t len);
    int64_t CacheSize(int op) {
	return 0;
    }
    int Flush() {
	return 0;
    }

    int Close();
    int Reconnect();
    int LocalAddr(string &laddr);
    int RemoteAddr(string &raddr);
    int SetSockOpt(int op, ...);
    
    int __errno, sockfd;
    int openmode;
    sockaddr_storage laddr, raddr;
    socklen_t laddrlen, raddrlen;
    string network, localaddr, remoteaddr;

 private:
    char *recvbuf, *sendbuf;
    int recvbuf_cap, recvbuf_len, recvbuf_idx;
    int sendbuf_cap, sendbuf_len, sendbuf_idx;

    int64_t raw_read(char *buf, int64_t len);
    int64_t raw_write(const char *buf, int64_t len);

    // Disable copy construction of UnixConn
    UnixConn (const UnixConn&);
    const UnixConn &operator = (const UnixConn&);
};

UnixConn *DialUnix(string net, string laddr, string raddr);



class TCPConn: public Conn {
 public:
    TCPConn();
    ~TCPConn();

    inline int Fd() {
	return sockfd;
    }
    inline int OpenMode() {
	return openmode;
    }
    int64_t Read(char *buf, int64_t len);
    int64_t ReadCache(char *buf, int64_t len);
    int64_t Write(const char *buf, int64_t len);
    inline int64_t CacheSize(int op) {
	switch (op) {
	case SO_READCACHE:
	    return recvbuf_len - recvbuf_idx;
	case SO_WRITECACHE:
	    return sendbuf_len - sendbuf_idx;
	}
	return -1;
    }
    int Flush();
    int Close();
    int Reconnect();
    int LocalAddr(string &laddr);
    int RemoteAddr(string &raddr);
    int SetSockOpt(int op, ...);
    
    // underlying socket infomation
    int __errno, sockfd;
    int openmode;
    sockaddr_storage laddr, raddr;
    socklen_t laddrlen, raddrlen;
    string network, localaddr, remoteaddr;

 private:
    char *recvbuf, *sendbuf;
    int recvbuf_cap, recvbuf_len, recvbuf_idx;
    int sendbuf_cap, sendbuf_len, sendbuf_idx;

    int64_t raw_read(char *buf, int64_t len);
    int64_t raw_write(const char *buf, int64_t len);

    void reset_recvbuf();
    void reset_sendbuf();

    // Disable copy construction of TCPConn
    TCPConn (const TCPConn&);
    const TCPConn &operator = (const TCPConn&);
};

TCPConn *DialTCP(string net, string laddr, string raddr);



class UDPConn: public Conn {

};


class TCPListener: public Listener {
 public:
    TCPListener();
    ~TCPListener();

    int Fd() {
	return sockfd;
    }
    TCPConn *Accept();
    int Close();
    int Addr(string &addr) {
	addr = localaddr;
	return 0;
    };
    int SetNonBlock(bool nonblock);
    int SetReuseAddr(bool reuse);

    int sockfd;
    int backlog;
    string localaddr;
    struct sockaddr_storage addr;
    socklen_t addrlen;

 private:

    // Disable copy construction of TCPConn
    TCPListener (const TCPListener&);
    const TCPListener &operator = (const TCPListener&);
};

TCPListener *ListenTCP(string net, string laddr, int backlog);


class UnixListener: public Listener {
 public:
    UnixListener();
    ~UnixListener();

    int Fd() {
	return sockfd;
    }
    UnixConn *Accept();
    int Close();
    int Addr(string &addr) {
	addr = localaddr;
	return 0;
    }
    int SetNonBlock(bool nonblock);
    
    int sockfd;
    int backlog;
    string localaddr;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    
 private:
    // Disable copy construction of TCPConn
    UnixListener (const UnixListener&);
    const UnixListener &operator = (const UnixListener&);
};

UnixListener *ListenUnix(string net, string laddr, int backlog);

}
 

#endif // _H_NET_
