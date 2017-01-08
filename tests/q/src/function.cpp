
#include <q/function.hpp>

#include <q-test/q-test.hpp>

int call_count;

struct non_copyable
{
	non_copyable( ) = default;
	non_copyable( non_copyable&& ) = default;
	non_copyable( const non_copyable& ) = delete;
};

struct non_trivial
{
	non_trivial( ) = delete;
	non_trivial( int ) { }
	non_trivial( non_trivial&& ) = default;
	non_trivial( const non_trivial& ) = default;
};

template< bool Mutable, typename T = void >
struct lambda
{
	typename std::decay< T >::type t_;
	void operator( )( ) { ++call_count; }
};

template< typename T >
struct lambda< false, T >
{
	typename std::decay< T >::type t_;
	void operator( )( ) const { ++call_count; }
};

template< >
struct lambda< true, void >
{
	void operator( )( ) { ++call_count; }
};

template< >
struct lambda< false, void >
{
	void operator( )( ) const { ++call_count; }
};

template< bool Mutable = true, typename T >
auto make_lambda_11( T&& t ) -> lambda< Mutable, T >
{
	return lambda< Mutable, T >{ std::forward< T >( t ) };
}

template< bool Mutable = true >
auto make_lambda_11( ) -> lambda< Mutable >
{
	return lambda< Mutable >( );
}

struct nothing { };

template< std::size_t Size >
struct _aligned_storage
{
	typename std::aligned_storage< Size >::type data_;
};

template< std::size_t Size, bool Copyable >
struct payload
: _aligned_storage< Size >
, std::conditional<
	Copyable,
	nothing,
	non_copyable
>::type
{ };

template< std::size_t Size, bool Mutable = true, bool Copyable = true >
auto make_lambda_11_size( ) -> lambda< Mutable, payload< Size, Copyable > >
{
	return make_lambda_11< Mutable >( payload< Size, Copyable >{ } );
}

void plain( ) { ++call_count; }


TEST( function, looks_like_a_function )
{
	q::unique_function< void( ) > u_void_void;
	q::function< void( ) > s_void_void;

	q::unique_function< q::promise< >( ) > u_promise_void;
	q::function< q::promise< >( ) > s_promise_void;

	q::unique_function< q::promise< >( int ) > u_promise_int;
	q::function< q::promise< >( int ) > s_promise_int;

	EXPECT_TRUE( q::is_function_t< decltype( u_void_void ) >::value );
	EXPECT_TRUE( q::is_function_t< decltype( s_void_void ) >::value );
	EXPECT_TRUE( q::is_function_t< decltype( u_promise_void ) >::value );
	EXPECT_TRUE( q::is_function_t< decltype( s_promise_void ) >::value );
	EXPECT_TRUE( q::is_function_t< decltype( u_promise_int ) >::value );
	EXPECT_TRUE( q::is_function_t< decltype( s_promise_int ) >::value );
}

TEST( function, nullptr )
{
	q::unique_function< void( ) > uf_uninitialized( nullptr );
	q::function< void( ) > f_uninitialized( nullptr );
	q::unused_variable( uf_uninitialized );
	q::unused_variable( f_uninitialized );
}

TEST( function, unique_function_arguments )
{
	q::unique_function< void( ) >
		uf_empty( [ ]( ) { } );

	q::unique_function< void( int ) >
		uf_one_arg_trivial( [ ]( int ) { } );

	q::unique_function< void( non_trivial ) >
		uf_one_arg_non_trivial( [ ]( non_trivial ) { } );

	q::unique_function< void( int, float ) >
		uf_two_args_trivial( [ ]( int, float ) { } );

	q::unique_function< void( int, float ) >
		uf_two_args_trivial_convertible( [ ]( int, non_trivial ) { } );

	q::unique_function< void( non_trivial, non_trivial ) >
		uf_two_args_non_trivial( [ ]( non_trivial, non_trivial ) { } );

	uf_empty( );
	uf_one_arg_trivial( 1 );
	uf_one_arg_non_trivial( 1 ); // Convertible
	uf_one_arg_non_trivial( non_trivial( 1 ) );
	uf_two_args_trivial( 1, 3.14 );
	uf_two_args_trivial( 1, (double)6.28 ); // Convertible
	uf_two_args_non_trivial( 1, 1 ); // Convertible
	uf_two_args_non_trivial( non_trivial( 1 ), non_trivial( 2 ) );
}

TEST( function, function_arguments )
{
	q::function< void( ) >
		f_empty( [ ]( ) { } );

	q::function< void( int ) >
		f_one_arg_trivial( [ ]( int ) { } );

	q::function< void( non_trivial ) >
		f_one_arg_non_trivial( [ ]( non_trivial ) { } );

	q::function< void( int, float ) >
		f_two_args_trivial( [ ]( int, float ) { } );

	q::function< void( int, float ) >
		f_two_args_trivial_convertible( [ ]( int, non_trivial ) { } );

	q::function< void( non_trivial, non_trivial ) >
		f_two_args_non_trivial( [ ]( non_trivial, non_trivial ) { } );

	f_empty( );
	f_one_arg_trivial( 1 );
	f_one_arg_non_trivial( 1 ); // Convertible
	f_one_arg_non_trivial( non_trivial( 1 ) );
	f_two_args_trivial( 1, 3.14 );
	f_two_args_trivial( 1, (double)6.28 ); // Convertible
	f_two_args_non_trivial( 1, 1 ); // Convertible
	f_two_args_non_trivial( non_trivial( 1 ), non_trivial( 2 ) );
}

