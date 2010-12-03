/*
 * deval: distributed evolutionary algorithm framework.
 *   Copyright (C) 2010  Alex Waterman
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.          
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alex Waterman - imNotListening@gmail.com
 */


#include <stdint.h>

#include <netinet/in.h>

#ifndef _CLIENT
#define _CLIENT

/* 
 * Typedefs and structures for representing a client connection and it's
 * type. Clients can be either SMP or single. Clients also need an IP address,
 * port, etc.
 */

struct deval_client {

  /* Access. */
  int             fd;

  /* Addressing. */
  struct sockaddr addr;

};


#endif
