/* Copyright (c) 2003-2007 MySQL AB, 2009 Sun Microsystems, Inc.
   Use is subject to license terms.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include <ndb_global.h>
#include <my_sys.h>
#include <my_pthread.h>

#ifdef HAVE_XFS_XFS_H
#include <xfs/xfs.h>
#endif

#include "AsyncFile.hpp"

#include <ErrorHandlingMacros.hpp>
#include <kernel_types.h>
#include <ndbd_malloc.hpp>
#include <NdbThread.h>
#include <signaldata/FsRef.hpp>
#include <signaldata/FsOpenReq.hpp>
#include <signaldata/FsReadWriteReq.hpp>

// use this to test broken pread code
//#define HAVE_BROKEN_PREAD 

#ifdef HAVE_BROKEN_PREAD
#undef HAVE_PWRITE
#undef HAVE_PREAD
#endif

#if defined NDB_WIN32
#else
// For readv and writev
#include <sys/uio.h> 
#endif

#ifndef NDB_WIN32
#include <dirent.h>
#endif

// Use this define if you want printouts from AsyncFile class
//#define DEBUG_ASYNCFILE

#ifdef DEBUG_ASYNCFILE
#include <NdbOut.hpp>
#define DEBUG(x) x
#define PRINT_ERRORANDFLAGS(f) printErrorAndFlags(f)
void printErrorAndFlags(Uint32 used_flags);
#else
#define DEBUG(x) 
#define PRINT_ERRORANDFLAGS(f)
#endif

// Define the size of the write buffer (for each thread)
#define WRITEBUFFERSIZE 262144

const char *actionName[] = {
  "open",
  "close",
  "closeRemove",
  "read",
  "readv",
  "write",
  "writev",
  "writeSync",
  "writevSync",
  "sync",
  "end" };

static int numAsyncFiles = 0;

extern "C" void * runAsyncFile(void* arg)
{
  ((AsyncFile*)arg)->run();
  return (NULL);
}

AsyncFile::AsyncFile(SimulatedBlock& fs) :
  theFileName(),
#ifdef NDB_WIN32
  hFile(INVALID_HANDLE_VALUE),
#else
  theFd(-1),
#endif
  theReportTo(0),
  theMemoryChannelPtr(NULL),
  m_fs(fs)
{
  m_page_ptr.setNull();
  m_current_request= m_last_request= 0;
  m_open_flags = 0;
}

void
AsyncFile::doStart() 
{
  // Stacksize for filesystem threads
#if !defined(DBUG_OFF) && defined (__hpux)
  // Empirical evidence indicates at least 32k
  const NDB_THREAD_STACKSIZE stackSize = 32768;
#else
  // Otherwise an 8k stack should be enough
  const NDB_THREAD_STACKSIZE stackSize = 8192;
#endif

  char buf[16];
  numAsyncFiles++;
  BaseString::snprintf(buf, sizeof(buf), "AsyncFile%d", numAsyncFiles);

  theStartMutexPtr = NdbMutex_Create();
  theStartConditionPtr = NdbCondition_Create();
  NdbMutex_Lock(theStartMutexPtr);
  theStartFlag = false;
  theThreadPtr = NdbThread_Create(runAsyncFile,
                                  (void**)this,
                                  stackSize,
                                  (char*)&buf,
                                  NDB_THREAD_PRIO_MEAN);
  if (theThreadPtr == 0)
    ERROR_SET(fatal, NDBD_EXIT_MEMALLOC, "","Could not allocate file system thread");

  NdbCondition_Wait(theStartConditionPtr,
                    theStartMutexPtr);    
  NdbMutex_Unlock(theStartMutexPtr);
  NdbMutex_Destroy(theStartMutexPtr);
  NdbCondition_Destroy(theStartConditionPtr);
}

AsyncFile::~AsyncFile() 
{
  void *status;
  Request request;
  request.action = Request::end;
  theMemoryChannelPtr->writeChannel( &request );
  NdbThread_WaitFor(theThreadPtr, &status);
  NdbThread_Destroy(&theThreadPtr);
  delete theMemoryChannelPtr;
}

void
AsyncFile::reportTo( MemoryChannel<Request> *reportTo )
{
  theReportTo = reportTo;
}

void AsyncFile::execute(Request* request) 
{
  theMemoryChannelPtr->writeChannel( request );
}

void
AsyncFile::run()
{
  Request *request;
  // Create theMemoryChannel in the thread that will wait for it
  NdbMutex_Lock(theStartMutexPtr);
  theMemoryChannelPtr = new MemoryChannel<Request>();
  theStartFlag = true;
  // Create write buffer for bigger writes
  theWriteBufferSize = WRITEBUFFERSIZE;
  theWriteBufferUnaligned = (char *) ndbd_malloc(theWriteBufferSize +
                                                 NDB_O_DIRECT_WRITE_ALIGNMENT-1);
  theWriteBuffer = (char *)
    (((UintPtr)theWriteBufferUnaligned + NDB_O_DIRECT_WRITE_ALIGNMENT - 1) &
     ~(UintPtr)(NDB_O_DIRECT_WRITE_ALIGNMENT - 1));

  NdbMutex_Unlock(theStartMutexPtr);
  NdbCondition_Signal(theStartConditionPtr);
  
  if (!theWriteBuffer) {
    DEBUG(ndbout_c("AsyncFile::writeReq, Failed allocating write buffer"));
    return;
  }//if

  while (1) {
    request = theMemoryChannelPtr->readChannel();
    if (!request) {
      DEBUG(ndbout_c("Nothing read from Memory Channel in AsyncFile"));
      endReq();
      return;
    }//if
    m_current_request= request;
    switch (request->action) {
    case Request:: open:
      openReq(request);
      break;
    case Request:: close:
      closeReq(request);
      break;
    case Request:: closeRemove:
      closeReq(request);
      removeReq(request);
      break;
    case Request:: readPartial:
    case Request:: read:
      readReq(request);
      break;
    case Request:: readv:
      readvReq(request);
      break;
    case Request:: write:
      writeReq(request);
      break;
    case Request:: writev:
      writevReq(request);
      break;
    case Request:: writeSync:
      writeReq(request);
      syncReq(request);
      break;
    case Request:: writevSync:
      writevReq(request);
      syncReq(request);
      break;
    case Request:: sync:
      syncReq(request);
      break;
    case Request:: append:
      appendReq(request);
      break;
    case Request:: append_synch:
      appendReq(request);
      syncReq(request);
      break;
    case Request::rmrf:
      rmrfReq(request, (char*)theFileName.c_str(), request->par.rmrf.own_directory);
      break;
    case Request:: end:
      if (theFd > 0)
        closeReq(request);
      endReq();
      return;
    default:
      DEBUG(ndbout_c("Invalid Request"));
      abort();
      break;
    }//switch
    m_last_request= request;
    m_current_request= 0;
    
    // No need to signal as ndbfs only uses tryRead
    theReportTo->writeChannelNoSignal(request);
  }//while
}//AsyncFile::run()

#ifdef O_DIRECT
static char g_odirect_readbuf[2*GLOBAL_PAGE_SIZE -1];
#endif

int
AsyncFile::check_odirect_write(Uint32 flags, int& new_flags, int mode)
{
  assert(new_flags & (O_CREAT | O_TRUNC));
#ifdef O_DIRECT
  int ret;
  char * bufptr = (char*)((UintPtr(g_odirect_readbuf)+(GLOBAL_PAGE_SIZE - 1)) & ~(GLOBAL_PAGE_SIZE - 1));
  while (((ret = ::write(theFd, bufptr, GLOBAL_PAGE_SIZE)) == -1) && 
         (errno == EINTR));
  if (ret == -1)
  {
    new_flags &= ~O_DIRECT;
    ndbout_c("%s Failed to write using O_DIRECT, disabling", 
             theFileName.c_str());
  }
  
  close(theFd);
  theFd = ::open(theFileName.c_str(), new_flags, mode);
  if (theFd == -1)
    return errno;
#endif

  return 0;
}

int
AsyncFile::check_odirect_read(Uint32 flags, int &new_flags, int mode)
{
#ifdef O_DIRECT
  int ret;
  char * bufptr = (char*)((UintPtr(g_odirect_readbuf)+(GLOBAL_PAGE_SIZE - 1)) & ~(GLOBAL_PAGE_SIZE - 1));
  while (((ret = ::read(theFd, bufptr, GLOBAL_PAGE_SIZE)) == -1) && 
         (errno == EINTR));
  if (ret == -1)
  {
    ndbout_c("%s Failed to read using O_DIRECT, disabling", 
             theFileName.c_str());
    goto reopen;
  }
  
  if(lseek(theFd, 0, SEEK_SET) != 0)
  {
    return errno;
  }
  
  if ((flags & FsOpenReq::OM_CHECK_SIZE) == 0)
  {
    struct stat buf;
    if ((fstat(theFd, &buf) == -1))
    {
      return errno;
    } 
    else if ((buf.st_size % GLOBAL_PAGE_SIZE) != 0)
    {
      ndbout_c("%s filesize not a multiple of %d, disabling O_DIRECT", 
               theFileName.c_str(), GLOBAL_PAGE_SIZE);
      goto reopen;
    }
  }
  
  return 0;
  
reopen:
  close(theFd);
  new_flags &= ~O_DIRECT;
  theFd = ::open(theFileName.c_str(), new_flags, mode);
  if (theFd == -1)
    return errno;  
#endif
  return 0;
}

void AsyncFile::openReq(Request* request)
{  
  m_auto_sync_freq = 0;
  m_write_wo_sync = 0;
  m_open_flags = request->par.open.flags;

  // for open.flags, see signal FSOPENREQ
#ifdef NDB_WIN32
  DWORD dwCreationDisposition;
  DWORD dwDesiredAccess = 0;
  DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_NO_BUFFERING;
  Uint32 flags = request->par.open.flags;
    
    // Convert file open flags from Solaris to Windows
  if ((flags & FsOpenReq::OM_CREATE) && (flags & FsOpenReq::OM_TRUNCATE)){
    dwCreationDisposition = CREATE_ALWAYS;
  } else if (flags & FsOpenReq::OM_TRUNCATE){
    dwCreationDisposition = TRUNCATE_EXISTING;
  } else if (flags & FsOpenReq::OM_CREATE){
    dwCreationDisposition = CREATE_NEW;
  } else {
    dwCreationDisposition = OPEN_EXISTING;
  }
  
  switch(flags & 3){
  case FsOpenReq::OM_READONLY:
    dwDesiredAccess = GENERIC_READ;
    break;
  case FsOpenReq::OM_WRITEONLY:
    dwDesiredAccess = GENERIC_WRITE;
    break;
  case FsOpenReq::OM_READWRITE:
    dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    break;
  default:
    request->error = 1000;
    break;
    return;
  }

  hFile = CreateFile(theFileName.c_str(), dwDesiredAccess, dwShareMode, 
                     0, dwCreationDisposition, dwFlagsAndAttributes, 0);
    
  if(INVALID_HANDLE_VALUE == hFile) {
    request->error = GetLastError();
    if(((ERROR_PATH_NOT_FOUND == request->error) || (ERROR_INVALID_NAME == request->error))
       && (flags & FsOpenReq::OM_CREATE)) {
      createDirectories();
      hFile = CreateFile(theFileName.c_str(), dwDesiredAccess, dwShareMode, 
                         0, dwCreationDisposition, dwFlagsAndAttributes, 0);
            
      if(INVALID_HANDLE_VALUE == hFile)
        request->error = GetLastError();
      else
        request->error = 0;
            
      return;
    }
  } 
  else {
    request->error = 0;
    return;
  }
#else
  Uint32 flags = request->par.open.flags;
  int new_flags = 0;

  // Convert file open flags from Solaris to Liux
  if (flags & FsOpenReq::OM_CREATE)
  {
    new_flags |= O_CREAT;
  }
  
  if (flags & FsOpenReq::OM_TRUNCATE){
#if 0
    if(Global_unlinkO_CREAT){
      unlink(theFileName.c_str());
    } else 
#endif
      new_flags |= O_TRUNC;
  }  

  if (flags & FsOpenReq::OM_AUTOSYNC)
  {
    m_auto_sync_freq = request->par.open.auto_sync_size;
  }

  if (flags & FsOpenReq::OM_APPEND){
    new_flags |= O_APPEND;
  }

  if (flags & FsOpenReq::OM_DIRECT) 
#ifdef O_DIRECT
  {
    new_flags |= O_DIRECT;
  }
#endif
  
  if ((flags & FsOpenReq::OM_SYNC) && ! (flags & FsOpenReq::OM_INIT))
  {
#ifdef O_SYNC
    new_flags |= O_SYNC;
#endif
  }

  const char * rw = "";
  switch(flags & 0x3){
  case FsOpenReq::OM_READONLY:
    rw = "r";
    new_flags |= O_RDONLY;
    break;
  case FsOpenReq::OM_WRITEONLY:
    rw = "w";
    new_flags |= O_WRONLY;
    break;
  case FsOpenReq::OM_READWRITE:
    rw = "rw";
    new_flags |= O_RDWR;
    break;
  default:
    request->error = 1000;
    break;
    return;
  }

  // allow for user to choose any permissionsa with umask
  const int mode = S_IRUSR | S_IWUSR |
	           S_IRGRP | S_IWGRP |
		   S_IROTH | S_IWOTH;
  if (flags & FsOpenReq::OM_CREATE_IF_NONE)
  {
    Uint32 tmp_flags = new_flags;
#ifdef O_DIRECT
    tmp_flags &= ~O_DIRECT;
#endif
    if ((theFd = ::open(theFileName.c_str(), tmp_flags, mode)) != -1) 
    {
      close(theFd);
      request->error = FsRef::fsErrFileExists;      
      return;
    }
    new_flags |= O_CREAT;
  }

no_odirect:
  if (-1 == (theFd = ::open(theFileName.c_str(), new_flags, mode))) 
  {
    PRINT_ERRORANDFLAGS(new_flags);
    if ((errno == ENOENT) && (new_flags & O_CREAT)) 
    {
      createDirectories();
      if (-1 == (theFd = ::open(theFileName.c_str(), new_flags, mode))) 
      {
#ifdef O_DIRECT
	if (new_flags & O_DIRECT)
	{
	  new_flags &= ~O_DIRECT;
	  goto no_odirect;
	}
#endif
        PRINT_ERRORANDFLAGS(new_flags);
        request->error = errno;
	return;
      }
    }
#ifdef O_DIRECT
    else if (new_flags & O_DIRECT)
    {
      new_flags &= ~O_DIRECT;
      goto no_odirect;
    }
#endif
    else
    {
      request->error = errno;
      return;
    }
  }
  
  if (flags & FsOpenReq::OM_CHECK_SIZE)
  {
    struct stat buf;
    if ((fstat(theFd, &buf) == -1))
    {
      request->error = errno;
    } 
    else if((Uint64)buf.st_size != request->par.open.file_size)
    {
      request->error = FsRef::fsErrInvalidFileSize;
    }
    if (request->error)
      return;
  }
  
  if (flags & FsOpenReq::OM_INIT)
  {
    off_t off = 0;
    const off_t sz = request->par.open.file_size;
    Uint32 tmp[sizeof(SignalHeader)+25];
    Signal * signal = (Signal*)(&tmp[0]);
    FsReadWriteReq* req = (FsReadWriteReq*)signal->getDataPtrSend();

    Uint32 index = 0;
    Uint32 block = refToBlock(request->theUserReference);

#ifdef HAVE_XFS_XFS_H
    if(platform_test_xfs_fd(theFd))
    {
      ndbout_c("Using xfsctl(XFS_IOC_RESVSP64) to allocate disk space");
      xfs_flock64_t fl;
      fl.l_whence= 0;
      fl.l_start= 0;
      fl.l_len= (off64_t)sz;
      if(xfsctl(NULL, theFd, XFS_IOC_RESVSP64, &fl) < 0)
        ndbout_c("failed to optimally allocate disk space");
    }
#endif
#ifdef HAVE_POSIX_FALLOCATE
    posix_fallocate(theFd, 0, sz);
#endif

    while(off < sz)
    {
      req->filePointer = 0;          // DATA 0
      req->userPointer = request->theUserPointer;          // DATA 2
      req->numberOfPages = 1;        // DATA 5  
      req->varIndex = index++;
      req->data.pageData[0] = m_page_ptr.i;

      m_fs.EXECUTE_DIRECT(block, GSN_FSWRITEREQ, signal, 
			  FsReadWriteReq::FixedLength + 1);
  retry:
      Uint32 size = request->par.open.page_size;
      char* buf = (char*)m_page_ptr.p;
      while(size > 0){
	const int n = write(theFd, buf, size);
	if(n == -1 && errno == EINTR)
	{
	  continue;
	}
	if(n == -1 || n == 0)
	{
	  break;
	}
	size -= n;
	buf += n;
      }
      if(size != 0)
      {
	int err = errno;
#ifdef O_DIRECT
	if ((new_flags & O_DIRECT) && off == 0)
	{
	  ndbout_c("error on first write(%d), disable O_DIRECT", err);
	  new_flags &= ~O_DIRECT;
	  close(theFd);
	  theFd = ::open(theFileName.c_str(), new_flags, mode);
	  if (theFd != -1)
	    goto retry;
	}
#endif
	close(theFd);
	unlink(theFileName.c_str());
	request->error = err;
	return;
      }
      off += request->par.open.page_size;
    }
    if(lseek(theFd, 0, SEEK_SET) != 0)
      request->error = errno;
  }
  else if (flags & FsOpenReq::OM_DIRECT)
  {
#ifdef O_DIRECT
    if (flags & (FsOpenReq::OM_TRUNCATE | FsOpenReq::OM_CREATE))
    {
      request->error = check_odirect_write(flags, new_flags, mode);
    }
    else
    {
      request->error = check_odirect_read(flags, new_flags, mode);
    }
    
    if (request->error)
      return;
#endif
  }
#ifdef VM_TRACE
  if (flags & FsOpenReq::OM_DIRECT)
  {
#ifdef O_DIRECT
    ndbout_c("%s %s O_DIRECT: %d",
             theFileName.c_str(), rw,
             !!(new_flags & O_DIRECT));
#else
    ndbout_c("%s %s O_DIRECT: 0",
             theFileName.c_str(), rw);
#endif
  }
#endif  
  if ((flags & FsOpenReq::OM_SYNC) && (flags & FsOpenReq::OM_INIT))
  {
#ifdef O_SYNC
    /**
     * reopen file with O_SYNC
     */
    close(theFd);
    new_flags &= ~(O_CREAT | O_TRUNC);
    new_flags |= O_SYNC;
    theFd = ::open(theFileName.c_str(), new_flags, mode);
    if (theFd == -1)
    {
      request->error = errno;
    }
