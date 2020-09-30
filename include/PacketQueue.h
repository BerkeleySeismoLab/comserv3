#ifndef __PACKETQUEUE_H__
#define __PACKETQUEUE_H__

/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#include <pthread.h>

const int PACKETQUEUE_QUEUE_SIZE = 500;

class QueuedPacket {
 public:
  QueuedPacket(char *, int, short);
  QueuedPacket();
  ~QueuedPacket();

  void update(char *, int, short);
  void clear();

  char data[512];
  int dataSize;
  short packetType;
};

class PacketQueue {
 public:
  PacketQueue();
  ~PacketQueue();
  void enqueuePacket(char *, int, short);
  QueuedPacket dequeuePacket();
  int numQueued();
  int numFree();
 private:
  void advanceHead();
  void advanceTail();
  QueuedPacket queue[PACKETQUEUE_QUEUE_SIZE];
  int queueHead;
  int queueTail;
  pthread_mutex_t queueLock;
  int nqueued;
};

#endif
