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

#ifndef LIBQ_FUNCTION_HPP
#define LIBQ_FUNCTION_HPP

#include <q/functional.hpp>

/**
 * q::function and q::unique_function can be configured (optimized) at compile
 * time, to match the target hardware more appropriately. The defaults are good
 * though.
 *
 * The following macros are available:
 *
 *   Q_RECORD_FUNCTION_STATS:
 *     Define to enable function (type and size) statistics to be printed at
 *     program exit. This slows down the application slightly (not
 *     dramatically).
 *
 *   Q_USE_FUNCTION_SIZE:
 *     The size of the q::function and q::unique_function. A larger value makes
 *     all functions take more space, but allows larger function objects (e.g.
 *     lambdas) to be inlined which improves speed. Defaults to a multiple of
 *     the (assumed) cache line size.
 *
 *   Q_USE_FUNCTION_ALIGN:
 *     The enforced alignment of the functions. Defaults to (assumed) cache
 *     line size.
 */

#ifdef Q_RECORD_FUNCTION_STATS
#	include <unordered_map>
#	include <map>
#	include <iostream>
#	include <thread>
#endif // Q_RECORD_FUNCTION_STATS

#ifdef Q_USE_FUNCTION_SIZE
#	define LIBQ__FUNCTION_INLINE_SIZE Q_USE_FUNCTION_INLINE_SIZE
#else
	// The default function size is twice the cache line size, which is
	// assumed to be 8 words (e.g. 64 bytes in 64-bit systems)
#	define LIBQ__FUNCTION_INLINE_SIZE_( ) \
		( 2 * LIBQ_ASSUMED_CACHE_LINE_SIZE )
#	define LIBQ__FUNCTION_INLINE_SIZE LIBQ__FUNCTION_INLINE_SIZE_( )
#endif
#ifdef Q_USE_FUNCTION_ALIGN
#	define LIBQ__FUNCTION_INLINE_ALIGN Q_USE_FUNCTION_ALIGN
#else
#	define LIBQ__FUNCTION_INLINE_ALIGN LIBQ_ASSUMED_CACHE_LINE_SIZE
#endif
#define LIBQ__FUNCTION_INLINE_STATE_SIZE sizeof( std::ptrdiff_t )

namespace q {

namespace detail {

template< typename Signature, typename Args = ::q::arguments_of_t< Signature > >
struct function_base;

template<
	typename Fn,
	typename Signature,
	bool Copyable = std::is_copy_constructible< Fn >::value,
	typename Args = ::q::arguments_of_t< Signature >
>
struct specific_function;

template< typename Signature, typename... Args >
struct function_base< Signature, ::q::arguments< Args... > >
{
	using this_type = function_base< Signature, ::q::arguments< Args... > >;
	using return_type = ::q::result_of_t< Signature >;

	virtual ~function_base( ) { }

	virtual bool is_copyable( ) const = 0;

	virtual bool is_mutable( ) const = 0;

	virtual return_type operator( )( Args&&... ) = 0;

	virtual this_type* move_to( void* ) = 0;

	virtual std::shared_ptr< this_type > move_to_shared( ) = 0;

	virtual std::shared_ptr< this_type > copy_to_shared( ) const = 0;

	virtual std::unique_ptr< this_type > copy_to_unique( ) const = 0;

	virtual this_type* copy_to( void* ) const = 0;

	virtual std::size_t size( ) const = 0;

protected:
	function_base( ) { }
	function_base( const function_base& ) = delete;
	function_base( function_base&& ) = default;

