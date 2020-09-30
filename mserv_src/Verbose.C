/*
 * File     :
 *  Verbose.C
 *
 * Purpose  :
 *  This routine checks the verbosity level using the configuration
 *  verbosity value, and returns a bool to indicate if the lines should
 *  be printed.
 *
 * Author   :
 *  Phil Maechling
 *
 * Mod Date :
 *  27 July 2002
 *
 * This program is free software; you can redistribute it and/or modify
 * it with the sole restriction that:
 * You must cause any work that you distribute or publish, that in
 * whole or in part contains or is derived from the Program or any
 * part thereof, to be licensed as a whole at no charge to all third parties.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#include "Verbose.h"
#include "mservDiag.h"

Verbose::Verbose()
{
  p_curLevel = D_SILENT;
  p_curTarget = D_NO_TARGET;
}

void Verbose::setVerbosity(const int level)
{
  p_curLevel = level;
}

void Verbose::setDiagnostic(const int diagnostic)
{
  p_curTarget = diagnostic;
}

int Verbose::showVerbosity() const
{
  return p_curLevel;
}

int Verbose::showDiagnostic() const
{
  return p_curTarget;
}

bool Verbose::show(const int level,const int target)
{
  bool res = false;
  if((p_curLevel >= level ) || (p_curTarget == target))
  {
    res = true;
  }
  return res;
}
