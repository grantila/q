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

#ifndef LIBQ_LIB_HPP
#define LIBQ_LIB_HPP

#include <q/scope.hpp>
#include <q/functional.hpp>
#include <q/exception.hpp>

#include <q/detail/lib.hpp>

#include <iostream>

namespace q {

namespace detail {

} // namespace detail

class settings
{
public:
	settings( ) = default;

	/**
	 * Registers a function to be called when an exception occurs where it
	 * cannot be propagated to the application properly, such as in
	 * functions where exceptions must not be thrown.
	 *
	 * Default behaviour is to print the exception information to stderr
	 * and call terminate().
	 */
	template< typename Fn >
	typename std::enable_if<
		(
			Q_ARITY_OF( Fn ) == 0 ||
			Q_ARGUMENTS_ARE( Fn, std::exception_ptr )::value ||
			Q_ARGUMENTS_ARE( Fn, std::exception_ptr&& )::value ||
			Q_ARGUMENTS_ARE( Fn, const std::exception_ptr& )::value
		) &&
		std::is_void< Q_RESULT_OF( Fn ) >::value,
		settings&
	>::type
	set_uncaught_exception_handler( Fn&& fn ) {
		register_uncaught_exception_handler( fn );
		return *this;
	}

	/**
	 * Registers a function to be called when a q chain of asynchronous
	 * tasks are not ended with a fail() check to catch exceptions, but
	 * where an exception instead is silently leaked and dropped. q will
	 * detect this and will call the provided function.
	 *
	 * Default behaviour is to print the exception information to stderr,
	 * and continue running.
	 *
	 * NOT IMPLEMENETED YET!
	 */
	template< typename Fn >
	typename std::enable_if<
		(
			Q_ARITY_OF( Fn ) == 0 ||
			Q_ARGUMENTS_ARE( Fn, std::exception_ptr )::value ||
			Q_ARGUMENTS_ARE( Fn, std::exception_ptr&& )::value ||
			Q_ARGUMENTS_ARE( Fn, const std::exception_ptr& )::value
		) &&
		std::is_void< Q_RESULT_OF( Fn ) >::value,
		settings&
	>::type
	set_silent_exception_handler( Fn&& fn ) { return *this; }

	/**
	 * Enables long stack support which means stack traces from tasks will
	 * include the call stack of the entire chain of promises which lead to
	 * the current position. This is very useful for debugging, but is very
	 * costly and should not be used in production.
	 *
	 * Defaults to false.
	 *
	 * NOT IMPLEMENTED
	 */
	settings& set_long_stack_support( bool ) { return *this; }
};

void initialize( settings = settings( ) );
void uninitialize( );

scope scoped_initialize( settings = settings( ) );

} // namespace q

#endif // LIBQ_LIB_HPP
