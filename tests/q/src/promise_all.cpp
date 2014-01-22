
#include <q/promise.hpp>
#include <q/execution_context.hpp>
#include <q/scope.hpp>

#include <gtest/gtest.h>

class PromiseAllTest
: public ::testing::Test
{
public:
	PromiseAllTest( )
	: scope_( nullptr )
	{ }

protected:
	virtual void SetUp( )
	{
		bd = q::make_execution_context< q::blocking_dispatcher >( "all" );
		auto prev_queue = q::set_default_queue( bd->queue( ) );
		auto reset_queue = q::make_scoped_function( [ prev_queue ]( )
		{
			q::set_default_queue( prev_queue );
		} );
		scope_ = std::move( reset_queue );
	}

	virtual void TearDown( )
	{
		scope_ = std::move( q::make_scope( nullptr ) );
		bd.reset( );
	}

	std::shared_ptr< q::specific_execution_context< q::blocking_dispatcher > > bd;

private:
	q::scope scope_;
};


TEST_F( PromiseAllTest, AllDifferentVoids )
{
	std::atomic< int > incremented( 0 );

	auto prom1 = q::with( )
	.then( [ &incremented ]( )
	{
		++incremented;
	} );
	auto prom2 = q::with( ).share( )
	.then( [ &incremented ]( )
	{
		++incremented;
	} );
	auto prom3 = q::with( )
	.then( [ &incremented ]( )
	{
		++incremented;
	} ).share( );
	auto prom4 = q::with( ).share( )
	.then( [ &incremented ]( )
	{
		++incremented;
	} ).share( );

	q::all( prom1, prom2, prom3, prom4 )
	.then( [ &incremented ]( )
	{
		incremented += incremented.load( );
	} )
	.then( [ this ]( )
	{
		bd->impl( )->terminate( q::termination::linger );
	} );

	bd->impl( )->start( );

	EXPECT_EQ( 8, incremented.load( ) );
}

TEST_F( PromiseAllTest, AllSameVoids )
{
	std::atomic< int > incremented( 0 );

	auto make_promise = [ &incremented ]( )
	{
		return q::with( )
		.then( [ &incremented ]( )
		{
			++incremented;
		} );
	};

	std::vector< q::promise< std::tuple< > > > promises;
	std::size_t iterations = 10;

	for ( std::size_t i = 0; i < iterations; ++i )
		promises.push_back( make_promise( ) );

	q::all( promises )
	.then( [ &incremented ]( )
	{
		incremented += incremented.load( );
	} )
	.then( [ this ]( )
	{
		bd->impl( )->terminate( q::termination::linger );
	} );

	bd->impl( )->start( );
	
	EXPECT_EQ( iterations * 2, incremented.load( ) );
}
