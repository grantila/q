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

#include <q/lib.hpp>
#include <q/pp.hpp>

#include <vector>

namespace {

std::vector< q::function< void( void ) > >* get_initializers( )
{
	static std::vector< q::function< void( void ) > > initializers;
	return &initializers;
}

} // anomymous namespace

namespace q {

namespace detail {

void register_internal_initializer( q::function< void( void ) >&& func )
{
	get_initializers( )->push_back(
#if defined( LIBQ_ON_GCC ) && ( LIBQ_ON_GCC < 40900 )
		q::function< void( void ) >( func )
#else
		std::move( func )
#endif
	);
}

} // namespace detail

void initialize( settings settings )
{
	auto initializers = *get_initializers( );

	for ( auto& func : initializers )
		func( );
	initializers.clear( );
}

void uninitialize( )
{ }

scope scoped_initialize( settings settings )
{
	initialize( settings );

	return make_scoped_function( uninitialize );
}

} // namespace q
