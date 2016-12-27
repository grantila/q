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

#ifndef LIBQ_RX_TEST_EXPECT_HPP
#define LIBQ_RX_TEST_EXPECT_HPP

#include <q-test/q-test.hpp>
#include <q-test/stringify.hpp>

/**
 * The EXPECT_OBSERVABLES_* macros allow easy testing of observables. The will
 * await the observables to be complete, and also compare individual elements
 * consumed from them.
 */

#define EXPECT_OBSERVABLE( expected, ... ) \
	::q::rx::test::expected_root( \
		*this, LIBQ_LOCATION, #expected, #__VA_ARGS__ \
	) \
	.that( expected )->

#define QRX_TEST_EXPECT_OBSERVABLE( expected, actual, ... ) \
	::q::rx::test::expected_root( \
		*this, LIBQ_LOCATION, #expected, #actual \
	) \
	.that( expected )->will. op ( __VA_ARGS__ )

#define EXPECT_OBSERVABLE_EQ( expected, ... ) \
	QRX_TEST_EXPECT_OBSERVABLE( equal, expected, __VA_ARGS__ )

#define EXPECT_OBSERVABLE_NE( expected, ... ) \
	QRX_TEST_EXPECT_OBSERVABLE( not_equal, expected, __VA_ARGS__ )

