/*
 * File     :
 *  Verbosity.h
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

#ifndef VERBOSE_H
#define VERBOSE_H

class Verbose
{
  public:

    Verbose();
    ~Verbose() {};
 
    void setVerbosity(const int level);
    int  showVerbosity() const; 

    void setDiagnostic(const int target);
    int  showDiagnostic() const; 
 
    bool show(const int level, const int target);
 
   private:

    int p_curLevel;
    int p_curTarget;
};
#endif
