/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/ex.hpp>

#include "src/kernel/activity/ExecImpl.hpp"
#include "src/msg/msg_private.hpp"
#include "src/simix/smx_private.hpp" /* MSG_task_listen looks inside the rdv directly. Not clean. */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_gos, msg, "Logging specific to MSG (gos)");

extern "C" {

/** \ingroup msg_task_usage
 * \brief Executes a task and waits for its termination.
 *
 * This function is used for describing the behavior of a process. It takes only one parameter.
 * \param task a #msg_task_t to execute on the location on which the process is running.
 * \return #MSG_OK if the task was successfully completed, #MSG_TASK_CANCELED or #MSG_HOST_FAILURE otherwise
 */
msg_error_t MSG_task_execute(msg_task_t task)
{
  return MSG_parallel_task_execute(task);
}

/** \ingroup msg_task_usage
 * \brief Executes a parallel task and waits for its termination.
 *
 * \param task a #msg_task_t to execute on the location on which the process is running.
 *
 * \return #MSG_OK if the task was successfully completed, #MSG_TASK_CANCELED
 * or #MSG_HOST_FAILURE otherwise
 */
msg_error_t MSG_parallel_task_execute(msg_task_t task)
{
  return MSG_parallel_task_execute_with_timeout(task, -1);
}

msg_error_t MSG_parallel_task_execute_with_timeout(msg_task_t task, double timeout)
{
  simdata_task_t simdata = task->simdata;
  e_smx_state_t comp_state;
  msg_error_t status = MSG_OK;

  TRACE_msg_task_execute_start(task);

  xbt_assert((not simdata->compute) && not task->simdata->isused,
             "This task is executed somewhere else. Go fix your code!");

  XBT_DEBUG("Computing on %s", MSG_process_get_name(MSG_process_self()));

  if (simdata->flops_amount <= 0.0 && not simdata->host_nb) {
    TRACE_msg_task_execute_end(task);
    return MSG_OK;
  }

  try {
    simdata->setUsed();

    if (simdata->host_nb > 0) {
      simdata->compute =
          boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(simcall_execution_parallel_start(
              task->name, simdata->host_nb, simdata->host_list, simdata->flops_parallel_amount,
              simdata->bytes_parallel_amount, -1.0, timeout));
      XBT_DEBUG("Parallel execution action created: %p", simdata->compute.get());
    } else {
      simdata->compute = boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(
          simcall_execution_start(task->name, simdata->flops_amount, simdata->priority, simdata->bound,
                                  MSG_process_get_host(MSG_process_self())));
    }
    simcall_set_category(simdata->compute, task->category);
    comp_state = simcall_execution_wait(simdata->compute);

    simdata->setNotUsed();

    XBT_DEBUG("Execution task '%s' finished in state %d", task->name, (int)comp_state);
  }
  catch (xbt_ex& e) {
    switch (e.category) {
    case cancel_error:
      status = MSG_TASK_CANCELED;
      break;
    case host_error:
      status = MSG_HOST_FAILURE;
      break;
    case timeout_error:
      status = MSG_TIMEOUT;
      break;
    default:
      throw;
    }
  }

  /* action ended, set comm and compute = nullptr, the actions is already destroyed in the main function */
  simdata->flops_amount = 0.0;
  simdata->comm = nullptr;
  simdata->compute = nullptr;
  TRACE_msg_task_execute_end(task);

  return status;
}

/** \ingroup msg_task_usage
 * \brief Sleep for the specified number of seconds
 *
 * Makes the current process sleep until \a time seconds have elapsed.
 *
 * \param nb_sec a number of second
 */
msg_error_t MSG_process_sleep(double nb_sec)
{
  msg_error_t status = MSG_OK;

  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::byName(instr_pid(MSG_process_self()))->getState("MSG_PROCESS_STATE")->pushEvent("sleep");

  try {
    simcall_process_sleep(nb_sec);
  }
  catch(xbt_ex& e) {
    if (e.category == cancel_error) {
      XBT_DEBUG("According to the JAVA API, a sleep call should only deal with HostFailureException, I'm lost.");
      // adsein: MSG_TASK_CANCELED is assigned when someone kills the process that made the sleep, this is not
      // correct. For instance, when the node is turned off, the error should be MSG_HOST_FAILURE, which is by the way
      // and according to the JAVA document, the only exception that can be triggered by MSG_Process_sleep call.
      // To avoid possible impacts in the code, I just raised a host_failure exception for the moment in the JAVA code
      // and did not change anythings at the C level.
      // See comment in the jmsg_process.c file, function JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_sleep(JNIEnv *env, jclass cls, jlong jmillis, jint jnanos)
      status = MSG_TASK_CANCELED;
    } else
      throw;
  }

  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::byName(instr_pid(MSG_process_self()))->getState("MSG_PROCESS_STATE")->popEvent();

  return status;
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox.
 *
 * This is a blocking function, the execution flow will be blocked until the task is received. See #MSG_task_irecv
 * for receiving tasks asynchronously.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_task_receive(msg_task_t * task, const char *alias)
{
  return MSG_task_receive_with_timeout(task, alias, -1);
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox at a given rate.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 * \param rate limit the reception to rate bandwidth (byte/sec)
 *
 * The rate parameter can be used to receive a task with a limited
 * bandwidth (smaller than the physical available value). Use
 * MSG_task_receive() if you don't limit the rate (or pass -1 as a
 * rate value do disable this feature).
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_task_receive_bounded(msg_task_t * task, const char *alias, double rate)
{
  return MSG_task_receive_with_timeout_bounded(task, alias, -1, rate);
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox with a given timeout.
 *
 * This is a blocking function with a timeout, the execution flow will be blocked until the task is received or the
 * timeout is achieved. See #MSG_task_irecv for receiving tasks asynchronously.  You can provide a -1 timeout
 * to obtain an infinite timeout.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 * \param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_receive)
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_receive_with_timeout(msg_task_t * task, const char *alias, double timeout)
{
  return MSG_task_receive_ext(task, alias, timeout, nullptr);
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox with a given timeout and at a given rate.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 * \param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_receive)
 * \param rate limit the reception to rate bandwidth (byte/sec)
 *
 * The rate parameter can be used to send a task with a limited
 * bandwidth (smaller than the physical available value). Use
 * MSG_task_receive() if you don't limit the rate (or pass -1 as a
 * rate value do disable this feature).
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_receive_with_timeout_bounded(msg_task_t * task, const char *alias, double timeout,double rate)
{
  return MSG_task_receive_ext_bounded(task, alias, timeout, nullptr, rate);
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox from a specific host with a given timeout.
 *
 * This is a blocking function with a timeout, the execution flow will be blocked until the task is received or the
 * timeout is achieved. See #MSG_task_irecv for receiving tasks asynchronously. You can provide a -1 timeout
 * to obtain an infinite timeout.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 * \param timeout is the maximum wait time for completion (provide -1 for no timeout)
 * \param host a #msg_host_t host from where the task was sent
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
* #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_receive_ext(msg_task_t * task, const char *alias, double timeout, msg_host_t host)
{
  XBT_DEBUG("MSG_task_receive_ext: Trying to receive a message on mailbox '%s'", alias);
  return MSG_task_receive_ext_bounded(task, alias, timeout, host, -1.0);
}

/** \ingroup msg_task_usage
 * \brief Receives a task from a mailbox from a specific host with a given timeout  and at a given rate.
 *
 * \param task a memory location for storing a #msg_task_t.
 * \param alias name of the mailbox to receive the task from
 * \param timeout is the maximum wait time for completion (provide -1 for no timeout)
 * \param host a #msg_host_t host from where the task was sent
 * \param rate limit the reception to rate bandwidth (byte/sec)
 *
 * The rate parameter can be used to receive a task with a limited
 * bandwidth (smaller than the physical available value). Use
 * MSG_task_receive_ext() if you don't limit the rate (or pass -1 as a
 * rate value do disable this feature).
 *
 * \return Returns
 * #MSG_OK if the task was successfully received,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_receive_ext_bounded(msg_task_t * task, const char *alias, double timeout, msg_host_t host,
                                         double rate)
{
  XBT_DEBUG("MSG_task_receive_ext: Trying to receive a message on mailbox '%s'", alias);
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(alias);
  msg_error_t ret = MSG_OK;
  /* We no longer support getting a task from a specific host */
  if (host)
    THROW_UNIMPLEMENTED;

  TRACE_msg_task_get_start();

  /* Sanity check */
  xbt_assert(task, "Null pointer for the task storage");

  if (*task)
    XBT_WARN("Asked to write the received task in a non empty struct -- proceeding.");

  /* Try to receive it by calling SIMIX network layer */
  try {
    simcall_comm_recv(MSG_process_self()->getImpl(), mailbox->getImpl(), task, nullptr, nullptr, nullptr, nullptr, timeout, rate);
    XBT_DEBUG("Got task %s from %s", (*task)->name, mailbox->getCname());
    (*task)->simdata->setNotUsed();
  }
  catch (xbt_ex& e) {
    switch (e.category) {
    case host_error:
    case cancel_error:
      ret = MSG_HOST_FAILURE;
      break;
    case network_error:
      ret = MSG_TRANSFER_FAILURE;
      break;
    case timeout_error:
      ret = MSG_TIMEOUT;
      break;
    default:
      throw;
    }
  }

  if (ret != MSG_HOST_FAILURE && ret != MSG_TRANSFER_FAILURE && ret != MSG_TIMEOUT) {
    TRACE_msg_task_get_end(*task);
  }
  return ret;
}

/* Internal function used to factorize code between MSG_task_isend_with_matching() and MSG_task_dsend(). */
static inline msg_comm_t MSG_task_isend_internal(msg_task_t task, const char* alias,
                                                 int (*match_fun)(void*, void*, void*), void* match_data,
                                                 void_f_pvoid_t cleanup, int detached)
{
  simdata_task_t t_simdata = nullptr;
  msg_process_t myself = MSG_process_self();
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(alias);
  int call_end = TRACE_msg_task_put_start(task);

  /* Prepare the task to send */
  t_simdata = task->simdata;
  t_simdata->sender = myself;
  t_simdata->source = MSG_host_self();
  t_simdata->setUsed();
  t_simdata->comm = nullptr;
  msg_global->sent_msg++;

  /* Send it by calling SIMIX network layer */
  smx_activity_t act =
      simcall_comm_isend(myself->getImpl(), mailbox->getImpl(), t_simdata->bytes_amount, t_simdata->rate, task,
                         sizeof(void*), (simix_match_func_t)match_fun, cleanup, nullptr, match_data, detached);
  t_simdata->comm = boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(act);

  msg_comm_t comm = nullptr;
  if (not detached) {
    comm = new simgrid::msg::Comm(task, nullptr, act);
  }

  if (TRACE_is_enabled())
    simcall_set_category(act, task->category);
  if (call_end)
    TRACE_msg_task_put_end();

  return comm;
}

/** \ingroup msg_task_usage
 * \brief Sends a task on a mailbox.
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test() to end the communication.
 *
 * \param task a #msg_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \return the msg_comm_t communication created
 */
msg_comm_t MSG_task_isend(msg_task_t task, const char *alias)
{
  return MSG_task_isend_internal(task, alias, nullptr, nullptr, nullptr, 0);
}

/** \ingroup msg_task_usage
 * \brief Sends a task on a mailbox with a maximum rate
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test() to end the communication. The maxrate
 * parameter allows the application to limit the bandwidth utilization of network links when sending the task.
 *
 * \param task a #msg_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \param maxrate the maximum communication rate for sending this task (byte/sec).
 * \return the msg_comm_t communication created
 */
msg_comm_t MSG_task_isend_bounded(msg_task_t task, const char *alias, double maxrate)
{
  task->simdata->rate = maxrate;
  return MSG_task_isend_internal(task, alias, nullptr, nullptr, nullptr, 0);
}

/** \ingroup msg_task_usage
 * \brief Sends a task on a mailbox, with support for matching requests
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test() to end the communication.
 *
 * \param task a #msg_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \param match_fun boolean function which parameters are:
 *        - match_data_provided_here
 *        - match_data_provided_by_other_side_if_any
 *        - the_smx_synchro_describing_the_other_side
 * \param match_data user provided data passed to match_fun
 * \return the msg_comm_t communication created
 */
msg_comm_t MSG_task_isend_with_matching(msg_task_t task, const char* alias, int (*match_fun)(void*, void*, void*),
                                        void* match_data)
{
  return MSG_task_isend_internal(task, alias, match_fun, match_data, nullptr, 0);
}

/** \ingroup msg_task_usage
 * \brief Sends a task on a mailbox.
 *
 * This is a non blocking detached send function.
 * Think of it as a best effort send. Keep in mind that the third parameter is only called if the communication fails.
 * If the communication does work, it is responsibility of the receiver code to free anything related to the task, as
 * usual. More details on this can be obtained on
 * <a href="http://lists.gforge.inria.fr/pipermail/simgrid-user/2011-November/002649.html">this thread</a>
 * in the SimGrid-user mailing list archive.
 *
 * \param task a #msg_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \param cleanup a function to destroy the task if the communication fails, e.g. MSG_task_destroy
 * (if nullptr, no function will be called)
 */
void MSG_task_dsend(msg_task_t task, const char *alias, void_f_pvoid_t cleanup)
{
  MSG_task_isend_internal(task, alias, nullptr, nullptr, cleanup, 1);
}

/** \ingroup msg_task_usage
 * \brief Sends a task on a mailbox with a maximal rate.
 *
 * This is a non blocking detached send function.
 * Think of it as a best effort send. Keep in mind that the third parameter is only called if the communication fails.
 * If the communication does work, it is responsibility of the receiver code to free anything related to the task, as
 * usual. More details on this can be obtained on
 * <a href="http://lists.gforge.inria.fr/pipermail/simgrid-user/2011-November/002649.html">this thread</a>
 * in the SimGrid-user mailing list archive.
 *
 * The rate parameter can be used to send a task with a limited
 * bandwidth (smaller than the physical available value). Use
 * MSG_task_dsend() if you don't limit the rate (or pass -1 as a rate
 * value do disable this feature).
 *
 * \param task a #msg_task_t to send on another location.
 * \param alias name of the mailbox to sent the task to
 * \param cleanup a function to destroy the task if the
 * communication fails, e.g. MSG_task_destroy
 * (if nullptr, no function will be called)
 * \param maxrate the maximum communication rate for sending this task (byte/sec)
 *
 */
void MSG_task_dsend_bounded(msg_task_t task, const char *alias, void_f_pvoid_t cleanup, double maxrate)
{
  task->simdata->rate = maxrate;
  MSG_task_dsend(task, alias, cleanup);
}

/** \ingroup msg_task_usage
 * \brief Starts listening for receiving a task from an asynchronous communication.
 *
 * This is a non blocking function: use MSG_comm_wait() or MSG_comm_test() to end the communication.
 *
 * \param task a memory location for storing a #msg_task_t. has to be valid until the end of the communication.
 * \param name of the mailbox to receive the task on
 * \return the msg_comm_t communication created
 */
msg_comm_t MSG_task_irecv(msg_task_t *task, const char *name)
{
  return MSG_task_irecv_bounded(task, name, -1.0);
}

/** \ingroup msg_task_usage
 * \brief Starts listening for receiving a task from an asynchronous communication at a given rate.
 *
 * The rate parameter can be used to receive a task with a limited
 * bandwidth (smaller than the physical available value). Use
 * MSG_task_irecv() if you don't limit the rate (or pass -1 as a rate
 * value do disable this feature).
 *
 * \param task a memory location for storing a #msg_task_t. has to be valid until the end of the communication.
 * \param name of the mailbox to receive the task on
 * \param rate limit the bandwidth to the given rate (byte/sec)
 * \return the msg_comm_t communication created
 */
msg_comm_t MSG_task_irecv_bounded(msg_task_t *task, const char *name, double rate)
{
  simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName(name);

  /* FIXME: these functions are not traceable */
  /* Sanity check */
  xbt_assert(task, "Null pointer for the task storage");

  if (*task)
    XBT_CRITICAL("MSG_task_irecv() was asked to write in a non empty task struct.");

  /* Try to receive it by calling SIMIX network layer */
  msg_comm_t comm =
      new simgrid::msg::Comm(nullptr, task, simcall_comm_irecv(SIMIX_process_self(), mbox->getImpl(), task, nullptr,
                                                               nullptr, nullptr, nullptr, rate));

  return comm;
}

/** \ingroup msg_task_usage
 * \brief Checks whether a communication is done, and if yes, finalizes it.
 * \param comm the communication to test
 * \return TRUE if the communication is finished
 * (but it may have failed, use MSG_comm_get_status() to know its status)
 * or FALSE if the communication is not finished yet
 * If the status is FALSE, don't forget to use MSG_process_sleep() after the test.
 */
int MSG_comm_test(msg_comm_t comm)
{
  int finished = 0;

  try {
    finished = simcall_comm_test(comm->s_comm);
    if (finished && comm->task_received != nullptr) {
      /* I am the receiver */
      (*comm->task_received)->simdata->setNotUsed();
    }
  }
  catch (xbt_ex& e) {
    switch (e.category) {
      case network_error:
        comm->status = MSG_TRANSFER_FAILURE;
        finished = 1;
        break;
      case timeout_error:
        comm->status = MSG_TIMEOUT;
        finished = 1;
        break;
      default:
        throw;
    }
  }

  return finished;
}

/** \ingroup msg_task_usage
 * \brief This function checks if a communication is finished.
 * \param comms a vector of communications
 * \return the position of the finished communication if any
 * (but it may have failed, use MSG_comm_get_status() to know its status),
 * or -1 if none is finished
 */
int MSG_comm_testany(xbt_dynar_t comms)
{
  int finished_index = -1;

  /* Create the equivalent array with SIMIX objects: */
  std::vector<simgrid::kernel::activity::ActivityImplPtr> s_comms;
  s_comms.reserve(xbt_dynar_length(comms));
  msg_comm_t comm;
  unsigned int cursor;
  xbt_dynar_foreach(comms, cursor, comm) {
    s_comms.push_back(comm->s_comm);
  }

  msg_error_t status = MSG_OK;
  try {
    finished_index = simcall_comm_testany(s_comms.data(), s_comms.size());
  }
  catch (xbt_ex& e) {
    switch (e.category) {
      case network_error:
        finished_index = e.value;
        status = MSG_TRANSFER_FAILURE;
        break;
      case timeout_error:
        finished_index = e.value;
        status = MSG_TIMEOUT;
        break;
      default:
        throw;
    }
  }

  if (finished_index != -1) {
    comm = xbt_dynar_get_as(comms, finished_index, msg_comm_t);
    /* the communication is finished */
    comm->status = status;

    if (status == MSG_OK && comm->task_received != nullptr) {
      /* I am the receiver */
      (*comm->task_received)->simdata->setNotUsed();
    }
  }

  return finished_index;
}

/** \ingroup msg_task_usage
 * \brief Destroys a communication.
 * \param comm the communication to destroy.
 */
void MSG_comm_destroy(msg_comm_t comm)
{
  delete comm;
}

/** \ingroup msg_task_usage
 * \brief Wait for the completion of a communication.
 *
 * It takes two parameters.
 * \param comm the communication to wait.
 * \param timeout Wait until the communication terminates or the timeout occurs.
 *                You can provide a -1 timeout to obtain an infinite timeout.
 * \return msg_error_t
 */
msg_error_t MSG_comm_wait(msg_comm_t comm, double timeout)
{
  try {
    simcall_comm_wait(comm->s_comm, timeout);

    if (comm->task_received != nullptr) {
      /* I am the receiver */
      (*comm->task_received)->simdata->setNotUsed();
    }

    /* FIXME: these functions are not traceable */
  }
  catch (xbt_ex& e) {
    switch (e.category) {
    case network_error:
      comm->status = MSG_TRANSFER_FAILURE;
      break;
    case timeout_error:
      comm->status = MSG_TIMEOUT;
      break;
    default:
      throw;
    }
  }

  return comm->status;
}

/** \ingroup msg_task_usage
* \brief This function is called by a sender and permit to wait for each communication
*
* \param comm a vector of communication
* \param nb_elem is the size of the comm vector
* \param timeout for each call of MSG_comm_wait
*/
void MSG_comm_waitall(msg_comm_t * comm, int nb_elem, double timeout)
{
  for (int i = 0; i < nb_elem; i++)
    MSG_comm_wait(comm[i], timeout);
}

/** \ingroup msg_task_usage
 * \brief This function waits for the first communication finished in a list.
 * \param comms a vector of communications
 * \return the position of the first finished communication
 * (but it may have failed, use MSG_comm_get_status() to know its status)
 */
int MSG_comm_waitany(xbt_dynar_t comms)
{
  int finished_index = -1;

  /* create the equivalent dynar with SIMIX objects */
  xbt_dynar_t s_comms = xbt_dynar_new(sizeof(smx_activity_t), [](void*ptr){
    intrusive_ptr_release(*(simgrid::kernel::activity::ActivityImpl**)ptr);
  });
  msg_comm_t comm;
  unsigned int cursor;
  xbt_dynar_foreach(comms, cursor, comm) {
    intrusive_ptr_add_ref(comm->s_comm.get());
    xbt_dynar_push_as(s_comms, simgrid::kernel::activity::ActivityImpl*, comm->s_comm.get());
  }

  msg_error_t status = MSG_OK;
  try {
    finished_index = simcall_comm_waitany(s_comms, -1);
  }
  catch(xbt_ex& e) {
    switch (e.category) {
      case network_error:
        finished_index = e.value;
        status = MSG_TRANSFER_FAILURE;
        break;
      case timeout_error:
        finished_index = e.value;
        status = MSG_TIMEOUT;
        break;
      default:
        throw;
    }
  }

  xbt_assert(finished_index != -1, "WaitAny returned -1");
  xbt_dynar_free(&s_comms);

  comm = xbt_dynar_get_as(comms, finished_index, msg_comm_t);
  /* the communication is finished */
  comm->status = status;

  if (comm->task_received != nullptr) {
    /* I am the receiver */
    (*comm->task_received)->simdata->setNotUsed();
  }

  return finished_index;
}

/**
 * \ingroup msg_task_usage
 * \brief Returns the error (if any) that occurred during a finished communication.
 * \param comm a finished communication
 * \return the status of the communication, or #MSG_OK if no error occurred
 * during the communication
 */
msg_error_t MSG_comm_get_status(msg_comm_t comm) {

  return comm->status;
}

/** \ingroup msg_task_usage
 * \brief Get a task (#msg_task_t) from a communication
 *
 * \param comm the communication where to get the task
 * \return the task from the communication
 */
msg_task_t MSG_comm_get_task(msg_comm_t comm)
{
  xbt_assert(comm, "Invalid parameter");

  return comm->task_received ? *comm->task_received : comm->task_sent;
}

/**
 * \brief This function is called by SIMIX in kernel mode to copy the data of a comm.
 * \param synchro the comm
 * \param buff the data copied
 * \param buff_size size of the buffer
 */
void MSG_comm_copy_data_from_SIMIX(smx_activity_t synchro, void* buff, size_t buff_size)
{
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);

  SIMIX_comm_copy_pointer_callback(comm, buff, buff_size);

  // notify the user callback if any
  if (msg_global->task_copy_callback) {
    msg_task_t task = static_cast<msg_task_t>(buff);
    msg_global->task_copy_callback(task, comm->src_proc->ciface(), comm->dst_proc->ciface());
  }
}