#endif
  }
#endif
}

int
AsyncFile::readBuffer(Request* req, char * buf, size_t size, off_t offset){
  int return_value;
  req->par.readWrite.pages[0].size = 0;  
#ifdef NDB_WIN32
  DWORD dwSFP = SetFilePointer(hFile, offset, 0, FILE_BEGIN);
  if(dwSFP != offset) {
    return GetLastError();
  }
#elif ! defined(HAVE_PREAD)
  off_t seek_val;
  while((seek_val= lseek(theFd, offset, SEEK_SET)) == (off_t)-1 
	&& errno == EINTR);
  if(seek_val == (off_t)-1)
  {
    return errno;
  }
#endif
    
  while (size > 0) {
    size_t bytes_read = 0;
    
#ifdef NDB_WIN32
    DWORD dwBytesRead;
    BOOL bRead = ReadFile(hFile, 
                          buf,
                          size,
                          &dwBytesRead,
                          0);
    if(!bRead){
      return GetLastError();
    } 
    bytes_read = dwBytesRead;
#elif  ! defined(HAVE_PREAD)
    return_value = ::read(theFd, buf, size);
#else // UNIX
    return_value = ::pread(theFd, buf, size, offset);
#endif
#ifndef NDB_WIN32
    if (return_value == -1 && errno == EINTR) {
      DEBUG(ndbout_c("EINTR in read"));
      continue;
    } else if (return_value == -1){
      return errno;
    } else {
      bytes_read = return_value;
    }
#endif

    req->par.readWrite.pages[0].size += bytes_read;
    if(bytes_read == 0){
      if(req->action == Request::readPartial)
      {
	return 0;
      }
      DEBUG(ndbout_c("Read underflow %d %d\n %x\n%d %d", 
		     size, offset, buf, bytes_read, return_value));
      return ERR_ReadUnderflow;
    }
    
    if(bytes_read != size){
      DEBUG(ndbout_c("Warning partial read %d != %d", 
		     bytes_read, size));
    }
    
    buf += bytes_read;
    size -= bytes_read;
    offset += bytes_read;
  }
  return 0;
}

