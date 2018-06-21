/* Copyright (c) 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "network_cm02.hpp"

namespace simgrid {
  namespace surf {

    class XBT_PRIVATE NetworkSmpiModel : public NetworkCm02Model {
    public:
      NetworkSmpiModel();
      ~NetworkSmpiModel();

      double latencyFactor(double size);
      double bandwidthFactor(double size);
      double bandwidthConstraint(double rate, double bound, double size);
    };
  }
}
