#include "klocker.h"

KLocker::KLocker(QReadWriteLock* _lock, Mode mode)
{
  lock       = _lock;
  has_locked = false;
  if (mode == Read)
    has_locked = lock->tryLockForRead();
  else if (mode == Write)
    has_locked = lock->tryLockForWrite();
}

bool KLocker::hasLocked()
{
  return has_locked;
}

KLocker::~KLocker()
{
  if (has_locked)
    lock->unlock();
}