void
AsyncFile::readReq( Request * request)
{
  for(int i = 0; i < request->par.readWrite.numberOfPages ; i++) {
    off_t offset = request->par.readWrite.pages[i].offset;
    size_t size  = request->par.readWrite.pages[i].size;
    char * buf   = request->par.readWrite.pages[i].buf;
    
    int err = readBuffer(request, buf, size, offset);
    if(err != 0){
      request->error = err;
      return;
    }
  }
}

void
AsyncFile::readvReq( Request * request)
{
#if ! defined(HAVE_PREAD)
  readReq(request);
  return;
#elif defined NDB_WIN32
  // ReadFileScatter?
  readReq(request);
  return;
#else
  int return_value;
  int length = 0;
  struct iovec iov[20]; // the parameter in the signal restricts this to 20 deep
  for(int i=0; i < request->par.readWrite.numberOfPages ; i++) {
    iov[i].iov_base= request->par.readWrite.pages[i].buf;
    iov[i].iov_len= request->par.readWrite.pages[i].size;
    length = length + iov[i].iov_len;
  }
  lseek( theFd, request->par.readWrite.pages[0].offset, SEEK_SET );
  return_value = ::readv(theFd, iov, request->par.readWrite.numberOfPages);
  if (return_value == -1) {
    request->error = errno;
    return;
  } else if (return_value != length) {
    request->error = 1011;
    return;
  }
#endif
}

