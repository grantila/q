/*
 * Copyright 2016 Gustaf Räntilä
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBQ_EXCEPTION_EXCEPTION_ERRNO_HPP
#define LIBQ_EXCEPTION_EXCEPTION_ERRNO_HPP

#include <memory>

#include <errno.h>

namespace q {

Q_MAKE_SIMPLE_EXCEPTION( errno_exception );
Q_MAKE_SIMPLE_EXCEPTION( non_existent_errno_exception );

#define Q_MAKE_ERRNO_EXCEPTION( Name ) \
	class Name \
	: public ::q::errno_exception \
	{ \
		virtual const char* name( ) const noexcept override \
		{ \
			return #Name ; \
		} \
		using ::q::errno_exception::errno_exception; \
	}

#define Q_MAKE_NON_EXISTENT_ERRNO_EXCEPTION( Name ) \
	class Name \
	: public ::q::non_existent_errno_exception \
	{ \
		virtual const char* name( ) const noexcept override \
		{ \
			return #Name ; \
		} \
		using ::q::non_existent_errno_exception \
			::non_existent_errno_exception; \
	}

std::exception_ptr get_exception_by_errno( int errno_ );

[[noreturn]] void throw_by_errno( int errno_ );

#ifndef Q__DEFINE_ERRNO_EXCEPTION_IMPL_
#	define Q__DEFINE_ERRNO_EXCEPTION_IMPL_( Errno, Name )
#endif

#define Q__DEFINE_ERRNO_EXCEPTION_( Errno, Name ) \
	Q_MAKE_ERRNO_EXCEPTION( errno_ ## Name ## _exception ); \
	Q__DEFINE_ERRNO_EXCEPTION_IMPL_( \
		Errno, errno_ ## Name ## _exception )

#define Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( Name ) \
	Q_MAKE_NON_EXISTENT_ERRNO_EXCEPTION( errno_ ## Name ## _exception );

// These are errno values combined from various platforms
// TODO: Go through them and make the names nicer

#ifdef EPERM // 1: Operation not permitted
	Q__DEFINE_ERRNO_EXCEPTION_( EPERM, perm );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( perm );
#endif

#ifdef ENOENT // 2: No such file or directory
	Q__DEFINE_ERRNO_EXCEPTION_( ENOENT, noent );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( noent );
#endif

#ifdef ESRCH // 3: No such process
	Q__DEFINE_ERRNO_EXCEPTION_( ESRCH, srch );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( srch );
#endif

#ifdef EINTR // 4: Interrupted system call
	Q__DEFINE_ERRNO_EXCEPTION_( EINTR, intr );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( intr );
#endif

#ifdef EIO // 5: Input/output error
	Q__DEFINE_ERRNO_EXCEPTION_( EIO, io );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( io );
#endif

#ifdef ENXIO // 6: Device not configured
	Q__DEFINE_ERRNO_EXCEPTION_( ENXIO, nxio );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nxio );
#endif

#ifdef E2BIG // 7: Argument list too long
	Q__DEFINE_ERRNO_EXCEPTION_( E2BIG, 2big );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( 2big );
#endif

#ifdef ENOEXEC // 8: Exec format error
	Q__DEFINE_ERRNO_EXCEPTION_( ENOEXEC, noexec );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( noexec );
#endif

#ifdef EBADF // 9: Bad file descriptor
	Q__DEFINE_ERRNO_EXCEPTION_( EBADF, badf );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( badf );
#endif

#ifdef ECHILD // 10: No child processes
	Q__DEFINE_ERRNO_EXCEPTION_( ECHILD, child );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( child );
#endif

#ifdef EDEADLK // 11: Resource deadlock avoided (is EAGAIN on Linux)
	Q__DEFINE_ERRNO_EXCEPTION_( EDEADLK, deadlk );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( deadlk );
#endif

#ifdef ENOMEM // 12: Cannot allocate memory
	Q__DEFINE_ERRNO_EXCEPTION_( ENOMEM, nomem );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nomem );
#endif

#ifdef EACCES // 13: Permission denied
	Q__DEFINE_ERRNO_EXCEPTION_( EACCES, acces );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( acces );
#endif

#ifdef EFAULT // 14: Bad address
	Q__DEFINE_ERRNO_EXCEPTION_( EFAULT, fault );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( fault );
#endif

#ifdef ENOTBLK // 15: Block device required
	Q__DEFINE_ERRNO_EXCEPTION_( ENOTBLK, notblk );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( notblk );
#endif

#ifdef EBUSY // 16: Device / Resource busy
	Q__DEFINE_ERRNO_EXCEPTION_( EBUSY, busy );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( busy );
#endif

#ifdef EEXIST // 17: File exists
	Q__DEFINE_ERRNO_EXCEPTION_( EEXIST, exist );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( exist );
#endif

#ifdef EXDEV // 18: Cross-device link
	Q__DEFINE_ERRNO_EXCEPTION_( EXDEV, xdev );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( xdev );
#endif

#ifdef ENODEV // 19: Operation not supported by device
	Q__DEFINE_ERRNO_EXCEPTION_( ENODEV, nodev );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nodev );
#endif

#ifdef ENOTDIR // 20: Not a directory
	Q__DEFINE_ERRNO_EXCEPTION_( ENOTDIR, notdir );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( notdir );
#endif

#ifdef EISDIR // 21: Is a directory
	Q__DEFINE_ERRNO_EXCEPTION_( EISDIR, isdir );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( isdir );
#endif

#ifdef EINVAL // 22: Invalid argument
	Q__DEFINE_ERRNO_EXCEPTION_( EINVAL, inval );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( inval );
#endif

#ifdef ENFILE // 23: Too many open files in system
	Q__DEFINE_ERRNO_EXCEPTION_( ENFILE, nfile );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nfile );
#endif

#ifdef EMFILE // 24: Too many open files
	Q__DEFINE_ERRNO_EXCEPTION_( EMFILE, mfile );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( mfile );
#endif

#ifdef ENOTTY // 25: Inappropriate ioctl for device
	Q__DEFINE_ERRNO_EXCEPTION_( ENOTTY, notty );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( notty );
#endif

#ifdef ETXTBSY // 26: Text file busy
	Q__DEFINE_ERRNO_EXCEPTION_( ETXTBSY, txtbsy );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( txtbsy );
#endif

#ifdef EFBIG // 27: File too large
	Q__DEFINE_ERRNO_EXCEPTION_( EFBIG, fbig );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( fbig );
#endif

#ifdef ENOSPC // 28: No space left on device
	Q__DEFINE_ERRNO_EXCEPTION_( ENOSPC, nospc );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nospc );
#endif

#ifdef ESPIPE // 29: Illegal seek
	Q__DEFINE_ERRNO_EXCEPTION_( ESPIPE, spipe );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( spipe );
#endif

#ifdef EROFS // 30: Read-only file system
	Q__DEFINE_ERRNO_EXCEPTION_( EROFS, rofs );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( rofs );
#endif

#ifdef EMLINK // 31: Too many links
	Q__DEFINE_ERRNO_EXCEPTION_( EMLINK, mlink );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( mlink );
#endif

#ifdef EPIPE // 32: Broken pipe
	Q__DEFINE_ERRNO_EXCEPTION_( EPIPE, pipe );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( pipe );
#endif

#ifdef EDOM // 33: Numerical argument out of domain
	Q__DEFINE_ERRNO_EXCEPTION_( EDOM, dom );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( dom );
#endif

#ifdef ERANGE // 34: Result too large
	Q__DEFINE_ERRNO_EXCEPTION_( ERANGE, range );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( range );
#endif

// End of base errno's

#ifdef EAGAIN // 35: Resource temporarily unavailable (is EDEADLK on Linux)
	Q__DEFINE_ERRNO_EXCEPTION_( EAGAIN, again );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( again );
#endif

#ifdef EINPROGRESS // 36: Operation now in progress
	Q__DEFINE_ERRNO_EXCEPTION_( EINPROGRESS, inprogress );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( inprogress );
#endif

#ifdef EALREADY // 37: Operation already in progress
	Q__DEFINE_ERRNO_EXCEPTION_( EALREADY, already );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( already );
#endif

#ifdef ENOTSOCK // 38: Socket operation on non-socket
	Q__DEFINE_ERRNO_EXCEPTION_( ENOTSOCK, notsock );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( notsock );
#endif

#ifdef EDESTADDRREQ // 39: Destination address required
	Q__DEFINE_ERRNO_EXCEPTION_( EDESTADDRREQ, destaddrreq );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( destaddrreq );
#endif

#ifdef EMSGSIZE // 40: Message too long
	Q__DEFINE_ERRNO_EXCEPTION_( EMSGSIZE, msgsize );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( msgsize );
#endif

#ifdef EPROTOTYPE // 41: Protocol wrong type for socket
	Q__DEFINE_ERRNO_EXCEPTION_( EPROTOTYPE, prototype );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( prototype );
#endif

#ifdef ENOPROTOOPT // 42: Protocol not available
	Q__DEFINE_ERRNO_EXCEPTION_( ENOPROTOOPT, noprotoopt );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( noprotoopt );
#endif

#ifdef EPROTONOSUPPORT // 43: Protocol not supported
	Q__DEFINE_ERRNO_EXCEPTION_( EPROTONOSUPPORT, protonosupport );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( protonosupport );
#endif

#ifdef ESOCKTNOSUPPORT // 44: Socket type not supported
	Q__DEFINE_ERRNO_EXCEPTION_( ESOCKTNOSUPPORT, socktnosupport );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( socktnosupport );
#endif

#ifdef ENOTSUP // 45: Operation not supported
	Q__DEFINE_ERRNO_EXCEPTION_( ENOTSUP, notsup );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( notsup );
#endif

#ifdef EPFNOSUPPORT // 46: Protocol family not supported
	Q__DEFINE_ERRNO_EXCEPTION_( EPFNOSUPPORT, pfnosupport );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( pfnosupport );
#endif

#ifdef EAFNOSUPPORT // 47: Address family not supported by protocol family
	Q__DEFINE_ERRNO_EXCEPTION_( EAFNOSUPPORT, afnosupport );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( afnosupport );
#endif

#ifdef EADDRINUSE // 48: Address already in use
	Q__DEFINE_ERRNO_EXCEPTION_( EADDRINUSE, addrinuse );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( addrinuse );
#endif

#ifdef EADDRNOTAVAIL // 49: Can't assign requested address
	Q__DEFINE_ERRNO_EXCEPTION_( EADDRNOTAVAIL, addrnotavail );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( addrnotavail );
#endif

#ifdef ENETDOWN // 50: Network is down
	Q__DEFINE_ERRNO_EXCEPTION_( ENETDOWN, netdown );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( netdown );
#endif

#ifdef ENETUNREACH // 51: Network is unreachable
	Q__DEFINE_ERRNO_EXCEPTION_( ENETUNREACH, netunreach );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( netunreach );
#endif

#ifdef ENETRESET // 52: Network dropped connection on reset
	Q__DEFINE_ERRNO_EXCEPTION_( ENETRESET, netreset );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( netreset );
#endif

#ifdef ECONNABORTED // 53: Software caused connection abort
	Q__DEFINE_ERRNO_EXCEPTION_( ECONNABORTED, connaborted );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( connaborted );
#endif

#ifdef ECONNRESET // 54: Connection reset by peer
	Q__DEFINE_ERRNO_EXCEPTION_( ECONNRESET, connreset );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( connreset );
#endif

#ifdef ENOBUFS // 55: No buffer space available
	Q__DEFINE_ERRNO_EXCEPTION_( ENOBUFS, nobufs );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nobufs );
#endif

#ifdef EISCONN // 56: Socket is already connected
	Q__DEFINE_ERRNO_EXCEPTION_( EISCONN, isconn );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( isconn );
#endif

#ifdef ENOTCONN // 57: Socket is not connected
	Q__DEFINE_ERRNO_EXCEPTION_( ENOTCONN, notconn );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( notconn );
#endif

#ifdef ESHUTDOWN // 58: Can't send after socket shutdown
	Q__DEFINE_ERRNO_EXCEPTION_( ESHUTDOWN, shutdown );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( shutdown );
#endif

#ifdef ETOOMANYREFS // 59: Too many references: can't splice
	Q__DEFINE_ERRNO_EXCEPTION_( ETOOMANYREFS, toomanyrefs );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( toomanyrefs );
#endif

#ifdef ETIMEDOUT // 60: Operation timed out
	Q__DEFINE_ERRNO_EXCEPTION_( ETIMEDOUT, timedout );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( timedout );
#endif

#ifdef ECONNREFUSED // 61: Connection refused
	Q__DEFINE_ERRNO_EXCEPTION_( ECONNREFUSED, connrefused );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( connrefused );
#endif

#ifdef ELOOP // 62: Too many levels of symbolic links
	Q__DEFINE_ERRNO_EXCEPTION_( ELOOP, loop );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( loop );
#endif

#ifdef ENAMETOOLONG // 63: File name too long
	Q__DEFINE_ERRNO_EXCEPTION_( ENAMETOOLONG, nametoolong );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nametoolong );
#endif

#ifdef EHOSTDOWN // 64: Host is down
	Q__DEFINE_ERRNO_EXCEPTION_( EHOSTDOWN, hostdown );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( hostdown );
#endif

#ifdef EHOSTUNREACH // 65: No route to host
	Q__DEFINE_ERRNO_EXCEPTION_( EHOSTUNREACH, hostunreach );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( hostunreach );
#endif

#ifdef ENOTEMPTY // 66: Directory not empty
	Q__DEFINE_ERRNO_EXCEPTION_( ENOTEMPTY, notempty );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( notempty );
#endif

#ifdef EPROCLIM // 67: Too many processes
	Q__DEFINE_ERRNO_EXCEPTION_( EPROCLIM, proclim );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( proclim );
#endif

#ifdef EUSERS // 68: Too many users
	Q__DEFINE_ERRNO_EXCEPTION_( EUSERS, users );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( users );
#endif

#ifdef EDQUOT // 69: Disc quota exceeded
	Q__DEFINE_ERRNO_EXCEPTION_( EDQUOT, dquot );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( dquot );
#endif

#ifdef ESTALE // 70: Stale NFS file handle
	Q__DEFINE_ERRNO_EXCEPTION_( ESTALE, stale );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( stale );
#endif

#ifdef EREMOTE // 71: Too many levels of remote in path
	Q__DEFINE_ERRNO_EXCEPTION_( EREMOTE, remote );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( remote );
#endif

#ifdef EBADRPC // 72: RPC struct is bad
	Q__DEFINE_ERRNO_EXCEPTION_( EBADRPC, badrpc );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( badrpc );
#endif

#ifdef ERPCMISMATCH // 73: RPC version wrong
	Q__DEFINE_ERRNO_EXCEPTION_( ERPCMISMATCH, rpcmismatch );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( rpcmismatch );
#endif

#ifdef EPROGUNAVAIL // 74: RPC prog. not avail
	Q__DEFINE_ERRNO_EXCEPTION_( EPROGUNAVAIL, progunavail );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( progunavail );
#endif

#ifdef EPROGMISMATCH // 75: Program version wrong
	Q__DEFINE_ERRNO_EXCEPTION_( EPROGMISMATCH, progmismatch );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( progmismatch );
#endif

#ifdef EPROCUNAVAIL // 76: Bad procedure for program
	Q__DEFINE_ERRNO_EXCEPTION_( EPROCUNAVAIL, procunavail );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( procunavail );
#endif

#ifdef ENOLCK // 77: No locks available
	Q__DEFINE_ERRNO_EXCEPTION_( ENOLCK, nolck );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nolck );
#endif

#ifdef ENOSYS // 78: Function not implemented
	Q__DEFINE_ERRNO_EXCEPTION_( ENOSYS, nosys );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nosys );
#endif

#ifdef EFTYPE // 79: Inappropriate file type or format
	Q__DEFINE_ERRNO_EXCEPTION_( EFTYPE, ftype );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( ftype );
#endif

#ifdef EAUTH // 80: Authentication error
	Q__DEFINE_ERRNO_EXCEPTION_( EAUTH, auth );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( auth );
#endif

#ifdef ENEEDAUTH // 81: Need authenticator
	Q__DEFINE_ERRNO_EXCEPTION_( ENEEDAUTH, needauth );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( needauth );
#endif

#ifdef EPWROFF // 82: Device power is off
	Q__DEFINE_ERRNO_EXCEPTION_( EPWROFF, pwroff );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( pwroff );
#endif

#ifdef EDEVERR // 83: Device error, e.g. paper out
	Q__DEFINE_ERRNO_EXCEPTION_( EDEVERR, deverr );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( deverr );
#endif

#ifdef EOVERFLOW // 84: Value too large to be stored in data type
	Q__DEFINE_ERRNO_EXCEPTION_( EOVERFLOW, overflow );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( overflow );
#endif

#ifdef EBADEXEC // 85: Bad executable
	Q__DEFINE_ERRNO_EXCEPTION_( EBADEXEC, badexec );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( badexec );
#endif

#ifdef EBADARCH // 86: Bad CPU type in executable
	Q__DEFINE_ERRNO_EXCEPTION_( EBADARCH, badarch );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( badarch );
#endif

#ifdef ESHLIBVERS // 87: Shared library version mismatch
	Q__DEFINE_ERRNO_EXCEPTION_( ESHLIBVERS, shlibvers );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( shlibvers );
#endif

#ifdef EBADMACHO // 88: Malformed Macho file
	Q__DEFINE_ERRNO_EXCEPTION_( EBADMACHO, badmacho );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( badmacho );
#endif

#ifdef ECANCELED // 89: Operation canceled
	Q__DEFINE_ERRNO_EXCEPTION_( ECANCELED, canceled );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( canceled );
#endif

#ifdef EIDRM // 90: Identifier removed
	Q__DEFINE_ERRNO_EXCEPTION_( EIDRM, idrm );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( idrm );
#endif

#ifdef ENOMSG // 91: No message of desired type */
	Q__DEFINE_ERRNO_EXCEPTION_( ENOMSG, nomsg );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nomsg );
