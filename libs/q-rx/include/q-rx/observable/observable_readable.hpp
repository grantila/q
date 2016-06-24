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

#ifndef LIBQ_RX_OBSERVABLE_OBSERVABLE_READABLE_HPP
#define LIBQ_RX_OBSERVABLE_OBSERVABLE_READABLE_HPP

namespace q { namespace rx { namespace detail {

template< typename T >
class observable_readable
{
public:
	virtual ~observable_readable( ) { }

	virtual q::promise< std::tuple< > >
	receive( std::function< void( T&& ) > fn, queue_ptr queue ) = 0;

	virtual queue_ptr get_queue( ) const = 0;

protected:
	observable_readable( ) { }
};

template< typename T >
class observable_readable_direct
: public observable_readable< T >
{
public:
	observable_readable_direct( q::readable< T >&& readable )
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( std::function< void( T&& ) > fn, queue_ptr queue )
	{
		return readable_.receive( )
		.then( fn, std::move( queue ) );
	}

	queue_ptr get_queue( ) const
	{
		return readable_.get_queue( );
	}

private:
	q::readable< T > readable_;
};

template< typename T >
class observable_readable_expect
: public observable_readable< T >
{
public:
	observable_readable_expect( q::readable< q::expect< T > >&& readable )
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( std::function< void( T&& ) > fn, queue_ptr queue )
	{
		return readable_.receive( )
		.then( [ fn ]( q::expect< T >&& exp )
		{
			return fn( exp.consume( ) );
		}, std::move( queue ) );
	}

	queue_ptr get_queue( ) const
	{
		return readable_.get_queue( );
	}

private:
	q::readable< q::expect< T > > readable_;
};

template< typename T >
class observable_readable_promise
: public observable_readable< T >
{
public:
	observable_readable_promise(
		q::readable< q::promise< std::tuple< T > > >&& readable
	)
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( std::function< void( T&& ) > fn, queue_ptr queue )
	{
		return readable_.receive( )
		.then( fn, std::move( queue ) );
	}

	queue_ptr get_queue( ) const
	{
		return readable_.get_queue( );
	}

private:
	q::readable< q::promise< std::tuple< T > > > readable_;
};

template< typename T >
class observable_readable_shared_promise
: public observable_readable< T >
{
public:
	observable_readable_shared_promise(
		q::readable< q::shared_promise< std::tuple< T > > >&& readable
	)
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( std::function< void( T&& ) > fn, queue_ptr queue )
	{
		return readable_.receive( )
		.then( fn, std::move( queue ) );
	}

	queue_ptr get_queue( ) const
	{
		return readable_.get_queue( );
	}

private:
	q::readable< q::shared_promise< std::tuple< T > > > readable_;
};

} } } // namespace detail, namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_OBSERVABLE_READABLE_HPP