int 
AsyncFile::extendfile(Request* request) {
#if ! defined(HAVE_PWRITE)
  // Find max size of this file in this request
  int maxOffset = 0;
  int maxSize = 0;
  for(int i=0; i < request->par.readWrite.numberOfPages ; i++) {
    if (request->par.readWrite.pages[i].offset > maxOffset) {
      maxOffset = request->par.readWrite.pages[i].offset;
      maxSize = request->par.readWrite.pages[i].size;
    }
  }
  DEBUG(ndbout_c("extendfile: maxOffset=%d, size=%d", maxOffset, maxSize));

  // Allocate a buffer and fill it with zeros
  void* pbuf = ndbd_malloc(maxSize);
  memset(pbuf, 0, maxSize);
  for (int p = 0; p <= maxOffset; p = p + maxSize) {
    int return_value;
    return_value = lseek(theFd, 
                         p,
                         SEEK_SET);
    if((return_value == -1 ) || (return_value != p)) {
      ndbd_free(pbuf,maxSize);
      return -1;
    }
    return_value = ::write(theFd, 
                           pbuf,
                           maxSize);
    if ((return_value == -1) || (return_value != maxSize)) {
      ndbd_free(pbuf,maxSize);
      return -1;
    }
  }
  ndbd_free(pbuf,maxSize);
  
  DEBUG(ndbout_c("extendfile: \"%s\" OK!", theFileName.c_str()));
  return 0;
#else
  request = request;
  DEBUG(ndbout_c("no pwrite"));
  abort();
  return -1;
#endif
}

