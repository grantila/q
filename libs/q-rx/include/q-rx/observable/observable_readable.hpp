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

using empty_promise = q::promise< >;

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
	using readable_promise_type = q::readable< q::promise< T > >;
	using readable_shared_promise_type =
		q::readable< q::shared_promise< T > >;
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
	using readable_promise_type = q::readable< q::promise< T... > >;
	using readable_shared_promise_type =
		q::readable< q::shared_promise< T... > >;
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
	using readable_promise_type = q::readable< q::promise< > >;
	using readable_shared_promise_type =
		q::readable< q::shared_promise< > >;
};

namespace detail {

struct tag_1 { };
struct tag_2 { };
struct tag_3 { };
struct tag_4 { };

template< typename A, typename B, typename C, typename D >
struct observable_readable_tag
{
	template<
		typename T,
		bool IsA = function_same_type_v< T, A >,
		bool IsB = function_same_type_v< T, B >,
		bool IsC = function_same_type_v< T, C >,
		bool IsD = function_same_type_v< T, D >
	>
	struct of
	{
		typedef std::false_type valid;
	};

	template< typename T, bool X, bool Y, bool Z >
	struct of< T, true, X, Y, Z >
	{
		typedef std::true_type valid;
		typedef detail::tag_1 type;
	};

	template< typename T, bool X, bool Y, bool Z >
	struct of< T, X, true, Y, Z >
	{
		typedef std::true_type valid;
		typedef detail::tag_2 type;
	};

	template< typename T, bool X, bool Y, bool Z >
	struct of< T, X, Y, true, Z >
	{
		typedef std::true_type valid;
		typedef detail::tag_3 type;
	};

	template< typename T, bool X, bool Y, bool Z >
	struct of< T, X, Y, Z, true >
	{
		typedef std::true_type valid;
		typedef detail::tag_4 type;
	};
};

} // namespace detail

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

	typedef detail::observable_readable_tag<
		receive_type,
		receiver_type_tuple,
		async_receive_type,
		async_receiver_type_tuple
	> get_tag;

	virtual q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 ) = 0;

	virtual q::promise< >
	_receive( receiver_type_tuple&& fn, queue_ptr queue, detail::tag_2 ) = 0;

	virtual q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 ) = 0;

	virtual q::promise< >
	_receive( async_receiver_type_tuple&& fn, queue_ptr queue, detail::tag_4 )
		= 0;
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

	typedef detail::observable_readable_tag<
		receive_type,
		receive_type_objectified,
		async_receive_type,
		async_receive_type_objectified
	> get_tag;

	virtual q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 ) = 0;

	virtual q::promise< >
	_receive( receive_type_objectified&& fn, queue_ptr queue, detail::tag_2 )
		= 0;

	virtual q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 ) = 0;

	virtual q::promise< >
	_receive(
		async_receive_type_objectified&& fn, queue_ptr queue, detail::tag_4 )
		= 0;
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

	typedef detail::observable_readable_tag<
		receive_type,
		receive_type_void,
		async_receive_type,
		async_receive_type_void
	> get_tag;

	virtual q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 ) = 0;

	virtual q::promise< >
	_receive( receive_type_void&& fn, queue_ptr queue, detail::tag_2 ) = 0;

	virtual q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 ) = 0;

	virtual q::promise< >
	_receive( async_receive_type_void&& fn, queue_ptr queue, detail::tag_4 )
		= 0;
};

