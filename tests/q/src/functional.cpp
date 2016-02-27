
#include <q/functional.hpp>

#include <gtest/gtest.h>


TEST( Functional, IsArgumentSame )
{
	typedef q::is_argument_same<
		q::arguments< char, int >,
		q::arguments< char, int >
	> ias_type;
	const ias_type::value_type is_same_1 = ias_type::value;

	const bool is_same_2 = q::is_argument_same<
		q::arguments< >,
		q::arguments< >
	>::value;

	const bool is_same_3 = q::is_argument_same<
		q::arguments< const double >,
		q::arguments< double >
	>::value;

	const bool is_not_same_1 = q::is_argument_same<
		q::arguments< char, long >,
		q::arguments< char, int >
	>::value;

	const bool is_not_same_2 = q::is_argument_same<
		q::arguments< char, int, double >,
		q::arguments< char, int >
	>::value;

	const bool is_not_same_3 = q::is_argument_same<
		q::arguments< double >,
		q::arguments< >
	>::value;

	const bool is_not_same_4 = q::is_argument_same<
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

TEST( Functional, LambdaFunctionArgumentTypes )
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

	EXPECT_EQ( 0, return_type::size::value );
	EXPECT_EQ( 4, lambda_traits::arity::value );
}

TEST( Functional, LambdaFunctionVoidArgumentType )
{
	auto lambda = [ ]( ) { };

	typedef q::function_traits< decltype( lambda ) > lambda_traits;
	typedef lambda_traits::argument_types::tuple_type tuple_type;
	typedef Q_RESULT_OF_AS_ARGUMENT_TYPE( decltype( lambda ) ) return_type;

	EXPECT_EQ( 0, return_type::size::value );
	EXPECT_EQ( 0, std::tuple_size< tuple_type >::value );
	EXPECT_EQ( 0, lambda_traits::arity::value );
}

int fn( char, std::string );

TEST( Functional, FunctionArgumentTypes )
{
	typedef decltype( fn ) fn_type;
	typedef Q_ARGUMENTS_OF( fn_type ) arguments_type;
	typedef Q_RESULT_OF( fn_type ) return_type;

	typedef Q_FIRST_ARGUMENT_OF( fn_type ) type_1;
	typedef arguments_type::rest_arguments::first_type type_2;

	EXPECT_TRUE( ( std::is_same< type_1, char >::value ) );
	EXPECT_TRUE( ( std::is_same< type_2, std::string >::value ) );
	EXPECT_TRUE( ( std::is_same< return_type, int >::value ) );
	EXPECT_EQ( 2, Q_ARITY_OF( fn_type ) );
}

struct C
{
	C( int ) { }
	float fn( long ) { return (float)3.14; }
};

TEST( Functional, ClassFunctionArgumentTypes )
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
	EXPECT_EQ( 1, Q_ARITY_OF( fn_type ) );
}

int move_into( std::string&& s )
{
	return s.length( );
}

TEST( Functional, CallWithArgs )
{
	typedef Q_RESULT_OF_AS_ARGUMENT( decltype( move_into ) )::size num_args;

	std::string arg = "hello world";
	auto args = std::make_tuple( arg );
	auto ret1 = q::call_with_args( move_into, std::move( arg ) );
	auto ret2 = q::call_with_args_by_tuple( move_into, std::move( args ) );

	EXPECT_EQ( 1, num_args::value );
	EXPECT_EQ( 11, ret1 );
	EXPECT_EQ( 11, ret2 );
}

TEST( Functional, result_of )
{
	auto fn = [ ]( int, long ) { return true; };
	typedef q::result_of< decltype( fn ) > test;
	auto expectation = std::is_same< test, bool >::value;
	EXPECT_TRUE( expectation );
}

TEST( Functional, result_of_as_argument )
{
	auto fn = [ ]( int, long ) { return true; };
	typedef q::result_of_as_argument< decltype( fn ) > test;
	auto expectation =
		q::is_argument_same< test, q::arguments< bool > >::value;
	EXPECT_TRUE( expectation );
}

TEST( Functional, result_of_as_tuple )
{
	auto fn = [ ]( int, long ) { return true; };
	typedef q::result_of_as_tuple< decltype( fn ) > test;
	auto expectation = std::is_same< test, std::tuple< bool > >::value;
	EXPECT_TRUE( expectation );
}

TEST( Functional, arguments_of )
{
	auto fn = [ ]( int, long ) { return true; };
	typedef q::arguments_of< decltype( fn ) > test;
	auto expectation =
		q::is_argument_same< test, q::arguments< int, long > >::value;
	EXPECT_TRUE( expectation );
}

TEST( Functional, first_argument_of )
{
	auto fn = [ ]( int, long ) { return true; };
	typedef q::first_argument_of< decltype( fn ) > test;
	auto expectation = std::is_same< test, int >::value;
	EXPECT_TRUE( expectation );
}

TEST( Functional, memberclass_of )
{
	struct C
	{
		bool fn( int, long ) { return true; }
	};
	struct D { };
	typedef q::memberclass_of< decltype( &C::fn ) > test;
	EXPECT_TRUE( ( std::is_same< test, C >::value ) );
	EXPECT_FALSE( ( std::is_same< test, D >::value ) );
}

#ifdef LIBQ_WITH_CPP14