void
AsyncFile::writeReq( Request * request)
{
  int page_num = 0;
  bool write_not_complete = true;

  while(write_not_complete) {
    int totsize = 0;
    off_t offset = request->par.readWrite.pages[page_num].offset;
    char* bufptr = theWriteBuffer;
      
    write_not_complete = false;
    if (request->par.readWrite.numberOfPages > 1) {
      off_t page_offset = offset;

      // Multiple page write, copy to buffer for one write
      for(int i=page_num; i < request->par.readWrite.numberOfPages; i++) {
        memcpy(bufptr, 
               request->par.readWrite.pages[i].buf, 
               request->par.readWrite.pages[i].size);
        bufptr += request->par.readWrite.pages[i].size;
        totsize += request->par.readWrite.pages[i].size;
        if (((i + 1) < request->par.readWrite.numberOfPages)) {
          // There are more pages to write
          // Check that offsets are consequtive
	  off_t tmp = page_offset + request->par.readWrite.pages[i].size;
          if (tmp != request->par.readWrite.pages[i+1].offset) {
            // Next page is not aligned with previous, not allowed
            DEBUG(ndbout_c("Page offsets are not aligned"));
            request->error = EINVAL;
            return;
          }
          if ((unsigned)(totsize + request->par.readWrite.pages[i+1].size) > (unsigned)theWriteBufferSize) {
            // We are not finished and the buffer is full
            write_not_complete = true;
            // Start again with next page
            page_num = i + 1;
            break;
          }
        }
        page_offset += request->par.readWrite.pages[i].size;
      }
      bufptr = theWriteBuffer;
    } else { 
      // One page write, write page directly
      bufptr = request->par.readWrite.pages[0].buf;
      totsize = request->par.readWrite.pages[0].size;
    }
    int err = writeBuffer(bufptr, totsize, offset);
    if(err != 0){
      request->error = err;
      return;
    }
  } // while(write_not_complete)

  if(m_auto_sync_freq && m_write_wo_sync > m_auto_sync_freq){
    syncReq(request);
  }
}

