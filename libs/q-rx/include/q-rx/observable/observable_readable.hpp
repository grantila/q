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
struct observable_readable_traits
{
	typedef std::function< void( T&& ) > receiver_type;
	typedef std::function< void( ) > receiver_type_void;
	typedef q::readable< T > readable_type;
	typedef q::readable< q::expect< T > > readable_expect_type;
	typedef q::readable< q::promise< std::tuple< T > > >
		readable_promise_type;
	typedef q::readable< q::shared_promise< std::tuple< T > > >
		readable_shared_promise_type;
};

template< >
struct observable_readable_traits< void >
{
	typedef std::function< void( ) > receiver_type;
	typedef std::function< void( void_t ) > receiver_type_objectified;
	typedef q::readable< > readable_type;
	typedef q::readable< q::expect< void > > readable_expect_type;
	typedef q::readable< q::promise< std::tuple< > > >
		readable_promise_type;
	typedef q::readable< q::shared_promise< std::tuple< > > >
		readable_shared_promise_type;
};

template< typename T >
struct observable_readable_objectified
{
	typedef typename observable_readable_traits< T >::receiver_type
		receive_type;

	virtual q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) = 0;
};

template< >
struct observable_readable_objectified< void >
{
	typedef typename observable_readable_traits< void >
		::receiver_type
		receive_type;
	typedef typename observable_readable_traits< void >
		::receiver_type_objectified
		receive_type_objectified;

	virtual q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) = 0;

	virtual q::promise< std::tuple< > >
	receive( receive_type_objectified&& fn, queue_ptr queue ) = 0;
};

template< >
struct observable_readable_objectified< void_t >
{
	typedef typename observable_readable_traits< void_t >
		::receiver_type
		receive_type;
	typedef typename observable_readable_traits< void_t >
		::receiver_type_void
		receive_type_void;

	virtual q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) = 0;

	virtual q::promise< std::tuple< > >
	receive( receive_type_void&& fn, queue_ptr queue ) = 0;
};

template< typename T >
class observable_readable
: public observable_readable_objectified< T >
{
public:
	using observable_readable_objectified< T >::receive;

	virtual ~observable_readable( ) { }

	virtual queue_ptr get_queue( ) const = 0;

	virtual std::size_t backlog( ) const = 0;

	virtual bool is_closed( ) const = 0;

	virtual std::exception_ptr get_exception( ) const = 0;

protected:
	observable_readable( ) { }
};

template< typename T >
class observable_readable_direct
: public observable_readable< T >
{
public:
	typedef typename observable_readable_traits< T >::receiver_type
		receive_type;
	typedef typename observable_readable_traits< T >::readable_type
		readable_type;

	observable_readable_direct( readable_type&& readable )
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_type readable_;
};

template< >
class observable_readable_direct< void >
: public observable_readable< void >
{
public:
	typedef typename observable_readable_traits< void >
		::receiver_type
		receive_type;
	typedef typename observable_readable_traits< void >
		::receiver_type_objectified
		receive_type_objectified;
	typedef typename observable_readable_traits< void >
		::readable_type
		readable_type;

	observable_readable_direct( readable_type&& readable )
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< std::tuple< > >
	receive( receive_type_objectified&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_type readable_;
};

template< >
class observable_readable_direct< void_t >
: public observable_readable< void_t >
{
public:
	typedef typename observable_readable_traits< void_t >
		::receiver_type
		receive_type;
	typedef typename observable_readable_traits< void_t >
		::receiver_type_void
		receiver_type_void;
	typedef typename observable_readable_traits< void_t >
		::readable_type
		readable_type;

	observable_readable_direct( readable_type&& readable )
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< std::tuple< > >
	receive( receiver_type_void&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_type readable_;
};


template< typename T >
class observable_readable_expect
: public observable_readable< T >
{
public:
	typedef typename observable_readable_traits< T >::receiver_type
		receive_type;
	typedef typename observable_readable_traits< T >::readable_expect_type
		readable_expect_type;

	observable_readable_expect( readable_expect_type&& readable )
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< T >&& exp )
		{
			return fn( exp.consume( ) );
		}, std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_expect_type readable_;
};

template< >
class observable_readable_expect< void >
: public observable_readable< void >
{
public:
	typedef typename observable_readable_traits< void >
		::receiver_type
		receive_type;
	typedef typename observable_readable_traits< void >
		::receiver_type_objectified
		receive_type_objectified;
	typedef typename observable_readable_traits< void >
		::readable_expect_type
		readable_expect_type;

	observable_readable_expect( readable_expect_type&& readable )
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< void >&& exp )
		{
			exp.consume( );
			return fn( );
		}, std::move( queue ) );
	}

	q::promise< std::tuple< > >
	receive( receive_type_objectified&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< void >&& exp )
		{
			exp.consume( );
			return fn( void_t( ) );
		}, std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_expect_type readable_;
};