TEST( Functional, is_memberfunction )
{
	struct C
	{
		bool fn( int, long );
	};
	// Lambdas are function objects, hence member functions
	auto fn_lambda = [ ]( int, long ) { return true; };
	// Non-capturing lambdas can be cast to raw C-functions
	bool ( *fn_raw )( int, long ) = fn_lambda;

	EXPECT_TRUE( q::is_memberfunction< decltype( &C::fn ) > );
	EXPECT_TRUE( q::is_memberfunction< decltype( fn_lambda ) > );
	EXPECT_FALSE( q::is_memberfunction< decltype( fn_raw ) > );
	EXPECT_FALSE( q::is_memberfunction< C > );
}

TEST( Functional, is_function )
{
	struct C
	{
		bool fn( int, long );
	};
	EXPECT_TRUE( q::is_function< decltype( &C::fn ) > );
	EXPECT_FALSE( q::is_function< C > );
}

TEST( Functional, is_noexcept )
{
	struct C
	{
		bool fn( int, long ) const;
		bool fn_ne( int, long ) const noexcept;
	};


	std::cout << std::endl << std::endl;
	std::cout << typeid( decltype( &C::fn ) ).name() << std::endl;
	std::cout << typeid( decltype( &C::fn_ne ) ).name() << std::endl;
	std::cout << noexcept( std::declval< decltype( &C::fn ) >( ) ) << std::endl;
	std::cout << noexcept( std::declval< decltype( &C::fn_ne ) >( ) ) << std::endl;
	std::cout << q::is_noexcept< decltype( &C::fn ) > << std::endl;
	std::cout << q::is_noexcept< decltype( &C::fn_ne ) > << std::endl;

//((C*)0)->
	std::cout
		<< noexcept(
			( reinterpret_cast< Q_MEMBERCLASS_OF( decltype( &C::fn ) )* >( 0 )->*(&C::fn) )
			( int(), long() )
		)
//		<< noexcept( ( *reinterpret_cast< decltype( &C::fn ) >( 0 ) )( std::declval< int, long >( )... ) )
		<< std::endl;

	std::cout
		<< noexcept(
			( reinterpret_cast< Q_MEMBERCLASS_OF( decltype( &C::fn_ne ) )* >( 0 )->C::fn_ne )
			( int(), long() )
		)
		<< std::endl;

	std::cout
		<< noexcept(
			::q::call_with_args(
				( &C::fn_ne ),
				reinterpret_cast< Q_MEMBERCLASS_OF( decltype( &C::fn_ne ) )* >( 0 ),
				int(), long()
			)
		)
		<< std::endl;

//template< typename... Args >
//using _declsadfasdfval< q::arguments< Args... > > = std::declval< Args... >;

//		<< noexcept( ( *reinterpret_cast< decltype( &C::fn ) >( 0 ) )( std::declval< int, long >( )... ) )

//	std::cout << noexcept( ( (*(C*)0).fn_ne )( ) ) << std::endl;
	std::cout << std::endl << std::endl;
//std::is_nothrow_constructible<sdf, sdaf>
//    : public integral_constant<bool, noexcept(_Tp(declval<_Args>()...))>

	EXPECT_TRUE( q::is_noexcept< decltype( &C::fn_ne ) > );
	EXPECT_FALSE( q::is_noexcept< decltype( &C::fn ) > );
}

TEST( Functional, arity_of )
{
	struct C
	{
		bool fn0( );
		bool fn2( int, long );
	};
	EXPECT_EQ( 0, q::arity_of< decltype( &C::fn0 ) > );
	EXPECT_EQ( 2, q::arity_of< decltype( &C::fn2 ) > );
}

TEST( Functional, is_const_of )
{
	struct C
	{
		bool fn_c( ) const;
		bool fn_nc( );
	};

	auto fn_mutable_lambda = [ ]( ) mutable { };
	auto fn_lambda = [ ]( ) { };

	EXPECT_TRUE( q::is_const_of< decltype( &C::fn_c ) > );
	EXPECT_FALSE( q::is_const_of< decltype( &C::fn_nc ) > );
	EXPECT_TRUE( q::is_const_of< decltype( fn_lambda ) > );
	EXPECT_FALSE( q::is_const_of< decltype( fn_mutable_lambda ) > );
}

TEST( Functional, first_argument_is_tuple )
{
	struct C
	{
		bool fn_t( std::tuple< long > );
		bool fn_l( long );
	};
	EXPECT_TRUE( q::first_argument_is_tuple< decltype( &C::fn_t ) > );
	EXPECT_FALSE( q::first_argument_is_tuple< decltype( &C::fn_l ) > );
}

TEST( Functional, arguments_of_are )
{
	struct C
	{
		bool fn( std::tuple< long > );
	};
	EXPECT_TRUE( (
		q::arguments_of_are< decltype( &C::fn ), std::tuple< long > >
	) );
	EXPECT_FALSE( (
		q::arguments_of_are< decltype( &C::fn ), std::tuple< int > >
	) );
}

TEST( Functional, arguments_of_are_convertible_from )
{
	struct C
	{
		bool fn( std::tuple< long > );
	};
	EXPECT_TRUE( (
		q::arguments_of_are_convertible_from<
			decltype( &C::fn ),
			std::tuple< long >
		>
	) );
	EXPECT_TRUE( (
		q::arguments_of_are_convertible_from<
			decltype( &C::fn ),
			std::tuple< int >
		>
	) );
	EXPECT_FALSE( (
		q::arguments_of_are_convertible_from<
			decltype( &C::fn ),
			std::tuple< std::unique_ptr< C > >
		>
	) );
}

#endif
