#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_COUNTREF_HPP__
#define __EDG_WORKLOAD_LOGGING_CLIENT_COUNTREF_HPP__

/**
 * Switching into glite.lb namespace (couple with EWL_END_NAMESPACE).
 */
#define EWL_BEGIN_NAMESPACE namespace glite { namespace lb {

/**
 * Leave the glite.lb namespace.
 */
#define EWL_END_NAMESPACE } }

EWL_BEGIN_NAMESPACE

/** Class implementing simple reference counting mechanism.
 * 
 * This class is used instead of simple pointers to enable sharing of
 * objects using simple reference counting mechanism. It encapsulates
 * the given (pointer to) object and remembers the number of
 * references to it. Taking and getting rid of the reference to
 * encapsulated object is explicit by calling member functions use()
 * and release().
 */
template<typename T>
class CountRef {
public:
	CountRef(void *); 
//	CountRef(void *,void (*)(void *));

	void use(void); 
	void release(void);

	void	*ptr; /**< Pointer to the encapsulated object. */

private:
	int	count;
//	void	(*destroy)(void *);
};

/** 
 * Encapsulate the given object and set reference count to 1.
 *
 */
template <typename T>
CountRef<T>::CountRef(void *p)
{
	ptr = p;
	count = 1;
}

/** Decrease the reference count, possibly deallocating the
 * encapsulated object.
 *
 * This method should be called when the holder no longer plans to use
 * the encapsulated object, instead of deleting it.
 */
template <typename T>
void CountRef<T>::release(void)
{
	if (--count == 0) {
		T::destroyFlesh(ptr);
		delete this;
	}
}

/** Increase the number of references to the object.
 *
 * This method should be called every time the pointer (ie. this
 * instance) is copied.
 */
template <typename T>
void CountRef<T>::use(void)
{
	count++;
}

EWL_END_NAMESPACE

#endif
