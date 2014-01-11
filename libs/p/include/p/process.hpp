/*
 * Copyright 2014 Gustaf Räntilä
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

#ifndef LIBP_PROCESS_HPP
#define LIBP_PROCESS_HPP

#include <q/functional.hpp>
#include <q/async_termination.hpp>

#include <memory>

namespace p {

class process
: public std::enable_shared_from_this< process >
, public q::async_termination<
	q::arguments< >,
	std::tuple< int >
>
{
public:
	template< typename Fn >
	typename std::enable_if<
		,
		promise< Q_RESULT_OF_AS_ARGUMENT_TYPE( Fn )::tuple_type >
	>::type
	run( Fn&& fn )
	{
		;
	}

	static std::shared_ptr< process > construct( );

protected:
	process( );

private:
	void run( );
	void do_terminate( ) override;

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

class agreed
{
	;
};

} // namespace p

#endif // LIBP_PROCESS_HPP
