/*
 * Routines for making a HTTP connection to a server on the Internet,
 * sending a simple HTTP GET request, and interpreting the response headers
 * that come back.
 *
 * E. Stark, 11/18/97 for CSE 230
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/socket.h>

#include "url.h"
#include "http.h"

typedef struct HDRNODE *HEADERS;
HEADERS http_parse_headers(HTTP *http);
void http_free_headers(HEADERS env);

/*
 * Routines to manage HTTP connections
 */

typedef enum { ST_REQ, ST_HDRS, ST_BODY, ST_DONE } HTTP_STATE;

struct http {
  FILE *file;			/* Stream to remote server */
  HTTP_STATE state;		/* State of the connection */
  int code;			/* Response code */
  char version[4];		/* HTTP version from the response */
  char *response;		/* Response string with message */
  HEADERS headers;		/* Reply headers */
};

/*
 * Open an HTTP connection for a specified IP address and port number
 */

HTTP *
http_open(IPADDR *addr, int port)
{
  HTTP *http;
  struct sockaddr_in sa;
  int sock;

  if(addr == NULL){
    return(NULL);
  }
  if((http = malloc(sizeof(*http))) == NULL){
    return(NULL);
  }
  bzero(http, sizeof(*http));
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    free(http);
    return(NULL);
  }
  bzero(&sa, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);

  bcopy(addr, &sa.sin_addr.s_addr, sizeof(struct in_addr));
  if(connect(sock, (struct sockaddr *)(&sa), sizeof(sa)) < 0
     || (http->file = fdopen(sock, "w+")) == NULL) {
    free(http);
    close(sock);
    return(NULL);
  }
  http->state = ST_REQ;
  return(http);
}

/*
 * Close an HTTP connection that was previously opened.
 */

int
http_close(HTTP *http)
{
  int err;

  err = fclose(http->file);
  free(http->response);
  http_free_headers(http->headers);
  free(http);
  return(err);
}

/*
 * Obtain the underlying FILE in an HTTP connection.
 * This can be used to issue additional headers after the request.
 */

FILE *
http_file(HTTP *http)
{
  return(http->file);
}

/*
 * Issue an HTTP GET request for a URL on a connection.
 * After calling this function, the caller may send additional
 * headers, if desired, then must call http_response() before
 * attempting to read any document returned on the connection.
 */

int http_request(HTTP *http, URL *up)
{
  void *prev;

  if(http->state != ST_REQ)
    return(1);
  /* Ignore SIGPIPE so we don't die while doing this */
  prev = signal(SIGPIPE, SIG_IGN);

  if(fprintf(http->file, "GET %s://%s:%d%s HTTP/1.0\r\nHost: %s\r\n",
	     url_method(up), url_hostname(up), url_port(up),
	     url_path(up), url_hostname(up)) == -1) {
    signal(SIGPIPE, prev);
    return(1);
  }
  http->state = ST_HDRS;
  signal(SIGPIPE, prev);
  return(0);
}

/*
 * Finish outputting an HTTP request and read the reply
 * headers from the response.  After calling this, http_getc()
 * may be used to collect any document returned as part of the
 * response.
 */

int
http_response(HTTP *http)
{
  void *prev;
  char *response = NULL;
    int len;
  size_t size = 0;//sizeof(response);

  if(http->state != ST_HDRS)
    return(1);
  /* Ignore SIGPIPE so we don't die while doing this */
  prev = signal(SIGPIPE, SIG_IGN);
  if(fprintf(http->file, "\r\n") == -1
     || fflush(http->file) == EOF) {
    signal(SIGPIPE, prev);
    return(1);
  }
  rewind(http->file);

  signal(SIGPIPE, prev);

    //size = len;

  getline(&response, &size, http->file);

  //size = strlen(response);
  len = size;


  if(response == NULL
     || (http->response = malloc(size+1)) == NULL)
    return(1);

    //size = strlen(response)-2;
  strncpy(http->response, response, size);
  //free(response);
  len = strlen(http->response) - 1;
  do{
    http->response[len--] = '\0';}
  while(len >= 0 &&
	(http->response[len] == '\r'
	 || http->response[len] == '\n'));

free(response);
  if(sscanf(http->response, "HTTP/%3s %d ", http->version, &http->code) != 2){
        return(1);

  }
  http->headers = http_parse_headers(http);
  http->state = ST_BODY;


  return(0);
}

/*
 * Retrieve the HTTP status line and code returned as the
 * first line of the response from the server
 */

char *
http_status(HTTP *http, int *code)
{
  if(http->state != ST_BODY)
    return(NULL);
  if(code != NULL)
    *code = http->code;
  return(http->response);
}

/*
 * Read the next character of a document from an HTTP connection
 */

int
http_getc(HTTP *http)
{
  if(http->state != ST_BODY)
    return(EOF);
  return(fgetc(http->file));
}

/*
 * Routines for parsing the RFC822-style headers that come back
 * as part of the response to an HTTP request.
 */

typedef struct HDRNODE {
    char *key;
    char *value;
    struct HDRNODE *next;
} HDRNODE;

/*
 * Function for parsing RFC 822 header lines directly from input stream.
 */