TEST( function, throw_copy_constructor )
{
	std::string s = "hello world";
	auto throw_copy_constructor_lambda = [ s ]( ) { };

	q::unique_function< void( ) > uf( throw_copy_constructor_lambda );
	q::function< void( ) > f( throw_copy_constructor_lambda );
}

TEST( function, throw_move_constructor )
{
	struct TMC
	{
		constexpr TMC( ) = default;
		TMC( const TMC& ) = default;
		TMC( TMC&& )
		{ }
	} tmc;
	auto throw_move_constructor_lambda = [ tmc ]( ) { };

	q::unique_function< void( ) > uf( throw_move_constructor_lambda );
	q::function< void( ) > f( throw_move_constructor_lambda );
}

TEST( function, plain_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf( plain );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, plain_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf( plain );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2( plain );

	q::unique_function< void( ) > _uf = f;
	q::unique_function< void( ) > uf2 = f2;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, empty_immutable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11< false >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, empty_immutable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11< false >( ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2( make_lambda_11< false >( ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, empty_mutable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11< true >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, empty_mutable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11< true >( ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2( make_lambda_11< true >( ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, empty_immutable_non_copyable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11< false >( non_copyable( ) ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, empty_immutable_non_copyable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11< false >( non_copyable( ) ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2(
		make_lambda_11< false >( non_copyable( ) ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, empty_mutable_non_copyable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11< true >( non_copyable( ) ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, empty_mutable_non_copyable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11< true >( non_copyable( ) ) );

	q::function< void( ) > f = uf.share( );

	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	EXPECT_EQ( 3, call_count );
}

TEST( function, small_immutable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 8, false >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, small_immutable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 8, false >( ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2( make_lambda_11_size< 8, false >( ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, small_mutable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 8, true >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, small_mutable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 8, true >( ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2( make_lambda_11_size< 8, true >( ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, small_immutable_non_copyable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 8, false, false >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, small_immutable_non_copyable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 8, false, false >( ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2(
		make_lambda_11_size< 8, false, false >( ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, small_mutable_non_copyable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 8, false, false >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, small_mutable_non_copyable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 8, false, false >( ) );

	q::function< void( ) > f = uf.share( );

	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	EXPECT_EQ( 3, call_count );
}

TEST( function, medium_immutable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 96, false >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, medium_immutable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 96, false >( ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2( make_lambda_11_size< 96, false >( ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, medium_mutable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 96, true >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, medium_mutable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 96, true >( ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2( make_lambda_11_size< 96, true >( ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, medium_immutable_non_copyable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 96, false, false >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, medium_immutable_non_copyable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 96, false, false >( ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2(
		make_lambda_11_size< 96, false, false >( ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, medium_mutable_non_copyable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 96, false, false >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, medium_mutable_non_copyable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 96, false, false >( ) );

	q::function< void( ) > f = uf.share( );

	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	EXPECT_EQ( 3, call_count );
}

TEST( function, large_immutable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 256, false >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, large_immutable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 256, false >( ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2( make_lambda_11_size< 256, false >( ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, large_mutable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 256, true >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, large_mutable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf( make_lambda_11_size< 256, true >( ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2( make_lambda_11_size< 256, true >( ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, large_immutable_non_copyable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 256, false, false >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, large_immutable_non_copyable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 256, false, false >( ) );

	q::function< void( ) > f = uf.share( );
	q::function< void( ) > f2(
		make_lambda_11_size< 256, false, false >( ) );

	q::unique_function< void( ) > uf2 = f2;
	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	f2( );
	uf2( );
	EXPECT_EQ( 5, call_count );
}

TEST( function, large_mutable_non_copyable_lambda_uninitialized_unique )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 256, false, false >( ) );

	uf( );
	EXPECT_EQ( 1, call_count );

	q::unique_function< void( ) > uf2( std::move( uf ) );

	EXPECT_FALSE( uf );
	EXPECT_TRUE( uf2 );

	uf2( );
	EXPECT_EQ( 2, call_count );

	EXPECT_THROW( uf( ), q::bad_function_call );
	uf2 = q::unique_function< void( ) >( );
	EXPECT_THROW( uf2( ), q::bad_function_call );
}

TEST( function, large_mutable_non_copyable_lambda_unique_shared_conversion )
{
	call_count = 0;

	q::unique_function< void( ) > uf(
		make_lambda_11_size< 256, false, false >( ) );

	q::function< void( ) > f = uf.share( );

	q::unique_function< void( ) > _uf = f;

	uf( );
	f( );
	_uf( );
	EXPECT_EQ( 3, call_count );
}
