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

#ifndef LIBQ_THREAD_HPP
#define LIBQ_THREAD_HPP

#include <q/async_termination.hpp>

#include <q/detail/lib.hpp>

namespace q {

/**
 * @returns the number of hard (real) cores available on this machine
 */
std::size_t hard_cores( );

/**
 * @returns the number of soft cores (aka "threads") available on this machine
 */
std::size_t soft_cores( );

namespace detail {

void set_thread_name( const std::string& name );
std::string get_thread_name( );

} // namespace detail


// TODO: Reconsider
// thread* current_thread( );

/**
 * ...
 */
template< typename Ret = void >
class thread
: public std::enable_shared_from_this< thread< Ret > >
, public async_termination<
	q::arguments< >,
	std::tuple< expect< Ret > >
>
{
public:
	typedef expect< Ret >             expect_type;
	typedef std::tuple< expect_type > result_type;
	typedef async_termination<
		q::arguments< >,
		result_type
	>                                 async_terminate_base;

	thread( ) = delete;
	thread( thread< Ret >&& ) = delete;
	thread( const thread< Ret >& ) = delete;
	thread& operator=( const thread< Ret >& ) = delete;

	virtual ~thread( ) noexcept
	{
		std::cerr << "destructing thread" << std::endl;
		if ( !try_join( ) )
			// We end up here if and only if the thread function
			// itself holds the last reference to 'this'.
			// This happens only if terminate( ) has not been
			// called, and in such case, the thread must be
			// detached and left dangling unmanaged for a short
			// amount of time. We won't be able to wait for it to
			// complete which is unfortunate.
			thread_.detach( );
	}

	std::string name( ) const
	{
		return name_;
	}

	promise< result_type > terminate( )
	{
		auto _this = this->shared_from_this( );

		return async_terminate_base::terminate( );
	}

	q::expect< > await_termination( )
	{
		try_join( );
		return q::fulfill< void >( );
	}

	template< typename Fn, typename... Args >
	static typename std::enable_if<
		std::is_same< Q_RESULT_OF( Fn ), Ret >::value,
		std::shared_ptr< thread< Ret > >
	>::type
	construct( std::string name, const queue_ptr& queue,
		Fn&& fn, Args&&... args )
	{
		auto self = std::shared_ptr< thread< Ret > >(
			new thread< Ret >( std::move( name ), queue ) );

		self->run(
			std::forward< Fn >( fn ),
			std::forward< Args >( args )...
		);

		return self;
	}

protected:
	thread( std::string&& name, const queue_ptr& queue )
	: async_terminate_base( queue )
	, name_( std::move( name ) )
	, running_( false )
	, self_ref_( nullptr )
	{ }

	template< typename Fn, typename... Args >
	typename std::enable_if<
		std::is_same< Q_RESULT_OF( Fn ), Ret >::value
	>::type
	run( Fn&& fn, Args&&... args )
	{
		auto tmp_fn = make_temporarily_copyable(
			std::forward< Fn >( fn ) );

		auto params = forward_decay_as_tuple(
			std::forward< Args >( args )... );

		auto tmp_params = make_temporarily_copyable(
			std::move( params ) );

		auto _this = this->shared_from_this( );
		self_ref_ = _this;

		auto thread_fn = [ _this, tmp_fn, tmp_params ]( ) mutable
		{
			::q::detail::set_thread_name( _this->name_ );

			expect_type expect;

			auto fn = [ tmp_fn, tmp_params ]( ) mutable
			{
				return ::q::call_with_args_by_tuple(
					tmp_fn.consume( ),
					tmp_params.consume( ) );
			};

			try
			{
				expect = ::q::call_with_args_by_fun(
					fulfill< Ret >, fn );
			}
			catch ( ... )
			{
				// Consider only one of these
				expect = refuse< Ret >(
					std::current_exception( ) );
				LIBQ_UNCAUGHT_EXCEPTION(
					std::current_exception( ) );
			}

			/* This might seem unsafe, but the destructor of the
			 * class will ensure we join (hence wait for this
			 * operation to complete, which is fast, and at most
			 * will involve just a context switch).
			 */
			_this->termination_done(
				std::make_tuple( std::move( expect ) ) );
			_this.reset( );
		};

		thread_ = std::thread( thread_fn );
		running_.store( true, std::memory_order_seq_cst );
	}

private:
	// This won't do anything. Termination is acknowledged within the
	// thread function instead.
	void do_terminate( ) override
	{
		self_ref_.reset( );
	}

	bool try_join( )
	{
		if ( !running_.load( std::memory_order_seq_cst ) )
			return true;

		if ( thread_.joinable( ) )
		{
			running_.store( false, std::memory_order_seq_cst );
			thread_.join( );
			return true;
		}

		return false;
	}

	std::string                      name_;
	std::thread                      thread_;
	std::atomic< bool >              running_;
	std::shared_ptr< thread< Ret > > self_ref_;
};

/**
 * This is a reusable thread on which multiple tasks can be dispatched.
 * Each task can have its own set of arguments (and types).
 */
class versatile_thread
: public thread< >
{
public:
	versatile_thread( const std::string& name );
	versatile_thread( versatile_thread&& );
	versatile_thread( const versatile_thread& ) = delete;
	~versatile_thread( );

	template< typename Fn, typename... Args >
	void add_task( Fn&& fn, Args&&... args );

private:
	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};


template< typename Fn, typename... Args >
std::shared_ptr< thread< Q_RESULT_OF( Fn ) > >
run( std::string name, const queue_ptr& queue, Fn&& fn, Args&&... args )
{
	return thread< Q_RESULT_OF( Fn ) >::construct(
	//return q::make_shared< thread< Q_RESULT_OF( Fn ) > >(
		std::move( name ),
		queue,
		std::forward< Fn >( fn ),
		std::forward< Args >( args )... );
}

} // namespace q

#endif // LIBQ_THREAD_HPP
