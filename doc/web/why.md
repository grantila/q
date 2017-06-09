# Revolutionary C++
### > Threads for program flow is insane

Ok, so you probably write C++ code, otherwise what would you even be doing here? And since you do C++, **you need to read this. All of it.**

C++ programs often use raw threads as a means for program flow, which is borderline insane. Threads should be used solely to distribute load across multiple CPU cores. The exception is of course multiplexing I/O or UI which might have special requirements (being relaxed from CPU intensive tasks).

**&mdash; Why is it a problem to spawn a lot of threads just to handle program flow?** First of all because the likelihood of concurrency bugs is high. Secondly because it is inefficient to use a lot of unnecessary threads which requires context switching and kernel scheduling. Thirdly, maybe someone else than you will read your code, and they will not make any sense out of it. Don't take it personally, but it's true.

**&mdash; Ok then, but why are people using threads for the wrong purpose?** Because it may seem like the only way to perform concurrency (to manage asynchronous tasks). If you want to perform tasks that shouldn't block each other, such as fetch incoming data, parse it, do something more and then write it somewhere, there is no easy way to do this asynchronously. In fact, not only is there no easy way to do it, it's hard. Very hard. Especially if you want the code to be self-explanatory and maintainable.

This is exactly what q is for, and a whole bunch more. Let's say we are writing some server application and have the following functions:

```c++
std::string read_input( );
request_thing parse_data( std::string data );
std::string format_output( request_thing req );
void handle_error( const std::exception& err );
```

We first read the input and get a string of data back. *This is a very simplified example, and not entirely realistic, please accept that.* We then parse it into some `request_thing` object which we use in the application. This object can be formatted for output, and we'll probably reply the requester with that output. Lastly we have a function that takes an error and handles it (somehow). If we write a blocking implementation, it could be something like this:

```c++
try {
	format_output( parse_data( read_input( ) ) );
} catch ( const std::exception& err ) {
	handle_error( err );
}
```

I could have written an example with threads, to have some of the functionality offloaded to another thread, but that would easily add 100 more lines (with the mutexes and semaphores and everything). And most likely, you have seen enough of such code already. I certainly have.

Now, the following example is written with q and ***is valid C++***, given that the functions described above slightly differ a bit in return type. Don't worry, this seems like some weird magic at first, but you'll get it very soon. This is an asynchronous version of the blocking example above (please compare them and try to see the similarities):

```c++
read_input( ).then( parse_data ).then( format_output ).fail( handle_error );
```

The above is the only example where I'll write *promise chains* like that (yeah, a *promise chain* is what this is, but you don't need to know that yet). Since C++ doesn't care about whitespace, I'll write it like this from now on:

```c++
read_input( )
.then( parse_data )
.then( format_output )
.fail( handle_error );
```

Before we go into the technical details, just look at the above code snippet, compare it with the synchronous version above (the try-catch). Which one is simplest, shortest, easiest to read and most self-explanatory? Just think about that for a second.

As mentioned, the return types must be a little bit different, you cannot call a function "`then`" on a `std::string` which `read_input` returns, right? **But what if `read_input` returns something that will *eventually become* a `std::string`, and when this happens, *then* we call `parse_data` with this string**. That sounds pretty reasonable, right? Actually, that sentence essentially describes the whole concept of q, and what *promises* are &mdash; a means for scheduling *continuations* on future values. *Continuations* are functions that will be called when some condition is met, conceptually when we want to *continue* after that condition has been met.

*Now I hope you are ready to experience the revolution, because the next section will blow your mind...*


# Scalability, concurrency, parallelism
### > yet
### > No explicit threads, no locks and no semaphores

Traditional multi-threaded C++ code consists of a set of threads, each with a single function bound to them. Usually you put data in a variable, signal the other thread to read this variable and start working. Then you do the same thing again to read back the result. So basically, each thread does nothing but blocks and waits until it has things to do.

More often, for running parallel background tasks, your variable needs to be a *queue* of data. You also need a mutex to lock the queue. For signalling you need a semaphore between the threads, possibly two (for reading the data back). This is probably what you were being taught at school, and very likely this is how much of your C++ code looks like, and has looked like at all your work places. Oh, did I forget about errors? Yeah, you need to be able to propagate errors as result somehow...

