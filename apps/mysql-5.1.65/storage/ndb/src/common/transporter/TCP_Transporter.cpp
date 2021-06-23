/* Copyright (C) 2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <ndb_global.h>

#include <NdbTCP.h>
#include "TCP_Transporter.hpp"
#include <NdbOut.hpp>
#include <NdbSleep.h>

#include <EventLogger.hpp>
extern EventLogger g_eventLogger;
// End of stuff to be moved

#ifdef NDB_WIN32
class ndbstrerror
{
public:
  ndbstrerror(int iError);
  ~ndbstrerror(void);
  operator char*(void) { return m_szError; };

private:
  int m_iError;
  char* m_szError;
};

ndbstrerror::ndbstrerror(int iError)
: m_iError(iError)
{
  FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    0,
    iError,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR)&m_szError,
    0,
    0);
}

ndbstrerror::~ndbstrerror(void)
{
  LocalFree( m_szError );
  m_szError = 0;
}
#else
#define ndbstrerror strerror
#endif

TCP_Transporter::TCP_Transporter(TransporterRegistry &t_reg,
				 int sendBufSize, int maxRecvSize, 
                                 const char *lHostName,
                                 const char *rHostName, 
                                 int r_port,
				 bool isMgmConnection_arg,
				 NodeId lNodeId,
                                 NodeId rNodeId,
				 NodeId serverNodeId,
                                 bool chksm, bool signalId,
                                 Uint32 _reportFreq) :
  Transporter(t_reg, tt_TCP_TRANSPORTER,
	      lHostName, rHostName, r_port, isMgmConnection_arg,
	      lNodeId, rNodeId, serverNodeId,
	      0, false, chksm, signalId),
  m_sendBuffer(sendBufSize)
{
  maxReceiveSize = maxRecvSize;
  
  // Initialize member variables
  theSocket     = NDB_INVALID_SOCKET;
  
  sendCount      = receiveCount = 0;
  sendSize       = receiveSize  = 0;
  reportFreq     = _reportFreq;

  sockOptRcvBufSize = 70080;
  sockOptSndBufSize = 71540;
  sockOptNodelay    = 1;
  sockOptTcpMaxSeg  = 4096;
}

TCP_Transporter::~TCP_Transporter() {
  
  // Disconnect
  if (theSocket != NDB_INVALID_SOCKET)
    doDisconnect();
  
  // Delete send buffers
  
  // Delete receive buffer!!
  receiveBuffer.destroy();
}

bool TCP_Transporter::connect_server_impl(NDB_SOCKET_TYPE sockfd)
{
  DBUG_ENTER("TCP_Transpporter::connect_server_impl");
  DBUG_RETURN(connect_common(sockfd));
}

bool TCP_Transporter::connect_client_impl(NDB_SOCKET_TYPE sockfd)
{
  DBUG_ENTER("TCP_Transpporter::connect_client_impl");
  DBUG_RETURN(connect_common(sockfd));
}

bool TCP_Transporter::connect_common(NDB_SOCKET_TYPE sockfd)
{
  theSocket = sockfd;
  setSocketOptions();
  setSocketNonBlocking(theSocket);
  DBUG_PRINT("info", ("Successfully set-up TCP transporter to node %d",
              remoteNodeId));
  return true;
}

bool
TCP_Transporter::initTransporter() {
  
  // Allocate buffer for receiving
  // Let it be the maximum size we receive plus 8 kB for any earlier received
  // incomplete messages (slack)
  Uint32 recBufSize = maxReceiveSize;
  if(recBufSize < MAX_MESSAGE_SIZE){
    recBufSize = MAX_MESSAGE_SIZE;
  }
  
  if(!receiveBuffer.init(recBufSize+MAX_MESSAGE_SIZE)){
    return false;
  }
  
  // Allocate buffers for sending
  if (!m_sendBuffer.initBuffer(remoteNodeId)) {
    // XXX What shall be done here? 
    // The same is valid for the other init-methods 
    return false;
  }
  
  return true;
}

void
TCP_Transporter::setSocketOptions(){
  int sockOptKeepAlive  = 1;

  if (setsockopt(theSocket, SOL_SOCKET, SO_RCVBUF,
                 (char*)&sockOptRcvBufSize, sizeof(sockOptRcvBufSize)) < 0) {
#ifdef DEBUG_TRANSPORTER
    g_eventLogger.error("The setsockopt SO_RCVBUF error code = %d", InetErrno);
#endif
  }//if
  
  if (setsockopt(theSocket, SOL_SOCKET, SO_SNDBUF,
                 (char*)&sockOptSndBufSize, sizeof(sockOptSndBufSize)) < 0) {
#ifdef DEBUG_TRANSPORTER
    g_eventLogger.error("The setsockopt SO_SNDBUF error code = %d", InetErrno);
#endif
  }//if
  
  if (setsockopt(theSocket, SOL_SOCKET, SO_KEEPALIVE,
                 (char*)&sockOptKeepAlive, sizeof(sockOptKeepAlive)) < 0) {
    ndbout_c("The setsockopt SO_KEEPALIVE error code = %d", InetErrno);
  }//if

  //-----------------------------------------------
  // Set the TCP_NODELAY option so also small packets are sent
  // as soon as possible
  //-----------------------------------------------
  if (setsockopt(theSocket, IPPROTO_TCP, TCP_NODELAY, 
                 (char*)&sockOptNodelay, sizeof(sockOptNodelay)) < 0) {
#ifdef DEBUG_TRANSPORTER
    g_eventLogger.error("The setsockopt TCP_NODELAY error code = %d", InetErrno);
#endif
  }//if
}


#ifdef NDB_WIN32

bool
TCP_Transporter::setSocketNonBlocking(NDB_SOCKET_TYPE socket){
  unsigned long  ul = 1;
  if(ioctlsocket(socket, FIONBIO, &ul))
  {
#ifdef DEBUG_TRANSPORTER
    g_eventLogger.error("Set non-blocking server error3: %d", InetErrno);
#endif
  }//if
  return true;
}

#else

bool
TCP_Transporter::setSocketNonBlocking(NDB_SOCKET_TYPE socket){
  int flags;
  flags = fcntl(socket, F_GETFL, 0);
  if (flags < 0) {
#ifdef DEBUG_TRANSPORTER
    g_eventLogger.error("Set non-blocking server error1: %s", strerror(InetErrno));
#endif
  }//if
  flags |= NDB_NONBLOCK;
  if (fcntl(socket, F_SETFL, flags) == -1) {
#ifdef DEBUG_TRANSPORTER
    g_eventLogger.error("Set non-blocking server error2: %s", strerror(InetErrno));
#endif
  }//if
  return true;
}

#endif

bool
TCP_Transporter::sendIsPossible(struct timeval * timeout) {
  if(theSocket != NDB_INVALID_SOCKET){
    fd_set   writeset;
    FD_ZERO(&writeset);
    FD_SET(theSocket, &writeset);
    
    int selectReply = select(theSocket + 1, NULL, &writeset, NULL, timeout);

    if ((selectReply > 0) && FD_ISSET(theSocket, &writeset)) 
      return true;
    else
      return false;
  }
  return false;
}

Uint32
TCP_Transporter::get_free_buffer() const 
{
  return m_sendBuffer.bufferSizeRemaining();
}

Uint32 *
TCP_Transporter::getWritePtr(Uint32 lenBytes, Uint32 prio){
  
  Uint32 * insertPtr = m_sendBuffer.getInsertPtr(lenBytes);
  
  struct timeval timeout = {0, 10000};

  if (insertPtr == 0) {
    //-------------------------------------------------
    // Buffer was completely full. We have severe problems.
    // We will attempt to wait for a small time
    //-------------------------------------------------
    if(sendIsPossible(&timeout)) {
      //-------------------------------------------------
      // Send is possible after the small timeout.
      //-------------------------------------------------
      if(!doSend()){
	return 0;
      } else {
	//-------------------------------------------------
	// Since send was successful we will make a renewed
	// attempt at inserting the signal into the buffer.
	//-------------------------------------------------
        insertPtr = m_sendBuffer.getInsertPtr(lenBytes);
      }//if
    } else {
      return 0;
    }//if
  }
  return insertPtr;
}

void
TCP_Transporter::updateWritePtr(Uint32 lenBytes, Uint32 prio){
  m_sendBuffer.updateInsertPtr(lenBytes);
  
  const int bufsize = m_sendBuffer.bufferSize();
  if(bufsize > TCP_SEND_LIMIT) {
    //-------------------------------------------------
    // Buffer is full and we are ready to send. We will
    // not wait since the signal is already in the buffer.
    // Force flag set has the same indication that we
    // should always send. If it is not possible to send
    // we will not worry since we will soon be back for
    // a renewed trial.
    //-------------------------------------------------
    struct timeval no_timeout = {0,0};
    if(sendIsPossible(&no_timeout)) {
      //-------------------------------------------------
      // Send was possible, attempt at a send.
      //-------------------------------------------------
      doSend();
    }//if
  }
}

#define DISCONNECT_ERRNO(e, sz) ((sz == 0) || \
               (!((sz == -1) && (e == EAGAIN) || (e == EWOULDBLOCK) || (e == EINTR))))


bool
TCP_Transporter::doSend() {
  // If no sendbuffers are used nothing is done
  // Sends the contents of the SendBuffers until they are empty
  // or until select does not select the socket for write.
  // Before calling send, the socket must be selected for write
  // using "select"
  // It writes on the external TCP/IP interface until the send buffer is empty
  // and as long as write is possible (test it using select)

  // Empty the SendBuffers
  
  bool sent_any = true;
  while (m_sendBuffer.dataSize > 0)
  {
    const char * const sendPtr = m_sendBuffer.sendPtr;
    const Uint32 sizeToSend    = m_sendBuffer.sendDataSize;
    const int nBytesSent = send(theSocket, sendPtr, sizeToSend, 0);
    
    if (nBytesSent > 0) 
    {
      sent_any = true;
      m_sendBuffer.bytesSent(nBytesSent);
      
      sendCount ++;
      sendSize  += nBytesSent;
      if(sendCount == reportFreq)
      {
	reportSendLen(get_callback_obj(), remoteNodeId, sendCount, sendSize);
	sendCount = 0;
	sendSize  = 0;
      }
    } 
    else 
    {
      if (nBytesSent < 0 && InetErrno == EAGAIN && sent_any)
        break;

      // Send failed
#if defined DEBUG_TRANSPORTER
      g_eventLogger.error("Send Failure(disconnect==%d) to node = %d nBytesSent = %d "
	       "errno = %d strerror = %s",
	       DISCONNECT_ERRNO(InetErrno, nBytesSent),
	       remoteNodeId, nBytesSent, InetErrno, 
	       (char*)ndbstrerror(InetErrno));
#endif   
      if(DISCONNECT_ERRNO(InetErrno, nBytesSent)){
	doDisconnect();
	report_disconnect(InetErrno);
      }
      
      return false;
    }
  }
  return true;
}

int
TCP_Transporter::doReceive() {
  // Select-function must return the socket for read
  // before this method is called
  // It reads the external TCP/IP interface once
  Uint32 size = receiveBuffer.sizeOfBuffer - receiveBuffer.sizeOfData;
  if(size > 0){
    const int nBytesRead = recv(theSocket, 
				receiveBuffer.insertPtr, 
				size < maxReceiveSize ? size : maxReceiveSize, 
				0);
    
    if (nBytesRead > 0) {
      receiveBuffer.sizeOfData += nBytesRead;
      receiveBuffer.insertPtr  += nBytesRead;
      
      if(receiveBuffer.sizeOfData > receiveBuffer.sizeOfBuffer){
#ifdef DEBUG_TRANSPORTER
	g_eventLogger.error("receiveBuffer.sizeOfData(%d) > receiveBuffer.sizeOfBuffer(%d)",
		 receiveBuffer.sizeOfData, receiveBuffer.sizeOfBuffer);
	g_eventLogger.error("nBytesRead = %d", nBytesRead);
#endif
	g_eventLogger.error("receiveBuffer.sizeOfData(%d) > receiveBuffer.sizeOfBuffer(%d)",
		 receiveBuffer.sizeOfData, receiveBuffer.sizeOfBuffer);
	report_error(TE_INVALID_MESSAGE_LENGTH);
	return 0;
      }
      
      receiveCount ++;
      receiveSize  += nBytesRead;
      
      if(receiveCount == reportFreq){
	reportReceiveLen(get_callback_obj(), remoteNodeId, receiveCount, receiveSize);
	receiveCount = 0;
	receiveSize  = 0;
      }
      return nBytesRead;
    } else {
#if defined DEBUG_TRANSPORTER
      g_eventLogger.error("Receive Failure(disconnect==%d) to node = %d nBytesSent = %d "
	       "errno = %d strerror = %s",
	       DISCONNECT_ERRNO(InetErrno, nBytesRead),
	       remoteNodeId, nBytesRead, InetErrno, 
	       (char*)ndbstrerror(InetErrno));
#endif   
      if(DISCONNECT_ERRNO(InetErrno, nBytesRead)){
	// The remote node has closed down
	doDisconnect();
	report_disconnect(InetErrno);
      } 
    }
    return nBytesRead;
  } else {
    return 0;
  }
}

void
TCP_Transporter::disconnectImpl() {
  if(theSocket != NDB_INVALID_SOCKET){
    if(NDB_CLOSE_SOCKET(theSocket) < 0){
      report_error(TE_ERROR_CLOSING_SOCKET);
    }
  }
  
  // Empty send och receive buffers 
  receiveBuffer.clear();
  m_sendBuffer.emptyBuffer();

  theSocket = NDB_INVALID_SOCKET;
}