	function_base& operator=( const function_base& ) = delete;
	function_base& operator=( function_base&& ) = delete;
};

template< typename Fn, typename Signature, bool Copyable, typename... Args >
struct specific_function< Fn, Signature, Copyable, ::q::arguments< Args... > >
: function_base< Signature >
{
	using base = function_base< Signature >;
	using return_type = typename base::return_type;
	using this_type = specific_function< Fn, Signature, Copyable >;

	specific_function( ) = delete;
	specific_function( specific_function&& ) = default;
	specific_function( const specific_function& ref )
	: fn_( ref.fn_ )
	{
		static_assert( Copyable,
			"The function is not copyable. "
			"(This is an internal error, please report)" );
	}

	specific_function( Fn&& fn )
	: fn_( std::move( fn ) )
	{ }

	specific_function( const Fn& fn )
	: fn_( fn )
	{ }

	~specific_function( )
	{ }

	bool is_copyable( ) const override
	{
		return Copyable;
	}

	bool is_mutable( ) const override
	{
		return is_mutable_of_t< Fn >::value;
	}

	return_type operator( )( Args&&... args ) override
	{
		return fn_( std::forward< Args >( args )... );
	}

	base* move_to( void* dest ) override
	{
		return ::new ( dest ) this_type( std::move( *this ) );
	}

	std::shared_ptr< base > move_to_shared( ) override
	{
		return std::make_shared< this_type >( std::move( *this ) );
	}

	std::shared_ptr< base > copy_to_shared( ) const override
	{
		return _copy_to_shared( );
	}

	std::unique_ptr< base > copy_to_unique( ) const override
	{
		return _copy_to_unique( );
	}

	base* copy_to( void* dest ) const override
	{
		return _copy_to( dest );
	}

	std::size_t size( ) const override
	{
		return sizeof( *this );
	}

private:
	template< bool C = Copyable >
	typename std::enable_if< C, base* >::type
	_copy_to( void* dest ) const
	{
		return ::new ( dest ) this_type( *this );
	}

	template< bool C = Copyable >
	typename std::enable_if< !C, base* >::type
	_copy_to( void* dest ) const
	{
		// This is just to make the compiler happy. We'll never try to
		// copy Fn from unique_function's, so this is not an issue.
		throw std::logic_error( "q::function internal error" );
	}

	template< bool C = Copyable >
	typename std::enable_if< C, std::unique_ptr< base > >::type
	_copy_to_unique( ) const
	{
		return q::make_unique< this_type >( fn_ );
	}

	template< bool C = Copyable >
	typename std::enable_if< !C, std::unique_ptr< base > >::type
	_copy_to_unique( ) const
	{
		throw std::logic_error( "q::function internal error" );
	}

	template< bool C = Copyable >
	typename std::enable_if< C, std::shared_ptr< base > >::type
	_copy_to_shared( ) const
	{
		return std::make_shared< this_type >( fn_ );
	}

	template< bool C = Copyable >
	typename std::enable_if< !C, std::shared_ptr< base > >::type
	_copy_to_shared( ) const
	{
		throw std::logic_error( "q::function internal error" );
	}

	Fn fn_;
};

enum class function_storage
{
	uninitialized = 0, // No function assigned
	plain = 1,         // Plain function (no function object), and no base*
	inlined = 2,       // Placement-new constructed in-place
	unique_ptr = 3,    // unique_ptr'd
	shared_ptr = 4     // shared_ptr'd
};

template< typename Fn, typename Signature, bool Shared, std::size_t DataSize >
struct suitable_storage_method
{
	/**
	 * The storage is initially as follows:
	 *   1. If the function fits inline, it will be inline, if:
	 *      * Shared is false (it's a unique_function)
	 *        or
	 *      * Shared is true (a function) and Fn is copyable
	 *   2. If the function will not be stored inline, it will be stored:
	 *      * In a unique_ptr if Shared is false (unique_function).
	 *        This can change if the unique_function is moved to a function
	 *        in which case it is transformed into a shared_ptr. If it's
	 *        then converted back to a unique_function again, it'll remain
	 *        a shared_ptr.
	 *      * In a unique_ptr if Shared is true (but transformed into a
	 *        shared_ptr once this function is copied).
	 */

	typedef std::is_copy_constructible< Fn > is_copyable;

	typedef bool_type<
		sizeof( specific_function< Fn, Signature > ) <= DataSize
		and
		// We shouldn't inline non-copyable lambdas into shared
		// functions
		( !Shared or is_copyable::value )
	> should_be_inlined;

