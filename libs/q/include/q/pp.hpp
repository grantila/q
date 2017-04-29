/*
 * Copyright 2013 Gustaf Räntilä
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

#ifndef LIBQ_PP_HPP
#define LIBQ_PP_HPP

#ifdef _WIN32
#	define LIBQ_ON_WINDOWS
#	ifdef _M_X64
#		define LIBQ_ON_X64
#	endif
#	ifdef _M_IX86
#		define LIBQ_ON_X86
#	endif
#	ifdef _M_ARM
#		define LIBQ_ON_ARM
#	endif
#	ifdef _M_ARM64
#		define LIBQ_ON_ARM64
#	endif
#	define Q_NO_FUNCTION_ALIGN
#elif defined( __APPLE__ )
#	define LIBQ_ON_OSX
#	define LIBQ_ON_POSIX
#elif defined( __linux__ )
#	define LIBQ_ON_LINUX
#	define LIBQ_ON_POSIX
#elif defined( __FreeBSD__ )
#	define LIBQ_ON_FREEBSD
#	define LIBQ_ON_BSD
#	define LIBQ_ON_POSIX
#elif defined( __NetBSD__ )
#	define LIBQ_ON_NETBSD
#	define LIBQ_ON_BSD
#	define LIBQ_ON_POSIX
#elif defined( __OpenBSD__ )
#	define LIBQ_ON_OPENBSD
#	define LIBQ_ON_BSD
#	define LIBQ_ON_POSIX
#elif defined( __DragonFly__ )
#	define LIBQ_ON_DRAGONFLYBSD
#	define LIBQ_ON_BSD
#	define LIBQ_ON_POSIX
#else
#	define LIBQ_ON_UNKNOWN
#endif

#ifdef LIBQ_ON_WINDOWS
#	define LIBQ_EOL "\r\n"
#else
#	define LIBQ_EOL "\n"
#endif

#if defined( __GNUC__ ) && !defined( __clang__ )
#	define LIBQ_ON_GCC 0 + ( \
		__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ )
#endif

#if defined( __clang__ )
#	define LIBQ_ON_CLANG
#endif

#if defined( LIBQ_ON_GCC ) || defined( LIBQ_ON_CLANG )
#	define LIBQ_ON_GCC_OR_CLANG
#endif

#if defined( LIBQ_ON_GCC ) || defined( LIBQ_ON_WINDOWS )
#	define LIBQ_WITH_BROKEN_NOEXCEPT
#endif

#if __cplusplus >= 201402L
#	define LIBQ_WITH_CPP14
#endif
#if __cplusplus >= 201406L
	// This particular version isn't set yet, and compiler support may vary
#	define LIBQ_WITH_CPP17
#endif

#ifdef LIBQ_WITH_CPP17
#	define Q_NODISCARD [[nodiscard]]
#elif defined( LIBQ_ON_GCC ) && ( LIBQ_ON_GCC >= 40000 )
#	define Q_NODISCARD __attribute__ ((warn_unused_result))
#elif defined( _MSC_VER ) && ( _MSC_VER >= 1700 )
#	define Q_NODISCARD _Check_return_
#else
#	define Q_NODISCARD
#endif

#define LIBQ_ASSUMED_CACHE_LINE_SIZE_( ) ( 8 * sizeof( std::intptr_t ) )
#define LIBQ_ASSUMED_CACHE_LINE_SIZE LIBQ_ASSUMED_CACHE_LINE_SIZE_( )

#define Q_MAX( a, b ) ( ( a < b ) ? b : a )
#define Q_MIN( a, b ) ( ( a < b ) ? a : b )

#define LIBQ_JOIN_( a, b ) a ## b
#define LIBQ_JOIN( a, b ) LIBQ_JOIN_( a, b )

#define LIBQ_LINE __LINE__
#define LIBQ_FILE __FILE__
#ifdef LIBQ_ON_WINDOWS
#	define LIBQ_FUNCTION __FUNCSIG__
#else
#	define LIBQ_FUNCTION __PRETTY_FUNCTION__
#endif

#define LIBQ_LOCATION LIBQ_FILE, LIBQ_LINE, LIBQ_FUNCTION

#define LIBQ_DATE __DATE__
#define LIBQ_TIME __TIME__

#define LIBQ_UNIQUE_COMPILETIME_VALUE LIBQ_DATE "-" LIBQ_TIME

// EXPAND is needed sometimes due to MSVC bugs
#define LIBQ_EXPAND( x ) x
#define LIBQ_FIRST( x, ... ) x
#define LIBQ_REST( x, ... ) __VA_ARGS__

// TODO: Make this more portable
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#	define LIBQ_LITTLE_ENDIAN
#endif


#define Q_ENUM_FLAGS( Name ) \
	static inline constexpr Name operator&( Name x, Name y ) \
	{ \
		return static_cast< Name >( \
			static_cast< \
				typename std::underlying_type< Name >::type \
			>( x ) \
			& \
			static_cast< \
				typename std::underlying_type< Name >::type \
			>( y ) ); \
	} \
	static inline constexpr Name operator|( Name x, Name y ) \
	{ \
		return static_cast< Name >( \
			static_cast< \
				typename std::underlying_type< Name >::type \
			>( x ) \
			| \
			static_cast< \
				typename std::underlying_type< Name >::type \
			>( y ) ); \
	} \
	static inline constexpr Name operator^( Name x, Name y ) \
	{ \
		return static_cast< Name >( \
			static_cast< \
				typename std::underlying_type< Name >::type \
			>( x ) \
			^ \
			static_cast< \
				typename std::underlying_type< Name >::type \
			>( y ) ); \
	} \
	static inline constexpr Name operator~( Name x ) \
	{ \
		return static_cast< Name >( \
			~static_cast< \
				typename std::underlying_type< Name >::type \
			>( x ) ); \
	} \
	static inline Name& operator&=( Name& x, Name y ) \
	{ \
		x = x & y; \
		return x; \
	} \
	static inline Name& operator|=( Name& x, Name y ) \
	{ \
		x = x | y; \
		return x; \
	} \
	static inline Name& operator^=( Name& x, Name y ) \
	{ \
		x = x ^ y; \
		return x; \
	}

#define Q_DEFINE_ENUM_FLAGS( Name ) \
	enum class Name; \
	Q_ENUM_FLAGS( Name ) \
	enum class Name

#define Q_ENUM_HAS( val, option ) \
	( ( val & option ) == option )

#endif // LIBQ_PP_HPP