HEADERS
http_parse_headers(HTTP *http)
{
    FILE *f = http->file;
    HEADERS env = NULL;//, last = NULL;
    HDRNODE *node;
    int len;
    char *cp; // *line, *ll
    char *l = NULL;
    size_t size = 0; // added, might not be grabbing the right size

    len = size;

    while((getline(&l, &size, f)) != -1) {
     // used to be ll
      len = strlen(l);

        //l = malloc(len+1); // used to be line =
        l[len] = '\0';

        while(len > 0 && (l[len-1] == '\n' || l[len-1] == '\r')){
          l[--len] = '\0'; // used to be --len
        }
        if(len == 0) {
          break;
        }

        node = malloc(sizeof(HDRNODE)); // ERRORRRRRRRR
        if (env == NULL){
          env = node;
        }
        else if (env->next == NULL){
          env->next = node;
        }
        else{
          HDRNODE *temp = env->next;
          while (temp->next != NULL){
            temp = temp->next;
          }
          temp->next = node;
        }
        node->next = NULL;

        for(cp = l; *cp == ' '; cp++) ;
          l = cp;
        for( ; *cp != ':' && *cp != '\0'; cp++) ;
          if(*cp == '\0' || *(cp+1) != ' ') {
            //free(line);
            free(node);
            continue;
          }
            *cp++ = '\0';
            node->key = strdup(l);
            while(*cp == ' ')
              cp++;

            node->value = strdup(cp);

            for(cp = node->key; *cp != '\0'; cp++);
              //if(isupper(*cp))
                //*cp = tolower(*cp);

              //last->next = node;
              //last = node;
              //free(line);
          //free(node);
    }
    free(l);
          return(env);

}

/*
 * Free headers previously created by http_parse_headers()
 */

void
http_free_headers(HEADERS env)
{
    HEADERS next;

    while(env != NULL) {
	free(env->key);
	free(env->value);
	next = env->next;
	free(env);
	env = next;
    }
}

/*
 * Find the value corresponding to a given key in the headers
 */

char *
http_headers_lookup(HTTP *http, char *key)
{
    HEADERS env = http->headers;


  char *targetLower = malloc(strlen(key)+1);
  char *p = targetLower;
  for( ; *key != '\0'; p = p+1){
    if (isalpha(*key)){
      *p = tolower(*key);
    }
    else{
      *p = *key;
    }
    key++;
  }
  *p = 0;

while(env != NULL) {
  char *keyLower = malloc(strlen(env->key)+1);
  char *pp = env->key;
  for(p = keyLower; *pp; p++){
    *p = tolower(*pp);
    pp++;
  }
  *p = 0;
  //printf("testing: %s, %s\n", targetLower, keyLower);

	if(!strcmp(keyLower, targetLower)){
    free(keyLower);
    free(targetLower);
    return(env->value);
  }
  free(keyLower);
	env = env->next;
  //free(keyLower);
    }
    free(targetLower);
    return(NULL);
}

void
search_headers(HTTP *http, char *target){
  HEADERS node = http->headers;
  char *key;
  char *value;

  char *targetLower = malloc(strlen(target)+1);
  char *p = targetLower;
  for( ; *target != '\0'; p = p+1){
    if (isalpha(*target)){
      *p = tolower(*target);
    }
    else{
      *p = *target;
    }
    target++;
  }
  *p = 0;
  while (node!= NULL){
    key = node->key;
    value = node->value;

    char *valueLower = malloc(strlen(value)+1);
    char *pp = value;
    for(p = valueLower; *pp; p++){
      *p = tolower(*pp);
      pp++;
    }
    *p = 0;
    char *keyLower = malloc(strlen(key)+1);
    pp = key;
    for(p = keyLower; *pp; p++){
      *p = tolower(*pp);
      pp++;
    }
    *p = 0;

    char *keyPointer;
    char *valuePointer;
    if (((keyPointer = strstr(keyLower, targetLower)) != NULL) || ((valuePointer = strstr(valueLower, targetLower)) != NULL)){
      if (keyPointer == NULL){
        keyPointer = valuePointer;
      }
      if (keyPointer != NULL){
        // if the found string starts at the first char in keyLower (valid)
        if (keyPointer == keyLower){
          //if the character after the string is a space, dash, or end of string
          if (*(keyPointer + strlen(targetLower)) == ' ' || *(keyPointer + strlen(targetLower)) == '-' || *(keyPointer + strlen(targetLower)) == 0){
            fprintf(stderr, "%s: %s\n", key, value);
          }
        }
        else{
          // if the first found string starts after a space or dash (valid)
          if (*(keyPointer - 1) == ' ' || *(keyPointer - 1) == '-'){
            //if the character after the string is a space, dash, or end of string
            if (*(keyPointer + strlen(targetLower)) == ' ' || *(keyPointer + strlen(targetLower)) == '-' || *(keyPointer + strlen(targetLower)) == 0){
              fprintf(stderr, "%s: %s\n", key, value);
            }
          }
        }
      }
    }
    free(valueLower);
    free(keyLower);
    node = node->next;
  }
  free(targetLower);

}

