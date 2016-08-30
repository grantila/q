
# Roadmap

## Things for 1.0

### Known bugs

 * A second .then on a normal promise (not shared) should synchronously throw an exception (or log this error somewhere decent), so this error isn't ignored. This because this *can never* be a "kind of ok" thing, but is 100% a logic bug.
   * Alternatively it could automatically create a new rejected promise with a special exception.

### Other

 * `static_assert`'s. More of them where applicable, to help developers understand what they do wrong.

### Features

 * Stacktrace on all platforms
 * hard_cores(), soft_cores(), processors() on all platforms
 * Tap
 * Maybe:
   * then-functions getting q::expect< T > or q::expect< tuple< T... > > by automatically using reflect()
   * long-stack-support
   * deadlock detection
   * logging
   * lock-on-queue

## Post 1.0

### Features

 * Cancellation (upstream and downstream). Care must be taken for thread safety (especially in terms of race conditions).
 * Auto-detection of uncaught exceptions (or even *possibly* uncaught exceptions).
 * Auto-detection of make_promise where resolver/rejecter never gets called (internal shared_ptr gets freed).
   * Should internally reject the promise with a special error.
 * Main interface change (needs consideration): promise< tuple< T... > > --> promise< T... >
 * Disallow references of any kind in promises (l-value and r-value). Allow l-value references through std::ref() only. This is because references *very often* cause issues and are mistakes. This means we only allow forwarding through copy or move, not by reference.

### Optimizations (together these will do a massive performance improvement)

 * Lock-free queues, at least when only having one queue on an event_dispatcher. Likely a small benefit, at least for non-x86 archs.
 * Inline state inside promises, for data less than n bytes (e.g. 16, 32 or 64), rather than dynamically allocated.
   Movability, copyability needs to be considered for unique/shared promises. Likely a huge improvement.
   Can be unioned with the exception.
   * For shared promises, this is not necessarily an improvement, and even for unique, it adds complexity since the code which eventually resolves the promise, must then point to the promise instead of the shared state. This means it needs to be updated whenever the promise is moved (before it is resolved), and this must not introduce race issues.
 * Short-cutting then-chains for subsequent same-queue already-resolved tasks, rather than putting them on the queue again. Likely a huge overall improvement. This means out-of-order fetching of jobs.
 * Allow thread pool to fetch multiple jobs at once, to minimize the cross-CPU synchronization in cause of multi-CPU computer. This could maybe be detected or configured.

### Possible optimizations

 * For chained class-matching .fail's ( fail(a).fail(b).fail(c) ), these could be merged into one large throw-catch-catch-catch, rather than the throw-catch, throw-catch, throw-catch as will be performed now. This would only cause one rethrowing per set of matches rather than multiple throws, likely speeding up quite a bit.
   * The value of speed-ups of exception logic should be compared against other more important tasks. Exception logic *should* be exceptional, after all.
   * Technically this can be done in at least two ways:
     * Recursive catches: try { try { try { re-throw; } catch } catch } catch
     * Modifying the signature of fail(), accepting variadic functions, each with its own class matching. These functions can be merged, also recursively.

## 2.0, 3.0, q-io, q-rx

Depending on the interface change for q (removing the inner tuple), q 2.0 should include q-io and q 3.0 should include q-rx. Potentially this could be version 3.0 and 4.0 respectively.