#endif

#ifdef EILSEQ // 92: Illegal byte sequence
	Q__DEFINE_ERRNO_EXCEPTION_( EILSEQ, ilseq );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( ilseq );
#endif

#ifdef ENOATTR // 93: Attribute not found
	Q__DEFINE_ERRNO_EXCEPTION_( ENOATTR, noattr );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( noattr );
#endif

#ifdef EBADMSG // 94: Bad message
	Q__DEFINE_ERRNO_EXCEPTION_( EBADMSG, badmsg );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( badmsg );
#endif

#ifdef EMULTIHOP // 95: Reserved
	Q__DEFINE_ERRNO_EXCEPTION_( EMULTIHOP, multihop );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( multihop );
#endif

#ifdef ENODATA // 96: No message available on STREAM
	Q__DEFINE_ERRNO_EXCEPTION_( ENODATA, nodata );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nodata );
#endif

#ifdef ENOLINK // 97: Reserved
	Q__DEFINE_ERRNO_EXCEPTION_( ENOLINK, nolink );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nolink );
#endif

#ifdef ENOSR // 98: No STREAM resources
	Q__DEFINE_ERRNO_EXCEPTION_( ENOSR, nosr );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nosr );
#endif

#ifdef ENOSTR // 99: Not a STREAM
	Q__DEFINE_ERRNO_EXCEPTION_( ENOSTR, nostr );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nostr );
