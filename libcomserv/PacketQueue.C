/* 
 * PacketQueue class
 *
 * 29 Sep 2020 DSN Updated for comserv3.
 * 03 Oct 2022 DSN Updated for runtime configuration of queueSize.
 */

#include <string.h>

#include "PacketQueue.h"
#include "Logger.h"

int DEBUG_PQ = 0;

extern Logger g_log;

QueuedPacket::QueuedPacket(char *packetData, int packetSize, short packetType) {
  this->update(packetData, packetSize, packetType);
}

QueuedPacket::QueuedPacket() {
  this->clear();
}

QueuedPacket::~QueuedPacket() {
  return;
}

void QueuedPacket::update(char *packetData, int packetSize, short packetType) {
  this->dataSize = packetSize;
  this->packetType = packetType;
  memcpy(this->data, packetData, packetSize);
}

void QueuedPacket::clear() {
  this->dataSize = 0;
  memcpy(this->data, "\0", 1);
  this->packetType = 0;
}

/************************************************************/
// Packets queued into the tail packet of the queue.
// Packets dequeued from the head of the queue.
// Packet in the queue is cleared when dequeued.
// A full queue is determined by detecting that the tail packet
// has info (eg datasize of tail packets is not 0).

PacketQueue::PacketQueue(int npackets) {
  queueHead = 0;
  queueTail = 0;
  nqueued = 0;
  queueSize = npackets;
  queue = new QueuedPacket[npackets];
  pthread_mutex_init(&(this->queueLock), NULL);
}

PacketQueue::~PacketQueue() {
  if (this->queue) delete[] queue;
  pthread_mutex_destroy(&(this->queueLock));
  return;
}


void PacketQueue::enqueuePacket(char *data, int dataSize, short packetType) {
  // Ensure that we are enqueueing a proper packet with dataSize > 0.
  if (dataSize <= 0) {
      g_log << "XXX Error: Attempting to enqueue packet with datasize = " << dataSize << std::endl;
    return;
  }
  pthread_mutex_lock(&(this->queueLock));
  // Determine whether we are lapping the queue (ie overwritting a QueuedPacket).
  if(this->queue[this->queueTail].dataSize != 0) {
    if (DEBUG_PQ) {
      g_log << "XXX Packet queue lapped (oldest unread packet lost)" << std::endl;
    }
  }
  this->queue[this->queueTail].update(data, dataSize, packetType);
  this->advanceTail();
  if (DEBUG_PQ) {
    g_log << "ENQUEUE: head:" << this->queueHead << " tail:" << this->queueTail << std::endl;
  }
  pthread_mutex_unlock(&(this->queueLock));
}


QueuedPacket PacketQueue::dequeuePacket() {
  pthread_mutex_lock(&(this->queueLock));
  QueuedPacket ret = this->queue[this->queueHead];
  QueuedPacket *ptr = &this->queue[this->queueHead];
  ptr->clear();
  // We ALWAYS return a packet to the caller.
  // If the queue was empty, the returned packet's datasize is 0.
  // Don't advance the head if the queue was empty, because we are
  // not returning a real packet.
  if(ret.dataSize) {
    this->advanceHead();
  }  
  if (DEBUG_PQ) {
    g_log << "DEQUEUE: head:" << this->queueHead << " tail:" << this->queueTail << std::endl;
  }
  pthread_mutex_unlock(&(this->queueLock));
  return ret;
}


int PacketQueue::maxPackets() {
  int result;
  result = queueSize;
  return result;
}


int PacketQueue::numQueued() {
  int result;
  pthread_mutex_lock(&(this->queueLock));
  result = this->nqueued;
  pthread_mutex_unlock(&(this->queueLock));
  return result;
}


int PacketQueue::numFree() {
  int result;
  pthread_mutex_lock(&(this->queueLock));
  result = this->queueSize - this->nqueued;
  pthread_mutex_unlock(&(this->queueLock));
  return result;
}


void PacketQueue::advanceTail() {

  this->queueTail++;
  if(this->queueTail == this->queueSize) {
    this->queueTail = 0;
  }

  // Determine whether the queue is full.
  // If so, ensure queuehead points to the new queuetail.
  if(this->queue[this->queueTail].dataSize != 0) {
    // The queue is full.
    // Reset qheadhead to the new value of queuetail.
    // The queueHead will be the index of the oldest packet in the queue.
    //
    if (DEBUG_PQ) {
      g_log << "XXX Packet queue full" << std::endl;
    }
    this->queueHead = this->queueTail;
    this->nqueued = this->queueSize;
  }
  else {
    ++this->nqueued;
  }
}

void PacketQueue::advanceHead() {
  this->queueHead++;
  if(this->queueHead == this->queueSize) {
    this->queueHead = 0;
  }
  --this->nqueued;
}
  
/**
 * Test code

Logger g_log;

int main(int argc, char *argv[]) {
  PacketQueue pq = PacketQueue(100);

  g_log.logToStdout(true);
  g_log.logToFile(false);

  printf("NumQueued: %d  NumFree: %d\n", pq.numQueued(), pq.numFree());
  printf ("===> Start queueing\n");
  for(int i = 0; i < 129; i++) {
    pq.enqueuePacket((char *)"packet1", strlen("packet1")+1, i);
    printf("Queued: %d\n", i);
    printf("NumQueued: %d  NumFree: %d\n", pq.numQueued(), pq.numFree());
  }
  printf ("===> End queueing\n");
  
  printf ("===> Start dequeueing\n");
  printf("NumQueued: %d  NumFree: %d\n", pq.numQueued(), pq.numFree());

  QueuedPacket thisPacket = pq.dequeuePacket();


  while(thisPacket.dataSize != 0) {
    printf("Dequeued: %d\n", thisPacket.packetType);
    printf("NumQueued: %d  NumFree: %d\n", pq.numQueued(), pq.numFree());
    thisPacket = pq.dequeuePacket();
  }
  printf ("===> End dequeueing\n");
}
*/
