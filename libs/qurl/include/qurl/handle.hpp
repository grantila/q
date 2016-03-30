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

#ifndef LIBQURL_HANDLE_HPP
#define LIBQURL_HANDLE_HPP

#include <qurl/types.hpp>

namespace qurl {

class handle
: public std::enable_shared_from_this< handle >
{
public:
	~handle( );

	void get_stuff( );

protected:
	handle( context_ptr );

private:
	friend class context;

	static handle_ptr construct( context_ptr );

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

} // namespace qurl

#endif // LIBQURL_HANDLE_HPP