#endif

#ifdef EPROTO // 100: Protocol error
	Q__DEFINE_ERRNO_EXCEPTION_( EPROTO, proto );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( proto );
#endif

#ifdef ETIME // 101: STREAM ioctl timeout
	Q__DEFINE_ERRNO_EXCEPTION_( ETIME, time );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( time );
#endif

#ifdef EOPNOTSUPP // 102: Operation not supported on socket
	Q__DEFINE_ERRNO_EXCEPTION_( EOPNOTSUPP, opnotsupp );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( opnotsupp );
#endif

#ifdef ENOPOLICY // 103: No such policy registered
	Q__DEFINE_ERRNO_EXCEPTION_( ENOPOLICY, nopolicy );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nopolicy );
#endif

#ifdef ENOTRECOVERABLE // 104: State not recoverable
	Q__DEFINE_ERRNO_EXCEPTION_( ENOTRECOVERABLE, notrecoverable );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( notrecoverable );
#endif

#ifdef EOWNERDEAD // 105: Previous owner died
	Q__DEFINE_ERRNO_EXCEPTION_( EOWNERDEAD, ownerdead );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( ownerdead );
#endif

#ifdef EQFULL // 106: Interface output queue is full
	Q__DEFINE_ERRNO_EXCEPTION_( EQFULL, qfull );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( qfull );