/** \ingroup msg_task_usage
 * \brief Sends a task to a mailbox
 *
 * This is a blocking function, the execution flow will be blocked until the task is sent (and received on the other
 * side if #MSG_task_receive is used).
 * See #MSG_task_isend for sending tasks asynchronously.
 *
 * \param task the task to be sent
 * \param alias the mailbox name to where the task is sent
 *
 * \return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_task_send(msg_task_t task, const char *alias)
{
  XBT_DEBUG("MSG_task_send: Trying to send a message on mailbox '%s'", alias);
  return MSG_task_send_with_timeout(task, alias, -1);
}

/** \ingroup msg_task_usage
 * \brief Sends a task to a mailbox with a maximum rate
 *
 * This is a blocking function, the execution flow will be blocked until the task is sent. The maxrate parameter allows
 * the application to limit the bandwidth utilization of network links when sending the task.
 *
 * The maxrate parameter can be used to send a task with a limited
 * bandwidth (smaller than the physical available value). Use
 * MSG_task_send() if you don't limit the rate (or pass -1 as a rate
 * value do disable this feature).
 *
 * \param task the task to be sent
 * \param alias the mailbox name to where the task is sent
 * \param maxrate the maximum communication rate for sending this task (byte/sec)
 *
 * \return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE otherwise.
 */