template< >
class observable_readable_expect< void_t >
: public observable_readable< void_t >
{
public:
	typedef typename observable_readable_traits< void_t >
		::receiver_type
		receive_type;
	typedef typename observable_readable_traits< void_t >
		::receiver_type_void
		receive_type_void;
	typedef typename observable_readable_traits< void_t >
		::readable_expect_type
		readable_expect_type;

	observable_readable_expect( readable_expect_type&& readable )
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< void_t >&& exp )
		{
			return fn( exp.consume( ) );
		}, std::move( queue ) );
	}

	q::promise< std::tuple< > >
	receive( receive_type_void&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< void_t >&& exp )
		{
			exp.consume( );
			return fn( );
		}, std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_expect_type readable_;
};


template< typename T >
class observable_readable_promise
: public observable_readable< T >
{
public:
	typedef typename observable_readable_traits< T >::receiver_type
		receive_type;
	typedef typename observable_readable_traits< T >::readable_promise_type
		readable_promise_type;

	observable_readable_promise( readable_promise_type&& readable )
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_promise_type readable_;
};

template< >
class observable_readable_promise< void >
: public observable_readable< void >
{
public:
	typedef typename observable_readable_traits< void >
		::receiver_type
		receive_type;
	typedef typename observable_readable_traits< void >
		::receiver_type_objectified
		receive_type_objectified;
	typedef typename observable_readable_traits< void >
		::readable_promise_type
		readable_promise_type;

	observable_readable_promise( readable_promise_type&& readable )
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< std::tuple< > >
	receive( receive_type_objectified&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_promise_type readable_;
};

template< >
class observable_readable_promise< void_t >
: public observable_readable< void_t >
{
public:
	typedef typename observable_readable_traits< void_t >
		::receiver_type
		receive_type;
	typedef typename observable_readable_traits< void_t >
		::receiver_type_void
		receive_type_void;
	typedef typename observable_readable_traits< void_t >
		::readable_promise_type
		readable_promise_type;

	observable_readable_promise( readable_promise_type&& readable )
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< std::tuple< > >
	receive( receive_type_void&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_promise_type readable_;
};


template< typename T >
class observable_readable_shared_promise
: public observable_readable< T >
{
public:
	typedef typename observable_readable_traits< T >::receiver_type
		receive_type;
	typedef typename observable_readable_traits< T >
		::readable_shared_promise_type readable_shared_promise_type;

	observable_readable_shared_promise(
		readable_shared_promise_type&& readable
	)
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_shared_promise_type readable_;
};

template< >
class observable_readable_shared_promise< void >
: public observable_readable< void >
{
public:
	typedef typename observable_readable_traits< void >
		::receiver_type
		receive_type;
	typedef typename observable_readable_traits< void >
		::receiver_type_objectified
		receive_type_objectified;
	typedef typename observable_readable_traits< void >
		::readable_shared_promise_type
		readable_shared_promise_type;

	observable_readable_shared_promise(
		readable_shared_promise_type&& readable
	)
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< std::tuple< > >
	receive( receive_type_objectified&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_shared_promise_type readable_;
};

template< >
class observable_readable_shared_promise< void_t >
: public observable_readable< void_t >
{
public:
	typedef typename observable_readable_traits< void_t >
		::receiver_type
		receive_type;
	typedef typename observable_readable_traits< void_t >
		::receiver_type_void
		receive_type_void;
	typedef typename observable_readable_traits< void_t >
		::readable_shared_promise_type
		readable_shared_promise_type;

	observable_readable_shared_promise(
		readable_shared_promise_type&& readable
	)
	: readable_( readable )
	{ }

	q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< std::tuple< > >
	receive( receive_type_void&& fn, queue_ptr queue ) override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	queue_ptr get_queue( ) const override
	{
		return readable_.get_queue( );
	}

	std::size_t backlog( ) const override
	{
		return readable_.buffer_count( );
	}

	bool is_closed( ) const override
	{
		return readable_.is_closed( );
	}

	std::exception_ptr get_exception( ) const override
	{
		return readable_.get_exception( );
	}

private:
	readable_shared_promise_type readable_;
};

} } } // namespace detail, namespace rx, namespace q

#endif // LIBQ_RX_OBSERVABLE_OBSERVABLE_READABLE_HPP
