
#include "core.hpp"

Q_TEST_MAKE_SCOPE( set_default );

TEST_F( set_default, no_default )
{
	auto promise1 = q::with( queue ).share( );

	auto main_thread_name = q::detail::get_thread_name( );

	auto promise2 = promise1
	.then( EXPECT_CALL_WRAPPER(
		[ &main_thread_name ]( )
		{
			auto thread_name = q::detail::get_thread_name( );
			EXPECT_NE( main_thread_name, thread_name );
		}
	), tp_queue )
	.then( EXPECT_CALL_WRAPPER(
		[ &main_thread_name ]( )
		{
			auto thread_name = q::detail::get_thread_name( );
			EXPECT_EQ( main_thread_name, thread_name );
		}
	) );

	run(
		q::all(
			std::move( promise1 ),
			std::move( promise2 )
		)
	);
}

TEST_F( set_default, set_default )
{
	auto promise1 = q::with( queue ).share( );

	auto main_thread_name = q::detail::get_thread_name( );

	auto promise2 = promise1
	.then( EXPECT_CALL_WRAPPER(
		[ &main_thread_name ]( )
		{
			auto thread_name = q::detail::get_thread_name( );
			EXPECT_NE( main_thread_name, thread_name );
		}
	), q::set_default( tp_queue ) )
	.then( EXPECT_CALL_WRAPPER(
		[ &main_thread_name ]( )
		{
			auto thread_name = q::detail::get_thread_name( );
			EXPECT_NE( main_thread_name, thread_name );
		}
	) );

	run(
		q::all(
			std::move( promise1 ),
			std::move( promise2 )
		)
	);
}
