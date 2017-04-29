/*
 * Copyright 2017 Gustaf Räntilä
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

#ifndef LIBQ_TEST_BACKEND_HPP
#define LIBQ_TEST_BACKEND_HPP

#ifdef QTEST_ON_GTEST
#	include <q-test/backends/gtest.hpp>
#elif defined( QTEST_ON_CATCH )
#	include <q-test/backends/catch.hpp>
#elif defined( QTEST_ON_BOOST ) || defined( QTEST_ON_BOOST_SH )
#	include <q-test/backends/boost.hpp>
#else
#	error No backend specified
	// Before including q-test headers, a backend must be defined
#endif


// Defaults:

#ifndef QTEST_BACKEND_FAIL_AT
#	define QTEST_BACKEND_FAIL_AT( file, line, ... ) \
		QTEST_BACKEND_FAIL( \
			file << ":" << line << LIBQ_EOL << __VA_ARGS__ )
#endif // QTEST_BACKEND_FAIL_AT


#endif // LIBQ_TEST_BACKEND_HPP