#endif

// Linux-specific:

#ifdef ECHRNG // 44: Channel number out of range
	Q__DEFINE_ERRNO_EXCEPTION_( ECHRNG, chrng );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( chrng );
#endif

#ifdef EL2NSYNC // 45: Level 2 not synchronized
	Q__DEFINE_ERRNO_EXCEPTION_( EL2NSYNC, l2nsync );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( l2nsync );
#endif

#ifdef EL3HLT // 46: Level 3 halted
	Q__DEFINE_ERRNO_EXCEPTION_( EL3HLT, l3hlt );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( l3hlt );
#endif

#ifdef EL3RST // 47: Level 3 reset
	Q__DEFINE_ERRNO_EXCEPTION_( EL3RST, l3rst );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( l3rst );
#endif

#ifdef ELNRNG // 48: Link number out of range
	Q__DEFINE_ERRNO_EXCEPTION_( ELNRNG, lnrng );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( lnrng );
#endif

#ifdef EUNATCH // 49: Protocol driver not attached
	Q__DEFINE_ERRNO_EXCEPTION_( EUNATCH, unatch );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( unatch );
#endif

#ifdef ENOCSI // 50: No CSI structure available
	Q__DEFINE_ERRNO_EXCEPTION_( ENOCSI, nocsi );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nocsi );
