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

#ifndef LIBQ_TEST_EXPECT_HPP
#define LIBQ_TEST_EXPECT_HPP

#include <q-test/q-test.hpp>

/**
 * Caveat; EVENTUALLY_EXPECT* does not allow two non-promises. You should use
 * gtest EXPECT_* alternatives instead. q-test does not encourage to re-invent
 * the wheel or to say that to non-promises are "eventually" available, when
 * they are instantly (synchronously) readable.
 *
 * This means that in the comparison between a and b, we must allow either a
 * *or* b to be promises, or both. The first (a) will be the initial
 * construction of the expect_value, which thereby must allow promises and
 * non-promises. It must then allow the second operand (b) to be both a promise
 * and a non-promise, but not a non-promise if (a) is a non-promise. This has a
 * great impact in how the templating is implemented.
 */

#define EVENTUALLY_EXPECT( expected, ... ) \
	::q::test::expected_root( *this, LIBQ_LOCATION, #expected, #__VA_ARGS__ ) \
	.that( expected )->

#define QTEST_EVENTUALLY_EXPECT( op, expected, ... ) \
	::q::test::expected_root( *this, LIBQ_LOCATION, #expected, #__VA_ARGS__ ) \
	.that( expected )->will. op ( __VA_ARGS__ )

#define EVENTUALLY_EXPECT_EQ( expected, ... ) \
	QTEST_EVENTUALLY_EXPECT( equal, expected, __VA_ARGS__ )

#define EVENTUALLY_EXPECT_NE( expected, ... ) \
	QTEST_EVENTUALLY_EXPECT( not_equal, expected, __VA_ARGS__ )

#define EVENTUALLY_EXPECT_GT( expected, ... ) \
	QTEST_EVENTUALLY_EXPECT( be_greater_than, expected, __VA_ARGS__ )

#define EVENTUALLY_EXPECT_LT( expected, ... ) \
	QTEST_EVENTUALLY_EXPECT( be_less_than, expected, __VA_ARGS__ )

#define EVENTUALLY_EXPECT_RESOLUTION( promise ) \
	::q::test::expected_root( *this, LIBQ_LOCATION, #promise ) \
	.that( promise )->to.be_resolved( )

#define EVENTUALLY_EXPECT_REJECTION( promise ) \
	::q::test::expected_root( *this, LIBQ_LOCATION, #promise ) \
	.that( promise )->to.be_rejected( )

#define EVENTUALLY_EXPECT_REJECTION_WITH( promise, Class ) \
	::q::test::expected_root( *this, LIBQ_LOCATION, #promise, #Class ) \
	.that( promise )->to.be_rejected_with< Class >( )

#define EVENTUALLY_EXPECT_SHARED( promise ) \
	EVENTUALLY_EXPECT( promise ) to.be_shared( )

#define EVENTUALLY_EXPECT_UNIQUE( promise ) \
	EVENTUALLY_EXPECT( promise ) to.be_unique( )


namespace {

static const std::string new_line = "\n";

struct can_stream_value_impl
{
	template< typename T >
	static std::true_type
	test_value( typename std::decay< decltype(
		std::declval< std::ostream& >( ) << std::declval< T >( )
	) >::type* );

	template< typename T >
	static std::false_type
	test_value( ... );

	template< typename T >
	struct test
	: decltype( test_value< T >( nullptr ) )
	{ };
};

template< typename T >
struct can_stream_value
: can_stream_value_impl::template test< T >
{ };

template< typename T >
static typename std::enable_if<
	can_stream_value< T >::value,
	std::string
>::type try_stream_value( T&& t )
{
	std::stringstream ss;
	ss << t;
	return ss.str( );
}

template< typename T >
static typename std::enable_if<
	!can_stream_value< T >::value,
	std::string
>::type try_stream_value( T&& t )
{
	return std::string( );
}

} // anonymous namespace

namespace q { namespace test {

template< typename T >
class expected_value;

class expected_root
{
public:
	expected_root(
		::q::test::fixture& fixture,
		std::string file,
		long line,
		std::string function,
		const char* expected = nullptr,
		const char* actual = nullptr )
	: fixture_( fixture )
	, file_( file )
	, line_( line )
	, function_( function )
	, expected_( expected )
	, actual_( actual )
	{ }

	expected_root( expected_root&& ) = default;
	expected_root( const expected_root& ) = delete;

	template< typename T >
	std::shared_ptr< expected_value< typename std::decay< T >::type > >
	operator( )( T&& t )
	{
		return that( std::forward< T >( t ) );
	}

	template< typename T >
	std::shared_ptr< expected_value< typename std::decay< T >::type > >
	that( T&& t )
	{
		return expected_value< typename std::decay< T >::type >
			::construct(
				std::move( *this ), std::forward< T >( t ) );
	}

	inline ::q::test::fixture& fixture( )
	{
		return fixture_;
	}

	inline const char* file( )
	{
		return file_.c_str( );
	}

	inline const long& line( )
	{
		return line_;
	}

	inline const char* expected_expr( )
	{
		return expected_;
	}
	inline const char* val1_expr( )
	{
		return expected_;
	}

	inline const char* actual_expr( )
	{
		return actual_;
	}
	inline const char* val2_expr( )
	{
		return actual_;
	}

	inline std::string expected_explanation( )
	{
		std::string expected;
		if ( expected_ )
			expected = new_line + "Expected: " + expected_;
		return expected;
	}

private:
	::q::test::fixture& fixture_;
	std::string file_;
	long line_;
	std::string function_;
	const char* expected_;
	const char* actual_;
};

template< typename T >
class expectation
{
public:
	;
};

template< typename Value >
class expected_any_comparator_base
{
protected:
	template< typename T > friend class expected_value;

	void set_value( const std::shared_ptr< Value >& value )
	{
		value_ = value;
	}

	std::shared_ptr< Value > get_value( ) const
	{
		return value_;
	}

	::q::test::fixture& get_fixture( )
	{
		return get_value( )->root( ).fixture( );
	}

	void await_promise( ::q::promise< std::tuple< > >&& promise )
	{
		get_fixture( ).await_promise( std::move( promise ) );
	}

	void await_promise( ::q::shared_promise< std::tuple< > >&& promise )
	{
		get_fixture( ).await_promise( promise.unshare( ) );
	}

	std::shared_ptr< Value > value_;

};

template< typename Value >
class expected_empty_comparator_base
: public expected_any_comparator_base< Value >
{
};

template< typename T, typename Value, typename Parent >
class expected_comparator_base
: public expected_any_comparator_base< Value >
{
public:
	typedef expected_any_comparator_base< Value > value_base;
	typedef expected_comparator_base< T, Value, Parent > this_type;
	typedef std::function< void( const T& ) > testee_type;
	typedef ::q::promise< std::tuple< T > > promise_type;
	typedef ::q::shared_promise< std::tuple< T > > shared_promise_type;

	template< typename Promise >
	struct is_any_promise
	: ::q::is_promise< typename std::decay< Promise >::type >
	{ };

#define QTEST_IMPLEMENT_COMPARISON_TEST( op ) \
	template< typename Promise > \
	typename std::enable_if< is_any_promise< Promise >::value >::type \
	op( Promise&& eventual_val2 ) \
	{ \
		typedef typename std::decay< Promise >::type  promise_type; \
		typedef typename promise_type::argument_types argument_types; \
		typedef typename argument_types::first_type   arg_type; \
\
		auto value = value_base::get_value( ); \
\
		auto expectation = eventual_val2 \
		.then( [ value ]( const arg_type& val2 ) \
		{ \
			this_type::_##op< arg_type >( value, val2 ); \
		} ); \
\
		value_base::await_promise( std::move( expectation ) ); \
	} \
\
	template< typename U > \
	typename std::enable_if< !is_any_promise< U >::value >::type \
	op( U&& val2 ) \
	{ \
		this_type::_##op< U >( value_base::get_value( ), val2 ); \
	}

	QTEST_IMPLEMENT_COMPARISON_TEST( equal )
	QTEST_IMPLEMENT_COMPARISON_TEST( not_equal )
	QTEST_IMPLEMENT_COMPARISON_TEST( be_greater_than )
	QTEST_IMPLEMENT_COMPARISON_TEST( be_less_than )

private:
	template< typename U >
	static void _equal(
		std::shared_ptr< Value > value,
		typename std::remove_reference< U >::type val2
	)
	{
		return Parent::test( value, [ value, val2 ]( T val1 )
		{
			if ( val1 == val2 )
				return;

			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Value of: "
				<< root.actual_expr( ) << std::endl
				<< "  actual: " << val2 << std::endl
				<< "Expected: "
				<< root.expected_expr( ) << std::endl
				<< "Which is: " << val1;
		} );
	}

	template< typename U >
	static void _not_equal(
		std::shared_ptr< Value > value,
		typename std::remove_reference< U >::type val2
	)
	{
		return Parent::test( value, [ value, val2 ]( T val1 )
		{
			if ( val1 != val2 )
				return;

			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected: "
				<< "(" << root.val1_expr( ) << ")"
				<< " != "
				<< "(" << root.val2_expr( ) << ")"
				<< ", actual: " << val1 << " vs " << val2;
		} );
	}

	template< typename U >
	static void _be_greater_than(
		std::shared_ptr< Value > value,
		typename std::remove_reference< U >::type val2
	)
	{
		return Parent::test( value, [ value, val2 ]( T val1 )
		{
			if ( val1 > val2 )
				return;

			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected: "
				<< "(" << root.val1_expr( ) << ")"
				<< " > "
				<< "(" << root.val2_expr( ) << ")"
				<< ", actual: " << val1 << " vs " << val2;
		} );
	}

	template< typename U >
	static void _be_less_than(
		std::shared_ptr< Value > value,
		typename std::remove_reference< U >::type val2
	)
	{
		return Parent::test( value, [ value, val2 ]( T val1 )
		{
			if ( val1 < val2 )
				return;

			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected: "
				<< "(" << root.val1_expr( ) << ")"
				<< " < "
				<< "(" << root.val2_expr( ) << ")"
				<< ", actual: " << val1 << " vs " << val2;
		} );
	}
};

template< typename T, bool IsPromise = ::q::is_promise< T >::value >
class expected_comparator;

template< typename T >
class expected_value
: public std::enable_shared_from_this< expected_value< T > >
{
public:
	expected_value( expected_root&& root, const T& t )
	: root_( std::move( root ) )
	, t_( q::make_unique< T >( t ) )
	{ }

	expected_value( expected_root&& root, T&& t )
	: root_( std::move( root ) )
	, t_( q::make_unique< T >( std::move( t ) ) )
	{ }

	static std::shared_ptr< expected_value< T > >
	construct( expected_root&& root, const T& t )
	{
		auto self = std::make_shared< expected_value< T > >(
			std::move( root ), t );
		self->to.set_value( self );
		self->will.set_value( self );
		return self;
	}

	static std::shared_ptr< expected_value< T > >
	construct( expected_root&& root, T&& t )
	{
		auto self = std::make_shared< expected_value< T > >(
			std::move( root ), std::move( t ) );
		self->to.set_value( self );
		self->will.set_value( self );
		return self;
	}

	expected_comparator< T > to;
	expected_comparator< T > will;

	expected_root& root( )
	{
		return root_;
	}

	T& get( ) const
	{
		return *t_;
	}

private:
	expected_root root_;
	std::unique_ptr< T > t_;
};

/**
 * Non-promise
 */
template< typename T >
class expected_comparator< T, false >
: public expected_comparator_base<
	T, expected_value< T >, expected_comparator< T, false >
>
{
public:
	typedef expected_comparator_base<
		T, expected_value< T >, expected_comparator< T, false >
	> base;

	static void test(
		std::shared_ptr< expected_value< T > > value,
		typename base::testee_type testee
	)
	{
		testee( value->get( ) );
	}
};

/**
 * Generic empty promise (shared or unique)
 */
template< typename Promise >
struct expected_empty_promise_comparator
: public expected_empty_comparator_base< expected_value< Promise > >
{
public:
	typedef expected_empty_comparator_base<
		expected_value< Promise >
	> base;

	void be_resolved( )
	{
		auto value = base::value_;

		auto expectation = value->get( ).strip( )
		.fail( [ value ]( std::exception_ptr e )
		{
			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected promise to be resolved."
				<< root.expected_explanation( )
				<< std::endl
				<< "But was rejected with: "
				<< q::to_string( e );
		} );

		base::await_promise( std::move( expectation ) );
	}

	void be_rejected( )
	{
		auto value = base::value_;

		auto expectation = value->get( )
		.then( [ value ]( )
		{
			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected promise to be rejected."
				<< root.expected_explanation( )
				<< std::endl
				<< "But was resolved.";
		} )
		.fail( [ ]( std::exception_ptr ) { } );

		base::await_promise( std::move( expectation ) );
	}

	template< typename Error >
	void be_rejected_with( )
	{
		auto value = base::value_;

		auto expectation = value->get( )
		.then( [ value ]( )
		{
			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected promise to be rejected."
				<< root.expected_explanation( )
				<< std::endl
				<< "But was resolved.";
		} )
		.fail( [ ]( const Error& ) { } )
		.fail( [ value ]( std::exception_ptr e )
		{
			auto& root = value->root( );

			std::string error_name;
			if ( root.actual_expr( ) )
				error_name = root.actual_expr( );
			else
				error_name = typeid( Error ).name( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected promise to be rejected with \""
				<< error_name << "\"."
				<< root.expected_explanation( )
				<< std::endl
				<< "But was rejected with: "
				<< q::to_string( e );
		} );

		base::await_promise( std::move( expectation ) );
	}
};

/**
 * Generic promise (shared or unique)
 */
template< typename T, typename Promise >
class expected_promise_comparator
: public expected_comparator_base<
	T,
	expected_value< Promise >,
	expected_promise_comparator< T, Promise >
>
{
public:
	typedef expected_comparator_base<
		T,
		expected_value< Promise >,
		expected_promise_comparator< T, Promise >
	> base;

	expected_promise_comparator( ) { }

	void be_resolved( )
	{
		auto value = base::value_;

		auto expectation = value->get( ).strip( )
		.fail( [ value ]( std::exception_ptr e )
		{
			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected promise to be resolved."
				<< root.expected_explanation( )
				<< std::endl
				<< "But was rejected with: "
				<< q::to_string( e );
		} );

		base::await_promise( std::move( expectation ) );
	}

	void be_rejected( )
	{
		auto value = base::value_;

		auto expectation = value->get( )
		.then( [ value ]( const T& t )
		{
			auto& root = value->root( );

			std::string val;
			if ( can_stream_value< T >::value )
				val = "But was resolved to: " +
					try_stream_value( t );
			else
				val = "But was resolved.";

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected promise to be rejected."
				<< root.expected_explanation( )
				<< std::endl
				<< val;
		} )
		.fail( [ ]( std::exception_ptr ) { } );

		base::await_promise( std::move( expectation ) );
	}

	template< typename Error >
	void be_rejected_with( )
	{
		auto value = base::value_;

		auto expectation = value->get( )
		.then( [ value ]( const T& t )
		{
			auto& root = value->root( );

			std::string val;
			if ( can_stream_value< T >::value )
				val = "But was resolved to: " +
					try_stream_value( t );
			else
				val = "But was resolved.";

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected promise to be rejected."
				<< root.expected_explanation( )
				<< std::endl
				<< val;
		} )
		.fail( [ ]( const Error& ) { } )
		.fail( [ value ]( std::exception_ptr e )
		{
			auto& root = value->root( );

			std::string error_name;
			if ( root.actual_expr( ) )
				error_name = root.actual_expr( );
			else
				error_name = typeid( Error ).name( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected promise to be rejected with \""
				<< error_name << "\"."
				<< root.expected_explanation( )
				<< std::endl
				<< "But was rejected with: "
				<< q::to_string( e );
		} );

		base::await_promise( std::move( expectation ) );
	}

	static void test(
		std::shared_ptr< expected_value< Promise > > value,
		typename base::testee_type testee
	)
	{
		if ( Promise::shared_type::value )
			test_shared( std::move( value ), std::move( testee ) );
		else
			test_unique( std::move( value ), std::move( testee ) );
	}

private:
	static void test_shared(
		std::shared_ptr< expected_value< Promise > >&& value,
		typename base::testee_type&& testee
	)
	{
		value->root( ).fixture( ).await_promise(
			value->get( )
			.then( [ testee ]( const T& t )
			{
				testee( t );
			} )
		);
	}

	static void test_unique(
		std::shared_ptr< expected_value< Promise > >&& value,
		typename base::testee_type&& testee
	)
	{
		value->root( ).fixture( ).await_promise(
			value->get( )
			.then( [ testee ]( T&& t )
			{
				testee( t );
			} )
		);
	}
};

/**
 * Empty unique promise
 */
template< >
class expected_comparator< q::promise< std::tuple< > >, true >
: public expected_empty_promise_comparator< q::promise< std::tuple< > > >
{
public:
	typedef q::promise< std::tuple< > > promise_type;
	typedef expected_empty_promise_comparator<
		promise_type
	> comparator;
	typedef comparator::base base;

	expected_comparator( ) { }

	void be_unique( ) { }

	void be_shared( )
	{
		auto& root = base::value_->root( );
		ADD_FAILURE_AT( root.file( ), root.line( ) )
			<< "Expected promise to be shared, but is unique."
			<< root.expected_explanation( )
			<< std::endl;
	}
};

/**
 * Non-empty unique promise
 */
template< typename T >
class expected_comparator< q::promise< std::tuple< T > >, true >
: public expected_promise_comparator< T, q::promise< std::tuple< T > > >
{
public:
	typedef q::promise< std::tuple< T > > promise_type;
	typedef expected_comparator_base<
		T,
		expected_value< q::promise< std::tuple< T > > >,
		expected_promise_comparator< T, q::promise< std::tuple< T > > >
	> base;

	expected_comparator( ) { }

	void be_unique( ) { }

	void be_shared( )
	{
		auto& root = base::value_->root( );
		ADD_FAILURE_AT( root.file( ), root.line( ) )
			<< "Expected promise to be shared, but is unique."
			<< root.expected_explanation( )
			<< std::endl;
	}
};

/**
 * Empty shared promise
 */
template< >
class expected_comparator< q::shared_promise< std::tuple< > >, true >
: public expected_empty_promise_comparator< q::shared_promise< std::tuple< > > >
{
public:
	typedef q::shared_promise< std::tuple< > > promise_type;
	typedef expected_empty_promise_comparator<
		promise_type
	> comparator;
	typedef comparator::base base;

	expected_comparator( ) { }

	void be_unique( ) { }

	void be_shared( )
	{
		auto& root = base::value_->root( );
		ADD_FAILURE_AT( root.file( ), root.line( ) )
			<< "Expected promise to be shared, but is unique."
			<< root.expected_explanation( )
			<< std::endl;
	}
};

/**
 * Non-empty shared promise
 */
template< typename T >
class expected_comparator< q::shared_promise< std::tuple< T > >, true >
: public expected_promise_comparator< T, q::shared_promise< std::tuple< T > > >
{
public:
	typedef q::shared_promise< std::tuple< T > > promise_type;
	typedef expected_comparator_base<
		T,
		expected_value< q::shared_promise< std::tuple< T > > >,
		expected_promise_comparator<
			T, q::shared_promise< std::tuple< T > >
		>
	> base;

	expected_comparator( ) { }

	void be_unique( )
	{
		auto& root = base::value_->root( );
		ADD_FAILURE_AT( root.file( ), root.line( ) )
			<< "Expected promise to be unique, but is shared."
			<< root.expected_explanation( )
			<< std::endl;
	}

	void be_shared( ) { }
};

} } // namespace test, namespace q

#endif // LIBQ_TEST_EXPECT_HPP