#define EXPECT_OBSERVABLE_COMPLETE( observable ) \
	::q::test::expected_root( *this, LIBQ_LOCATION, #observable ) \
	.that( observable )->to.be_resolved( )

#define EXPECT_OBSERVABLE_REJECTION( observable ) \
	::q::test::expected_root( *this, LIBQ_LOCATION, #observable ) \
	.that( observable )->to.be_rejected( )

#define EXPECT_OBSERVABLE_REJECTION_WITH( observable, Class ) \
	::q::test::expected_root( *this, LIBQ_LOCATION, #observable, #Class ) \
	.that( observable )->to.be_rejected_with< Class >( )


namespace q { namespace rx { namespace test {

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
			expected = q::test::new_line + "Expected: " + expected_;
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

	void await_promise( ::q::promise< >&& promise )
	{
		get_fixture( ).await_promise( std::move( promise ) );
	}

	void await_promise( ::q::shared_promise< >&& promise )
	{
		get_fixture( ).await_promise( promise.unshare( ) );
	}

	std::shared_ptr< Value > value_;

};

template< typename Value, typename Parent, typename... T >
class expected_comparator_base
: public expected_any_comparator_base< Value >
{
public:
	typedef expected_any_comparator_base< Value > value_base;
	typedef expected_comparator_base< Value, Parent, T... > this_type;
	typedef std::tuple< T... > tuple_type;
	typedef std::function< void( const tuple_type& ) > testee_type;
	typedef ::q::promise< T... > promise_type;
	typedef ::q::shared_promise< T... > shared_promise_type;

	template< typename Promise >
	struct is_any_promise
	: ::q::is_promise< typename std::decay< Promise >::type >
	{ };

#define QTEST_IMPLEMENT_COMPARISON_TEST( op ) \
	template< typename Promise > \
	typename std::enable_if< is_any_promise< Promise >::value >::type \
	op( Promise&& eventual_val2 ) \
	{ \
		typedef typename std::decay< Promise >::type promise_type; \
		typedef typename std::decay< \
			typename promise_type::tuple_type \
		>::type tuple_type; \
\
		auto value = value_base::get_value( ); \
\
		auto expectation = eventual_val2 \
		.then( [ value ]( const tuple_type& val2 ) \
		{ \
			this_type::_##op( value, val2 ); \
		} ); \
\
		value_base::await_promise( std::move( expectation ) ); \
	} \
\
	template< typename U > \
	typename std::enable_if< \
		!is_any_promise< U >::value \
		and \
		!q::is_tuple< typename std::decay< U >::type >::value \
	>::type \
	op( U&& val2 ) \
	{ \
		this_type::_##op( \
			value_base::get_value( ), \
			std::make_tuple( std::forward< U >( val2 ) ) ); \
	} \
\
	template< typename U > \
	typename std::enable_if< \
		!is_any_promise< U >::value \
		and \
		q::is_tuple< typename std::decay< U >::type >::value \
	>::type \
	op( U&& val2 ) \
	{ \
		this_type::_##op( \
			value_base::get_value( ), std::forward< U >( val2 ) ); \
	}

	QTEST_IMPLEMENT_COMPARISON_TEST( equal )
	QTEST_IMPLEMENT_COMPARISON_TEST( not_equal )
	QTEST_IMPLEMENT_COMPARISON_TEST( be_greater_than )
	QTEST_IMPLEMENT_COMPARISON_TEST( be_less_than )

private:
	template< typename U, std::size_t Args = sizeof...( T ) >
	static typename std::enable_if<
		Args == 1
		and
		!q::is_tuple< typename std::decay< U >::type >::value
	>::type
	_equal(
		std::shared_ptr< Value > value,
		typename std::remove_reference< U >::type val2
	)
	{
		_equal( value, std::make_tuple( val2 ) );
	}

	template< typename U >
	static typename std::enable_if<
		q::is_tuple< typename std::decay< U >::type >::value
	>::type
	_equal(
		std::shared_ptr< Value > value,
		U&& _val2
	)
	{
		typename std::decay< U >::type val2 =
			std::forward< U >( _val2 );

		return Parent::test( value, [ value, val2 ](
			const tuple_type& val1
		)
		{
			if ( val1 == val2 )
				return;

			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Value of: "
				<< root.actual_expr( ) << std::endl
				<< "  actual: "
				<< q::test::stringify_value( val2 )
				<< std::endl
				<< "Expected: "
				<< root.expected_expr( ) << std::endl
				<< "Which is: "
				<< q::test::stringify_value( val1 );
		} );
	}

	template< typename U, std::size_t Args = sizeof...( T ) >
	static typename std::enable_if<
		Args == 1
		and
		!q::is_tuple< typename std::decay< U >::type >::value
	>::type
	_not_equal(
		std::shared_ptr< Value > value,
		typename std::remove_reference< U >::type val2
	)
	{
		_not_equal( value, std::make_tuple( val2 ) );
	}

	template< typename U >
	static typename std::enable_if<
		q::is_tuple< typename std::decay< U >::type >::value
	>::type
	_not_equal(
		std::shared_ptr< Value > value,
		U&& _val2
	)
	{
		typename std::decay< U >::type val2 =
			std::forward< U >( _val2 );

		return Parent::test( value, [ value, val2 ](
			const tuple_type& val1
		)
		{
			if ( val1 != val2 )
				return;

			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected: "
				<< "(" << root.val1_expr( ) << ")"
				<< " != "
				<< "(" << root.val2_expr( ) << ")"
				<< ", actual: "
				<< q::test::stringify_value( val1 ) << " vs "
				<< q::test::stringify_value( val2 );
		} );
	}

	template< typename U, std::size_t Args = sizeof...( T ) >
	static typename std::enable_if<
		Args == 1
		and
		!q::is_tuple< typename std::decay< U >::type >::value
	>::type
	_be_greater_than(
		std::shared_ptr< Value > value,
		typename std::remove_reference< U >::type val2
	)
	{
		_be_greater_than( value, std::make_tuple( val2 ) );
	}

	template< typename U, std::size_t Args = sizeof...( T ) >
	static typename std::enable_if<
		q::is_tuple< typename std::decay< U >::type >::value
	>::type
	_be_greater_than(
		std::shared_ptr< Value > value,
		U&& _val2
	)
	{
		typename std::decay< U >::type val2 =
			std::forward< U >( _val2 );

		return Parent::test( value, [ value, val2 ](
			const tuple_type& val1
		)
		{
			if ( val1 > val2 )
				return;

			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected: "
				<< "(" << root.val1_expr( ) << ")"
				<< " > "
				<< "(" << root.val2_expr( ) << ")"
				<< ", actual: "
				<< q::test::stringify_value( val1 ) << " vs "
				<< q::test::stringify_value( val2 );
		} );
	}

	template< typename U, std::size_t Args = sizeof...( T ) >
	static typename std::enable_if<
		Args == 1
		and
		!q::is_tuple< typename std::decay< U >::type >::value
	>::type
	_be_less_than(
		std::shared_ptr< Value > value,
		typename std::remove_reference< U >::type val2
	)
	{
		_be_less_than( value, std::make_tuple( val2 ) );
	}

	template< typename U, std::size_t Args = sizeof...( T ) >
	static typename std::enable_if<
		q::is_tuple< typename std::decay< U >::type >::value
	>::type
	_be_less_than(
		std::shared_ptr< Value > value,
		U&& _val2
	)
	{
		typename std::decay< U >::type val2 =
			std::forward< U >( _val2 );

		return Parent::test( value, [ value, val2 ](
			const tuple_type& val1
		)
		{
			if ( val1 < val2 )
				return;

			auto& root = value->root( );

			ADD_FAILURE_AT( root.file( ), root.line( ) )
				<< "Expected: "
				<< "(" << root.val1_expr( ) << ")"
				<< " < "
				<< "(" << root.val2_expr( ) << ")"
				<< ", actual: "
				<< q::test::stringify_value( val1 ) << " vs "
				<< q::test::stringify_value( val2 );
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
 * Non-promise, non-tuple
 */
template< typename T >
class expected_comparator< T, false >
: public expected_comparator_base<
	expected_value< T >, expected_comparator< T, false >, T
>
{
public:
	typedef expected_comparator_base<
		expected_value< T >, expected_comparator< T, false >, T
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
 * Non-promise, tuple
 */
template< typename... T >
class expected_comparator< std::tuple< T... >, false >
: public expected_comparator_base<
	expected_value< std::tuple< T... > >,
	expected_comparator< std::tuple< T... >, false >,
	T...
>
{
public:
	typedef expected_comparator_base<
		expected_value< std::tuple< T... > >,
		expected_comparator< std::tuple< T... >, false >,
		T...
	> base;

	static void test(
		std::shared_ptr< expected_value< std::tuple< T... > > > value,
		typename base::testee_type testee
	)
	{
		testee( value->get( ) );
	}
};

/**
 * Generic promise (shared or unique)
 */
template< typename Promise, typename... T >
class expected_promise_comparator
: public expected_comparator_base<
	expected_value< Promise >,
	expected_promise_comparator< Promise, T... >,
	T...
>
{
public:
	typedef expected_comparator_base<
		expected_value< Promise >,
		expected_promise_comparator< Promise, T... >,
		T...
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
		auto value = this->get_value( );

		auto expectation = value->get( )
		.then( [ value ]( const std::tuple< T... >& t )
		{
			auto& root = value->root( );

			std::string val;
			if ( sizeof...( T ) == 0 )
				val = "But was resolved.";
			else
				val = "But was resolved to: " +
					q::test::stringify_value( t );

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
		auto value = this->get_value( );

		auto expectation = value->get( )
		.then( [ value ]( const std::tuple< T... >& t )
		{
			auto& root = value->root( );

			std::string val;
			if ( sizeof...( T ) == 0 )
				val = "But was resolved.";
			else
				val = "But was resolved to: " +
					q::test::stringify_value( t );

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
			.then( [ testee ]( const std::tuple< T... >& t )
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
			.then( [ testee ]( std::tuple< T... >&& t )
			{
				testee( std::move( t ) );
			} )
		);
	}
};

/**
 * Unique promise
 */
template< typename... T >
class expected_comparator< q::promise< T... >, true >
: public expected_promise_comparator< q::promise< T... >, T... >
{
public:
	typedef q::promise< T... > promise_type;
	typedef expected_comparator_base<
		expected_value< q::promise< T... > >,
		expected_promise_comparator< q::promise< T... >, T... >,
		T...
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
 * Shared promise
 */
template< typename... T >
class expected_comparator< q::shared_promise< T... >, true >
: public expected_promise_comparator<
	q::shared_promise< T... >, T...
>
{
public:
	typedef q::shared_promise< T... > promise_type;
	typedef expected_comparator_base<
		expected_value< q::shared_promise< T... > >,
		expected_promise_comparator< q::shared_promise< T... >, T... >,
		T...
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

} } } // namespace test, namespace rx, namespace q

#endif // LIBQ_RX_TEST_EXPECT_HPP
