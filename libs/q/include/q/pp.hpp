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


#define LIBQ_LINE __LINE__
#define LIBQ_FILE __FILE__
#ifdef LIBQ_ON_WINDOWS
#	define LIBQ_FUNCTION __FUNCTION__
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

#endif // LIBQ_PP_HPP
