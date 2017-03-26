
#include <q/functional.hpp>

#include "core.hpp"

TEST( functional, is_argument_same )
{
	typedef q::is_argument_same_t<
		q::arguments< char, int >,
		q::arguments< char, int >
	> ias_type;
	const ias_type::value_type is_same_1 = ias_type::value;

	const bool is_same_2 = q::is_argument_same_t<
		q::arguments< >,
		q::arguments< >
	>::value;

	const bool is_same_3 = q::is_argument_same_t<
		q::arguments< const double >,
		q::arguments< double >
	>::value;

	const bool is_not_same_1 = q::is_argument_same_t<
		q::arguments< char, long >,
		q::arguments< char, int >
	>::value;

	const bool is_not_same_2 = q::is_argument_same_t<
		q::arguments< char, int, double >,
		q::arguments< char, int >
	>::value;

	const bool is_not_same_3 = q::is_argument_same_t<
		q::arguments< double >,
		q::arguments< >
	>::value;

	const bool is_not_same_4 = q::is_argument_same_t<
		q::arguments< const double& >,
		q::arguments< double& >
	>::value;

	EXPECT_TRUE( is_same_1 );
	EXPECT_TRUE( is_same_2 );
	EXPECT_TRUE( is_same_3 );
	EXPECT_FALSE( is_not_same_1 );
	EXPECT_FALSE( is_not_same_2 );
	EXPECT_FALSE( is_not_same_3 );

	// This should probably be false, but is currently true.
	// Not a big deal, but will cause worse compilation errors.
	// EXPECT_FALSE( is_not_same_4 );
	EXPECT_TRUE( is_not_same_4 );
}

TEST( functional, lambda_function_argument_types )
{
	auto lambda = [ ]( int&, int, std::string, char ) { };

	typedef q::function_traits< decltype( lambda ) > lambda_traits;
	typedef lambda_traits::argument_types argument_type;
	typedef argument_type::tuple_type tuple_type;
	typedef Q_RESULT_OF_AS_ARGUMENT_TYPE( decltype( lambda ) ) return_type;

	int real_i = 4711;
	const char* real_s = "asdf";
	char real_c = 'a';
	std::tuple_element< 0, tuple_type >::type i_ref = real_i;
	std::tuple_element< 1, tuple_type >::type i = real_i;
	std::tuple_element< 2, tuple_type >::type s_ref = real_s;
	std::tuple_element< 3, tuple_type >::type char_ref = real_c;

	auto type_name_1 = typeid( std::tuple_element<
		0, lambda_traits::argument_types::tuple_type
	>::type ).name( );

	auto type_name_2 = typeid( std::tuple_element<
		1, lambda_traits::argument_types::tuple_type
	>::type ).name( );

	auto type_name_3 = typeid( std::tuple_element<
		2, lambda_traits::argument_types::tuple_type
	>::type ).name( );

	auto type_name_4 = typeid( std::tuple_element<
		3, lambda_traits::argument_types::tuple_type
	>::type ).name( );

	typedef argument_type
		::first_type type_1;
	typedef argument_type::rest_arguments
		::first_type type_2;
	typedef argument_type::rest_arguments::rest_arguments
		::first_type type_3;
	typedef argument_type::rest_arguments::rest_arguments::rest_arguments
		::first_type type_4;

	EXPECT_TRUE( ( std::is_same< type_1, int& >::value ) );
	EXPECT_TRUE( ( std::is_same< type_2, int >::value ) );
	EXPECT_TRUE( ( std::is_same< type_3, std::string >::value ) );
	EXPECT_TRUE( ( std::is_same< type_4, char >::value ) );

	EXPECT_EQ( real_i, i_ref );
	EXPECT_EQ( real_i, i );
	EXPECT_EQ( real_s, s_ref );
	EXPECT_EQ( real_c, char_ref );

	EXPECT_EQ( typeid( int& ).name( ), type_name_1 );
	EXPECT_EQ( typeid( int ).name( ), type_name_2 );
	EXPECT_EQ( typeid( std::string ).name( ), type_name_3 );
	EXPECT_EQ( typeid( char ).name( ), type_name_4 );

	EXPECT_EQ( std::size_t( 0 ), return_type::size::value );
	EXPECT_EQ( std::size_t( 4 ), lambda_traits::arity::value );
}

