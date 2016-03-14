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

#ifndef LIBQIO_EVENT_HPP
#define LIBQIO_EVENT_HPP

#include <q-io/types.hpp>
#include <q-io/clock.hpp>
#include <q-io/dispatcher.hpp>

#include <memory>

namespace q { namespace io {

class event
{
public:
	~event( );

protected:
	event( );

	virtual void on_event_read( ) noexcept { }
	virtual void on_event_write( ) noexcept { }
	virtual void on_event_timeout( ) noexcept { }

	struct pimpl;
	std::unique_ptr< pimpl > pimpl_;

	dispatcher::pimpl& get_dispatcher_pimpl( );

	dispatcher_ptr get_dispatcher( );

private:
	friend class dispatcher;
	friend class server_socket;

	static void invoke( int fd, short events, void* arg );

	void attach( const dispatcher_ptr& dispatcher );

	virtual void
	sub_attach( const dispatcher_ptr& dispatcher ) noexcept = 0;
};

} } // namespace io, namespace q

#endif // LIBQIO_EVENT_HPP
