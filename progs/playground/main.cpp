
#include <q/type_traits.hpp>
#include <q/functional.hpp>
#include <q/log.hpp>
#include <q/promise.hpp>

#include <q/blocking_dispatcher.hpp>
#include <q/threadpool.hpp>
#include <q/scheduler.hpp>
#include <q/stacktrace.hpp>

#include <q/channel.hpp>

#include <q/q.hpp>

#include <string>
#include <iostream>

template< typename... Args >
void noop( Args&&... ) { }

template< typename T >
struct print;

template< typename T >
struct print< std::tuple< T > >
{
	static void it( const std::tuple< T >& t )
	{
		std::cout << std::get< 0 >( t ) << std::endl;
	}
};

template< typename T1, typename T2 >
struct print< std::tuple< T1, T2 > >
{
	static void it( const std::tuple< T1, T2 >& t )
	{
		std::cout << std::get< 0 >( t ) << ", " << std::get< 1 >( t ) << std::endl;
	}
};

template< typename... Args >
void f( Args&&... args )
{
	auto params = q::forward_decay_as_tuple( std::forward< Args >( args )... );

	print< decltype( params ) >::it( params );

	auto x = std::move( params );
	print< decltype( x ) >::it( params );
//	noop( params );
}



class Movable
{
public:
	Movable( ) = default;
	Movable( int i ) : i_( i ) { }
	Movable( const Movable& ) = delete;
	Movable& operator=( const Movable& ) = delete;

	Movable( Movable&& ref ) : i_( ref.i_ ) { }
	Movable& operator=( Movable&& ) { return *this; }

	int get( ) const { return i_; }

	void dummy( int, double ) { }
	void dummy_const( int, double, float ) const { }

	template< typename... >
	int variadic( );

private:
	int i_;
};

std::ostream& operator<<( std::ostream& os, const Movable& mov )
{
	return os << mov.get( );
}

void print_movable( Movable&& m )
{
	std::cout << "print_movable: " << m << std::endl;
}

Movable test_f( int i, double&& d, std::string s, Movable&& m )
{
	std::cout << "=====" << std::endl;
	std::cout << i << std::endl;
	std::cout << d << std::endl;
	std::cout << s << std::endl;
	std::cout << "=====" << std::endl;
	return std::move( m );
}

int g( )
{
	std::cout << "g called" << std::endl;
	return 0;
}

template< typename... T >
void var_g( T&&... t )
{
//	std::cout << "var_g: " << std::forward_as_tuple( std::forward< T >( t )... ) << std::endl;
}

template< typename... T >
void var_f( T&&... t )
{
	var_g( std::forward< T >( t )... );
}

std::string s__;
const std::string& return_const_lvalue_ref( ) { return s__; }

void do_something( const std::string&& s ) { }


template< typename T >
void UNUSED( T ) { }

std::tuple< std::string > return_string_tuple( )
{ return std::make_tuple( "test" ); }





/*
template< typename Fn, typename... Args >
struct curry
: curry< Fn, 
{
	operator( )( Args... args )
	{
		fn( args );
	}

private:
	Fn fn;
};
*/


/*
class make_execution_context
{
public:


private:
	event_dispatcher_ptr event_dispatcher_;
	scheduler_ptr scheduler_;
	queue_ptr queue_;
//	auto tpd = q::threadpool::construct( "pool" );
};

template< typename EventDispatcher >
make_execution_context(  )
*/

q::scope initialize( )
{
	q::settings settings;
	settings.set_long_stack_support( true );

	return q::scoped_initialize( settings );
}

struct my_exception : q::exception { };

void e_g( )
{
	Q_THROW( my_exception( ) );
}

void e_f( )
{
	e_g( );
}