#endif

#ifdef EL2HLT // 51: Level 2 halted
	Q__DEFINE_ERRNO_EXCEPTION_( EL2HLT, l2hlt );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( l2hlt );
#endif

#ifdef EBADE // 52: Invalid exchange
	Q__DEFINE_ERRNO_EXCEPTION_( EBADE, bade );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( bade );
#endif

#ifdef EBADR // 53: Invalid request descriptor
	Q__DEFINE_ERRNO_EXCEPTION_( EBADR, badr );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( badr );
#endif

#ifdef EXFULL // 54: Exchange full
	Q__DEFINE_ERRNO_EXCEPTION_( EXFULL, xfull );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( xfull );
#endif

#ifdef ENOANO // 55: No anode
	Q__DEFINE_ERRNO_EXCEPTION_( ENOANO, noano );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( noano );
#endif

#ifdef EBADRQC // 56: Invalid request code
	Q__DEFINE_ERRNO_EXCEPTION_( EBADRQC, badrqc );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( badrqc );
#endif

#ifdef EBADSLT // 57: Invalid slot
	Q__DEFINE_ERRNO_EXCEPTION_( EBADSLT, badslt );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( badslt );
#endif

#ifdef ENONET // 64: Machine is not on the network
	Q__DEFINE_ERRNO_EXCEPTION_( ENONET, nonet );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nonet );