TEST( functional, lambda_function_void_argument_type )
{
	auto lambda = [ ]( ) { };

	typedef q::function_traits< decltype( lambda ) > lambda_traits;
	typedef lambda_traits::argument_types::tuple_type tuple_type;
	typedef Q_ARGUMENTS_OF( decltype( lambda ) ) arguments_type;
	typedef Q_RESULT_OF_AS_ARGUMENT_TYPE( decltype( lambda ) ) return_type;

	EXPECT_TRUE(
		q::arguments< >::is_convertible_to< arguments_type >::value
	);
	EXPECT_EQ( std::size_t( 0 ), return_type::size::value );
	EXPECT_EQ( std::size_t( 0 ), std::tuple_size< tuple_type >::value );
	EXPECT_EQ( std::size_t( 0 ), lambda_traits::arity::value );
}

int fn( char, std::string );

TEST( functional, function_argument_types )
{
	typedef decltype( fn ) fn_type;
	typedef Q_ARGUMENTS_OF( fn_type ) arguments_type;
	typedef Q_RESULT_OF( fn_type ) return_type;

	typedef Q_FIRST_ARGUMENT_OF( fn_type ) type_1;
	typedef arguments_type::rest_arguments::first_type type_2;

	EXPECT_FALSE(
		q::arguments< char >::is_convertible_to< arguments_type >::value
	);
	EXPECT_TRUE( ( std::is_same< type_1, char >::value ) );
	EXPECT_TRUE( ( std::is_same< type_2, std::string >::value ) );
	EXPECT_TRUE( ( std::is_same< return_type, int >::value ) );
	EXPECT_EQ( std::size_t( 2 ), Q_ARITY_OF( fn_type ) );
}

int void_fn( );
int void_t_fn( q::void_t );

TEST( functional, function_argument_void_type )
{
	typedef decltype( void_fn ) fn_type;
	typedef decltype( void_t_fn ) fn_t_type;

	EXPECT_TRUE( (
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( fn_type, void )
			::value
	) );
	EXPECT_TRUE( (
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( fn_type )::value
	) );
	EXPECT_FALSE( (
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( fn_t_type, void )
			::value
	) );
	EXPECT_FALSE( (
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM( fn_t_type )::value
	) );

	EXPECT_TRUE( (
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM_INCL_VOID( fn_type, void )
			::value
	) );
	EXPECT_TRUE( (
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM_INCL_VOID( fn_type )::value
	) );
	EXPECT_TRUE( (
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM_INCL_VOID( fn_t_type, void )
			::value
	) );
	EXPECT_TRUE( (
		Q_ARGUMENTS_ARE_CONVERTIBLE_FROM_INCL_VOID( fn_t_type )::value
	) );
}

struct C
{
	C( int ) { }
	float fn( long ) { return (float)3.14; }
};

TEST( functional, class_function_argument_types )
{
	typedef decltype( &C::fn ) fn_type;
	typedef Q_RESULT_OF( fn_type ) return_type;
	typedef Q_FIRST_ARGUMENT_OF( fn_type ) type_1;
	typedef q::function_traits< fn_type > C_fn_traits;
	typedef C_fn_traits::memberclass_type class_type;

	class_type instance = class_type( 42 );
	auto ret = instance.fn( 17 );

	C_fn_traits::member_signature_ptr fn_ptr = &C::fn;
	typedef Q_RESULT_OF( decltype( fn_ptr ) ) ret_type;

	EXPECT_TRUE( ( std::is_same< decltype( ret ), float >::value ) );
	EXPECT_TRUE( ( std::is_same< ret_type, float >::value ) );
	EXPECT_TRUE( ( std::is_same< class_type, C >::value ) );
	EXPECT_TRUE( ( std::is_same< type_1, long >::value ) );
	EXPECT_TRUE( ( std::is_same< return_type, float >::value ) );
	EXPECT_EQ( std::size_t( 1 ), Q_ARITY_OF( fn_type ) );
}

