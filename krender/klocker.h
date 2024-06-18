#ifndef KLOCKER_H
#define KLOCKER_H

#include <QReadWriteLock>

class KLocker
{
  QReadWriteLock* lock;

public:
  enum Mode
  {
    Read,
    Write
  };
  bool has_locked;
  KLocker(QReadWriteLock* lock, Mode);
  bool hasLocked();
  virtual ~KLocker();
};

#endif  // KLOCKER_H
