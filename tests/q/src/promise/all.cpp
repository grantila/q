
#include "../core.hpp"

Q_TEST_MAKE_SCOPE( promise_all_test );

TEST_F( promise_all_test, all_different_voids )
{
	std::atomic< int > incremented( 0 );

	auto prom1 = q::with( queue )
	.then( [ &incremented ]( )
	{
		++incremented;
	} );
	auto prom2 = q::with( queue ).share( )
	.then( [ &incremented ]( )
	{
		++incremented;
	} );
	auto prom3 = q::with( queue )
	.then( [ &incremented ]( )
	{
		++incremented;
	} ).share( );
	auto prom4 = q::with( queue ).share( )
	.then( [ &incremented ]( )
	{
		++incremented;
	} ).share( );

	run(
		q::all( prom1, prom2, prom3, prom4 )
		.then( [ &incremented ]( )
		{
			incremented += incremented.load( );
		} )
	);

	EXPECT_EQ( 8, incremented.load( ) );
}

TEST_F( promise_all_test, all_same_voids )
{
	std::atomic< std::size_t > incremented( 0 );

	auto qu = queue;

	auto make_promise = [ &incremented, qu ]( )
	{
		return q::with( qu )
		.then( [ &incremented ]( )
		{
			++incremented;
		} );
	};

	std::vector< q::promise< > > promises;
	std::size_t iterations = 10;

	for ( std::size_t i = 0; i < iterations; ++i )
		promises.push_back( make_promise( ) );

	run(
		q::all( promises, qu )
		.then( [ &incremented ]( )
		{
			incremented += incremented.load( );
		} )
	);

	EXPECT_EQ( iterations * 2, incremented.load( ) );
}