	typedef typename std::integral_constant<
		function_storage,
		is_plain_function_t< Fn >::value
			? function_storage::plain
			: should_be_inlined::value
				? function_storage::inlined
				: Shared and !is_copyable::value
					? function_storage::shared_ptr
					: function_storage::unique_ptr
	> type;
};

[[noreturn]] void _throw_bad_function_call_exception( );

#ifdef Q_RECORD_FUNCTION_STATS
template< typename T = void >
struct function_size_recorder
{
	void add( std::size_t sz, bool shared, function_storage type )
	{
		std::unique_lock< std::mutex > l( mut_ );
		++recorded_sizes_[ sz ];
		++shared_[ shared ];
		++types_[ static_cast< std::uint16_t >( type ) ];
	}

	std::unordered_map< std::size_t, std::size_t > recorded_sizes_;
	std::unordered_map< bool, std::size_t > shared_;
	std::unordered_map< std::int16_t, std::size_t > types_;

	~function_size_recorder( )
	{
		std::map< std::size_t, std::size_t > ordered_sizes(
			recorded_sizes_.begin( ), recorded_sizes_.end( ) );

		std::cout << "Function sizes:" << std::endl;

		bool cache_lines[ ] = { false, false, false };
		std::size_t cur_cache_line = 0;
		std::size_t cache_line = 0;

		auto summarize = [ & ]( std::size_t sz = 999 )
		{
			while (
				cur_cache_line < 3 &&
				!cache_lines[ cur_cache_line ] &&
				( cur_cache_line + 1 ) * 64 < sz + 16
			)
			{
				cache_lines[ cur_cache_line++ ] = true;
				std::cout
					<< "\t==> Up to "
					<< ( cur_cache_line * 64 )
					<< " - 16 bytes: "
					<< cache_line << std::endl;
				cache_line = 0;
			}
		};

		for ( auto& p : ordered_sizes )
		{
			std::size_t sz = p.first;
			std::size_t num = p.second;

			summarize( sz );

			cache_line += num;
			std::cout
				<< "\tSize " << sz
				<< " allocated " << num
				<< " times" << std::endl;
		}
		summarize( );

		std::cout << "Methods: "
			<< types_[ 1 ] << " plain "
			<< types_[ 2 ] << " inlined "
			<< types_[ 3 ] << " unique "
			<< types_[ 4 ] << " shared."
			<< std::endl;

		std::cout << "Total: "
			<< shared_[ false ] << " unique and "
			<< shared_[ true ] << " shared."
			<< std::endl;
	}

