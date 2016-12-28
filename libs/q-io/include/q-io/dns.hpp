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

#ifndef LIBQIO_DNS_HPP
#define LIBQIO_DNS_HPP

#include <q-io/ip.hpp>
#include <q-io/types.hpp>

#include <q/promise.hpp>

#include <memory>

namespace q { namespace io {

struct resolver_response
{
	ip_addresses ips;
	std::chrono::seconds ttl;
};
/*
class resolver
: public std::enable_shared_from_this< resolver >
{
public:
	enum class settings
	{
		no_local_overrides     = 0x1,
		force_dispatcher_alive = 0x2
	};

	enum class resolve_flags
	{
		normal        = 0x1,
		no_dns_search = 0x2,
		ipv4          = 0x4,
		ipv6          = 0x8
	};

	~resolver( );

	q::promise< resolver_response >
	lookup(
		q::queue_ptr queue,
		const std::string& name,
		resolve_flags = resolve_flags::normal );

protected:
	resolver( dispatcher_ptr dispatcher, settings = settings( ) );

private:
	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;
};

Q_ENUM_FLAGS( resolver::settings )
Q_ENUM_FLAGS( resolver::resolve_flags )

*/

} } // namespace io, namespace q

#endif // LIBQIO_DNS_HPP
