 q
===

[![Join the chat at https://gitter.im/i-promise/Lobby](https://badges.gitter.im/i-promise/Lobby.svg)](https://gitter.im/i-promise/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
A platform-independent promise library for C++, implementing asynchronous continuations.

Tested on GCC (Linux), XCode (OSX) and Visual Studio 2015 (Windows).

 * [x] Web: [libq.io](http://libq.io/) ([API](http://libq.io/api))
 * [x] Twitter: [@grantila](https://twitter.com/grantila)

[![Build Status](https://semaphoreci.com/api/v1/gustaf/q/branches/master/badge.svg)](https://semaphoreci.com/gustaf/q)

Version
----
Still unversioned until closing in on 1.0. The API is likely not going to change until 1.0.

License
----
Apache License 2.0

What it is
==========

The library q (or libq) is following the core ideas and naming in the [Promises/A+](http://promises-aplus.github.io/promises-spec/) specification, which provides a simplistic and straightforward API for deferring functions, and invoke completions asynchronously. The name is borrowed from the well-known JavaScript implementation with the [same name](http://github.com/kriskowal/q), but is influenced more by [Bluebird](http://bluebirdjs.com/) by providing a lot of functional tools such as `filter`, `map` and `reduce` (yet to be implemented / decided to be implemented) as well as `finally`, `reflect` and `tap`. Although q provides a large amount of features, they all exist to make the promise implementation as solid and easy-to-use as possible. A generic and useful event based routine `delay` is implemented in core q, but more high-resolution timer events as well as other event based asynchronous I/O (e.g. file or network), is not built-in. Instead, this is provided by support libraries (like q-io). This is to keep `q` free from any dependencies, it only depends on C++11. The built-in `delay` is supported by the `blocking_dispatcher` and `threadpool`, but are of low-resolution system timers.

q provides, a part from the pure asynchronous IoC methods, a wide range of tools and helpers to make the use as simple and obvious as possible, while still being perfectly type safe. One example if this, is the automatic `std::tuple` expansion of *promise chains*.

The concept of q is that tasks are dispatched on queues, and a queue is attached to an `event_dispatcher`. q comes with its own `blocking_dispatcher`, which is like an event loop, to dispatch asynchronous tasks in order, but it is also easily bridged with existing event loops, such as native Win32, GTK, QT or Node.js. However, q also comes with a thread pool, which also is an `event_dispatcher`.

> One of the most important reasons to use q is that one can run tasks not only asynchronous, but also on different threads, without the need to use mutexes and other locks to isolate data. Instead, one will perform tasks on certain queues which dispatches the tasks only on certain threads. This separation of logic and execution provides a means to decide on which thread a certain function should be called, allowing shared state to always be accessed from the same thread. It also provides fantastic means to, at any time, change what logic is supposed to run on what threads to optimize the program for performance.


Components and features
=======================

q primarily features a *promise* implementation that is fast, thread safe and type safe. The interface exposes a very simple syntax which makes the code flow natural and easy to understand and follow, something traditional callback style programming soldom does.

However, q comes with a wide set of optional features surrounding this:

 - `async_termination`
  - Simple way of *shutting down* objects and letting the owner asynchronously wait for the object to finish.
  - Templetized input and output data types (optional).
 - `channel< T >`
  - A way of sending arbitrary data/tasks from one or more producers, received by one or more receivers.
  - Similar to Go channels.
 - `event_dispatcher`
  - Generic interface to be able to easily build bridges between q and other event loops.
 - `exception`
  - Containing arbitrary properties (objects).
  - Easily streamed to `std::iostream` (even raw `std::exception_ptr`'s).
  - Exceptions containing sets of exceptions, useful when multiple failures are collected at once, such as after `q::all( )`.
 - `expect< T >`
  - Small footprint wrapper for any data type (`T`), or an exception.
  - Useful when one wants to delay the handling of a result which might contain a value or an exception.
 - `functional`
  - Type helper to deduce return value and argument types of functions, member functions, function pointers, function objects and lambda expressions.
  - `call_with_args`, `call_with_args_by_tuple`, etc
    - Easy to use function call helpers, simplifying calling functions with variadic arguments, or arguments packed in `std::tuple`'s.
 - log framework
  - Simple API to write to logs by *streaming* data.
  - Can be integrated into most existing log mechanisms, or used as the core logging framework.
 - `memory`
  - `make_shared( )` feature similar to `std::make_shared` but allows one to have protected constructors to enforce the use of `make_shared`, without having to be *friend* with `std::make_shared`. Also allows one to override the constructor with a custom `construct( )` function which will be called instead, when doing `q::make_shared`.
 - `mutex`
  - Mutex classes very similar to the different `std::mutex` classes, but enforcing names and providing dead-lock detection (*not implemented yet*).
 - `promise`
  - What the rest of this README explains.
 - `queue`
  - A queue on which tasks are dispatched. When scheduling a completion task on a promise, one can choose on which queue to schedule the task. Queues are attached to `scheduler`s which runs on top of `event_dispatcher`s, such as the simple `blocking_dispatcher` or `threadpool`s.
 - `scope`
  - Generic object which upon destruction will destruct arbitrarily attached data of any type.
  - Provides a way of being able to return a scope from a function, yet not exposing what the scope is, but allowing the callee to hold the object to delay destruction.
 - `stacktrace`
  - Platform independent stacktrace implementation.
  - Can also be replaced by the user to allow for a custom implementation, which then will serve stacktraces to the rest of q.
 - `temporarily_copyable`
  - A way of allowing a non-copyable object to be copied, by internally doing a move-on-copy. This should be used with care!
  - Serves a great purpose as C++11 won't allow data to be moved into lambdas, only copied. q is C++11 compatible.
 - `thread`
  - Like `mutex`, this is similar to the standard's `std::thread`, but providing very useful extra features:
    - Sets thread name, platform independently, by requiring threads to have names.
    - Interfaces the `async_termination`, allowing the owner of the thread to wait for its completion and retrieving the result asychronously, scheduled with a `promise`.
  - Provides an extremely simple `run( name, fn, args... )` helper function, which just runs a function with arbitrary arguments and returns a `promise` with the returned value, which one can `.then( )` schedule completions for.
 - `threadpool`
  - What the name says, also requiring the pool to have a name. Each thread will be individually named.
 - `type_traits`
  - `bool_type< bool >` which is `std::true_type` or `std::false_type`.
  - `negate< Fn >::of< ... >` which negates meta functions which results in `std::true_type` or `std::false_type`.
  - `isnt< >`, `is_tuple< >`, `is_copyable< >`, `is_movable< >`, `is_pointer_like< >`, `tuple_arguments< >`, `tuple_unpackable_to< >`, `variadic_index_sequence`...
  - `arguments< ... >`
    - A very useful meta programming utility to handle variadic templates, a core feature to almost the entire library.
  - `fold` and `two_fold`
    - One-dimensional and two-dimensional meta fold functions, which fold arbitrary types using custom fold features.
  - logical meta operators, `logic_and`, `logic_or`, `logic_eq`, `generic_operator`...
  - `hierarchical_condition`
    - A condition which one can inject into a `fold` to fold hierarchically over sets of types, and where `std::tuple` packed types are unpacked hierarchically. This is useful when checking if all types in a tuple, and its sub-tuples satisifies arbitrary conditions, and is a necessary core feature on which q heavily depends.
    - `hierarcichally_satisifies_condition`, `hierarchically_satisfies_all_conditions`, `hierarchically_satisfies_any_condition`
      - Generic meta functions which fold over variadic types. Has some implementations:
      - `is_copy_constructible`, `is_move_constructible`, ...


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
    std::cerr << "Connection problem: " << e << std::endl;
} )
.fail( [ ]( std::exception_ptr e )
{
    std::cerr << "Unknown error" << std::endl;
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

Using threads without locks
---------------------------

Running a function on a newly created thread, "waiting" (asynchronously) for the function to complete, and then using the result in a function scheduled on another thread (or the main thread). Note that we don't need mutexes or semaphores.

```c++
q::run( "thread name", [ ]( )
{
    // Thread function which can perform heavy tasks
    return sort_strings( ); // Returns a vector of strings
} )
->terminate( ) // Will not really terminate, but rather await completion
.then( [ ]( std::vector< std::string >&& strings )
{
    // The result from the thread function is *moved* to this function
    std::cout << strings.size( ) << " strings are now sorted" << std::endl;
} );
```

Awating multiple asynchronously completed tasks
-----------------------------------------------

Lets say you have multiple (two or more) tasks which will complete promises and you want to await the result for all of them. This is done with `q::all( )` which combines the return values of the different promises and unpacks them as function arguments in the following `then( )` task.

```c++
q::promise< std::tuple< double > > a( );
q::promise< std::tuple< std::string, int > > b( );
q::promise< std::tuple< > > c( );
q::promise< std::tuple< std::vector< char > > > d( );

q::all( a( ), b( ), c( ), d( ) )
.then( [ ]( double d, std::string&& s, int i, std::vector< char >&& v )
{
    // All 4 functions succeeded, as all promises completed successfully.
} )
.fail( [ ]( std::exception_ptr e )
{
    // At least one of the functions failed, or returned a promise that failed.
    std::cerr << q::stream_exception( e ) << std::endl;
} );
```

A similar problem is having a variably sized set of *same-type* promises. Such can also be completed with `q::all( )`, although the signature for the following `then( )` task will be slightly different.

```c++
std::vector< q::promise< std::tuple< std::string, int > > > promises;

q::all( promises )
.then( [ ]( std::vector< std::tuple< std::string, int > >&& values )
{
    // Data from all promises is collected in one std::vector and *moved* to this function.
    // This means we can move it forward, and the data had never been copied.
    do_stuff( std::move( values ) );
} )
.fail( [ ]( q::combined_promise_exception< std::tuple< std::string > >&& e )
{
    // At least one promise failed.
    // The exception will contain information about which promises failed and which didn't.
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
./build.sh ; open build/q.xcodeproj
```

For Visual Studio (2015) (using Git Bash shell)
```sh
git clone git@github.com:grantila/q.git
cd q
./build.sh ; start build/q.sln
```