int main( int argc, char** argv )
{
	std::string s = "Hello world";
	int i = 4711;
	double d = 3.1415;
	Movable m( i );

	auto q_scope = initialize( );

	std::cout << "The computer has "
		<< q::processors( ) << " processors" << std::endl;

	auto bd = q::make_shared< q::blocking_dispatcher >( "main" );
	auto queue = q::make_shared< q::queue >( 0 );

	auto sched = q::make_shared< q::direct_scheduler >( bd );
	sched->add_queue( queue );

	auto error_stuff  = q::with( queue ).share( );
	auto thread_stuff = q::with( queue );

	{
		auto t = q::run( "test thread", queue, [ ]( ) -> std::string
		{
			std::cerr << "thread running" << std::endl;
			return "hello";
		} );

		thread_stuff = thread_stuff
		.then( [ t ]( ) mutable
		{
			return t->terminate( );
		} )
		.then( [ ]( q::expect< std::string >&& ret )
		{
			std::cerr << "thread terminated ";
			if ( ret.has_exception( ) )
				std::cerr
					<< "with exception: "
					<< q::stream_exception( ret.exception( ) )
					<< std::endl;
			else
				std::cerr
					<< "successfully: "
					<< ret.consume( ) << std::endl;
		} );
	}

	auto error_stuff2 = error_stuff
	.then( [ ]( )
	{
		e_f( );
	} )
	.fail( [ queue ]( std::exception_ptr e ) -> q::promise< std::tuple< > >
	{
		try
		{
			std::rethrow_exception( e );
		}
		catch ( my_exception& e )
		{
			auto info = e.get_info< q::stacktrace >( );
			if ( info )
			{
				auto st = info->consume( );
				std::cerr << st << std::endl;
			}
		}
		return q::with( queue );
	} )
	;

	std::cerr << "is promise: " << q::is_promise< decltype( thread_stuff ) >::value << std::endl;
	std::cerr << "are promise: " << q::are_promises< decltype( thread_stuff ) >::value << std::endl;
	std::cerr << "is promise: " << q::is_promise< decltype( error_stuff2 ) >::value << std::endl;
	std::cerr << "are promise: " << q::are_promises< decltype( error_stuff2 ) >::value << std::endl;
	std::cerr << "are promises: " << q::are_promises< decltype( error_stuff2 ), decltype( thread_stuff ) >::value << std::endl;
	std::cerr << "are promises: " << q::are_promises< decltype( error_stuff2 ), std::true_type >::value << std::endl;
	std::cerr << "are promises2: " << q::are_promises< q::promise< std::tuple< > >, q::promise< std::tuple< > > >::value << std::endl;

	q::all( std::move( error_stuff2 ), std::move( thread_stuff ) )
	.then( [ bd ]( )
	{
		//    return bd->terminate( q::event_dispatcher::termination::linger );
		return bd->terminate( q::termination::annihilate );
	} )
	;


	std::cout << "thread completed" << std::endl;

	std::cerr << "backlog b" << std::endl;

	{
		auto testpool = q::make_shared< q::threadpool >(
			"testpool", queue );
		testpool->terminate( q::termination::linger ).then( [ ]( )
		{
			std::cerr << "thread pool destroyed" << std::endl;
		} );
	}
	std::cerr << "threadpool created and destroyed" << std::endl;

	auto tpd = q::make_shared< q::threadpool >( "pool", queue );
	auto bg_queue = q::make_shared< q::queue >( 0 );

	auto bg_sched = q::make_shared< q::direct_scheduler >( tpd );
	bg_sched->add_queue( bg_queue );

	auto testfun = [ ]( ) -> std::tuple< std::string >
	{
		std::string word = "cow";
		std::cout << "then called, returning: " << word << std::endl;
		return std::make_tuple( word );
	};

	Q_RESULT_OF_AS_ARGUMENT( decltype( testfun ) )* testptr = nullptr;

	var_f( *testptr );

	auto testfn = [ ]( std::string&& s ) { ; };
	typedef decltype( testfn ) fn_type;
	typedef std::tuple< std::tuple< std::string > > my_type;
	bool are_same = Q_ARGUMENTS_ARE( fn_type, my_type )::value;

	bool is_arg_same = ::q::is_argument_same_or_convertible<
		Q_ARGUMENTS_OF( fn_type ),
		::q::arguments< my_type >
	>::value;

	std::cout << "arguments are same: " << are_same << ", " << is_arg_same << std::endl;
	std::cout << typeid( Q_ARGUMENTS_OF( fn_type )::first_type ).name( ) << std::endl;
	std::cout << typeid( ::q::arguments< my_type >::first_type ).name( ) << std::endl;


	std::function< void( std::string&& ) > string_to_void = [ bd ]( std::string&& s ) mutable
	{
		std::string msg = s;
		// std::string msg = std::get< 0 >( s );
		std::cout << "then got \"" << msg << "\", .stopping..." << std::endl;
	};

	typedef std::tuple< std::string >                    type_from;
	typedef Q_ARGUMENTS_OF( decltype( string_to_void ) ) type_to;

	auto is_conv = ::q::tuple_arguments< type_from >::is_convertible_to< type_to >::value;
	std::cout << "::::: " << is_conv << std::endl;


	std::function< Movable( ) > movable_fn = [ ]( ) -> Movable
	{
		Movable m( 1234 );
		return std::move( m );
	};

	std::tuple< Movable > t( movable_fn( ) );
	var_f( movable_fn( ) );
	q::call_with_args_by_tuple( print_movable, std::move( t ) );


	auto prom = q::with( queue )
	/* */
	.then( [ ]( ) -> std::string
	{
		std::string word = "cow";
		std::cout << "then called, returning: " << word << std::endl;
		// return std::make_tuple( word );
		return std::move( word );
	} )
	.then( string_to_void )
	.then( [ ]( )
	{
		return 0.5;
	} );
	/* */

	auto bg_prom = q::with( queue, 5 )
	.then( [ ]( int i ) -> int
	{
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
		std::cout << "background thread got " << i << std::endl;
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
		return i * 2;
	}, bg_queue )
	.then( [ ]( int i )
	{
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
		std::cout << "background thread got " << i << std::endl;
		std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
	}, bg_queue );

	auto shared_prom = prom.share( );
/* */
	shared_prom.then( [ ]( double d )
	{
		std::cout << "got " << d << std::endl;
	} );

	shared_prom.then( [ ]( double d )
	{
		std::cout << "got " << d << " again" << std::endl;
	} );
/* */
	/* */
	auto prom2 = bg_prom
	.then( [ ]( ) -> std::tuple< Movable, Movable >
	{
		std::cout << "has waited for background threads" << std::endl;
		Movable m1( 1234 );
		Movable m2( 5678 );
		return std::make_tuple( std::move( m1 ), std::move( m2 ) );
	} );

	prom2
	.then( [ ]( Movable&& m1, Movable&& m2 )
	{
		std::cout << "then movable: " << m1 << m2 << std::endl;
	} )
	.then( [ tpd ]( )
	{
		return tpd->terminate( q::termination::linger );
	} )
	.then( [ bd ]( )
	{
		return bd->terminate( q::termination::linger );
	} )
	/* */
	;

	auto chan = q::make_shared< q::channel< int, std::string > >( queue, 2 );
	chan->send( 12, std::string( "years old whiskey" ) );
	chan->send( 12, "years old whiskey" );
	chan->send( 99, "luftballoons" );

	auto chan_out = [ ]( int i, std::string s )
	{
		std::cout << i << " " << s << std::endl;
	};

	chan->receive( ).then( chan_out );
	chan->receive( ).then( chan_out );
	chan->receive( ).then( chan_out );

	shared_prom
	.then( [ chan ]( double )
	{
		chan->send( 1, "earth" );
	} );

	std::cerr << "backlog b" << std::endl;

	bd->start( );

//	return 0;

	std::cerr << "backlog c" << std::endl;

	std::tuple< const std::string& > s_tuple( s );
	s = "hello there";
	std::cout << std::get< 0 >( s_tuple ) << std::endl;
	do_something( std::move( return_const_lvalue_ref( ) ) );

	typedef q::function_traits< decltype( &Movable::variadic< int > ) > variadic_traits;

	var_f( i );
	var_f( i, s, d );
	var_f( );

	auto auto_params = std::make_tuple( std::string( "hello" ), 47 );

//	q::call_with_args( var_f, auto_params );

	std::cout
		<< Q_ARITY_OF( decltype( &Movable::dummy ) )
		<< ", "
		<< Q_ARITY_OF( decltype( &Movable::dummy_const ) )

#ifndef LIBQ_ON_WINDOWS
		<< ", "
		<< variadic_traits::arity::value
		<< ", "
		<< typeid( variadic_traits::type ).name( )
#endif

//		<< ", "
//		<< typeid( variadic_traits::deduced_type ).name( )
		<< ", "
		<< std::endl;

	q::call_with_args( g );

	auto args = std::make_tuple( i, std::move( d ), s, std::move( m ) );

	auto x = q::call_with_args_by_tuple( test_f, std::move( args ) );
	std::cout << "return: " << x.get( ) << std::endl;

	f( s );
	f( i );
	f( s, i );
	f( std::move( s ), std::move( i ) );
	f( std::move( s ), std::move( i ) );

	return 0;
}
