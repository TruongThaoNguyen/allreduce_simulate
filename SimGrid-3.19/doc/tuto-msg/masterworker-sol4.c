/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

#include <stdio.h> /* sprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

#define FINALIZE ((void*)221297) /* a magic number to tell people to stop working */

static char* build_channel_name(char* buffer, const char* sender, const char* receiver)
{
  strcpy(buffer, sender);
  strcat(buffer, ":");
  strcat(buffer, receiver);
  return buffer;
}

/* forward definitions */
static int master(int argc, char* argv[]);
static int worker(int argc, char* argv[]);

static int master(int argc, char* argv[])
{
  msg_host_t host_self = MSG_host_self();
  char* master_name    = (char*)MSG_host_get_name(host_self);
  char channel[1024];

  TRACE_category(master_name);

  double timeout   = xbt_str_parse_double(argv[1], "Invalid timeout: %s");            /** - timeout      */
  double comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s"); /** - Task compute cost    */
  double comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s"); /** - Task communication size */

  /* Get the info about the worker processes */
  int workers_count   = MSG_get_host_number();
  msg_host_t* workers = xbt_dynar_to_array(MSG_hosts_as_dynar());

  for (int i = 0; i < workers_count; i++) // Remove my host from the list
    if (host_self == workers[i]) {
      workers[i] = workers[workers_count - 1];
      workers_count--;
      break;
    }

  for (int i = 0; i < workers_count; i++)
    MSG_process_create("worker", worker, (void*)master_name, workers[i]);
  XBT_INFO("Got %d workers and will send tasks for %g seconds", workers_count, timeout);

  /* Dispatch the tasks */
  int task_num = 0;
  while (MSG_get_clock() < timeout) {

    /* Retrieve the next incomming request */
    XBT_DEBUG("Retrieve the next incomming request on %s", master_name);
    msg_task_t request = NULL;
    int res            = MSG_task_receive(&(request), master_name);
    xbt_assert(res == MSG_OK, "MSG_task_receive failed");
    msg_host_t requester = MSG_task_get_data(request);
    MSG_task_destroy(request);

    /* Prepare the task to be sent */
    char sprintf_buffer[64];
    sprintf(sprintf_buffer, "Task_%d", task_num);
    msg_task_t task = MSG_task_create(sprintf_buffer, comp_size, comm_size, NULL);
    MSG_task_set_category(task, master_name);

    /* Send this out */
    build_channel_name(channel, master_name, MSG_host_get_name(requester));

    XBT_DEBUG("Sending '%s' to channel '%s'", task->name, channel);
    MSG_task_send(task, channel);
    XBT_DEBUG("Sent");
    task_num++;
  }

  XBT_DEBUG("Time is up. Let's tell everybody the computation is over.");
  for (int i = 0; i < workers_count; i++) { /* We don't write in order, but the total amount is right */

    /* Don't write to a worker that did not request for work, or it will deadlock: both would be sending something */
    msg_task_t request = NULL;
    int res            = MSG_task_receive(&(request), master_name);
    xbt_assert(res == MSG_OK, "MSG_task_receive failed");
    msg_host_t requester = MSG_task_get_data(request);
    MSG_task_destroy(request);

    XBT_DEBUG("Stop worker %s", MSG_host_get_name(requester));
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, FINALIZE);
    MSG_task_send(finalize, build_channel_name(channel, master_name, MSG_host_get_name(requester)));
  }

  XBT_INFO("Sent %d tasks in total!", task_num);
  free(workers);
  return 0;
}

/** Worker function  */
static int worker(int argc, char* argv[])
{
  char channel[1024];

  const char* my_master = MSG_process_get_data(MSG_process_self());
  build_channel_name(channel, my_master, MSG_host_get_name(MSG_host_self()));

  XBT_DEBUG("Receiving on channel \"%s\"", channel);

  while (1) {
    /* Send a request */
    XBT_DEBUG("Sent a request to my master on %s", my_master);
    msg_task_t request = MSG_task_create("request", 0, 0, MSG_host_self());
    MSG_task_send(request, my_master);

    /* Wait for the answer */
    msg_task_t task = NULL;
    int res         = MSG_task_receive(&(task), channel);
    xbt_assert(res == MSG_OK, "MSG_task_receive failed");

    XBT_DEBUG("Received '%s'", MSG_task_get_name(task));
    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }

    XBT_DEBUG("Processing '%s'", MSG_task_get_name(task));
    MSG_task_execute(task);
    XBT_DEBUG("'%s' done", MSG_task_get_name(task));
    MSG_task_destroy(task);
  }
  XBT_DEBUG("I'm done. See you!");
  return 0;
}

/** Main function */
int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  /*  Create a simulated platform */
  MSG_create_environment(argv[1]);

  /*   Application deployment */
  MSG_function_register("master", master);
  MSG_function_register("worker", worker);
  MSG_launch_application(argv[2]);

  /* Run the simulation */
  msg_error_t res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return (res != MSG_OK);
}