#endif

#ifdef ENOPKG // 65: Package not installed
	Q__DEFINE_ERRNO_EXCEPTION_( ENOPKG, nopkg );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nopkg );
#endif

#ifdef EADV // 68: Advertise error
	Q__DEFINE_ERRNO_EXCEPTION_( EADV, adv );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( adv );
#endif

#ifdef ESRMNT // 69: Srmount error
	Q__DEFINE_ERRNO_EXCEPTION_( ESRMNT, srmnt );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( srmnt );
#endif

#ifdef ECOMM // 70: Communication error on send
	Q__DEFINE_ERRNO_EXCEPTION_( ECOMM, comm );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( comm );
#endif

#ifdef EDOTDOT // 73: RFS specific error
	Q__DEFINE_ERRNO_EXCEPTION_( EDOTDOT, dotdot );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( dotdot );
#endif

#ifdef ENOTUNIQ // 76: Name not unique on network
	Q__DEFINE_ERRNO_EXCEPTION_( ENOTUNIQ, notuniq );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( notuniq );
#endif

#ifdef EBADFD // 77: File descriptor in bad state
	Q__DEFINE_ERRNO_EXCEPTION_( EBADFD, badfd );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( badfd );
#endif

#ifdef EREMCHG // 78: Remote address changed
	Q__DEFINE_ERRNO_EXCEPTION_( EREMCHG, remchg );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( remchg );
