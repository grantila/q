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

#ifndef LIBQIO_UDP_PACKET_HPP
#define LIBQIO_UDP_PACKET_HPP

#include <q-io/ip.hpp>
#include <q-io/types.hpp>

#include <q/block.hpp>
#include <q/expect.hpp>
#include <q/exception.hpp>

namespace q { namespace io {

Q_DEFINE_ENUM_FLAGS( udp_bind )
{
	ip_v6_only = 0x1,
	reuseaddr  = 0x2
};

Q_MAKE_SIMPLE_EXCEPTION( udp_packet_error );

struct udp_packet
{
	q::expect< q::byte_block > data;
	ip_address remote_address;
	std::uint16_t remote_port;
};

} } // namespace io, namespace q

#endif // LIBQIO_UDP_PACKET_HPP