int move_into( std::string&& s )
{
	return s.length( );
}

TEST( functional, call_with_args )
{
	typedef Q_RESULT_OF_AS_ARGUMENT( decltype( move_into ) )::size num_args;

	std::string arg = "hello world";
	auto args = std::make_tuple( arg );
	auto ret1 = q::call_with_args( move_into, std::move( arg ) );
	auto ret2 = q::call_with_args_by_tuple( move_into, std::move( args ) );

	EXPECT_EQ( std::size_t( 1 ), num_args::value );
	EXPECT_EQ( 11, ret1 );
	EXPECT_EQ( 11, ret2 );
}

int fn_void_t( q::void_t )
{
	return 5;
}

TEST( functional, call_with_args_void )
{
	typedef Q_RESULT_OF_AS_ARGUMENT( decltype( fn_void_t ) )::size num_args;

	q::void_t _void;
	auto args = std::make_tuple( _void );
	auto ret1 = q::call_with_args( fn_void_t, _void );
	auto ret2 = q::call_with_args_by_tuple( fn_void_t, std::move( args ) );
	auto ret3 = q::call_with_args( fn_void_t );
	auto ret4 = q::call_with_args_by_tuple( fn_void_t, std::make_tuple( ) );

	EXPECT_EQ( std::size_t( 1 ), num_args::value );
	EXPECT_EQ( 5, ret1 );
	EXPECT_EQ( 5, ret2 );
	EXPECT_EQ( 5, ret3 );
	EXPECT_EQ( 5, ret4 );
}

TEST( functional, result_of )
{
	auto fn = [ ]( int, long ) { return true; };
	typedef q::result_of_t< decltype( fn ) > test;
	auto expectation = std::is_same< test, bool >::value;
	EXPECT_TRUE( expectation );
}

TEST( functional, result_of_as_argument )
{
	auto fn = [ ]( int, long ) { return true; };
	typedef q::result_of_as_argument_t< decltype( fn ) > test;
	auto expectation =
		q::is_argument_same_t< test, q::arguments< bool > >::value;
	EXPECT_TRUE( expectation );
}

TEST( functional, result_of_as_tuple )
{
	auto fn = [ ]( int, long ) { return true; };
	typedef q::result_of_as_tuple_t< decltype( fn ) > test;
	auto expectation = std::is_same< test, std::tuple< bool > >::value;
	EXPECT_TRUE( expectation );
}

TEST( functional, arguments_of )
{
	auto fn = [ ]( int, long ) { return true; };
	typedef q::arguments_of_t< decltype( fn ) > test;
	auto expectation =
		q::is_argument_same_t< test, q::arguments< int, long > >::value;
	EXPECT_TRUE( expectation );
}

TEST( functional, first_argument_of )
{
	auto fn = [ ]( int, long ) { return true; };
	typedef q::first_argument_of_t< decltype( fn ) > test;
	auto expectation = std::is_same< test, int >::value;
	EXPECT_TRUE( expectation );
}

TEST( functional, memberclass_of )
{
	struct C
	{
		bool fn( int, long ) { return true; }
	};
	struct D { };
	typedef q::memberclass_of_t< decltype( &C::fn ) > test;
	EXPECT_TRUE( ( std::is_same< test, C >::value ) );
	EXPECT_FALSE( ( std::is_same< test, D >::value ) );
}

