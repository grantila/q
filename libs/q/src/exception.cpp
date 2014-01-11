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

#include <q/exception.hpp>

#include <q/stacktrace.hpp> // TODO: Remove

#include <iostream>

namespace q {

struct exception::pimpl
{
	std::vector< std::shared_ptr< detail::exception_info_base > > infos_;
};

exception::exception( )
: pimpl_( new pimpl )
{
	*this << get_stacktrace( );
}

exception::exception( exception&& ref )
: pimpl_( std::move( ref.pimpl_ ) )
{ }

exception::exception( const exception& ref )
: pimpl_( new pimpl )
{
	pimpl_->infos_ = ref.pimpl_->infos_;
}

exception::~exception( )
{ }

const std::vector< std::shared_ptr< detail::exception_info_base > >&
exception::infos( ) const
{
	return pimpl_->infos_;
}

void exception::add_info( std::shared_ptr< detail::exception_info_base >&& ptr )
{
	pimpl_->infos_.push_back( std::move( ptr ) );
}

std::vector< std::shared_ptr< detail::exception_info_base > >&
exception::infos( )
{
	return pimpl_->infos_;
}

std::ostream& operator<<( std::ostream& os, const exception& e )
{
	const auto& infos = e.infos( );

	if ( infos.empty( ) )
	{
		os << "[ q::exception ]";
	}
	else
	{
		os << "=== [ q::exception ]: ================================";
		for ( const auto& info : infos )
		{
			os << info->string( ) << std::endl;
		}
		if ( infos.empty( ) )
		os << "======================================================";
	}

	return os;
}


stream_exception::stream_exception( std::exception_ptr&& e )
: exception_( std::move( e ) )
{ }

stream_exception::stream_exception( const std::exception_ptr& e )
: exception_( e )
{ }

const std::exception_ptr& stream_exception::exception( ) const
{
	return exception_;
}

std::ostream& operator<<( std::ostream& os , const stream_exception& se )
{
	try
	{
		std::rethrow_exception( se.exception( ) );
	}
	catch ( const exception& e )
	{
		os << e;
	}
	catch ( const std::exception& e )
	{
		os << e.what( );
	}
	catch ( ... )
	{
		// TODO: Add possibility to register further catchers
		os << "unknown exception";
	}

	return os;
}

} // namespace q
