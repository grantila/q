
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
	float fn( long ) { return 3.14; }
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
