# Test of Using TBB with a non-TBB Thread

It is useful to allow a task running under TBB to launch a computation outside of TBB control (say to a GPU) and then at a later time
have a new TBB task be started once the outside computation has finished. Which the outside computation is running we want to be able
to have other TBB tasks be able to use the same thread as the one which launched th outside computation. The challenge for this goal is
how to keep the TBB _wait_ from prematurely ending in the case where all existing tasks have finished but the outside computation has 
not yet completed and started the deferred TBB task.

This repository contains four examples of how this could be done.

## external_task
This example uses the deprecated `tbb::task` interface for the implementation. A `tbb::empty_task` is used to make sure the 
`tbb::task::wait_for_all` performed by the main routine stays active until the deferred task is run. This is done by passing the
`tbb::empty_task` to the final task which we want run and have that final task do the needed `decrement_ref_count`. The deferred 
`tbb::task` is launched from the outside thread via the use of `tbb::task_arena::enqueue` where the `tbb::task_arena` used is the default
one of the application.

## external_group_looping
This example uses `tbb::task_group`. A `tbb::task_group::wait` will quasi-immediately exit once the last queued task for the group
finishes. No attempt is made to extend the life of the `wait`. Instead a `std::atomic` is used to keep track if the very last task
has completed. A `while` loop is performed over that atomic with the body of the loop continously calling `wait`. Like the **external_task**
example, a `tbb::task_arena` is used to allow the outside thread to spawn a new task into the `task_group`.

## external_group_extra_thread
This example is very similar to **external_group_looping**. The difference is a `std::mutex` is used to keep the `tbb::task_group::wait`
alive. The main thread first takes the lock on the `std::mutex` and then starts a new task in the group which does a wait on that mutex.
The mutex is not unlocked until the last task for the job finishes. In this way there is always a TBB task in the group live for the entire
time. The downside is this design requires N+1 TBB threads to be used compared to the other designs which only need N threads.

## external_group_extended
This example is similar to **external_group_looping**. The difference is a new `test::task_group` class is created by inheriting from 
`tbb::internal::task_group_base`. The new class only differs from the regular `tbb::task_group` in the addition of the use of a
`task_group_run_handle` class. The `test::task_group::wait` function now only exits once all outstanding group tasks have finished and all 
`task_group_run_handle`s associated with the group have been destroyed.
