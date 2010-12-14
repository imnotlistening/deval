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

#include <client.h>

#include <pthread.h>
#include <netinet/in.h>

#ifndef _SERV_H
#define _SERV_H

/* The server's data. List of clients, etc. */
struct serv {

  /* Some administrative stuff for setting up the server. */
  int                  serv_fd;
  struct sockaddr      server_addr;

  /* The client list and associated data. */
  struct deval_client *clientp;
  unsigned int         clients;

  /* The admin list. */
  struct deval_client *adminp;
  unsigned int         admins;

  /* The big bad server pthread lock. */
  

};

#endif
