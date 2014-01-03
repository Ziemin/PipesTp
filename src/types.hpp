#ifndef TYPES_HPP
#define TYPES_HPP

#include <memory>
#include "pipe_interface.h"

typedef OrgFreedesktopTelepathyPipeInterface Pipe;
typedef std::shared_ptr<Pipe> PipePtr;

#endif
