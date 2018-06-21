/* Copyright (c) 2015-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_TRANSITION_HPP
#define SIMGRID_MC_TRANSITION_HPP

namespace simgrid {
namespace mc {

/** An element in the recorded path
 *
 *  At each decision point, we need to record which process transition
 *  is triggered and potentially which value is associated with this
 *  transition. The value is used to find which communication is triggered
 *  in things like waitany and for associating a given value of MC_random()
 *  calls.
 */
struct Transition {
  int pid = 0;

  /* Which transition was executed for this simcall
   *
   * Some simcalls can lead to different transitions:
   *
   * * waitany/testany can trigger on different messages;
   *
   * * random can produce different values.
   */
  int argument = 0;
};

}
}

#endif