int
AsyncFile::writeBuffer(const char * buf, size_t size, off_t offset, 
		       size_t chunk_size)
{
  size_t bytes_to_write = chunk_size;
  int return_value;

  m_write_wo_sync += size;

#ifdef NDB_WIN32
  DWORD dwSFP = SetFilePointer(hFile, offset, 0, FILE_BEGIN);
  if(dwSFP != offset) {
    return GetLastError();
  }
#elif ! defined(HAVE_PWRITE)
  off_t seek_val;
  while((seek_val= lseek(theFd, offset, SEEK_SET)) == (off_t)-1 
	&& errno == EINTR);
  if(seek_val == (off_t)-1)
  {
    return errno;
  }
#endif
    
  while (size > 0) {
    if (size < bytes_to_write){
      // We are at the last chunk
      bytes_to_write = size;
    }
    size_t bytes_written = 0;
    
#ifdef NDB_WIN32
    DWORD dwWritten;
    BOOL bWrite = WriteFile(hFile, buf, bytes_to_write, &dwWritten, 0);
    if(!bWrite) {
      return GetLastError();
    }
    bytes_written = dwWritten;
    if (bytes_written != bytes_to_write) {
      DEBUG(ndbout_c("Warning partial write %d != %d", bytes_written, bytes_to_write));
    }
    
#elif ! defined(HAVE_PWRITE)
    return_value = ::write(theFd, buf, bytes_to_write);
#else // UNIX
    return_value = ::pwrite(theFd, buf, bytes_to_write, offset);
#endif
#ifndef NDB_WIN32
    if (return_value == -1 && errno == EINTR) {
      bytes_written = 0;
      DEBUG(ndbout_c("EINTR in write"));
    } else if (return_value == -1){
      return errno;
    } else {
      bytes_written = return_value;

      if(bytes_written == 0){
        DEBUG(ndbout_c("no bytes written"));
	abort();
      }
      
      if(bytes_written != bytes_to_write){
	DEBUG(ndbout_c("Warning partial write %d != %d", 
		 bytes_written, bytes_to_write));
      }
    }
#endif
    
    buf += bytes_written;
    size -= bytes_written;
    offset += bytes_written;
  }
  return 0;
}

void
AsyncFile::writevReq( Request * request)
{
  // WriteFileGather on WIN32?
  writeReq(request);
}


void
AsyncFile::closeReq(Request * request)
{
  if (m_open_flags & (
      FsOpenReq::OM_WRITEONLY |
      FsOpenReq::OM_READWRITE |
      FsOpenReq::OM_APPEND )) {
    syncReq(request);
  }
#ifdef NDB_WIN32
  if(!CloseHandle(hFile)) {
    request->error = GetLastError();
  }
  hFile = INVALID_HANDLE_VALUE;
#else
  if (-1 == ::close(theFd)) {
#ifndef DBUG_OFF
    if (theFd == -1) {
      DEBUG(ndbout_c("close on fd = -1"));
      abort();
    }
#endif
    request->error = errno;
  }
  theFd = -1;
#endif
}

bool AsyncFile::isOpen(){
#ifdef NDB_WIN32
  return (hFile != INVALID_HANDLE_VALUE);
#else
  return (theFd != -1);
#endif
}


void
AsyncFile::syncReq(Request * request)
{
  if(m_auto_sync_freq && m_write_wo_sync == 0){
    return;
  }
#ifdef NDB_WIN32
  if(!FlushFileBuffers(hFile)) {
    request->error = GetLastError();
    return;
  }
#else
  if (-1 == ::fsync(theFd)){
    request->error = errno;
    return;
  }
#endif
  m_write_wo_sync = 0;
}

