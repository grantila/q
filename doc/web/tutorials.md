
# The first staggering steps
### > Just a few lines, and everything is up and running

You need to ***initialize*** q before you do anything else. Also, before you exit, you should de-initialize it. That's a good habit, so that you can run static analysis and run-time analysis of your application and see that it behaves properly, and doesn't leak memory, etc.

To initialize, you can either use `q::initialize( )` and finally `q::uninitialize( )`, or only use `q::scoped_initialize( )`. The return value of `q::scoped_initialize( )` is a `q::scope` object that upon destruction will automatically de-initialize q. This is recommended. Both `q::initialize( )` and `q::scoped_initialize( )` take an optional settings object.

```c++
#include <q/lib.hpp>

int main( int argc, char** argv )
{
	q::settings settings;
#	ifdef DEBUG
		// This is very useful when you debug exceptions flowing through
		// promise chains, but it slows your program down quite a bit.
		settings.set_long_stack_support( true );
#	endif // DEBUG

	auto q_scope = q::scoped_initialize( settings );

	// ... do the rest of the q and program setup here

	// When q_scope goes out of scope, q is automatically de-initialized
}
```


# Hello world
### > The asynchronous version

To start using the promises in q, you need a few more lines of code. You need a queue on which you can *enqueue* a number of tasks, and an event dispatcher which can *dispatch* the tasks in this queue. There is a convencience helper which creates both, called `q::execution_context`. Also, please understand that `main` must not go out of scope when tasks are running. We need an event dispatcher that will block until it is explicitly terminated (when the program is shutting down).

```c++
#include <stdio>
#include <q/lib.hpp>
#include <q/promise.hpp>
#include <q/execution_context.hpp>

int main( int argc, char** argv )
{
	auto q_scope = q::scoped_initialize( ); // Configure at your wish

	auto ec = q::make_execution_context< q::blocking_dispatcher >( "main" );

	q::with( ec->queue( ) )
	.then( [ ]( )
	{
		std::cout << "Hello world" << std::endl;
	} )
	.finally( [ ec ]( )
	{
		ec->dispatcher( )->terminate( q::termination::linger );
	} );

	ec->dispatcher( )->start( ); // This will start to dispatch the tasks
}
```

What's going on above? Well, on line 12 we start a *promise chain*, which requires us to specify on which queue tasks shall be dispatched by default. As you might remember when you read the "Why" page, this can be changed at any time, but a default is always required. The return value from `q::with( queue, [ values... ] )` is a promise. Since we didn't specify any values, the return value is an "empty promise", or `q::promise< >`. Since the promise is empty, *continuations* (such as the lambda on line 13) must not have any arguments, or the code will not compile. This strict type safetyness of q is extremely powerful.