msg_error_t MSG_task_send_bounded(msg_task_t task, const char *alias, double maxrate)
{
  task->simdata->rate = maxrate;
  return MSG_task_send(task, alias);
}

/** \ingroup msg_task_usage
 * \brief Sends a task to a mailbox with a timeout
 *
 * This is a blocking function, the execution flow will be blocked until the task is sent or the timeout is achieved.
 *
 * \param task the task to be sent
 * \param alias the mailbox name to where the task is sent
 * \param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_send)
 *
 * \return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_send_with_timeout(msg_task_t task, const char *alias, double timeout)
{
  msg_error_t ret = MSG_OK;
  simdata_task_t t_simdata = nullptr;
  msg_process_t process = MSG_process_self();
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName(alias);

  int call_end = TRACE_msg_task_put_start(task);

  /* Prepare the task to send */
  t_simdata = task->simdata;
  t_simdata->sender = process;
  t_simdata->source = MSG_host_self();

  t_simdata->setUsed();

  t_simdata->comm = nullptr;
  msg_global->sent_msg++;

  /* Try to send it by calling SIMIX network layer */
  try {
    smx_activity_t comm = nullptr; /* MC needs the comm to be set to nullptr during the simix call  */
    comm = simcall_comm_isend(SIMIX_process_self(), mailbox->getImpl(),t_simdata->bytes_amount,
                              t_simdata->rate, task, sizeof(void *), nullptr, nullptr, nullptr, task, 0);
    if (TRACE_is_enabled())
      simcall_set_category(comm, task->category);
    t_simdata->comm = boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(comm);
    simcall_comm_wait(comm, timeout);
  }
  catch (xbt_ex& e) {
    switch (e.category) {
    case cancel_error:
      ret = MSG_HOST_FAILURE;
      break;
    case network_error:
      ret = MSG_TRANSFER_FAILURE;
      break;
    case timeout_error:
      ret = MSG_TIMEOUT;
      break;
    default:
      throw;
    }

    /* If the send failed, it is not used anymore */
    t_simdata->setNotUsed();
  }

  if (call_end)
    TRACE_msg_task_put_end();
  return ret;
}

