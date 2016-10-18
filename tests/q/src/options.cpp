
#include <q/type_traits.hpp>

#include <q-test/q-test.hpp>

#include <q/options.hpp>
#include <q/queue.hpp>
#include <q/concurrency.hpp>

template< typename... T >
void unused( T&&... ) { }

TEST( options, zero_types )
{
	typedef q::options< > options;

	auto fun = [ ]( options opts = options( ) ) { };

	fun( );
	fun( { } );

	options opts;
	unused( opts );
}

TEST( options, one_type )
{
	typedef q::options< q::concurrency > options;

	auto fun = [ ]( options opts = options( ) ) { return opts; };

	auto fun_opts1 = fun( );
	EXPECT_FALSE( fun_opts1.has< q::concurrency >( ) );
	EXPECT_EQ( 1, fun_opts1.get< q::concurrency >( ).get( ) );

	auto fun_opts2 = fun( { q::concurrency( 4 ) } );
	EXPECT_TRUE( fun_opts2.has< q::concurrency >( ) );
	EXPECT_EQ(
		fun_opts2.get< q::concurrency >( ).get( ),
		q::concurrency( 4 ).get( )
	);
}

TEST( options, two_types )
{
	typedef q::options< q::concurrency, q::queue_ptr > options;

	auto fun = [ ]( options opts = options( ) ) { return opts; };

	auto fun_opts1 = fun( );
	EXPECT_FALSE( fun_opts1.has< q::concurrency >( ) );
	EXPECT_FALSE( fun_opts1.has< q::queue_ptr >( ) );
	EXPECT_EQ( 1, fun_opts1.get< q::concurrency >( ).get( ) );

	auto fun_opts2 = fun( { } );
	EXPECT_FALSE( fun_opts1.has< q::concurrency >( ) );
	EXPECT_FALSE( fun_opts1.has< q::queue_ptr >( ) );
	EXPECT_NE( fun_opts2.get< q::concurrency >( ).get( ), 0 );
	EXPECT_NE( fun_opts2.get< q::concurrency >( ).get( ), 4 );

	auto fun_opts3 = fun( { q::concurrency( ) } );
	EXPECT_TRUE( fun_opts3.has< q::concurrency >( ) );
	EXPECT_FALSE( fun_opts3.has< q::queue_ptr >( ) );

	auto fun_opts4 = fun( { q::queue_ptr( ), q::concurrency( 4 ) } );
	EXPECT_TRUE( fun_opts4.has< q::concurrency >( ) );
	EXPECT_TRUE( fun_opts4.has< q::queue_ptr >( ) );
	EXPECT_EQ( fun_opts4.get< q::concurrency >( ).get( ), 4 );

	options opts1;
	EXPECT_FALSE( opts1.has< q::concurrency >( ) );
	EXPECT_FALSE( opts1.has< q::queue_ptr >( ) );

	options opts2( { q::concurrency( ) } );
	EXPECT_TRUE( opts2.has< q::concurrency >( ) );
	EXPECT_FALSE( opts2.has< q::queue_ptr >( ) );
}

TEST( options, required_type )
{
	typedef q::options< q::concurrency, q::required< q::queue_ptr > >
		options;

	auto fun = [ ]( options opts ) { return opts; };

	/*
	// Shouldn't compile
	auto fun_opts_invalid = fun( { } );
	EXPECT_TRUE( false );
	/* */

	auto fun_opts1 = fun( { q::queue_ptr( ) } );
	EXPECT_FALSE( fun_opts1.has< q::concurrency >( ) );
	EXPECT_TRUE( fun_opts1.has< q::queue_ptr >( ) );

	auto fun_opts2 = fun( { q::queue_ptr( ), q::concurrency( 4 ) } );
	EXPECT_TRUE( fun_opts2.has< q::concurrency >( ) );
	EXPECT_TRUE( fun_opts2.has< q::queue_ptr >( ) );
	EXPECT_EQ( fun_opts2.get< q::concurrency >( ).get( ), 4 );
}
