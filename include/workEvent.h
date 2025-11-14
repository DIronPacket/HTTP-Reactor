#ifndef WORKEVENT_H
#define WORKEVENT_H
#include <vector>
#include <thread>
#include <string>
#include "myserver.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "customConnector.h"
#include "data.h"
#include "utils.h"
// #include "yaml-cpp/yaml.h"
#include <sys/stat.h>

class MyServer;
class CustomConnector;
class HttpRequest;
class HttpResponse;

int parseFileSaveLocal(HttpResponse *httpResponse,HttpRequest *httpRequest, std::shared_ptr<ReadWriteLock> io_mutex);

#endif