
#include <q/type_traits.hpp>

#include "core.hpp"

Q_TEST_MAKE_SCOPE( memory );

TEST_F( memory, reinterpret_pointer_cast )
{
	bool done_inner = false;
	bool done_outer = false;
	auto destr = EXPECT_CALL_WRAPPER( [ & ]( )
	{
		EXPECT_TRUE( done_inner );
		done_outer = true;
	} );

	struct B
	{
		int i;
	};

	struct S : B
	{
		S( q::function< void( ) > fn )
		: fn_( fn )
		{
			i = 5;
		}

		~S( )
		{
			fn_( );
		}

		q::function< void( ) > fn_;
	};

	struct T : B
	{ };

	{
		std::shared_ptr< T > t_ptr;

		{
			auto s = std::make_shared< S >( destr );
			t_ptr = q::reinterpret_pointer_cast< T >( s );
		}

		EXPECT_EQ( 5, t_ptr->i );

		done_inner = true;

		EXPECT_FALSE( done_outer );
	}

	EXPECT_TRUE( done_outer );
}