/** \ingroup msg_task_usage
 * \brief Sends a task to a mailbox with a timeout and with a maximum rate
 *
 * This is a blocking function, the execution flow will be blocked until the task is sent or the timeout is achieved.
 *
 * The maxrate parameter can be used to send a task with a limited
 * bandwidth (smaller than the physical available value). Use
 * MSG_task_send_with_timeout() if you don't limit the rate (or pass -1 as a rate
 * value do disable this feature).
 *
 * \param task the task to be sent
 * \param alias the mailbox name to where the task is sent
 * \param timeout is the maximum wait time for completion (if -1, this call is the same as #MSG_task_send)
 * \param maxrate the maximum communication rate for sending this task (byte/sec)
 *
 * \return Returns #MSG_OK if the task was successfully sent,
 * #MSG_HOST_FAILURE, or #MSG_TRANSFER_FAILURE, or #MSG_TIMEOUT otherwise.
 */
msg_error_t MSG_task_send_with_timeout_bounded(msg_task_t task, const char *alias, double timeout, double maxrate)
{
  task->simdata->rate = maxrate;
  return MSG_task_send_with_timeout(task, alias, timeout);
}

/** \ingroup msg_task_usage
 * \brief Check if there is a communication going on in a mailbox.
 *
 * \param alias the name of the mailbox to be considered
 *
 * \return Returns 1 if there is a communication, 0 otherwise
 */
