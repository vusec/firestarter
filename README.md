# FIRestarter
[FIRestarter: Practical Software Crash Recovery with Targeted Library-level Fault Injection [DSN'21]](https://download.vusec.net/papers/firestarter_dsn21.pdf)

Despite advances in software testing, many bugs still plague deployed software, leading to crashes and thus service
disruption in high-availability production applications. Existing crash recovery solutions are either limited to transient faults or
require manual annotations to target predetermined persistent bugs. Moreover, existing solutions are generally inefficient, hindering
practical deployment.

We present __FIRestarter__ (Fault Injection-based
Restarter), an efficient and automatic crash recovery solution for
commodity user applications. To eliminate the need for manual
annotations, FIRestarter injects targeted software faults at the
library interface to automatically trigger error handling code
for standard library calls already part of the application. In
particular, when a crash occurs, we roll back the application state
before the last recoverable library call, inject a fault, and restart
execution forcing the call to immediately return a predetermined
error code. This strategy allows the application to automatically
bypass the crashing code upon such a restart and exploits
existing error-handling code to recover from even persistent
bugs. Moreover, since library calls lie pervasively throughout the
code, our design provides a large recovery surface despite the
automated approach. Finally, FIRestarter’s recovery windows are
small and frequent compared to traditional checkpoint-restart,
which enables new optimizations such as the ability to support
rollback by means of hybrid hardware/software transactional
memory instrumentation and improve performance. We apply
FIRestarter to a number of event-driven server applications and
show our solution achieves near-instantaneous, state-preserving
crash recovery in the face of even persistent crashes. On popular
web servers, our evaluation results show a recovery surface of
at least 77%, with low performance overhead of at most 17%.

We will present [this paper](https://download.vusec.net/papers/firestarter_dsn21.pdf) at [DSN '21](http://dsn2021.ntu.edu.tw/).

### Source code

We are publishing the source code here, during the conference in June 21-24, 2021.
