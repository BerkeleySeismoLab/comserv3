/* 
 * PacketQueue class
 *
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#include <string.h>

#include "PacketQueue.h"
#include "Logger.h"

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

PacketQueue::PacketQueue() {
  queueHead = 0;
  queueTail = 0;
  nqueued = 0;
  pthread_mutex_init(&(this->queueLock), NULL);
}

PacketQueue::~PacketQueue() {
  pthread_mutex_destroy(&(this->queueLock));
  return;
}


void PacketQueue::enqueuePacket(char *data, int dataSize, short packetType) {
  pthread_mutex_lock(&(this->queueLock));
  this->queue[this->queueTail].update(data, dataSize, packetType);
  this->advanceTail();
  //g_log << "ENQUEUE: head:" << this->queueHead << " tail:" << this->queueTail << std::endl;
  pthread_mutex_unlock(&(this->queueLock));
}


QueuedPacket PacketQueue::dequeuePacket() {
  pthread_mutex_lock(&(this->queueLock));
  QueuedPacket ret = this->queue[this->queueHead];
  QueuedPacket *ptr = &this->queue[this->queueHead];
  ptr->clear();
  if(ret.dataSize) {
    // don't advance the head if this was empty to begin with
    this->advanceHead();
  }  
  pthread_mutex_unlock(&(this->queueLock));
  //g_log << "DEQUEUE: head:" << this->queueHead << " tail:" << this->queueTail << std::endl;
  return ret;
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
  result = PACKETQUEUE_QUEUE_SIZE - this->nqueued;
  pthread_mutex_unlock(&(this->queueLock));
  return result;
}


void PacketQueue::advanceTail() {

  this->queueTail++;
  if(this->queueTail == PACKETQUEUE_QUEUE_SIZE) {
    this->queueTail = 0;
  }

  if(this->queue[this->queueTail].dataSize != 0) {
    g_log << "XXX Packet queue lapped" << std::endl;
    // reset the head to where the tail is now, since this
    // is the new head
    this->queueHead = this->queueTail;
    this->nqueued = PACKETQUEUE_QUEUE_SIZE;
  }
  else {
    ++this->nqueued;
  }
}

void PacketQueue::advanceHead() {
  this->queueHead++;
  if(this->queueHead == PACKETQUEUE_QUEUE_SIZE) {
    this->queueHead = 0;
  }
   --this->nqueued;
}
  
/**
 * Test code

Logger g_log;

int main(int argc, char *argv[]) {
  PacketQueue pq = PacketQueue();

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
