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

namespace {

using empty_promise = q::promise< std::tuple< > >;

} // anonymous namespace

template< typename T >
struct observable_readable_traits
{
	using receiver_type = std::function< void( T&& ) >;
	using receiver_type_tuple = std::function< void( std::tuple< T >&& ) >;
	using receiver_type_void = std::function< void( ) >;
	using async_receiver_type = std::function< empty_promise( T&& ) >;
	using async_receiver_type_tuple =
		std::function< empty_promise( std::tuple< T >&& ) >;
	using async_receiver_type_void = std::function< empty_promise( ) >;

	using readable_type = q::readable< T >;
	using readable_expect_type = q::readable< q::expect< T > >;
	using readable_promise_type = q::readable< q::promise< std::tuple< T > > >;
	using readable_shared_promise_type =
		q::readable< q::shared_promise< std::tuple< T > > >;
};

template< typename... T >
struct observable_readable_traits< std::tuple< T... > >
{
	using receiver_type = std::function< void( std::tuple< T... >&& ) >;
	using receiver_type_tuple = std::function< void( T&&... ) >;
	using receiver_type_void = std::function< void( ) >;
	using async_receiver_type =
		std::function< empty_promise( std::tuple< T... >&& ) >;
	using async_receiver_type_tuple = std::function< empty_promise( T&&... ) >;
	using async_receiver_type_void = std::function< empty_promise( ) >;

	using readable_type = q::readable< T... >;
	using readable_expect_type = q::readable< q::expect< std::tuple< T... > > >;
	using readable_promise_type =
		q::readable< q::promise< std::tuple< T... > > >;
	using readable_shared_promise_type =
		q::readable< q::shared_promise< std::tuple< T... > > >;
};

template< >
struct observable_readable_traits< void >
{
	using receiver_type = std::function< void( ) >;
	using receiver_type_tuple = void_t;
	using receiver_type_objectified = std::function< void( void_t ) >;
	using async_receiver_type = std::function< empty_promise( ) >;
	using async_receiver_type_tuple = void_t;
	using async_receiver_type_objectified =
		std::function< empty_promise( void_t ) >;

	using readable_type = q::readable< >;
	using readable_expect_type = q::readable< q::expect< void > >;
	using readable_promise_type = q::readable< q::promise< std::tuple< > > >;
	using readable_shared_promise_type =
		q::readable< q::shared_promise< std::tuple< > > >;
};

template< typename T >
struct observable_readable_objectified
{
	using receive_type = typename observable_readable_traits< T >
		::receiver_type;
	using receiver_type_tuple = typename observable_readable_traits< T >
		::receiver_type_tuple;
	using async_receive_type = typename observable_readable_traits< T >
		::async_receiver_type;
	using async_receiver_type_tuple = typename observable_readable_traits< T >
		::async_receiver_type_tuple;

	virtual q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) = 0;

	virtual q::promise< std::tuple< > >
	receive( receiver_type_tuple&& fn, queue_ptr queue ) = 0;
};

template< >
struct observable_readable_objectified< void >
{
	using receive_type = typename observable_readable_traits< void >
		::receiver_type;
	using receive_type_objectified = typename observable_readable_traits< void >
		::receiver_type_objectified;
	using async_receive_type = typename observable_readable_traits< void >
		::async_receiver_type;
	using async_receive_type_objectified =
		typename observable_readable_traits< void >
		::async_receiver_type_objectified;

	virtual q::promise< std::tuple< > >
	receive( receive_type&& fn, queue_ptr queue ) = 0;

	virtual q::promise< std::tuple< > >
	receive( receive_type_objectified&& fn, queue_ptr queue ) = 0;
};

template< >
struct observable_readable_objectified< void_t >
{
	using receive_type = typename observable_readable_traits< void_t >
		::receiver_type;
	using receive_type_void = typename observable_readable_traits< void_t >
		::receiver_type_void;
	using async_receive_type = typename observable_readable_traits< void_t >
		::async_receiver_type;
	using async_receive_type_void =
		typename observable_readable_traits< void_t >
		::async_receiver_type_void;

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
	using receive_type = typename observable_readable_traits< T >
		::receiver_type;
	using receiver_type_tuple = typename observable_readable_traits< T >
		::receiver_type_tuple;
	using async_receive_type = typename observable_readable_traits< T >
		::async_receiver_type;
	using async_receiver_type_tuple = typename observable_readable_traits< T >
		::async_receiver_type_tuple;
	using readable_type = typename observable_readable_traits< T >
		::readable_type;

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
	receive( receiver_type_tuple&& fn, queue_ptr queue ) override
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
	using receive_type = typename observable_readable_traits< void >
		::receiver_type;
	using receive_type_objectified = typename observable_readable_traits< void >
		::receiver_type_objectified;
	using async_receive_type = typename observable_readable_traits< void >
		::async_receiver_type;
	using async_receive_type_objectified =
		typename observable_readable_traits< void >
		::async_receiver_type_objectified;
	using readable_type = typename observable_readable_traits< void >
		::readable_type;

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
	using receive_type = typename observable_readable_traits< void_t >
		::receiver_type;
	using receiver_type_void = typename observable_readable_traits< void_t >
		::receiver_type_void;
	using async_receive_type = typename observable_readable_traits< void_t >
		::async_receiver_type;
	using async_receiver_type_void =
		typename observable_readable_traits< void_t >
		::async_receiver_type_void;
	using readable_type = typename observable_readable_traits< void_t >
		::readable_type;

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
	using receive_type = typename observable_readable_traits< T >
		::receiver_type;
	using receiver_type_tuple = typename observable_readable_traits< T >
		::receiver_type_tuple;
	using async_receive_type = typename observable_readable_traits< T >
		::async_receiver_type;
	using async_receiver_type_tuple = typename observable_readable_traits< T >
		::async_receiver_type_tuple;
	using readable_expect_type = typename observable_readable_traits< T >
		::readable_expect_type;

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

