// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT

#include "sandbox.hh"
#include "shared.h"

#include <stdio.h>

int sum(int a, int b)
{
  fprintf(stderr, "Adding %d to %d in sandbox\n", a, b);
  return a + b;
}

int crash()
{
  abort();
}

extern "C" void sandbox_init(sandbox::ExportedLibrary* library)
{
#define EXPORTED_FUNCTION(x, name) library->export_function(name);
#include "functions.inc"
}