Been there? Done that? Yes you have, you have been there and you have done that. I'm writing this because I have been there too.

Now, there are more "modern" approaches, like coupling data and logic in a `std::function` and having a queue of such entities. The other thread can then perform any job and not just a particular kind of task. This is indeed more generic and in that case a better solution, but the core of the problem is not solved. Error handling is not much easier to get right, so that errors actually reaches the original caller. Not to mention that you still need to spray mutexes all over the code. Another problem is the fact that other developers, new to the code, will not understand much about what random `std::function`'s floating around is actually doing. Tracking bugs gets a lot harder, and the program flow gets trashed. The simplicity of the source code will suffer and in fact become quite complex. This increases the risk for bugs because of misunderstandings.

So, we've all (well, many of us anyway) tried really hard to solve the problem of program flow and concurrency, and the result isn't very satisfying. Maybe it has to be like this. Perhaps this is the price you have to pay for writing C++.

No, think again.

I'm gonna assume you don't enjoy the dead locks and all the other nasty bugs caused by mistakes in synchronization. Or how the program ended up with 50 threads, without you knowing how that happened. We deserve better! After all, C++ is hard enough as it is, wouldn't you agree? Now to the good news, however, first ***you really need to prepare yourself to radically change the way you think about concurrent programming***. You need to give up the idea that you control the program flow in each line of code, and rather let a system handle the scheduling of your tasks for you, as well as the data flow between these tasks. That is the only way to set you free from the problems described above.

In the same example as above, let's say the tasks are scheduled on the same thread as the event loop we use to get the input, some network library perhaps. In other words, you probably call `read_input` from the I/O thread. The `parse_data` function may take some time though, maybe it needs to communicate with a database, or maybe it just uses a lot of CPU. You don't want CPU load to delay network logic, such as outgoing responses to requests. Now, what if we have created a thread pool? Sweet! We can offload this function to another thread. We can create the good old fashioned mutexes and semaphores and go ahead as described, with the high risk of bugs associated with that approach, to synchronize data to and from this thread. Or, we don't. Instead, let's create a `q::queue` and "attach" it to the thread pool, we call this `q::queue` "`worker_queue`". Now, what if we could say that the `parse_data` function should be scheduled on the "`worker_queue`", hence run on this thread pool, wouldn't that be totally awesome? We can, and in fact *it is exactly that easy*, no more, no less. Note that this is **correct and valid C++ code and not pseudo-code**:

```c++
read_input( )
.then( parse_data, worker_queue )
.then( format_output )
.fail( handle_error );
```

All you changed from the earlier example was to specify the queue (attached to a thread pool, event loop or whatever) on which `parse_data` should run. You added exactly this: "`, worker_queue`" and all of a sudden your program is multi-threaded and scales up to the number of cores.

**Congratulations**, you now have an application which offloads the heavy tasks to a background thread pool. You noticed all the mutexes and semaphores in this example, right? Exactly, no more of that stuff. What about the exceptions that might have been thrown from `parse_data` in the thread pool, that we now magically handle in `handle_error` on the event loop thread, *by decoupling logic and execution, **thank you very much***? If this doesn't stimulate your curiosity, I don't know what will...


# Wohoo, I've actually read this far
### > "Are you a wizard?"

**&mdash; This seems cool, although I don't get all of it just yet, I need more examples!** You really need to go ahead and read the tutorials. You'll master q very soon!

**&mdash; I get all of this, I've used similar techniques before so this is not new to me.** I'm completely certain that you haven't used anything else that combines both the multi-threaded parts as well as the type magic when passing data between continuations (basically because I think this library is unique like that). Regardless, you're a smart hacker and most likely you should check out the API. But, starting up a program from scratch with q isn't really super obvious. You need to setup event dispatchers, queues and stuff like that (it's just a few lines of code, but still). Check out at least the first couple of examples in the tutorials. And, honestly, there's a whole lot more to q you should see examples of before you dig into the API, or you risk architecting and designing something inferior. So, I recommend you to read all the tutorials, it's pretty fun reading too. That said, have a nice time with q! And yes, I'm probably a wizard...
