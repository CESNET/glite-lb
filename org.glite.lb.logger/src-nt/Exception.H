#ifndef _EXCEPTION_H
#define _EXCEPTION_H

class Exception {
};

class FatalException {
};


#define E_ASSERT(a) if(!(a)) { throw new FatalException; }

#endif