	q::promise< std::tuple< > >
	receive( receiver_type_tuple&& fn, queue_ptr queue ) override
	{
		Q_THROW( 1 ); // Just for 'override' compile-time reasons
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
	using receive_type = typename observable_readable_traits< void >
		::receiver_type;
	using receive_type_objectified = typename observable_readable_traits< void >
		::receiver_type_objectified;
	using async_receive_type = typename observable_readable_traits< void >
		::async_receiver_type;
	using async_receive_type_objectified =
		typename observable_readable_traits< void >
		::async_receiver_type_objectified;
	using readable_expect_type = typename observable_readable_traits< void >
		::readable_expect_type;

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
	using receive_type = typename observable_readable_traits< void_t >
		::receiver_type;
	using receive_type_void = typename observable_readable_traits< void_t >
		::receiver_type_void;
	using async_receive_type = typename observable_readable_traits< void_t >
		::async_receiver_type;
	using async_receive_type_void =
		typename observable_readable_traits< void_t >
		::async_receiver_type_void;
	using readable_expect_type = typename observable_readable_traits< void_t >
		::readable_expect_type;

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
	using receive_type = typename observable_readable_traits< T >
		::receiver_type;
	using receiver_type_tuple = typename observable_readable_traits< T >
		::receiver_type_tuple;
	using async_receive_type = typename observable_readable_traits< T >
		::async_receiver_type;
	using async_receiver_type_tuple = typename observable_readable_traits< T >
		::async_receiver_type_tuple;
	using readable_promise_type = typename observable_readable_traits< T >
		::readable_promise_type;

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
	receive( receiver_type_tuple&& fn, queue_ptr queue ) override
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
	using receive_type = typename observable_readable_traits< void >
		::receiver_type;
	using receive_type_objectified = typename observable_readable_traits< void >
		::receiver_type_objectified;
	using async_receive_type = typename observable_readable_traits< void >
		::async_receiver_type;
	using async_receive_type_objectified =
		typename observable_readable_traits< void >
		::async_receiver_type_objectified;
	using readable_promise_type = typename observable_readable_traits< void >
		::readable_promise_type;

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
	using receive_type = typename observable_readable_traits< void_t >
		::receiver_type;
	using receive_type_void = typename observable_readable_traits< void_t >
		::receiver_type_void;
	using async_receive_type = typename observable_readable_traits< void_t >
		::async_receiver_type;
	using async_receive_type_void =
		typename observable_readable_traits< void_t >
		::async_receiver_type_void;
	using readable_promise_type = typename observable_readable_traits< void_t >
		::readable_promise_type;

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
	using receive_type = typename observable_readable_traits< T >
		::receiver_type;
	using receiver_type_tuple = typename observable_readable_traits< T >
		::receiver_type_tuple;
	using async_receive_type = typename observable_readable_traits< T >
		::async_receiver_type;
	using async_receiver_type_tuple = typename observable_readable_traits< T >
		::async_receiver_type_tuple;
	using readable_shared_promise_type =
		typename observable_readable_traits< T >
		::readable_shared_promise_type;

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
	receive( receiver_type_tuple&& fn, queue_ptr queue ) override
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
	using receive_type = typename observable_readable_traits< void >
		::receiver_type;
	using receive_type_objectified = typename observable_readable_traits< void >
		::receiver_type_objectified;
	using async_receive_type = typename observable_readable_traits< void >
		::async_receiver_type;
	using async_receive_type_objectified =
		typename observable_readable_traits< void >
		::async_receiver_type_objectified;
	using readable_shared_promise_type =
		typename observable_readable_traits< void >
		::readable_shared_promise_type;

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
	using receive_type = typename observable_readable_traits< void_t >
		::receiver_type;
	using receive_type_void = typename observable_readable_traits< void_t >
		::receiver_type_void;
	using async_receive_type = typename observable_readable_traits< void_t >
		::async_receiver_type;
	using async_receive_type_void =
		typename observable_readable_traits< void_t >
		::async_receiver_type_void;
	using readable_shared_promise_type =
		typename observable_readable_traits< void_t >
		::readable_shared_promise_type;

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