	static function_size_recorder< T > instance;

private:
	std::mutex mut_;
};
template< typename T >
function_size_recorder< T > function_size_recorder< T >::instance;
#endif // Q_RECORD_FUNCTION_STATS

template< typename Signature, bool Shared, std::size_t TotalSize >
class alignas( LIBQ__FUNCTION_INLINE_ALIGN ) any_function
: copyable_if_t< Shared >
{
public:
	typedef std::integral_constant<
		std::size_t,
		TotalSize - 2 * LIBQ__FUNCTION_INLINE_STATE_SIZE
	> DataSize;
	using this_type = any_function< Signature, Shared, TotalSize >;
	using shared_type = any_function< Signature, true, TotalSize >;

	using base = detail::function_base< Signature >;
	using argument_types = arguments_of_t< Signature >;
	using return_type = typename base::return_type;
	using shared_heap_type = std::shared_ptr< base >;
	using unique_heap_type = std::unique_ptr< base >;

	template<
		typename Fn,
		typename DecayedFn = typename std::decay< Fn >::type
	>
	using matching_function = bool_type<
		is_function_t< DecayedFn >::value
		and
		!std::is_same< DecayedFn, this_type >::value
		and
		arguments_of_t< Signature >
		::template is_convertible_to<
			arguments_of_t< DecayedFn >
		>::value
		and
		result_of_as_argument_t< DecayedFn >
		::template is_convertible_to<
			result_of_as_argument_t< Signature >
		>::value
		and
		std::is_move_constructible< DecayedFn >::value
		and
		(
			(
				std::is_rvalue_reference< Fn&& >::value
				and
				!std::is_const< Fn >::value
			)
			or
			std::is_copy_constructible< DecayedFn >::value
		)
		and
		(
			// Mutable non-copyable lambas should not be
			// allowed in q::function:s, only
			// q::unique_function:s.
			!Shared
			or
			!is_mutable_of_t< DecayedFn >::value
			or
			std::is_copy_constructible< DecayedFn >::value
		)
	>;

	any_function( ) noexcept
	: method_( function_storage::uninitialized )
	{ }

	any_function( std::nullptr_t ) noexcept
	: method_( function_storage::uninitialized )
	{ }

	template< typename Fn >
	any_function(
		Fn&& fn,
		typename std::enable_if<
			matching_function< Fn >::value
			and
			std::is_copy_constructible<
				typename std::decay< Fn >::type
			>::value
		>::type* = 0
	)
	: method_( function_storage::uninitialized )
	{
		typedef detail::specific_function<
			typename std::decay< Fn >::type,
			Signature
		> specific_base;

		using method = typename detail::suitable_storage_method<
			typename std::decay< Fn >::type,
			Signature,
			Shared,
			DataSize::value
		>::type;

#ifdef Q_RECORD_FUNCTION_STATS
		function_size_recorder< >::instance.add(
			sizeof( specific_base ), Shared, method::value );
#endif // Q_RECORD_FUNCTION_STATS

		_set_plain< method::value >( std::forward< Fn >( fn ) );

		if ( method::value == function_storage::inlined )
			ptr_ = ::new ( &base_ ) specific_base(
				std::forward< Fn >( fn ) );

		else if ( method::value == function_storage::unique_ptr )
		{

			::new ( &base_ ) unique_heap_type(
				q::make_unique< specific_base >(
					std::forward< Fn >( fn ) ) );
			ptr_ = reinterpret_cast< unique_heap_type* >( &base_ )
				->get( );
		}

		else if ( method::value == function_storage::shared_ptr )
		{
			::new ( &base_ ) shared_heap_type(
				std::make_shared< specific_base >(
					std::forward< Fn >( fn ) ) );
			ptr_ = reinterpret_cast< shared_heap_type* >( &base_ )
					->get( );
		}

		method_ = method::value;
	}

	template< typename Fn >
	any_function(
		Fn&& fn,
		typename std::enable_if<
			matching_function< Fn >::value
			and
			!std::is_copy_constructible<
				typename std::decay< Fn >::type
			>::value
			and
			std::is_rvalue_reference< Fn&& >::value
		>::type* = 0
	)
	: method_( function_storage::uninitialized )
	{
		typedef detail::specific_function<
			typename std::decay< Fn >::type,
			Signature
		> specific_base;

		using method = typename detail::suitable_storage_method<
			typename std::decay< Fn >::type,
			Signature,
			Shared,
			DataSize::value
		>::type;

#ifdef Q_RECORD_FUNCTION_STATS
		function_size_recorder< >::instance.add(
			sizeof( specific_base ), Shared, method::value );
#endif // Q_RECORD_FUNCTION_STATS

		_set_plain< method::value >( std::forward< Fn >( fn ) );

		if ( method::value == function_storage::inlined )
			ptr_ = ::new ( &base_ ) specific_base( std::move( fn ) );

		else if ( method::value == function_storage::unique_ptr )
		{
			::new ( &base_ ) unique_heap_type(
				q::make_unique< specific_base >(
					std::move( fn ) ) );
			ptr_ = reinterpret_cast< unique_heap_type* >( &base_ )
				->get( );
		}

		else if ( method::value == function_storage::shared_ptr )
		{
			::new ( &base_ ) shared_heap_type(
				std::make_shared< specific_base >(
					std::move( fn ) ) );
			ptr_ = reinterpret_cast< shared_heap_type* >( &base_ )
					->get( );
		}

		method_ = method::value;
	}

	any_function( any_function&& ref )
	: method_( function_storage::uninitialized )
	{
		_move_from( std::move( ref ) );
	}

	any_function( const any_function& ref )
	: method_( function_storage::uninitialized )
	{
		_copy_from( ref );
	}

	/**
	 * Move construct unique_function from function. The other way isn't
	 * possible (needs share() to be called).
	 */
	template< bool _Shared = Shared >
	any_function(
		shared_type&& other,
		typename std::enable_if< !_Shared && !Shared >::type* = 0
	)
	: method_( function_storage::uninitialized )
	{
		_move_from_shared( std::move( other ) );
	}

	/**
	 * Copy construct unique_function from function. The other way isn't
	 * possible (needs share() to be called).
	 */
	template< bool _Shared = Shared >
	any_function(
		const shared_type& other,
		typename std::enable_if< !_Shared && !Shared >::type* = 0
	)
	: method_( function_storage::uninitialized )
	{
		_copy_from_shared( other );
	}

	~any_function( )
	{
		_reset( );
	}

	/**
	 * Convert this unique_function into a function. Both the returned
	 * function and the current unique_function will continue to work.
	 *
	 * NOTE; This function may cause the wrapped function to be shared
	 * between function instances, which may cause bad side effects e.g. if
	 * the wrapped function is a capturing mutable lambda. Use with care!
	 *
	 * When move assigning, the following possible situations exist:
	 *
	 * 1. {unique -> shared} plain:      Keep
	 * 2. {unique -> shared} inlined:    Keep inlined if copyable (a),
	 *                                   otherwise move to shared_ptr (b)
	 * 3. {unique -> shared} unique_ptr: Keep unique if mutable (and
	 *                                   copyable of course) (a), otherwise
	 *                                   convert to shared_ptr (b)
	 * 4. {unique -> shared} shared_ptr: Keep
	 *
	 * Method 2 and 3 may transform `this` and re-allocate the wrapped
	 * function. This may also make potential capturing mutable lambas
	 * into non-pure functions, if they aren't copyable. That is to be
	 * expected though.
	 */
	template< bool _Shared = Shared >
	typename std::enable_if<
		!_Shared and !Shared,
		shared_type
	>::type
	share( )
	{
		shared_type ret;

		if ( method_ == function_storage::uninitialized )
			return ret;

		else if ( method_ == function_storage::plain ) // 1
		{
			ret.sig_ = sig_;
			ret.method_ = method_;
			return ret;
		}

		base* _base = _get_base( );

		const bool is_copyable = _base->is_copyable( );
		const bool is_mutable = _base->is_mutable( );

		const bool keep_inlined = // 2a
			method_ == function_storage::inlined && is_copyable;

		const bool inline_to_shared_ptr = // 2b
			method_ == function_storage::inlined && !is_copyable;

		const bool keep_unique_ptr =
			method_ == function_storage::unique_ptr &&
			is_copyable &&
			is_mutable; // 3a

		const bool convert_unique_to_shared_ptr =
			method_ == function_storage::unique_ptr; // 3b

		if ( keep_inlined ) // 2a
		{
			// We can place it inline, and later allow copying it
			// if necessary.
			ret.ptr_ = _base->copy_to( &ret.base_ );
			ret.method_ = function_storage::inlined;
		}
		else if ( inline_to_shared_ptr ) // 2b
		{
			shared_heap_type rebound( _base->move_to_shared( ) );
			method_ = function_storage::uninitialized;
			_base->~base( );

			::new ( &base_ ) shared_heap_type( rebound );
			ptr_ = rebound.get( );
			method_ = function_storage::shared_ptr;

			::new ( &ret.base_ ) shared_heap_type( rebound );
			ret.ptr_ = rebound.get( );
			ret.method_ = function_storage::shared_ptr;
		}
		else if ( keep_unique_ptr ) // 3a
		{
			::new ( &ret.base_ ) unique_heap_type(
				_base->copy_to_unique( ) );
			ret.ptr_ = reinterpret_cast< unique_heap_type* >(
				&ret.base_
			)->get( );
			ret.method_ = function_storage::unique_ptr;
		}
		else if ( convert_unique_to_shared_ptr ) // 3b
		{
			// This method actually changes the storage method in
			// in this unique_function.
			auto this_unique_ptr =
				reinterpret_cast< unique_heap_type* >( &base_ );

			shared_heap_type rebound( this_unique_ptr->release( ) );
			method_ = function_storage::uninitialized;
			this_unique_ptr->~unique_ptr( );

			::new ( &base_ ) shared_heap_type( rebound );
			ptr_ = rebound.get( );
			method_ = function_storage::shared_ptr;

			ret.method_ = function_storage::shared_ptr;
			::new ( &ret.base_ ) shared_heap_type( rebound );
			ret.ptr_ = rebound.get( );
			method_ = function_storage::shared_ptr;
		}
		else // 4
		{
			auto this_shared_ptr =
				reinterpret_cast< shared_heap_type* >( &base_ );

			::new ( &ret.base_ ) shared_heap_type(
				*this_shared_ptr );
			ret.ptr_ = this_shared_ptr->get( );
			ret.method_ = function_storage::shared_ptr;
		}

		return ret;
	}

	any_function& operator=( any_function&& ref )
	{
		return _move_from( std::move( ref ) );
	}

	any_function& operator=( const any_function& ref )
	{
		return _copy_from( ref );
	}

	template< bool _Shared = Shared >
	typename std::enable_if< !_Shared && !Shared, this_type >::type&
	operator=( shared_type&& other )
	{
		return _move_from_shared( std::move( other ) );
	}

	template< bool _Shared = Shared >
	typename std::enable_if< !_Shared && !Shared, this_type >::type&
	operator=( const shared_type& other )
	{
		return _copy_from_shared( other );
	}

	template< typename... Args >
	typename std::enable_if<
		arguments< Args... >::template is_convertible_to<
			argument_types
		>::value,
		return_type
	>::type
	operator( )( Args&&... args )
	{
		if ( !*this )
			detail::_throw_bad_function_call_exception( );

		if ( method_ == function_storage::plain )
		{
			return ( *sig_ )( std::forward< Args >( args )... );
		}

		base* _base = _get_base( );

		if ( !_base )
			detail::_throw_bad_function_call_exception( );

		return ( *_base )( std::forward< Args >( args )... );
	}

	bool operator!( ) const
	{
		return method_ == function_storage::uninitialized;
	}

	operator bool( ) const
	{
		return method_ != function_storage::uninitialized;
	}

private:
	template< typename, bool, std::size_t >
	friend class any_function;

	template< function_storage method, typename Fn >
	typename std::enable_if<
		method == function_storage::plain
	>::type
	_set_plain( Fn&& fn )
	{
		sig_ = static_cast< Signature* >( std::forward< Fn >( fn ) );
		method_ = method;
	}

	template< function_storage method, typename Fn >
	typename std::enable_if<
		method != function_storage::plain
	>::type
	_set_plain( Fn&& fn )
	{ }

	// Beware, the constness is lost
	base* _get_base( ) const
	{
		return reinterpret_cast< base* >( ptr_ );
	}

	void _reset( )
	{
		if ( method_ == function_storage::inlined )
			_get_base( )->~base( );
		else if ( method_ == function_storage::unique_ptr )
			reinterpret_cast< unique_heap_type* >( &base_ )
				->~unique_ptr( );
		else if ( method_ == function_storage::shared_ptr )
			reinterpret_cast< shared_heap_type* >( &base_ )
				->~shared_ptr( );

		method_ = function_storage::uninitialized;
	}

	any_function& _move_from( any_function&& ref )
	{
		if ( &ref == this )
			return *this;

		_reset( );

		auto om = ref.method_;

		if ( om == function_storage::uninitialized )
			return *this;

		else if ( om == function_storage::plain )
		{
			sig_ = ref.sig_;
		}
		else if ( om == function_storage::inlined )
		{
			auto ref_base = ref._get_base( );

			ptr_ = ref_base->move_to( &base_ );
		}
		else if ( om == function_storage::unique_ptr )
		{
			auto ref_unique_ptr =
				reinterpret_cast< unique_heap_type* >(
					&ref.base_ );

			// Move-construct ref into this
			::new ( &base_ ) unique_heap_type(
				std::move( *ref_unique_ptr ) );
			ptr_ = reinterpret_cast< unique_heap_type* >( &base_ )
				->get( );
		}
		else if ( om == function_storage::shared_ptr )
		{
			auto ref_shared_ptr =
				reinterpret_cast< shared_heap_type* >(
					&ref.base_ );

			// Move-construct ref into this
			::new ( &base_ ) shared_heap_type(
				std::move( *ref_shared_ptr ) );
			ptr_ = reinterpret_cast< shared_heap_type* >( &base_ )
				->get( );
		}

		method_ = om;

		ref._reset( );

		return *this;
	}

	/**
	 * When copy assigning, the following possible situations exist:
	 *
	 * 1. plain:      Keep
	 * 2. inlined:    Keep (must be copyable, ensured in ctor)
	 * 3. unique_ptr: Copy to shared_ptr (must be copyable, ensured in
	 *                ctor), since it might be copied multiple times, this
	 *                will ensure only one real copy. (a)
	 *                If the wrapped function is copyable an mutable, we'll
	 *                keep it a unique_ptr (b)
	 * 4. shared_ptr: Keep
	 */
	any_function& _copy_from( const any_function& ref )
	{
		if ( &ref == this )
			return *this;

		_reset( );

		auto om = ref.method_;

		if ( om == function_storage::uninitialized )
		{
			return *this;
		}
		else if ( om == function_storage::plain ) // 1
		{
			sig_ = ref.sig_;
			method_ = om;
			return *this;
		}

		const base* _base = ref._get_base( );

		const bool is_copyable = _base->is_copyable( );
		const bool is_mutable = _base->is_mutable( );

		const bool keep_unique_ptr =
			om == function_storage::unique_ptr &&
			is_copyable &&
			is_mutable; // 3b
		const bool convert_unique_ptr_to_shared_ptr =
			om == function_storage::unique_ptr &&
			!keep_unique_ptr; // 3a

		if ( om == function_storage::inlined ) // 2
		{
			ptr_ = _base->copy_to( &base_ );
		}
		else if ( convert_unique_ptr_to_shared_ptr ) // 3a
		{
			// This is the first time we copy this function, so we
			// copy this unique_ptr to shared_ptr and then rely on
			// copying (ref-increment) that.
			::new ( &base_ ) shared_heap_type(
				_base->copy_to_shared( ) );
			ptr_ = reinterpret_cast< shared_heap_type* >( &base_ )
					->get( );
			method_ = function_storage::shared_ptr;
			return *this;
		}
		else if ( keep_unique_ptr ) // 3b
		{
			::new ( &base_ ) unique_heap_type(
				_base->copy_to_unique( ) );
			ptr_ = reinterpret_cast< unique_heap_type* >( &base_ )
					->get( );
		}
		else if ( om == function_storage::shared_ptr ) // 4
		{
			auto ref_shared_ptr =
				reinterpret_cast< const shared_heap_type* >(
					&ref.base_ );

			// Copy-construct shared_ptr
			::new ( &base_ ) shared_heap_type( *ref_shared_ptr );
			ptr_ = reinterpret_cast< shared_heap_type* >( &base_ )
				->get( );
		}

		method_ = om;

		return *this;
	}

	/**
	 * When move assigning, the following possible situations exist:
	 *
	 * 1. {shared -> unique} plain:      Keep
	 * 2. {shared -> unique} inlined:    Keep
	 * 3. {shared -> unique} unique_ptr: Keep
	 * 4. {shared -> unique} shared_ptr: Keep shared_ptr'd if not unique
	 *                                   ownership (i.e. use_count > 1),
	 *                                   otherwise convert to inlined if
	 *                                   there's room for it.
	 */
	template< bool _Shared = Shared >
	typename std::enable_if< !_Shared && !Shared, this_type >::type&
	_move_from_shared( shared_type&& other )
	{
		_reset( );

		auto om = other.method_;
		if ( om == function_storage::uninitialized )
			return *this;

		else if ( om == function_storage::plain ) // 1
		{
			sig_ = other.sig_;
			method_ = om;
			other._reset( );
			return *this;
		}

		auto other_base = other._get_base( );

		if ( om == function_storage::inlined ) // 2
		{
			// We can place it inline, and later allow copying it
			// if necessary.
			ptr_ = other_base->move_to( &base_ );
			method_ = function_storage::inlined;
			other._reset( );
			return *this;
		}
		else if ( om == function_storage::unique_ptr ) // 3
		{
			// Move other's unique_ptr into this
			auto& other_unique_ptr =
				*reinterpret_cast< unique_heap_type* >(
					&other.base_ );
			::new ( &base_ ) unique_heap_type(
				std::move( other_unique_ptr ) );
			ptr_ = reinterpret_cast< unique_heap_type* >( &base_ )
					->get( );
			method_ = function_storage::unique_ptr;
			other._reset( );
			return *this;
		}

		/**
		 * We've reached method 4, which means we keep the shared_ptr,
		 * i.e. move it from 'other' into 'this'.
		 * But, if there's room to store it inline, and 'other' is the
		 * unique owner (i.e. use_count > 1), we'll move it from the
		 * shared_ptr into inline this. Whenever it's used, it'll mean
		 * one less memory fetch.
		 */

		auto other_shared_ptr =
			reinterpret_cast< shared_heap_type* >( &other.base_ );

		const bool should_inline =
			other_base->size( ) <= DataSize::value
			and
			other_shared_ptr->unique( );

		if ( should_inline )
		{
			ptr_ = other_base->move_to( &base_ );
			method_ = function_storage::inlined;
		}
		else
		{
			::new ( &base_ ) shared_heap_type(
				std::move( *other_shared_ptr ) );
			ptr_ = reinterpret_cast< shared_heap_type* >( &base_ )
				->get( );
			method_ = function_storage::shared_ptr;
		}

		other._reset( );

		return *this;
	}

	/**
	 * When copy assigning, the following possible situations exist:
	 *
	 * 1. {shared -> unique} plain:      Keep plain
	 * 2. {shared -> unique} inlined:    Keep inlined
	 * 2. {shared -> unique} unique_ptr: Keep unique_ptr
	 * 3. {shared -> unique} shared_ptr: Keep shared_ptr
	 */
	template< bool _Shared = Shared >
	typename std::enable_if< !_Shared && !Shared, this_type >::type&
	_copy_from_shared( const shared_type& other )
	{
		_reset( );

		auto om = other.method_;
		if ( om == function_storage::uninitialized )
			return *this;

		else if ( om == function_storage::plain ) // 1
		{
			sig_ = other.sig_;
			method_ = om;
		}
		else if ( om == function_storage::inlined ) // 2
		{
			const auto other_base = other._get_base( );

			ptr_ = other_base->copy_to( &base_ );
			method_ = function_storage::inlined;
		}
		else if ( om == function_storage::unique_ptr ) // 3
		{
			auto& other_unique_ptr =
				*reinterpret_cast< const unique_heap_type* >(
					&other.base_ );

			::new ( &base_ ) unique_heap_type(
				other_unique_ptr->copy_to_unique( ) );
			ptr_ = reinterpret_cast< unique_heap_type* >( &base_ )
				->get( );
			method_ = function_storage::unique_ptr;
		}
		else // 4
		{
			auto other_shared_ptr =
				reinterpret_cast< const shared_heap_type* >(
					&other.base_ );

			::new ( &base_ ) shared_heap_type( *other_shared_ptr );
			ptr_ = reinterpret_cast< shared_heap_type* >( &base_ )
				->get( );
			method_ = function_storage::shared_ptr;
		}

		return *this;
	}

	typedef typename std::aligned_storage<
		DataSize::value,
		LIBQ__FUNCTION_INLINE_STATE_SIZE * 2
	>::type data_type;

	data_type base_;
	function_storage method_;
	union
	{
		base* ptr_;
		Signature* sig_;
	};
};

} // namespace detail

template< typename Signature >
using unique_function = detail::any_function<
	Signature,
	false,
	LIBQ__FUNCTION_INLINE_SIZE
>;

template< typename Signature >
using function = detail::any_function<
	Signature,
	true,
	LIBQ__FUNCTION_INLINE_SIZE
>;

template< typename Signature, bool Shared, std::size_t Words >
using custom_function = detail::any_function<
	Signature,
	Shared,
	// Add three words (two for the any_function, one for the v-table),
	// then round up to nearest 8-word (assumed cache line size).
	sizeof( std::ptrdiff_t ) * ( ( Words + 3 + 7 ) / 8 ) * 8
>;

} // namespace q

#endif // LIBQ_FUNCTION_HPP