Speaking of line 13, here's where the first continuation is scheduled on the `queue` from our execution context. This is more or less the only function we intend to run. However, we want our program to exit when we're done with "hello world", and even if the "hello world"-function on line 13 (or practically 15) would fail (throw an exception), we still want to stop the blocking dispatcher, so we use the `finally` method of scheduling a continuation (more on why later). This last continuation (on line 17) terminates the blocking dispatcher (or more precisely, asks it nicely to stop when there's nothing more to do).

Now the "Aha!"; Nothing so far has really happened. We have just scheduled one task on a queue, the task on line 13. What we've also done is say that "when this task *completes*, then please schedule the next continuation", namely the task on line 17. Yet, none of these tasks will be performed unless the event dispatcher which runs the tasks on `ec->queue( )` is started.

So, on line 22, the bootstrapping will take place, and the tasks on the queue attached to the blocking event dispatcher will start to be called. By the way, congratulations, you have just created (or copy-pasted) your first real program using q!


# Promise chains
### > Passing values

I'll assume you have a queue on which you can dispatch tasks now. I'll ignore setting it up, and I leave it to you to terminate the dispatchers, etc.

Let's do the same "hello world" thing again, but a little different, namely "hello " + something. Why not also do something else after greeting the world?

```c++
q::with( queue, "world" )           // "world" is convertible to ...
.then( [ ]( std::string something ) // ... a std::string.
{
	std::cout << "Hello " << something << std::endl;
	return std::make_tuple( 5, "there" );  // tuple< int, char* > is ...
} )
.then( [ ]( int times, std::string where ) // ... auto-destructured
{
	for ( int i = 0; i < times; ++i )
		std::cout << "Is anybody " << where << "?" << std::endl;
} );
```
You see how easy and natural it is to pass data between continuations with promise chaining? The continuation on line 7 *auto destructures* the tuple of data (which is what the promise actually holds). In other words it "expands" the tuple into individual arguments. This will later prove to be very useful for functions like `all( )`...


# Shared promises
### > and multiple continuations

Promises aren't copyable, and can only have 1 continuation. This is for two reasons. First, it is faster and uses less memory, since data will be *moved* from one promise to the next. Secondly, because some data types aren't copyable, and a promise of such a type is therefore not copyable neither. The following code illustrates this (although a string is actually copyable).

```c++
auto promise1 = q::with( queue, std::string( "hello world" ) );
// This would fail compile-time due to promise1 being non-copyable:
// auto promise2 = promise1;

promise1.then( [ ]( std::string&& s ) { } );
promise1.then( [ ]( std::string&& s ) { } ); // This fails run-time
```
The run-time failure is because the data (`std::string`) in `promise1` is *moved* into the first continuation (on line 5), so the second continuation (on line 6) would move an *already moved* `std::string` into it if that would run. So in fact, the `then` call on line 6 will fail, not when it tries to run the continuation - it will fail when the `then` function is being called, synchronously in this example.

Now, it is sometimes useful to share a promise &mdash; being able to copy a promise and give it away, and also being able to schedule multiple continuations on it. The rescue is the `share( )` function returning a `q::shared_promise< ... >` which is copyable and allow multiple continuations, because they *copy* the data between each other and into each scheduled continuation. Please understand that if you use them, and share data that might be modified by multiple continuations running on different threads, you need the good old mutexes again. Only `const` could potentially help you a bit. Also understand that `share( )` invalidates the promise, just like `then( ... )` would have done, so after you have called `share( )` on a promise, you may not use it again, only the new `shared_promise`!

```c++
auto promise1 = q::with( queue, std::string( "hello world" ) );
auto promise2 = promise1.share( ); // promise 2 is a shared_promise

// This would fail run-time, since promise1 is invalidated after share( )
// promise1.then( [ ]( std::string&& s) { } );

promise2.then( [ ]( std::string s ) { } ); // Both these are
promise2.then( [ ]( std::string s ) { } ); // just fine
auto promise3 = promise2; // Also fine, a shared_promise is copyable
```


# Awaiting multiple promises
### > with auto-destructuring

Consider we await different things (promises) and we need to do something once "all" of them are done, we use `q::all( )`. This auto-destructures the values from the promises nicely, so that the continuation gets the values as arguments directly:

```c++
auto promise1 = q::with( queue, std::string( "hello world" ) );
auto promise2 = q::with( queue, 4711, my_complex );

q::all( std::move( promise1 ), std::move( promise2 ) )
.then( [ ]( std::string&& s, int i, std::complex< float > complex )
{
	// Do something useful here;
} );
```

Note `std::move` here. As described above, promises aren't copyable, so we must *move* the two promises into `q::all`, and get a new promise back. Also important to know is which the default queue of the returned promise will have:

> `q::all` returns a promise with a default queue being the same as the first promise given to to `q::all`, `promise1` in the example above. If you want to be explicit about this, create a new temporary *empty* promise with an explicit queue, and append the rest of the promises after, like `q::all( q::with( queue ), ... )`.

The above example shows one kind of `q::all( )` usage, when you have a certain amount of differently-typed values you await. The other way is to use *containers* of same-type promises. In this case, the return value is a new container but to `q::expect` instead of `q::promise`. A `q::expect< T >` wraps a value T, similar to a promise, but is guaranteed to either have the value T or an exception, at all times. It's not a future value, but something you can read synchronously.

```c++
// This is basically a list of promises of floats
std::vector< q::promise< float > > list;

q:all( list, queue )
.then( [ ]( std::vector< q::expect< float > >&& values )
{
	// Now all promises have settled (i.e. are not waiting anymore),
	// but some of them might have failed.
	for ( auto& val : values )
	{
		if ( val.has_exception( ) )
			std::cout << "Failure: " << val.exception( ) << std::endl;
		else
			std::cout << "Value: " << val.get( ) << std::endl;
	}
} );
```


# Reflection
### > Good for inspection

Sometimes, perhaps most often when having an array of promises, it can be useful to not have a promise of type `T`, but of a `q::expect< T >`. This because the `expect` cannot "fail". Instead, you can inspect each expect-wrapped value to see if there actually is a value, or exception.

The way to do this is to call `reflect_tuple` on a `promise< T >`, which returns a new promise of the type `promise< expect< T > >` (or if the upstream promise had multiple template types, the result will be `promise< expect< std::tuple< T... > > >`).

```c++
// Given a q::promise< float >

promise.reflect( ) // -> q::promise< expect< float > >
.then( [ ]( q::expect< float >&& e )
{
	e.has_exception( ); // -> bool
	// ... do something useful here
} );
```


# Making a promise
### > and keeping it...

So far, all you've seen are examples where we already have promises, but where do they come from? Who creates them? Well, most applications have some kind of I/O, where the I (input) triggers code, be it user input (keyboard, mouse), network (new connection or new data to read), or sensors, timers, triggers, watchers... And usually there's an output, one way or another.

In these cases of events (or inputs), you want to start a promise chain - you want actions to happen because of this event. One way is to create promise with `q::with( )` as already shown. This way you get a promise which is already resolved (unless the input to `with` is a promise itself of course). Another is by using `q::make_promise( )`, which takes a function and calls it with two functions as arguments, `resolve` and `reject`. Well, they're not really functions, but that doesn't matter. They are copyable, and that is something you will find very handy. Anyway, by using this method you can resolve a promise given arbitrary reasons, and if your function throws an exception, it is forwarded to the returned promise, making this method both synchronously and asynchronously safe.

Consider a database client library. It probably has a `q::queue` internally which it is supposed to schedule tasks on. It also has some network connection to the database server. Let's say we can send data on the connection, and register a callback for when a certain response arrives. Let's say this function is called `on_result`. To you, this is where the input (or event) starts, so the `on_result` function will be where you want to create the promise chain. Note though, that we want *any error we might make* in the `query` function, to be propagated back to the user *in the promise*, and not thrown synchronously. So we let `make_promise` embody the entire function. This is a good (maybe even *best*) practice.

```c++
q::promise< database::result >
database::query( std::string query )
{
	return q::make_promise( this->queue, [ this, query ](
		q::resolver< database::result > resolve,
		q::rejecter< database::result > reject
	)
	{
		// Internal representation of the query
		auto processed_query = this->preprocess_query( query );
		// The unique ID of the query
		auto id = processed_query.get_id( );

		// When we get a response, we resolve this promise
		this->on_result( id, [ this, resolve, reject ]( std::string response )
		{
			// Parse the response
			try
			{
				auto parsed_response = this->parse_response( response );
				if ( parsed_response.ok( ) )
					resolve( parsed_response.get_result( ) );
				else
					reject( std::make_exception_ptr(
						parsed_response.get_error( ) ) );
			}
			catch ( err )
			{
				reject( std::current_exception( ) );
			}
		} );

		// Send the query to the server
		this->send( processed_query );
	} );
}
```

Writing generic libraries (database client drivers, or any other) is not suitable for inexperienced programmers, other than to learn. If you feel the above is too tricky to grasp, perhaps you just want to use that database driver and write some nice business logic? This is maybe how your code would look:

```c++
q::promise< > user_context::save_login( )
{
	std::stringstream query;
	query
		<< "INSERT INTO user_logins VALUES ("
		<< this->get_id( ) << ", "
		<< this->login_time( ) << ", "
		<< this->ip( )
		<< ")";

	return this->db->query( query.str( ) )
	// Remove the database result from the output by converting:
	// q::promise< database::result > into q::promise< >
	// Errors are still forwarded
	.then( [ ]( database::result&& result ) { } );
}
```

If you're using C++14, you have an option to use `q::make_promise_of< T >` instead. The function you give, is allowed to have `auto` as data types for the `resolve` and `reject` functions:

```c++
auto promise = q::make_promise_of< int >(
	queue, // This will be the default queue in the returned promise
	[ ]( auto resolve, auto reject )
	{
		// Either we
		resolve( 47 );
		// or we
		reject( std::make_exception_ptr( MyException( ) ) );
	}
);
```


# Chains and queues
### > which queue?

Consider the functions `f` and `g` returning promises in this code snippet. It looks so simple and stupid you could probably just skip this example. Don't. Just don't. *We're going deep and hard core now...*

```c++
f( )
.then( [ ]( )
{
	return g( );
} )
.then( [ ]( )
{
	return h( );
} );
```
As you may realize, this is the same as

```c++
f( )
.then( g )
.then( h );
```
Now, what will happen here, and on which queue will `h` be scheduled?

First of all, the promise returned by `f` will be awaited. When it is resolved (to void in this case), the function `g` will be called. Note that `g`'s *returned promise will be awaited* before we call h. This may seem entirely obvious. However, you don't know by this example, what the *default queue* is in the promise returned by `g`. It may not be the same as the one returned by `f`. The promise on which we schedule `h` (the promise starting at line 3 in the short code snippet (returned by `f( ).then( g )`) *does have* a default queue, all promises do. The question is which one. Which queue is it?!

Is it the same as the one in the promise returned by `f` or the same as the one in the promise returned by `g`? Or, is it undefined at compile time and can be either one at run-time? Please think for a while and make a good guess. Spoiler alert! Also, maybe what you guess and what you hope for is not the same... So guess again ;)

It's going to be very important for you to know this, and get it right. We're essentially talking about on which thread tasks will run!

The answer is that the queue will be the default queue in the promise returned by `f`. Now lets discuss why, because honestly, any of the other possibilities could somehow make sense.

Your code will likely consist of these chains, i.e. a lot of "then" calls after each other. To begin with, it is *very good* that you can *know* on which queue a certain task runs, so leaving this undefined just because a situation is a bit complex, is unsatisfying and dangerous. Also, let's say you're analyzing some code, more explicitly, a certain promise chain. If you know the queue in the first promise in that code, you should be able to know the rest of the queues as well - you should know how tasks are scheduled in that piece of code! Just because some code somewhere else will run tasks on other threads, that shouldn't affect the promise chain you're looking at and working on. If that was the case, you'd have to follow all function calls everywhere to know what's going on, or practically know your entire code base line by line in your head. Now, you don't, you know this by looking at one single promise chain, regardless of what each task in that chain does internally.

A good example is the silly database code above. You probably didn't think about it, but the `user_context::save_login( )` function (in an example above) returned a promise with the default queue being the one in the database class. Consider we called this function from somewhere where we do CPU intensive stuff. We might have some background jobs being performed every now and then (like every second or minute or whatever). Imagine this:

```c++
void garbage_collector::collect( )
{
	q::with( this->background_queue )
	.then( [ this ]( )
	{
		return this->cleanup_memory( );
	} )
	.then( [ this ]( )
	{
		return this->new_users;
	} )
	.map( [ ]( user_context_ptr user )
	{
		return user->save_login( );
	} )
	.then( [ this ]( )
	{
		return this->do_cpu_intensive_stuff( );
	} )
	.fail( [ ]( std::exception_ptr e )
	{
		std::cerr << "collect: " << q::stream_exception( e ) << std::endl;
	} );
}
```

Cool collect function, right? Maybe not, but it illustrates different things being done in a sequence. In this case cleaning up memory, and then for each new user it's saving some stuff to a database (*although you don't really know that it's a database by looking at this code*) and then doing something really cpu intensive. The `map` is pretty neat, isn't it? The `stream_exception` is quite useful as well.

We *do not want* to leave promises floating around without checking for errors. We need to end *every* promise chain with a `fail` handler. This doesn't mean that we must do it in every function, since they can return promises and pass the potential errors back, but at every *end of the chain* we must check for errors. And, we might not want to do all the above things at once, so doing them one after another can make sense (again, this is an example). Ok, so we agree that the code above is more or less reasonable. Now let's get back to the issue of queues.

What if the *default queue* to the task scheduled on line 16 would be the one from any of the `save_login( )` functions? Then `do_cpu_intensive_stuff` would run on the database queue/thread. WAT? Exactly. That is *almost certainaly* not what we would have wanted - to, by mistake, run some random heavy garbage collect function on the database thread.

> This is the reason **inner promises are awaited, but their default queues are forgotten**. You can think of it this way; We don't care on what queue tasks were run in *inner functions*. We just await the (asynchronous) value from the function.

This also means that the following code:

```c++
f( )
.then( [ ]( )
{
	return g( );
} )
.then( [ ]( )
{
	return h( );
} );
```

is not the same as:

```c++
f( )
.then( [ ]( )
{
	return g( )
	.then( [ ]( )
	{
		return h( );
	} );
} );
```

They both seem to do the same, and *in most cases they probably will*. But, the queue on which `h` is scheduled *can potentially be different*, which can affect your program a lot. It can actually crash it if you somehow expect `h` to run on the same thread as `f`, perhaps by sharing state without locks.

In the first example, there is basically only one chain, and it begins with `f`. Actually `g` and `h` are chains too, but nothing is *directly* scheduled after them, they are returned and *merged* into the `f` chain. In the second example, `h` is scheduled directly on the `g` chain. Then, the `g` chain is awaited by the `f` chain, which is the return value of the whole expression. You **must** get this. You must also realize that in terms of *data* flowing between these functions, the lambda that calls `h( )` doesn't care whether the first or second example is used. It'll always get the data from `g`. The difference is only on what queue the `h`-lambda will be scheduled.

If you want the same bevaviour as the first example, but with code like the second, for whatever reason (I'm not the one to question your code), you can do it this way:

```c++
auto promise = f( ); // Save the promise from f, even though it gets ...
promise.then( [ ]( ) // ... invalidated here, we can still get its queue ...
{
	return g( )
	.then( [ ]( )
	{
		return h( );
	}, promise.get_queue( ) ); // ... and apply the queue here
} );
```

q is wonderfully easy to use, and allows you to do so much so effectively, but if you don't get this right, you're in for a debug session... In other words:

> You don't need to think very much when using q, it will fail at compile time for almost any mistake you make, but take an extra look at the *queues* you're using when things become complex.

### Temporarily or permanently change queue

As you have seen, you can specify queues explicitly to `then` (actually, to `fail` too) as a second argument. What exactly does that mean? Please don't dismiss this as too simple, read on. This too, is something you *must* understand. Consider:

```c++
q::with( queue_x )
.then( a )
.then( b, queue_y )
.then( c )
```

You most likely understand what this does by now, at least, more or less. Three functions are being scheduled, `a`, `b` and `c`, and data may flow between them. But what about the queues?

`a` will be scheduled on `queue_x`, I hope that was clear to you too. `b` will be scheduled on... uuh... wait for it... `queue_y`! Phew, you got that right too I hope. That's exactly how you bypass the default queue in the chain, and say *"no, I want **this** queue instead!*". Maybe you want this to run on a "background" thread pool, because it is a heavy operation, or on an explicit thread pool with only 1 thread because you want to access shared data without using locks (mutexes).

Now, what about `c`?

I'm not gonna painfully delay this, so; *it will be scheduled on* `queue_x`. There. Like with the decision about *merging promise chains* above, this needs a good default too, reflecting what you *most certainly* expect. The principle of least surprise is a very good principle. I can only hope you didn't get too surprised here. Anyway, once you start using q a lot, you'll realize that this default is what you want, probably every time. You want one particular task to be scheduled differently than all the other. You want the beginning of your promise chain to use the same queue as in its end. Most likely every time. Maybe it's not obvious to you now, but it will be.

However, you *can* change the default queue in a chain, at any given time, if you so wish. You do this by wrapping the queue inside `q::set_detault( )`. This will also cause your code to be obvious to other developers, even if they never heard of `set_default`, they probably understand what it does:

```c++
q::with( queue_x )
.then( a )
.then( b, q::set_default( queue_y ) )
.then( c ) // b, c, and all subsequent "then"-calls, will be on queue_y
```

Now you know practically everything you need to know about promise chains and queues, and given this is the core of q, you actually completed a kind of milestone. Congrats, really.


# Exceptions in chains
### > try-catch ⟶ then-fail

You've seen the `fail( )` function be used in previous examples, and you probably understand what they're used for. You can see `then` and `fail` as asynchronous versions of `try` and `catch`. Combined with queues and their backing on threads, this means that exceptions thrown in functions on one thread, can be caught in functions on another thread. This becomes completely transparent to the developer and shows how powerful the decoupling of logic and execution is.

With `try`, you can `catch` specific errors given their class, or catch everything using `...`. In q, `fail` is implemented to mimic this just as easily.

The following synchronous code:

```c++
try { do_something_that_may_throw( ); }
catch ( MyError& err ) { /* ... */ }
catch ( ... ) { /* ... */ }
```

can be translated to the asynchronous version:

```c++
q::with( queue )
.then( do_something_that_may_throw )
.fail( [ ]( MyError& err ) { /* ... */ } )
.fail( [ ]( std::exception_ptr err ) { /* ... */ } )
```

They are, as you see, practically identical. The `...` is replaced with `std::exception_ptr`, but other than that, it's hard to find differences. Even the fact that a particular catch handler (for `MyError` e.g.) can actually *rethrow* is the same. The rethrown exception will cause the next `catch`/`fail` handler to catch it, both in traditional try-catch (you should know this by the way), as well as in q's then-fail.

So far, it may seem trivial to use `fail`, but you'll eventually end up in situations where your code won't compile, and you can't figure out why. Likely, this is because of the return value of `fail` which **must** match the type of the previous promise.

First of all, we need to understand what happens when code `throw`s an exception. Normal code flow is bypassed, which asynchronously means that functions scheduled with `then` will not be run. Instead, q will look for the first `fail` handler which matches the exception type (just like with try-catch). But, what if there was no exception? This is what you must keep in mind. Consider:

```c++
foo( )
.then( bar )
.fail( handle_error )
.then( finish )
```

This is *more or less* equivalent to the synchronous version:

```c++
MyType val;
try
{
	val = bar( foo( ) );
}
catch ( ... )
{
	val = handle_error( std::current_exception( ) )
}
finish( val );
```

"Uh, what?" you might think. Where did that `MyType` come from, and why would the value from `bar` be forwarded into `finish`?

Well, look at the asynchronous version above (*isn't it just a whole lot easier to read than the synchronous?*) Anyway, what if there was no exception? Then the code flow will technically become:

```c++
foo( )
.then( bar )
// .fail( handle_error )
.then( finish )
```

and given what you've read so far, you can **expect** values from `foo` to be forwarded to `bar`, just like the value returned by `bar` will be forwarded to `finish`, right? I mean, that's what q is very much about! If it's still not obvious, read these examples again, and let it sink in. You need to understand this.

So fundamentally, when you have a `fail` handler scheduled on a promise of a type (and not just void), you need to think of the two cases - one where there's a value and one where there's an exception. So;

> Your `fail` handler **must** return the same data type as the promise it is scheduled on, since the returned promise after `fail` must be able to forward values from *before fail*, if such existed.

This is really not weird, or even *bad*. In fact, once you accept this, you'll realize that this forces you to *analyze and decide* what part of your code has the *responsibility* (perhaps because of its *ownership*) to handle errors.

It also means that you can write pretty straight forward retry-logic with recursion, just as an example.

If you didn't get it already, I can tell you what you otherwise would conclude after having used q for a while; you will want to end your chains with a then-function which returns void (because, there is nowhere to return any value to). It is after that final `then`, you want to check for errors with `fail`. At least this is how you *most often* want it to be.

### Finally

Now, sometimes you want to inspect a promise chain, or ensure code to clean up, log, or in any other way run regardless of whether there was a failure or not. For this, we use `finally`. It will always be scheduled. It can return a `promise` of void, just to allow asynchronicity, and this promise will be awaited, and can potentially contain an exception.

```c++
foo( )
.finally( [ ]( ){ /* ... */ } )
.then( bar )
```

There is another nice function besides `then`, `fail` and `finally`, which is `tap`, described later. All of these, together with `reflect`, form the complete code flow handling of promises. You're almost becoming a guru by just reading this!


# Thread pools
### > because parallelism!

Enough of promise chains for a while. If you're new to these kinds of asynchronous program flows, it might be time for a break and talk about something easy and straight forward. Thread pools.

When you need to parallelise your code, because of CPU intensive tasks, or because you want to keep a thread as little used as possible (a UI thread or I/O thread), you need an extra thread, or a pool of them. In q, you'll always create a `q::threadpool`, even if you only need 1 thread, since the `threadpool` is an `event_dispatcher` on which we can attach queues through a scheduler. *Uuuh, what?* Well, just accept that sentence for now, and read on.

Now, thread pools, just like threads and mutexes in q, require names. This significantly helps debugging programs, and causes no noticable (and practically not even measurable) slowdown.

Before I continue talking about thread pools, let me just tell you that you probably don't want to create one, just like that. You most likely want to create an `execution_context` instead, but ok, this is how you create a thread pool with 2 threads (default is `q::hard_cores( )`):

```c++
auto tp = q::make_shared< q::threadpool >( "my pool", queue, 2 );
```

As you see, you need a `queue`. Why? Well, this is no ordinary thread pool. It follows the `async_termination` interface, described below. This means that you can asynchronously await the termination of this thread pool, but on which queue shall those continuations be scheduled and run? It can't be a thread from the thread pool, when it is deleted... You guessed right, that's why you need to specify *another* queue for this, perhaps some *main queue* or whatever you'll call it.

You can notify this threadpool there's more work to do (`notify( )`) and register a function which can fetch more work (`set_task_fetcher( )`).

Now, don't do any of the above. Use an `execution_context` context instead.

### execution_context

The `execution_context` sets up the `event_dispatcher` (`threadpool` in this case), a `queue` and a `scheduler`. They are easy to create, you just specify which type of `event_dispatcher` and `scheduler` you want. The arguments to `make_execution_context` is the same as to the underlying `event_dispatcher`, so for a `threadpool` this means:

```c++
auto ec = q::make_execution_context< q::threadpool, q::direct_scheduler >(
	"main background pool", queue, 2 );
```

Now, when you've created this execution context, you want a queue on which you can dispatch tasks which will be run on the *thread pool*, and not the one you gave the constructor. You get this with the `queue( )` function:

```c++
main_queue; // Let's assume we have a queue on which termination
            // continuations will be scheduled.
auto ec = q::make_execution_context< q::threadpool, q::direct_scheduler >(
	"main background pool", main_queue, 2 );
auto tp_queue = ec->queue( ); // This queue is the one to use to run tasks
                              // on the thread pool.
```

Yes, it's actually *that easy*, which is why this chapter is done.


# Multiple queues
### > Queue priorities

For very complex situations, and only when you really know what you're doing, you might want to allow multiple queues to be attached to an `event_dispatcher`, and maybe letting these queues have different priorities so that tasks on one queue will be dispatched before tasks in the lower priority queues. This is possible in q, by using the `priority_scheduler` instead of the *overall preferred* `direct_scheduler` (which only allows one queue).

If you attach multiple queues with identical priority, the `priority_scheduler` will round-robin between them for fairness and to try to prevent starvation. For queues with different priority, please understand that tasks in a queue will not be dispatched (and removed from the queue) until higher-priority queues on the same scheduler are empty. **There is no fuzziness.**

Also, the `direct_scheduler` is more optimized and is very fast (it's more or less a no-op). The `priority_scheduler` is quite fast too, be is much more complex, so for very high amounts of (small) tasks per second (tenths of thousands), this may be noticable.

The `execution_context` will automatically create a queue with priority zero, but you can add more queues to the `priority_scheduler` (you cannot do this to a `direct_scheduler`, that will throw an exception). The priority is any integer.

```c++
auto ec = q::make_execution_context< q::threadpool, q::priority_scheduler >(
	"main background pool", main_queue );

auto zero_prio_queue = ec->queue( ); // Default-created 0-priority queue

// Create a higher-prio queue and add it to the priority scheduler
auto high_prio_queue = q::make_shared< q::queue >( 1 );
ec->scheduler( )->add_queue( high_prio_queue );

// You can now use both zero_prio_queue and high_prio_queue
```

Please, allow me to stress the importance of thinking twice about using the priority scheduler. You, as the developer of your program, know your business logic. You know best how to deal with prioritizing tasks in your application, so you can probably do that yourself a lot better than blindly relying on q to do its best. This doesn't make the priority scheduler worthless, it is highly useful in some cases, but don't just use one because *it seems like a good idea*, use it because you have analyzed your program flow and come to the conclusion that for a certain kind of tasks, this is the right thing to do. Do it when you have thought about fairness and starvation and how that can affect your program, e.g. during high load or high amounts of inputs/events. If you don't know what starvation is, **use the default (direct) scheduler**.


# Locks
### > When you really have to

When you really need to lock, you need a mutex. C++ have these built-in, `std::mutex`. They do what they're supposed to do, but they're dangerous if used sloppily, e.g. they can deadlock. Well, the mutex isn't dangerous, you are, as a programmer, for you are the reason behind the deadlock, if it occurs. Anyway, for this reason, you should consider replacing your `std::mutex` usage into `q::mutex` which has deadlock-detection and nice helpers. They also force you to *name* your mutexes which will help when a deadlock occurs. q will then look at your mutexes and tell you how the deadlock occurred. If *long stack support* is enabled in q, the whole call/promise chains leading to the deadlock will be logged too, making it dead simple to debug and fix. Traditionally, the simpleness of fixing, is not something deadlocks are very famous for...

The deadlock detection only applies to lock graphs with only `q::mutex`es. A deadlock can still occur if another mutex (e.g. `std::mutex`) is part of the deadlock.


# Logging
### > Cutting down promise trees

q has support for tapping into a promise chain without modifying it. You already know this from `finally`, although in that case, the values in the promise are not reachable inside the finally-handler. Using `tap`, you get the value as a const reference, and you are expected to only read these values synchronously. If tap performs anything asynchronous, it has to *copy* the data first. This is because after the tap handler is called, the values will be *moved* into the next `then` handler (or similar function).

Contrary to `finally`, `tap` will only be scheduled when there's a value, not an exception.

```c++
q::with( queue, std::string( "hello world" ) )
.tap( [ ]( const std::string& s )
{
	std::cout << "Logging string: " << s << std::endl;
	// This function must not return anything, not even a promise
} )
.then( [ ]( std::string&& s )
{
	// Doing something else with s ...
} )
```

# Asynchronous termination
### > When you're eventually shutting down

You've seen it in previous examples - the `async_termination` thing. Let's first argue why we need it.

A program architected as modules with explicit (or implicit) dependencies between each other, that must be able to shut down cleanly, will also need means to make these module have some kind of termination. This means that each module must be able to be *terminated*. Terminated here means stopped from doing anything, perhaps cleaning up inner data, and *put in a state where they can be destructed/deallocated*.

If any of these modules have asynchronous ongoing tasks (perhaps the module has an inner thread, although this is not at all necessary for it to have ongoing tasks), then this module's termination will have to be asynchronous, meaning that the code that terminated it, will need to asynchronously await the termination to complete before it is allowed to destruct the module (or remove its reference to it, such as a `shared_ptr`).

Also, if this module is being owned by another module, the latter must support asynchronous termination too! We quickly end up with a lot of modules which cannot easily (synchronously) be terminated, but which need asynchronous termination. This is why q provides a generic interface for this.

### Ownership

If module A owns module B, module B's asynchronous termination must finish (complete) the termination on a queue available after its destruction, especially it needs to be available to A.

In fact, when A created B, it told B on which queue termination continuations should be scheduled. You might remember this from the `execution_context` examples above. It doesn't technically have to be exactly like this, but one way or another, A must be certain about B's termination if it is supposed to own it. Otherwise it risks deleting B before B has terminated (unloaded).

### The interface

The `async_termination` interface allows a class to be asynchronously terminated. The class extending `async_termination` defines which argument(s) the `terminate` function should require (this can be used to provide different kind of termination), as well as what the potential *return value* from the termination should be.

The flow is like this; Someone *terminates* an instance of a class using the `terminate( Args... )` function. This will eventually trigger a `do_terminate( Args... )` to be called. This is a function the class must override from the `async_termination` interface. When the class is done with the termination, it calls the `termination_done( Ret )` function. This can actually be called even before the `do_terminate` has been triggered.

Let's say we're writing a class which should represent a process (an operating system process). We should be able to terminate this process with a signal (an `int`), and the process can exit with different *exit codes* (also an `int`). Users can't just delete the `process` object, since that could leave zombie processes! No, we must respect the `async_termination`.

```c++
class process
: public q::async_termination<
	q:arguments< int >, // Arguments to terminate()
	int,  // Return value from terminate() (as a promise!)
>
{
	// Constructors and functions and whatever stuff here...
private:
	void do_terminate( int signal ) override
	{
		// Do something useful here, such as:
		// kill( pid, signal );
		// wait for exit code, and call the variable "exit_code"

		// Later when the process is exited, we do:
		termination_done( exit_code );
	}
}
```

Any user of this class, will use it like this:

```c++
auto p = std::make_shared< process >( "my_program" );
// Then, eventually terminate it:
p->terminate( 15 ) // Uses SIGTERM just as an example
.then( [ ]( int exit_code )
{
	std::cout
		<< "Process 'my_program' exited with code "
		<< exit_code << std::endl;
	// The instance of "process" here, p, is now cleanly terminated
	// (regardless of the exit_code), so we can now release our reference
	// to it, and allow it to be destructed/freed.
} )
```

For example, the `threadpool` in q isn't only an `event_dispatcher`, it is also an `async_termination` with `q::termination` enum as argument, and `void` as return. This means we can terminate it, with the option to specify how it should deal with the backlog of tasks, and when we await it, we'll just get `void` back, when the threadpool is terminated and ready to be freed.

> Please use this interface anywhere in your code to provide a generic means of termination. You'll find it quite useful and your classes will have an elegant and robust way to be terminated.

But of course, and as already said, this is most useful when you do have asynchronous tasks which need to be awaited on termination. Many of your classes will not need this. But what if you just love this interface so much you want to use it for other things as well? Actually, `sync_termination`(!) exists, and is not just for fun. It is being used in `sync_event_dispatcher` (contrary to `async_termination` which is used in the `event_dispatcher`). The `blocking_dispatcher` uses it, meaning it has a `terminate` function, which returns a value synchronously (blocking), rather than a promise.

Or you just use `async_termination` everywhere in your code because it's nice to have the same kind of interface everywhere.


# Unit tests with q-test
### > Eventual expectations

Sooner or later, you're going to write unit tests for code which uses promises. If you're expecting the promise returned by a function to eventually be resolved to a certain value, there are a lot of things you'll need to prepare.

You need to ensure your test doesn't return before the promises are all awaited and resolved. You need to handle promise rejections (asynchronous exceptions). You might want to ensure that a function is actually being called (such as a `.then` callback).

### Welcome q-test, with fixtures and spies

`q-test` is a library which extends Google's unit test framework `gtest`. You can use all the features in `gtest`, but with `q-test`, you get asynchrony support.

To create a unit test with `q-test`, you need to instanciate a `q-test` fixture, using the `Q_TEST_MAKE_SCOPE` macro:

```c++
#include <q-test/expect.hpp>

Q_TEST_MAKE_SCOPE( StringStuff );
```

This fixture contains all you need to get up and running with testing promises. Unit tests on this fixture will have access to a default queue called `queue`, a _"background queue"_ called `bg_queue`. There's also a spy which is used to ensure functions are called. More on this below.

The last thing you do in the unit test, is to call the function `run( )` and give it a promise which needs to be awaited in the test.

Consider the following example:

```c++
#include <q-test/expect.hpp>

Q_TEST_MAKE_SCOPE( StringStuff );

TEST_F( StringStuff, CountSpaces )
{
	// We expect count_spaces returns a promise which will eventually contain 2
	auto p = my_lib::count_spaces( "hello there world" );

	auto p2 = p.then( [ ]( int i )
	{
		EXPECT_EQ( i, 2 ); // This is a normal gtest expectation
	} );

	run( p2 );
}
```

The above test is a correct (but not optimal) usage of `q-test`. We await `p`, expect it to resolve (if it is rejected, the test will fail, `q-test` handles this for us). When we do get the value, we use `gtest` to test the correctness of `count_spaces`.

Lastly, we await `p2`. This will block the execution at that point, until `p2` either resolves or rejects.

### Ensure functions are called

Now, in more complex scenarios, we might want to ensure a callback to `.then` is actually called. We do it this way:

```c++
TEST_F( StringStuff, CountSpaces )
{
	// We expect count_spaces returns a promise which will eventually contain 2
	auto p = my_lib::count_spaces( "hello there world" )
	.then( EXPECT_CALL_WRAPPER( [ ]( int i )
	{
		EXPECT_EQ( i, 2 ); // This is a normal gtest expectation
	} ) );

	run( p2 );
}
```

What happens above, is that the `EXPECT_CALL_WRAPPER` macro is generating a function which has the same signature as the function provided (in this case taking an `int` and returning `void`). It encloses and invokes this function, but also ensure itself to be called.

There's also `EXPECT_NO_CALL_WRAPPER( fn )` which expects the function *not* to be called, and `EXPECT_N_CALLS_WRAPPER( n, fn )` which expect the function to be called exactly `n` times.

### Generated test functions

There are three more interesting macros that can be used, `EXPECT_CALL( ret, args... )( ret-val )`, `EXPECT_NO_CALL( ret, args... )( ret-val )` and `EXPECT_N_CALLS( ret, args... )( ret-val )` which actually create functions returning the value `ret-val`.

```c++
	// These two:
	EXPECT_NO_CALL( void, int )( )            // 1
	EXPECT_NO_CALL( int, std::string )( 314 ) // 2

	// creates the following lambdas:
	[ ]( int ) -> void { }                    // 1
	[ ]( std::string ) -> int { return 314; } // 2
	// although the auto-created ones above will contain
	// spies to ensure the functions are not called!
```

This is useful if you just want to ensure that a certain `then` or `fail` isn't called. It's especially useful to ensure a certain error is (or _is not_) called:

```c++
TEST_F( StringStuff, CountSpaces )
{
	// Now we expect that a specific error is thrown
	auto p = my_lib::count_spaces( "hello there world" )
	.then( EXPECT_NO_CALL( int, int )( 0 ) )
	.fail( EXPECT_NO_CALL( int, const StringTooLarge& )( 0 ) )
	.fail( EXPECT_NO_CALL( int, const InvalidString& )( 0 ) )
	.fail( EXPECT_CALL( int, const TooShortString& )( 0 ) )
	// We must return an empty promise to run( )
	.then( [ ]( int ) { } );

	run( p2 );
}
```

### Super-simplifications

Now that you know the above, you can build great unit tests, and therefore you deserve to know about the following magic macros, simplifying things even further:

```c++
TEST_F( Fix, Fun )
{
	// A shared_promise which will resolve to 6
	auto p = q::with( queue, 6 ).share( );

	auto err = std::logic_error( "err" );
	// A shared_promise which looks like it resolves to an int,
	// but will throw a std::logic_error.
	auto pe = q::reject< q::arguments< int > >( queue, err ).share( );

	EVENTUALLY_EXPECT_EQ( p, p );
	EVENTUALLY_EXPECT_EQ( p, 6 );
	EVENTUALLY_EXPECT_NE( p, 7 );
	EVENTUALLY_EXPECT_GT( p, 5 );
	EVENTUALLY_EXPECT_LT( p, 7 );

	// We expect this to be resolved - not rejected
	EVENTUALLY_EXPECT_RESOLUTION( p );

	// We expect pe to be rejected
	EVENTUALLY_EXPECT_REJECTION( pe );

	// We expect pe to be rejected with a std::logic_error
	EVENTUALLY_EXPECT_REJECTION_WITH( pe, std::logic_error );

	// Expect a promise to be shared or unique
	EVENTUALLY_EXPECT_SHARED( q::with( queue, 6 ).share( ) );
	EVENTUALLY_EXPECT_UNIQUE( q::with( queue, 6 ) );
}
```

The `EVENTUALLY_EXPECT_*` macros ensure that the promises are properly awaited. You can think of them as magic asynchronous versions to gtest's `EXPECT_*` macros. They'll even output just as useful error messages if they fail.


<<EOF

New logic:
 * make_promise and make_promise_of should enqueue the user callback on the provided queue not *only* return a promise with that queue as default.
 * make_promise_sync and make_promise_of_sync should call the user callback synchronously (directly) and not on the provided queue.


# Thread safety between modules

Maybe you didn't realize it in the example with a database function called `database::query( )` above, but it wasn't particularly thread safe. If you did realize it, you're certainly very very good!

Anyway, if any part of the source code can run this function, from any thread, this function must apply locks when it is touching shared state (members in the database object, `this`). One way to ensure this is to use a locking mechanism, like a mutex. But, wasn't q supposed to fix this, and give you the possibility to do things thread safe without locks?

### Must-know about make_promise()

What you must know about `q::make_promise( )`, and this is going to be crucial for you, is that the function you give it (which resolves or rejects the returned promise), **will be run synchronously**. This is so that you *can* reach state in the same scope, although of course you need to be careful when doing so.

`q::make_promise( )` vs `q::make_promise_on_queue( )`.

Instead, one way to ensure thread safety is to wrap all public functions (that could be called from other threads) with a `q::make_promise`

# Channels
### > Asynchronous communication with vows
 * temporary, expect, scope, functional
 * temporary, expect, scope