int MSG_task_listen(const char *alias)
{
  simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName(alias);
  return mbox->listen() ? 1 : 0;
}

/** \ingroup msg_task_usage
 * \brief Look if there is a communication on a mailbox and return the PID of the sender process.
 *
 * \param alias the name of the mailbox to be considered
 *
 * \return Returns the PID of sender process,
 * -1 if there is no communication in the mailbox.
 */
int MSG_task_listen_from(const char *alias)
{
  simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName(alias);
  simgrid::kernel::activity::CommImplPtr comm =
      boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(mbox->front());

  if (not comm)
    return -1;

  return MSG_process_get_PID(static_cast<msg_task_t>(comm->src_buff)->simdata->sender);
}

/** \ingroup msg_task_usage
 * \brief Sets the tracing category of a task.
 *
 * This function should be called after the creation of a MSG task, to define the category of that task. The
 * first parameter task must contain a task that was  created with the function #MSG_task_create. The second
 * parameter category must contain a category that was previously declared with the function #TRACE_category
 * (or with #TRACE_category_with_color).
 *
 * See \ref outcomes_vizu for details on how to trace the (categorized) resource utilization.
 *
 * \param task the task that is going to be categorized
 * \param category the name of the category to be associated to the task
 *
 * \see MSG_task_get_category, TRACE_category, TRACE_category_with_color
 */
void MSG_task_set_category (msg_task_t task, const char *category)
{
  TRACE_msg_set_task_category (task, category);
}

/** \ingroup msg_task_usage
 *
 * \brief Gets the current tracing category of a task.
 *
 * \param task the task to be considered
 *
 * \see MSG_task_set_category
 *
 * \return Returns the name of the tracing category of the given task, nullptr otherwise
 */
const char *MSG_task_get_category (msg_task_t task)
{
  return task->category;
}
}