template< typename T >
class observable_readable
: public observable_readable_objectified< T >
{
public:
	using observable_readable_objectified< T >::_receive;
	using typename observable_readable_objectified< T >::get_tag;

	template< typename Fn >
	typename std::enable_if<
		get_tag::template of< decayed_function_t< Fn > >::valid::value,
		q::promise< >
	>::type
	receive( Fn&& fn, queue_ptr queue )
	{
		typedef typename get_tag
			::template of< decayed_function_t< Fn > >::type tag;

		return _receive(
			std::forward< Fn >( fn ),
			std::move( queue ),
			tag( )
		);
	}

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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( receiver_type_tuple&& fn, queue_ptr queue, detail::tag_2 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( async_receiver_type_tuple&& fn, queue_ptr queue, detail::tag_4 )
	override
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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( receive_type_objectified&& fn, queue_ptr queue, detail::tag_2 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive(
		async_receive_type_objectified&& fn, queue_ptr queue, detail::tag_4 )
	override
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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( receiver_type_void&& fn, queue_ptr queue, detail::tag_2 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( async_receiver_type_void&& fn, queue_ptr queue, detail::tag_4 )
	override
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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< T >&& exp )
		{
			return fn( exp.consume( ) );
		}, std::move( queue ) );
	}

	q::promise< >
	_receive( receiver_type_tuple&& fn, queue_ptr queue, detail::tag_2 )
	override
	{
		Q_THROW( 1 ); // Just for 'override' compile-time reasons
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< T >&& exp )
		{
			return fn( exp.consume( ) );
		}, std::move( queue ) );
	}

	q::promise< >
	_receive( async_receiver_type_tuple&& fn, queue_ptr queue, detail::tag_4 )
	override
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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< void >&& exp )
		{
			exp.consume( );
			return fn( );
		}, std::move( queue ) );
	}

	q::promise< >
	_receive(
		receive_type_objectified&& fn, queue_ptr queue, detail::tag_2 )
	override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< void >&& exp )
		{
			exp.consume( );
			return fn( void_t( ) );
		}, std::move( queue ) );
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< void >&& exp )
		{
			exp.consume( );
			return fn( );
		}, std::move( queue ) );
	}

	q::promise< >
	_receive(
		async_receive_type_objectified&& fn,
		queue_ptr queue,
		detail::tag_4 )
	override
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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< void_t >&& exp )
		{
			return fn( exp.consume( ) );
		}, std::move( queue ) );
	}

	q::promise< >
	_receive( receive_type_void&& fn, queue_ptr queue, detail::tag_2 )
	override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< void_t >&& exp )
		{
			exp.consume( );
			return fn( );
		}, std::move( queue ) );
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( [ fn{ std::move( fn ) } ]( q::expect< void_t >&& exp )
		{
			return fn( exp.consume( ) );
		}, std::move( queue ) );
	}

	q::promise< >
	_receive(
		async_receive_type_void&& fn, queue_ptr queue, detail::tag_4 )
	override
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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( receiver_type_tuple&& fn, queue_ptr queue, detail::tag_2 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive(
		async_receiver_type_tuple&& fn,
		queue_ptr queue,
		detail::tag_4 )
	override
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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive(
		receive_type_objectified&& fn, queue_ptr queue, detail::tag_2
	)
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive(
		async_receive_type_objectified&& fn,
		queue_ptr queue,
		detail::tag_4
	)
	override
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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( receive_type_void&& fn, queue_ptr queue, detail::tag_2 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive(
		async_receive_type_void&& fn, queue_ptr queue, detail::tag_4
	)
	override
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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( receiver_type_tuple&& fn, queue_ptr queue, detail::tag_2 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive(
		async_receiver_type_tuple&& fn,
		queue_ptr queue,
		detail::tag_4
	)
	override
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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive(
		receive_type_objectified&& fn,
		queue_ptr queue,
		detail::tag_2
	)
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive(
		async_receive_type_objectified&& fn,
		queue_ptr queue,
		detail::tag_4
	)
	override
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

	q::promise< >
	_receive( receive_type&& fn, queue_ptr queue, detail::tag_1 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( receive_type_void&& fn, queue_ptr queue, detail::tag_2 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive( async_receive_type&& fn, queue_ptr queue, detail::tag_3 )
	override
	{
		return readable_.receive( )
		.then( std::move( fn ), std::move( queue ) );
	}

	q::promise< >
	_receive(
		async_receive_type_void&& fn, queue_ptr queue, detail::tag_4 )
	override
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
