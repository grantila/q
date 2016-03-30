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

#include <q/lib.hpp>

#include "internals.hpp"

namespace {

static struct qurl_init
{
public:
	qurl_init( )
	{
		auto init = [ ]( ) -> void*
		{
			auto ret = curl_global_init( CURL_GLOBAL_DEFAULT );
			if ( CURLE_OK != ret )
				Q_THROW( q::exception( ) );

			return nullptr;
		};

		auto uninit = [ ]( void* ) -> void
		{
			curl_global_cleanup( );
		};

		q::register_initialization( init, uninit );
	}
} qurl_init_;

} // anonymous namespace
