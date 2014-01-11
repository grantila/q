 q
===
A platform-independent asynchronous IoC library for C++.

Version
----
2.0

License
----
Apache License 2.0

What it is
==========

The library q is not following, but lightly mimicing the [Promises/A+](http://promises-aplus.github.io/promises-spec/) specification, which provides a simplistic and straightforward API for deferring functions, and invoke completions asynchronously. The name is borrowed from the well-known JavaScript implementation with the [same name](http://github.com/kriskowal/q).

q provides, a part from the pure asynchronous IoC methods, a wide range of tools and helpers to make the use as simple and obvious as possible, while still being perfectly type safe. One example if this, is the automatic `std::tuple` expansion of *promise chains*.

The concept of q is that tasks are dispatched on queues, and a queue is attached to an `event_dispatcher`. q comes with its own `blocking_dispatcher`, which is like an event loop, to dispatch asynchronous tasks in order, but it is also easily bridged with existing event loops, such as native Win32, GTK, QT or Node.js. However, q also comes with a thread pool, which also is an `event_dispatcher`.

> One of the most important reasons to use q is that one can run tasks not only asynchronous, but also on different threads, without the need to use mutexes and other locks to isolate data, as one instead will perform certain tasks on certain queues which dispatches the tasks only on certain threads.



Introduction
============

For a C++ programmer, it is likely easiest to learn and to understand the library by some examples.

Asynchronous tasks
------------------

The following example shows how q can be used for networking.
```c++
q::promise< std::tuple< std::string, std::string > > read_message_from_someone()
{
    return connect_to_server( )
    .then( [ ]( connection& c )
    {
        return c.get_next_message( );
    } );
}

...

read_message_from_someone( )
.then( [ ]( std::string&& username, std::string&& msg )
{
    std::cout << "User " << username << " says: " << msg << std::endl;
} )
.fail( [ ]( const ConnectionException& e )
{
    td::cerr << "Connection problem: " << e << std::endl;
} )
.fail( [ ]( std::exception_ptr e )
{
    td::cerr << "Unknown error" << std::endl;
} );
```

Asynchronous termination
------------------------

Terminating an object means to put it in a state where it is not performing anything anymore, and can be freed. However, sometimes there are references to the object, or it is internally not done with its inner tasks. It could e.g. have a thread that is currently working. q comes with a class called `async_termination` which can be subclassed, to provide a smooth way of terminating objects and waiting for them to complete.
```c++
auto object = q::make_shared< my_class >( );
object->perform_background_task( );
object->terminate( )
.then( [ ]( )
{
    std::cout << "object has now completed and can safely be freed" << std::endl;
} );
```


Installation
============

q uses CMake to generate build scripts.

Using Makefiles (for multiple platforms)
```sh
git clone git@github.com:grantila/q.git
cd q
./build.sh
```

For Xcode
```sh
git clone git@github.com:grantila/q.git
cd q
BUILDTYPE=Xcode ./build.sh ; open obj/q.xcodeproj
```