void
AsyncFile::appendReq(Request * request){
  
  const char * buf = request->par.append.buf;
  Uint32 size = request->par.append.size;

  m_write_wo_sync += size;

#ifdef NDB_WIN32
  DWORD dwWritten = 0;  
  while(size > 0){
    if(!WriteFile(hFile, buf, size, &dwWritten, 0)){
      request->error = GetLastError();
      return ;
    }
    
    buf += dwWritten;
    size -= dwWritten;
  }
#else
  while(size > 0){
    const int n = write(theFd, buf, size);
    if(n == -1 && errno == EINTR){
      continue;
    }
    if(n == -1){
      request->error = errno;
      return;
    }
    if(n == 0){
      DEBUG(ndbout_c("append with n=0"));
      abort();
    }
    size -= n;
    buf += n;
  }
#endif

  if(m_auto_sync_freq && m_write_wo_sync > m_auto_sync_freq){
    syncReq(request);
  }
}

void
AsyncFile::removeReq(Request * request)
{
#ifdef NDB_WIN32
  if(!DeleteFile(theFileName.c_str())) {
    request->error = GetLastError();
  }
#else
  if (-1 == ::remove(theFileName.c_str())) {
    request->error = errno;

  }
#endif
}

void
AsyncFile::rmrfReq(Request * request, char * path, bool removePath){
  Uint32 path_len = strlen(path);
  Uint32 path_max_copy = PATH_MAX - path_len;
  char* path_add = &path[path_len];
#ifndef NDB_WIN32
  if(!request->par.rmrf.directory){
    // Remove file
    if(unlink((const char *)path) != 0 && errno != ENOENT)
      request->error = errno;
    return;
  }
  // Remove directory 
  DIR* dirp = opendir((const char *)path);
  if(dirp == 0){
    if(errno != ENOENT) 
      request->error = errno;
    return;
  }
  struct dirent * dp;
  while ((dp = readdir(dirp)) != NULL){
    if ((strcmp(".", dp->d_name) != 0) && (strcmp("..", dp->d_name) != 0)) {
      BaseString::snprintf(path_add, (size_t)path_max_copy, "%s%s",
	       DIR_SEPARATOR, dp->d_name);
      if(remove((const char*)path) == 0){
        path[path_len] = 0;
	continue;
      }
      
      rmrfReq(request, path, true);
      path[path_len] = 0;
      if(request->error != 0){
	closedir(dirp);
	return;
      }
    }
  }
  closedir(dirp);
  if(removePath && rmdir((const char *)path) != 0){
    request->error = errno;
  }
  return;
#else

  if(!request->par.rmrf.directory){
    // Remove file
    if(!DeleteFile(path)){
      DWORD dwError = GetLastError();
      if(dwError!=ERROR_FILE_NOT_FOUND)
	request->error = dwError;
    }
    return;
  }
  
  strcat(path, "\\*");
  WIN32_FIND_DATA ffd;
  HANDLE hFindFile = FindFirstFile(path, &ffd);
  path[path_len] = 0;
  if(INVALID_HANDLE_VALUE==hFindFile){
    DWORD dwError = GetLastError();
    if(dwError!=ERROR_PATH_NOT_FOUND)
      request->error = dwError;
    return;
  }
  
  do {
    if(0!=strcmp(".", ffd.cFileName) && 0!=strcmp("..", ffd.cFileName)){
      strcat(path, "\\");
      strcat(path, ffd.cFileName);
      if(DeleteFile(path)) {
        path[path_len] = 0;
	continue;
      }//if
      
      rmrfReq(request, path, true);
      path[path_len] = 0;
      if(request->error != 0){
	FindClose(hFindFile);
	return;
      }
    }
  } while(FindNextFile(hFindFile, &ffd));
  
  FindClose(hFindFile);
  
  if(removePath && !RemoveDirectory(path))
    request->error = GetLastError();
  
#endif
}

void AsyncFile::endReq()
{
  // Thread is ended with return
  if (theWriteBufferUnaligned)
    ndbd_free(theWriteBufferUnaligned, theWriteBufferSize);
}


void AsyncFile::createDirectories()
{
  char* tmp;
  const char * name = theFileName.c_str();
  const char * base = theFileName.get_base_name();
  while((tmp = (char *)strstr(base, DIR_SEPARATOR)))
  {
    char t = tmp[0];
    tmp[0] = 0;
#ifdef NDB_WIN32
    CreateDirectory(name, 0);
#else
    mkdir(name, S_IRUSR | S_IWUSR | S_IXUSR | S_IXGRP | S_IRGRP);
#endif
    tmp[0] = t;
    base = tmp + sizeof(DIR_SEPARATOR);
  }
}