#endif

#ifdef ELIBACC // 79: Can not access a needed shared library
	Q__DEFINE_ERRNO_EXCEPTION_( ELIBACC, libacc );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( libacc );
#endif

#ifdef ELIBBAD // 80: Accessing a corrupted shared library
	Q__DEFINE_ERRNO_EXCEPTION_( ELIBBAD, libbad );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( libbad );
#endif

#ifdef ELIBSCN // 81: .lib section in a.out corrupted
	Q__DEFINE_ERRNO_EXCEPTION_( ELIBSCN, libscn );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( libscn );
#endif

#ifdef ELIBMAX // 82: Attempting to link in too many shared libraries
	Q__DEFINE_ERRNO_EXCEPTION_( ELIBMAX, libmax );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( libmax );
#endif

#ifdef ELIBEXEC // 83: Cannot exec a shared library directly
	Q__DEFINE_ERRNO_EXCEPTION_( ELIBEXEC, libexec );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( libexec );
#endif

#ifdef ERESTART // 85: Interrupted system call should be restarted
	Q__DEFINE_ERRNO_EXCEPTION_( ERESTART, restart );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( restart );
#endif

#ifdef ESTRPIPE // 86: Streams pipe error
	Q__DEFINE_ERRNO_EXCEPTION_( ESTRPIPE, strpipe );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( strpipe );
#endif

#ifdef EUCLEAN // 117: Structure needs cleaning
	Q__DEFINE_ERRNO_EXCEPTION_( EUCLEAN, uclean );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( uclean );
#endif

#ifdef ENOTNAM // 118: Not a XENIX named type file
	Q__DEFINE_ERRNO_EXCEPTION_( ENOTNAM, notnam );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( notnam );
#endif

#ifdef ENAVAIL // 119: No XENIX semaphores available
	Q__DEFINE_ERRNO_EXCEPTION_( ENAVAIL, navail );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( navail );
#endif

#ifdef EISNAM // 120: Is a named type file
	Q__DEFINE_ERRNO_EXCEPTION_( EISNAM, isnam );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( isnam );
#endif

#ifdef EREMOTEIO // 121: Remote I/O error
	Q__DEFINE_ERRNO_EXCEPTION_( EREMOTEIO, remoteio );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( remoteio );
#endif

#ifdef EMEDIUMTYPE // 124: Wrong medium type
	Q__DEFINE_ERRNO_EXCEPTION_( EMEDIUMTYPE, mediumtype );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( mediumtype );
#endif

#ifdef ENOKEY // 126: Required key not available
	Q__DEFINE_ERRNO_EXCEPTION_( ENOKEY, nokey );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( nokey );
#endif

#ifdef EKEYEXPIRED // 127: Key has expired
	Q__DEFINE_ERRNO_EXCEPTION_( EKEYEXPIRED, keyexpired );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( keyexpired );
#endif

#ifdef EKEYREVOKED // 128: Key has been revoked
	Q__DEFINE_ERRNO_EXCEPTION_( EKEYREVOKED, keyrevoked );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( keyrevoked );
#endif

#ifdef EKEYREJECTED // 129: Key was rejected by service
	Q__DEFINE_ERRNO_EXCEPTION_( EKEYREJECTED, keyrejected );
#else
	Q__DEFINE_NON_EXISTENT_ERRNO_EXCEPTION_( keyrejected );
#endif

} // namespace q

#endif // LIBQ_EXCEPTION_EXCEPTION_ERRNO_HPP
