#ifndef __PACKETQUEUE_H__
#define __PACKETQUEUE_H__

/*
 * 29 Sep 2020 DSN Updated for comserv3.
 * 03 Oct 2022 DSN Updated for runtime configuration of queueSize.
 */

#include <pthread.h>

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
  PacketQueue(int n);
  ~PacketQueue();
  void enqueuePacket(char *, int, short);
  QueuedPacket dequeuePacket();
  int maxPackets();
  int numQueued();
  int numFree();
 private:
  void advanceHead();
  void advanceTail();
  QueuedPacket *queue;
  int queueSize;
  int queueHead;
  int queueTail;
  pthread_mutex_t queueLock;
  int nqueued;
};

#endif
