/*
    hmail
    Copyright © Alex Waugh 2005

    $Id: $

    Simple command line program that sends mail to an SMTP relay.


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>



#define BUFFER_SIZE 4096

void getresponse(int code)
{
    /**/
}

char *extractaddress(char *line)
{
    char *address;
    char *end = line;
    char *start;
    char *addr;

    while (*end >= ' ') end++;

    address = malloc((end - line) + 1);
    /* if (address == NULL) */

    addr = address;

    while (line < end && *line != '>') {
        if (*line == '(') {
            while (line < end && *line != ')') line++;
            if (*line == ')') line++;
        } else if (*line == '<') {
            addr = address;
            line++;
        } else if (*line != ' ') {
            *addr++ = *line++;
        } else {
            line++;
        }
    }
    *addr = '\0';

    return address;
}

char *getline(char *buffer, size_t buflen, size_t *bufpos)
{
    char *line = buffer + *bufpos;

    while (*bufpos + 1 < buflen) {
        if (buffer[*bufpos] == '\r' && buffer[*bufpos + 1] == '\n') {
            (*bufpos) += 2;
            return line;
        }
        (*bufpos)++;
    }
    return NULL;
}

int main(void)
{
    char buffer[BUFFER_SIZE];
    size_t buflen;
    size_t bufpos;
    char *to = NULL;
    char *from = NULL;
    int socket = -1;
    int written;

    openlog("hmail", 0, LOG_MAIL);

    do {
        buflen = fread(buffer, 1, BUFFER_SIZE, stdin);
        bufpos = 0;

        while (to == NULL || from == NULL) {
            char *line = getline(buffer, buflen, &bufpos);

            if (line == NULL) {
               syslog(1, "%s header not found in first %d bytes of email", to ? "TO" : "FROM",BUFFER_SIZE);
               return EXIT_FAILURE;
            }

            if (strncasecmp(line, "To:", 3) == 0) {
                to = extractaddress(line + 3);
            } else if (strncasecmp(line, "From:", 5) == 0) {
                from = extractaddress(line + 5);
            }
        }

        bufpos = 0;
        if (socket != EOF) {
            char *hostname;
            char *domain;
            struct hostent *hp;
            struct sockaddr_in sockaddr;
            char tmp[4096];

            hostname = getenv("Inet$Hostname");
            if (hostname) hostname = strdup(hostname);
            domain = getenv("Inet$LocalDomain");

            socket = socket(AF_INET, SOCK_STREAM, 0);
            if (socket == EOF) {
                syslog(1, "Unable to open socket (%s)", strerror(errno));
                return EXIT_FAILURE;
            }

            hp = gethostbyname("smtp.cp15.org");
            if (hp == NULL) {
                syslog(1, "Unable to resolve (%s)", strerror(errno));/*h_errno?*/
                return EXIT_FAILURE;
            }

            memset(&(sockaddr), 0, sizeof(sockaddr));
            memcpy(&(sockaddr.sin_addr), hp->h_addr, hp->h_length);
            sockaddr.sin_family = AF_INET;
            sockaddr.sin_port = 25;/**/

            if (connect(socket, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in) == EOF) {
                syslog(1, "Unable to connect socket (%s)", strerror(errno));/*h_errno?*/
                return EXIT_FAILURE;
            }

            sprintf(tmp, "EHLO %s.%s\r\n",hostname ? hostname : "", domain ? domain : "");
            write(socket, tmp, strlen(tmp));
            getresponse(200);
            sprintf(tmp, "MAIL FROM:<%s>\r\n",from);
            write(socket, tmp, strlen(tmp));
            getresponse(200);
            sprintf(tmp, "RCPT TO:<%s>\r\n",to);
            write(socket, tmp, strlen(tmp));
            getresponse(200);
            sprintf(tmp, "DATA\r\n");
            write(socket, tmp, strlen(tmp));
            getresponse(300);
        }

        do {
            written = write(socket, buffer + bufpos, buflen - bufpos);
            if (written == EOF) {
               /*close sock */
               syslog(1, "Writing to socket failed (%s)",strerror(errno));
               return EXIT_FAILURE;
            }
            bufpos += written;
         } while (bufpos < buflen);

    } while (buflen > 0);

    sprintf(tmp, "\r\n.\r\n");
    write(socket, tmp, strlen(tmp));
    getresponse(200);
    sprintf(tmp, "QUIT\r\n");
    write(socket, tmp, strlen(tmp));

    /*close sock*/

    return EXIT_SUCCESS;
}