#ifdef DEBUG_ASYNCFILE
void printErrorAndFlags(Uint32 used_flags) {
  char buf[255];
  sprintf(buf, "PEAF: errno=%d \"", errno);

  switch(errno) {
  case     EACCES:
    strcat(buf, "EACCES");
    break;
  case     EDQUOT: 
    strcat(buf, "EDQUOT");
    break;
  case     EEXIST    :
    strcat(buf, "EEXIST");
    break;
  case     EINTR     :
    strcat(buf, "EINTR");
    break;
  case     EFAULT    :
    strcat(buf, "EFAULT");
    break;
  case     EIO       :
    strcat(buf, "EIO");
    break;
  case     EISDIR    :
    strcat(buf, "EISDIR");
    break;
  case     ELOOP       :
    strcat(buf, "ELOOP");
    break;
  case     EMFILE   :
    strcat(buf, "EMFILE");
    break;
  case     ENFILE    :
    strcat(buf, "ENFILE");
    break;
  case     ENOENT    :
    strcat(buf, "ENOENT ");
    break;
  case     ENOSPC    :
    strcat(buf, "ENOSPC");
    break;
  case     ENOTDIR  :
    strcat(buf, "ENOTDIR");
    break;
  case     ENXIO     : 
    strcat(buf, "ENXIO");
    break;  
  case     EOPNOTSUPP:
    strcat(buf, "EOPNOTSUPP");
    break;
  case     EMULTIHOP :
    strcat(buf, "EMULTIHOP");
    break;
  case     ENOLINK    :
    strcat(buf, "ENOLINK");
    break;
  case     ENOSR      :
    strcat(buf, "ENOSR");
    break;      
  case     EOVERFLOW :
    strcat(buf,  "EOVERFLOW");
    break;
  case     EROFS    :
    strcat(buf,  "EROFS");
    break;
  case     EAGAIN :   
    strcat(buf,  "EAGAIN");
    break;
  case     EINVAL    :
    strcat(buf,  "EINVAL");
    break;
  case     ENOMEM    :
    strcat(buf, "ENOMEM");
    break;
  case     ETXTBSY   :
    strcat(buf, "ETXTBSY");
    break;
  case ENAMETOOLONG:
    strcat(buf, "ENAMETOOLONG");
    break;
  case EBADF:
    strcat(buf, "EBADF");
    break;
  case ESPIPE:
    strcat(buf, "ESPIPE");
    break;
  case ESTALE:
    strcat(buf, "ESTALE");
    break;
  default:  
    strcat(buf, "EOTHER");
    break;
  }
  strcat(buf, "\" ");
  strcat(buf, " flags: ");
  switch(used_flags & 3){
  case O_RDONLY:
    strcat(buf, "O_RDONLY, ");
    break;
  case O_WRONLY:
    strcat(buf, "O_WRONLY, ");
    break;
  case O_RDWR:
    strcat(buf, "O_RDWR, ");
    break;
  default:
    strcat(buf, "Unknown!!, ");
  }

  if((used_flags & O_APPEND)==O_APPEND)
    strcat(buf, "O_APPEND, ");
  if((used_flags & O_CREAT)==O_CREAT)
    strcat(buf, "O_CREAT, ");
  if((used_flags & O_EXCL)==O_EXCL)
    strcat(buf, "O_EXCL, ");
  if((used_flags & O_NOCTTY) == O_NOCTTY)
    strcat(buf, "O_NOCTTY, ");
  if((used_flags & O_NONBLOCK)==O_NONBLOCK)
    strcat(buf, "O_NONBLOCK, ");
  if((used_flags & O_TRUNC)==O_TRUNC)
    strcat(buf, "O_TRUNC, ");
  if((used_flags & O_DSYNC)==O_DSYNC)
    strcat(buf, "O_DSYNC, ");
  if((used_flags & O_NDELAY)==O_NDELAY)
    strcat(buf, "O_NDELAY, ");
  if((used_flags & O_RSYNC)==O_RSYNC)
    strcat(buf, "O_RSYNC, ");
#ifdef O_SYNC
  if((used_flags & O_SYNC)==O_SYNC)
    strcat(buf, "O_SYNC, ");
#endif
  DEBUG(ndbout_c(buf));

}
#endif

NdbOut&
operator<<(NdbOut& out, const Request& req)
{
  out << "[ Request: file: " << hex << req.file 
      << " userRef: " << hex << req.theUserReference
      << " userData: " << dec << req.theUserPointer
      << " theFilePointer: " << req.theFilePointer
      << " action: ";
  switch(req.action){
  case Request::open:
    out << "open";
    break;
  case Request::close:
    out << "close";
    break;
  case Request::closeRemove:
    out << "closeRemove";
    break;
  case Request::read:   // Allways leave readv directly after 
    out << "read";
    break;
  case Request::readv:
    out << "readv";
    break;
  case Request::write:// Allways leave writev directly after 
    out << "write";
    break;
  case Request::writev:
    out << "writev";
    break;
  case Request::writeSync:// Allways leave writevSync directly after 
    out << "writeSync";
    break;
    // writeSync because SimblockAsyncFileSystem depends on it
  case Request::writevSync:
    out << "writevSync";
    break;
  case Request::sync:
    out << "sync";
    break;
  case Request::end:
    out << "end";
    break;
  case Request::append:
    out << "append";
    break;
  case Request::rmrf:
    out << "rmrf";
    break;
  default:
    out << (Uint32)req.action;
    break;
  }
  out << " ]";
  return out;
}