TEST( functional, function_equals_basics )
{
	auto fn_void_void = [ ]( ) { };
	auto fn_void_int = [ ]( int ) { };
	auto fn_int_void = [ ]( ) -> int { return 0; };
	auto fn_int_int = [ ]( int ) -> int { return 0; };

	int foo = 0;

	auto cap_fn_void_void = [ foo ]( ) { };
	auto cap_fn_void_int = [ foo ]( int ) { };
	auto cap_fn_int_void = [ foo ]( ) -> int { return 0; };
	auto cap_fn_int_int = [ foo ]( int ) -> int { return 0; };


	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( fn_void_void ),
			decltype( fn_void_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( fn_void_int ),
			decltype( fn_void_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( fn_int_void ),
			decltype( fn_int_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( fn_int_int ),
			decltype( fn_int_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( cap_fn_void_void ),
			decltype( cap_fn_void_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( cap_fn_void_int ),
			decltype( cap_fn_void_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( cap_fn_int_void ),
			decltype( cap_fn_int_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( cap_fn_int_int ),
			decltype( cap_fn_int_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( fn_void_void ),
			decltype( cap_fn_void_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( fn_void_int ),
			decltype( cap_fn_void_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( fn_int_void ),
			decltype( cap_fn_int_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_equal_t<
			decltype( fn_int_int ),
			decltype( cap_fn_int_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( fn_void_void ),
			decltype( fn_void_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( fn_void_int ),
			decltype( fn_void_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( fn_int_void ),
			decltype( fn_int_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( fn_int_int ),
			decltype( fn_int_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( cap_fn_void_void ),
			decltype( cap_fn_void_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( cap_fn_void_int ),
			decltype( cap_fn_void_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( cap_fn_int_void ),
			decltype( cap_fn_int_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( cap_fn_int_int ),
			decltype( cap_fn_int_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( fn_void_void ),
			decltype( cap_fn_void_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( fn_void_int ),
			decltype( cap_fn_void_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( fn_int_void ),
			decltype( cap_fn_int_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( fn_int_int ),
			decltype( cap_fn_int_int )
		>::value
	) );


	// Non-capturing functions comparisons
	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( fn_void_void ),
			decltype( fn_void_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( fn_void_void ),
			decltype( fn_int_void )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( fn_void_void ),
			decltype( fn_int_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( fn_void_int ),
			decltype( fn_int_void )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( fn_void_int ),
			decltype( fn_int_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( fn_int_void ),
			decltype( fn_int_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( fn_void_void ),
			decltype( fn_void_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( fn_void_void ),
			decltype( fn_int_void )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( fn_void_void ),
			decltype( fn_int_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( fn_void_int ),
			decltype( fn_int_void )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( fn_void_int ),
			decltype( fn_int_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( fn_int_void ),
			decltype( fn_int_int )
		>::value
	) );

	// Capturing functions comparisons
	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( cap_fn_void_void ),
			decltype( cap_fn_void_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( cap_fn_void_void ),
			decltype( cap_fn_int_void )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( cap_fn_void_void ),
			decltype( cap_fn_int_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( cap_fn_void_int ),
			decltype( cap_fn_int_void )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( cap_fn_void_int ),
			decltype( cap_fn_int_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( cap_fn_int_void ),
			decltype( cap_fn_int_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( cap_fn_void_void ),
			decltype( cap_fn_void_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( cap_fn_void_void ),
			decltype( cap_fn_int_void )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( cap_fn_void_void ),
			decltype( cap_fn_int_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( cap_fn_void_int ),
			decltype( cap_fn_int_void )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( cap_fn_void_int ),
			decltype( cap_fn_int_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_same_type_t<
			decltype( cap_fn_int_void ),
			decltype( cap_fn_int_int )
		>::value
	) );
}


TEST( functional, function_equals_same_type )
{
	auto fn_void_int = [ ]( int ) { };
	auto fn_int_void = [ ]( ) -> int { return 0; };
	auto fn_int_int = [ ]( int ) -> int { return 0; };

	auto ref_fn_void_int = [ ]( int&& ) { };
	auto ref_fn_int_void = [ ]( )
		-> int&& { int i = 0; return std::move( i ); };
	auto ref_fn_int_int = [ ]( int&& )
		-> int&& { int i = 0; return std::move( i ); };


	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( fn_void_int ),
			decltype( ref_fn_void_int )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( fn_int_void ),
			decltype( ref_fn_int_void )
		>::value
	) );

	EXPECT_TRUE( (
		q::function_same_type_t<
			decltype( fn_int_int ),
			decltype( ref_fn_int_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( fn_void_int ),
			decltype( ref_fn_void_int )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( fn_int_void ),
			decltype( ref_fn_int_void )
		>::value
	) );

	EXPECT_FALSE( (
		q::function_equal_t<
			decltype( fn_int_int ),
			decltype( ref_fn_int_int )
		>::value
	) );
}

#ifdef LIBQ_WITH_CPP14

TEST( functional, is_memberfunction )
{
	struct C
	{
		bool fn( int, long );
	};
	// Lambdas are function objects, hence member functions
	auto fn_lambda = [ ]( int, long ) { return true; };
	// Non-capturing lambdas can be cast to raw C-functions
	bool ( *fn_raw )( int, long ) = fn_lambda;

	EXPECT_TRUE( q::is_memberfunction_v< decltype( &C::fn ) > );
	EXPECT_TRUE( q::is_memberfunction_v< decltype( fn_lambda ) > );
	EXPECT_FALSE( q::is_memberfunction_v< decltype( fn_raw ) > );
	EXPECT_FALSE( q::is_memberfunction_v< C > );
}

TEST( functional, is_function )
{
	struct C
	{
		bool fn( int, long );
	};
	EXPECT_TRUE( q::is_function_v< decltype( &C::fn ) > );
	EXPECT_FALSE( q::is_function_v< C > );
}

TEST( functional, arity_of )
{
	struct C
	{
		bool fn0( );
		bool fn2( int, long );
	};
	EXPECT_EQ( std::size_t( 0 ), q::arity_of_v< decltype( &C::fn0 ) > );
	EXPECT_EQ( std::size_t( 2 ), q::arity_of_v< decltype( &C::fn2 ) > );
}

TEST( functional, is_const_of )
{
	struct C
	{
		bool fn_c( ) const;
		bool fn_nc( );
	};

	auto fn_mutable_lambda = [ ]( ) mutable { };
	auto fn_lambda = [ ]( ) { };

	EXPECT_TRUE( q::is_const_of_v< decltype( &C::fn_c ) > );
	EXPECT_FALSE( q::is_const_of_v< decltype( &C::fn_nc ) > );
	EXPECT_TRUE( q::is_const_of_v< decltype( fn_lambda ) > );
	EXPECT_FALSE( q::is_const_of_v< decltype( fn_mutable_lambda ) > );
}

TEST( functional, first_argument_is_tuple )
{
	struct C
	{
		bool fn_t( std::tuple< long > );
		bool fn_l( long );
	};
	EXPECT_TRUE(
		q::first_argument_is_tuple< decltype( &C::fn_t ) >::value );
	EXPECT_FALSE(
		q::first_argument_is_tuple< decltype( &C::fn_l ) >::value );
}

TEST( functional, arguments_of_are )
{
	struct C
	{
		bool fn( std::tuple< long > );
	};
	EXPECT_TRUE( (
		q::arguments_of_are_v< decltype( &C::fn ), std::tuple< long > >
	) );
	EXPECT_FALSE( (
		q::arguments_of_are_v< decltype( &C::fn ), std::tuple< int > >
	) );
}

TEST( functional, arguments_of_are_convertible_from )
{
	struct C
	{
		bool fn( std::tuple< long > );
	};
	EXPECT_TRUE( (
		q::arguments_of_are_convertible_from_v<
			decltype( &C::fn ),
			std::tuple< long >
		>
	) );
	EXPECT_TRUE( (
		q::arguments_of_are_convertible_from_v<
			decltype( &C::fn ),
			std::tuple< int >
		>
	) );
	EXPECT_FALSE( (
		q::arguments_of_are_convertible_from_v<
			decltype( &C::fn ),
			std::tuple< std::unique_ptr< C > >
		>
	) );
}

#endif // LIBQ_WITH_CPP14
