/*
 * Copyright 2014 Gustaf Räntilä
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

#ifndef LIBP_ENV_HPP
#define LIBP_ENV_HPP

#include <string>
#include <memory>

namespace p {

class environment
{
public:
	environment( environment&& ) = default;
	~environment( );

	static environment current( );

	template< typename Map >
	void merge( Map&& values )
	{
		for ( auto& keyval : values )
			( *this )[ keyval.first ] = keyval.second;
	}

	class holder
	{
	public:
		std::string string( ) const;
		operator std::string( ) const;

		holder& operator=( std::string value );

		holder( ) = delete;
		holder( holder&& ) = default;
		holder( const holder& ) = delete;

	private:
		friend class environment;

		holder( environment& env, std::string&& name );

		environment& env_;
		std::string name_;
	};

	holder operator[ ]( std::string name );

private:
	environment( );

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} // namespace q

#endif // LIBP_ENV_HPP
